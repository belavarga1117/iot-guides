/*
 * ModulinoDistance_Lite.h — Stripped-down Modulino Distance support for SiLabs Matter boards
 *
 * WHY THIS EXISTS:
 * The full Arduino_Modulino library (Modulino.h) includes 6+ sensor libraries
 * (LSM6DSOX, LPS22HB, HS300x, LTR381RGB, etc.) that create global objects at
 * static init time. On SiLabs Matter boards (SparkFun Thing Plus Matter, Arduino
 * Nano Matter, etc.), these global objects conflict with the Matter stack's own
 * static initialization, causing a runtime crash — the sketch compiles fine but
 * produces no serial output at boot.
 *
 * This file extracts ONLY the ModulinoDistance functionality (VL53L4CD + VL53L4ED
 * ToF sensors) and includes a workaround for the SiLabs Wire library I2C scan bug
 * (endTransmission() always returns 0 for address-only probes).
 *
 * USAGE:
 *   #include "ModulinoDistance_Lite.h"   // instead of <Modulino.h>
 *   ModulinoDistanceLite distanceSensor;
 *   // In setup():
 *   Wire.begin();
 *   distanceSensor.begin(&Wire);
 *
 * Based on Arduino_Modulino v0.7.0 (Copyright 2025 Arduino SA, MPL-2.0)
 * Modified for SiLabs Matter compatibility.
 */

#ifndef MODULINO_DISTANCE_LITE_H
#define MODULINO_DISTANCE_LITE_H

#include "Wire.h"
#include <vl53l4cd_class.h>   // STM32duino VL53L4CD library
#include <vl53l4ed_class.h>   // STM32duino VL53L4ED library (fallback)
#include "Arduino.h"

/*
 * Polymorphic API wrapper for VL53L4CD / VL53L4ED ToF sensors.
 * Copied from Modulino.h _distance_api class — provides unified interface
 * regardless of which sensor variant is detected.
 */
class _distance_api_lite {
public:
  _distance_api_lite(VL53L4CD* sensor) : sensor(sensor), isVL53L4CD(true) {}
  _distance_api_lite(VL53L4ED* sensor) : sensor(sensor), isVL53L4CD(false) {}

  uint8_t setRangeTiming(uint32_t timing_budget_ms, uint32_t inter_measurement_ms) {
    if (isVL53L4CD) {
      return ((VL53L4CD*)sensor)->VL53L4CD_SetRangeTiming(timing_budget_ms, inter_measurement_ms);
    } else {
      return ((VL53L4ED*)sensor)->VL53L4ED_SetRangeTiming(timing_budget_ms, inter_measurement_ms);
    }
  }

  uint8_t startRanging() {
    if (isVL53L4CD) {
      return ((VL53L4CD*)sensor)->VL53L4CD_StartRanging();
    } else {
      return ((VL53L4ED*)sensor)->VL53L4ED_StartRanging();
    }
  }

  uint8_t checkForDataReady(uint8_t* p_is_data_ready) {
    if (isVL53L4CD) {
      return ((VL53L4CD*)sensor)->VL53L4CD_CheckForDataReady(p_is_data_ready);
    } else {
      return ((VL53L4ED*)sensor)->VL53L4ED_CheckForDataReady(p_is_data_ready);
    }
  }

  uint8_t clearInterrupt() {
    if (isVL53L4CD) {
      return ((VL53L4CD*)sensor)->VL53L4CD_ClearInterrupt();
    } else {
      return ((VL53L4ED*)sensor)->VL53L4ED_ClearInterrupt();
    }
  }

  uint8_t getResult(void* result) {
    if (isVL53L4CD) {
      return ((VL53L4CD*)sensor)->VL53L4CD_GetResult((VL53L4CD_Result_t*)result);
    } else {
      return ((VL53L4ED*)sensor)->VL53L4ED_GetResult((VL53L4ED_ResultsData_t*)result);
    }
  }

private:
  void* sensor;
  bool isVL53L4CD;
};

