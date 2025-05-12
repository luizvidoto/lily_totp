/*
  TOTP Authenticator Optimized - Multi-Language & Unified Header v4.1
  Projeto para ESP32 com TFT, RTC DS3231 e geração TOTP.
*/


#include <Arduino.h>
#include <OneButton.h>
#include <TFT_eSPI.h>
#include <Base32.h>
#include <ESP32Time.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include "config.h"
#include "types.h"
#include "pin_config.h"
#include "localization.h"
#include "totp.h"
#include "ui.h"
#include "globals.h"

// Function declarations
void setupHardware();
void setupUI();
void loadPreferences();
void handleButtons();
void updateTime();
void updateBatteryStatus();
void processSerialCommands();
bool parseJsonTime(const char* json);
bool parseJsonService(const char* json);

void setup() {
    Serial.begin(115200);
    
    setupHardware();
    setupUI();
    loadPreferences();
    
    if (!initTOTPServices()) {
        ui_showTemporaryMessage(getText(STR_ERROR_NVS_LOAD), COLOR_ERROR, 3000);
    }
    
    // Button handlers
    btn_prev.attachClick([]() {
        last_activity = millis();
        if (current_screen == SCREEN_TOTP_VIEW && num_services > 0) {
            current_service_index = (current_service_index - 1 + num_services) % num_services;
        }
    });
    
    btn_next.attachClick([]() {
        last_activity = millis();
        if (current_screen == SCREEN_TOTP_VIEW && num_services > 0) {
            current_service_index = (current_service_index + 1) % num_services;
        }
    });
    
    btn_sel.attachClick([]() {
        last_activity = millis();
        if (current_screen == SCREEN_TOTP_VIEW) {
            current_screen = SCREEN_MENU_MAIN;
        }
    });
}

void loop() {
    unsigned long current_time = millis();
    
    // Update buttons
    btn_prev.tick();
    btn_next.tick();
    btn_sel.tick();
    
    // Check for serial commands
    processSerialCommands();
    
    // Update RTC sync
    if (current_time - last_rtc_sync >= RTC_SYNC_INTERVAL_MS) {
        updateTime();
        last_rtc_sync = current_time;
    }
    
    // Update battery status
    updateBatteryStatus();
    
    // Screen timeout
    if (current_time - last_activity >= INACTIVITY_TIMEOUT_MS) {
        if (current_screen != SCREEN_TOTP_VIEW) {
            current_screen = SCREEN_TOTP_VIEW;
        }
    }
    
    // Update display
    bool full_redraw = false;
    if (current_time - last_screen_update >= SCREEN_UPDATE_INTERVAL_MS) {
        ui_drawScreen(full_redraw);
        last_screen_update = current_time;
    }
    
    delay(LOOP_DELAY_MS);
}

void setupHardware() {
    // Configure pin modes
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    pinMode(PIN_VBAT, INPUT);
    
    // Initialize RTC
    rtc.setTime(0, 0, 0, 1, 1, 2024);  // Set a default time
}

void setupUI() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(COLOR_BG);
    tft.setTextDatum(TL_DATUM);
}

void loadPreferences() {
    Preferences preferences;
    if (preferences.begin("totp-app", true)) {
        timezone_offset = preferences.getInt("timezone", 0);
        current_language = (Language)preferences.getInt("language", LANG_PT_BR);
        current_strings = languages[current_language];
        preferences.end();
    }
}

void updateTime() {
    time_t epoch = rtc.getEpoch();
    current_timestamp_utc = epoch;
    
    struct tm timeinfo;
    time_t local_time = epoch + (timezone_offset * 3600);
    gmtime_r(&local_time, &timeinfo);
    
    current_hour = timeinfo.tm_hour;
    current_minute = timeinfo.tm_min;
    current_second = timeinfo.tm_sec;
}

void updateBatteryStatus() {
    // Read battery voltage and calculate percentage
    float voltage = analogReadMilliVolts(PIN_VBAT) / 1000.0f;
    battery_info.voltage = voltage;
    battery_info.is_usb_powered = voltage > 4.3f;
    
    // Calculate percentage (assuming 3.3V min, 4.2V max)
    float percentage = (voltage - 3.3f) * 100.0f / (4.2f - 3.3f);
    battery_info.level_percent = constrain(percentage, 0, 100);
}

void processSerialCommands() {
    if (Serial.available()) {
        String json = Serial.readStringUntil('\n');
        json.trim();
        
        if (current_screen == SCREEN_TIME_EDIT) {
            if (parseJsonTime(json.c_str())) {
                ui_showTemporaryMessage(getText(STR_TIME_ADJUSTED_FMT), COLOR_SUCCESS, 2000);
            } else {
                ui_showTemporaryMessage(getText(STR_ERROR_JSON_INVALID_TIME), COLOR_ERROR, 2000);
            }
        }
        else if (current_screen == SCREEN_SERVICE_ADD_WAIT) {
            if (parseJsonService(json.c_str())) {
                current_screen = SCREEN_SERVICE_ADD_CONFIRM;
            } else {
                ui_showTemporaryMessage(getText(STR_ERROR_JSON_INVALID_SERVICE), COLOR_ERROR, 2000);
            }
        }
    }
}

bool parseJsonTime(const char* json) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.println(error.c_str());
        return false;
    }
    
    if (!doc.containsKey("y") || !doc.containsKey("mo") || !doc.containsKey("d") ||
        !doc.containsKey("h") || !doc.containsKey("m") || !doc.containsKey("s")) {
        return false;
    }
    
    rtc.setTime(doc["s"], doc["m"], doc["h"], doc["d"], doc["mo"], doc["y"]);
    return true;
}

bool parseJsonService(const char* json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.println(error.c_str());
        return false;
    }
    
    if (!doc.containsKey("name") || !doc.containsKey("secret")) {
        return false;
    }
    
    const char* name = doc["name"];
    const char* secret = doc["secret"];
    
    return addTOTPService(name, secret);
}
