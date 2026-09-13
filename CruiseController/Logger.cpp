#include "Logger.h"

#include <stdarg.h>

#include "Config.h"

void setupLogger() {
  Serial.begin(LogConfig::BAUD_RATE);
  uint32_t waitUntil = millis() + 2000;
  while (!Serial && static_cast<int32_t>(millis() - waitUntil) < 0) {
    delay(10);
  }
}

void logLine(const String& message) {
  Serial.print('[');
  Serial.print(millis());
  Serial.print("ms] ");
  Serial.println(message);
}

void logf(const char* format, ...) {
  char buffer[160];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  logLine(String(buffer));
}
