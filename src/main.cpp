/*
  TOTP Authenticator Optimized v4
  Projeto para ESP32 com TFT, RTC DS3231 e geração TOTP.
  Melhorias:
   - Otimização de uso de memória e buffers
   - Atualização parcial da tela para eliminar flicker
   - Código modularizado e legível
   - Tratamento robusto de erros e validação de entradas via Serial
   - Gestão inteligente de energia (dimming e potencial sleep)
   - Experiência aprimorada com animações e transições suaves
*/

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <TFT_eSPI.h>
#include "OneButton.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include "mbedtls/md.h"
#include "pin_config.h"

// ---- Constantes e Configurações ----
constexpr uint32_t TOTP_INTERVAL = 30;
constexpr int MAX_SERVICES = 50;
constexpr size_t MAX_SERVICE_NAME_LEN = 20;
constexpr size_t MAX_SECRET_B32_LEN = 64;
constexpr size_t MAX_SECRET_BIN_LEN = 40;

constexpr int VISIBLE_MENU_ITEMS = 3;
constexpr uint32_t INACTIVITY_TIMEOUT_MS = 30000;
constexpr uint32_t RTC_SYNC_INTERVAL_MS = 60000;
constexpr uint32_t SCREEN_UPDATE_INTERVAL_MS = 1000;
constexpr int MENU_ANIMATION_DURATION_MS = 150;

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
#define UI_CLOCK_WIDTH_ESTIMATE 60 // Largura estimada para relógio HH:MM
#define UI_BATT_WIDTH 24
#define UI_BATT_HEIGHT 16 // Mantido da sua versão
#define UI_CONTENT_Y_START (UI_HEADER_HEIGHT + UI_PADDING) // Y inicial para conteúdo abaixo do header
#define UI_TOTP_CODE_Y (UI_CONTENT_Y_START + 5)  // Ajustado Y para conteúdo
#define UI_TOTP_CODE_FONT_SIZE 3
#define UI_PROGRESS_BAR_Y (UI_TOTP_CODE_Y + 45)
#define UI_PROGRESS_BAR_HEIGHT 15
#define UI_PROGRESS_BAR_WIDTH (SCREEN_WIDTH - 2 * UI_PADDING - 20)
#define UI_PROGRESS_BAR_X ((SCREEN_WIDTH - UI_PROGRESS_BAR_WIDTH) / 2)
#define UI_FOOTER_Y (SCREEN_HEIGHT - UI_PADDING) // Ajustado Y do rodapé
#define UI_MENU_ITEM_HEIGHT 30
#define UI_MENU_START_Y UI_CONTENT_Y_START // Menu começa logo abaixo do header
  
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

enum ScreenState {
  SCREEN_TOTP_VIEW,
  SCREEN_MENU_MAIN,
  SCREEN_SERVICE_ADD_WAIT,
  SCREEN_SERVICE_ADD_CONFIRM,
  SCREEN_TIME_EDIT,
  SCREEN_SERVICE_DELETE_CONFIRM,
  SCREEN_TIMEZONE_EDIT,
  SCREEN_MESSAGE
};

// ---- Variáveis Globais ----
RTC_DS3231 rtc;
TFT_eSPI tft = TFT_eSPI();
Preferences preferences;

// Sprites para atualização parcial da tela
TFT_eSprite spr_totp_code = TFT_eSprite(&tft);
TFT_eSprite spr_progress_bar = TFT_eSprite(&tft);

// Botões
OneButton btn_prev(PIN_BUTTON_0, true, true);
OneButton btn_next(PIN_BUTTON_1, true, true);

// Serviços e TOTP
TOTPService services[MAX_SERVICES];
int service_count = 0;
int current_service_index = 0;
CurrentTOTPInfo current_totp;

// Estado e Variáveis de UI
ScreenState current_screen = SCREEN_MENU_MAIN;
int current_menu_index = 0;
int menu_top_visible_index = 0;
int menu_highlight_y_current = -1;
int menu_highlight_y_target = -1;
unsigned long menu_animation_start_time = 0;
bool is_menu_animating = false;

// Variáveis para ajustes e mensagens
char temp_service_name[MAX_SERVICE_NAME_LEN + 1];
char temp_service_secret[MAX_SECRET_B32_LEN + 1];
int edit_time_field = 0, edit_hour, edit_minute, edit_second;
char message_buffer[100];
uint16_t message_color = COLOR_FG;
int gmt_offset = 0;
const char* NVS_TZ_OFFSET_KEY = "tz_offset";

// Tempo e interações
uint32_t last_interaction_time = 0;
uint32_t last_rtc_sync_time = 0;
uint32_t last_screen_update_time = 0;

// Menu Principal
const char *menu_options[] = {
  "Adicionar Servico",
  "Ver Codigos TOTP",
  "Ajustar Hora/Data",
  "Ajustar Fuso Horario"
};
const int NUM_MENU_OPTIONS = sizeof(menu_options) / sizeof(menu_options[0]);

// Brilho de Tela
uint8_t current_brightness = 10;
constexpr uint8_t BRIGHTNESS_USB = 12;
constexpr uint8_t BRIGHTNESS_BATTERY = 8;
constexpr uint8_t BRIGHTNESS_DIMMED = 2;

// Informações da bateria
BatteryInfo battery_info;

// ---- Protótipos de Funções ----
void initHardware();
void initSprites();
void loadServices();
void updateTimeFromRTC();
void updateRTCFromSystem();
bool decodeCurrentServiceKey();
uint32_t generateTOTP(const uint8_t *key, size_t keyLength, uint64_t timestamp, uint32_t interval = TOTP_INTERVAL);
void updateCurrentTOTP();

bool storage_saveServiceList();
bool storage_saveService(const char *name, const char *secret_b32);
bool storage_deleteService(int index);

void processSerialInput();
void processServiceAdd(JsonDocument &doc);
void processTimeSet(JsonDocument &doc);

void updateBatteryStatus();
void setScreenBrightness(uint8_t level);
void updateScreenBrightness();

void changeScreen(ScreenState new_screen);

void btn_prev_click();
void btn_next_click();
void btn_next_double_click();
void btn_next_long_press_start();

void ui_drawScreen(bool full_redraw);
void ui_drawHeader(const char *title);
void ui_updateHeaderDynamic();
void ui_drawClockDirect(bool show_seconds = false);
void ui_drawBatteryDirect();
void ui_updateProgressBarSprite(uint64_t current_timestamp);
void ui_updateTOTPCodeSprite();
void ui_showTemporaryMessage(const char *msg, uint16_t color, uint32_t duration_ms);

