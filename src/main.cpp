#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>      // Para RTC DS3231
#include <TimeLib.h>      // Para funções de tempo (now(), hour(), etc.) sincronizadas com RTC
#include <TFT_eSPI.h>     // Para display TFT (contém TFT_eSprite)
#include "OneButton.h"    // Para gerenciamento de botões
#include <Preferences.h>  // Para armazenamento NVS
#include <ArduinoJson.h>  // Para parsing JSON robusto
#include "mbedtls/md.h"   // Para HMAC-SHA1 (TOTP)
#include "pin_config.h"   // SEU ARQUIVO DE CONFIGURAÇÃO DE PINOS

// --- Constantes Globais ---
const uint32_t TOTP_INTERVAL = 30;
const int MAX_SERVICES = 50;
const size_t MAX_SERVICE_NAME_LEN = 20;
const size_t MAX_SECRET_B32_LEN = 64;
const size_t MAX_SECRET_BIN_LEN = 40;

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

// --- Estruturas de Dados ---
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

// --- Estados da Aplicação ---
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

// --- Variáveis Globais ---
RTC_DS3231 rtc;
TFT_eSPI tft = TFT_eSPI();
Preferences preferences;

// Sprites (Removido spr_service_name)
TFT_eSprite spr_totp_code = TFT_eSprite(&tft);
TFT_eSprite spr_progress_bar = TFT_eSprite(&tft);

// Botões
OneButton btn_prev(PIN_BUTTON_0, true, true);
OneButton btn_next(PIN_BUTTON_1, true, true);

// Serviços
TOTPService services[MAX_SERVICES];
int service_count = 0;
int current_service_index = 0;

// Estado
ScreenState current_screen = SCREEN_MENU_MAIN;
CurrentTOTPInfo current_totp;
BatteryInfo battery_info;
uint8_t current_brightness = 10;
const uint8_t BRIGHTNESS_USB = 12;
const uint8_t BRIGHTNESS_BATTERY = 8;
const uint8_t BRIGHTNESS_DIMMED = 2;
const uint32_t INACTIVITY_TIMEOUT_MS = 30000;
const uint32_t RTC_SYNC_INTERVAL_MS = 60000;
uint32_t last_interaction_time = 0;
uint32_t last_rtc_sync_time = 0;
uint32_t last_screen_update_time = 0;

// Temporárias
char temp_service_name[MAX_SERVICE_NAME_LEN + 1];
char temp_service_secret[MAX_SECRET_B32_LEN + 1];
int edit_time_field = 0;
int edit_hour, edit_minute, edit_second;
int current_menu_index = 0;
char message_buffer[100];
uint16_t message_color = COLOR_FG;

// Fuso Horário
int gmt_offset = 0;
const char* NVS_TZ_OFFSET_KEY = "tz_offset";

// Menu
const char *menu_options[] = {
    "Adicionar Servico",
    "Ver Codigos TOTP",
    "Ajustar Hora/Data",
    "Ajustar Fuso Horario"
};
const int NUM_MENU_OPTIONS = sizeof(menu_options) / sizeof(menu_options[0]);

// --- Protótipos ---
void setup();
void loop();
void changeScreen(ScreenState new_screen);
void updateRTCFromSystem();
void updateTimeLibFromRTC();
int base32_decode(const uint8_t *encoded, size_t encodedLength, uint8_t *result, size_t bufSize);
bool decodeCurrentServiceKey();
uint32_t generateTOTP(const uint8_t *key, size_t keyLength, uint64_t timestamp, uint32_t interval);
void updateCurrentTOTP();
bool storage_loadServices();
bool storage_saveService(const char *name, const char *secret_b32);
bool storage_deleteService(int index);
bool storage_saveServiceList();
void serial_processInput();
void serial_processServiceAdd(JsonDocument &doc);
void serial_processTimeSet(JsonDocument &doc);
void power_updateBatteryStatus();
void power_setBrightness(uint8_t level);
void power_updateScreenBrightness();
void btn_prev_click();
void btn_next_click();
void btn_next_double_click();
void btn_next_long_press_start();
void ui_initSprites();
void ui_drawScreen(bool full_redraw); // Função central de desenho
// Funções de desenho de conteúdo específico (sem header)
void ui_drawScreenTOTPContent(bool full_redraw);
void ui_drawScreenMenuContent(bool full_redraw);
void ui_drawScreenServiceAddWaitContent(bool full_redraw);
void ui_drawScreenServiceAddConfirmContent(bool full_redraw);
void ui_drawScreenTimeEditContent(bool full_redraw);
void ui_drawScreenServiceDeleteConfirmContent(bool full_redraw);
void ui_drawScreenTimezoneEditContent(bool full_redraw);
void ui_drawScreenMessage(bool full_redraw); // Sem header
// Header e Helpers
const char* getScreenTitle(ScreenState state); // <<< NOVO HELPER
void ui_drawHeader(const char *title);         // Desenha fundo e título
void ui_updateHeaderDynamic();                 // Atualiza relógio e bateria
void ui_drawClockDirect(bool show_seconds = false); // Sua função
void ui_drawBatteryDirect();                 // Sua função
// Sprites
void ui_updateProgressBarSprite(uint64_t current_timestamp);
void ui_updateTOTPCodeSprite();
// ui_updateServiceNameSprite(); // REMOVIDO
void ui_showTemporaryMessage(const char *msg, uint16_t color, uint32_t duration_ms);

