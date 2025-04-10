/*
  TOTP Authenticator Optimized - Multi-Language & Unified Header v4.1
  Projeto para ESP32 com TFT, RTC DS3231 e geração TOTP.
*/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <TFT_eSPI.h>
#include "OneButton.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <MFRC522.h>
#include "mbedtls/md.h"
#include "pin_config.h" // SEU ARQUIVO DE CONFIGURAÇÃO DE PINOS

// ---- Constantes e Configurações ----
constexpr uint32_t TOTP_INTERVAL = 30;
constexpr int MAX_SERVICES = 50;
constexpr size_t MAX_SERVICE_NAME_LEN = 20;
constexpr size_t MAX_SECRET_B32_LEN = 64;
constexpr size_t MAX_SECRET_BIN_LEN = 40;

constexpr int VISIBLE_MENU_ITEMS = 3;
constexpr uint32_t INACTIVITY_TIMEOUT_MS = 30000;
constexpr uint32_t RTC_SYNC_INTERVAL_MS = 60000;
constexpr uint32_t SCREEN_UPDATE_INTERVAL_MS = 1000; // Intervalo base para atualização
constexpr int MENU_ANIMATION_DURATION_MS = 150;
constexpr uint32_t LOOP_DELAY_MS = 15; // Delay principal do loop

// --- Constantes da Interface Gráfica ---
#define SCREEN_WIDTH tft.width()
#define SCREEN_HEIGHT tft.height()
// Cores
#define COLOR_BG TFT_BLACK
#define COLOR_FG TFT_WHITE
#define COLOR_ACCENT TFT_CYAN
#define COLOR_SUCCESS TFT_GREEN
#define COLOR_ERROR TFT_RED
#define COLOR_BAR_FG TFT_WHITE
#define COLOR_BAR_BG TFT_DARKGREY
#define COLOR_HIGHLIGHT_BG TFT_WHITE
#define COLOR_HIGHLIGHT_FG TFT_BLACK

// Posições e Tamanhos
#define UI_PADDING 5
#define UI_HEADER_HEIGHT 25
#define UI_CLOCK_WIDTH_ESTIMATE 85 // Largura estimada para relógio HH:MM:SS
#define UI_BATT_WIDTH 24
#define UI_BATT_HEIGHT 16
#define UI_CONTENT_Y_START (UI_HEADER_HEIGHT + UI_PADDING) // Y inicial para conteúdo abaixo do header
#define UI_TOTP_CODE_FONT_SIZE 3
#define UI_PROGRESS_BAR_HEIGHT 15
#define UI_PROGRESS_BAR_WIDTH (SCREEN_WIDTH - 2 * UI_PADDING - 20)
#define UI_PROGRESS_BAR_X ((SCREEN_WIDTH - UI_PROGRESS_BAR_WIDTH) / 2)
#define UI_FOOTER_TEXT_Y (SCREEN_HEIGHT - UI_PADDING / 2) // Y da linha de base do texto do rodapé
#define UI_MENU_ITEM_HEIGHT 30
#define UI_MENU_START_Y UI_CONTENT_Y_START

// ---- Internacionalização (i18n) ----
enum Language {
  LANG_PT_BR,
  LANG_EN_US,
  NUM_LANGUAGES // Deve ser o último
};

enum StringID {
  STR_TITLE_TOTP_CODE,
  STR_TITLE_MAIN_MENU,
  STR_TITLE_ADD_SERVICE,
  STR_TITLE_CONFIRM_ADD,
  STR_TITLE_ADJUST_TIME,
  STR_TITLE_CONFIRM_DELETE,
  STR_TITLE_ADJUST_TIMEZONE,
  STR_TITLE_SELECT_LANGUAGE,
  STR_MENU_ADD_SERVICE,
  STR_MENU_READ_RFID,
  STR_MENU_VIEW_CODES,
  STR_MENU_ADJUST_TIME,
  STR_MENU_ADJUST_TIMEZONE,
  STR_MENU_SELECT_LANGUAGE,
  STR_LANG_PORTUGUESE,
  STR_LANG_ENGLISH,
  STR_AWAITING_JSON,
  STR_VIA_SERIAL,
  STR_EXAMPLE_JSON_SERVICE,
  STR_EXAMPLE_JSON_TIME,
  STR_SERVICES_STATUS_FMT,
  STR_FOOTER_GENERIC_NAV,
  STR_FOOTER_CONFIRM_NAV,
  STR_FOOTER_TIME_EDIT_NAV,
  STR_FOOTER_TIMEZONE_NAV,
  STR_FOOTER_LANG_NAV,
  STR_TIME_EDIT_INFO_FMT,
  STR_TIME_EDIT_JSON_HINT,
  STR_TIMEZONE_LABEL,
  STR_CONFIRM_ADD_PROMPT,
  STR_CONFIRM_DELETE_PROMPT,
  STR_SERVICE_ADDED,
  STR_SERVICE_DELETED,
  STR_TIME_ADJUSTED_FMT,
  STR_TIMEZONE_SAVED_FMT,
  STR_LANG_SAVED,
  STR_ERROR_RTC,
  STR_ERROR_NO_SERVICES,
  STR_ERROR_JSON_PARSE_FMT,
  STR_ERROR_JSON_INVALID_SERVICE,
  STR_ERROR_JSON_INVALID_TIME,
  STR_ERROR_SERVICE_NAME_INVALID,
  STR_ERROR_SECRET_INVALID,
  STR_ERROR_TIME_VALUES_INVALID,
  STR_ERROR_SAVING,
  STR_ERROR_DELETING,
  STR_ERROR_NVS_LOAD,
  STR_ERROR_NVS_SAVE,
  STR_ERROR_B32_DECODE,
  STR_ERROR_MAX_SERVICES,
  STR_TOTP_CODE_ERROR,
  STR_TOTP_NO_SERVICE_TITLE,
  NUM_STRINGS // Deve ser o último
};

struct LanguageStrings {
  const char *strings[NUM_STRINGS];
};

const LanguageStrings strings_pt_br = {{
    "Codigo TOTP",
    "Menu Principal",
    "Adicionar Servico",
    "Confirmar Adicao",
    "Ajustar Hora",
    "Confirmar Exclusao",
    "Ajustar Fuso",
    "Selecionar Idioma",
    "Adicionar Servico",
    "Ler Cartao RFID",
    "Ver Codigos TOTP",
    "Ajustar Hora/Data",
    "Ajustar Fuso Horario",
    "Idioma",
    "Portugues (BR)",
    "Ingles (US)",
    "Aguardando JSON",
    "via Serial...",
    "Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}",
    "{'y':..,'mo':..,'d':..,'h':..,'m':..,'s':..}",
    "Servicos: %d/%d",
    "Prev/Next | LP: Sel | DblClick: Menu",
    "Prev: Canc | Next: Conf",
    "Prev/Next: +/- | LP: Campo/Salvar | Dbl: Menu",
    "Prev/Next: +/- | LP: Salvar | Dbl: Menu",
    "Prev/Next | LP: Salvar | Dbl: Menu",
    "(Fuso atual: GMT %+d)",
    "Ou envie JSON via Serial (UTC):",
    "GMT %+d",
    "Adicionar Servico:",
    "Deletar servico:",
    "Servico Adicionado!",
    "Servico Deletado",
    "Hora Ajustada:\n%02d:%02d:%02d",
    "Fuso Salvo:\nGMT %+d",
    "Idioma Salvo!",
    "Erro RTC!",
    "Nenhum servico\ncadastrado!",
    "Erro JSON:\n%s",
    "JSON Invalido:\nFalta 'name'/'secret'",
    "JSON Invalido:\nFaltam campos Y/M/D h:m:s",
    "Nome Servico\nInvalido!",
    "Segredo Invalido!",
    "Valores Data/Hora\nInvalidos!",
    "Erro ao Salvar!",
    "Erro ao Deletar",
    "[NVS] Erro ao carregar",
    "[NVS] Erro ao salvar",
    "Falha chave B32!",
    "Max Servicos!",
    "------",
    "Codigo TOTP",
}};

const LanguageStrings strings_en_us = {{
    "TOTP Code",
    "Main Menu",
    "Add Service",
    "Read RFID Card",
    "Confirm Addition",
    "Adjust Time",
    "Confirm Deletion",
    "Adjust Timezone",
    "Select Language",
    "Add Service",
    "View TOTP Codes",
    "Adjust Time/Date",
    "Adjust Timezone",
    "Language",
    "Portuguese (BR)",
    "English (US)",
    "Awaiting JSON",
    "via Serial...",
    "Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}",
    "{'y':..,'mo':..,'d':..,'h':..,'m':..,'s':..}",
    "Services: %d/%d",
    "Prev/Next | LP: Sel | DblClick: Menu",
    "Prev: Canc | Next: Conf",
    "Prev/Next: +/- | LP: Field/Save | Dbl: Menu",
    "Prev/Next: +/- | LP: Save | Dbl: Menu",
    "Prev/Next | LP: Save | Dbl: Menu",
    "(Current Zone: GMT %+d)",
    "Or send JSON via Serial (UTC):",
    "GMT %+d",
    "Add Service:",
    "Delete service:",
    "Service Added!",
    "Service Deleted",
    "Time Adjusted:\n%02d:%02d:%02d",
    "Zone Saved:\nGMT %+d",
    "Language Saved!",
    "RTC Error!",
    "No services\nregistered!",
    "JSON Error:\n%s",
    "Invalid JSON:\nMissing 'name'/'secret'",
    "Invalid JSON:\nMissing Y/M/D h:m:s",
    "Invalid Service\nName!",
    "Invalid Secret!",
    "Invalid Date/Time\nValues!",
    "Error Saving!",
    "Error Deleting",
    "[NVS] Load error",
    "[NVS] Save error",
    "B32 Key Error!",
    "Max Services!",
    "------",
    "TOTP Code",
}};

const LanguageStrings *languages[NUM_LANGUAGES] = {&strings_pt_br, &strings_en_us};
Language current_language = LANG_PT_BR;
const LanguageStrings *current_strings = languages[current_language];
const char *NVS_LANG_KEY = "language";
int current_language_menu_index = 0;

// ---- Estruturas de Dados ----
struct TOTPService {
  char name[MAX_SERVICE_NAME_LEN + 1];
  char secret_b32[MAX_SECRET_B32_LEN + 1];
};

struct CurrentTOTPInfo {
  char code[7];
  uint32_t last_generated_interval;
  uint8_t key_bin[MAX_SECRET_BIN_LEN];
  size_t key_bin_len;
  bool valid_key;
};

struct BatteryInfo {
  float voltage;
  bool is_usb_powered;
  int level_percent;
};

// ---- Estados da Aplicação ----
enum ScreenState {
  SCREEN_TOTP_VIEW,
  SCREEN_MENU_MAIN,
  SCREEN_SERVICE_ADD_WAIT,
  SCREEN_SERVICE_ADD_CONFIRM,
  SCREEN_TIME_EDIT,
  SCREEN_SERVICE_DELETE_CONFIRM,
  SCREEN_TIMEZONE_EDIT,
  SCREEN_LANGUAGE_SELECT,
  SCREEN_MESSAGE,
  SCREEN_READ_RFID
};

