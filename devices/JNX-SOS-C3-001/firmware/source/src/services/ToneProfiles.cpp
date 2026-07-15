#include "ToneProfiles.h"

#include "../config/ProductConfig.h"

namespace {
constexpr ToneProfile kProfiles[ProductConfig::kProfileCount] = {
    {1, "Emergency Evacuation", "Fast rising emergency sweep for plant or campus evacuation.",
     "1200Hz to 2000Hz fast sweep", "50% audio square wave", "General emergency warning", 80,
     true, 15, TonePatternMode::SWEEP, 1200, 2000, 800, 0, 0},
    {2, "Fire Zone Alarm", "Alternating fire alarm pattern for industrial fire response.",
     "1000Hz / 1800Hz alternating", "50% audio square wave", "Fire emergency alert", 70, true,
     15, TonePatternMode::ALTERNATE, 1000, 1800, 350, 0, 0},
    {3, "Flood Warning", "Slow dramatic sweep for far-area flood or weather warning.",
     "500Hz to 1800Hz slow sweep", "50% audio square wave", "Wide-area disaster warning", 65,
     false, 30, TonePatternMode::SWEEP, 500, 1800, 4000, 0, 0},
    {4, "Landslide / Hazard Alert", "Sharp pulsed alert for immediate local danger.",
     "2000Hz pulse 400ms ON / 200ms OFF", "50% audio square wave", "Hazard edge warning", 70,
     true, 15, TonePatternMode::PULSE, 2000, 0, 0, 400, 200},
    {5, "Factory Shift Start / Duty On", "Fast hooter pattern for shift start notification.",
     "1600Hz / 2000Hz every 200ms", "50% audio square wave", "Routine shift start", 78, false,
     15, TonePatternMode::ALTERNATE, 1600, 2000, 200, 0, 0},
    {6, "Factory Shift End / Duty Off", "Long sweeping hooter for shift end or shutdown.",
     "800Hz to 2000Hz over 2s", "50% audio square wave", "Routine shift end", 72, true,
     15, TonePatternMode::SWEEP, 800, 2000, 2000, 0, 0},
    {7, "Lunch Break Bell", "Two-tone repeating pattern suitable for routine meal break calls.",
     "900Hz / 1400Hz alternating", "50% audio square wave", "Routine lunch break", 72, true,
     15, TonePatternMode::ALTERNATE, 900, 1400, 600, 0, 0},
    {8, "Assembly Point Call", "Classic sweep for general assembly or muster point call.",
     "700Hz to 1800Hz sweep", "50% audio square wave", "Routine public assembly call", 75, true,
     15, TonePatternMode::SWEEP, 700, 1800, 1500, 0, 0},
    {9, "Distress SOS", "Three short, three long, three short distress pattern.",
     "1800Hz Morse SOS burst pattern", "50% audio square wave", "Manual distress signaling", 75,
     true, 30, TonePatternMode::SOS, 1800, 0, 0, 0, 0},
    {10, "All Clear / Test Tone", "Steady tone for controlled all-clear or supervised test.",
     "2000Hz constant tone", "50% audio square wave", "Status indication or test", 65, false,
     15, TonePatternMode::CONSTANT, 2000, 0, 0, 0, 0},
};
}  // namespace

const ToneProfile* ToneProfiles::all() { return kProfiles; }

size_t ToneProfiles::count() { return ProductConfig::kProfileCount; }

const ToneProfile* ToneProfiles::findById(uint8_t id) {
  for (size_t i = 0; i < count(); ++i) {
    if (kProfiles[i].id == id) {
      return &kProfiles[i];
    }
  }
  return nullptr;
}

const ToneProfile* ToneProfiles::defaultProfile() { return findById(1); }