// --- Implementação Funções Base32 e TOTP ---
int base32_decode(const uint8_t *encoded, size_t encodedLength, uint8_t *result, size_t bufSize) { int buffer = 0, bitsLeft = 0, count = 0; for (size_t i = 0; i < encodedLength && count < bufSize; i++) { uint8_t ch = encoded[i], value = 255; if (ch >= 'A' && ch <= 'Z') value = ch - 'A'; else if (ch >= 'a' && ch <= 'z') value = ch - 'a'; else if (ch >= '2' && ch <= '7') value = ch - '2' + 26; else if (ch == '=') continue; else continue; buffer <<= 5; buffer |= value; bitsLeft += 5; if (bitsLeft >= 8) { result[count++] = (buffer >> (bitsLeft - 8)) & 0xFF; bitsLeft -= 8; } } return count; }
uint32_t generateTOTP(const uint8_t *key, size_t keyLength, uint64_t timestamp, uint32_t interval = TOTP_INTERVAL) { if (!key || keyLength == 0) return 0; uint64_t counter = timestamp / interval; uint8_t counterBytes[8]; for (int i = 7; i >= 0; i--) { counterBytes[i] = counter & 0xFF; counter >>= 8; } uint8_t hash[20]; mbedtls_md_context_t ctx; mbedtls_md_info_t const *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1); if (!info) return 0; mbedtls_md_init(&ctx); if (mbedtls_md_setup(&ctx, info, 1) != 0) { mbedtls_md_free(&ctx); return 0; } if (mbedtls_md_hmac_starts(&ctx, key, keyLength) != 0) { mbedtls_md_free(&ctx); return 0; } if (mbedtls_md_hmac_update(&ctx, counterBytes, 8) != 0) { mbedtls_md_free(&ctx); return 0; } if (mbedtls_md_hmac_finish(&ctx, hash) != 0) { mbedtls_md_free(&ctx); return 0; } mbedtls_md_free(&ctx); int offset = hash[19] & 0x0F; uint32_t binaryCode = ((hash[offset] & 0x7F) << 24) | ((hash[offset + 1] & 0xFF) << 16) | ((hash[offset + 2] & 0xFF) << 8) | (hash[offset + 3] & 0xFF); return binaryCode % 1000000; }

// --- Implementação das Funções ---

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("\n[SETUP] Iniciando TOTP Authenticator Unified Header v3...");

    pinMode(PIN_POWER_ON, OUTPUT); digitalWrite(PIN_POWER_ON, HIGH);
    pinMode(PIN_LCD_BL, OUTPUT); digitalWrite(PIN_LCD_BL, LOW);
    pinMode(PIN_BAT_VOLT, INPUT);
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);

    Serial.print("[SETUP] Inicializando RTC DS3231... ");
    if (!rtc.begin()) {
        Serial.println("FALHOU!"); tft.init(); tft.fillScreen(COLOR_ERROR);
        tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
        tft.drawString("Erro RTC!", 20, 50); while (1) delay(1000);
    }
    Serial.println("OK.");
    if (rtc.lostPower()) Serial.println("[SETUP] RTC perdeu energia.");
    updateTimeLibFromRTC();

    Serial.print("[SETUP] Inicializando TFT_eSPI... ");
    tft.init(); tft.setRotation(3); tft.fillScreen(COLOR_BG); Serial.println("OK.");

    Serial.print("[SETUP] Criando Sprites... ");
    ui_initSprites(); Serial.println("OK."); // Inicializa sprites restantes (sem nome de serviço)

    preferences.begin("totp-app", true);
    gmt_offset = preferences.getInt(NVS_TZ_OFFSET_KEY, 0);
    preferences.end();
    Serial.printf("[SETUP] Fuso horário carregado: GMT%+d\n", gmt_offset);

    Serial.print("[SETUP] Carregando serviços do NVS... ");
    if (storage_loadServices()) Serial.printf("OK. %d serviços.\n", service_count);
    else { Serial.println("FALHOU ou vazio."); service_count = 0; }

    if (service_count > 0) { current_service_index = 0; decodeCurrentServiceKey(); }
    else { current_totp.valid_key = false; }

    btn_prev.attachClick(btn_prev_click);
    btn_next.attachClick(btn_next_click);
    btn_next.attachDoubleClick(btn_next_double_click);
    btn_next.attachLongPressStart(btn_next_long_press_start);
    Serial.println("[SETUP] Botões configurados.");

    power_updateBatteryStatus();
    current_brightness = battery_info.is_usb_powered ? BRIGHTNESS_USB : BRIGHTNESS_BATTERY;
    power_setBrightness(current_brightness);
    last_interaction_time = millis(); last_rtc_sync_time = millis(); last_screen_update_time = 0;

    changeScreen(SCREEN_MENU_MAIN); // Inicia no menu
    Serial.println("[SETUP] Inicialização concluída.");
}

void loop() {
    btn_prev.tick(); btn_next.tick();

    if (current_screen == SCREEN_SERVICE_ADD_WAIT || current_screen == SCREEN_TIME_EDIT) {
        serial_processInput();
    }

    uint32_t current_millis = millis();

    if (current_millis - last_rtc_sync_time >= RTC_SYNC_INTERVAL_MS) {
        updateTimeLibFromRTC(); last_rtc_sync_time = current_millis;
    }

    power_updateScreenBrightness();

    bool needs_update = false;
    if (current_millis - last_screen_update_time >= 1000) {
        needs_update = true;
        last_screen_update_time = current_millis;
        power_updateBatteryStatus(); // Atualiza status bateria para header
        if (current_screen == SCREEN_TOTP_VIEW && service_count > 0) {
            updateCurrentTOTP(); // Atualiza lógica TOTP
        }
    }

    // Redesenha a tela (header dinâmico + conteúdo específico) apenas se necessário
    if (needs_update) {
         ui_drawScreen(false);
    }

    delay(10);
}

// --- Funções Core ---
void changeScreen(ScreenState new_screen) {
    current_screen = new_screen;
    Serial.printf("[SCREEN] Mudando para estado: %d\n", new_screen);
    ui_drawScreen(true); // FORÇA redesenho COMPLETO ao mudar de tela
    last_interaction_time = millis();
}
void updateTimeLibFromRTC() { setTime(rtc.now().unixtime()); }
void updateRTCFromSystem() { DateTime dt(year(), month(), day(), hour(), minute(), second()); rtc.adjust(dt); Serial.println("[RTC] Sincronizado."); }

