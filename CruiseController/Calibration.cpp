#include "Calibration.h"

#include "Config.h"
#include "Logger.h"

#if ENABLE_ADC_LEARNING
#include <Preferences.h>
#endif

namespace {
struct BandDefault {
  int min;
  int max;
};

BandDefault defaultBandFor(LeverButton b) {
  using namespace AdcBands;
  switch (b) {
    case LeverButton::MAIN: return {ADC_MAIN_MIN, ADC_MAIN_MAX};
    case LeverButton::CANCEL: return {ADC_CANCEL_MIN, ADC_CANCEL_MAX};
    case LeverButton::RES: return {ADC_RES_MIN, ADC_RES_MAX};
    case LeverButton::SET: return {ADC_SET_MIN, ADC_SET_MAX};
    default: return {0, 0};
  }
}

int defaultCenterFor(LeverButton b) {
  BandDefault d = defaultBandFor(b);
  return (d.min + d.max) / 2;
}

// Runtime-effective centers/threshold. Initialized to the Config.h defaults
// and only overwritten if a learned value is found in NVS at boot.
int centerMain = defaultCenterFor(LeverButton::MAIN);
int centerCancel = defaultCenterFor(LeverButton::CANCEL);
int centerRes = defaultCenterFor(LeverButton::RES);
int centerSet = defaultCenterFor(LeverButton::SET);
int neutralMin = AdcBands::ADC_NEUTRAL_MIN;

// A few counts of margin kept below a learned open-circuit reading, so a
// slightly noisy reading right after learning doesn't immediately fail to
// clear the neutral threshold.
constexpr int NEUTRAL_LEARN_MARGIN = 100;

#if ENABLE_ADC_LEARNING
Preferences prefs;
constexpr const char* NVS_NAMESPACE = "ccs3cal";
#endif

int* centerVarFor(LeverButton b) {
  switch (b) {
    case LeverButton::MAIN: return &centerMain;
    case LeverButton::CANCEL: return &centerCancel;
    case LeverButton::RES: return &centerRes;
    case LeverButton::SET: return &centerSet;
    default: return nullptr;
  }
}

const char* nvsKeyFor(LeverButton b) {
  switch (b) {
    case LeverButton::MAIN: return "main";
    case LeverButton::CANCEL: return "cancel";
    case LeverButton::RES: return "res";
    case LeverButton::SET: return "set";
    default: return nullptr;
  }
}
}  // namespace

void loadAdcCalibration() {
#if ENABLE_ADC_LEARNING
  prefs.begin(NVS_NAMESPACE, /*readOnly=*/false);

  LeverButton buttons[] = {LeverButton::MAIN, LeverButton::CANCEL, LeverButton::RES, LeverButton::SET};
  for (LeverButton b : buttons) {
    const char* key = nvsKeyFor(b);
    if (prefs.isKey(key)) {
      *centerVarFor(b) = prefs.getInt(key, defaultCenterFor(b));
    }
  }
  if (prefs.isKey("neutralMin")) {
    neutralMin = prefs.getInt("neutralMin", AdcBands::ADC_NEUTRAL_MIN);
  }

  logf("Calibration loaded: main=%d cancel=%d res=%d set=%d neutralMin=%d", centerMain, centerCancel,
       centerRes, centerSet, neutralMin);
#else
  logLine("ADC learning disabled at compile time -- using fixed Config.h bands");
#endif
}

void saveAdcCalibration() {
#if ENABLE_ADC_LEARNING
  prefs.putInt(nvsKeyFor(LeverButton::MAIN), centerMain);
  prefs.putInt(nvsKeyFor(LeverButton::CANCEL), centerCancel);
  prefs.putInt(nvsKeyFor(LeverButton::RES), centerRes);
  prefs.putInt(nvsKeyFor(LeverButton::SET), centerSet);
  prefs.putInt("neutralMin", neutralMin);
#endif
}

void resetAdcCalibration() {
  centerMain = defaultCenterFor(LeverButton::MAIN);
  centerCancel = defaultCenterFor(LeverButton::CANCEL);
  centerRes = defaultCenterFor(LeverButton::RES);
  centerSet = defaultCenterFor(LeverButton::SET);
  neutralMin = AdcBands::ADC_NEUTRAL_MIN;

#if ENABLE_ADC_LEARNING
  prefs.clear();
#endif

  logLine("Calibration reset to Config.h defaults");
}

