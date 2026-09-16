#include "AdcFilter.h"

#include <algorithm>

#include "Config.h"

namespace {
using AdcFilterConfig::SAMPLE_COUNT;

int ringBuffer[AdcFilterConfig::SAMPLE_COUNT];
uint8_t ringCount = 0;   // how many slots have ever been written (ramps up to SAMPLE_COUNT after boot)
uint8_t ringIndex = 0;   // next slot to write
int lastRaw = 4095;
uint32_t lastSampleAt = 0;

bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}
}  // namespace

void setupAdcFilter() {
  analogReadResolution(12);
  pinMode(Pins::ADC_CRUISE, INPUT);

  // Prime the buffer with real readings (not zeros) so the very first
  // median right after boot already reflects the actual lever position
  // instead of a false low reading.
  lastRaw = analogRead(Pins::ADC_CRUISE);
  for (uint8_t i = 0; i < SAMPLE_COUNT; i++) ringBuffer[i] = lastRaw;
  ringCount = SAMPLE_COUNT;
  ringIndex = 0;
}

void updateAdcFilter(uint32_t now) {
  if (!elapsed(now, lastSampleAt + AdcFilterConfig::SAMPLE_INTERVAL_MS)) return;
  lastSampleAt = now;

  lastRaw = analogRead(Pins::ADC_CRUISE);
  ringBuffer[ringIndex] = lastRaw;
  ringIndex = (ringIndex + 1) % SAMPLE_COUNT;
  if (ringCount < SAMPLE_COUNT) ringCount++;
}

int readAdc() { return lastRaw; }

int filterAdc() {
  // Sorting a copy for the median is plenty fast at this buffer size
  // (<=20 ints) versus the multi-ms sample interval budget.
  int sorted[AdcFilterConfig::SAMPLE_COUNT];
  uint8_t n = ringCount;
  for (uint8_t i = 0; i < n; i++) sorted[i] = ringBuffer[i];
  std::sort(sorted, sorted + n);
  return sorted[n / 2];
}