void ui_drawScreenTOTPContent(bool full_redraw);
void ui_drawScreenMenuContent(bool full_redraw);
void ui_drawScreenServiceAddWaitContent(bool full_redraw);
void ui_drawScreenServiceAddConfirmContent(bool full_redraw);
void ui_drawScreenTimeEditContent(bool full_redraw);
void ui_drawScreenServiceDeleteConfirmContent(bool full_redraw);
void ui_drawScreenTimezoneEditContent(bool full_redraw);
void ui_drawScreenMessage(bool full_redraw);
const char* getScreenTitle(ScreenState state);

// ---- Funções de Decodificação Base32 e TOTP ----
int base32_decode(const uint8_t *encoded, size_t encodedLength, uint8_t *result, size_t bufSize) {
  int buffer = 0, bitsLeft = 0, count = 0;
  for (size_t i = 0; i < encodedLength && count < bufSize; i++) {
    uint8_t ch = encoded[i], value = 255;
    if (ch >= 'A' && ch <= 'Z') value = ch - 'A';
    else if (ch >= 'a' && ch <= 'z') value = ch - 'a';
    else if (ch >= '2' && ch <= '7') value = ch - '2' + 26;
    else if (ch == '=') continue;
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

uint32_t generateTOTP(const uint8_t *key, size_t keyLength, uint64_t timestamp, uint32_t interval) {
  if (!key || keyLength == 0) return 0;
  uint64_t counter = timestamp / interval;
  uint8_t counterBytes[8];
  for (int i = 7; i >= 0; i--) {
    counterBytes[i] = counter & 0xFF;
    counter >>= 8;
  }
  uint8_t hash[20];
  mbedtls_md_context_t ctx;
  mbedtls_md_info_t const *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
  if (!info) return 0;
  mbedtls_md_init(&ctx);
  if (mbedtls_md_setup(&ctx, info, 1) != 0) { mbedtls_md_free(&ctx); return 0; }
  if (mbedtls_md_hmac_starts(&ctx, key, keyLength) != 0) { mbedtls_md_free(&ctx); return 0; }
  if (mbedtls_md_hmac_update(&ctx, counterBytes, 8) != 0) { mbedtls_md_free(&ctx); return 0; }
  if (mbedtls_md_hmac_finish(&ctx, hash) != 0) { mbedtls_md_free(&ctx); return 0; }
  mbedtls_md_free(&ctx);
  int offset = hash[19] & 0x0F;
  uint32_t binaryCode = ((hash[offset] & 0x7F) << 24) |
                        ((hash[offset + 1] & 0xFF) << 16) |
                        ((hash[offset + 2] & 0xFF) << 8)  |
                        (hash[offset + 3] & 0xFF);
  return binaryCode % 1000000;
}

// ---- Setup e Loop Principal ----
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n[SETUP] Inicializando TOTP Authenticator Optimized v4");

  initHardware();
  initSprites();
  
  // Carrega preferências e serviços
  preferences.begin("totp-app", true);
  gmt_offset = preferences.getInt(NVS_TZ_OFFSET_KEY, 0);
  preferences.end();
  Serial.printf("[SETUP] Fuso Horario: GMT%+d\n", gmt_offset);

  loadServices();
  if (service_count > 0) {
    current_service_index = 0;
    decodeCurrentServiceKey();
  } else {
    current_totp.valid_key = false;
  }
  
  // Configura botões
  btn_prev.attachClick(btn_prev_click);
  btn_next.attachClick(btn_next_click);
  btn_next.attachDoubleClick(btn_next_double_click);
  btn_next.attachLongPressStart(btn_next_long_press_start);
  Serial.println("[SETUP] Botões configurados.");
  
  updateBatteryStatus();
  current_brightness = battery_info.is_usb_powered ? BRIGHTNESS_USB : BRIGHTNESS_BATTERY;
  setScreenBrightness(current_brightness);
  
  last_interaction_time = millis();
  last_rtc_sync_time = millis();
  last_screen_update_time = 0;
  
  changeScreen(SCREEN_MENU_MAIN);
  Serial.println("[SETUP] Inicialização concluída.");
}

void loop() {
  btn_prev.tick();
  btn_next.tick();
  
  // Processa entrada Serial se necessário (em telas de entrada)
  if (current_screen == SCREEN_SERVICE_ADD_WAIT || current_screen == SCREEN_TIME_EDIT) {
    processSerialInput();
  }
  
  uint32_t currentMillis = millis();
  if (currentMillis - last_rtc_sync_time >= RTC_SYNC_INTERVAL_MS) {
    updateTimeFromRTC();
    last_rtc_sync_time = currentMillis;
  }
  
  updateScreenBrightness();
  
  bool needsUpdate = false;
  if (currentMillis - last_screen_update_time >= SCREEN_UPDATE_INTERVAL_MS) {
    needsUpdate = true;
    last_screen_update_time = currentMillis;
    updateBatteryStatus();
    if (current_screen == SCREEN_TOTP_VIEW && service_count > 0) {
      updateCurrentTOTP();
    }
  }
  
  if (needsUpdate || is_menu_animating) {
    ui_drawScreen(false);
  }
  
  delay(10); // Pequeno delay para suavizar animações
}

// ---- Funções de Inicialização ----
void initHardware() {
  // Inicializa Serial, RTC, TFT e pinos
  pinMode(PIN_POWER_ON, OUTPUT); digitalWrite(PIN_POWER_ON, HIGH);
  pinMode(PIN_LCD_BL, OUTPUT); digitalWrite(PIN_LCD_BL, LOW);
  pinMode(PIN_BAT_VOLT, INPUT);
  Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
  
  Serial.print("[SETUP] Inicializando RTC DS3231... ");
  if (!rtc.begin()) {
    Serial.println("FALHOU!");
    tft.init();
    tft.fillScreen(COLOR_ERROR);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawString("Erro RTC!", 20, 50);
    while (1) delay(1000);
  }
  Serial.println("OK.");
  if (rtc.lostPower()) Serial.println("[SETUP] RTC perdeu energia.");
  updateTimeFromRTC();
  
  Serial.print("[SETUP] Inicializando TFT_eSPI... ");
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(COLOR_BG);
  Serial.println("OK.");
}

void initSprites() {
  // Inicializa sprites para TOTP e barra de progresso
  spr_totp_code.setColorDepth(16);
  spr_totp_code.createSprite(150, 40);
  spr_totp_code.setTextDatum(MC_DATUM);
  
  spr_progress_bar.setColorDepth(8);
  spr_progress_bar.createSprite(UI_PROGRESS_BAR_WIDTH - 4, UI_PROGRESS_BAR_HEIGHT - 4);
}

void loadServices() {
  if (!preferences.begin("totp-app", true)) return;
  service_count = preferences.getInt("svc_count", 0);
  if (service_count > MAX_SERVICES) service_count = MAX_SERVICES;
  bool success = true;
  for (int i = 0; i < service_count; i++) {
    char nk[16], sk[16];
    snprintf(nk, sizeof(nk), "svc_%d_name", i);
    snprintf(sk, sizeof(sk), "svc_%d_secret", i);
    String n = preferences.getString(nk, "");
    String s = preferences.getString(sk, "");
    if (n.length() > 0 && n.length() <= MAX_SERVICE_NAME_LEN &&
        s.length() > 0 && s.length() <= MAX_SECRET_B32_LEN) {
      strncpy(services[i].name, n.c_str(), MAX_SERVICE_NAME_LEN);
      services[i].name[MAX_SERVICE_NAME_LEN] = '\0';
      strncpy(services[i].secret_b32, s.c_str(), MAX_SECRET_B32_LEN);
      services[i].secret_b32[MAX_SECRET_B32_LEN] = '\0';
    } else {
      memset(&services[i], 0, sizeof(TOTPService));
      success = false;
    }
  }
  preferences.end();
  
  // Reorganiza lista removendo entradas inválidas
  int validCount = 0;
  for (int i = 0; i < service_count; i++) {
    if (services[i].name[0] != '\0') {
      if (i != validCount) services[validCount] = services[i];
      validCount++;
    }
  }
  service_count = validCount;
  // Opcional: salvar lista se houve remoção de entradas inválidas
}

// ---- Funções de Tempo ----
void updateTimeFromRTC() {
  setTime(rtc.now().unixtime());
}

void updateRTCFromSystem() {
  DateTime dt(year(), month(), day(), hour(), minute(), second());
  rtc.adjust(dt);
  Serial.println("[RTC] Sincronizado com sistema.");
}

// ---- Funções TOTP ----
bool decodeCurrentServiceKey() {
  if (service_count == 0 || current_service_index >= service_count) {
    current_totp.valid_key = false;
    return false;
  }
  const char *s = services[current_service_index].secret_b32;
  int len = base32_decode((const uint8_t*)s, strlen(s), current_totp.key_bin, MAX_SECRET_BIN_LEN);
  if (len > 0) {
    current_totp.key_bin_len = len;
    current_totp.valid_key = true;
    current_totp.last_generated_interval = 0;
    updateCurrentTOTP();
    return true;
  } else {
    current_totp.valid_key = false;
    current_totp.key_bin_len = 0;
    snprintf(current_totp.code, sizeof(current_totp.code), "ERRO");
    return false;
  }
}

void updateCurrentTOTP() {
  if (!current_totp.valid_key) {
    snprintf(current_totp.code, sizeof(current_totp.code), "------");
    return;
  }
  uint64_t t = now();
  uint32_t interval = t / TOTP_INTERVAL;
  if (interval != current_totp.last_generated_interval) {
    uint32_t code_val = generateTOTP(current_totp.key_bin, current_totp.key_bin_len, t);
    snprintf(current_totp.code, sizeof(current_totp.code), "%06lu", code_val);
    current_totp.last_generated_interval = interval;
  }
}

// ---- Funções de Armazenamento (NVS) ----
bool storage_saveServiceList() {
  if (!preferences.begin("totp-app", false)) return false;
  int oldCount = preferences.getInt("svc_count", 0);
  for (int i = service_count; i < oldCount; i++) {
    char nk[16], sk[16];
    snprintf(nk, sizeof(nk), "svc_%d_name", i);
    snprintf(sk, sizeof(sk), "svc_%d_secret", i);
    preferences.remove(nk);
    preferences.remove(sk);
  }
  preferences.putInt("svc_count", service_count);
  bool success = true;
  for (int i = 0; i < service_count; i++) {
    char nk[16], sk[16];
    snprintf(nk, sizeof(nk), "svc_%d_name", i);
    snprintf(sk, sizeof(sk), "svc_%d_secret", i);
    if (!preferences.putString(nk, services[i].name)) success = false;
    if (!preferences.putString(sk, services[i].secret_b32)) success = false;
  }
  preferences.end();
  return success;
}

bool storage_saveService(const char *n, const char *s) {
  if (service_count >= MAX_SERVICES) return false;
  strncpy(services[service_count].name, n, MAX_SERVICE_NAME_LEN);
  services[service_count].name[MAX_SERVICE_NAME_LEN] = '\0';
  strncpy(services[service_count].secret_b32, s, MAX_SECRET_B32_LEN);
  services[service_count].secret_b32[MAX_SECRET_B32_LEN] = '\0';
  service_count++;
  return storage_saveServiceList();
}

bool storage_deleteService(int idx) {
  if (idx < 0 || idx >= service_count) return false;
  for (int i = idx; i < service_count - 1; i++) {
    services[i] = services[i+1];
  }
  service_count--;
  memset(&services[service_count], 0, sizeof(TOTPService));
  if (current_service_index >= service_count && service_count > 0)
    current_service_index = service_count - 1;
  else if (service_count == 0)
    current_service_index = 0;
  
  bool suc = storage_saveServiceList();
  if (service_count > 0) decodeCurrentServiceKey();
  else {
    current_totp.valid_key = false;
    snprintf(current_totp.code, sizeof(current_totp.code), "------");
  }
  return suc;
}

// ---- Funções de Entrada Serial com Validação ----
void processSerialInput() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;
    Serial.printf("[SERIAL] RX: %s\n", input.c_str());
    last_interaction_time = millis();
    
    // Cria um documento JSON com tamanho adequado
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, input);
    if (error) {
      snprintf(message_buffer, sizeof(message_buffer), "Erro JSON:\n%s", error.c_str());
      ui_showTemporaryMessage(message_buffer, COLOR_ERROR, 3000);
      if (current_screen == SCREEN_SERVICE_ADD_WAIT || current_screen == SCREEN_TIME_EDIT)
        changeScreen(SCREEN_MENU_MAIN);
      return;
    }
    if (current_screen == SCREEN_SERVICE_ADD_WAIT) {
      processServiceAdd(doc);
    } else if (current_screen == SCREEN_TIME_EDIT) {
      processTimeSet(doc);
    }
  }
}

