#include "globals.h"
#include "config.h" // Para SS_PIN, RST_PIN
#include "types.h"

// ---- Objetos de Hardware e Bibliotecas ----
RTC_DS3231 rtc; // Objeto RTC
TFT_eSPI tft = TFT_eSPI(); // Objeto TFT
Preferences preferences; // Objeto para NVS
MFRC522 mfrc522(SDA_PIN, RST_PIN); // Objeto MFRC522

// Sprites
TFT_eSprite spr_totp_code = TFT_eSprite(&tft);
TFT_eSprite spr_progress_bar = TFT_eSprite(&tft);
TFT_eSprite spr_header_clock = TFT_eSprite(&tft);
TFT_eSprite spr_header_battery = TFT_eSprite(&tft);

// Botões (pinos definidos em pin_config.h)
OneButton btn_prev(PIN_BUTTON_0, true, true);
OneButton btn_next(PIN_BUTTON_1, true, true);

// ---- Estado Global da Aplicação ----
TOTPService services[MAX_SERVICES];
int service_count = 0;
int current_service_index = 0;
CurrentTOTPInfo current_totp;
ScreenState current_screen = ScreenState::SCREEN_STARTUP; // Inicia na tela de Startup
ScreenState previous_screen = ScreenState::SCREEN_STARTUP;

BatteryInfo battery_info;
uint8_t current_target_brightness = BRIGHTNESS_BATTERY;
uint32_t last_interaction_time = 0;

int gmt_offset = 0;
Language current_language = Language::LANG_PT_BR; // Padrão
const LanguageStrings *current_strings_ptr = nullptr; // Será definido em i18n.cpp

char temp_service_name[MAX_SERVICE_NAME_LEN + 1];
char temp_service_secret[MAX_SECRET_B32_LEN + 1];
int edit_time_field = 0;
int edit_hour = 0, edit_minute = 0, edit_second = 0;

int current_menu_index = 0;
int menu_top_visible_index = 0;
int menu_highlight_y_current = -1;
int menu_highlight_y_target = -1;
int menu_highlight_y_anim_start = -1;
unsigned long menu_animation_start_time = 0;
bool is_menu_animating = false;

// Definição das opções do menu principal e seu tamanho <<< ADICIONE ESTAS LINHAS ABAIXO
const StringID menuOptionIDs[] = {
    StringID::STR_MENU_VIEW_CODES,
    StringID::STR_MENU_ADD_SERVICE,
    StringID::STR_MENU_READ_RFID_OPTION,
    StringID::STR_MENU_ADJUST_TIME,
    StringID::STR_MENU_ADJUST_TIMEZONE,
    StringID::STR_MENU_SELECT_LANGUAGE
};
const int NUM_MENU_OPTIONS = sizeof(menuOptionIDs) / sizeof(menuOptionIDs[0]);

char message_buffer[120];
uint16_t message_color = COLOR_FG;
uint32_t message_duration_ms = 0;
ScreenState message_next_screen = ScreenState::SCREEN_MENU_MAIN;

char card_id[16] = {0};

uint32_t last_rtc_sync_time = 0;
uint32_t last_screen_update_time = 0;

// ---- Definição das Chaves NVS (declaradas extern em config.h) ----
const char* NVS_NAMESPACE = "totp-app";
const char* NVS_LANG_KEY = "language";
const char* NVS_TZ_OFFSET_KEY = "tz_offset";
const char* NVS_SVC_COUNT_KEY = "svc_count";
const char* NVS_SVC_NAME_PREFIX = "svc_";
const char* NVS_SVC_NAME_SUFFIX = "_n";
const char* NVS_SVC_SECRET_SUFFIX = "_s";