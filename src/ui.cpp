#include "ui.h"
#include "config.h"
#include "types.h"
#include "localization.h"
#include "totp.h"
#include <TFT_eSPI.h>

// UI state variables
TFT_eSPI tft;
TFT_eSprite spr_totp_code(&tft);
TFT_eSprite spr_progress_bar(&tft);
int menu_top_visible_index = 0;
int menu_highlight_y_current = UI_MENU_START_Y;
int menu_highlight_y_target = UI_MENU_START_Y;
unsigned long menu_animation_start_time = 0;
bool is_menu_animating = false;

// Temporary message state
static char temp_message[64];
static uint16_t temp_message_color;
static uint32_t temp_message_end_time;

void ui_drawScreen(bool full_redraw) {
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
        case SCREEN_LANGUAGE_SELECT:
            ui_drawScreenLanguageSelectContent(full_redraw);
            break;
        case SCREEN_MESSAGE:
            ui_drawScreenMessage(full_redraw);
            break;
        case SCREEN_READ_RFID:
            ui_drawScreenReadRFID(full_redraw);
            break;
    }
}

void ui_drawHeader(const char *title) {
    tft.fillRect(0, 0, tft.width(), UI_HEADER_HEIGHT, COLOR_BG);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.drawString(title, UI_PADDING, UI_PADDING);
    ui_drawClockDirect(true);
    ui_drawBatteryDirect();
}

void ui_updateHeaderDynamic() {
    ui_drawClockDirect(true);
    ui_drawBatteryDirect();
}

void ui_drawClockDirect(bool show_seconds) {
    char time_str[9];
    if (show_seconds) {
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", current_hour, current_minute, current_second);
    } else {
        snprintf(time_str, sizeof(time_str), "%02d:%02d", current_hour, current_minute);
    }
    
    tft.setTextColor(COLOR_FG, COLOR_BG);
    int16_t x = tft.width() - UI_PADDING - UI_CLOCK_WIDTH_ESTIMATE;
    int16_t y = UI_PADDING;
    tft.fillRect(x, y, UI_CLOCK_WIDTH_ESTIMATE, UI_HEADER_HEIGHT - UI_PADDING, COLOR_BG);
    tft.drawString(time_str, x, y);
}

void ui_drawBatteryDirect() {
    int x = tft.width() - UI_PADDING - UI_BATT_WIDTH;
    int y = UI_PADDING;
    
    // Draw battery outline
    tft.drawRect(x, y, UI_BATT_WIDTH, UI_BATT_HEIGHT, COLOR_FG);
    tft.fillRect(x + UI_BATT_WIDTH, y + UI_BATT_HEIGHT/4, 2, UI_BATT_HEIGHT/2, COLOR_FG);
    
    // Draw battery level
    int level_width = ((UI_BATT_WIDTH - 4) * battery_info.level_percent) / 100;
    uint16_t level_color = battery_info.level_percent > 20 ? COLOR_SUCCESS : COLOR_ERROR;
    tft.fillRect(x + 2, y + 2, level_width, UI_BATT_HEIGHT - 4, level_color);
    
    // Clear unfilled area
    if (level_width < UI_BATT_WIDTH - 4) {
        tft.fillRect(x + 2 + level_width, y + 2, 
                    UI_BATT_WIDTH - 4 - level_width, 
                    UI_BATT_HEIGHT - 4, COLOR_BG);
    }
}

void ui_updateProgressBarSprite(uint64_t current_timestamp_utc) {
    if (!spr_progress_bar.created()) {
        spr_progress_bar.createSprite(tft.width() - (UI_PADDING * 2), UI_PROGRESS_BAR_HEIGHT);
    }
    
    uint32_t interval = calculateTOTPInterval(current_timestamp_utc);
    uint32_t seconds_in_interval = current_timestamp_utc % TOTP_INTERVAL;
    int progress = (seconds_in_interval * spr_progress_bar.width()) / TOTP_INTERVAL;
    
    spr_progress_bar.fillSprite(COLOR_BAR_BG);
    if (progress > 0) {
        spr_progress_bar.fillRect(0, 0, progress, UI_PROGRESS_BAR_HEIGHT, COLOR_BAR_FG);
    }
}

void ui_updateTOTPCodeSprite() {
    if (!spr_totp_code.created()) {
        spr_totp_code.createSprite(tft.width() - (UI_PADDING * 2), 
                                  tft.fontHeight(UI_TOTP_CODE_FONT_SIZE));
    }
    
    spr_totp_code.fillSprite(COLOR_BG);
    spr_totp_code.setTextColor(COLOR_FG, COLOR_BG);
    spr_totp_code.setTextDatum(TC_DATUM);
    spr_totp_code.drawString(current_totp.code, 
                            spr_totp_code.width() / 2, 
                            0, UI_TOTP_CODE_FONT_SIZE);
}

void ui_showTemporaryMessage(const char *message, uint16_t color, uint32_t duration) {
    strlcpy(temp_message, message, sizeof(temp_message));
    temp_message_color = color;
    temp_message_end_time = millis() + duration;
    current_screen = SCREEN_MESSAGE;
}

// Screen-specific implementations
void ui_drawScreenTOTPContent(bool full_redraw) {
    if (full_redraw) {
        tft.fillScreen(COLOR_BG);
        ui_drawHeader(getText(STR_TITLE_TOTP_CODE));
    }
    
    if (num_services > 0) {
        // Service name
        tft.setTextColor(COLOR_FG, COLOR_BG);
        tft.fillRect(UI_PADDING, UI_CONTENT_Y_START, 
                    tft.width() - (UI_PADDING * 2), 30, COLOR_BG);
        tft.drawString(services[current_service_index].name, 
                      tft.width() / 2, UI_CONTENT_Y_START, 2);
        
        // TOTP code
        ui_updateTOTPCodeSprite();
        spr_totp_code.pushSprite(UI_PADDING, UI_CONTENT_Y_START + 40);
        
        // Progress bar
        ui_updateProgressBarSprite(current_timestamp_utc);
        spr_progress_bar.pushSprite(UI_PADDING, 
                                   UI_CONTENT_Y_START + 80);
    } else {
        tft.setTextColor(COLOR_ERROR, COLOR_BG);
        tft.drawString(getText(STR_ERROR_NO_SERVICES), 
                      tft.width() / 2, tft.height() / 2, 2);
    }
    
    // Footer
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawString(getText(STR_FOOTER_GENERIC_NAV), 
                  tft.width() / 2, tft.height() - 20, 1);
}

// Implementation for other screen drawing functions follows similar pattern
// Each screen has its own layout and interaction elements
// For brevity, only the TOTP view implementation is shown here
void ui_drawScreenMenuContent(bool full_redraw) {
    // Menu screen implementation
}

void ui_drawScreenServiceAddWaitContent(bool full_redraw) {
    // Service add screen implementation
}

void ui_drawScreenServiceAddConfirmContent(bool full_redraw) {
    // Service confirm screen implementation
}

void ui_drawScreenTimeEditContent(bool full_redraw) {
    // Time edit screen implementation
}

void ui_drawScreenServiceDeleteConfirmContent(bool full_redraw) {
    // Delete confirm screen implementation
}

void ui_drawScreenTimezoneEditContent(bool full_redraw) {
    // Timezone edit screen implementation
}

void ui_drawScreenLanguageSelectContent(bool full_redraw) {
    // Language selection screen implementation
}

void ui_drawScreenMessage(bool full_redraw) {
    // Message screen implementation
}

void ui_drawScreenReadRFID(bool full_redraw) {
    // RFID reading screen implementation
}