// --- Funções TOTP ---
bool decodeCurrentServiceKey(){ if(service_count == 0 || current_service_index >= service_count){current_totp.valid_key=false; return false;} const char* s=services[current_service_index].secret_b32; int len=base32_decode((const uint8_t*)s,strlen(s),current_totp.key_bin,MAX_SECRET_BIN_LEN); if(len > 0){current_totp.key_bin_len=len; current_totp.valid_key=true; current_totp.last_generated_interval=0; updateCurrentTOTP(); return true;} else {current_totp.valid_key=false; current_totp.key_bin_len=0; snprintf(current_totp.code, sizeof(current_totp.code), "ERRO"); return false;} }
void updateCurrentTOTP(){ if (!current_totp.valid_key) { snprintf(current_totp.code, sizeof(current_totp.code), "------"); return; } uint64_t t = now(); uint32_t interval = t / TOTP_INTERVAL; if (interval != current_totp.last_generated_interval) { uint32_t code_val = generateTOTP(current_totp.key_bin, current_totp.key_bin_len, t); snprintf(current_totp.code, sizeof(current_totp.code), "%06lu", code_val); current_totp.last_generated_interval = interval; } }

// --- Funções NVS ---
bool storage_loadServices(){ if (!preferences.begin("totp-app", true)) return false; service_count = preferences.getInt("svc_count", 0); if (service_count > MAX_SERVICES) service_count = MAX_SERVICES; bool success = true; for (int i=0; i<service_count; i++) { char nk[16], sk[16]; snprintf(nk, sizeof(nk), "svc_%d_name", i); snprintf(sk, sizeof(sk), "svc_%d_secret", i); String n=preferences.getString(nk,""); String s=preferences.getString(sk,""); if(n.length()>0 && n.length()<=MAX_SERVICE_NAME_LEN && s.length()>0 && s.length()<=MAX_SECRET_B32_LEN) { strncpy(services[i].name,n.c_str(),MAX_SERVICE_NAME_LEN); services[i].name[MAX_SERVICE_NAME_LEN] = '\0'; strncpy(services[i].secret_b32,s.c_str(),MAX_SECRET_B32_LEN); services[i].secret_b32[MAX_SECRET_B32_LEN] = '\0'; } else { memset(&services[i], 0, sizeof(TOTPService)); success=false; } } preferences.end(); int vc=0; for(int i=0; i<service_count; ++i) if(services[i].name[0]!='\0') { if(i!=vc) services[vc]=services[i]; vc++; } if(vc!=service_count) { service_count=vc; /* storage_saveServiceList(); */ } return success;}
bool storage_saveServiceList(){ if(!preferences.begin("totp-app", false)) return false; int oc=preferences.getInt("svc_count", 0); for(int i=service_count; i<oc; ++i) { char nk[16], sk[16]; snprintf(nk,sizeof(nk),"svc_%d_name",i); snprintf(sk,sizeof(sk),"svc_%d_secret",i); preferences.remove(nk); preferences.remove(sk); } preferences.putInt("svc_count",service_count); bool s=true; for(int i=0; i<service_count; i++) { char nk[16], sk[16]; snprintf(nk,sizeof(nk),"svc_%d_name",i); snprintf(sk,sizeof(sk),"svc_%d_secret",i); if(!preferences.putString(nk,services[i].name)) s=false; if(!preferences.putString(sk,services[i].secret_b32)) s=false; } preferences.end(); return s; }
bool storage_saveService(const char *n, const char *s){ if(service_count>=MAX_SERVICES) return false; strncpy(services[service_count].name,n,MAX_SERVICE_NAME_LEN); services[service_count].name[MAX_SERVICE_NAME_LEN]='\0'; strncpy(services[service_count].secret_b32,s,MAX_SECRET_B32_LEN); services[service_count].secret_b32[MAX_SECRET_B32_LEN]='\0'; service_count++; return storage_saveServiceList(); }
bool storage_deleteService(int idx){ if(idx<0||idx>=service_count) return false; for(int i=idx; i<service_count-1; i++) services[i]=services[i+1]; service_count--; memset(&services[service_count],0,sizeof(TOTPService)); if(current_service_index>=service_count && service_count>0) current_service_index=service_count-1; else if(service_count==0) current_service_index=0; bool suc=storage_saveServiceList(); if(service_count>0) decodeCurrentServiceKey(); else {current_totp.valid_key=false; snprintf(current_totp.code,sizeof(current_totp.code),"------");} return suc; }

// --- Serial Input ---
void serial_processInput(){ if (Serial.available() > 0) { String input=Serial.readStringUntil('\n'); input.trim(); if(input.length()==0) return; Serial.printf("[SERIAL] RX: %s\n",input.c_str()); last_interaction_time=millis(); StaticJsonDocument<256> doc; DeserializationError error=deserializeJson(doc,input); if(error){snprintf(message_buffer, sizeof(message_buffer), "Erro JSON:\n%s", error.c_str()); ui_showTemporaryMessage(message_buffer, COLOR_ERROR, 3000); if (current_screen==SCREEN_SERVICE_ADD_WAIT || current_screen==SCREEN_TIME_EDIT) changeScreen(SCREEN_MENU_MAIN); return;} if (current_screen==SCREEN_SERVICE_ADD_WAIT) serial_processServiceAdd(doc); else if (current_screen==SCREEN_TIME_EDIT) serial_processTimeSet(doc); } }
void serial_processServiceAdd(JsonDocument &doc){ if(!doc.containsKey("name") || !doc["name"].is<const char*>() || !doc.containsKey("secret") || !doc["secret"].is<const char*>()) { ui_showTemporaryMessage("JSON Invalido:\nFalta 'name'/'secret'", COLOR_ERROR, 3000); changeScreen(SCREEN_MENU_MAIN); return; } const char* n=doc["name"]; const char* s=doc["secret"]; if(strlen(n)==0||strlen(n)>MAX_SERVICE_NAME_LEN){ ui_showTemporaryMessage("Nome Invalido!",COLOR_ERROR,3000); changeScreen(SCREEN_MENU_MAIN); return;} if(strlen(s)==0||strlen(s)>MAX_SECRET_B32_LEN){ ui_showTemporaryMessage("Segredo Invalido!",COLOR_ERROR,3000); changeScreen(SCREEN_MENU_MAIN); return;} strncpy(temp_service_name,n,sizeof(temp_service_name)-1); temp_service_name[sizeof(temp_service_name)-1]='\0'; strncpy(temp_service_secret,s,sizeof(temp_service_secret)-1); temp_service_secret[sizeof(temp_service_secret)-1]='\0'; changeScreen(SCREEN_SERVICE_ADD_CONFIRM); }
void serial_processTimeSet(JsonDocument &doc){ if(!doc.containsKey("year")||!doc["year"].is<int>()||!doc.containsKey("month")||!doc["month"].is<int>()||!doc.containsKey("day")||!doc["day"].is<int>()||!doc.containsKey("hour")||!doc["hour"].is<int>()||!doc.containsKey("minute")||!doc["minute"].is<int>()||!doc.containsKey("second")||!doc["second"].is<int>()){ ui_showTemporaryMessage("JSON Invalido:\nFaltam campos Y/M/D h:m:s", COLOR_ERROR, 3000); changeScreen(SCREEN_MENU_MAIN); return;} int y=doc["year"],m=doc["month"],d=doc["day"],h=doc["hour"],mn=doc["minute"],s=doc["second"]; if(y<2023||y>2100||m<1||m>12||d<1||d>31||h<0||h>23||mn<0||mn>59||s<0||s>59){ ui_showTemporaryMessage("Valores Data/Hora\nInvalidos!",COLOR_ERROR,3000); changeScreen(SCREEN_MENU_MAIN); return;} setTime(h,mn,s,d,m,y); updateRTCFromSystem(); time_t local_t=now()+gmt_offset*3600; snprintf(message_buffer,sizeof(message_buffer),"Hora Ajustada:\n%02d:%02d:%02d",hour(local_t),minute(local_t),second(local_t)); ui_showTemporaryMessage(message_buffer,COLOR_SUCCESS,3000); changeScreen(SCREEN_MENU_MAIN); }