void getAdcBand(LeverButton button, int* outMin, int* outMax) {
  // Offsets from the *default* center to the *default* min/max are kept
  // separately (not a single halfWidth) because Config.h bands are not
  // always symmetric around their midpoint -- e.g. CANCEL [70,125] centers
  // at 97, which is 27 below max but 28 above min. Applying one shared
  // halfWidth to both sides silently shrank whichever side rounded down.
  BandDefault d = defaultBandFor(button);
  int defaultCenter = defaultCenterFor(button);
  int lowOffset = defaultCenter - d.min;
  int highOffset = d.max - defaultCenter;

  int center = defaultCenter;
  int* var = centerVarFor(button);
  if (var) center = *var;

  *outMin = center - lowOffset;
  *outMax = center + highOffset;
}

int getAdcNeutralMin() { return neutralMin; }

namespace {
// True if `value` sits inside `button`'s *current effective* band (learned
// center + configured width), used to sanity-check new learn requests
// against every *other* button so a mis-held learn can't silently push two
// buttons' bands into each other.
bool withinCurrentBand(LeverButton button, int value) {
  int lo, hi;
  getAdcBand(button, &lo, &hi);
  return value >= lo && value <= hi;
}
}  // namespace

bool learnAdcCenter(LeverButton button, int adcValue) {
#if !ENABLE_ADC_LEARNING
  (void)button;
  (void)adcValue;
  logLine("Learning is disabled (ENABLE_ADC_LEARNING=0) -- ignoring");
  return false;
#else
  int* var = centerVarFor(button);
  if (!var) return false;

  if (adcValue < 0 || adcValue >= neutralMin) {
    logf("Refusing to learn %s center = %d -- that reads as released (>= neutralMin=%d); hold the button while sending the learn command",
         leverButtonName(button), adcValue, neutralMin);
    return false;
  }

  LeverButton others[] = {LeverButton::MAIN, LeverButton::CANCEL, LeverButton::RES, LeverButton::SET};
  for (LeverButton other : others) {
    if (other == button) continue;
    if (withinCurrentBand(other, adcValue)) {
      logf("Refusing to learn %s center = %d -- that falls inside %s's current band; looks like the wrong button was held",
           leverButtonName(button), adcValue, leverButtonName(other));
      return false;
    }
  }

  *var = adcValue;
  saveAdcCalibration();
  logf("Learned %s center = %d (saved)", leverButtonName(button), adcValue);
  return true;
#endif
}

bool learnAdcNeutral(int adcValue) {
#if !ENABLE_ADC_LEARNING
  (void)adcValue;
  logLine("Learning is disabled (ENABLE_ADC_LEARNING=0) -- ignoring");
  return false;
#else
  int candidate = adcValue - NEUTRAL_LEARN_MARGIN;

  LeverButton buttons[] = {LeverButton::MAIN, LeverButton::CANCEL, LeverButton::RES, LeverButton::SET};
  for (LeverButton b : buttons) {
    int lo, hi;
    getAdcBand(b, &lo, &hi);
    if (candidate <= hi) {
      logf("Refusing to learn NEUTRAL from adc=%d -- too close to %s's band [%d..%d]; release the lever fully first",
           adcValue, leverButtonName(b), lo, hi);
      return false;
    }
  }

  neutralMin = candidate;
  saveAdcCalibration();
  logf("Learned NEUTRAL: open-circuit adc=%d -> neutralMin=%d (saved)", adcValue, neutralMin);
  return true;
#endif
}

void printAdcCalibration() {
  int mMin, mMax, cMin, cMax, rMin, rMax, sMin, sMax;
  getAdcBand(LeverButton::MAIN, &mMin, &mMax);
  getAdcBand(LeverButton::CANCEL, &cMin, &cMax);
  getAdcBand(LeverButton::RES, &rMin, &rMax);
  getAdcBand(LeverButton::SET, &sMin, &sMax);

  logLine("---- Calibration ----");
  logf("MAIN   [%4d..%4d]", mMin, mMax);
  logf("CANCEL [%4d..%4d]", cMin, cMax);
  logf("RES+   [%4d..%4d]", rMin, rMax);
  logf("SET-   [%4d..%4d]", sMin, sMax);
  logf("NEUTRAL >= %d", getAdcNeutralMin());
  logLine("----------------------");
}
