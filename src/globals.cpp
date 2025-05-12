#include "globals.h"
#include "pin_config.h"

// Hardware Interfaces
TFT_eSPI tft;
TFT_eSprite spr_totp_code(&tft);
TFT_eSprite spr_progress_bar(&tft);
ESP32Time rtc;
OneButton btn_prev(BTN_PREV, true);
OneButton btn_next(BTN_NEXT, true);
OneButton btn_sel(BTN_SEL, true);
MFRC522 mfrc522(SDA_PIN, RST_PIN);

// Application State
ScreenState current_screen = SCREEN_TOTP_VIEW;
Language current_language = LANG_PT_BR;
const LanguageStrings *current_strings = nullptr;
int current_service_index = 0;
int current_language_menu_index = 0;
int8_t timezone_offset = 0;
char card_id[16] = {0};

// Time Management
uint64_t current_timestamp_utc = 0;
int current_hour = 0;
int current_minute = 0;
int current_second = 0;
unsigned long last_rtc_sync = 0;
unsigned long last_screen_update = 0;
unsigned long last_activity = 0;

// Menu State
int menu_top_visible_index = 0;
int menu_highlight_y_current = -1;
int menu_highlight_y_target = -1;
unsigned long menu_animation_start_time = 0;
bool is_menu_animating = false;

// System Status
BatteryInfo battery_info = {0};

// Constants
const char* NVS_LANG_KEY = "language";
const char* NVS_TZ_OFFSET_KEY = "tz_offset";
const StringID menuOptionIDs[] = {
    STR_MENU_ADD_SERVICE,
    STR_MENU_READ_RFID,
    STR_MENU_VIEW_CODES,
    STR_MENU_ADJUST_TIME,
    STR_MENU_ADJUST_TIMEZONE,
    STR_MENU_SELECT_LANGUAGE
};
const int NUM_MENU_OPTIONS = sizeof(menuOptionIDs) / sizeof(menuOptionIDs[0]);