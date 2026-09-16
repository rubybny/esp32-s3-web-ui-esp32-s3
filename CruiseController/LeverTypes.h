#pragma once

// The five things the lever can be read as, plus UNKNOWN for "none of the
// above". UNKNOWN is a real, distinct state (not folded into NONE) because
// it must behave differently: NONE is "confirmed released, safe to accept a
// new operation"; UNKNOWN is "reading doesn't match anything we trust right
// now", which must turn outputs off but must NOT be treated as a clean
// release for the purposes of the neutral lockout (see LeverState.cpp).
enum class LeverButton : uint8_t {
  NONE,
  MAIN,
  CANCEL,
  RES,
  SET,
  UNKNOWN,
};

inline const char* leverButtonName(LeverButton b) {
  switch (b) {
    case LeverButton::MAIN: return "MAIN";
    case LeverButton::CANCEL: return "CANCEL";
    case LeverButton::RES: return "RES+";
    case LeverButton::SET: return "SET-";
    case LeverButton::UNKNOWN: return "UNKNOWN";
    case LeverButton::NONE:
    default: return "NONE";
  }
}

inline bool isRealButton(LeverButton b) {
  return b == LeverButton::MAIN || b == LeverButton::CANCEL || b == LeverButton::RES ||
         b == LeverButton::SET;
}