void processServiceAdd(JsonDocument &doc) {
  if (!doc.containsKey("name") || !doc["name"].is<const char*>() ||
      !doc.containsKey("secret") || !doc["secret"].is<const char*>()) {
    ui_showTemporaryMessage("JSON Invalido:\nFalta 'name'/'secret'", COLOR_ERROR, 3000);
    changeScreen(SCREEN_MENU_MAIN);
    return;
  }
  const char *n = doc["name"];
  const char *s = doc["secret"];
  if (strlen(n) == 0 || strlen(n) > MAX_SERVICE_NAME_LEN) {
    ui_showTemporaryMessage("Nome Invalido!", COLOR_ERROR, 3000);
    changeScreen(SCREEN_MENU_MAIN);
    return;
  }
  if (strlen(s) == 0 || strlen(s) > MAX_SECRET_B32_LEN) {
    ui_showTemporaryMessage("Segredo Invalido!", COLOR_ERROR, 3000);
    changeScreen(SCREEN_MENU_MAIN);
    return;
  }
  strncpy(temp_service_name, n, sizeof(temp_service_name) - 1);
  temp_service_name[sizeof(temp_service_name) - 1] = '\0';
  strncpy(temp_service_secret, s, sizeof(temp_service_secret) - 1);
  temp_service_secret[sizeof(temp_service_secret) - 1] = '\0';
  changeScreen(SCREEN_SERVICE_ADD_CONFIRM);
}