// ---- Variáveis Globais ----
RTC_DS3231 rtc;
MFRC522 mfrc522(SDA_PIN, RST_PIN); 
TFT_eSPI tft = TFT_eSPI();
Preferences preferences;
TFT_eSprite spr_totp_code = TFT_eSprite(&tft);
TFT_eSprite spr_progress_bar = TFT_eSprite(&tft);
OneButton btn_prev(PIN_BUTTON_0, true, true);
OneButton btn_next(PIN_BUTTON_1, true, true);
TOTPService services[MAX_SERVICES];
int service_count = 0;
int current_service_index = 0;
CurrentTOTPInfo current_totp;
ScreenState current_screen = SCREEN_MENU_MAIN;
BatteryInfo battery_info;
uint8_t current_brightness = 10;
uint32_t last_interaction_time = 0;
uint32_t last_rtc_sync_time = 0;
uint32_t last_screen_update_time = 0;
char temp_service_name[MAX_SERVICE_NAME_LEN + 1];
char temp_service_secret[MAX_SECRET_B32_LEN + 1];
int edit_time_field = 0, edit_hour, edit_minute, edit_second;
int current_menu_index = 0;
char message_buffer[100];
uint16_t message_color = COLOR_FG;
int gmt_offset = 0;
const char *NVS_TZ_OFFSET_KEY = "tz_offset";
int menu_top_visible_index = 0;
int menu_highlight_y_current = -1;
int menu_highlight_y_target = -1;
unsigned long menu_animation_start_time = 0;
bool is_menu_animating = false;
constexpr uint8_t BRIGHTNESS_USB = 12;
constexpr uint8_t BRIGHTNESS_BATTERY = 8;
constexpr uint8_t BRIGHTNESS_DIMMED = 2;
char card_id[16];

// Identificadores das opções do menu principal
const StringID menuOptionIDs[] = {
    STR_MENU_ADD_SERVICE,
    STR_MENU_READ_RFID,
    STR_MENU_VIEW_CODES,
    STR_MENU_ADJUST_TIME,
    STR_MENU_ADJUST_TIMEZONE,
    STR_MENU_SELECT_LANGUAGE};
const int NUM_MENU_OPTIONS = sizeof(menuOptionIDs) / sizeof(menuOptionIDs[0]);

// ---- Protótipos de Funções ----
// Core Logic & Hardware
void setup();
void loop();
void initHardware();
void initSprites();
void changeScreen(ScreenState);
// Time & RTC
void updateTimeFromRTC();
void updateRTCFromSystem();
// RFID
void readRFIDCard();
// TOTP & Crypto
int base32_decode(const uint8_t *, size_t, uint8_t *, size_t);
bool decodeCurrentServiceKey();
uint32_t generateTOTP(const uint8_t *, size_t, uint64_t, uint32_t);
void updateCurrentTOTP();
// Storage (NVS)
void loadServices();
bool storage_saveServiceList();
bool storage_saveService(const char *, const char *);
bool storage_deleteService(int);
// Serial Input
void processSerialInput();
void processServiceAdd(JsonDocument &);
void processTimeSet(JsonDocument &);
// Power & Battery
void updateBatteryStatus();
void setScreenBrightness(uint8_t);
void updateScreenBrightness();
// Button Callbacks
void btn_prev_click();
void btn_next_click();
void btn_next_double_click();
void btn_next_long_press_start();
// UI Drawing - Main Orchestration & Helpers
void ui_drawScreen(bool);
void ui_drawHeader(const char *);
void ui_updateHeaderDynamic();
void ui_drawClockDirect(bool);
void ui_drawBatteryDirect();
void ui_updateProgressBarSprite(uint64_t);
void ui_updateTOTPCodeSprite();
void ui_showTemporaryMessage(const char *, uint16_t, uint32_t);
// UI Drawing - Screen Specific Content
void ui_drawScreenTOTPContent(bool);
void ui_drawScreenMenuContent(bool);
void ui_drawScreenServiceAddWaitContent(bool);
void ui_drawScreenServiceAddConfirmContent(bool);
void ui_drawScreenTimeEditContent(bool);
void ui_drawScreenServiceDeleteConfirmContent(bool);
void ui_drawScreenTimezoneEditContent(bool);
void ui_drawScreenLanguageSelectContent(bool);
void ui_drawScreenMessage(bool);
void ui_drawScreenReadRFID(bool);
// i18n Helper
const char *getScreenTitle(ScreenState);
const char *getText(StringID);

// ---- Funções de Decodificação Base32 e TOTP ----
int base32_decode(const uint8_t *encoded, size_t encodedLength, uint8_t *result, size_t bufSize) {
    int buffer = 0, bitsLeft = 0, count = 0;
    for (size_t i = 0; i < encodedLength && count < bufSize; i++) {
        uint8_t ch = encoded[i], value = 255;
        if (ch >= 'A' && ch <= 'Z') value = ch - 'A';
        else if (ch >= 'a' && ch <= 'z') value = ch - 'a';
        else if (ch >= '2' && ch <= '7') value = ch - '2' + 26;
        else if (ch == '=') continue; // Ignora padding
        else continue; // Ignora caracteres inválidos
        buffer = (buffer << 5) | value;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            result[count++] = (buffer >> (bitsLeft - 8)) & 0xFF;
            bitsLeft -= 8;
        }
    }
    return count;
}

uint32_t generateTOTP(const uint8_t *key, size_t keyLength, uint64_t timestamp, uint32_t interval = TOTP_INTERVAL) {
    if (!key || keyLength == 0) {
        Serial.printf("[ERROR] generateTOTP: Chave inválida (len=%d)\n", keyLength);
        return 0;
    }
    uint64_t counter = timestamp / interval;
    uint8_t counterBytes[8];
    // Converte counter para Big-Endian byte array
    for (int i = 7; i >= 0; i--) {
        counterBytes[i] = counter & 0xFF;
        counter >>= 8;
    }

    uint8_t hash[20]; // SHA-1 output é 20 bytes
    mbedtls_md_context_t ctx;
    mbedtls_md_info_t const *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (!info) {
        Serial.println("[ERROR] generateTOTP: mbedtls_md_info_from_type falhou");
        return 0; // Não conseguiu obter info SHA1
    }

    mbedtls_md_init(&ctx);
    // Configura para HMAC-SHA1
    if (mbedtls_md_setup(&ctx, info, 1) != 0) { // 1 = use HMAC
        Serial.println("[ERROR] generateTOTP: mbedtls_md_setup falhou");
        mbedtls_md_free(&ctx); return 0;
    }
    if (mbedtls_md_hmac_starts(&ctx, key, keyLength) != 0) {
        Serial.println("[ERROR] generateTOTP: mbedtls_md_hmac_starts falhou");
        mbedtls_md_free(&ctx); return 0;
    }
    if (mbedtls_md_hmac_update(&ctx, counterBytes, 8) != 0) {
        Serial.println("[ERROR] generateTOTP: mbedtls_md_hmac_update falhou");
        mbedtls_md_free(&ctx); return 0;
    }
    if (mbedtls_md_hmac_finish(&ctx, hash) != 0) {
        Serial.println("[ERROR] generateTOTP: mbedtls_md_hmac_finish falhou");
        mbedtls_md_free(&ctx); return 0;
    }
    mbedtls_md_free(&ctx); // Libera contexto

    // Extração dinâmica (RFC 4226)
    int offset = hash[19] & 0x0F; // Último nibble do hash define o offset (0-15)
    // Extrai 4 bytes a partir do offset, zera o bit mais significativo do primeiro byte
    uint32_t binaryCode =
        ((hash[offset] & 0x7F) << 24) |
        ((hash[offset + 1] & 0xFF) << 16) |
        ((hash[offset + 2] & 0xFF) << 8)  |
        (hash[offset + 3] & 0xFF);

    // Retorna os últimos 6 dígitos
    return binaryCode % 1000000;
}

// ---- Função Auxiliar de Idioma ----
const char* getText(StringID id) {
  if (id < NUM_STRINGS && current_strings != nullptr) {
    const char* text = current_strings->strings[id];
    // Retorna uma string vazia segura se o ponteiro for nulo (erro na definição)
    return (text != nullptr) ? text : "";
  }
  // Log de erro se ID for inválido
  Serial.printf("[ERROR] getText: ID inválido %d ou ponteiro nulo\n", id);
  return "?ERR?";
}

// ---- Setup e Loop Principal ----
void setup() {
  Serial.begin(115200);
  while (!Serial); // Espera Serial (opcional, mas bom para debug inicial)
  Serial.println("\n[SETUP] Iniciando TOTP Authenticator Multi-Idioma v4.1...");

  // Carrega configurações ANTES de usar textos na inicialização do HW
  preferences.begin("totp-app", true); // Abre NVS para leitura
  int saved_lang_index = preferences.getInt(NVS_LANG_KEY, (int)LANG_PT_BR); // Padrão PT_BR se não existir
  if (saved_lang_index >= 0 && saved_lang_index < NUM_LANGUAGES) {
      current_language = (Language)saved_lang_index;
  } else {
      current_language = LANG_PT_BR; // Fallback se valor salvo for inválido
      Serial.printf("[WARN] Idioma NVS inválido (%d), usando padrão.\n", saved_lang_index);
  }
  current_strings = languages[current_language]; // Define ponteiro global para textos
  gmt_offset = preferences.getInt(NVS_TZ_OFFSET_KEY, 0); // Padrão GMT+0
  preferences.end(); // Fecha NVS
  Serial.printf("[SETUP] Idioma: %d, Fuso: GMT%+d\n", current_language, gmt_offset);

  // Inicializa Hardware (RTC, TFT, Pinos)
  initHardware(); // Usa getText internamente para msg de erro RTC se necessário

  // Inicializa Sprites
  initSprites();

  // Carrega Serviços do NVS
  loadServices();

  // Decodifica chave do serviço inicial (se houver)
  if (service_count > 0) {
    current_service_index = 0; // Começa no primeiro
    if (!decodeCurrentServiceKey()) {
        // O erro já foi logado em decodeCurrentServiceKey
        // A UI mostrará o erro B32
    }
  } else {
    current_totp.valid_key = false; // Nenhum serviço
    snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(STR_TOTP_CODE_ERROR));
  }

  // Configura callbacks dos botões
  btn_prev.attachClick(btn_prev_click);
  btn_next.attachClick(btn_next_click);
  btn_next.attachDoubleClick(btn_next_double_click);
  btn_next.attachLongPressStart(btn_next_long_press_start);
  Serial.println("[SETUP] Botões configurados.");

  // Configuração inicial de energia e timers
  updateBatteryStatus();
  current_brightness = battery_info.is_usb_powered ? BRIGHTNESS_USB : BRIGHTNESS_BATTERY;
  setScreenBrightness(current_brightness);
  last_interaction_time = millis();
  last_rtc_sync_time = millis();
  last_screen_update_time = 0; // Força atualização da tela no primeiro loop

  changeScreen(SCREEN_MENU_MAIN); // Inicia na tela do menu principal
  Serial.println("[SETUP] Inicialização concluída.");
}