// --- Power/Battery ---
void power_updateBatteryStatus(){ int adc=analogRead(PIN_BAT_VOLT); battery_info.voltage=(adc/4095.0)*3.3*2.0; const float USB_V=4.15, BATT_MAX=4.1, BATT_MIN=3.2; battery_info.is_usb_powered=(battery_info.voltage>=USB_V); if(battery_info.is_usb_powered) battery_info.level_percent=100; else {battery_info.level_percent=map(battery_info.voltage*100,BATT_MIN*100,BATT_MAX*100,0,100); battery_info.level_percent=constrain(battery_info.level_percent,0,100);} }
void power_setBrightness(uint8_t v){ const uint8_t MAX_LVL=16; static uint8_t cur_lvl_int=0; v=constrain(v,0,MAX_LVL); if(v==current_brightness&&v!=0&&cur_lvl_int!=0) return; if(v==0){digitalWrite(PIN_LCD_BL,LOW); cur_lvl_int=0;} else { if(cur_lvl_int==0){digitalWrite(PIN_LCD_BL,HIGH); cur_lvl_int=MAX_LVL; delayMicroseconds(50);} int f=MAX_LVL-cur_lvl_int, t=MAX_LVL-v, n=(MAX_LVL+t-f)%MAX_LVL; for(int i=0; i<n; i++){digitalWrite(PIN_LCD_BL,LOW); delayMicroseconds(5); digitalWrite(PIN_LCD_BL,HIGH); delayMicroseconds(1);} cur_lvl_int=v;} current_brightness=v;}
void power_updateScreenBrightness(){ uint8_t target; if(battery_info.is_usb_powered) target=BRIGHTNESS_USB; else target=(millis()-last_interaction_time > INACTIVITY_TIMEOUT_MS)?BRIGHTNESS_DIMMED:BRIGHTNESS_BATTERY; power_setBrightness(target); }

// --- Callbacks Botões ---
void btn_prev_click(){ last_interaction_time = millis(); bool r=true; switch(current_screen){ case SCREEN_TOTP_VIEW: if(service_count>0){current_service_index=(current_service_index-1+service_count)%service_count; decodeCurrentServiceKey();} else r=false; break; case SCREEN_MENU_MAIN: current_menu_index=(current_menu_index-1+NUM_MENU_OPTIONS)%NUM_MENU_OPTIONS; break; case SCREEN_TIME_EDIT: if(edit_time_field==0) edit_hour=(edit_hour-1+24)%24; else if(edit_time_field==1) edit_minute=(edit_minute-1+60)%60; else if(edit_time_field==2) edit_second=(edit_second-1+60)%60; break; case SCREEN_TIMEZONE_EDIT: gmt_offset--; if(gmt_offset<-12) gmt_offset=14; break; case SCREEN_SERVICE_DELETE_CONFIRM: case SCREEN_SERVICE_ADD_CONFIRM: changeScreen(SCREEN_MENU_MAIN); r=false; break; default: r=false; break; } if(r) ui_drawScreen(true); }
void btn_next_click(){ last_interaction_time = millis(); bool r=true; switch(current_screen){ case SCREEN_TOTP_VIEW: if(service_count>0){current_service_index=(current_service_index+1)%service_count; decodeCurrentServiceKey();} else r=false; break; case SCREEN_MENU_MAIN: current_menu_index=(current_menu_index+1)%NUM_MENU_OPTIONS; break; case SCREEN_TIME_EDIT: if(edit_time_field==0) edit_hour=(edit_hour+1)%24; else if(edit_time_field==1) edit_minute=(edit_minute+1)%60; else if(edit_time_field==2) edit_second=(edit_second+1)%60; break; case SCREEN_TIMEZONE_EDIT: gmt_offset++; if(gmt_offset>14) gmt_offset=-12; break; case SCREEN_SERVICE_DELETE_CONFIRM: if(storage_deleteService(current_service_index)) ui_showTemporaryMessage("Servico Deletado",COLOR_SUCCESS,2000); else ui_showTemporaryMessage("Erro ao Deletar",COLOR_ERROR,2000); changeScreen(SCREEN_TOTP_VIEW); r=false; break; case SCREEN_SERVICE_ADD_CONFIRM: if(storage_saveService(temp_service_name,temp_service_secret)){ui_showTemporaryMessage("Servico Adicionado!",COLOR_SUCCESS,2000); current_service_index=service_count-1; decodeCurrentServiceKey(); changeScreen(SCREEN_TOTP_VIEW);} else {ui_showTemporaryMessage("Erro ao Salvar!",COLOR_ERROR,2000); changeScreen(SCREEN_MENU_MAIN);} r=false; break; default: r=false; break;} if(r) ui_drawScreen(true); }
void btn_next_double_click(){ last_interaction_time=millis(); if(current_screen!=SCREEN_MENU_MAIN) changeScreen(SCREEN_MENU_MAIN); }
void btn_next_long_press_start(){ last_interaction_time=millis(); switch(current_screen){ case SCREEN_TOTP_VIEW: if(service_count>0) changeScreen(SCREEN_SERVICE_DELETE_CONFIRM); break; case SCREEN_MENU_MAIN: switch(current_menu_index){ case 0: changeScreen(SCREEN_SERVICE_ADD_WAIT); break; case 1: if(service_count>0) changeScreen(SCREEN_TOTP_VIEW); else ui_showTemporaryMessage("Nenhum servico...",COLOR_ACCENT,2000); break; case 2: edit_hour=hour(); edit_minute=minute(); edit_second=second(); edit_time_field=0; changeScreen(SCREEN_TIME_EDIT); break; case 3: changeScreen(SCREEN_TIMEZONE_EDIT); break; } break; case SCREEN_TIME_EDIT: edit_time_field++; if(edit_time_field>2){setTime(edit_hour,edit_minute,edit_second,day(),month(),year()); updateRTCFromSystem(); time_t local_t=now()+gmt_offset*3600; snprintf(message_buffer,sizeof(message_buffer),"Hora Ajustada:\n%02d:%02d:%02d",hour(local_t),minute(local_t),second(local_t)); ui_showTemporaryMessage(message_buffer,COLOR_SUCCESS,2500); /* changeScreen called by message */} else {ui_drawScreen(true);} break; case SCREEN_TIMEZONE_EDIT: preferences.begin("totp-app",false); preferences.putInt(NVS_TZ_OFFSET_KEY,gmt_offset); preferences.end(); snprintf(message_buffer,sizeof(message_buffer),"Fuso Salvo:\nGMT %+d",gmt_offset); ui_showTemporaryMessage(message_buffer,COLOR_SUCCESS,2000); /* changeScreen called by message */ break; default: break;} }

