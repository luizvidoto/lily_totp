#pragma once

#include "types.h"

// Declare language string tables
extern const LanguageStrings strings_pt_br;
extern const LanguageStrings strings_en_us;
extern const LanguageStrings *languages[NUM_LANGUAGES];
extern Language current_language;
extern const LanguageStrings *current_strings;

// Function declarations
const char* getText(StringID id);
bool saveLanguagePreference(Language lang);