void loop() {
  // Processa eventos dos botões
  btn_prev.tick();
  btn_next.tick();

  // Processa leitura RFID somente nas telas apropriadas
    if (current_screen == SCREEN_READ_RFID) {
        readRFIDCard(); // Lê cartão RFID
    }

  // Processa entrada Serial apenas em telas específicas
  if (current_screen == SCREEN_SERVICE_ADD_WAIT || current_screen == SCREEN_TIME_EDIT) {
    processSerialInput();
  }

  uint32_t currentMillis = millis();

  // Sincroniza o tempo do sistema com o RTC periodicamente
  if (currentMillis - last_rtc_sync_time >= RTC_SYNC_INTERVAL_MS) {
    updateTimeFromRTC();
    last_rtc_sync_time = currentMillis;
  }

  // Ajusta o brilho da tela com base na inatividade e alimentação
  updateScreenBrightness();

  // Verifica se é hora de uma atualização regular da tela
  bool needsRegularUpdate = false;
  if (currentMillis - last_screen_update_time >= SCREEN_UPDATE_INTERVAL_MS) {
    needsRegularUpdate = true;
    last_screen_update_time = currentMillis;
    updateBatteryStatus(); // Atualiza info da bateria
    // Atualiza o código TOTP se necessário
    if (current_screen == SCREEN_TOTP_VIEW && service_count > 0) {
      updateCurrentTOTP();
    }
  }

  // Redesenha a tela se for a atualização regular OU se o menu estiver animando
  if (needsRegularUpdate || is_menu_animating) {
    ui_drawScreen(false); // Chama desenho parcial (atualiza header dinâmico e conteúdo)
  }

  delay(LOOP_DELAY_MS); // Pequeno delay para ceder tempo e suavizar animação
}

// ---- Funções de Inicialização ----
void initHardware() {
  pinMode(PIN_POWER_ON, OUTPUT); digitalWrite(PIN_POWER_ON, HIGH); // Garante alimentação se houver controle
  pinMode(PIN_LCD_BL, OUTPUT); digitalWrite(PIN_LCD_BL, LOW);    // Backlight começa desligado
  pinMode(PIN_BAT_VOLT, INPUT);                                 // Pino de leitura da bateria
  Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);                         // Inicializa I2C para RTC

  Serial.print("[HW] RFID_RC522... ");
    // Inicializa SPI (SCK, MISO, MOSI, SS)
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, RST_PIN);
    mfrc522.PCD_Init(); 

  Serial.print("[HW] RTC... ");
  if (!rtc.begin()) {
      Serial.println("FALHOU!");
      // Tenta inicializar TFT para mostrar erro antes de travar
      tft.init();
      tft.fillScreen(COLOR_ERROR);
      tft.setTextColor(TFT_WHITE, COLOR_ERROR); // Texto branco sobre fundo vermelho
      tft.setTextDatum(MC_DATUM);               // Centralizado
      tft.setTextSize(2);                       // Tamanho razoável
      tft.drawString(getText(STR_ERROR_RTC), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
      while(1) delay(1000); // Trava aqui
  }
  Serial.println("OK.");
  if (rtc.lostPower()) { // Verifica se RTC perdeu energia (bateria fraca/removida)
      Serial.println("RTC perdeu energia. Ajuste a hora!");
      // TODO: Adicionar lógica para forçar ajuste de hora ou usar tempo padrão?
  }
  updateTimeFromRTC(); // Sincroniza TimeLib com RTC na inicialização

  Serial.print("[HW] TFT... ");
  tft.init();             // Inicializa o driver TFT
  tft.setRotation(3);     // Define rotação (ajustar para seu display)
  tft.fillScreen(COLOR_BG); // Limpa tela inicial
  Serial.println("OK.");
}

void initSprites() {
  // Cria sprite para o código TOTP (maior, mais cores para antialiasing da fonte)
  spr_totp_code.setColorDepth(16);
  spr_totp_code.createSprite(150, 40); // Ajustar tamanho se necessário
  spr_totp_code.setTextDatum(MC_DATUM); // Alinhamento Middle Center

  // Cria sprite para a barra de progresso (menor, menos cores)
  spr_progress_bar.setColorDepth(8);
  spr_progress_bar.createSprite(UI_PROGRESS_BAR_WIDTH - 4, UI_PROGRESS_BAR_HEIGHT - 4); // Tamanho interno
}

void loadServices() {
    if (!preferences.begin("totp-app", true)) { // Abre NVS no modo somente leitura
        Serial.println(getText(STR_ERROR_NVS_LOAD));
        service_count = 0; // Zera contagem se não conseguir ler
        return;
    }
    service_count = preferences.getInt("svc_count", 0); // Lê contador, default 0
    // Validações básicas do contador
    if (service_count < 0) service_count = 0;
    if (service_count > MAX_SERVICES) {
        Serial.printf("[WARN] Contador NVS (%d) > MAX (%d). Limitando.\n", service_count, MAX_SERVICES);
        service_count = MAX_SERVICES;
    }

    int valid_count = 0; // Contador para serviços válidos encontrados
    for (int i = 0; i < service_count; i++) {
        char name_key[16]; char secret_key[16];
        snprintf(name_key, sizeof(name_key), "svc_%d_name", i);
        snprintf(secret_key, sizeof(secret_key), "svc_%d_secret", i);

        String name_str = preferences.getString(name_key, "");
        String secret_str = preferences.getString(secret_key, "");

        // Verifica se os dados carregados são válidos (não vazios e dentro dos limites)
        if (name_str.length() > 0 && name_str.length() <= MAX_SERVICE_NAME_LEN &&
            secret_str.length() > 0 && secret_str.length() <= MAX_SECRET_B32_LEN)
        {
            // Copia para a posição correta no array 'services', compactando a lista
            strncpy(services[valid_count].name, name_str.c_str(), MAX_SERVICE_NAME_LEN);
            services[valid_count].name[MAX_SERVICE_NAME_LEN] = '\0';
            strncpy(services[valid_count].secret_b32, secret_str.c_str(), MAX_SECRET_B32_LEN);
            services[valid_count].secret_b32[MAX_SECRET_B32_LEN] = '\0';
            valid_count++; // Incrementa apenas se o serviço for válido
        } else {
            Serial.printf("[WARN] Serviço %d inválido/ausente no NVS. Pulando.\n", i);
            // Não incrementa valid_count
        }
    }
    preferences.end(); // Fecha NVS

    // Se encontramos menos serviços válidos do que o contador indicava, ajusta
    if (valid_count != service_count) {
        Serial.printf("Serviços compactados de %d para %d.\n", service_count, valid_count);
        service_count = valid_count;
        // Opcional: Salvar a lista compactada de volta no NVS
        // storage_saveServiceList();
    }
    Serial.printf("[NVS] %d serviços válidos carregados.\n", service_count);
}


// ---- Funções de Tempo ----
void updateTimeFromRTC() {
    setTime(rtc.now().unixtime());
    //Serial.println("[TIME] TimeLib sincronizado com RTC.");
}
void updateRTCFromSystem() {
    // Cria objeto DateTime com a hora ATUAL do TimeLib (que é UTC)
    DateTime dt_to_set(year(), month(), day(), hour(), minute(), second());
    // Ajusta o hardware RTC
    rtc.adjust(dt_to_set);
    Serial.println("[RTC] RTC HW atualizado com hora do sistema (UTC).");
}

// ---- Funções TOTP ----
bool decodeCurrentServiceKey(){
    if(service_count <= 0 || current_service_index >= service_count || current_service_index < 0){
        current_totp.valid_key = false;
        snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(STR_TOTP_CODE_ERROR));
        // Serial.println("[TOTP] Índice inválido ou sem serviços.");
        return false;
    }
    const char* secret_b32 = services[current_service_index].secret_b32;
    // Serial.printf("[TOTP] Decodificando chave para '%s': %s\n", services[current_service_index].name, secret_b32);
    int decoded_len = base32_decode((const uint8_t*)secret_b32, strlen(secret_b32), current_totp.key_bin, MAX_SECRET_BIN_LEN);

    if(decoded_len > 0 && decoded_len <= MAX_SECRET_BIN_LEN){
        current_totp.key_bin_len = decoded_len;
        current_totp.valid_key = true;
        current_totp.last_generated_interval = 0; // Força geração inicial
        updateCurrentTOTP(); // Gera o primeiro código para a nova chave
        // Serial.printf("[TOTP] Chave decodificada com sucesso (%d bytes).\n", decoded_len);
        return true;
    } else {
        current_totp.valid_key = false;
        current_totp.key_bin_len = 0;
        snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(STR_ERROR_B32_DECODE));
        Serial.printf("[ERROR] Falha ao decodificar Base32 para '%s'. Len: %d\n", services[current_service_index].name, decoded_len);
        return false;
    }
}

void updateCurrentTOTP(){
    // Se não houver chave válida carregada, mostra erro e sai
    if (!current_totp.valid_key) {
        snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(STR_TOTP_CODE_ERROR));
        return;
    }
    uint64_t current_unix_time_utc = now(); // Usa tempo UTC do TimeLib
    uint32_t current_interval = current_unix_time_utc / TOTP_INTERVAL;

    // Gera um novo código apenas se o intervalo de tempo mudou
    if (current_interval != current_totp.last_generated_interval) {
        uint32_t totp_code_val = generateTOTP(current_totp.key_bin, current_totp.key_bin_len, current_unix_time_utc);
        snprintf(current_totp.code, sizeof(current_totp.code), "%06lu", totp_code_val); // Formata com 6 dígitos
        current_totp.last_generated_interval = current_interval; // Atualiza último intervalo gerado
        // Serial.printf("[TOTP] Novo código gerado: %s\n", current_totp.code);
    }
    // Se o intervalo não mudou, o código existente em current_totp.code é mantido
}

// ---- Funções de Armazenamento (NVS) ----
bool storage_saveServiceList() {
    if(!preferences.begin("totp-app", false)) { // Abre NVS para leitura/escrita
        Serial.println(getText(STR_ERROR_NVS_SAVE));
        return false;
    }
    int old_count = preferences.getInt("svc_count", 0); // Lê contador antigo
    // Remove chaves de serviços que não existem mais (se lista diminuiu)
    for(int i = service_count; i < old_count; ++i) {
        char name_key[16], secret_key[16];
        snprintf(name_key,sizeof(name_key),"svc_%d_name",i);
        snprintf(secret_key,sizeof(secret_key),"svc_%d_secret",i);
        preferences.remove(name_key);
        preferences.remove(secret_key);
    }
    // Salva o novo contador de serviços
    preferences.putInt("svc_count", service_count);
    bool success = true;
    // Salva cada serviço atual no NVS
    for(int i = 0; i < service_count; i++) {
        char name_key[16], secret_key[16];
        snprintf(name_key,sizeof(name_key),"svc_%d_name",i);
        snprintf(secret_key,sizeof(secret_key),"svc_%d_secret",i);
        if(!preferences.putString(name_key, services[i].name)) success = false;
        if(!preferences.putString(secret_key, services[i].secret_b32)) success = false;
    }
    preferences.end(); // Fecha NVS
    if (!success) Serial.println(getText(STR_ERROR_NVS_SAVE));
    return success;
}

bool storage_saveService(const char *name, const char *secret_b32) {
    if(service_count >= MAX_SERVICES){
        ui_showTemporaryMessage(getText(STR_ERROR_MAX_SERVICES), COLOR_ERROR, 2000);
        return false;
    }
    // Adiciona ao array em memória
    strncpy(services[service_count].name, name, MAX_SERVICE_NAME_LEN);
    services[service_count].name[MAX_SERVICE_NAME_LEN] = '\0';
    strncpy(services[service_count].secret_b32, secret_b32, MAX_SECRET_B32_LEN);
    services[service_count].secret_b32[MAX_SECRET_B32_LEN] = '\0';
    service_count++; // Incrementa contador
    // Salva a lista inteira atualizada no NVS
    return storage_saveServiceList();
}