// --- UI Drawing ---

void ui_initSprites() {
    // spr_service_name REMOVIDO
    spr_totp_code.setColorDepth(16);
    spr_totp_code.createSprite(150, 40); // Ajustar tamanho se necessário
    spr_totp_code.setTextDatum(MC_DATUM);

    spr_progress_bar.setColorDepth(8);
    spr_progress_bar.createSprite(UI_PROGRESS_BAR_WIDTH - 4, UI_PROGRESS_BAR_HEIGHT - 4);
}

// Função Auxiliar para Título da Tela
const char* getScreenTitle(ScreenState state) {
    switch (state) {
        case SCREEN_TOTP_VIEW:
             // Se houver serviços, mostra o nome, senão um título genérico
             return (service_count > 0 && current_service_index < service_count) ? services[current_service_index].name : "Codigo TOTP";
        case SCREEN_MENU_MAIN:              return "Menu Principal";
        case SCREEN_SERVICE_ADD_WAIT:       return "Adicionar Servico";
        case SCREEN_SERVICE_ADD_CONFIRM:    return "Confirmar Adicao";
        case SCREEN_TIME_EDIT:              return "Ajustar Hora";
        case SCREEN_SERVICE_DELETE_CONFIRM: return "Confirmar Exclusao";
        case SCREEN_TIMEZONE_EDIT:          return "Ajustar Fuso";
        default:                            return ""; // Para SCREEN_MESSAGE ou outros
    }
}

// Desenha APENAS o fundo e o título estático do header
void ui_drawHeader(const char *title) {
    tft.fillRect(0, 0, SCREEN_WIDTH, UI_HEADER_HEIGHT, COLOR_ACCENT);
    uint8_t prev_datum = tft.getTextDatum(); uint16_t prev_fg = tft.textcolor;
    uint16_t prev_bg = tft.textbgcolor; uint8_t prev_size = tft.textsize;

    tft.setTextColor(COLOR_BG, COLOR_ACCENT); tft.setTextDatum(ML_DATUM); tft.setTextSize(2);
    // Desenha título com padding esquerdo
    tft.drawString(title, UI_PADDING, UI_HEADER_HEIGHT / 2);

    tft.setTextDatum(prev_datum); 
    tft.setTextColor(prev_fg, prev_bg); 
    tft.setTextSize(prev_size);
}

// Atualiza APENAS os elementos dinâmicos do Header (Relógio e Bateria)
void ui_updateHeaderDynamic() {
    // Chama suas funções de desenho direto
    ui_drawClockDirect(); // Mostra segundos no header
    ui_drawBatteryDirect();
}

// Função central de desenho da tela
void ui_drawScreen(bool full_redraw = false) {
    if (current_screen == SCREEN_MESSAGE) {
        ui_drawScreenMessage(full_redraw); // Tela de mensagem não tem header padrão
        return;
    }

    // --- Gerenciamento do Header ---
    if (full_redraw) {
        ui_drawHeader(getScreenTitle(current_screen)); // Desenha fundo e título
    }
    ui_updateHeaderDynamic();                      // Apenas atualiza relógio e bateria

    // --- Desenho do Conteúdo Específico da Tela (Abaixo do Header) ---
    switch (current_screen) {
        case SCREEN_TOTP_VIEW:              ui_drawScreenTOTPContent(full_redraw); break;
        case SCREEN_MENU_MAIN:              ui_drawScreenMenuContent(full_redraw); break;
        case SCREEN_SERVICE_ADD_WAIT:       ui_drawScreenServiceAddWaitContent(full_redraw); break;
        case SCREEN_SERVICE_ADD_CONFIRM:    ui_drawScreenServiceAddConfirmContent(full_redraw); break;
        case SCREEN_TIME_EDIT:              ui_drawScreenTimeEditContent(full_redraw); break;
        case SCREEN_SERVICE_DELETE_CONFIRM: ui_drawScreenServiceDeleteConfirmContent(full_redraw); break;
        case SCREEN_TIMEZONE_EDIT:          ui_drawScreenTimezoneEditContent(full_redraw); break;
    }
}


