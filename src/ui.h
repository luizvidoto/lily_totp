#pragma once

#include "types.h"
#include "globals.h"

// Drawing functions
void ui_drawScreen(bool full_redraw);
void ui_drawHeader(const char *title);
void ui_updateHeaderDynamic();
void ui_drawClockDirect(bool show_seconds);
void ui_drawBatteryDirect();
void ui_updateProgressBarSprite(uint64_t current_timestamp_utc);
void ui_updateTOTPCodeSprite();
void ui_showTemporaryMessage(const char *message, uint16_t color, uint32_t duration);

// Screen-specific drawing functions
void ui_drawScreenTOTPContent(bool full_redraw);
void ui_drawScreenMenuContent(bool full_redraw);
void ui_drawScreenServiceAddWaitContent(bool full_redraw);
void ui_drawScreenServiceAddConfirmContent(bool full_redraw);
void ui_drawScreenTimeEditContent(bool full_redraw);
void ui_drawScreenServiceDeleteConfirmContent(bool full_redraw);
void ui_drawScreenTimezoneEditContent(bool full_redraw);
void ui_drawScreenLanguageSelectContent(bool full_redraw);
void ui_drawScreenMessage(bool full_redraw);
void ui_drawScreenReadRFID(bool full_redraw);