bool storage_deleteService(int index) {
    if(index < 0 || index >= service_count) {
        Serial.printf("[ERROR] Tentativa de deletar índice inválido: %d\n", index);
        return false;
    }
    Serial.printf("[NVS] Deletando '%s' (idx %d)\n", services[index].name, index);
    // Desloca os elementos seguintes para cobrir o espaço removido
    for(int i = index; i < service_count - 1; i++) {
        services[i] = services[i+1];
    }
    service_count--; // Decrementa o contador
    memset(&services[service_count], 0, sizeof(TOTPService)); // Limpa a última posição (agora vazia)

    // Ajusta o índice do serviço atual, se necessário
    if(current_service_index >= service_count && service_count > 0) {
        current_service_index = service_count - 1; // Seleciona o último item se o deletado era o último ou além
    } else if (service_count == 0) {
        current_service_index = 0; // Zera se não houver mais serviços
    }
    // Se deletou um item *antes* do atual, o índice atual continua apontando para o mesmo serviço (que subiu uma posição)
    // Se deletou o item *atual*, o índice agora aponta para o próximo item (que tomou o lugar do deletado)

    bool suc = storage_saveServiceList(); // Salva a nova lista (menor) no NVS

    // Recarrega a chave do serviço que agora está no índice atual
    if(service_count > 0) {
        decodeCurrentServiceKey();
    } else { // Se não houver mais serviços
        current_totp.valid_key = false;
        snprintf(current_totp.code, sizeof(current_totp.code),"%s",getText(STR_TOTP_CODE_ERROR));
    }
    return suc;
}

// ---- Funções de Entrada Serial ----
void processSerialInput() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() == 0) return; // Ignora linhas vazias

        Serial.printf("[SERIAL] RX: %s\n", input.c_str());
        last_interaction_time = millis(); // Considera entrada serial como interação

        // Tenta parsear o JSON
        StaticJsonDocument<256> doc; // Ajustar tamanho se necessário
        DeserializationError error = deserializeJson(doc, input);

        if (error) {
            // Mostra erro de parsing
            snprintf(message_buffer, sizeof(message_buffer), getText(STR_ERROR_JSON_PARSE_FMT), error.c_str());
            ui_showTemporaryMessage(message_buffer, COLOR_ERROR, 3000);
            // Volta ao menu em caso de erro nas telas de entrada
            if (current_screen == SCREEN_SERVICE_ADD_WAIT || current_screen == SCREEN_TIME_EDIT) {
                changeScreen(SCREEN_MENU_MAIN);
            }
            return;
        }

        // Delega o processamento baseado na tela atual
        if (current_screen == SCREEN_SERVICE_ADD_WAIT) {
            processServiceAdd(doc);
        } else if (current_screen == SCREEN_TIME_EDIT) {
            processTimeSet(doc);
        }
    }
}

void processServiceAdd(JsonDocument &doc) {
    // Verifica se os campos obrigatórios existem e são do tipo correto
    if(!doc.containsKey("name") || !doc["name"].is<const char*>() || !doc.containsKey("secret") || !doc["secret"].is<const char*>()) {
        ui_showTemporaryMessage(getText(STR_ERROR_JSON_INVALID_SERVICE), COLOR_ERROR, 3000);
        changeScreen(SCREEN_MENU_MAIN); return;
    }
    const char* name = doc["name"];
    const char* secret = doc["secret"];

    // Valida comprimento dos dados
    if(strlen(name) == 0 || strlen(name) > MAX_SERVICE_NAME_LEN){
        ui_showTemporaryMessage(getText(STR_ERROR_SERVICE_NAME_INVALID), COLOR_ERROR, 3000);
        changeScreen(SCREEN_MENU_MAIN); return;
    }
    if(strlen(secret) == 0 || strlen(secret) > MAX_SECRET_B32_LEN){
        ui_showTemporaryMessage(getText(STR_ERROR_SECRET_INVALID), COLOR_ERROR, 3000);
        changeScreen(SCREEN_MENU_MAIN); return;
    }

    // TODO: Adicionar validação dos caracteres do segredo Base32 se desejado

    // Copia dados válidos para variáveis temporárias e vai para confirmação
    strncpy(temp_service_name, name, sizeof(temp_service_name) - 1); temp_service_name[sizeof(temp_service_name) - 1] = '\0';
    strncpy(temp_service_secret, secret, sizeof(temp_service_secret) - 1); temp_service_secret[sizeof(temp_service_secret) - 1] = '\0';
    changeScreen(SCREEN_SERVICE_ADD_CONFIRM);
}

void processTimeSet(JsonDocument &doc) {
    // Verifica campos de data e hora
    if(!doc.containsKey("year")||!doc["year"].is<int>()||!doc.containsKey("month")||!doc["month"].is<int>()||
       !doc.containsKey("day")||!doc["day"].is<int>()||!doc.containsKey("hour")||!doc["hour"].is<int>()||
       !doc.containsKey("minute")||!doc["minute"].is<int>()||!doc.containsKey("second")||!doc["second"].is<int>()) {
        ui_showTemporaryMessage(getText(STR_ERROR_JSON_INVALID_TIME), COLOR_ERROR, 3000);
        changeScreen(SCREEN_MENU_MAIN); return;
    }
    int y = doc["year"], m = doc["month"], d = doc["day"], h = doc["hour"], mn = doc["minute"], s = doc["second"];

    // Valida os intervalos dos valores
    if(y < 2023 || y > 2100 || m < 1 || m > 12 || d < 1 || d > 31 || h < 0 || h > 23 || mn < 0 || mn > 59 || s < 0 || s > 59) {
        ui_showTemporaryMessage(getText(STR_ERROR_TIME_VALUES_INVALID), COLOR_ERROR, 3000);
        changeScreen(SCREEN_MENU_MAIN); return;
    }

    // Se tudo ok, define o tempo do sistema (UTC) e atualiza o RTC
    setTime(h, mn, s, d, m, y);
    updateRTCFromSystem();

    // Mostra mensagem de sucesso com a hora LOCAL ajustada
    time_t local_adjusted_time = now() + gmt_offset * 3600;
    snprintf(message_buffer, sizeof(message_buffer), getText(STR_TIME_ADJUSTED_FMT), hour(local_adjusted_time), minute(local_adjusted_time), second(local_adjusted_time));
    ui_showTemporaryMessage(message_buffer, COLOR_SUCCESS, 3000);
    // changeScreen(SCREEN_MENU_MAIN); // Chamado pela função de mensagem
}

// ---- Funções de Gestão de Energia ----
void updateBatteryStatus() {
    int adcValue = analogRead(PIN_BAT_VOLT);
    // *** VERIFIQUE/AJUSTE ESTA FÓRMULA PARA SEU HARDWARE ESPECÍFICO ***
    battery_info.voltage = (adcValue / 4095.0) * 3.3 * 2.0; // Exemplo comum T-Display S3

    // Define limiares (ajustar conforme sua bateria LiPo/Li-Ion)
    constexpr float USB_THRESHOLD_VOLTAGE = 4.15; // Acima disso, assume USB
    constexpr float BATTERY_MAX_VOLTAGE = 4.10;   // Tensão considerada 100%
    constexpr float BATTERY_MIN_VOLTAGE = 3.20;   // Tensão considerada 0% (cuidado!)

    battery_info.is_usb_powered = (battery_info.voltage >= USB_THRESHOLD_VOLTAGE);

    if (battery_info.is_usb_powered) {
        battery_info.level_percent = 100;
    } else {
        // Mapeamento linear da tensão (entre min e max) para percentual
        battery_info.level_percent = map(battery_info.voltage * 100, (int)(BATTERY_MIN_VOLTAGE * 100), (int)(BATTERY_MAX_VOLTAGE * 100), 0, 100);
        battery_info.level_percent = constrain(battery_info.level_percent, 0, 100); // Garante 0-100%
    }
    // Serial.printf("[BATT] V: %.2fV, USB: %d, Nivel: %d%%\n", battery_info.voltage, battery_info.is_usb_powered, battery_info.level_percent);
}

void setScreenBrightness(uint8_t level) {
    // --- Implementação Preferencial: PWM por Hardware ---
    // Descomente e ajuste se souber o canal e pino corretos e tiver configurado o ledc
    /*
    const int ledcChannel = 0; // Canal LEDC (0-15)
    const int ledcResolution = 8; // Resolução em bits (8 = 0-255)
    const double ledcMaxDuty = (1 << ledcResolution) - 1; // Valor máximo (255 para 8 bits)

    // Configura apenas uma vez (pode ser no setup ou aqui com flag)
    static bool ledcConfigured = false;
    if (!ledcConfigured) {
        ledcSetup(ledcChannel, 5000, ledcResolution); // Frequência 5kHz
        ledcAttachPin(PIN_LCD_BL, ledcChannel);      // Anexa pino ao canal
        ledcConfigured = true;
    }

    level = constrain(level, 0, 16); // Garante nível 0-16
    // Mapeia 0-16 para 0-255 (ou qual for a resolução)
    uint32_t dutyCycle = map(level, 0, 16, 0, ledcMaxDuty);
    ledcWrite(ledcChannel, dutyCycle); // Define o brilho
    current_brightness = level;
    // Serial.printf("[BRIGHT] HW PWM Nivel: %d -> Duty: %d\n", level, dutyCycle);
    return; // Sai da função se usou HW PWM
    */

    // --- Fallback: PWM por Software (como estava antes) ---
    const uint8_t MAX_LVL = 16; static uint8_t cur_lvl_int = 0;
    level = constrain(level, 0, MAX_LVL);
    if (level == current_brightness && level != 0 && cur_lvl_int != 0) return; // Otimização

    if (level == 0) { // Desligar
        digitalWrite(PIN_LCD_BL, LOW);
        cur_lvl_int = 0;
    } else { // Ligar ou ajustar
        if (cur_lvl_int == 0) { // Estava desligado?
            digitalWrite(PIN_LCD_BL, HIGH); cur_lvl_int = MAX_LVL; delayMicroseconds(50);
        }
        // Lógica de pulsos para escurecer (original, pode precisar ajuste/causar flicker)
        int from = MAX_LVL - cur_lvl_int; int to = MAX_LVL - level;
        int num_pulses = (MAX_LVL + to - from) % MAX_LVL;
        for (int i = 0; i < num_pulses; i++) {
             digitalWrite(PIN_LCD_BL, LOW); delayMicroseconds(5);
             digitalWrite(PIN_LCD_BL, HIGH); delayMicroseconds(1);
        }
         cur_lvl_int = level; // Atualiza nível interno de pulsação
    }
    current_brightness = level; // Atualiza nível alvo
    // Serial.printf("[BRIGHT] SW PWM Nivel: %d\n", level);
}

void updateScreenBrightness() {
    uint8_t target_brightness;
    if (battery_info.is_usb_powered) {
        target_brightness = BRIGHTNESS_USB; // Mais brilho se no USB
    } else {
        // Verifica inatividade na bateria
        target_brightness = (millis() - last_interaction_time > INACTIVITY_TIMEOUT_MS) ? BRIGHTNESS_DIMMED : BRIGHTNESS_BATTERY;
    }
    setScreenBrightness(target_brightness); // Aplica o brilho calculado
}