void processTimeSet(JsonDocument &doc) {
  if (!doc.containsKey("year") || !doc["year"].is<int>() ||
      !doc.containsKey("month") || !doc["month"].is<int>() ||
      !doc.containsKey("day") || !doc["day"].is<int>() ||
      !doc.containsKey("hour") || !doc["hour"].is<int>() ||
      !doc.containsKey("minute") || !doc["minute"].is<int>() ||
      !doc.containsKey("second") || !doc["second"].is<int>()) {
    ui_showTemporaryMessage("JSON Invalido:\nFaltam campos Y/M/D h:m:s", COLOR_ERROR, 3000);
    changeScreen(SCREEN_MENU_MAIN);
    return;
  }
  int y = doc["year"], m = doc["month"], d = doc["day"],
      h = doc["hour"], mn = doc["minute"], s = doc["second"];
  if (y < 2023 || y > 2100 || m < 1 || m > 12 || d < 1 || d > 31 ||
      h < 0 || h > 23 || mn < 0 || mn > 59 || s < 0 || s > 59) {
    ui_showTemporaryMessage("Valores Data/Hora\nInvalidos!", COLOR_ERROR, 3000);
    changeScreen(SCREEN_MENU_MAIN);
    return;
  }
  setTime(h, mn, s, d, m, y);
  updateRTCFromSystem();
  time_t local_t = now() + gmt_offset * 3600;
  snprintf(message_buffer, sizeof(message_buffer), "Hora Ajustada:\n%02d:%02d:%02d", hour(local_t), minute(local_t), second(local_t));
  ui_showTemporaryMessage(message_buffer, COLOR_SUCCESS, 3000);
  changeScreen(SCREEN_MENU_MAIN);
}

// ---- Funções de Gestão de Energia ----
void updateBatteryStatus() {
  int adc = analogRead(PIN_BAT_VOLT);
  battery_info.voltage = (adc / 4095.0) * 3.3 * 2.0;
  constexpr float USB_V = 4.15, BATT_MAX = 4.1, BATT_MIN = 3.2;
  battery_info.is_usb_powered = (battery_info.voltage >= USB_V);
  if (battery_info.is_usb_powered)
    battery_info.level_percent = 100;
  else {
    battery_info.level_percent = map(battery_info.voltage * 100, BATT_MIN * 100, BATT_MAX * 100, 0, 100);
    battery_info.level_percent = constrain(battery_info.level_percent, 0, 100);
  }
}

void setScreenBrightness(uint8_t v) {
  const uint8_t MAX_LVL = 16;
  static uint8_t current_lvl_internal = 0;
  v = constrain(v, 0, MAX_LVL);
  if (v == current_brightness && v != 0 && current_lvl_internal != 0) return;
  if (v == 0) {
    digitalWrite(PIN_LCD_BL, LOW);
    current_lvl_internal = 0;
  } else {
    if (current_lvl_internal == 0) {
      digitalWrite(PIN_LCD_BL, HIGH);
      current_lvl_internal = MAX_LVL;
      delayMicroseconds(50);
    }
    int diff = (MAX_LVL - current_lvl_internal) - (MAX_LVL - v);
    int steps = (diff + MAX_LVL) % MAX_LVL;
    for (int i = 0; i < steps; i++) {
      digitalWrite(PIN_LCD_BL, LOW);
      delayMicroseconds(5);
      digitalWrite(PIN_LCD_BL, HIGH);
      delayMicroseconds(1);
    }
    current_lvl_internal = v;
  }
  current_brightness = v;
}

void updateScreenBrightness() {
  uint8_t target = battery_info.is_usb_powered ? BRIGHTNESS_USB :
                   ((millis() - last_interaction_time > INACTIVITY_TIMEOUT_MS) ? BRIGHTNESS_DIMMED : BRIGHTNESS_BATTERY);
  setScreenBrightness(target);
}

// ---- Função de Alteração de Tela ----
void changeScreen(ScreenState new_screen) {
  is_menu_animating = false;
  menu_highlight_y_current = -1;
  current_screen = new_screen;
  Serial.printf("[SCREEN] Mudando para: %d\n", new_screen);
  if (new_screen == SCREEN_MENU_MAIN) {
    menu_top_visible_index = max(0, min(current_menu_index - VISIBLE_MENU_ITEMS / 2, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS));
  }
  ui_drawScreen(true);
  last_interaction_time = millis();
}

// ---- Callbacks dos Botões ----
void btn_prev_click() {
  last_interaction_time = millis();
  bool redraw = true;
  switch (current_screen) {
    case SCREEN_TOTP_VIEW:
      if (service_count > 0) {
        current_service_index = (current_service_index - 1 + service_count) % service_count;
        decodeCurrentServiceKey();
      } else {
        redraw = false;
      }
      break;
    case SCREEN_MENU_MAIN:
      if (NUM_MENU_OPTIONS > 0) {
        int old_visible_pos = current_menu_index - menu_top_visible_index;
        int old_highlight_y = UI_MENU_START_Y + old_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
        current_menu_index = (current_menu_index - 1 + NUM_MENU_OPTIONS) % NUM_MENU_OPTIONS;
        if (current_menu_index < menu_top_visible_index)
          menu_top_visible_index = current_menu_index;
        else if (current_menu_index == NUM_MENU_OPTIONS - 1 && menu_top_visible_index > 0)
          menu_top_visible_index = max(0, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS);
        int new_visible_pos = current_menu_index - menu_top_visible_index;
        int new_highlight_y = UI_MENU_START_Y + new_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
        if (menu_highlight_y_current != new_highlight_y) {
          if (menu_highlight_y_current == -1) menu_highlight_y_current = old_highlight_y;
          menu_highlight_y_target = new_highlight_y;
          menu_animation_start_time = millis();
          is_menu_animating = true;
        }
      }
      break;
    case SCREEN_TIME_EDIT:
      if (edit_time_field == 0) edit_hour = (edit_hour - 1 + 24) % 24;
      else if (edit_time_field == 1) edit_minute = (edit_minute - 1 + 60) % 60;
      else if (edit_time_field == 2) edit_second = (edit_second - 1 + 60) % 60;
      break;
    case SCREEN_TIMEZONE_EDIT:
      gmt_offset = (gmt_offset - 1 < -12) ? 14 : gmt_offset - 1;
      break;
    case SCREEN_SERVICE_DELETE_CONFIRM:
    case SCREEN_SERVICE_ADD_CONFIRM:
      changeScreen(SCREEN_MENU_MAIN);
      redraw = false;
      break;
    default:
      redraw = false;
      break;
  }
  if (redraw) ui_drawScreen(true);
}

