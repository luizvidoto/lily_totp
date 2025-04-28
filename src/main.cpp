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
#include <MFRC522.h>
#include "mbedtls/md.h"

#include "config.h"
#include "globals.h"
#include "types.h"
#include "hardware.h"
#include "i18n.h"
#include "storage.h"
#include "totp.h"
#include "input.h"
#include "ui.h"

// ---- Protótipos de Funções ----
// Core Logic & Hardware
void setup();
void loop();


// ---- Setup e Loop Principal ----
void setup() {
  Serial.begin(115200);
  while (!Serial); // Espera Serial (opcional, mas bom para debug inicial)
  Serial.println("\n[SETUP] Iniciando TOTP Authenticator Multi-Idioma v4.1...");

  // Carrega configurações ANTES de usar textos na inicialização do HW
  preferences.begin("totp-app", true); // Abre NVS para leitura
  int saved_lang_index = preferences.getInt(NVS_KEY_LANGUAGE, (int)Language::PT_BR); // Padrão PT_BR se não existir
  if (saved_lang_index >= 0 && saved_lang_index < NUM_LANGUAGES) {
      current_language = (Language)saved_lang_index;
  } else {
      current_language = Language::PT_BR; // Fallback se valor salvo for inválido
      Serial.printf("[WARN] Idioma NVS inválido (%d), usando padrão.\n", saved_lang_index);
  }
  //current_strings = languages[current_language]; // Define ponteiro global para textos
  gmt_offset_hours = preferences.getInt(NVS_KEY_TZ_OFFSET, 0); // Padrão GMT+0
  preferences.end(); // Fecha NVS
  Serial.printf("[SETUP] Idioma: %d, Fuso: GMT%+d\n", current_language, gmt_offset_hours);

  // Inicializa Hardware (RTC, TFT, Pinos)
  initBaseHardware(); // Usa getText internamente para msg de erro RTC se necessário

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
    current_totp.valid_key_loaded = false; // Nenhum serviço
    snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(STR_TOTP_CODE_ERROR));
  }

  // Configura callbacks dos botões
  configureButtonCallbacks();
  Serial.println("[SETUP] Botões configurados.");

  // Configuração inicial de energia e timers
  updateBatteryStatus();
  current_brightness_level = battery_info.is_usb_powered ? BRIGHTNESS_USB : BRIGHTNESS_BATTERY;
  setScreenBrightness(current_brightness_level);
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