// ---- Função de Alteração de Tela ----
void changeScreen(ScreenState new_screen) {
    is_menu_animating = false;          // Para animação do menu se sair dele
    menu_highlight_y_current = -1;      // Reseta posição Y do highlight
    current_screen = new_screen;        // Define a nova tela ativa
    Serial.printf("[SCREEN] Mudando para: %d\n", new_screen);

    // Ao entrar no menu, tenta centralizar a visão no item selecionado
    if (new_screen == SCREEN_MENU_MAIN && NUM_MENU_OPTIONS > 0) {
        menu_top_visible_index = max(0, min(current_menu_index - VISIBLE_MENU_ITEMS / 2, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS));
    }

    ui_drawScreen(true); // Força um redesenho COMPLETO da nova tela
    last_interaction_time = millis(); // Reseta timer de inatividade
}

// ---- Callbacks dos Botões ----
void btn_prev_click() {
    last_interaction_time = millis(); // Reseta inatividade
    bool needs_full_redraw = false; // Flag para redesenho completo da tela atual

    switch (current_screen) {
        case SCREEN_TOTP_VIEW:
            if (service_count > 0) {
                current_service_index = (current_service_index - 1 + service_count) % service_count; // Volta para serviço anterior
                if (decodeCurrentServiceKey()) { // Tenta decodificar nova chave
                    changeScreen(SCREEN_TOTP_VIEW); // Força redraw completo da tela TOTP com novo título/código
                } else {
                    // Erro ao decodificar, a UI mostrará erro, mas força redraw
                    changeScreen(SCREEN_TOTP_VIEW);
                }
            }
            // Não precisa de redraw extra aqui, changeScreen cuida
            break;
        case SCREEN_MENU_MAIN:
            if (NUM_MENU_OPTIONS > 0) {
                int old_visible_pos = current_menu_index - menu_top_visible_index;
                int old_highlight_y = UI_MENU_START_Y + old_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
                current_menu_index = (current_menu_index - 1 + NUM_MENU_OPTIONS) % NUM_MENU_OPTIONS;

                // Lógica de rolagem para cima
                if (current_menu_index < menu_top_visible_index) {
                    menu_top_visible_index = current_menu_index;
                } else if (current_menu_index == NUM_MENU_OPTIONS - 1 && menu_top_visible_index > 0) {
                     menu_top_visible_index = max(0, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS); // Mostra últimos itens
                }
                menu_top_visible_index = max(0, menu_top_visible_index); // Garante não negativo

                // Calcula posição alvo e inicia animação
                int new_visible_pos = current_menu_index - menu_top_visible_index;
                int new_highlight_y = UI_MENU_START_Y + new_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
                if (menu_highlight_y_current != new_highlight_y) {
                     if (menu_highlight_y_current == -1) menu_highlight_y_current = old_highlight_y;
                     menu_highlight_y_target = new_highlight_y;
                     menu_animation_start_time = millis();
                     is_menu_animating = true;
                }
            }
            // O loop cuidará da atualização da animação
            break;
        case SCREEN_TIME_EDIT: // Decrementa valor do campo
            if(edit_time_field == 0) edit_hour = (edit_hour - 1 + 24) % 24;
            else if(edit_time_field == 1) edit_minute = (edit_minute - 1 + 60) % 60;
            else if(edit_time_field == 2) edit_second = (edit_second - 1 + 60) % 60;
            needs_full_redraw = true; // Precisa redesenhar a tela de edição
            break;
        case SCREEN_TIMEZONE_EDIT: // Decrementa offset GMT
            gmt_offset = (gmt_offset - 1 < -12) ? 14 : gmt_offset - 1; // Wrap -12 a +14
            needs_full_redraw = true; // Precisa redesenhar a tela de fuso
            break;
        case SCREEN_LANGUAGE_SELECT: // Navega para idioma anterior
            current_language_menu_index = (current_language_menu_index - 1 + NUM_LANGUAGES) % NUM_LANGUAGES;
            needs_full_redraw = true; // Precisa redesenhar a lista de idiomas
            break;
        case SCREEN_SERVICE_DELETE_CONFIRM: // Cancela exclusão
        case SCREEN_SERVICE_ADD_CONFIRM:    // Cancela adição
            changeScreen(SCREEN_MENU_MAIN); // Volta para o menu
            // changeScreen já faz o redraw
            break;
        default: break; // Nenhuma ação padrão
    }
    // Redesenha a tela atual completamente se a flag foi setada
    if (needs_full_redraw) ui_drawScreen(true);
}

void btn_next_click() {
    last_interaction_time = millis();
    bool needs_full_redraw = false;

    switch (current_screen) {
        case SCREEN_TOTP_VIEW:
            if (service_count > 0) {
                current_service_index = (current_service_index + 1) % service_count; // Avança para próximo serviço
                if (decodeCurrentServiceKey()) {
                    changeScreen(SCREEN_TOTP_VIEW); // Força redraw com novo título/código
                } else {
                    changeScreen(SCREEN_TOTP_VIEW); // Força redraw mesmo com erro
                }
            }
            break;
        case SCREEN_MENU_MAIN:
             if (NUM_MENU_OPTIONS > 0) {
                 int old_visible_pos = current_menu_index - menu_top_visible_index;
                 int old_highlight_y = UI_MENU_START_Y + old_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
                 current_menu_index = (current_menu_index + 1) % NUM_MENU_OPTIONS;

                 // Lógica de rolagem para baixo
                 if (current_menu_index >= menu_top_visible_index + VISIBLE_MENU_ITEMS) {
                     menu_top_visible_index = current_menu_index - VISIBLE_MENU_ITEMS + 1;
                 } else if (current_menu_index == 0 && menu_top_visible_index != 0) {
                     menu_top_visible_index = 0; // Wrap around
                 }
                 menu_top_visible_index = min(menu_top_visible_index, max(0, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS)); // Garante limite

                 // Calcula posição alvo e inicia animação
                 int new_visible_pos = current_menu_index - menu_top_visible_index;
                 int new_highlight_y = UI_MENU_START_Y + new_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
                 if (menu_highlight_y_current != new_highlight_y) {
                     if (menu_highlight_y_current == -1) menu_highlight_y_current = old_highlight_y;
                     menu_highlight_y_target = new_highlight_y;
                     menu_animation_start_time = millis();
                     is_menu_animating = true;
                 }
             }
             break; // Loop cuida da animação
        case SCREEN_TIME_EDIT: // Incrementa valor do campo
            if(edit_time_field == 0) edit_hour = (edit_hour + 1) % 24;
            else if(edit_time_field == 1) edit_minute = (edit_minute + 1) % 60;
            else if(edit_time_field == 2) edit_second = (edit_second + 1) % 60;
            needs_full_redraw = true;
            break;
        case SCREEN_TIMEZONE_EDIT: // Incrementa offset GMT
            gmt_offset = (gmt_offset + 1 > 14) ? -12 : gmt_offset + 1; // Wrap -12 a +14
            needs_full_redraw = true;
            break;
        case SCREEN_LANGUAGE_SELECT: // Navega para próximo idioma
            current_language_menu_index = (current_language_menu_index + 1) % NUM_LANGUAGES;
            needs_full_redraw = true;
            break;
        case SCREEN_SERVICE_DELETE_CONFIRM: // Confirma exclusão
            if(storage_deleteService(current_service_index)) {
                ui_showTemporaryMessage(getText(STR_SERVICE_DELETED), COLOR_SUCCESS, 2000);
                // A mensagem chamará changeScreen para TOTP_VIEW ou MENU
            } else {
                ui_showTemporaryMessage(getText(STR_ERROR_DELETING), COLOR_ERROR, 2000);
                // A mensagem chamará changeScreen para MENU
            }
            break;
        case SCREEN_SERVICE_ADD_CONFIRM: // Confirma adição
             if(storage_saveService(temp_service_name, temp_service_secret)){
                 current_service_index = service_count - 1; // Seleciona o recém-adicionado
                 decodeCurrentServiceKey(); // Decodifica chave
                 ui_showTemporaryMessage(getText(STR_SERVICE_ADDED), COLOR_SUCCESS, 1500); // Mostra sucesso
                 // A mensagem chamará changeScreen(SCREEN_MENU_MAIN), precisamos ir para TOTP
                 // TODO: Melhorar lógica pós-mensagem para ir para tela correta
                 // Por enquanto, a mensagem sempre volta ao menu. O usuário pode navegar para TOTP.
             } else {
                 // Erro ao salvar já mostra mensagem em storage_saveService
                 changeScreen(SCREEN_MENU_MAIN); // Volta ao menu em caso de erro
             }
             break;
        default: break;
    }
    if (needs_full_redraw) ui_drawScreen(true);
}

void btn_next_double_click() {
    last_interaction_time = millis();
    if (current_screen != SCREEN_MENU_MAIN) {
        changeScreen(SCREEN_MENU_MAIN); // Double click sempre volta ao menu
    }
}

void btn_next_long_press_start() {
    last_interaction_time = millis();
    bool change_screen_handled = false; // Flag para controle de redraw/changeScreen

    switch(current_screen) {
        case SCREEN_TOTP_VIEW:
            if (service_count > 0) {
                changeScreen(SCREEN_SERVICE_DELETE_CONFIRM);
                change_screen_handled = true;
            }
            break;
        case SCREEN_MENU_MAIN:
            change_screen_handled = true; // Ação do menu sempre lida com a tela
            switch(menuOptionIDs[current_menu_index]){ // Seleciona ação baseada no ID
                case STR_MENU_ADD_SERVICE:      changeScreen(SCREEN_SERVICE_ADD_WAIT); break;
                case STR_MENU_READ_RFID:        changeScreen(SCREEN_READ_RFID); break;
                case STR_MENU_VIEW_CODES:       if(service_count > 0) changeScreen(SCREEN_TOTP_VIEW); else ui_showTemporaryMessage(getText(STR_ERROR_NO_SERVICES), COLOR_ACCENT, 2000); break;
                case STR_MENU_ADJUST_TIME:      edit_hour=hour(); edit_minute=minute(); edit_second=second(); edit_time_field=0; changeScreen(SCREEN_TIME_EDIT); break;
                case STR_MENU_ADJUST_TIMEZONE:  changeScreen(SCREEN_TIMEZONE_EDIT); break;
                case STR_MENU_SELECT_LANGUAGE:  current_language_menu_index = current_language; changeScreen(SCREEN_LANGUAGE_SELECT); break;
            }
            break;
        case SCREEN_TIME_EDIT: // Avança campo ou salva
            edit_time_field++;
            if (edit_time_field > 2) { // Passou dos segundos, salvar
                setTime(edit_hour, edit_minute, edit_second, day(), month(), year()); // Salva UTC
                updateRTCFromSystem(); // Atualiza RTC
                time_t local_t = now() + gmt_offset * 3600; // Calcula hora local para msg
                snprintf(message_buffer, sizeof(message_buffer), getText(STR_TIME_ADJUSTED_FMT), hour(local_t), minute(local_t), second(local_t));
                ui_showTemporaryMessage(message_buffer, COLOR_SUCCESS, 2500);
                change_screen_handled = true; // Mensagem cuida da transição
            } else {
                ui_drawScreen(true); // Apenas redesenha para mostrar novo campo ativo
            }
            break;
        case SCREEN_TIMEZONE_EDIT: // Salva fuso horário
            preferences.begin("totp-app", false); // Abre NVS para escrita
            preferences.putInt(NVS_TZ_OFFSET_KEY, gmt_offset);
            preferences.end();
            Serial.printf("[NVS] Fuso salvo: GMT%+d\n", gmt_offset);
            snprintf(message_buffer, sizeof(message_buffer), getText(STR_TIMEZONE_SAVED_FMT), gmt_offset);
            ui_showTemporaryMessage(message_buffer, COLOR_SUCCESS, 2000);
            change_screen_handled = true; // Mensagem cuida da transição
            break;
        case SCREEN_LANGUAGE_SELECT: // Salva idioma selecionado
             change_screen_handled = true; // Sempre lida com a tela
             if (current_language_menu_index != current_language) { // Se mudou
                 current_language = (Language)current_language_menu_index;
                 current_strings = languages[current_language]; // Atualiza ponteiro global
                 preferences.begin("totp-app", false);
                 preferences.putInt(NVS_LANG_KEY, current_language);
                 preferences.end();
                 Serial.printf("[LANG] Idioma salvo: %d\n", current_language);
                 // Mostra msg de sucesso e força redraw completo da tela anterior (menu)
                 // A função de mensagem chamará changeScreen(SCREEN_MENU_MAIN)
                 ui_showTemporaryMessage(getText(STR_LANG_SAVED), COLOR_SUCCESS, 1500);

             } else {
                 changeScreen(SCREEN_MENU_MAIN); // Volta se não mudou
             }
             break;
        default: break;
    }
    // Evita redesenho extra desnecessário se uma ação já mudou a tela ou mostrou mensagem
    // if (!change_screen_handled) ui_drawScreen(true);
}


