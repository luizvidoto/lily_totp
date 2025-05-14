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
uint32_t last_lock_check_time = 0; // Para o timer de bloqueio por inatividade

// Lock state
bool is_locked = true; // Começa bloqueado por padrão

// RFID Card Management
char authorized_card_ids[MAX_AUTHORIZED_CARDS][MAX_CARD_ID_LEN];
int authorized_card_count = 0;
int current_manage_card_index = 0; // Para navegação na lista de gerenciamento de cartões
char temp_card_id[MAX_CARD_ID_LEN]; // Para adicionar/confirmar novo cartão
// Novas variáveis para rolagem da lista de cartões gerenciados
int manage_cards_top_visible_index = 0;
// int manage_cards_highlight_y_current = -1; // Para animação, se desejado depois
// int manage_cards_highlight_y_target = -1;

// Variável para editar o timeout de bloqueio
int temp_lock_timeout_minutes = 5; // Valor padrão, será carregado do NVS

// ---- Novas Globais para Configuração de Brilho ----
// Valores atuais de brilho (serão carregados do NVS, com fallback para defines de config.h)
uint8_t brightness_usb_level = BRIGHTNESS_USB; 
uint8_t brightness_battery_level = BRIGHTNESS_BATTERY;
uint8_t brightness_dimmed_level = BRIGHTNESS_DIMMED;

// Variável temporária para editar um valor de brilho
int temp_brightness_value = 0;

// REMOVE Enum definition from here, it's now in globals.h
// enum class BrightnessSettingToAdjust {
// USB,
// BATTERY,
// DIMMED
// };
BrightnessSettingToAdjust current_brightness_setting_to_adjust; // Definition of the variable
// ---- Fim das Novas Globais para Configuração de Brilho ----

int gmt_offset = 0;
Language current_language = Language::LANG_PT_BR; // Padrão
const LanguageStrings *current_strings_ptr = nullptr; // Será definido em i18n.cpp

int temp_gmt_offset = 0;
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

// Definição das opções do menu principal e seu tamanho
const StringID menuOptionIDs[] = {
    StringID::STR_MENU_VIEW_CODES,      // Top-left: Codes
    StringID::STR_ACTION_CONFIG,        // Top-middle: Config
    StringID::STR_MENU_ADJUST_TIMEZONE, // Top-right: Zone
    StringID::STR_MENU_READ_RFID_OPTION,// Bottom-left: RFID
    StringID::STR_MENU_SELECT_LANGUAGE,   // Bottom-middle: Lang
    StringID::STR_MENU_ADJUST_TIME        // Bottom-right: Date
};
const int NUM_MENU_OPTIONS = sizeof(menuOptionIDs) / sizeof(menuOptionIDs[0]);

// ---- Novo: Opções do Submenu de Configurações ----
const StringID configMenuOptionIDs[] = {
    StringID::STR_MENU_CONFIG_MANAGE_CARDS,
    StringID::STR_MENU_CONFIG_LOCK_TIMEOUT, // Placeholder
    StringID::STR_MENU_CONFIG_BRIGHTNESS    // Placeholder
};
const int NUM_CONFIG_MENU_OPTIONS = sizeof(configMenuOptionIDs) / sizeof(configMenuOptionIDs[0]);
int current_config_menu_index = 0; // Índice para o submenu de configurações
// Variáveis para animação do submenu de config (se for usar o mesmo estilo de animação)
// int config_menu_top_visible_index = 0;
// int config_menu_highlight_y_current = -1;
// int config_menu_highlight_y_target = -1;
// int config_menu_highlight_y_anim_start = -1;
// unsigned long config_menu_animation_start_time = 0;
// bool is_config_menu_animating = false;

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
const char* NVS_SVC_NAME_PREFIX = "svc_"; // Ex: svc_0_n, svc_0_s
const char* NVS_SVC_NAME_SUFFIX = "_n";
const char* NVS_SVC_SECRET_SUFFIX = "_s";
// Novas chaves NVS para RFID e estado de bloqueio
const char* NVS_LOCK_STATE_KEY = "lock_state";
const char* NVS_AUTH_CARD_COUNT_KEY = "auth_c_cnt";
const char* NVS_AUTH_CARD_PREFIX = "auth_c_"; // Ex: auth_c_0, auth_c_1
const char* NVS_LOCK_TIMEOUT_KEY = "lock_timeout";
// Novas chaves NVS para níveis de brilho
const char* NVS_BRIGHTNESS_USB_KEY = "br_usb";
const char* NVS_BRIGHTNESS_BATTERY_KEY = "br_bat";
const char* NVS_BRIGHTNESS_DIMMED_KEY = "br_dim";