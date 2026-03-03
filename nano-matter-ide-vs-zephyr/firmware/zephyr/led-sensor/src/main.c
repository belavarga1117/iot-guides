/*
 * Zephyr RTOS: LED + Grid-EYE + BLE Demo (The "After")
 *
 * Three independent threads run concurrently:
 *   Thread 1 (priority 5): LED animation — PWM breathing or GPIO blink
 *   Thread 2 (priority 7): Grid-EYE sensor polling via sensor subsystem
 *   Thread 3 (priority 9): BLE advertising with max temperature
 *
 * The sensor thread reads the Grid-EYE 3 times per poll (~39ms of I2C).
 * This is the SAME workload as the Arduino demo — but here, the I2C reads
 * run on Thread 2's time slice while Thread 1 keeps animating the LED.
 * Zero stutter — the kernel scheduler handles the concurrency.
 *
 * This SAME code compiles for:
 *   west build -b arduino_nano_matter   (EFR32/MGM240S — Thread/BLE SoC)
 *   west build -b siwx917_dk2605a       (SiWx917 — Wi-Fi 6/BLE SoC)
 *
 * Hardware: Board + Grid-EYE AMG8833 via Qwiic (I2C address 0x69)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* BLE headers */
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>

/* PWM header — only used if the board has PWM LEDs */
#if DT_NODE_HAS_STATUS(DT_ALIAS(pwm_led2), okay)
#include <zephyr/drivers/pwm.h>
#endif

/* ── Configuration ── */

/* Thread stack sizes */
#define LED_STACK_SIZE    1024
#define SENSOR_STACK_SIZE 2048
#define BLE_STACK_SIZE    2048

/* Thread priorities (lower number = higher priority) */
#define LED_PRIORITY    5   /* Highest: LED must never stutter */
#define SENSOR_PRIORITY 7   /* Medium: sensor polling */
#define BLE_PRIORITY    9   /* Lowest: BLE advertising */

/* Timing intervals */
#define LED_TICK_MS     10   /* 100 Hz LED update rate */
#define SENSOR_POLL_MS  100  /* 10 Hz sensor polling (same rate as Arduino) */
#define BLE_UPDATE_MS   1000 /* 1 Hz BLE data update */

/* ── Shared State ── */

/* Mutex protects max_temp from concurrent access */
static struct k_mutex temp_mutex;

/* Latest max temperature from Grid-EYE (shared between sensor + BLE threads) */
static float max_temp;

/* Flag: sensor has provided at least one reading */
static bool sensor_ready;

/* ── LED Thread ── */

/*
 * Board-adaptive LED control:
 * - Arduino Nano Matter has PWM LEDs (smooth breathing animation)
 * - SiWx917 DK has GPIO-only LEDs (blink pattern)
 * The code adapts at compile time using devicetree macros.
 */
#if DT_NODE_HAS_STATUS(DT_ALIAS(pwm_led2), okay)
/* PWM LED available — smooth breathing */
static const struct pwm_dt_spec blue_pwm_led =
	PWM_DT_SPEC_GET(DT_ALIAS(pwm_led2));
#define HAS_PWM_LED 1
#else
/* GPIO fallback — simple blink */
static const struct gpio_dt_spec led0_gpio =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#define HAS_PWM_LED 0
#endif

/*
 * LED animation thread.
 * Runs every LED_TICK_MS (10ms) = 100Hz update rate.
 * The kernel scheduler ensures this thread is never starved by sensor I/O.
 */
static void led_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("[LED] Thread started (priority %d, tick %d ms)\n",
	       LED_PRIORITY, LED_TICK_MS);

#if HAS_PWM_LED
	/* PWM breathing: ramp brightness up and down smoothly */
	if (!pwm_is_ready_dt(&blue_pwm_led)) {
		printk("[LED] ERROR: PWM device not ready\n");
		return;
	}
	printk("[LED] PWM mode — smooth breathing on blue LED\n");

	uint32_t period = blue_pwm_led.period;
	uint32_t pulse = 0;
	int32_t step = period / 128; /* 128 brightness steps per ramp */

	while (1) {
		uint32_t tick_start = k_uptime_get_32();

		/* Set PWM duty cycle (pulse width controls brightness) */
		pwm_set_pulse_dt(&blue_pwm_led, pulse);

		/*
		 * Advance brightness using int64_t to avoid unsigned wrap.
		 * Without this, pulse (uint32_t) + step (negative int32_t) can
		 * underflow to a huge value before the boundary check catches it.
		 */
		int64_t next_pulse = (int64_t)pulse + step;
		if (next_pulse >= (int64_t)period) {
			next_pulse = period;
			step = -step;
		} else if (next_pulse <= 0) {
			next_pulse = 0;
			step = -step;
		}
		pulse = (uint32_t)next_pulse;

		/* Log timing every 500 ticks (~5 seconds) to prove consistency */
		static int tick_count;
		tick_count++;
		if (tick_count % 500 == 0) {
			uint32_t elapsed = k_uptime_get_32() - tick_start;
			printk("[LED] tick #%d: %u ms (target: <%d ms)\n",
			       tick_count, elapsed, LED_TICK_MS);
		}

		k_sleep(K_MSEC(LED_TICK_MS));
	}