// ---- Funções de Desenho da Interface ----
void ui_drawScreen(bool full_redraw) {
    // Tela de mensagem é especial, sem header padrão
    if (current_screen == SCREEN_MESSAGE) {
        ui_drawScreenMessage(full_redraw);
        return;
    }

    // --- Gerenciamento do Header Unificado ---
    if (full_redraw) {
        // Desenha fundo e título estático do header apenas no redraw completo
        ui_drawHeader(getScreenTitle(current_screen));
    }
    // Atualiza relógio e bateria (partes dinâmicas) SEMPRE
    ui_updateHeaderDynamic();

    // --- Desenho do Conteúdo Específico da Tela (Abaixo do Header) ---
    switch(current_screen){
        case SCREEN_TOTP_VIEW:              ui_drawScreenTOTPContent(full_redraw); break;
        case SCREEN_MENU_MAIN:              ui_drawScreenMenuContent(full_redraw); break;
        case SCREEN_SERVICE_ADD_WAIT:       ui_drawScreenServiceAddWaitContent(full_redraw); break;
        case SCREEN_SERVICE_ADD_CONFIRM:    ui_drawScreenServiceAddConfirmContent(full_redraw); break;
        case SCREEN_TIME_EDIT:              ui_drawScreenTimeEditContent(full_redraw); break;
        case SCREEN_SERVICE_DELETE_CONFIRM: ui_drawScreenServiceDeleteConfirmContent(full_redraw); break;
        case SCREEN_TIMEZONE_EDIT:          ui_drawScreenTimezoneEditContent(full_redraw); break;
        case SCREEN_LANGUAGE_SELECT:        ui_drawScreenLanguageSelectContent(full_redraw); break;
        case SCREEN_READ_RFID:              ui_drawScreenReadRFID(full_redraw); break;
        default: // Tela desconhecida, limpa área de conteúdo
            if(full_redraw) tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG);
            break;
    }
}

const char* getScreenTitle(ScreenState state) {
    // Para TOTP, o título é o nome do serviço (ou um default)
    if (state == SCREEN_TOTP_VIEW) {
        return (service_count > 0 && current_service_index < service_count)
               ? services[current_service_index].name
               : getText(STR_TOTP_NO_SERVICE_TITLE);
    }
    // Para outras telas, usa o ID de string correspondente
    StringID title_id;
    switch(state){
        case SCREEN_MENU_MAIN:              title_id = STR_TITLE_MAIN_MENU; break;
        case SCREEN_SERVICE_ADD_WAIT:       title_id = STR_TITLE_ADD_SERVICE; break;
        case SCREEN_SERVICE_ADD_CONFIRM:    title_id = STR_TITLE_CONFIRM_ADD; break;
        case SCREEN_TIME_EDIT:              title_id = STR_TITLE_ADJUST_TIME; break;
        case SCREEN_SERVICE_DELETE_CONFIRM: title_id = STR_TITLE_CONFIRM_DELETE; break;
        case SCREEN_TIMEZONE_EDIT:          title_id = STR_TITLE_ADJUST_TIMEZONE; break;
        case SCREEN_LANGUAGE_SELECT:        title_id = STR_TITLE_SELECT_LANGUAGE; break;
        default:                            return ""; // Sem título para mensagem ou desconhecido
    }
    return getText(title_id); // Retorna o texto traduzido
}

void ui_drawHeader(const char *title) {
    // Desenha fundo da barra do header
    tft.fillRect(0, 0, SCREEN_WIDTH, UI_HEADER_HEIGHT, COLOR_ACCENT);

    // Guarda estado atual do texto para restaurar depois
    uint8_t prev_datum = tft.getTextDatum();
    uint16_t prev_fg = tft.textcolor;
    uint16_t prev_bg = tft.textbgcolor;
    uint8_t prev_size = tft.textsize;

    // Configura texto para o título
    tft.setTextColor(COLOR_BG, COLOR_ACCENT); // Texto escuro sobre fundo ciano
    tft.setTextDatum(ML_DATUM);              // Alinha pela Middle Left
    tft.setTextSize(2);                      // Tamanho 2 para título

    // Truncamento simples do título se for muito longo para caber
    char truncated_title[MAX_SERVICE_NAME_LEN + 4]; // +4 para "..." e \0
    int max_chars = MAX_SERVICE_NAME_LEN; // Limite baseado na constante
    if (strlen(title) > max_chars) {
        strncpy(truncated_title, title, max_chars);
        strcpy(truncated_title + max_chars, "..."); // Adiciona reticências
    } else {
        strcpy(truncated_title, title); // Copia título inteiro
    }

    // Desenha o título (truncado se necessário) com padding esquerdo
    tft.drawString(truncated_title, UI_PADDING, UI_HEADER_HEIGHT / 2);

    // Restaura estado anterior do texto
    tft.setTextDatum(prev_datum);
    tft.setTextColor(prev_fg, prev_bg);
    tft.setTextSize(prev_size);
}

void ui_updateHeaderDynamic() {
    // Chama as funções para desenhar/atualizar relógio e bateria
    ui_drawClockDirect(true); // Mostra segundos no header
    ui_drawBatteryDirect();
}

// Desenha relógio (sua função, ajustada para MR_DATUM e posição)
void ui_drawClockDirect(bool show_seconds) {
    time_t now_utc = now();
    time_t local_time = now_utc + gmt_offset * 3600; // Calcula hora local
    // Guarda estado do texto
    uint8_t prev_datum = tft.getTextDatum();
    uint16_t prev_fg = tft.textcolor, prev_bg = tft.textbgcolor;
    uint8_t prev_size = tft.textsize;

    char time_str[9]; // "HH:MM:SS\0"
    int textWidth;
    int textHeight = tft.fontHeight(2); // Altura para tamanho 2

    // Formata a string e calcula a largura necessária
    if (show_seconds) {
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour(local_time), minute(local_time), second(local_time));
        textWidth = tft.textWidth("00:00:00", 2); // Usa string máxima para limpar área
    } else {
        snprintf(time_str, sizeof(time_str), "%02d:%02d", hour(local_time), minute(local_time));
        textWidth = tft.textWidth("00:00", 2);
    }

    // Calcula posição X para alinhar à DIREITA antes da bateria
    int x_right_edge = SCREEN_WIDTH - UI_BATT_WIDTH - UI_PADDING * 2;
    int y_center = UI_HEADER_HEIGHT / 2; // Centro vertical do header

    // Configura texto e desenha
    tft.setTextColor(COLOR_BG, COLOR_ACCENT); // Texto BG sobre fundo ACCENT
    tft.setTextDatum(MR_DATUM);              // Alinha pela Middle Right
    tft.setTextSize(2);
    // Limpa a área EXATA onde o texto será desenhado usando a cor de fundo
    tft.fillRect(x_right_edge - textWidth, y_center - textHeight / 2, textWidth, textHeight, COLOR_ACCENT);
    // Desenha a string alinhada à direita na posição x_right_edge
    tft.drawString(time_str, x_right_edge, y_center);

    // Restaura estado do texto
    tft.setTextDatum(prev_datum);
    tft.setTextColor(prev_fg, prev_bg);
    tft.setTextSize(prev_size);
}

// Desenha bateria (sua função, posição ajustada)
void ui_drawBatteryDirect() {
    // Calcula posição X para ficar na extremidade direita
    int x = SCREEN_WIDTH - UI_BATT_WIDTH - UI_PADDING;
    int y = (UI_HEADER_HEIGHT - UI_BATT_HEIGHT) / 2; // Centraliza Y no header
    int icon_w = UI_BATT_WIDTH - 2, icon_h = UI_BATT_HEIGHT - 2;
    int icon_x = x + 1, icon_y = y + 1;
    int term_w = 2, term_h = icon_h / 2;
    int term_x = icon_x + icon_w, term_y = icon_y + (icon_h - term_h) / 2;
    uint16_t fg_color = COLOR_BG, bg_color = COLOR_ACCENT; // Cores para o header

    tft.fillRect(x, y, UI_BATT_WIDTH, UI_BATT_HEIGHT, bg_color); // Limpa área do ícone

    if (battery_info.is_usb_powered) { // Ícone USB/Raio
        tft.drawRect(icon_x, icon_y, icon_w, icon_h, fg_color); // Contorno
        tft.fillRect(term_x, term_y, term_w, term_h, fg_color); // Terminal
        // Desenho do raio (simplificado para clareza)
        int bolt_x = icon_x + icon_w / 2 - 3, bolt_y = icon_y + 1;
        int bolt_h_half = (icon_h > 2) ? (icon_h - 2) / 2 : 0;
        int bolt_y_mid = bolt_y + bolt_h_half;
        // Dentro de ui_drawBatteryDirect, no bloco if (battery_info.is_usb_powered):
        int bolt_y_end = (icon_h > 2) ? (icon_y + icon_h - 2) : bolt_y; // <<-- CORREÇÃO AQUI (icon_y em vez de iy)
        tft.drawLine(bolt_x + 3, bolt_y, bolt_x, bolt_y_mid, fg_color);
        tft.drawLine(bolt_x, bolt_y_mid, bolt_x + 3, bolt_y_mid, fg_color);
        tft.drawLine(bolt_x + 3, bolt_y_mid, bolt_x, bolt_y_end, fg_color);
    } else { // Ícone Bateria Normal
        tft.drawRect(icon_x, icon_y, icon_w, icon_h, fg_color); // Contorno
        tft.fillRect(term_x, term_y, term_w, term_h, fg_color); // Terminal
        // Calcula preenchimento
        int fill_w_max = (icon_w > 2) ? (icon_w - 2) : 0; // Largura interna útil
        int fill_h = (icon_h > 2) ? (icon_h - 2) : 0;     // Altura interna útil
        int fill_w = fill_w_max * battery_info.level_percent / 100;
        fill_w = constrain(fill_w, 0, fill_w_max);
        // Define cor do preenchimento
        uint16_t fill_color = COLOR_SUCCESS; // Verde por padrão
        if (battery_info.level_percent < 50) fill_color = TFT_YELLOW; // Amarelo se < 50%
        if (battery_info.level_percent < 20) fill_color = COLOR_ERROR;  // Vermelho se < 20%
        // Desenha preenchimento
        if (fill_w > 0) tft.fillRect(icon_x + 1, icon_y + 1, fill_w, fill_h, fill_color);
        // Limpa a parte não preenchida com a cor de fundo do header
        if (fill_w < fill_w_max) tft.fillRect(icon_x + 1 + fill_w, icon_y + 1, fill_w_max - fill_w, fill_h, bg_color);
    }
}