void btn_next_click() {
  last_interaction_time = millis();
  bool redraw = true;
  switch (current_screen) {
    case SCREEN_TOTP_VIEW:
      if (service_count > 0) {
        current_service_index = (current_service_index + 1) % service_count;
        decodeCurrentServiceKey();
      } else {
        redraw = false;
      }
      break;
    case SCREEN_MENU_MAIN:
      if (NUM_MENU_OPTIONS > 0) {
        int old_visible_pos = current_menu_index - menu_top_visible_index;
        int old_highlight_y = UI_MENU_START_Y + old_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
        current_menu_index = (current_menu_index + 1) % NUM_MENU_OPTIONS;
        if (current_menu_index >= menu_top_visible_index + VISIBLE_MENU_ITEMS)
          menu_top_visible_index = current_menu_index - VISIBLE_MENU_ITEMS + 1;
        else if (current_menu_index == 0 && menu_top_visible_index != 0)
          menu_top_visible_index = 0;
        menu_top_visible_index = min(menu_top_visible_index, max(0, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS));
        int new_visible_pos = current_menu_index - menu_top_visible_index;
        int new_highlight_y = UI_MENU_START_Y + new_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
        if (menu_highlight_y_current != new_highlight_y) {
          if (menu_highlight_y_current == -1) menu_highlight_y_current = old_highlight_y;
          menu_highlight_y_target = new_highlight_y;
          menu_animation_start_time = millis();
          is_menu_animating = true;
        }
      }
      break;
    case SCREEN_TIME_EDIT:
      if (edit_time_field == 0) edit_hour = (edit_hour + 1) % 24;
      else if (edit_time_field == 1) edit_minute = (edit_minute + 1) % 60;
      else if (edit_time_field == 2) edit_second = (edit_second + 1) % 60;
      break;
    case SCREEN_TIMEZONE_EDIT:
      gmt_offset = (gmt_offset + 1 > 14) ? -12 : gmt_offset + 1;
      break;
    case SCREEN_SERVICE_DELETE_CONFIRM:
      if (storage_deleteService(current_service_index)) {
        ui_showTemporaryMessage("Servico Deletado", COLOR_SUCCESS, 2000);
      } else {
        ui_showTemporaryMessage("Erro ao Deletar", COLOR_ERROR, 2000);
      }
      changeScreen(SCREEN_TOTP_VIEW);
      redraw = false;
      break;
    case SCREEN_SERVICE_ADD_CONFIRM:
      if (storage_saveService(temp_service_name, temp_service_secret)) {
        ui_showTemporaryMessage("Servico Adicionado!", COLOR_SUCCESS, 2000);
        current_service_index = service_count - 1;
        decodeCurrentServiceKey();
        changeScreen(SCREEN_TOTP_VIEW);
      } else {
        ui_showTemporaryMessage("Erro ao Salvar!", COLOR_ERROR, 2000);
        changeScreen(SCREEN_MENU_MAIN);
      }
      redraw = false;
      break;
    default:
      redraw = false;
      break;
  }
  if (redraw) ui_drawScreen(true);
}

void btn_next_double_click() {
  last_interaction_time = millis();
  if (current_screen != SCREEN_MENU_MAIN) {
    changeScreen(SCREEN_MENU_MAIN);
  }
}

void btn_next_long_press_start() {
  last_interaction_time = millis();
  switch (current_screen) {
    case SCREEN_TOTP_VIEW:
      if (service_count > 0)
        changeScreen(SCREEN_SERVICE_DELETE_CONFIRM);
      break;
    case SCREEN_MENU_MAIN:
      switch (current_menu_index) {
        case 0:
          changeScreen(SCREEN_SERVICE_ADD_WAIT);
          break;
        case 1:
          if (service_count > 0)
            changeScreen(SCREEN_TOTP_VIEW);
          else
            ui_showTemporaryMessage("Nenhum servico...", COLOR_ACCENT, 2000);
          break;
        case 2:
          edit_hour = hour();
          edit_minute = minute();
          edit_second = second();
          edit_time_field = 0;
          changeScreen(SCREEN_TIME_EDIT);
          break;
        case 3:
          changeScreen(SCREEN_TIMEZONE_EDIT);
          break;
      }
      break;
    case SCREEN_TIME_EDIT:
      edit_time_field++;
      if (edit_time_field > 2) {
        setTime(edit_hour, edit_minute, edit_second, day(), month(), year());
        updateRTCFromSystem();
        time_t local_time = now() + gmt_offset * 3600;
        snprintf(message_buffer, sizeof(message_buffer), "Hora Ajustada:\n%02d:%02d:%02d",
                 hour(local_time), minute(local_time), second(local_time));
        ui_showTemporaryMessage(message_buffer, COLOR_SUCCESS, 2500);
      } else {
        ui_drawScreen(true);
      }
      break;
    case SCREEN_TIMEZONE_EDIT:
      preferences.begin("totp-app", false);
      preferences.putInt(NVS_TZ_OFFSET_KEY, gmt_offset);
      preferences.end();
      snprintf(message_buffer, sizeof(message_buffer), "Fuso Salvo:\nGMT %+d", gmt_offset);
      ui_showTemporaryMessage(message_buffer, COLOR_SUCCESS, 2000);
      break;
    default:
      break;
  }
}

// ---- Funções de Desenho da Interface ----
void ui_drawScreen(bool full_redraw) {
  if (current_screen == SCREEN_MESSAGE) {
    ui_drawScreenMessage(full_redraw);
    return;
  }
  
  if (full_redraw) {
    ui_drawHeader(getScreenTitle(current_screen));
  }
  ui_updateHeaderDynamic();
  
  switch (current_screen) {
    case SCREEN_TOTP_VIEW:
      ui_drawScreenTOTPContent(full_redraw);
      break;
    case SCREEN_MENU_MAIN:
      ui_drawScreenMenuContent(full_redraw);
      break;
    case SCREEN_SERVICE_ADD_WAIT:
      ui_drawScreenServiceAddWaitContent(full_redraw);
      break;
    case SCREEN_SERVICE_ADD_CONFIRM:
      ui_drawScreenServiceAddConfirmContent(full_redraw);
      break;
    case SCREEN_TIME_EDIT:
      ui_drawScreenTimeEditContent(full_redraw);
      break;
    case SCREEN_SERVICE_DELETE_CONFIRM:
      ui_drawScreenServiceDeleteConfirmContent(full_redraw);
      break;
    case SCREEN_TIMEZONE_EDIT:
      ui_drawScreenTimezoneEditContent(full_redraw);
      break;
    default:
      break;
  }
}

