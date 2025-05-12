#pragma once

#include <Arduino.h>
#include <ESP32Time.h>
#include <OneButton.h>
#include <TFT_eSPI.h>
#include <MFRC522.h>
#include "types.h"

// Hardware Interfaces
extern TFT_eSPI tft;
extern TFT_eSprite spr_totp_code;
extern TFT_eSprite spr_progress_bar;
extern ESP32Time rtc;
extern OneButton btn_prev;
extern OneButton btn_next;
extern OneButton btn_sel;
extern MFRC522 mfrc522;

// Application State
extern ScreenState current_screen;
extern Language current_language;
extern const LanguageStrings *current_strings;
extern int current_service_index;
extern int current_language_menu_index;
extern int8_t timezone_offset;
extern char card_id[16];

// Time Management
extern uint64_t current_timestamp_utc;
extern int current_hour;
extern int current_minute;
extern int current_second;
extern unsigned long last_rtc_sync;
extern unsigned long last_screen_update;
extern unsigned long last_activity;

// Menu State
extern int menu_top_visible_index;
extern int menu_highlight_y_current;
extern int menu_highlight_y_target;
extern unsigned long menu_animation_start_time;
extern bool is_menu_animating;

// System Status
extern BatteryInfo battery_info;

// Constants
extern const char* NVS_LANG_KEY;
extern const char* NVS_TZ_OFFSET_KEY;
extern const StringID menuOptionIDs[];
extern const int NUM_MENU_OPTIONS;