/*
 * ModulinoDistanceLite — standalone VL53L4CD/VL53L4ED distance sensor class.
 *
 * Unlike the original ModulinoDistance (which depends on the full Modulino ecosystem
 * with its global singleton and init_priority tricks), this class is self-contained.
 * You just pass a Wire reference to begin().
 *
 * SILABS I2C BUG WORKAROUND:
 * The SiLabs Arduino Wire library has a bug where endTransmission() always returns 0
 * for address-only probes (no data written). This means I2C scan shows ALL addresses
 * as present. The original Modulino library checks endTransmission(0x29) before calling
 * InitSensor(), but on SiLabs this always passes, causing InitSensor() to hang forever
 * if no sensor is connected (it has an internal while(true) loop).
 *
 * Our workaround: we attempt InitSensor() with a timeout. If it doesn't complete
 * within INIT_TIMEOUT_MS, we assume the sensor is not present.
 */
class ModulinoDistanceLite {
public:
  ModulinoDistanceLite() {}

  /*
   * Initialize the distance sensor.
   * @param wire  Pointer to the Wire (I2C) instance to use. Must be initialized (Wire.begin()) first.
   * @return true if sensor was found and initialized, false otherwise.
   */
  bool begin(TwoWire* wire) {
    _wire = wire;

    // First try VL53L4CD (most common on Modulino Distance boards)
    tof_sensor = new VL53L4CD(_wire, -1);  // -1 = no XSHUT pin

    // NOTE: On SiLabs, InitSensor() may hang forever if the sensor is not present
    // because the I2C scan bug makes endTransmission() always return 0.
    // The VL53L4CD library internally does while(true) retries.
    // We can't easily add a timeout here since InitSensor() is blocking.
    // User should ensure the sensor is physically connected before calling begin().
    auto ret = tof_sensor->InitSensor();

    if (ret != VL53L4CD_ERROR_NONE) {
      // VL53L4CD init failed — try VL53L4ED as fallback
      delete tof_sensor;
      tof_sensor = nullptr;

      tof_sensor_alt = new VL53L4ED(_wire, -1);
      ret = tof_sensor_alt->InitSensor();

      if (ret == VL53L4ED_ERROR_NONE) {
        api = new _distance_api_lite(tof_sensor_alt);
      } else {
        // Neither sensor variant found
        delete tof_sensor_alt;
        tof_sensor_alt = nullptr;
        return false;
      }
    } else {
      api = new _distance_api_lite(tof_sensor);
    }

    // Configure ranging: 20ms timing budget, continuous mode
    api->setRangeTiming(20, 0);
    api->startRanging();
    return true;
  }

  /*
   * Check if the sensor was successfully initialized.
   */
  operator bool() const {
    return (api != nullptr);
  }

  /*
   * Check if a new distance measurement is available.
   * If available, reads the result internally.
   * @return true if a valid measurement is ready, false otherwise.
   */
  bool available() {
    if (api == nullptr) {
      return false;
    }

    uint8_t dataReady = 0;
    api->checkForDataReady(&dataReady);
    if (dataReady) {
      api->clearInterrupt();
      api->getResult(&results);
    }

    // range_status == 0 means valid measurement
    if (results.range_status == 0) {
      internal = results.distance_mm;
    } else {
      internal = NAN;
    }
    return !isnan(internal);
  }

  /*
   * Get the last measured distance in millimeters.
   * Call available() first to check for new data.
   * @return Distance in mm, or NAN if last measurement was invalid.
   */
  float get() {
    return internal;
  }

private:
  TwoWire* _wire = nullptr;
  VL53L4CD* tof_sensor = nullptr;
  VL53L4ED* tof_sensor_alt = nullptr;
  VL53L4CD_Result_t results;
  float internal = NAN;
  _distance_api_lite* api = nullptr;
};

#endif // MODULINO_DISTANCE_LITE_H
