#pragma once

#include <Arduino.h>

void setupLogger();
void logLine(const String& message);
void logf(const char* format, ...);

// Gated by DEBUG_MODE (see Config.h). A no-op call still costs a function
// call + varargs pack when DEBUG_MODE is 0, but nothing is printed --
// callers do not need to wrap call sites in #if DEBUG_MODE themselves.
void debugf(const char* format, ...);
