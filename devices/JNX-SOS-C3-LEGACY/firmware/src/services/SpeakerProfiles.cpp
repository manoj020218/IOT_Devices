#include "SpeakerProfiles.h"

namespace {
constexpr SpeakerProfile kProfiles[] = {
    {SpeakerProfileId::SUH15_SAFE, "Ahuja SUH-15 Safe",
     "12V safe profile with burst cooling for 15W 8-ohm horn.", 600, 1200, 20, 20, 10,
     2000, 500},
    {SpeakerProfileId::SUH25, "Ahuja SUH-25",
     "12V profile with wider sweep and longer active window for 25W 8-ohm horn.", 500, 1500,
     15, 30, 8, 0, 0},
};
}  // namespace

const SpeakerProfile* SpeakerProfiles::all() { return kProfiles; }

size_t SpeakerProfiles::count() { return sizeof(kProfiles) / sizeof(kProfiles[0]); }

const SpeakerProfile* SpeakerProfiles::defaultProfile() { return &kProfiles[0]; }

const SpeakerProfile* SpeakerProfiles::findById(SpeakerProfileId id) {
  for (size_t i = 0; i < count(); ++i) {
    if (kProfiles[i].id == id) {
      return &kProfiles[i];
    }
  }
  return defaultProfile();
}

const char* SpeakerProfiles::toString(SpeakerProfileId id) { return findById(id)->name; }

SpeakerProfileId SpeakerProfiles::fromString(const String& value) {
  return value.equalsIgnoreCase("Ahuja SUH-25") || value.equalsIgnoreCase("SUH25")
             ? SpeakerProfileId::SUH25
             : SpeakerProfileId::SUH15_SAFE;
}
