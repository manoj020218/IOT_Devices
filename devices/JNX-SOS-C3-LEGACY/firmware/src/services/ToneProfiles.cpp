#include "ToneProfiles.h"

#include "../config/ProductConfig.h"

namespace {
constexpr ToneProfile kProfiles[ProductConfig::kProfileCount] = {
    {1, "Police Siren", "Classic rise/fall police sweep.", "700Hz to 1800Hz sweep",
     "Moderate-high duty with clamp", "Urban alerting", 75, true, TonePatternMode::SWEEP,
     700, 1800, 1500, 0, 0},
    {2, "Emergency Siren", "Fast emergency sweep for primary alert.",
     "1200Hz to 2400Hz fast sweep", "Default operating duty profile",
     "Primary emergency notification", 80, true, TonePatternMode::SWEEP, 1200, 2400, 800, 0, 0},
    {3, "War Siren", "Slow dramatic sweep for wide-area warning.", "500Hz to 1800Hz slow sweep",
     "Reduced sustained duty recommended", "Civil warning or drill", 65, false,
     TonePatternMode::SWEEP, 500, 1800, 4000, 0, 0},
    {4, "Danger Siren", "Sharp pulsed danger tone.", "2000Hz pulse 400ms ON / 200ms OFF",
     "Pulse pattern reduces thermal load", "Immediate hazard warning", 70, true,
     TonePatternMode::PULSE, 2000, 0, 0, 400, 200},
    {5, "Fire Alarm", "Alternating high/low fire alarm pattern.", "1000Hz / 1800Hz alternating",
     "Alternating duty with clamp", "Fire evacuation tone", 70, true,
     TonePatternMode::ALTERNATE, 1000, 1800, 350, 0, 0},
    {6, "Ambulance Siren", "Two-tone alternating ambulance sound.",
     "900Hz / 1400Hz alternating", "Balanced duty for long runs", "Medical response alert", 72,
     true, TonePatternMode::ALTERNATE, 900, 1400, 600, 0, 0},
    {7, "Fast Hooter", "Fast alternating hooter tone.", "1600Hz / 2200Hz every 200ms",
     "Short bursts preferred", "Crowd or vehicle alert", 78, false, TonePatternMode::ALTERNATE,
     1600, 2200, 200, 0, 0},
    {8, "Slow Hooter", "Long sweeping hooter pattern.", "800Hz to 2200Hz over 2s",
     "Conservative duty for general warning", "Slow warning cadence", 72, true,
     TonePatternMode::SWEEP, 800, 2200, 2000, 0, 0},
    {9, "SOS Pattern", "Three short, three long, three short.",
     "1800Hz Morse SOS burst pattern", "Patterned duty for emergency signaling",
     "Distress signaling", 75, true, TonePatternMode::SOS, 1800, 0, 0, 0, 0},
    {10, "Continuous Alert Tone", "Steady alert tone.", "2000Hz constant tone",
     "Use only with enforced rest cycle", "Continuous hazard indication", 65, false,
     TonePatternMode::CONSTANT, 2000, 0, 0, 0, 0},
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

const ToneProfile* ToneProfiles::defaultProfile() { return findById(2); }