// --- Funções de Desenho de Conteúdo Específico (Abaixo do Header) ---

void ui_drawScreenTOTPContent(bool full_redraw) {

    // Calcula a área útil para o conteúdo (abaixo do header, acima do rodapé)
    int footer_height = tft.fontHeight(1) + UI_PADDING; // Altura estimada do rodapé
    int content_y_start = UI_HEADER_HEIGHT;
    int content_height = SCREEN_HEIGHT - content_y_start - footer_height;
    int content_center_y = content_y_start + content_height / 2; // Centro vertical da área de conteúdo

    // Calcula a posição Y para centralizar o sprite do código TOTP
    int totp_code_sprite_y = content_center_y - spr_totp_code.height() / 2;

    // Calcula a posição Y para a barra de progresso (abaixo do código)
    int progress_bar_y = totp_code_sprite_y + spr_totp_code.height() + 10; // 10px de espaço

    if (full_redraw) {
        // Limpa a área ABAIXO do header
        tft.fillRect(0, content_y_start, SCREEN_WIDTH, content_height + footer_height, COLOR_BG);

        // Desenha elementos estáticos da tela TOTP (contorno da barra, texto de ajuda)
        // *** Usa a nova posição Y calculada para a barra ***
        tft.drawRoundRect(UI_PROGRESS_BAR_X - 2, progress_bar_y - 2, // Usa a nova Y da barra
                          UI_PROGRESS_BAR_WIDTH + 4, UI_PROGRESS_BAR_HEIGHT + 4, 4, COLOR_FG);

        // Desenha rodapé
        tft.setTextDatum(BC_DATUM); // Alinha pelo centro inferior
        tft.setTextColor(TFT_DARKGREY);
        tft.setTextSize(1);
        tft.drawString("Prev/Next | LP: Del | DblClick: Menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT - UI_PADDING / 2); // Posição rodapé
        tft.setTextDatum(TL_DATUM); // Reset datum
        tft.setTextColor(COLOR_FG, COLOR_BG); // Reset cores
    }

    // Atualiza e desenha APENAS os sprites de conteúdo (código, barra)
    uint64_t current_unix_time_utc = now();
    ui_updateTOTPCodeSprite();
    ui_updateProgressBarSprite(current_unix_time_utc);

    // Push sprites para as NOVAS posições calculadas
    spr_totp_code.pushSprite((SCREEN_WIDTH - spr_totp_code.width()) / 2, totp_code_sprite_y); // Centralizado verticalmente
    spr_progress_bar.pushSprite(UI_PROGRESS_BAR_X, progress_bar_y); // Abaixo do código
}

void ui_drawScreenMenuContent(bool full_redraw) {
    if (full_redraw) {
        // Limpa área ABAIXO do header
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG);
        // Desenha rodapé estático
        tft.setTextDatum(BL_DATUM); tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1);
        char status_buf[30]; snprintf(status_buf, sizeof(status_buf), "Servicos: %d/%d", service_count, MAX_SERVICES);
        tft.drawString(status_buf, UI_PADDING, SCREEN_HEIGHT - UI_PADDING / 2); // Posição rodapé
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
    }

    // Desenha itens do menu
    tft.setTextSize(2); int item_y = UI_MENU_START_Y; // Y inicial abaixo do header
    for (int i = 0; i < NUM_MENU_OPTIONS; i++) {
         if (i == current_menu_index) { tft.fillRect(UI_PADDING, item_y, SCREEN_WIDTH - 2 * UI_PADDING, UI_MENU_ITEM_HEIGHT, COLOR_HIGHLIGHT_BG); tft.setTextColor(COLOR_HIGHLIGHT_FG); }
         else { tft.fillRect(UI_PADDING, item_y, SCREEN_WIDTH - 2 * UI_PADDING, UI_MENU_ITEM_HEIGHT, COLOR_BG); tft.setTextColor(COLOR_FG); }
         tft.setTextDatum(ML_DATUM); tft.drawString(menu_options[i], UI_PADDING * 3, item_y + UI_MENU_ITEM_HEIGHT / 2); item_y += UI_MENU_ITEM_HEIGHT + 5;
    }
    tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
}

void ui_drawScreenServiceAddWaitContent(bool full_redraw) {
    if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG); // Limpa área conteúdo
        tft.setTextColor(COLOR_FG); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
        int center_y = UI_HEADER_HEIGHT + (SCREEN_HEIGHT - UI_HEADER_HEIGHT) / 2; // Centro da área de conteúdo
        tft.drawString("Aguardando JSON", SCREEN_WIDTH / 2, center_y - 30);
        tft.drawString("via Serial...", SCREEN_WIDTH / 2, center_y );
        tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY);
        tft.drawString("Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}", SCREEN_WIDTH / 2, center_y + 30);
        tft.setTextDatum(BC_DATUM); // Rodapé
        tft.drawString("DblClick: Menu", SCREEN_WIDTH/2, SCREEN_HEIGHT - UI_PADDING / 2);
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
    }
    // Conteúdo estático
}

void ui_drawScreenServiceAddConfirmContent(bool full_redraw) {
     if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG);
        tft.setTextColor(COLOR_ACCENT); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
        int center_y = UI_HEADER_HEIGHT + (SCREEN_HEIGHT - UI_HEADER_HEIGHT) / 3; // Ajuste Y
        tft.drawString("Adicionar Servico:", SCREEN_WIDTH / 2, center_y);
        tft.setTextColor(COLOR_FG);
        tft.drawString(temp_service_name, SCREEN_WIDTH / 2, center_y + 35); // Abaixo
        tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1); tft.setTextDatum(BC_DATUM); // Rodapé
        tft.drawString("Prev: Cancelar | Next: Confirmar", SCREEN_WIDTH / 2, SCREEN_HEIGHT - UI_PADDING / 2);
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
     }
    // Conteúdo estático
}

