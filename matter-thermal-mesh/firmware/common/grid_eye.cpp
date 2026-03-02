/*
 * grid_eye.cpp — AMG8833 Grid-EYE I2C Driver Implementation
 */

#include "grid_eye.h"

GridEye::GridEye(uint8_t address, TwoWire* wire)
    : _address(address), _wire(wire) {}

bool GridEye::begin() {
  _wire->begin();
  reset();
  delay(100);
  wake();
  setFrameRate(GRIDEYE_FPS_10);
  delay(50);
  return isConnected();
}

bool GridEye::isConnected() {
  _wire->beginTransmission(_address);
  return (_wire->endTransmission() == 0);
}

bool GridEye::readFrame(GridEyeFrame& frame) {
  if (!isConnected()) return false;

  frame.maxTemp = -999.0f;
  frame.minTemp = 999.0f;
  float sum = 0.0f;

  // Read 64 pixels (128 bytes, 2 bytes per pixel starting at 0x80)
  for (uint8_t i = 0; i < GRIDEYE_PIXEL_COUNT; i++) {
    uint8_t reg = GRIDEYE_REG_PIXEL_START + (i * 2);
    uint16_t raw = readRegister16(reg);
    float temp = convertPixelRaw(raw);

    frame.pixels[i] = temp;
    sum += temp;

    if (temp > frame.maxTemp) frame.maxTemp = temp;
    if (temp < frame.minTemp) frame.minTemp = temp;
  }

  frame.avgTemp = sum / GRIDEYE_PIXEL_COUNT;
  frame.thermistor = readThermistor();

  return true;
}

float GridEye::readPixel(uint8_t index) {
  if (index >= GRIDEYE_PIXEL_COUNT) return 0.0f;
  uint8_t reg = GRIDEYE_REG_PIXEL_START + (index * 2);
  uint16_t raw = readRegister16(reg);
  return convertPixelRaw(raw);
}

float GridEye::readThermistor() {
  uint16_t raw = readRegister16(GRIDEYE_REG_THERMISTOR_L);
  return convertThermistorRaw(raw);
}

void GridEye::setFrameRate(uint8_t fps) {
  writeRegister8(GRIDEYE_REG_FRAMERATE, fps);
}

void GridEye::wake() {
  writeRegister8(GRIDEYE_REG_POWER_CONTROL, GRIDEYE_NORMAL_MODE);
}

void GridEye::sleep() {
  writeRegister8(GRIDEYE_REG_POWER_CONTROL, GRIDEYE_SLEEP_MODE);
}

void GridEye::reset() {
  writeRegister8(GRIDEYE_REG_RESET, GRIDEYE_INITIAL_RESET);
}

// --- Private helpers ---

uint8_t GridEye::readRegister8(uint8_t reg) {
  _wire->beginTransmission(_address);
  _wire->write(reg);
  _wire->endTransmission(false);
  _wire->requestFrom(_address, (uint8_t)1);
  return _wire->read();
}

uint16_t GridEye::readRegister16(uint8_t reg) {
  _wire->beginTransmission(_address);
  _wire->write(reg);
  _wire->endTransmission(false);
  _wire->requestFrom(_address, (uint8_t)2);
  uint8_t low = _wire->read();
  uint8_t high = _wire->read();
  return (uint16_t)high << 8 | low;
}

void GridEye::writeRegister8(uint8_t reg, uint8_t value) {
  _wire->beginTransmission(_address);
  _wire->write(reg);
  _wire->write(value);
  _wire->endTransmission();
}

// Pixel: 12-bit signed, LSB = 0.25°C
// Bits 11-0 = value, bit 11 = sign
float GridEye::convertPixelRaw(uint16_t raw) {
  raw &= 0x0FFF;  // Mask to 12 bits
  if (raw & 0x0800) {
    // Negative: two's complement
    return -((~raw & 0x07FF) + 1) * 0.25f;
  }
  return raw * 0.25f;
}

// Thermistor: 12-bit signed, LSB = 0.0625°C
float GridEye::convertThermistorRaw(uint16_t raw) {
  raw &= 0x0FFF;
  if (raw & 0x0800) {
    return -((~raw & 0x07FF) + 1) * 0.0625f;
  }
  return raw * 0.0625f;
}
