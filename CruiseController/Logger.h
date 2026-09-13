#pragma once

#include <Arduino.h>

void setupLogger();
void logLine(const String& message);
void logf(const char* format, ...);