void ui_drawScreenTimeEditContent(bool full_redraw) {
     if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG); // Limpa conteúdo
        // Desenha rodapé e info JSON
        tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1); tft.setTextDatum(BC_DATUM);
        tft.drawString("Prev/Next: +/- | LP: Campo/Salvar | Dbl: Menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT - UI_PADDING / 2);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Ou envie JSON via Serial (UTC):", SCREEN_WIDTH/2, SCREEN_HEIGHT - 50);
        tft.drawString("{'y':...,'mo':...,'d':...,'h':...,'m':...,'s':...}", SCREEN_WIDTH/2, SCREEN_HEIGHT - 35);
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
     }

     tft.setTextDatum(MC_DATUM); 
     tft.setTextColor(COLOR_FG, COLOR_BG);
     uint8_t time_font_size = 3; 
     int text_height = tft.fontHeight(time_font_size);
     int time_y_pos = SCREEN_HEIGHT / 2 - text_height; // Ajuste Y para centralizar melhor com a linha de fuso abaixo

     tft.fillRect(0, time_y_pos - text_height / 2 - 5, SCREEN_WIDTH, text_height + 30, COLOR_BG); // Limpa área hora + fuso + marcador

     tft.setTextSize(time_font_size); 
     char time_str_buf[12];
     snprintf(time_str_buf, sizeof(time_str_buf), "%02d:%02d:%02d", edit_hour, edit_minute, edit_second);
     tft.drawString(time_str_buf, SCREEN_WIDTH / 2, time_y_pos);

     // Marcador
     int field_width = tft.textWidth("00"); 
     int separator_width = tft.textWidth(":"); 
     int total_width = 3 * field_width + 2 * separator_width;
     int start_x = (SCREEN_WIDTH - total_width) / 2; 
     int marker_y = time_y_pos - 20; 
     int marker_height = 4;
     int marker_x = start_x; 
     if (edit_time_field == 1) marker_x = start_x + field_width + separator_width; 
     else if (edit_time_field == 2) marker_x = start_x + 2 * (field_width + separator_width);
     // tft.fillRect(start_x, marker_y, total_width, marker_height, COLOR_BG); // Limpeza já feita acima
     tft.fillRect(marker_x, marker_y, field_width, marker_height, COLOR_ACCENT); // Desenha marcador

     // <<< NOVO: Mostra fuso atual >>>
     tft.setTextDatum(MC_DATUM); 
     tft.setTextColor(COLOR_FG, COLOR_BG);
     tft.setTextSize(1);
     char tz_info_buf[30];
     int fuse_y = time_y_pos + 20;
     snprintf(tz_info_buf, sizeof(tz_info_buf), "(Fuso atual: GMT %+d)", gmt_offset);
     tft.drawString(tz_info_buf, SCREEN_WIDTH / 2, fuse_y); // Abaixo do marcador

     tft.setTextDatum(TL_DATUM); tft.setTextSize(1); tft.setTextColor(COLOR_FG, COLOR_BG);
}

void ui_drawScreenServiceDeleteConfirmContent(bool full_redraw) {
     if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG); // Limpa conteúdo
        tft.setTextColor(COLOR_ERROR); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
        int center_y = UI_HEADER_HEIGHT + (SCREEN_HEIGHT - UI_HEADER_HEIGHT) / 3; // Ajuste Y
        tft.drawString("Deletar servico:", SCREEN_WIDTH / 2, center_y);
        tft.setTextColor(COLOR_FG);
        if (service_count > 0 && current_service_index < service_count) tft.drawString(services[current_service_index].name, SCREEN_WIDTH / 2, center_y + 35);
        else tft.drawString("???", SCREEN_WIDTH / 2, center_y + 35);
        tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1); tft.setTextDatum(BC_DATUM); // Rodapé
        tft.drawString("Prev: Cancelar | Next: Confirmar", SCREEN_WIDTH / 2, SCREEN_HEIGHT - UI_PADDING / 2);
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
     }
     // Conteúdo estático
}

void ui_drawScreenTimezoneEditContent(bool full_redraw) {
    if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, COLOR_BG); // Limpa conteúdo
        // Desenha rodapé
        tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1); tft.setTextDatum(BC_DATUM);
        tft.drawString("Prev/Next: +/- | LP: Salvar | Dbl: Menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT - UI_PADDING / 2);
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG);
    }
    // Desenha valor GMT no centro da área de conteúdo
    tft.setTextDatum(MC_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); tft.setTextSize(3);
    int content_center_y = UI_HEADER_HEIGHT + (SCREEN_HEIGHT - UI_HEADER_HEIGHT) / 2;
    int text_height = tft.fontHeight(3);
    tft.fillRect(0, content_center_y - text_height / 2 - 5, SCREEN_WIDTH, text_height + 10, COLOR_BG); // Limpa área
    char tz_str[10]; snprintf(tz_str, sizeof(tz_str), "GMT %+d", gmt_offset);
    tft.drawString(tz_str, SCREEN_WIDTH / 2, content_center_y);
    tft.setTextDatum(TL_DATUM); tft.setTextSize(1);
}

// --- Funções Auxiliares de UI ---

// Desenha relógio (usado pelo header)
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
        textWidth = tft.textWidth("00:00:00", 2); // Largura do texto para 2 dígitos + ":" + ":" + "00"
    } else {
        snprintf(time_str, sizeof(time_str), "%02d:%02d", hour(local_time), minute(local_time));
        textWidth = tft.textWidth("00:00", 2);
    }

    int x = SCREEN_WIDTH - textWidth - UI_BATT_WIDTH - UI_PADDING * 2; // Ajusta posição para o relógio
    int y = 5;
    
    tft.setTextColor(COLOR_BG, COLOR_ACCENT); 
    tft.setTextDatum(TL_DATUM); 
    tft.setTextSize(2);
    tft.fillRect(x, y, textWidth, textHeight, COLOR_ACCENT); // Limpa área
    tft.drawString(time_str, x, y);

    // Serial.println("TextWidth: " + String(textWidth) + ", TextHeight: " + String(textHeight) + ", x: " + String(x) + ", y: " + String(y));

    // reset
    tft.setTextDatum(prev_datum); 
    tft.setTextColor(prev_fg, prev_bg);
}

