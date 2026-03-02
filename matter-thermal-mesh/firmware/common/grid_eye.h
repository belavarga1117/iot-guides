/*
 * grid_eye.h — AMG8833 Grid-EYE 8×8 Infrared Thermal Sensor Driver
 *
 * I2C driver for SparkFun Grid-EYE Qwiic breakout.
 * Addresses: 0x68 (AD_SELECT=LOW), 0x69 (AD_SELECT=HIGH)
 *
 * IMPORTANT: SparkFun Qwiic Grid-EYE breakout has AD_SELECT pulled HIGH
 * by default, so the address is 0x69, NOT the AMG8833 datasheet default 0x68.
 * This was verified on multiple boards via I2C data-write scan (the SiLabs
 * Wire library's address-only probe always returns 0 due to a bug).
 *
 * Returns 64 temperature readings in °C.
 */

#ifndef GRID_EYE_H
#define GRID_EYE_H

#include <Arduino.h>
#include <Wire.h>

// I2C addresses
#define GRIDEYE_ADDR_DEFAULT 0x68
#define GRIDEYE_ADDR_ALT     0x69

// AMG8833 registers
#define GRIDEYE_REG_POWER_CONTROL  0x00
#define GRIDEYE_REG_RESET          0x01
#define GRIDEYE_REG_FRAMERATE      0x02
#define GRIDEYE_REG_INT_CONTROL    0x03
#define GRIDEYE_REG_STATUS         0x04
#define GRIDEYE_REG_STATUS_CLEAR   0x05
#define GRIDEYE_REG_THERMISTOR_L   0x0E
#define GRIDEYE_REG_THERMISTOR_H   0x0F
#define GRIDEYE_REG_PIXEL_START    0x80

// Power modes
#define GRIDEYE_NORMAL_MODE  0x00
#define GRIDEYE_SLEEP_MODE   0x10
#define GRIDEYE_STANDBY_60   0x20
#define GRIDEYE_STANDBY_10   0x21

// Frame rates
#define GRIDEYE_FPS_10  0x00
#define GRIDEYE_FPS_1   0x01

// Reset commands
#define GRIDEYE_FLAG_RESET    0x30
#define GRIDEYE_INITIAL_RESET 0x3F

#define GRIDEYE_PIXEL_COUNT 64

struct GridEyeFrame {
  float pixels[GRIDEYE_PIXEL_COUNT];  // 8×8 temperatures in °C
  float thermistor;                    // On-chip thermistor (ambient) in °C
  float maxTemp;                       // Hottest pixel
  float minTemp;                       // Coldest pixel
  float avgTemp;                       // Average across all pixels
};

class GridEye {
public:
  GridEye(uint8_t address = GRIDEYE_ADDR_DEFAULT, TwoWire* wire = &Wire);

  bool begin();
  bool isConnected();

  // Read full 8×8 frame
  bool readFrame(GridEyeFrame& frame);

  // Read single pixel (0-63)
  float readPixel(uint8_t index);

  // Read on-chip thermistor (ambient temperature)
  float readThermistor();

  // Configuration
  void setFrameRate(uint8_t fps);  // GRIDEYE_FPS_10 or GRIDEYE_FPS_1
  void wake();
  void sleep();
  void reset();

  uint8_t getAddress() const { return _address; }

private:
  uint8_t _address;
  TwoWire* _wire;

  uint8_t readRegister8(uint8_t reg);
  uint16_t readRegister16(uint8_t reg);
  void writeRegister8(uint8_t reg, uint8_t value);
  float convertPixelRaw(uint16_t raw);
  float convertThermistorRaw(uint16_t raw);
};

#endif // GRID_EYE_H