const char* getScreenTitle(ScreenState state) {
  switch (state) {
    case SCREEN_TOTP_VIEW:
      return (service_count > 0 && current_service_index < service_count) ? services[current_service_index].name : "Codigo TOTP";
    case SCREEN_MENU_MAIN:
      return "Menu Principal";
    case SCREEN_SERVICE_ADD_WAIT:
      return "Adicionar Servico";
    case SCREEN_SERVICE_ADD_CONFIRM:
      return "Confirmar Adicao";
    case SCREEN_TIME_EDIT:
      return "Ajustar Hora";
    case SCREEN_SERVICE_DELETE_CONFIRM:
      return "Confirmar Exclusao";
    case SCREEN_TIMEZONE_EDIT:
      return "Ajustar Fuso";
    default:
      return "";
  }
}

void ui_drawHeader(const char *title) {
  tft.fillRect(0, 0, tft.width(), UI_HEADER_HEIGHT, COLOR_ACCENT);
  uint8_t prev_datum = tft.getTextDatum();
  uint16_t prev_fg = tft.textcolor;
  uint16_t prev_bg = tft.textbgcolor;
  uint8_t prev_size = tft.textsize;
  
  tft.setTextColor(COLOR_BG, COLOR_ACCENT);
  tft.setTextDatum(ML_DATUM);
  tft.setTextSize(2);
  tft.drawString(title, UI_PADDING, UI_HEADER_HEIGHT / 2);
  
  tft.setTextDatum(prev_datum);
  tft.setTextColor(prev_fg, prev_bg);
  tft.setTextSize(prev_size);
}

void ui_updateHeaderDynamic() {
  ui_drawClockDirect();
  ui_drawBatteryDirect();
}

void ui_drawClockDirect(bool show_seconds) {
  time_t now_utc = now();
  time_t local_time = now_utc + gmt_offset * 3600;
  uint8_t prev_datum = tft.getTextDatum();
  uint16_t prev_fg = tft.textcolor;
  uint16_t prev_bg = tft.textbgcolor;
  
  char time_str[9];
  int textWidth = 0;
  int textHeight = tft.fontHeight(2) / 2;
  
  if (show_seconds) {
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour(local_time), minute(local_time), second(local_time));
    textWidth = tft.textWidth("00:00:00", 2);
  } else {
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hour(local_time), minute(local_time));
    textWidth = tft.textWidth("00:00", 2);
  }
  
  int x = tft.width() - textWidth - UI_BATT_WIDTH - UI_PADDING * 2;
  int y = 5;
  
  tft.setTextColor(COLOR_BG, COLOR_ACCENT);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(2);
  tft.fillRect(x, y, textWidth, textHeight, COLOR_ACCENT);
  tft.drawString(time_str, x, y);
  
  tft.setTextDatum(prev_datum);
  tft.setTextColor(prev_fg, prev_bg);
}

void ui_drawBatteryDirect() {
  int x = tft.width() - UI_BATT_WIDTH - UI_PADDING;
  int y = (UI_HEADER_HEIGHT - UI_BATT_HEIGHT) / 2;
  int icon_w = UI_BATT_WIDTH - 2, icon_h = UI_BATT_HEIGHT - 2;
  int icon_x = x + 1, icon_y = y + 1;
  int term_w = 2, term_h = icon_h / 2;
  int term_x = icon_x + icon_w, term_y = icon_y + (icon_h - term_h) / 2;
  uint16_t fg_color = COLOR_BG, bg_color = COLOR_ACCENT;
  
  tft.fillRect(x, y, UI_BATT_WIDTH, UI_BATT_HEIGHT, bg_color);
  
  if (battery_info.is_usb_powered) {
    tft.drawRect(icon_x, icon_y, icon_w, icon_h, fg_color);
    tft.fillRect(term_x, term_y, term_w, term_h, fg_color);
    int bolt_x = icon_x + icon_w / 2 - 3, bolt_y = icon_y + 1;
    int bolt_y_mid = bolt_y + (icon_h > 2 ? (icon_h - 2) / 2 : 0);
    int bolt_y_end = icon_h > 2 ? (icon_y + icon_h - 2) : bolt_y;
    tft.drawLine(bolt_x + 3, bolt_y, bolt_x, bolt_y_mid, fg_color);
    tft.drawLine(bolt_x, bolt_y_mid, bolt_x + 3, bolt_y_mid, fg_color);
    tft.drawLine(bolt_x + 3, bolt_y_mid, bolt_x, bolt_y_end, fg_color);
  } else {
    tft.drawRect(icon_x, icon_y, icon_w, icon_h, fg_color);
    tft.fillRect(term_x, term_y, term_w, term_h, fg_color);
    int fill_w_max = icon_w > 2 ? icon_w - 2 : 0;
    int fill_h = icon_h > 2 ? icon_h - 2 : 0;
    int fill_w = (fill_w_max * battery_info.level_percent) / 100;
    fill_w = constrain(fill_w, 0, fill_w_max);
    uint16_t fill_color = COLOR_SUCCESS;
    if (battery_info.level_percent < 50) fill_color = TFT_YELLOW;
    if (battery_info.level_percent < 20) fill_color = COLOR_ERROR;
    if (fill_w > 0) tft.fillRect(icon_x + 1, icon_y + 1, fill_w, fill_h, fill_color);
    if (fill_w < fill_w_max) tft.fillRect(icon_x + 1 + fill_w, icon_y + 1, fill_w_max - fill_w, fill_h, bg_color);
  }
}

void ui_updateProgressBarSprite(uint64_t t) {
  uint32_t r = TOTP_INTERVAL - (t % TOTP_INTERVAL);
  int bw = spr_progress_bar.width(), bh = spr_progress_bar.height();
  int pw = map(r, 0, TOTP_INTERVAL, 0, bw);
  pw = constrain(pw, 0, bw);
  spr_progress_bar.fillRect(0, 0, bw, bh, COLOR_BAR_BG);
  if (pw > 0) spr_progress_bar.fillRect(0, 0, pw, bh, COLOR_BAR_FG);
}

void ui_updateTOTPCodeSprite() {
  spr_totp_code.fillSprite(COLOR_BG);
  spr_totp_code.setTextColor(COLOR_FG);
  spr_totp_code.setTextSize(UI_TOTP_CODE_FONT_SIZE);
  if (service_count == 0 || !current_totp.valid_key)
    spr_totp_code.drawString("------", spr_totp_code.width() / 2, spr_totp_code.height() / 2);
  else
    spr_totp_code.drawString(current_totp.code, spr_totp_code.width() / 2, spr_totp_code.height() / 2);
}