// Desenha bateria (usado pelo header)
void ui_drawBatteryDirect() {
    // Calcula X para ficar na extremidade direita
    int x = SCREEN_WIDTH - UI_BATT_WIDTH - UI_PADDING;
    int y = (UI_HEADER_HEIGHT - UI_BATT_HEIGHT) / 2; // Centraliza Y no header
    int icon_w = UI_BATT_WIDTH - 2, icon_h = UI_BATT_HEIGHT - 2;
    int icon_x = x + 1, icon_y = y + 1;
    int term_w = 2, term_h = icon_h / 2;
    int term_x = icon_x + icon_w, term_y = icon_y + (icon_h - term_h) / 2;
    uint16_t fg_color = COLOR_BG, bg_color = COLOR_ACCENT;

    tft.fillRect(x, y, UI_BATT_WIDTH, UI_BATT_HEIGHT, bg_color); // Limpa área

    if (battery_info.is_usb_powered) { // Ícone USB/Raio
        tft.drawRect(icon_x, icon_y, icon_w, icon_h, fg_color); tft.fillRect(term_x, term_y, term_w, term_h, fg_color);
        int bolt_x = icon_x + icon_w / 2 - 3, bolt_y = icon_y + 1;
        int bolt_h_half = (icon_h > 2) ? (icon_h - 2) / 2 : 0, bolt_y_mid = bolt_y + bolt_h_half;
        int bolt_y_end = (icon_h > 2) ? (bolt_y + icon_h - 2) : bolt_y;
        tft.drawLine(bolt_x + 3, bolt_y, bolt_x, bolt_y_mid, fg_color); tft.drawLine(bolt_x, bolt_y_mid, bolt_x + 3, bolt_y_mid, fg_color); tft.drawLine(bolt_x + 3, bolt_y_mid, bolt_x, bolt_y_end, fg_color);
    } else { // Ícone Bateria Normal
        tft.drawRect(icon_x, icon_y, icon_w, icon_h, fg_color); tft.fillRect(term_x, term_y, term_w, term_h, fg_color);
        int fill_w_max = (icon_w > 2) ? (icon_w - 2) : 0, fill_h = (icon_h > 2) ? (icon_h - 2) : 0;
        int fill_w = fill_w_max * battery_info.level_percent / 100; fill_w = constrain(fill_w, 0, fill_w_max);
        uint16_t fill_color = COLOR_SUCCESS; if (battery_info.level_percent < 50) fill_color = TFT_YELLOW; if (battery_info.level_percent < 20) fill_color = COLOR_ERROR;
        if (fill_w > 0) tft.fillRect(icon_x + 1, icon_y + 1, fill_w, fill_h, fill_color);
        if (fill_w < fill_w_max) tft.fillRect(icon_x + 1 + fill_w, icon_y + 1, fill_w_max - fill_w, fill_h, bg_color); // Limpa restante
    }
}

// Update Sprites
void ui_updateProgressBarSprite(uint64_t t){ uint32_t r=TOTP_INTERVAL-(t%TOTP_INTERVAL); int bw=spr_progress_bar.width(),bh=spr_progress_bar.height(),pw=map(r,0,TOTP_INTERVAL,0,bw); pw=constrain(pw,0,bw); spr_progress_bar.fillRect(0,0,bw,bh,COLOR_BAR_BG); if(pw>0) spr_progress_bar.fillRect(0,0,pw,bh,COLOR_BAR_FG); }
void ui_updateTOTPCodeSprite(){ spr_totp_code.fillSprite(COLOR_BG); spr_totp_code.setTextColor(COLOR_FG); spr_totp_code.setTextSize(UI_TOTP_CODE_FONT_SIZE); if(service_count==0||!current_totp.valid_key) spr_totp_code.drawString("------",spr_totp_code.width()/2,spr_totp_code.height()/2); else spr_totp_code.drawString(current_totp.code,spr_totp_code.width()/2,spr_totp_code.height()/2); }

// Mensagem Temporária
void ui_showTemporaryMessage(const char *msg, uint16_t color, uint32_t duration_ms) {
    ScreenState previous_screen = current_screen;
    strncpy(message_buffer, msg, sizeof(message_buffer) - 1); message_buffer[sizeof(message_buffer) - 1] = '\0'; message_color = color;
    changeScreen(SCREEN_MESSAGE);
    uint32_t start_time = millis(); while (millis() - start_time < duration_ms) { delay(50); }
    // Decide para onde voltar
    if (previous_screen == SCREEN_MESSAGE || previous_screen == SCREEN_SERVICE_ADD_CONFIRM || previous_screen == SCREEN_SERVICE_DELETE_CONFIRM || previous_screen == SCREEN_TIME_EDIT || previous_screen == SCREEN_SERVICE_ADD_WAIT || previous_screen == SCREEN_TIMEZONE_EDIT) { changeScreen(SCREEN_MENU_MAIN); }
    else { changeScreen(previous_screen); }
}

// Desenho da tela de Mensagem (sem header)
void ui_drawScreenMessage(bool full_redraw) {
     if (full_redraw) {
         tft.fillScreen(COLOR_BG); // Limpa TUDO
         tft.setTextColor(message_color, COLOR_BG); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
         char temp_buffer[sizeof(message_buffer)]; strncpy(temp_buffer, message_buffer, sizeof(temp_buffer)); temp_buffer[sizeof(temp_buffer)-1] = '\0';
         char *line1 = strtok(temp_buffer, "\n"), *line2 = strtok(NULL, "\n");
         int y_pos = SCREEN_HEIGHT / 2, line_height = tft.fontHeight(2) + 5;
         if (line1 && line2) { y_pos -= line_height / 2; tft.drawString(line1, SCREEN_WIDTH / 2, y_pos); tft.drawString(line2, SCREEN_WIDTH / 2, y_pos + line_height); }
         else if (line1) { tft.drawString(line1, SCREEN_WIDTH / 2, y_pos); }
         tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); // Reset
     }
     // Tela estática
}