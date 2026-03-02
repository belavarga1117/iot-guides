/*
 * thermal_processor.h — Presence Detection from Grid-EYE Thermal Data
 *
 * Takes 8×8 thermal frames and determines:
 * - Is someone present? (binary occupancy)
 * - How many people? (blob counting)
 * - Where are they? (hotspot position)
 */

#ifndef THERMAL_PROCESSOR_H
#define THERMAL_PROCESSOR_H

#include "grid_eye.h"

// Default thresholds (tunable per room)
#define DEFAULT_PRESENCE_THRESHOLD  28.0f  // °C — human skin temp at distance
#define DEFAULT_DELTA_THRESHOLD     2.5f   // °C above ambient = presence
#define MAX_BLOBS                   4

struct Blob {
  uint8_t centerX;    // 0-7 grid position
  uint8_t centerY;    // 0-7 grid position
  float peakTemp;     // Hottest pixel in blob
  uint8_t pixelCount; // Number of pixels in blob
};

struct PresenceResult {
  bool occupied;                 // Anyone detected?
  uint8_t personCount;           // Number of distinct blobs
  float maxTemp;                 // Hottest pixel overall
  float ambientTemp;             // From thermistor
  Blob blobs[MAX_BLOBS];        // Detected blobs (up to MAX_BLOBS)
};

class ThermalProcessor {
public:
  ThermalProcessor();

  // Configure thresholds
  void setAbsoluteThreshold(float tempC);
  void setDeltaThreshold(float deltaC);

  // Process a frame and detect presence
  PresenceResult process(const GridEyeFrame& frame);

  // Get interpolated 8×8 → 16×16 (for smoother visualization)
  void interpolate8to16(const float pixels[64], float out[256]);

private:
  float _absThreshold;
  float _deltaThreshold;

  // Simple flood-fill blob detection on 8×8 grid
  uint8_t findBlobs(const bool occupied[64], const float pixels[64],
                    Blob blobs[MAX_BLOBS]);
  void floodFill(const bool occupied[64], bool visited[64],
                 uint8_t x, uint8_t y, uint8_t* count,
                 float* peakTemp, float* sumX, float* sumY,
                 const float pixels[64]);
};

#endif // THERMAL_PROCESSOR_H