void ui_showTemporaryMessage(const char *msg, uint16_t color, uint32_t duration_ms) {
  ScreenState prev_screen = current_screen;
  strncpy(message_buffer, msg, sizeof(message_buffer) - 1);
  message_buffer[sizeof(message_buffer) - 1] = '\0';
  message_color = color;
  changeScreen(SCREEN_MESSAGE);
  uint32_t start_time = millis();
  while (millis() - start_time < duration_ms) {
    delay(50);
  }
  // Retorna para a tela anterior
  if (prev_screen == SCREEN_MESSAGE ||
      prev_screen == SCREEN_SERVICE_ADD_CONFIRM ||
      prev_screen == SCREEN_SERVICE_DELETE_CONFIRM ||
      prev_screen == SCREEN_TIME_EDIT ||
      prev_screen == SCREEN_SERVICE_ADD_WAIT ||
      prev_screen == SCREEN_TIMEZONE_EDIT) {
    changeScreen(SCREEN_MENU_MAIN);
  } else {
    changeScreen(prev_screen);
  }
}

// ---- Telas Específicas de UI ----
void ui_drawScreenTOTPContent(bool full_redraw) {
  int footer_height = tft.fontHeight(1) + UI_PADDING;
  int content_y_start = UI_HEADER_HEIGHT;
  int content_height = tft.height() - content_y_start - footer_height;
  int content_center_y = content_y_start + content_height / 2;
  
  int totp_code_y = content_center_y - spr_totp_code.height() / 2;
  int progress_bar_y = totp_code_y + spr_totp_code.height() + 10;
  
  if (full_redraw) {
    tft.fillRect(0, content_y_start, tft.width(), content_height + footer_height, COLOR_BG);
    tft.drawRoundRect(UI_PROGRESS_BAR_X - 2, progress_bar_y - 2, UI_PROGRESS_BAR_WIDTH + 4, UI_PROGRESS_BAR_HEIGHT + 4, 4, COLOR_FG);
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(TFT_DARKGREY);
    tft.setTextSize(1);
    tft.drawString("Prev/Next | LP: Del | DblClick: Menu", tft.width() / 2, tft.height() - UI_PADDING / 2);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
  }
  
  uint64_t current_time = now();
  ui_updateTOTPCodeSprite();
  ui_updateProgressBarSprite(current_time);
  
  spr_totp_code.pushSprite((tft.width() - spr_totp_code.width()) / 2, totp_code_y);
  spr_progress_bar.pushSprite(UI_PROGRESS_BAR_X, progress_bar_y);
}

void ui_drawScreenMenuContent(bool full_redraw) {
  int item_y = UI_MENU_START_Y;
  int item_spacing = 5;
  int item_total_height = UI_MENU_ITEM_HEIGHT + item_spacing;
  
  if (is_menu_animating) {
    unsigned long elapsed = millis() - menu_animation_start_time;
    if (elapsed >= MENU_ANIMATION_DURATION_MS) {
      menu_highlight_y_current = menu_highlight_y_target;
      is_menu_animating = false;
    } else {
      float progress = (float)elapsed / MENU_ANIMATION_DURATION_MS;
      menu_highlight_y_current = menu_highlight_y_target + (menu_highlight_y_current - menu_highlight_y_target) * (1.0f - progress);
    }
  } else if (menu_highlight_y_current == -1 && NUM_MENU_OPTIONS > 0) {
    int initial_pos = current_menu_index - menu_top_visible_index;
    menu_highlight_y_current = UI_MENU_START_Y + initial_pos * item_total_height;
    menu_highlight_y_target = menu_highlight_y_current;
  }
  
  if (full_redraw) {
    tft.fillRect(0, UI_HEADER_HEIGHT, tft.width(), tft.height() - UI_HEADER_HEIGHT, COLOR_BG);
    tft.setTextDatum(BL_DATUM);
    tft.setTextColor(TFT_DARKGREY);
    tft.setTextSize(1);
    char status_buf[30];
    snprintf(status_buf, sizeof(status_buf), "Servicos: %d/%d", service_count, MAX_SERVICES);
    tft.drawString(status_buf, UI_PADDING, tft.height() - UI_PADDING / 2);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
  } else {
    tft.fillRect(UI_PADDING, item_y, tft.width() - 2 * UI_PADDING, VISIBLE_MENU_ITEMS * item_total_height, COLOR_BG);
  }
  
  if (menu_highlight_y_current != -1 && NUM_MENU_OPTIONS > 0) {
    tft.fillRect(UI_PADDING, menu_highlight_y_current, tft.width() - 2 * UI_PADDING - 10, UI_MENU_ITEM_HEIGHT, COLOR_HIGHLIGHT_BG);
  }
  
  tft.setTextSize(2);
  tft.setTextDatum(ML_DATUM);
  int draw_count = 0;
  for (int i = menu_top_visible_index; i < NUM_MENU_OPTIONS && draw_count < VISIBLE_MENU_ITEMS; i++) {
    int current_item_y = item_y + draw_count * item_total_height;
    if (i == current_menu_index)
      tft.setTextColor(COLOR_HIGHLIGHT_FG);
    else
      tft.setTextColor(COLOR_FG);
    tft.drawString(menu_options[i], UI_PADDING * 3, current_item_y + UI_MENU_ITEM_HEIGHT / 2);
    draw_count++;
  }
  
  if (NUM_MENU_OPTIONS > VISIBLE_MENU_ITEMS) {
    int scrollbar_x = tft.width() - UI_PADDING - 6;
    int scrollbar_w = 4;
    int scrollbar_y = UI_MENU_START_Y;
    int scrollbar_h = VISIBLE_MENU_ITEMS * item_total_height - item_spacing;
    tft.fillRect(scrollbar_x, scrollbar_y, scrollbar_w, scrollbar_h, TFT_DARKGREY);
    int thumb_h = max(5, scrollbar_h * VISIBLE_MENU_ITEMS / NUM_MENU_OPTIONS);
    int thumb_max_y = scrollbar_h - thumb_h;
    int thumb_y = scrollbar_y + round((float)thumb_max_y * menu_top_visible_index / (NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS));
    thumb_y = constrain(thumb_y, scrollbar_y, scrollbar_y + thumb_max_y);
    tft.fillRect(scrollbar_x, thumb_y, scrollbar_w, thumb_h, COLOR_ACCENT);
  }
  
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COLOR_FG, COLOR_BG);
  tft.setTextSize(1);
}