// ---- Update Sprites ----
void ui_updateProgressBarSprite(uint64_t current_timestamp_utc) {
    uint32_t seconds_remaining = TOTP_INTERVAL - (current_timestamp_utc % TOTP_INTERVAL);
    int bar_width = spr_progress_bar.width();
    int bar_height = spr_progress_bar.height();
    int progress_w = map(seconds_remaining, 0, TOTP_INTERVAL, 0, bar_width);
    progress_w = constrain(progress_w, 0, bar_width);

    spr_progress_bar.fillRect(0, 0, bar_width, bar_height, COLOR_BAR_BG); // Fundo
    if (progress_w > 0) {
        spr_progress_bar.fillRect(0, 0, progress_w, bar_height, COLOR_BAR_FG); // Preenchimento
    }
}

void ui_updateTOTPCodeSprite() {
    spr_totp_code.fillSprite(COLOR_BG); // Limpa sprite
    spr_totp_code.setTextColor(COLOR_FG, COLOR_BG); // Define cores
    spr_totp_code.setTextSize(UI_TOTP_CODE_FONT_SIZE); // Define tamanho
    // Desenha o código atual ou a string de erro/placeholder
    spr_totp_code.drawString(current_totp.code, spr_totp_code.width() / 2, spr_totp_code.height() / 2);
}

// ---- Telas Específicas de UI (Conteúdo Abaixo do Header) ----
void ui_drawScreenTOTPContent(bool full_redraw) {
    // Calcula área útil e posições centralizadas
    int footer_height = tft.fontHeight(1) + UI_PADDING;
    int content_y_start = UI_HEADER_HEIGHT;
    int content_height = SCREEN_HEIGHT - content_y_start - footer_height;
    int content_center_y = content_y_start + content_height / 2;
    int totp_code_sprite_y = content_center_y - spr_totp_code.height() / 2; // Centraliza sprite do código
    int progress_bar_y = totp_code_sprite_y + spr_totp_code.height() + 10; // Barra abaixo do código

    if (full_redraw) {
        tft.fillRect(0, content_y_start, SCREEN_WIDTH, content_height + footer_height, COLOR_BG); // Limpa área
        // Desenha contorno da barra na posição calculada
        tft.drawRoundRect(UI_PROGRESS_BAR_X - 2, progress_bar_y - 2, UI_PROGRESS_BAR_WIDTH + 4, UI_PROGRESS_BAR_HEIGHT + 4, 4, COLOR_FG);
        // Desenha rodapé
        tft.setTextDatum(BC_DATUM); tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1);
        tft.drawString(getText(STR_FOOTER_GENERIC_NAV), SCREEN_WIDTH / 2, UI_FOOTER_TEXT_Y);
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); // Reset
    }

    // Atualiza e desenha sprites
    uint64_t current_unix_time_utc = now();
    ui_updateTOTPCodeSprite();
    ui_updateProgressBarSprite(current_unix_time_utc);
    spr_totp_code.pushSprite((SCREEN_WIDTH - spr_totp_code.width()) / 2, totp_code_sprite_y); // Push na posição Y centralizada
    spr_progress_bar.pushSprite(UI_PROGRESS_BAR_X, progress_bar_y); // Push na posição Y abaixo
}

void ui_drawScreenMenuContent(bool full_redraw) {
    int item_y = UI_MENU_START_Y;
    int item_spacing = 5;
    int item_total_height = UI_MENU_ITEM_HEIGHT + item_spacing;

    // --- Calcula posição da animação ---
    if (is_menu_animating) {
        unsigned long elapsed = millis() - menu_animation_start_time;
        if (elapsed >= MENU_ANIMATION_DURATION_MS) {
            menu_highlight_y_current = menu_highlight_y_target;
            is_menu_animating = false; // Termina animação
        } else {
            // Interpolação linear simples
            float progress = (float)elapsed / MENU_ANIMATION_DURATION_MS;
            menu_highlight_y_current = menu_highlight_y_target + (int)((menu_highlight_y_current - menu_highlight_y_target) * (1.0f - progress));
        }
    } else if (menu_highlight_y_current == -1 && NUM_MENU_OPTIONS > 0) {
         // Define posição inicial se ainda não definida
         int initial_visible_pos = current_menu_index - menu_top_visible_index;
         menu_highlight_y_current = UI_MENU_START_Y + initial_visible_pos * item_total_height;
         menu_highlight_y_target = menu_highlight_y_current; // Evita animação inicial
    }

    // --- Limpeza e Desenho dos Itens Visíveis ---
    if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG); // Limpa toda área de conteúdo
        // Desenha rodapé estático
        tft.setTextDatum(BL_DATUM); tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1);
        char status_buf[40]; snprintf(status_buf, sizeof(status_buf), getText(STR_SERVICES_STATUS_FMT), service_count, MAX_SERVICES);
        tft.drawString(status_buf, UI_PADDING, UI_FOOTER_TEXT_Y);
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
    } else {
         // Limpa apenas a área dos itens visíveis + highlight para atualização parcial
         tft.fillRect(UI_PADDING, item_y, SCREEN_WIDTH - 2 * UI_PADDING, VISIBLE_MENU_ITEMS * item_total_height, COLOR_BG);
    }

    // Desenha o retângulo de destaque animado
     if (menu_highlight_y_current != -1 && NUM_MENU_OPTIONS > 0) {
        tft.fillRect(UI_PADDING, menu_highlight_y_current, SCREEN_WIDTH - 2 * UI_PADDING - 10, UI_MENU_ITEM_HEIGHT, COLOR_HIGHLIGHT_BG); // Deixa espaço para scrollbar
     }

    // Desenha os itens do menu visíveis
    tft.setTextSize(2);
    tft.setTextDatum(ML_DATUM);
    int draw_count = 0;
    for (int i = menu_top_visible_index; i < NUM_MENU_OPTIONS && draw_count < VISIBLE_MENU_ITEMS; ++i) {
        int current_item_draw_y = item_y + draw_count * item_total_height;
        // Define cor do texto: invertido se for o item selecionado (mesmo durante animação)
        tft.setTextColor((i == current_menu_index) ? COLOR_HIGHLIGHT_FG : COLOR_FG);
        // Desenha texto do item usando ID e getText
        tft.drawString(getText(menuOptionIDs[i]), UI_PADDING * 3, current_item_draw_y + UI_MENU_ITEM_HEIGHT / 2);
        draw_count++;
    }

    // --- Desenha Barra de Rolagem ---
    if (NUM_MENU_OPTIONS > VISIBLE_MENU_ITEMS) {
        int scrollbar_x = SCREEN_WIDTH - UI_PADDING - 6; // Posição X da barra
        int scrollbar_w = 4; // Largura
        int scrollbar_y = UI_MENU_START_Y; // Y inicial
        int scrollbar_h = VISIBLE_MENU_ITEMS * item_total_height - item_spacing; // Altura da área visível

        tft.fillRect(scrollbar_x, scrollbar_y, scrollbar_w, scrollbar_h, TFT_DARKGREY); // Trilho

        int thumb_h = max(5, scrollbar_h * VISIBLE_MENU_ITEMS / NUM_MENU_OPTIONS); // Altura proporcional
        int thumb_max_y = scrollbar_h - thumb_h; // Deslocamento máximo
        // Calcula posição Y do indicador baseado no item do topo visível
        int thumb_y = scrollbar_y;
        if (NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS > 0) { // Evita divisão por zero
             thumb_y += round((float)thumb_max_y * menu_top_visible_index / (NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS));
        }
        thumb_y = constrain(thumb_y, scrollbar_y, scrollbar_y + thumb_max_y); // Garante limites

        tft.fillRect(scrollbar_x, thumb_y, scrollbar_w, thumb_h, COLOR_ACCENT); // Indicador
    }

    // Reseta configurações de texto
    tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); tft.setTextSize(1);
}

void ui_drawScreenServiceAddWaitContent(bool full_redraw) {
    if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG); // Limpa conteúdo
        tft.setTextColor(COLOR_FG); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
        int center_y = UI_HEADER_HEIGHT + (SCREEN_HEIGHT - UI_HEADER_HEIGHT) / 2;
        tft.drawString(getText(STR_AWAITING_JSON), SCREEN_WIDTH / 2, center_y - 30);
        tft.drawString(getText(STR_VIA_SERIAL), SCREEN_WIDTH / 2, center_y );
        tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY);
        tft.drawString(getText(STR_EXAMPLE_JSON_SERVICE), SCREEN_WIDTH / 2, center_y + 30);
        tft.setTextDatum(BC_DATUM); tft.drawString(getText(STR_FOOTER_GENERIC_NAV), SCREEN_WIDTH/2, UI_FOOTER_TEXT_Y);
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
    }
}

void ui_drawScreenServiceAddConfirmContent(bool full_redraw) {
     if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG);
        tft.setTextColor(COLOR_ACCENT); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
        int center_y = UI_HEADER_HEIGHT + (SCREEN_HEIGHT - UI_HEADER_HEIGHT) / 3;
        tft.drawString(getText(STR_CONFIRM_ADD_PROMPT), SCREEN_WIDTH / 2, center_y);
        tft.setTextColor(COLOR_FG); tft.drawString(temp_service_name, SCREEN_WIDTH / 2, center_y + 35);
        tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1); tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(STR_FOOTER_CONFIRM_NAV), SCREEN_WIDTH / 2, UI_FOOTER_TEXT_Y);
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
     }
}

void ui_drawScreenTimeEditContent(bool full_redraw) {
    if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG); // Limpa conteúdo
        tft.setTextColor(TFT_DARKGREY); 
        tft.setTextSize(1); 
        tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(STR_FOOTER_TIME_EDIT_NAV), SCREEN_WIDTH / 2, UI_FOOTER_TEXT_Y);
        tft.setTextDatum(MC_DATUM); 
        // Info Fuse
        char tz_info_buf[40]; 
        snprintf(tz_info_buf, sizeof(tz_info_buf), getText(STR_TIME_EDIT_INFO_FMT), gmt_offset);
        tft.drawString(tz_info_buf, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50);
        // JSON hint
        tft.drawString(getText(STR_TIME_EDIT_JSON_HINT), SCREEN_WIDTH/2, SCREEN_HEIGHT - 35);
        tft.drawString(getText(STR_EXAMPLE_JSON_TIME), SCREEN_WIDTH/2, SCREEN_HEIGHT - 20);
        // reset
        tft.setTextDatum(TL_DATUM); 
        tft.setTextColor(COLOR_FG, COLOR_BG);
    }

    tft.setTextDatum(MC_DATUM); 
    tft.setTextColor(COLOR_FG, COLOR_BG);
    uint8_t time_font_size = 3; 
    tft.setTextSize(time_font_size);
    int text_height = tft.fontHeight(time_font_size);
    int content_center_y = UI_HEADER_HEIGHT + (SCREEN_HEIGHT - UI_HEADER_HEIGHT) / 2;
    int time_y_pos = content_center_y - text_height / 2 ;
    
    // Marcador variables
    int field_width = tft.textWidth("00"); 
    int separator_width = tft.textWidth(":"); 
    int total_width = 3 * field_width + 2 * separator_width;
    int start_x = (SCREEN_WIDTH - total_width) / 2; 
    int marker_y = time_y_pos - 17; // Acima do texto
    int marker_height = 4;
    int marker_x = start_x; 
    if (edit_time_field == 1) marker_x = start_x + field_width + separator_width; 
    else if (edit_time_field == 2) marker_x = start_x + 2 * (field_width + separator_width);

    // Limpa área maior para hora marcador
    tft.fillRect(0, marker_y - 5, SCREEN_WIDTH, text_height + 30, COLOR_BG);
     char time_str_buf[12]; snprintf(time_str_buf, sizeof(time_str_buf), "%02d:%02d:%02d", edit_hour, edit_minute, edit_second);
    tft.drawString(time_str_buf, SCREEN_WIDTH / 2, time_y_pos);
    // Marcador draw
    tft.fillRect(marker_x, marker_y, field_width, marker_height, COLOR_ACCENT);
    // Reset
    tft.setTextDatum(TL_DATUM); tft.setTextSize(1); tft.setTextColor(COLOR_FG, COLOR_BG);
}