#else
	/* GPIO blink fallback for boards without PWM */
	if (!gpio_is_ready_dt(&led0_gpio)) {
		printk("[LED] ERROR: GPIO device not ready\n");
		return;
	}
	gpio_pin_configure_dt(&led0_gpio, GPIO_OUTPUT_ACTIVE);
	printk("[LED] GPIO mode — blink pattern on LED0\n");

	while (1) {
		gpio_pin_toggle_dt(&led0_gpio);
		k_sleep(K_MSEC(LED_TICK_MS * 25)); /* 250ms blink for visibility */
	}
#endif
}

/* ── Sensor Thread ── */

/*
 * Sensor polling thread.
 * Uses Zephyr sensor subsystem: sensor_sample_fetch() + sensor_channel_get().
 * The AMG88xx driver is configured via devicetree overlay — no hardcoded addresses.
 * This I2C read blocks THIS thread for ~8ms, but the LED thread keeps running.
 */
static void sensor_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("[SENSOR] Thread started (priority %d, poll %d ms)\n",
	       SENSOR_PRIORITY, SENSOR_POLL_MS);

	/* Get AMG88xx device from devicetree — no hardcoded I2C address */
	const struct device *amg = DEVICE_DT_GET_ONE(panasonic_amg88xx);

	if (!device_is_ready(amg)) {
		printk("[SENSOR] ERROR: AMG88xx device not ready\n");
		return;
	}
	printk("[SENSOR] AMG88xx initialized via devicetree\n");

	/* 64 temperature values — one per pixel in the 8×8 grid */
	struct sensor_value temps[64];

	while (1) {
		uint32_t read_start = k_uptime_get_32();

		/*
		 * Fetch all 64 pixel temperatures from the sensor.
		 * Read 3 times — same I2C workload as the Arduino demo.
		 * Despite 3 × 13ms = ~39ms of I2C blocking on THIS thread,
		 * the LED thread keeps breathing smoothly. That's the point.
		 */
		int ret = 0;

		for (int r = 0; r < 3; r++) {
			ret = sensor_sample_fetch(amg);
			if (ret) {
				printk("[SENSOR] Fetch error on read %d: %d\n",
				       r, ret);
				break;
			}
		}

		if (ret) {
			k_sleep(K_MSEC(SENSOR_POLL_MS));
			continue;
		}

		/* Convert raw data to sensor_value structs (uses last fetch) */
		ret = sensor_channel_get(amg, SENSOR_CHAN_AMBIENT_TEMP, temps);
		if (ret) {
			printk("[SENSOR] Channel get error: %d\n", ret);
			k_sleep(K_MSEC(SENSOR_POLL_MS));
			continue;
		}

		/* Find hottest pixel across all 64 */
		float local_max = (float)temps[0].val1 +
				  (float)temps[0].val2 / 1000000.0f;
		for (int i = 1; i < 64; i++) {
			float t = (float)temps[i].val1 +
				  (float)temps[i].val2 / 1000000.0f;
			if (t > local_max) {
				local_max = t;
			}
		}

		/* Update shared state — mutex protects from BLE thread */
		k_mutex_lock(&temp_mutex, K_FOREVER);
		max_temp = local_max;
		sensor_ready = true;
		k_mutex_unlock(&temp_mutex);

		uint32_t read_elapsed = k_uptime_get_32() - read_start;
		/* Print integer and fractional parts separately (no float printf) */
		int temp_int = (int)local_max;
		int temp_frac = (int)((local_max - temp_int) * 100);
		if (temp_frac < 0) {
			temp_frac = -temp_frac;
		}
		printk("[SENSOR] 3x read took %u ms | Max: %d.%02d C\n",
		       read_elapsed, temp_int, temp_frac);

		k_sleep(K_MSEC(SENSOR_POLL_MS));
	}
}

/* ── BLE Thread ── */

