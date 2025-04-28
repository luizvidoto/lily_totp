#include "globals.h" // Inclui as declarações 'extern'

//=============================================================================
// Definições e Inicializações das Variáveis Globais
//=============================================================================

// --- Hardware Objects ---
// Os construtores são chamados aqui, inicializando os objetos
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr_header_clock = TFT_eSprite(&tft); // Associado ao tft principal
TFT_eSprite spr_header_batt = TFT_eSprite(&tft);  // Associado ao tft principal
TFT_eSprite spr_totp_code = TFT_eSprite(&tft);    // Associado ao tft principal
TFT_eSprite spr_progress_bar = TFT_eSprite(&tft);// Associado ao tft principal
RTC_DS3231 rtc;                                   // Objeto RTC (usa Wire padrão se não especificado)
MFRC522 mfrc522(SDA_PIN, RST_PIN);                // Objeto RFID com pinos definidos em config.h
OneButton btn_prev(PIN_BUTTON_0, true, true);     // Botão PREV (ativo baixo, pullup interno habilitado)
OneButton btn_next(PIN_BUTTON_1, true, true);     // Botão NEXT (ativo baixo, pullup interno habilitado)
Preferences preferences;                          // Objeto NVS

// --- Application State ---
ScreenState current_screen = ScreenState::SCREEN_MENU_MAIN; // Inicia na tela de boot
ScreenState previous_screen = ScreenState::SCREEN_MENU_MAIN; // Padrão para voltar após boot/msg
TOTPService services[MAX_SERVICES];                // Aloca memória para o array de serviços
int service_count = 0;                             // Nenhum serviço carregado inicialmente
int current_service_index = -1;                    // Nenhum serviço selecionado inicialmente
CurrentTOTPInfo current_totp = { "------", 0, false }; // Inicializa com placeholder, sem intervalo, chave inválida
BatteryInfo battery_info = { 0.0f, false, 0 };     // Estado inicial da bateria
int gmt_offset_hours = 0;                          // Fuso padrão GMT+0
Language current_language = Language::PT_BR;      // Idioma padrão (será sobrescrito pelo NVS se existir)

// Identificadores das opções do menu principal
const StringID menuOptionIDs[] = {
    STR_MENU_ADD_SERVICE,
    STR_MENU_READ_RFID,
    STR_MENU_VIEW_CODES,
    STR_MENU_ADJUST_TIME,
    STR_MENU_ADJUST_TIMEZONE,
    STR_MENU_SELECT_LANGUAGE};
const int NUM_MENU_OPTIONS = sizeof(menuOptionIDs) / sizeof(menuOptionIDs[0]);

// --- UI State ---
uint8_t current_brightness_level = 0;              // Brilho inicial (será definido no setup)
MenuState main_menu_state = { 0, 0, -1, -1, 0, false }; // Estado inicial do menu principal
MenuState lang_menu_state = { 0, 0, -1, -1, 0, false }; // Estado inicial do menu de idioma
TempData temp_data = { "", "", 0, 0, 0, 0, 0, Language::PT_BR, "" }; // Dados temporários zerados/padrão

// --- Timers ---
uint32_t last_interaction_time = 0;
uint32_t last_rtc_sync_time = 0;
uint32_t last_screen_update_time = 0;
uint32_t message_end_time = 0;                     // Nenhuma mensagem ativa inicialmente

// --- Buffers ---
char message_buffer[120] = {0};                    // Inicializa o buffer vazio
uint16_t message_color = COLOR_FG;                 // Cor padrão para mensagens

// --- Flags ---
bool request_full_redraw = true;                   // Solicita redesenho inicial completo
bool rtc_available = false;                        // Assume que RTC não está disponível até ser inicializado