#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "types.h" // Para ScreenState, StringID, etc.
#include <stdint.h> // Para uint16_t

// ---- Funções de Inicialização da UI ----
void ui_init_display();
void ui_init_sprites();

// ---- Gerenciamento de Tela e Desenho ----
void ui_change_screen(ScreenState new_screen, bool force_full_redraw = true);
void ui_draw_screen(bool full_redraw); // Função principal de desenho

// ---- Mensagens Temporárias ----
// Define a mensagem, cor, duração e para qual tela ir depois.
// A exibição real acontece no próximo ciclo de ui_draw_screen se current_screen == SCREEN_MESSAGE.
void ui_queue_message(const char* msg, uint16_t color, uint32_t duration_ms, ScreenState next_screen_after_msg);
void ui_queue_message_fmt(StringID str_id, uint16_t color, uint32_t duration_ms, ScreenState next_screen_after_msg, ...); // Para mensagens formatadas

// ---- Funções Auxiliares de Desenho (internas ao ui_manager.cpp, mas podem ser expostas se necessário) ----
// Estas são chamadas por ui_draw_screen e não diretamente do loop principal.

// Funções de atualização de Sprites (chamadas antes de desenhar a tela TOTP ou header)
void ui_update_totp_code_sprite();
void ui_update_progress_bar_sprite(uint64_t current_timestamp_utc);
void ui_update_header_clock_sprite(bool show_seconds);
void ui_update_header_battery_sprite();

// Funções para obter títulos e rodapés (usando i18n)
const char* ui_get_screen_title_string(ScreenState state);
const char* ui_get_footer_string(ScreenState state);

#endif // UI_MANAGER_H