void ui_drawScreenServiceAddWaitContent(bool full_redraw) {
  if (full_redraw) {
    tft.fillRect(0, UI_HEADER_HEIGHT, tft.width(), tft.height() - UI_HEADER_HEIGHT, COLOR_BG);
    tft.setTextColor(COLOR_FG);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    int center_y = UI_HEADER_HEIGHT + (tft.height() - UI_HEADER_HEIGHT) / 2;
    tft.drawString("Aguardando JSON", tft.width() / 2, center_y - 30);
    tft.drawString("via Serial...", tft.width() / 2, center_y);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}", tft.width() / 2, center_y + 30);
    tft.setTextDatum(BC_DATUM);
    tft.drawString("DblClick: Menu", tft.width()/2, tft.height() - UI_PADDING / 2);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
  }
}

void ui_drawScreenServiceAddConfirmContent(bool full_redraw) {
  if (full_redraw) {
    tft.fillRect(0, UI_HEADER_HEIGHT, tft.width(), tft.height() - UI_HEADER_HEIGHT, COLOR_BG);
    tft.setTextColor(COLOR_ACCENT);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    int center_y = UI_HEADER_HEIGHT + (tft.height() - UI_HEADER_HEIGHT) / 3;
    tft.drawString("Adicionar Servico:", tft.width()/2, center_y);
    tft.setTextColor(COLOR_FG);
    tft.drawString(temp_service_name, tft.width()/2, center_y + 35);
    tft.setTextColor(TFT_DARKGREY);
    tft.setTextSize(1);
    tft.setTextDatum(BC_DATUM);
    tft.drawString("Prev: Cancelar | Next: Confirmar", tft.width()/2, tft.height() - UI_PADDING / 2);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
  }
}

void ui_drawScreenTimeEditContent(bool full_redraw) {
  if (full_redraw) {
    tft.fillRect(0, UI_HEADER_HEIGHT, tft.width(), tft.height() - UI_HEADER_HEIGHT, COLOR_BG);
    tft.setTextColor(TFT_DARKGREY);
    tft.setTextSize(1);
    tft.setTextDatum(BC_DATUM);
    tft.drawString("Prev/Next: +/- | LP: Campo/Salvar | Dbl: Menu", tft.width()/2, tft.height() - UI_PADDING / 2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Ou envie JSON via Serial (UTC):", tft.width()/2, tft.height() - 50);
    tft.drawString("{'y':...,'mo':...,'d':...,'h':...,'m':...,'s':...}", tft.width()/2, tft.height() - 35);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
  }
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COLOR_FG, COLOR_BG);
  uint8_t time_font_size = 3;
  int text_height = tft.fontHeight(time_font_size);
  int time_y = tft.height() / 2 - text_height;
  tft.fillRect(0, time_y - text_height/2 - 5, tft.width(), text_height + 30, COLOR_BG);
  tft.setTextSize(time_font_size);
  char time_buf[12];
  snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", edit_hour, edit_minute, edit_second);
  tft.drawString(time_buf, tft.width()/2, time_y);
  int field_width = tft.textWidth("00");
  int sep_width = tft.textWidth(":");
  int total_width = 3 * field_width + 2 * sep_width;
  int start_x = (tft.width() - total_width) / 2;
  int marker_y = time_y - 20;
  tft.fillRect(start_x, marker_y, total_width, 4, COLOR_BG);
  int marker_x = start_x;
  if (edit_time_field == 1) marker_x = start_x + field_width + sep_width;
  else if (edit_time_field == 2) marker_x = start_x + 2*(field_width+sep_width);
  tft.fillRect(marker_x, marker_y, field_width, 4, COLOR_ACCENT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COLOR_FG, COLOR_BG);
  tft.setTextSize(1);
  char tz_buf[30];
  int fuse_y = time_y + 20;
  snprintf(tz_buf, sizeof(tz_buf), "(Fuso atual: GMT %+d)", gmt_offset);
  tft.drawString(tz_buf, tft.width()/2, fuse_y);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_FG, COLOR_BG);
}

void ui_drawScreenServiceDeleteConfirmContent(bool full_redraw) {
  if (full_redraw) {
    tft.fillRect(0, UI_HEADER_HEIGHT, tft.width(), tft.height() - UI_HEADER_HEIGHT, COLOR_BG);
    tft.setTextColor(COLOR_ERROR);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    int center_y = UI_HEADER_HEIGHT + (tft.height() - UI_HEADER_HEIGHT) / 3;
    tft.drawString("Deletar servico:", tft.width()/2, center_y);
    tft.setTextColor(COLOR_FG);
    if (service_count > 0 && current_service_index < service_count)
      tft.drawString(services[current_service_index].name, tft.width()/2, center_y + 35);
    else
      tft.drawString("???", tft.width()/2, center_y + 35);
    tft.setTextColor(TFT_DARKGREY);
    tft.setTextSize(1);
    tft.setTextDatum(BC_DATUM);
    tft.drawString("Prev: Cancelar | Next: Confirmar", tft.width()/2, tft.height() - UI_PADDING / 2);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
  }
}

void ui_drawScreenTimezoneEditContent(bool full_redraw) {
  if (full_redraw) {
    tft.fillRect(0, UI_HEADER_HEIGHT, tft.width(), tft.height() - UI_HEADER_HEIGHT, COLOR_BG);
    tft.setTextColor(TFT_DARKGREY);
    tft.setTextSize(1);
    tft.setTextDatum(BC_DATUM);
    tft.drawString("Prev/Next: +/- | LP: Salvar | Dbl: Menu", tft.width()/2, tft.height() - UI_PADDING / 2);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
  }
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COLOR_FG, COLOR_BG);
  tft.setTextSize(3);
  int center_y = UI_HEADER_HEIGHT + (tft.height() - UI_HEADER_HEIGHT) / 2;
  int text_h = tft.fontHeight(3);
  tft.fillRect(0, center_y - text_h/2 - 5, tft.width(), text_h + 10, COLOR_BG);
  char tz_str[10];
  snprintf(tz_str, sizeof(tz_str), "GMT %+d", gmt_offset);
  tft.drawString(tz_str, tft.width()/2, center_y);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
}

void ui_drawScreenMessage(bool full_redraw) {
  if (full_redraw) {
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(message_color, COLOR_BG);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    char temp_buf[sizeof(message_buffer)];
    strncpy(temp_buf, message_buffer, sizeof(temp_buf));
    temp_buf[sizeof(temp_buf)-1] = '\0';
    char *line1 = strtok(temp_buf, "\n");
    char *line2 = strtok(NULL, "\n");
    int y_pos = tft.height()/2;
    int line_h = tft.fontHeight(2) + 5;
    if (line1 && line2) {
      y_pos -= line_h/2;
      tft.drawString(line1, tft.width()/2, y_pos);
      tft.drawString(line2, tft.width()/2, y_pos + line_h);
    } else if (line1) {
      tft.drawString(line1, tft.width()/2, y_pos);
    }
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
  }
}

// FIM DO CÓDIGO.