void ui_drawScreenServiceDeleteConfirmContent(bool full_redraw) {
     if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG);
        tft.setTextColor(COLOR_ERROR); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
        int center_y = UI_HEADER_HEIGHT + (SCREEN_HEIGHT - UI_HEADER_HEIGHT) / 3;
        tft.drawString(getText(STR_CONFIRM_DELETE_PROMPT), SCREEN_WIDTH / 2, center_y);
        tft.setTextColor(COLOR_FG);
        if (service_count > 0 && current_service_index < service_count) tft.drawString(services[current_service_index].name, SCREEN_WIDTH / 2, center_y + 35);
        else tft.drawString("???", SCREEN_WIDTH / 2, center_y + 35); // Fallback
        tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1); tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(STR_FOOTER_CONFIRM_NAV), SCREEN_WIDTH / 2, UI_FOOTER_TEXT_Y);
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
     }
}

void ui_drawScreenTimezoneEditContent(bool full_redraw) {
    if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG);
        tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1); tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(STR_FOOTER_TIMEZONE_NAV), SCREEN_WIDTH / 2, UI_FOOTER_TEXT_Y);
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
    }
    tft.setTextDatum(MC_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); tft.setTextSize(3);
    int content_center_y = UI_HEADER_HEIGHT + (SCREEN_HEIGHT - UI_HEADER_HEIGHT) / 2;
    int text_h = tft.fontHeight(3);
    tft.fillRect(0, content_center_y - text_h/2 - 5, SCREEN_WIDTH, text_h + 10, COLOR_BG); // Limpa área texto
    char tz_str[10]; snprintf(tz_str, sizeof(tz_str), getText(STR_TIMEZONE_LABEL), gmt_offset);
    tft.drawString(tz_str, SCREEN_WIDTH / 2, content_center_y);
    tft.setTextDatum(TL_DATUM); tft.setTextSize(1);
}

void ui_drawScreenLanguageSelectContent(bool full_redraw) {
    if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG); // Limpa conteúdo
        tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1); tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(STR_FOOTER_LANG_NAV), SCREEN_WIDTH / 2, UI_FOOTER_TEXT_Y); // Rodapé
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
    }

    // Desenha lista de idiomas
    tft.setTextSize(2);
    int item_y = UI_MENU_START_Y;
    int item_total_height = UI_MENU_ITEM_HEIGHT + 5;
    const StringID langNames[] = {STR_LANG_PORTUGUESE, STR_LANG_ENGLISH}; // Mapeia índice para StringID

    for (int i = 0; i < NUM_LANGUAGES; i++) {
        int current_item_draw_y = item_y + i * item_total_height;
        // Define cores baseado na seleção e idioma atual
        if (i == current_language_menu_index) { // Item atualmente selecionado pelo cursor
            tft.fillRect(UI_PADDING, current_item_draw_y, SCREEN_WIDTH - 2 * UI_PADDING, UI_MENU_ITEM_HEIGHT, COLOR_HIGHLIGHT_BG);
            tft.setTextColor(COLOR_HIGHLIGHT_FG);
        } else { // Item não selecionado pelo cursor
             tft.fillRect(UI_PADDING, current_item_draw_y, SCREEN_WIDTH - 2 * UI_PADDING, UI_MENU_ITEM_HEIGHT, COLOR_BG);
             tft.setTextColor(COLOR_FG);
             // Adiciona um marcador '*' se for o idioma ATUALMENTE ATIVO, mas não selecionado
             if (i == current_language) {
                 uint16_t prev_fg = tft.textcolor; // Salva cor atual
                 tft.setTextColor(COLOR_ACCENT); // Cor do marcador
                 tft.drawString("*", SCREEN_WIDTH - UI_PADDING * 4, current_item_draw_y + UI_MENU_ITEM_HEIGHT / 2);
                 tft.setTextColor(prev_fg); // Restaura cor
             }
        }
        // Desenha nome do idioma
        tft.setTextDatum(ML_DATUM);
        if (i < sizeof(langNames) / sizeof(langNames[0])) {
            tft.drawString(getText(langNames[i]), UI_PADDING * 3, current_item_draw_y + UI_MENU_ITEM_HEIGHT / 2);
        } else {
            tft.drawString("???", UI_PADDING * 3, current_item_draw_y + UI_MENU_ITEM_HEIGHT / 2); // Fallback
        }
    }
    // Reseta configurações de texto
    tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); tft.setTextSize(1);
}

void ui_drawScreenMessage(bool full_redraw) {
     if (full_redraw) {
         tft.fillScreen(COLOR_BG); // Limpa TUDO (sem header)
         tft.setTextColor(message_color, COLOR_BG); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
         // Desenha mensagem (com suporte a quebra de linha \n)
         char temp_buffer[sizeof(message_buffer)]; strncpy(temp_buffer, message_buffer, sizeof(temp_buffer)); temp_buffer[sizeof(temp_buffer)-1] = '\0';
         char *line1 = strtok(temp_buffer, "\n"), *line2 = strtok(NULL, "\n");
         int y_pos = SCREEN_HEIGHT / 2, line_height = tft.fontHeight(2) + 5;
         if (line1 && line2) { // Duas linhas
             y_pos -= line_height / 2; // Ajusta Y para centralizar
             tft.drawString(line1, SCREEN_WIDTH / 2, y_pos);
             tft.drawString(line2, SCREEN_WIDTH / 2, y_pos + line_height);
         } else if (line1) { // Apenas uma linha
             tft.drawString(line1, SCREEN_WIDTH / 2, y_pos);
         }
         // Reseta
         tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); tft.setTextSize(1);
     }
}

void ui_drawScreenReadRFID(bool full_redraw) {
    if (full_redraw) {
        tft.fillScreen(COLOR_BG); // Limpa TUDO (sem header)
        tft.setTextColor(COLOR_FG, COLOR_BG); 
        tft.setTextSize(2); 
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Aproxime Cartao", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        
        // Exibe ID do cartão apenas se já tiver sido lido
        if(strlen(card_id) > 0) {
            char display_text[32];
            sprintf(display_text, "ID: %s", card_id);
            tft.drawString(display_text, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 30);
        }
        
        // Reseta
        tft.setTextDatum(TL_DATUM); 
        tft.setTextColor(COLOR_FG, COLOR_BG); 
        tft.setTextSize(1);
    }
}

// ---- Mensagem Temporária ----
void ui_showTemporaryMessage(const char *msg, uint16_t color, uint32_t duration_ms) {
    ScreenState previous_screen = current_screen; // Salva tela anterior
    // Copia mensagem para buffer global
    strncpy(message_buffer, msg, sizeof(message_buffer) - 1); message_buffer[sizeof(message_buffer) - 1] = '\0';
    message_color = color;

    changeScreen(SCREEN_MESSAGE); // Mostra tela de mensagem

    // Espera ou interação (botões não interrompem aqui, mas poderiam)
    uint32_t start_time = millis();
    while (millis() - start_time < duration_ms) {
         // btn_prev.tick(); btn_next.tick(); // Permitir interação?
         delay(50);
    }

    // Decide para qual tela voltar
    // Se a tela anterior era uma confirmação, edição ou a própria msg, volta ao MENU
    if (previous_screen == SCREEN_MESSAGE || previous_screen == SCREEN_SERVICE_ADD_CONFIRM ||
        previous_screen == SCREEN_SERVICE_DELETE_CONFIRM || previous_screen == SCREEN_TIME_EDIT ||
        previous_screen == SCREEN_SERVICE_ADD_WAIT || previous_screen == SCREEN_TIMEZONE_EDIT ||
        previous_screen == SCREEN_LANGUAGE_SELECT)
    {
        // Caso especial: Após adicionar serviço com sucesso, ir para TOTP
        if (previous_screen == SCREEN_SERVICE_ADD_CONFIRM && color == COLOR_SUCCESS) {
             changeScreen(SCREEN_TOTP_VIEW); // Vai para a tela TOTP ver o novo código
        } else {
             changeScreen(SCREEN_MENU_MAIN); // Volta para o menu nos outros casos
        }
    } else {
         changeScreen(previous_screen); // Volta para onde estava (provavelmente TOTP_VIEW)
    }
}

void readRFIDCard(){
     // Verifica se há novos cartões
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Verifica se conseguiu ler o cartão
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Formata o UID do cartão para a variável global 'card_id'
  sprintf(card_id, "%02X%02X%02X%02X",
    mfrc522.uid.uidByte[0],
    mfrc522.uid.uidByte[1],
    mfrc522.uid.uidByte[2],
    mfrc522.uid.uidByte[3]);

    // Após ler o cartão, atualiza a tela imediatamente
    ui_drawScreenReadRFID(true);

    delay(500);  // evita leitura duplicada
}

// FIM DO CÓDIGO

// #include <Arduino.h>
// #include <SPI.h>
// #include <Wire.h>
// #include <RTClib.h>
// #include <TimeLib.h>
// #include <TFT_eSPI.h>
// #include "OneButton.h"
// #include <Preferences.h>
// #include <ArduinoJson.h>
// #include "mbedtls/md.h"
// #include "pin_config.h" // SEU ARQUIVO DE CONFIGURAÇÃO DE PINOS
// #include <MFRC522.h>



// MFRC522 mfrc522(SS_PIN, RST_PIN); 

// void setup() {
//   Serial.begin(115200); 
//   while (!Serial);

//   // Inicializa SPI (SCK, MISO, MOSI, SS)
//   SPI.begin(10, 12, 11, SS_PIN);
//   mfrc522.PCD_Init(); 
  
//   Serial.println("Coloque o cartão RFID próximo ao leitor...");
//   Serial.print("Versão do firmware MFRC522: 0x");
//   Serial.println(mfrc522.PCD_ReadRegister(mfrc522.VersionReg), HEX);
// }

// void loop() {
//   // Verifica se há novos cartões
//   if (!mfrc522.PICC_IsNewCardPresent()) {
//     return;
//   }

//   // Verifica se conseguiu ler o cartão
//   if (!mfrc522.PICC_ReadCardSerial()) {
//     return;
//   }

//   // Exibe UID do cartão
//   Serial.print("UID do cartão: ");
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
//     Serial.print(mfrc522.uid.uidByte[i], HEX);
//   } 
//   Serial.println();

//   delay(1000);
// }