/*
 * BLE advertising thread.
 * Broadcasts the latest max temperature as manufacturer-specific data.
 * Any BLE scanner (Simplicity Connect, nRF Connect) can see "ZephyrGridEye"
 * and read the temperature from the manufacturer data field.
 */

/* Manufacturer data: company ID (0xFFFF = test) + temperature × 100 */
static uint8_t mfg_data[] = {
	0xFF, 0xFF,  /* Company ID: 0xFFFF (reserved for testing) */
	0x00, 0x00,  /* Temperature × 100, big-endian int16 */
};

/* Advertising data array — updated in-place when temperature changes */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS,
		      (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
};

/* Scan response with device name */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, "ZephyrGridEye",
		sizeof("ZephyrGridEye") - 1),
};

static void ble_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("[BLE] Thread started (priority %d, update %d ms)\n",
	       BLE_PRIORITY, BLE_UPDATE_MS);

	/* Initialize Bluetooth subsystem */
	int err = bt_enable(NULL);

	if (err) {
		printk("[BLE] Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("[BLE] Bluetooth initialized\n");

	/* Start advertising — non-connectable, just broadcasting data */
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("[BLE] Advertising start failed (err %d)\n", err);
		return;
	}
	printk("[BLE] Advertising as 'ZephyrGridEye'\n");

	while (1) {
		/* Read latest temperature from shared state */
		float temp;
		bool ready;

		k_mutex_lock(&temp_mutex, K_FOREVER);
		ready = sensor_ready;
		temp = max_temp;

		/*
		 * Encode temperature × 100 as big-endian int16 INSIDE the mutex.
		 * mfg_data[] is referenced by the BT stack via the ad[] pointer —
		 * the two-byte write must be atomic w.r.t. bt_le_adv_update_data()
		 * to avoid a torn read (one byte old, one byte new).
		 */
		int16_t temp_x100 = (int16_t)(temp * 100);
		mfg_data[2] = (uint8_t)(temp_x100 >> 8);
		mfg_data[3] = (uint8_t)(temp_x100 & 0xFF);
		k_mutex_unlock(&temp_mutex);

		/* Wait for first valid sensor reading before advertising */
		if (!ready) {
			printk("[BLE] Waiting for first sensor reading...\n");
			k_sleep(K_MSEC(BLE_UPDATE_MS));
			continue;
		}

		/* Update advertising data with new temperature */
		err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad),
					    sd, ARRAY_SIZE(sd));
		if (err) {
			printk("[BLE] Adv data update error: %d\n", err);
		}

		int temp_int = (int)temp;
		int temp_frac = (int)((temp - temp_int) * 100);
		if (temp_frac < 0) {
			temp_frac = -temp_frac;
		}
		printk("[BLE] Broadcasting temp: %d.%02d C\n",
		       temp_int, temp_frac);

		k_sleep(K_MSEC(BLE_UPDATE_MS));
	}
}

/* ── Thread Definitions ── */

/*
 * K_THREAD_DEFINE creates threads at compile time.
 * They start automatically when the kernel boots — no manual thread_create().
 * Each thread has its own stack, priority, and entry function.
 */
K_THREAD_DEFINE(led_tid, LED_STACK_SIZE,
		led_thread_fn, NULL, NULL, NULL,
		LED_PRIORITY, 0, 0);

K_THREAD_DEFINE(sensor_tid, SENSOR_STACK_SIZE,
		sensor_thread_fn, NULL, NULL, NULL,
		SENSOR_PRIORITY, 0, 0);

K_THREAD_DEFINE(ble_tid, BLE_STACK_SIZE,
		ble_thread_fn, NULL, NULL, NULL,
		BLE_PRIORITY, 0, 0);

/* ── Main ── */

int main(void)
{
	/* Initialize the temperature mutex */
	k_mutex_init(&temp_mutex);

	printk("\n");
	printk("=============================================\n");
	printk("  Zephyr LED + Grid-EYE + BLE Demo\n");
	printk("=============================================\n");
	printk("Board: %s\n", CONFIG_BOARD);
	printk("Threads:\n");
	printk("  LED    (priority %d) — %s\n", LED_PRIORITY,
	       HAS_PWM_LED ? "PWM breathing" : "GPIO blink");
	printk("  Sensor (priority %d) — Grid-EYE 3x read every %d ms\n",
	       SENSOR_PRIORITY, SENSOR_POLL_MS);
	printk("  BLE    (priority %d) — advertise every %d ms\n",
	       BLE_PRIORITY, BLE_UPDATE_MS);
	printk("=============================================\n\n");

	/* Threads are auto-started by K_THREAD_DEFINE — main can return */
	return 0;
}
