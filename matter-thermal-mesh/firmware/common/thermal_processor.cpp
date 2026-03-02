/*
 * thermal_processor.cpp — Presence Detection Implementation
 */

#include "thermal_processor.h"

ThermalProcessor::ThermalProcessor()
    : _absThreshold(DEFAULT_PRESENCE_THRESHOLD),
      _deltaThreshold(DEFAULT_DELTA_THRESHOLD) {}

void ThermalProcessor::setAbsoluteThreshold(float tempC) {
  _absThreshold = tempC;
}

void ThermalProcessor::setDeltaThreshold(float deltaC) {
  _deltaThreshold = deltaC;
}

PresenceResult ThermalProcessor::process(const GridEyeFrame& frame) {
  PresenceResult result;
  result.occupied = false;
  result.personCount = 0;
  result.maxTemp = frame.maxTemp;
  result.ambientTemp = frame.thermistor;

  // Dynamic threshold: ambient + delta OR absolute, whichever is lower
  float dynamicThreshold = frame.thermistor + _deltaThreshold;
  float threshold = min(dynamicThreshold, _absThreshold);

  // Mark pixels above threshold
  bool hotPixels[GRIDEYE_PIXEL_COUNT];
  for (uint8_t i = 0; i < GRIDEYE_PIXEL_COUNT; i++) {
    hotPixels[i] = (frame.pixels[i] >= threshold);
  }

  // Find blobs via flood fill
  result.personCount = findBlobs(hotPixels, frame.pixels, result.blobs);
  result.occupied = (result.personCount > 0);

  return result;
}

uint8_t ThermalProcessor::findBlobs(const bool occupied[64],
                                     const float pixels[64],
                                     Blob blobs[MAX_BLOBS]) {
  bool visited[GRIDEYE_PIXEL_COUNT] = {false};
  uint8_t blobCount = 0;

  for (uint8_t y = 0; y < 8 && blobCount < MAX_BLOBS; y++) {
    for (uint8_t x = 0; x < 8 && blobCount < MAX_BLOBS; x++) {
      uint8_t idx = y * 8 + x;
      if (occupied[idx] && !visited[idx]) {
        uint8_t count = 0;
        float peakTemp = 0.0f;
        float sumX = 0.0f, sumY = 0.0f;

        floodFill(occupied, visited, x, y, &count, &peakTemp,
                  &sumX, &sumY, pixels);

        // Minimum 2 pixels to count as a person (noise filter)
        if (count >= 2) {
          blobs[blobCount].centerX = (uint8_t)(sumX / count);
          blobs[blobCount].centerY = (uint8_t)(sumY / count);
          blobs[blobCount].peakTemp = peakTemp;
          blobs[blobCount].pixelCount = count;
          blobCount++;
        }
      }
    }
  }

  return blobCount;
}

void ThermalProcessor::floodFill(const bool occupied[64], bool visited[64],
                                  uint8_t x, uint8_t y,
                                  uint8_t* count, float* peakTemp,
                                  float* sumX, float* sumY,
                                  const float pixels[64]) {
  if (x >= 8 || y >= 8) return;
  uint8_t idx = y * 8 + x;
  if (!occupied[idx] || visited[idx]) return;

  visited[idx] = true;
  (*count)++;
  *sumX += x;
  *sumY += y;
  if (pixels[idx] > *peakTemp) *peakTemp = pixels[idx];

  // 4-connected neighbors
  if (x > 0) floodFill(occupied, visited, x - 1, y, count, peakTemp, sumX, sumY, pixels);
  if (x < 7) floodFill(occupied, visited, x + 1, y, count, peakTemp, sumX, sumY, pixels);
  if (y > 0) floodFill(occupied, visited, x, y - 1, count, peakTemp, sumX, sumY, pixels);
  if (y < 7) floodFill(occupied, visited, x, y + 1, count, peakTemp, sumX, sumY, pixels);
}

// Bilinear interpolation: 8×8 → 16×16 for smoother heatmap rendering
void ThermalProcessor::interpolate8to16(const float pixels[64], float out[256]) {
  for (uint8_t oy = 0; oy < 16; oy++) {
    for (uint8_t ox = 0; ox < 16; ox++) {
      float srcX = ox * 7.0f / 15.0f;
      float srcY = oy * 7.0f / 15.0f;

      uint8_t x0 = (uint8_t)srcX;
      uint8_t y0 = (uint8_t)srcY;
      uint8_t x1 = min((uint8_t)(x0 + 1), (uint8_t)7);
      uint8_t y1 = min((uint8_t)(y0 + 1), (uint8_t)7);

      float xFrac = srcX - x0;
      float yFrac = srcY - y0;

      float topLeft = pixels[y0 * 8 + x0];
      float topRight = pixels[y0 * 8 + x1];
      float botLeft = pixels[y1 * 8 + x0];
      float botRight = pixels[y1 * 8 + x1];

      float top = topLeft + (topRight - topLeft) * xFrac;
      float bot = botLeft + (botRight - botLeft) * xFrac;

      out[oy * 16 + ox] = top + (bot - top) * yFrac;
    }
  }
}
