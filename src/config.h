#pragma once

#include <Arduino.h>
#include "pin_config.h"

// ---- Constantes e Configurações ----
constexpr uint32_t TOTP_INTERVAL = 30;
constexpr int MAX_SERVICES = 50;
constexpr size_t MAX_SERVICE_NAME_LEN = 20;
constexpr size_t MAX_SECRET_B32_LEN = 64;
constexpr size_t MAX_SECRET_BIN_LEN = 40;

// Constantes de UI e Timing
constexpr int VISIBLE_MENU_ITEMS = 3;
constexpr uint32_t INACTIVITY_TIMEOUT_MS = 30000;
constexpr uint32_t RTC_SYNC_INTERVAL_MS = 60000;
constexpr uint32_t SCREEN_UPDATE_INTERVAL_MS = 1000;
constexpr int MENU_ANIMATION_DURATION_MS = 150;
constexpr uint32_t LOOP_DELAY_MS = 15;

// Cores da Interface
#define COLOR_BG TFT_BLACK
#define COLOR_FG TFT_WHITE
#define COLOR_ACCENT TFT_CYAN
#define COLOR_SUCCESS TFT_GREEN
#define COLOR_ERROR TFT_RED
#define COLOR_BAR_FG TFT_WHITE
#define COLOR_BAR_BG TFT_DARKGREY
#define COLOR_HIGHLIGHT_BG TFT_WHITE
#define COLOR_HIGHLIGHT_FG TFT_BLACK

// Posições e Tamanhos da UI
#define UI_PADDING 5
#define UI_HEADER_HEIGHT 25
#define UI_CLOCK_WIDTH_ESTIMATE 85
#define UI_BATT_WIDTH 24
#define UI_BATT_HEIGHT 16
#define UI_CONTENT_Y_START (UI_HEADER_HEIGHT + UI_PADDING)
#define UI_TOTP_CODE_FONT_SIZE 3
#define UI_PROGRESS_BAR_HEIGHT 15
#define UI_MENU_ITEM_HEIGHT 30
#define UI_MENU_START_Y UI_CONTENT_Y_START