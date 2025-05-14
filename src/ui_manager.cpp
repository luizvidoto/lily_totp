#include "ui_manager.h"
#include "globals.h"    // Acesso a tft, sprites, current_screen, service_count, etc.
#include "config.h"     // Acesso a constantes de UI, cores, etc.
#include "i18n.h"       // Para getText()
#include "time_keeper.h"// Para time_now_utc()
#include <Arduino.h>    // Para Serial, snprintf, millis, etc.
#include <stdarg.h>     // Para va_list em ui_queue_message_fmt

// Variáveis estáticas para dimensões dinâmicas da UI
static int screen_width = 0;
static int screen_height = 0;
static int progress_bar_width = 0;
static int progress_bar_x = 0;
static int footer_text_y = 0;

// ---- Icon Bitmaps ----
// Provided by user
static const unsigned char PROGMEM codes_menu_icon[] = {0x3f,0xff,0x00,0x40,0x00,0x80,0x80,0x00,0x40,0x8f,0xfc,0x40,0x9f,0xfe,0x40,0x9f,0xfe,0x40,0x8f,0xfc,0x40,0x80,0x00,0x40,0x80,0x00,0x40,0x9c,0x36,0x40,0x9c,0x36,0x40,0x80,0x00,0x40,0x40,0x00,0x80,0x3f,0xff,0x00}; // New codes icon (pager-like)
static const unsigned char PROGMEM image_pager_icon_bits[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xff,0xff,0xff,0xff,0xc0,0x07,0xff,0xff,0xff,0xff,0xe0,0x0f,0xff,0xff,0xff,0xff,0xf0,0x1f,0xff,0xff,0xff,0xff,0xf8,0x1f,0x00,0x00,0x00,0x00,0xf8,0x1e,0x00,0x00,0x00,0x00,0x78,0x1e,0x00,0x08,0x00,0x00,0x78,0x1e,0x3b,0xde,0x37,0x90,0x78,0x1e,0x28,0x42,0x56,0x30,0x78,0x1e,0x08,0xcc,0xd7,0xbc,0x78,0x1e,0x09,0x82,0xf0,0xe4,0x78,0x1e,0x0b,0xde,0x17,0xbc,0x78,0x1e,0x0b,0xcc,0x13,0x10,0x78,0x1e,0x00,0x00,0x00,0x00,0x78,0x1f,0x00,0x00,0x00,0x00,0xf8,0x1f,0xff,0xff,0xff,0xff,0xf8,0x1f,0xff,0xff,0xff,0xff,0xf8,0x1f,0xff,0xff,0xff,0xff,0xf8,0x1f,0xff,0xff,0xff,0xff,0xf8,0x1f,0x00,0x7e,0x1f,0xc3,0xf8,0x1e,0x00,0x7c,0x0f,0x81,0xf8,0x1e,0xff,0x3c,0x0f,0x81,0xf8,0x1e,0x7f,0x3c,0x0f,0x81,0xf8,0x1e,0x00,0x7e,0x0f,0xc1,0xf8,0x1f,0x00,0xff,0x3f,0xe7,0xf8,0x1f,0xff,0xff,0xff,0xff,0xf8,0x1f,0xff,0xff,0xff,0xff,0xf8,0x0f,0xff,0xff,0xff,0xff,0xf0,0x0f,0xff,0xff,0xff,0xff,0xe0,0x03,0xff,0xff,0xff,0xff,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char PROGMEM image_config_icon_bits[] = {0x03,0xc0,0x12,0x48,0x2c,0x34,0x40,0x02,0x23,0xc4,0x24,0x24,0xc8,0x13,0x88,0x11,0x88,0x11,0xc8,0x13,0x24,0x24,0x23,0xc4,0x40,0x02,0x2c,0x34,0x12,0x48,0x03,0xc0};
static const unsigned char PROGMEM image_date_icon_bits[] = {0x00,0x00,0x09,0x20,0x7f,0xfc,0xc9,0x26,0x80,0x02,0x8c,0x22,0x92,0x62,0x92,0xa2,0x82,0x22,0x84,0x22,0x88,0x22,0x90,0x22,0x9e,0xf2,0xc0,0x06,0x7f,0xfc,0x00,0x00};
static const unsigned char PROGMEM image_lang_icon_bits[] = {0x01,0x00,0x03,0x80,0x03,0x80,0x07,0xc0,0x0f,0xe0,0x0f,0xe0,0x1f,0xf0,0x3f,0xf8,0x7f,0xfc,0x3f,0xf8,0x1f,0xf0,0x0f,0xe0,0x0f,0xe0,0x07,0xc0,0x03,0x80,0x01,0x00};
static const unsigned char PROGMEM image_rfid_icon_bits[] = {0x01,0xf0,0x02,0x08,0x04,0x34,0x04,0x4a,0x04,0x4a,0x04,0x32,0x04,0x02,0x08,0x02,0x12,0x04,0x25,0xf8,0x4b,0x00,0x94,0x00,0xac,0x00,0x90,0x00,0xf0,0x00,0x00,0x00};
static const unsigned char PROGMEM image_zones_icon_bits[] = {0x00,0x07,0x70,0x09,0x8e,0x11,0xc1,0xa2,0x30,0x44,0x0c,0x08,0x03,0x10,0x01,0x08,0x02,0xc8,0x05,0x44,0x7a,0x24,0x8c,0x24,0x64,0x12,0x14,0x12,0x14,0x0a,0x08,0x0c};
static const unsigned char PROGMEM image_padlock_icon_bits[] = {0x0f,0x80,0x10,0x40,0x27,0x20,0x48,0x90,0x50,0x50,0x50,0x50,0x7f,0xf0,0xc0,0x18,0xa7,0x28,0x88,0x88,0x88,0x88,0x85,0x08,0x85,0x08,0xa2,0x28,0xc0,0x18,0x7f,0xf0};

// Icon dimensions
const int ICON_PAGER_WIDTH = 48; // New pager icon dimensions
const int ICON_PAGER_HEIGHT = 32;
const int ICON_NEW_CODES_WIDTH = 24; // New codes icon (pager-like) dimensions
const int ICON_NEW_CODES_HEIGHT = 15;
const int ICON_DEFAULT_WIDTH = 16;
const int ICON_DEFAULT_HEIGHT = 16;
const int ICON_PADLOCK_WIDTH = 16;
const int ICON_PADLOCK_HEIGHT = 16;

// Helper function to reverse bits in a byte
static uint8_t reverse_byte_bits(uint8_t b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

// Protótipos de funções de desenho específicas de cada tela (internas)
static void ui_draw_header_static(const char* title_text);
static void ui_draw_header_dynamic_sprites();
static void ui_draw_footer(const char* footer_text);

static void ui_draw_content_startup(bool full_redraw);
static void ui_draw_content_totp_view(bool full_redraw);
static void ui_draw_content_menu_main(bool full_redraw);
static void ui_draw_content_service_add_wait(bool full_redraw);
static void ui_draw_content_service_add_confirm(bool full_redraw);
static void ui_draw_content_time_edit(bool full_redraw);
static void ui_draw_content_service_delete_confirm(bool full_redraw);
static void ui_draw_content_timezone_edit(bool full_redraw);
static void ui_draw_content_language_select(bool full_redraw);
static void ui_draw_content_message(bool full_redraw); // Tela de mensagem é especial
static void ui_draw_content_read_rfid(bool full_redraw);
static void ui_draw_content_locked(bool full_redraw); // New lock screen content function
static void ui_draw_content_manage_cards(bool full_redraw); // New manage cards screen
static void ui_draw_content_add_card_wait(bool full_redraw);
static void ui_draw_content_add_card_confirm(bool full_redraw);
static void ui_draw_content_delete_card_confirm(bool full_redraw);
static void ui_draw_content_config_menu(bool full_redraw); // New config submenu
static void ui_draw_content_adjust_lock_timeout(bool full_redraw); // New adjust lock timeout screen
static void ui_draw_content_brightness_menu(bool full_redraw);       // New brightness selection menu
static void ui_draw_content_adjust_brightness_value(bool full_redraw); // New adjust brightness value screen
static void ui_draw_content_setup_first_card(bool full_redraw); // New: Setup first card screen


// ---- Funções de Inicialização da UI ----
void ui_init_display() {
    tft.init();
    tft.setRotation(3); // Ajuste conforme seu display
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextDatum(TL_DATUM); // Padrão Top-Left
    tft.setTextSize(1);
    Serial.println("[UI] Display TFT inicializado.");

    // --- CALCULA E ARMAZENA DIMENSÕES DINÂMICAS ---
    screen_width = tft.width();
    screen_height = tft.height();
    progress_bar_width = screen_width - 2 * UI_PADDING - 20; // Calcula baseado na largura real
    progress_bar_x = (screen_width - progress_bar_width) / 2;
    footer_text_y = screen_height - (tft.fontHeight(1) / 2) - (UI_PADDING / 2) ; // Calcula Y do footer dinamicamente

    Serial.printf("[UI] Dimensões calculadas: W=%d, H=%d, BarW=%d, BarX=%d, FootY=%d\n",
                  screen_width, screen_height, progress_bar_width, progress_bar_x, footer_text_y);
}

void ui_init_sprites() {
    // Sprite para o código TOTP
    spr_totp_code.setColorDepth(16); // Mais cores para antialiasing da fonte grande
    spr_totp_code.createSprite(160, 40); // Ajustar tamanho conforme UI_TOTP_CODE_FONT_SIZE
    spr_totp_code.setTextDatum(MC_DATUM);
    spr_totp_code.setTextColor(COLOR_FG, COLOR_BG); // Definir cores padrão

    // Sprite para a barra de progresso
    spr_progress_bar.setColorDepth(8); // Menos cores são suficientes
    // -4 para dar uma pequena borda ao redor da barra dentro do contorno desenhado diretamente na tft
    spr_progress_bar.createSprite(progress_bar_width - 4, UI_PROGRESS_BAR_HEIGHT - 4);
    // Não precisa de setTextColor ou setTextDatum aqui, pois é só preenchimento

    // Sprite para o relógio no header
    spr_header_clock.setColorDepth(8); // 8 bits devem ser suficientes
    spr_header_clock.createSprite(UI_CLOCK_WIDTH_ESTIMATE, UI_HEADER_HEIGHT - 4); // Altura um pouco menor que header
    spr_header_clock.setTextDatum(MR_DATUM); // Middle Right para alinhar à direita
    spr_header_clock.setTextColor(COLOR_BG, COLOR_ACCENT); // Texto escuro sobre fundo do header

    // Sprite para o ícone de bateria no header
    spr_header_battery.setColorDepth(8);
    spr_header_battery.createSprite(UI_BATT_WIDTH, UI_BATT_HEIGHT);
    // Não precisa de texto, será desenhado graficamente

    Serial.println("[UI] Sprites inicializados.");
}


// ---- Gerenciamento de Tela e Desenho ----
void ui_change_screen(ScreenState new_screen, bool force_full_redraw) {
    if (current_screen == new_screen && !force_full_redraw) {
        // Se a tela é a mesma e não está forçando redraw, apenas atualiza interaçao
        last_interaction_time = millis();
        return;
    }

    previous_screen = current_screen; // Guarda a tela anterior
    current_screen = new_screen;
    Serial.printf("[UI] Mudando para tela: %d\n", static_cast<int>(new_screen));

    is_menu_animating = false; // Para qualquer animação de menu ao sair da tela de menu
    menu_highlight_y_current = -1; // Reseta posição Y do highlight do menu

    // Ajustes específicos ao entrar em certas telas
    if (new_screen == ScreenState::SCREEN_MENU_MAIN && service_count > 0) {
        // Tenta centralizar o item de menu selecionado
        menu_top_visible_index = max(0, min(current_menu_index - VISIBLE_MENU_ITEMS / 2,
                                            NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS)); // Usa NUM_MENU_OPTIONS
    } else if (new_screen == ScreenState::SCREEN_TOTP_VIEW && service_count == 0) {
        // Se não há serviços, não deveria estar na tela TOTP. Redireciona para o menu.
        // Ou mostra uma mensagem e depois vai para o menu.
        ui_queue_message(getText(StringID::STR_ERROR_NO_SERVICES), COLOR_ACCENT, 2000, ScreenState::SCREEN_MENU_MAIN);
        current_screen = ScreenState::SCREEN_MESSAGE; // Força a tela de mensagem
    }

    ui_draw_screen(true); // Força um redesenho COMPLETO da nova tela
    last_interaction_time = millis(); // Reseta timer de inatividade
}

void ui_draw_screen(bool full_redraw) {
    // A tela de mensagem é especial, ocupa a tela inteira e não tem header/footer padrão.
    if (current_screen == ScreenState::SCREEN_MESSAGE) {
        ui_draw_content_message(full_redraw); // full_redraw aqui geralmente será true vindo de ui_change_screen
        return;
    }

    // --- LIMPEZA INICIAL EM FULL REDRAW (EXCETO PARA MENSAGEM) ---
    if (full_redraw) {
        tft.fillScreen(COLOR_BG); // Limpa a tela inteira com a cor de fundo padrão
    }

    // --- Header ---
    if (full_redraw) {
        // Desenha o título estático do header apenas no redraw completo
        ui_draw_header_static(ui_get_screen_title_string(current_screen));
    }
    // Atualiza e desenha os sprites dinâmicos do header (relógio, bateria) SEMPRE que a tela é desenhada
    // (exceto se for uma atualização muito rápida e nada mudou, mas por simplicidade, atualizamos)
    ui_draw_header_dynamic_sprites();


    // --- Conteúdo Específico da Tela (Abaixo do Header, Acima do Footer) ---
    // A área de conteúdo é entre UI_CONTENT_Y_START e tft.height() - altura_footer
    // Cada função de conteúdo é responsável por limpar sua área se full_redraw for true.
    switch (current_screen) {
        case ScreenState::SCREEN_STARTUP:                   ui_draw_content_startup(full_redraw); break;
        case ScreenState::SCREEN_LOCKED:                    ui_draw_content_locked(full_redraw); break; // Call new lock screen function
        case ScreenState::SCREEN_TOTP_VIEW:                 ui_draw_content_totp_view(full_redraw); break;
        case ScreenState::SCREEN_MENU_MAIN:                 ui_draw_content_menu_main(full_redraw); break;
        case ScreenState::SCREEN_SERVICE_ADD_WAIT:          ui_draw_content_service_add_wait(full_redraw); break;
        case ScreenState::SCREEN_SERVICE_ADD_CONFIRM:       ui_draw_content_service_add_confirm(full_redraw); break;
        case ScreenState::SCREEN_TIME_EDIT:                 ui_draw_content_time_edit(full_redraw); break;
        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM:    ui_draw_content_service_delete_confirm(full_redraw); break;
        case ScreenState::SCREEN_TIMEZONE_EDIT:             ui_draw_content_timezone_edit(full_redraw); break;
        case ScreenState::SCREEN_LANGUAGE_SELECT:           ui_draw_content_language_select(full_redraw); break;
        case ScreenState::SCREEN_READ_RFID:                 ui_draw_content_read_rfid(full_redraw); break;
        case ScreenState::SCREEN_MANAGE_CARDS:              ui_draw_content_manage_cards(full_redraw); break;
        case ScreenState::SCREEN_ADD_CARD_WAIT:             ui_draw_content_add_card_wait(full_redraw); break;
        case ScreenState::SCREEN_ADD_CARD_CONFIRM:          ui_draw_content_add_card_confirm(full_redraw); break;
        case ScreenState::SCREEN_DELETE_CARD_CONFIRM:       ui_draw_content_delete_card_confirm(full_redraw); break;
        case ScreenState::SCREEN_CONFIG_MENU:               ui_draw_content_config_menu(full_redraw); break;
        case ScreenState::SCREEN_ADJUST_LOCK_TIMEOUT:       ui_draw_content_adjust_lock_timeout(full_redraw); break;
        case ScreenState::SCREEN_BRIGHTNESS_MENU:           ui_draw_content_brightness_menu(full_redraw); break;
        case ScreenState::SCREEN_ADJUST_BRIGHTNESS_VALUE:   ui_draw_content_adjust_brightness_value(full_redraw); break;
        case ScreenState::SCREEN_SETUP_FIRST_CARD:          ui_draw_content_setup_first_card(full_redraw); break;
        // SCREEN_MESSAGE é tratado no início da função.
        default:
            if (full_redraw) { // Tela desconhecida, limpa área de conteúdo
                tft.fillRect(0, UI_CONTENT_Y_START, tft.width(), tft.height() - UI_CONTENT_Y_START, COLOR_BG);
                tft.setTextColor(COLOR_ERROR); tft.setTextDatum(MC_DATUM);
                tft.drawString("Unknown Screen!", tft.width()/2, tft.height()/2);
                tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG);
            }
            break;
    }

    // --- Footer ---
    // O footer é geralmente estático para cada tela, então só redesenhamos em full_redraw.
    // Algumas telas podem não ter footer ou ter um footer dinâmico (tratado em sua ui_draw_content_*)
    if (full_redraw) {
        const char* footer_str = ui_get_footer_string(current_screen);
        if (footer_str) { // Se houver um footer definido para esta tela
            ui_draw_footer(footer_str);
        } else { 
            // Se não há footer, a área já foi limpa pelo fillScreen(COLOR_BG)
            // uint8_t prev_size = tft.textsize;
            // tft.setTextSize(1);
            // int footer_h_approx = tft.fontHeight() + UI_PADDING;
            // tft.setTextSize(prev_size);
            // tft.fillRect(0, screen_height - footer_h_approx, screen_width, footer_h_approx, COLOR_BG); // Não mais necessário aqui
        }
    }
}

// ---- Desenho do Header ----
static void ui_draw_header_static(const char* title_text) {
    tft.fillRect(0, 0, tft.width(), UI_HEADER_HEIGHT, COLOR_ACCENT); // Fundo do header

    uint8_t prev_datum = tft.getTextDatum();
    uint16_t prev_fg = tft.textcolor;
    uint16_t prev_bg = tft.textbgcolor;
    uint8_t prev_size = tft.textsize;

    tft.setTextColor(COLOR_BG, COLOR_ACCENT); // Texto escuro sobre fundo do header
    tft.setTextDatum(ML_DATUM); // Middle Left
    tft.setTextSize(2);

    // Truncamento inteligente do título se necessário
    char display_title[MAX_SERVICE_NAME_LEN + 4]; // "NomeServicoMuitoLongo..."
    int title_width = tft.textWidth(title_text);
    // Largura disponível para o título (descontando paddings e espaço para relógio/bateria)
    int available_width = tft.width() - UI_PADDING - (UI_CLOCK_WIDTH_ESTIMATE + UI_BATT_WIDTH + UI_PADDING * 2);

    if (title_width > available_width) {
        int len = strlen(title_text);
        int cutoff = len;
        // Reduz o comprimento até caber, adicionando "..."
        while (cutoff > 0 && tft.textWidth(String(title_text).substring(0, cutoff) + "...") > available_width) {
            cutoff--;
        }
        if (cutoff > 0) {
            snprintf(display_title, sizeof(display_title), "%.*s...", cutoff, title_text);
        } else { // Se nem "..." cabe, mostra só "..."
            strcpy(display_title, "...");
        }
    } else {
        strncpy(display_title, title_text, sizeof(display_title) -1);
        display_title[sizeof(display_title)-1] = '\0';
    }

    tft.drawString(display_title, UI_PADDING, UI_HEADER_HEIGHT / 2);

    tft.setTextDatum(prev_datum);
    tft.setTextColor(prev_fg, prev_bg);
    tft.setTextSize(prev_size);
}

static void ui_draw_header_dynamic_sprites() {
    // Atualiza os sprites do relógio e da bateria
    ui_update_header_clock_sprite(true); // true para mostrar segundos
    ui_update_header_battery_sprite();

    // Calcula posições para os sprites no header
    int clock_x = tft.width() - UI_PADDING - UI_BATT_WIDTH - UI_PADDING - spr_header_clock.width();
    int clock_y = (UI_HEADER_HEIGHT - spr_header_clock.height()) / 2;
    int batt_x = tft.width() - UI_PADDING - spr_header_battery.width();
    int batt_y = (UI_HEADER_HEIGHT - spr_header_battery.height()) / 2;

    // Push dos sprites para o TFT
    spr_header_clock.pushSprite(clock_x, clock_y);
    spr_header_battery.pushSprite(batt_x, batt_y);
}

// ---- Desenho do Footer ----
static void ui_draw_footer(const char* footer_text) {
    if (!footer_text || strlen(footer_text) == 0) return;

    uint8_t prev_datum = tft.getTextDatum();
    uint16_t prev_fg = tft.textcolor;
    uint16_t prev_bg = tft.textbgcolor;
    uint8_t prev_size = tft.textsize;

    // Calcula a altura da área do rodapé para limpar corretamente
    // Assume que o footer usa tamanho de texto 1
    tft.setTextSize(1);
    int text_height = tft.fontHeight(); // Altura do texto no tamanho 1
    int footer_area_h = text_height + UI_PADDING; // Altura total da área do footer
    int footer_area_y = screen_height - footer_area_h; // Y inicial da área do footer

    // Limpa a área do rodapé com a cor de fundo principal
    tft.fillRect(0, footer_area_y, screen_width, footer_area_h, COLOR_BG);

    // Configurações para desenhar o texto do rodapé
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(COLOR_DIM_TEXT, COLOR_BG); // Cor do texto e fundo para o texto
    // tft.setTextSize(1); // Já definido acima

    tft.drawString(footer_text, screen_width / 2, footer_text_y); // Usa a variável global footer_text_y

    // Restaura estado do texto
    tft.setTextDatum(prev_datum);
    tft.setTextColor(prev_fg, prev_bg);
    tft.setTextSize(prev_size);
}


// ---- Funções de Atualização de Sprites ----
void ui_update_totp_code_sprite() {
    spr_totp_code.fillSprite(COLOR_BG); // Limpa com cor de fundo da tela
    spr_totp_code.setTextSize(UI_TOTP_CODE_FONT_SIZE);
    // Cor já deve estar definida em ui_init_sprites ou pode ser ajustada aqui se necessário
    // spr_totp_code.setTextColor(current_totp.valid_key ? COLOR_FG : COLOR_ERROR, COLOR_BG);
    spr_totp_code.drawString(current_totp.code, spr_totp_code.width() / 2, spr_totp_code.height() / 2);
}

void ui_update_progress_bar_sprite(uint64_t current_timestamp_utc) {
    uint32_t seconds_elapsed_in_interval = current_timestamp_utc % TOTP_INTERVAL;
    uint32_t seconds_remaining = TOTP_INTERVAL - seconds_elapsed_in_interval;

    int bar_width_pixels = spr_progress_bar.width(); // Largura interna do sprite
    int fill_width = map(seconds_remaining, 0, TOTP_INTERVAL, 0, bar_width_pixels);
    fill_width = constrain(fill_width, 0, bar_width_pixels);

    spr_progress_bar.fillSprite(COLOR_BAR_BG); // Fundo da barra
    if (fill_width > 0) {
        spr_progress_bar.fillRect(0, 0, fill_width, spr_progress_bar.height(), COLOR_BAR_FG); // Preenchimento
    }
}

void ui_update_header_clock_sprite(bool show_seconds) {
    time_t lt = time_local_now(); // Usa função de time_keeper para obter hora local

    char time_str[9]; // "HH:MM:SS\0"
    if (show_seconds) {
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour(lt), minute(lt), second(lt));
    } else {
        snprintf(time_str, sizeof(time_str), "%02d:%02d", hour(lt), minute(lt));
    }

    spr_header_clock.fillSprite(COLOR_ACCENT); // Fundo do header
    spr_header_clock.setTextSize(2); // Tamanho do texto do relógio
    // Cor já definida em init (COLOR_BG sobre COLOR_ACCENT)
    spr_header_clock.drawString(time_str, spr_header_clock.width() - 2, spr_header_clock.height() / 2); // -2 para um pequeno padding direito
}

void ui_update_header_battery_sprite() {
    spr_header_battery.fillSprite(COLOR_ACCENT); // Fundo do header

    int icon_w = spr_header_battery.width() - 2; // -2 para bordas
    int icon_h = spr_header_battery.height() - 2;
    int icon_x = 1, icon_y = 1; // Posição relativa dentro do sprite

    int term_w = 2; // Largura do terminal da bateria
    int term_h = icon_h / 2;
    int term_x = icon_x + icon_w;
    int term_y = icon_y + (icon_h - term_h) / 2;

    uint16_t fg_color = COLOR_BG; // Cor do contorno e elementos da bateria (contraste com COLOR_ACCENT)

    if (battery_info.is_usb_powered) {
        spr_header_battery.drawRect(icon_x, icon_y, icon_w, icon_h, fg_color);
        spr_header_battery.fillRect(term_x, term_y, term_w, term_h, fg_color);
        // Desenho do raio (simplificado)
        int bolt_cx = icon_x + icon_w / 2;
        int bolt_cy = icon_y + icon_h / 2;
        // Pontos do raio: (x,y)
        // P1(cx, icon_y+1) P2(cx-2, bolt_cy) P3(cx+1, bolt_cy) P4(cx-1, icon_y+icon_h-1)
        spr_header_battery.drawLine(bolt_cx, icon_y + 2, bolt_cx - 2, bolt_cy, fg_color);
        spr_header_battery.drawLine(bolt_cx - 2, bolt_cy, bolt_cx + 1, bolt_cy, fg_color);
        spr_header_battery.drawLine(bolt_cx + 1, bolt_cy, bolt_cx -1, icon_y + icon_h - 2, fg_color);

    } else {
        spr_header_battery.drawRect(icon_x, icon_y, icon_w, icon_h, fg_color);
        spr_header_battery.fillRect(term_x, term_y, term_w, term_h, fg_color);

        int fill_max_w = icon_w - 2; // Largura interna para preenchimento
        int fill_h = icon_h - 2;     // Altura interna
        int fill_x = icon_x + 1;
        int fill_y = icon_y + 1;

        int fill_w = map(battery_info.level_percent, 0, 100, 0, fill_max_w);
        fill_w = constrain(fill_w, 0, fill_max_w);

        uint16_t fill_color = COLOR_SUCCESS; // Verde
        if (battery_info.level_percent < 50) fill_color = TFT_YELLOW;
        if (battery_info.level_percent < 20) fill_color = COLOR_ERROR;

        if (fill_w > 0) {
            spr_header_battery.fillRect(fill_x, fill_y, fill_w, fill_h, fill_color);
        }
        // A parte não preenchida já está com COLOR_ACCENT do fillSprite inicial
    }
}

// ---- Títulos e Rodapés ----
const char* ui_get_screen_title_string(ScreenState state) {
    if (state == ScreenState::SCREEN_TOTP_VIEW) {
        return (service_count > 0 && current_service_index >= 0 && current_service_index < service_count)
               ? services[current_service_index].name
               : getText(StringID::STR_TOTP_NO_SERVICE_TITLE);
    }
    StringID title_id;
    switch (state) {
        case ScreenState::SCREEN_STARTUP:                 title_id = StringID::STR_STARTUP_MSG; break; // Ou "" se não quiser título
        case ScreenState::SCREEN_LOCKED:                  title_id = StringID::STR_TITLE_LOCKED; break;
        case ScreenState::SCREEN_MENU_MAIN:               title_id = StringID::STR_TITLE_MAIN_MENU; break;
        case ScreenState::SCREEN_SERVICE_ADD_WAIT:        title_id = StringID::STR_TITLE_ADD_SERVICE; break;
        case ScreenState::SCREEN_SERVICE_ADD_CONFIRM:     title_id = StringID::STR_TITLE_CONFIRM_ADD; break;
        case ScreenState::SCREEN_TIME_EDIT:               title_id = StringID::STR_TITLE_ADJUST_TIME; break;
        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM:  title_id = StringID::STR_TITLE_CONFIRM_DELETE; break;
        case ScreenState::SCREEN_TIMEZONE_EDIT:           title_id = StringID::STR_TITLE_ADJUST_TIMEZONE; break;
        case ScreenState::SCREEN_LANGUAGE_SELECT:         title_id = StringID::STR_TITLE_SELECT_LANGUAGE; break;
        case ScreenState::SCREEN_READ_RFID:               title_id = StringID::STR_TITLE_READ_RFID; break;
        case ScreenState::SCREEN_MANAGE_CARDS:            title_id = StringID::STR_TITLE_MANAGE_CARDS; break;
        case ScreenState::SCREEN_ADD_CARD_WAIT:           title_id = StringID::STR_TITLE_ADD_CARD; break;
        case ScreenState::SCREEN_ADD_CARD_CONFIRM:        title_id = StringID::STR_TITLE_CONFIRM_ADD_CARD; break;
        case ScreenState::SCREEN_DELETE_CARD_CONFIRM:     title_id = StringID::STR_TITLE_CONFIRM_DELETE_CARD; break;
        case ScreenState::SCREEN_CONFIG_MENU:             title_id = StringID::STR_TITLE_CONFIG_MENU; break;
        case ScreenState::SCREEN_ADJUST_LOCK_TIMEOUT:     title_id = StringID::STR_TITLE_ADJUST_LOCK_TIMEOUT; break;
        case ScreenState::SCREEN_BRIGHTNESS_MENU:         title_id = StringID::STR_TITLE_BRIGHTNESS_MENU; break;
        case ScreenState::SCREEN_ADJUST_BRIGHTNESS_VALUE:
            // Title for adjust brightness value screen is dynamic
            switch(current_brightness_setting_to_adjust) {
                case BrightnessSettingToAdjust::USB:
                    return getText(StringID::STR_MENU_BRIGHTNESS_USB); // Or a more specific title like "Adjust USB Brightness"
                case BrightnessSettingToAdjust::BATTERY:
                    return getText(StringID::STR_MENU_BRIGHTNESS_BATTERY);
                case BrightnessSettingToAdjust::DIMMED:
                    return getText(StringID::STR_MENU_BRIGHTNESS_DIMMED);
                default: return "Adjust Brightness";
            }
            break; // Should not be reached due to return in cases
        case ScreenState::SCREEN_SETUP_FIRST_CARD:        title_id = StringID::STR_TITLE_SETUP_FIRST_CARD; break;
        case ScreenState::SCREEN_MESSAGE:                 return ""; // Mensagem não tem título padrão
        default: return "Error";
    }
    return getText(title_id);
}

const char* ui_get_footer_string(ScreenState state) {
    StringID footer_id;
    switch (state) {
        case ScreenState::SCREEN_TOTP_VIEW:
        case ScreenState::SCREEN_MENU_MAIN:
        case ScreenState::SCREEN_SERVICE_ADD_WAIT:
        case ScreenState::SCREEN_READ_RFID:
            footer_id = StringID::STR_FOOTER_GENERIC_NAV; break;
        case ScreenState::SCREEN_SERVICE_ADD_CONFIRM:
        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM:
            footer_id = StringID::STR_FOOTER_CONFIRM_NAV; break;
        case ScreenState::SCREEN_TIME_EDIT:
            footer_id = StringID::STR_FOOTER_TIME_EDIT_NAV; break;
        case ScreenState::SCREEN_TIMEZONE_EDIT:
            footer_id = StringID::STR_FOOTER_TIMEZONE_NAV; break;
        case ScreenState::SCREEN_LANGUAGE_SELECT:
            footer_id = StringID::STR_FOOTER_LANG_NAV; break;
        case ScreenState::SCREEN_MANAGE_CARDS:
            footer_id = StringID::STR_FOOTER_MANAGE_CARDS_NAV; break;
        case ScreenState::SCREEN_ADD_CARD_WAIT:
            footer_id = StringID::STR_FOOTER_ADD_CARD_NAV; break;
        case ScreenState::SCREEN_ADD_CARD_CONFIRM:
        case ScreenState::SCREEN_DELETE_CARD_CONFIRM:
            footer_id = StringID::STR_FOOTER_CONFIRM_CARD_NAV; break;
        case ScreenState::SCREEN_CONFIG_MENU:
            footer_id = StringID::STR_FOOTER_CONFIG_SUBMENU_NAV; break;
        case ScreenState::SCREEN_ADJUST_LOCK_TIMEOUT:
        case ScreenState::SCREEN_ADJUST_BRIGHTNESS_VALUE:
            footer_id = StringID::STR_FOOTER_ADJUST_VALUE_NAV; break;
        case ScreenState::SCREEN_BRIGHTNESS_MENU:         // Uses same footer as config submenu
            footer_id = StringID::STR_FOOTER_CONFIG_SUBMENU_NAV; break;
        case ScreenState::SCREEN_SETUP_FIRST_CARD:        // No footer for this screen
        case ScreenState::SCREEN_STARTUP:
        case ScreenState::SCREEN_MESSAGE:
        case ScreenState::SCREEN_LOCKED: // No standard footer for locked screen
        default:
            return nullptr; // Sem rodapé padrão para estas telas
    }
    return getText(footer_id);
}

// ---- Mensagens Temporárias ----
void ui_queue_message(const char* msg, uint16_t color, uint32_t duration, ScreenState next_screen) {
    strncpy(message_buffer, msg, sizeof(message_buffer) - 1);
    message_buffer[sizeof(message_buffer) - 1] = '\0';
    message_color = color;
    message_duration_ms = duration;
    message_next_screen = next_screen;

    // Muda para a tela de mensagem. O loop principal e ui_draw_screen cuidarão do resto.
    ui_change_screen(ScreenState::SCREEN_MESSAGE, true);
}

void ui_queue_message_fmt(StringID str_id, uint16_t color, uint32_t duration, ScreenState next_screen, ...) {
    char formatted_msg[sizeof(message_buffer)];
    const char* fmt_str = getText(str_id);

    va_list args;
    va_start(args, next_screen); // O último argumento fixo antes de '...'
    vsnprintf(formatted_msg, sizeof(formatted_msg), fmt_str, args);
    va_end(args);

    ui_queue_message(formatted_msg, color, duration, next_screen);
}


// ---- Implementações de Desenho de Conteúdo Específico de Cada Tela ----

static void ui_draw_content_startup(bool full_redraw) {
    if (full_redraw) {
        tft.fillRect(0, UI_CONTENT_Y_START, tft.width(), tft.height() - UI_CONTENT_Y_START, COLOR_BG);
        tft.setTextColor(COLOR_FG, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        tft.drawString(getText(StringID::STR_STARTUP_MSG), tft.width() / 2, tft.height() / 2);
        tft.setTextDatum(TL_DATUM); // Reset datum
        tft.setTextSize(1);
    }
}

static void ui_draw_content_totp_view(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int content_h = tft.height() - content_y - (tft.fontHeight(1) + UI_PADDING); // Desconta footer

    if (full_redraw) {
        tft.fillRect(0, content_y, tft.width(), tft.height() - content_y, COLOR_BG); // Limpa área de conteúdo e footer

        // Desenha contorno da barra de progresso (o sprite preenche o interior)
        tft.drawRoundRect(progress_bar_x - 2,
                          content_y + (content_h / 2) + spr_totp_code.height() / 2 + 10 - 2, // Posição Y da barra
                          progress_bar_width + 4, UI_PROGRESS_BAR_HEIGHT + 4, 4, COLOR_FG);
    }

    // Atualiza e desenha os sprites do código TOTP e da barra de progresso
    ui_update_totp_code_sprite();
    ui_update_progress_bar_sprite(time_now_utc()); // Passa o tempo UTC atual

    // Calcula posições para centralizar o código TOTP e posicionar a barra abaixo
    int code_sprite_y = content_y + (content_h / 2) - spr_totp_code.height() / 2 - (UI_PROGRESS_BAR_HEIGHT / 2) - 5;
    int progress_bar_sprite_y = code_sprite_y + spr_totp_code.height() + 10;

    spr_totp_code.pushSprite((tft.width() - spr_totp_code.width()) / 2, code_sprite_y);
    spr_progress_bar.pushSprite(progress_bar_x, progress_bar_sprite_y);
}

// --- Rewritten Main Menu Drawing Function ---
static void ui_draw_content_menu_main(bool full_redraw) {
    int content_y_start = UI_CONTENT_Y_START;
    int content_width = screen_width;
    int available_height = screen_height - content_y_start - (tft.fontHeight(1) + UI_PADDING) - UI_PADDING;

    if (full_redraw) {
        tft.fillRect(0, content_y_start, content_width, screen_height - content_y_start, COLOR_BG);
    }

    const int cols = 3;
    const int rows = 2;
    const int total_buttons = NUM_MENU_OPTIONS;

    int button_grid_margin_x = UI_PADDING * 3;
    int button_grid_margin_y = UI_PADDING * 2;
    int button_spacing_x = UI_PADDING * 2;
    int button_spacing_y = UI_PADDING * 2;

    int grid_area_width = content_width - 2 * button_grid_margin_x;
    int grid_area_height = available_height - 2 * button_grid_margin_y;

    int button_w = (grid_area_width - (cols - 1) * button_spacing_x) / cols;
    int button_h = (grid_area_height - (rows - 1) * button_spacing_y) / rows;

    // Map menuOptionIDs to icons, their widths, and heights
    // Order must match menuOptionIDs in globals.cpp:
    // STR_MENU_VIEW_CODES, STR_ACTION_CONFIG, STR_MENU_ADJUST_TIMEZONE,
    // STR_MENU_READ_RFID_OPTION, STR_MENU_SELECT_LANGUAGE, STR_MENU_ADJUST_TIME
    const unsigned char* icons_data[total_buttons] = {
        codes_menu_icon, image_config_icon_bits, image_zones_icon_bits, // Use new codes_menu_icon for the first item
        image_rfid_icon_bits, image_lang_icon_bits, image_date_icon_bits
    };
    int icon_widths[total_buttons] = {
        ICON_NEW_CODES_WIDTH, ICON_DEFAULT_WIDTH, ICON_DEFAULT_WIDTH, // Use new codes icon width
        ICON_DEFAULT_WIDTH, ICON_DEFAULT_WIDTH, ICON_DEFAULT_WIDTH
    };
    int icon_heights[total_buttons] = {
        ICON_NEW_CODES_HEIGHT, ICON_DEFAULT_HEIGHT, ICON_DEFAULT_HEIGHT, // Use new codes icon height
        ICON_DEFAULT_HEIGHT, ICON_DEFAULT_HEIGHT, ICON_DEFAULT_HEIGHT
    };

    StringID labels[total_buttons] = {
        StringID::STR_LABEL_CODES, StringID::STR_LABEL_CONFIG, StringID::STR_LABEL_ZONE,
        StringID::STR_LABEL_RFID, StringID::STR_LABEL_LANG, StringID::STR_LABEL_DATE
    };

    for (int i = 0; i < total_buttons; ++i) {
        int r = i / cols;
        int c = i % cols;

        int button_x = button_grid_margin_x + c * (button_w + button_spacing_x);
        int button_y = content_y_start + button_grid_margin_y + r * (button_h + button_spacing_y);

        bool is_selected = (i == current_menu_index);
        uint16_t fg_color = is_selected ? COLOR_HIGHLIGHT_FG : COLOR_FG;
        uint16_t bg_color = is_selected ? COLOR_HIGHLIGHT_BG : COLOR_BAR_BG;

        if (is_selected) {
            tft.fillRoundRect(button_x, button_y, button_w, button_h, 5, COLOR_HIGHLIGHT_BG);
            tft.drawRoundRect(button_x, button_y, button_w, button_h, 5, COLOR_FG); 
        } else {
            tft.fillRoundRect(button_x, button_y, button_w, button_h, 5, COLOR_BAR_BG);
            tft.drawRoundRect(button_x, button_y, button_w, button_h, 5, COLOR_DIM_TEXT);
        }

        const unsigned char* progmem_icon_ptr = icons_data[i]; // Pointer to PROGMEM data
        int current_icon_w = icon_widths[i];
        int current_icon_h = icon_heights[i];
        int bytes_per_row = (current_icon_w + 7) / 8;
        int total_icon_bytes = bytes_per_row * current_icon_h;

        // Determine max possible bytes for any icon to size ram_icon_data buffer.
        // Pager: 48x32 -> (48/8)*32 = 6*32 = 192 bytes.
        // Old Codes: 24x15 -> (24/8)*15 = 3*15 = 45 bytes.
        // Default: 16x16 -> (16/8)*16 = 2*16 = 32 bytes.
        // Max is 192 bytes.
        uint8_t ram_icon_data[192]; 

        // 1. Copy from PROGMEM to RAM
        for (int k = 0; k < total_icon_bytes; ++k) {
            ram_icon_data[k] = pgm_read_byte(&progmem_icon_ptr[k]);
        }

        // 2. Reverse bits within each byte (if your rendering requires it, as determined before)
        for (int k = 0; k < total_icon_bytes; ++k) {
            ram_icon_data[k] = reverse_byte_bits(ram_icon_data[k]);
        }

        // Calculate icon position (centered in top 2/3 of button)
        int icon_area_h = button_h * 2 / 3;
        int icon_x_pos = button_x + (button_w - current_icon_w) / 2;
        int icon_y_pos = button_y + (icon_area_h - current_icon_h) / 2 + UI_PADDING / 2;

        tft.drawXBitmap(icon_x_pos, icon_y_pos, ram_icon_data, current_icon_w, current_icon_h, fg_color);

        // Draw Label (in bottom 1/3 of button)
        int label_area_y_start = button_y + icon_area_h;
        int label_area_h = button_h - icon_area_h;
        tft.setTextColor(fg_color, bg_color); // Set text color with button background
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(1);
        tft.drawString(getText(labels[i]), button_x + button_w / 2, label_area_y_start + label_area_h / 2 - UI_PADDING / 4);
    }

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(1);
}

static void ui_draw_content_service_add_wait(bool full_redraw) {
    if (full_redraw) {
        int content_y = UI_CONTENT_Y_START;
        int content_h = tft.height() - content_y - (tft.fontHeight(1) + UI_PADDING);
        tft.fillRect(0, content_y, tft.width(), content_h, COLOR_BG); // Limpa área de conteúdo

        tft.setTextColor(COLOR_FG, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        int center_y_area = content_y + content_h / 2;

        tft.drawString(getText(StringID::STR_AWAITING_JSON), tft.width() / 2, center_y_area - 25);
        tft.drawString(getText(StringID::STR_VIA_SERIAL), tft.width() / 2, center_y_area);

        tft.setTextSize(1);
        tft.setTextColor(COLOR_DIM_TEXT, COLOR_BG);
        tft.drawString(getText(StringID::STR_EXAMPLE_JSON_SERVICE), tft.width() / 2, center_y_area + 30);

        tft.setTextDatum(TL_DATUM); // Reset datum
        tft.setTextSize(1);
        tft.setTextColor(COLOR_FG, COLOR_BG);
    }
}

static void ui_draw_content_service_add_confirm(bool full_redraw) {
    if (full_redraw) {
        int content_y = UI_CONTENT_Y_START;
        int content_h = tft.height() - content_y - (tft.fontHeight(1) + UI_PADDING);
        tft.fillRect(0, content_y, tft.width(), content_h, COLOR_BG);

        tft.setTextDatum(MC_DATUM);
        int center_y_area = content_y + content_h / 2;

        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.setTextSize(2);
        tft.drawString(getText(StringID::STR_CONFIRM_ADD_PROMPT), tft.width() / 2, center_y_area - 20);

        tft.setTextColor(COLOR_FG, COLOR_BG);
        tft.setTextSize(2); // Ou 1 se o nome for muito grande
        // Truncar nome do serviço se necessário para caber
        char display_name[MAX_SERVICE_NAME_LEN + 4];
        if (strlen(temp_service_name) > 15) { // Exemplo de limite
            snprintf(display_name, sizeof(display_name), "%.12s...", temp_service_name);
        } else {
            strncpy(display_name, temp_service_name, sizeof(display_name)-1);
            display_name[sizeof(display_name)-1] = '\0';
        }
        tft.drawString(display_name, tft.width() / 2, center_y_area + 20);

        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_FG, COLOR_BG);
    }
}

// Dentro de ui_manager.cpp -> ui_draw_content_time_edit
static void ui_draw_content_time_edit(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int footer_h_approx = tft.fontHeight(1) + UI_PADDING;
    int content_h = screen_height - content_y - footer_h_approx;
    int center_y_area = content_y + content_h / 2;

    // --- FORÇAR LIMPEZA COMPLETA DA ÁREA DE CONTEÚDO ---
    tft.fillRect(0, content_y, screen_width, content_h, COLOR_BG);
    // --- FIM DA LIMPEZA FORÇADA ---

    // Desenha os elementos estáticos (hints de JSON, fuso) sempre
    tft.setTextColor(COLOR_DIM_TEXT, COLOR_BG);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    char tz_info_buf[40];
    snprintf(tz_info_buf, sizeof(tz_info_buf), getText(StringID::STR_TIME_EDIT_INFO_FMT), gmt_offset);
    int hints_y_start = center_y_area + tft.fontHeight(3) / 2 + 20; // Abaixo de onde a hora estará
    tft.drawString(tz_info_buf, screen_width / 2, hints_y_start);
    tft.drawString(getText(StringID::STR_TIME_EDIT_JSON_HINT), screen_width / 2, hints_y_start + 15);
    tft.drawString(getText(StringID::STR_EXAMPLE_JSON_TIME), screen_width / 2, hints_y_start + 30);

    // --- Desenha a hora e o marcador ---
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(3); // Tamanho grande para a hora

    char time_str_buf[12];
    snprintf(time_str_buf, sizeof(time_str_buf), "%02d:%02d:%02d", edit_hour, edit_minute, edit_second);

    // Desenha a string de hora centralizada (confiando no setTextDatum)
    tft.drawString(time_str_buf, screen_width / 2, center_y_area);

    // --- USA LARGURAS ESTIMADAS PARA O MARCADOR ---
    const int field_width_est = 48;      // <<< ESTIMATIVA para "00" (AJUSTE SE NECESSÁRIO)
    const int separator_width_est = 12;  // <<< ESTIMATIVA para ":" (AJUSTE SE NECESSÁRIO)
    const int total_time_width_est = 3 * field_width_est + 2 * separator_width_est; // Largura total estimada
    const int time_start_x_est = (screen_width - total_time_width_est) / 2; // X inicial baseado em estimativas
    const int font_height_est = 24; // <<< ESTIMATIVA para altura da fonte tamanho 3 (AJUSTE SE NECESSÁRIO)

    // --- LOGS DE DEPURAÇÃO (v5 - Estimated Height) ---
    Serial.printf("Time Edit Draw Debug (v5 - Estimated Height):\n");
    Serial.printf("  screen_width: %d, center_y_area: %d\n", screen_width, center_y_area);
    Serial.printf("  Estimates: FieldW=%d, SepW=%d, TotalW=%d, StartX=%d, FontH=%d\n",
                  field_width_est, separator_width_est, total_time_width_est, time_start_x_est, font_height_est);
    Serial.printf("  edit_time_field: %d\n", edit_time_field);
    // --- FIM DOS LOGS ---

    // Calcula X do marcador baseado no X inicial ESTIMADO e larguras ESTIMADAS
    int marker_x = time_start_x_est; // Default para hora (campo 0)
    if (edit_time_field == 1) { // Minuto
        marker_x = time_start_x_est + field_width_est + separator_width_est;
    } else if (edit_time_field == 2) { // Segundo
        marker_x = time_start_x_est + 2 * (field_width_est + separator_width_est);
    }

    // Calcula marker_y usando a altura da fonte ESTIMADA
    int marker_y = center_y_area + font_height_est / 2 - 2; // Abaixo do texto (USA font_height_est)
    int marker_h = 4; // Altura do marcador

    Serial.printf("  Calculated marker_x: %d, marker_y: %d\n", marker_x, marker_y);

    // Verifica se as coordenadas são válidas (usa field_width_est para largura do marcador)
    if (marker_x >= 0 && marker_x + field_width_est <= screen_width && marker_y >= 0 && marker_y + marker_h <= screen_height) {
        tft.fillRect(marker_x, marker_y, field_width_est, marker_h, COLOR_ACCENT); // Desenha o marcador
        Serial.println("  Marker drawn.");
   } else {
        Serial.printf("  [WARN] Marker coordinates out of screen!\n");
   }

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_FG, COLOR_BG);
}

static void ui_draw_content_service_delete_confirm(bool full_redraw) {
    if (full_redraw) {
        int content_y = UI_CONTENT_Y_START;
        int content_h = tft.height() - content_y - (tft.fontHeight(1) + UI_PADDING);
        tft.fillRect(0, content_y, tft.width(), content_h, COLOR_BG);

        tft.setTextDatum(MC_DATUM);
        int center_y_area = content_y + content_h / 2;

        tft.setTextColor(COLOR_ERROR, COLOR_BG); // Cor de perigo
        tft.setTextSize(2);

        char prompt_buf[60];
        const char* service_name_to_delete = (service_count > 0 && current_service_index < service_count) ? services[current_service_index].name : "???";
        snprintf(prompt_buf, sizeof(prompt_buf), getText(StringID::STR_CONFIRM_DELETE_PROMPT_FMT), service_name_to_delete);
        // Quebrar manualmente se for muito longo, ou usar drawStringWordWrap
        // Simples:
        char* line1 = strtok(prompt_buf, "\n"); // Se o template já tiver \n
        char* line2 = strtok(NULL, "\n");
        if (line2) {
            tft.drawString(line1, tft.width()/2, center_y_area - 15);
            tft.drawString(line2, tft.width()/2, center_y_area + 15);
        } else {
            tft.drawString(prompt_buf, tft.width() / 2, center_y_area);
        }


        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_FG, COLOR_BG);
    }
}

static void ui_draw_content_timezone_edit(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int content_h = tft.height() - content_y - (tft.fontHeight(1) + UI_PADDING);
    int center_y_area = content_y + content_h / 2;

    if (full_redraw) {
        tft.fillRect(0, content_y, tft.width(), content_h, COLOR_BG); // Limpa área de conteúdo
    } else {
        // Limpa apenas a área do texto do fuso horário
        tft.fillRect(0, center_y_area - tft.fontHeight(3)/2 - 5, tft.width(), tft.fontHeight(3) + 10, COLOR_BG);
    }

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(3);

    char tz_str[15];
    snprintf(tz_str, sizeof(tz_str), getText(StringID::STR_TIMEZONE_LABEL), temp_gmt_offset);
    tft.drawString(tz_str, tft.width() / 2, center_y_area);

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
}

static void ui_draw_content_language_select(bool full_redraw) {
    int item_y_start = UI_MENU_START_Y;
    int item_total_h = UI_MENU_ITEM_HEIGHT + UI_MENU_ITEM_SPACING;
    int lang_menu_area_h = static_cast<int>(Language::NUM_LANGUAGES) * item_total_h - UI_MENU_ITEM_SPACING;

    if (full_redraw) {
        tft.fillRect(0, UI_CONTENT_Y_START, tft.width(), tft.height() - UI_CONTENT_Y_START, COLOR_BG);
    } else {
        // Limpa apenas a área dos itens do menu de idiomas
        tft.fillRect(UI_PADDING, item_y_start, tft.width() - 2 * UI_PADDING, lang_menu_area_h, COLOR_BG);
    }

    tft.setTextSize(2);
    tft.setTextDatum(ML_DATUM);

    const StringID lang_names[] = {StringID::STR_LANG_PORTUGUESE, StringID::STR_LANG_ENGLISH}; // Mapeia índice para StringID

    for (int i = 0; i < static_cast<int>(Language::NUM_LANGUAGES); ++i) {
        int item_render_y = item_y_start + i * item_total_h;
        bool is_selected_in_menu = (i == current_language_menu_index);
        bool is_currently_active = (static_cast<Language>(i) == current_language);

        uint16_t fg_c = is_selected_in_menu ? COLOR_HIGHLIGHT_FG : COLOR_FG;
        uint16_t bg_c = is_selected_in_menu ? COLOR_HIGHLIGHT_BG : COLOR_BG;

        tft.fillRect(UI_PADDING, item_render_y, tft.width() - 2 * UI_PADDING, UI_MENU_ITEM_HEIGHT, bg_c);
        tft.setTextColor(fg_c, bg_c);

        String lang_text = getText(lang_names[i]);
        if (is_currently_active && !is_selected_in_menu) {
            lang_text += " *"; // Adiciona marcador para idioma ativo se não for o selecionado no menu
        } else if (is_currently_active && is_selected_in_menu) {
             // Poderia adicionar um marcador diferente ou nada
        }

        tft.drawString(lang_text, UI_PADDING * 3, item_render_y + UI_MENU_ITEM_HEIGHT / 2);
    }

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(1);
}

static void ui_draw_content_message(bool full_redraw) {
    // A tela de mensagem é especial: ocupa a tela inteira.
    // O timer para voltar à tela anterior é gerenciado no loop principal ou input_handler.
    if (full_redraw) {
        tft.fillScreen(message_color == COLOR_ERROR ? COLOR_ERROR : (message_color == COLOR_SUCCESS ? TFT_DARKGREEN : COLOR_BG) ); // Fundo diferente para erro/sucesso
        tft.setTextColor(TFT_WHITE); // Texto branco para bom contraste
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);

        // Suporte simples para quebra de linha com '\n'
        char temp_msg_buf[sizeof(message_buffer)];
        strncpy(temp_msg_buf, message_buffer, sizeof(temp_msg_buf)-1);
        temp_msg_buf[sizeof(temp_msg_buf)-1] = '\0';

        char* line1 = strtok(temp_msg_buf, "\n");
        char* line2 = strtok(NULL, "\n");

        int y_pos = tft.height() / 2;
        int line_h = tft.fontHeight(2) + 5;

        if (line1 && line2) {
            tft.drawString(line1, tft.width() / 2, y_pos - line_h / 2);
            tft.drawString(line2, tft.width() / 2, y_pos + line_h / 2);
        } else if (line1) {
            tft.drawString(line1, tft.width() / 2, y_pos);
        }

        tft.setTextDatum(TL_DATUM); // Reset datum
        tft.setTextColor(COLOR_FG, COLOR_BG); // Reset colors
        tft.setTextSize(1);
    }
    // Nenhuma atualização parcial para tela de mensagem, ela é estática até o tempo acabar.
}

static void ui_draw_content_read_rfid(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int content_h = tft.height() - content_y - (tft.fontHeight(1) + UI_PADDING); // Desconta footer
    int center_y_area = content_y + content_h / 2;

    if (full_redraw) {
        tft.fillRect(0, content_y, tft.width(), content_h, COLOR_BG); // Limpa área de conteúdo
    } else {
        // Para atualizações parciais (ex: mostrar ID do cartão), limpa só a área do ID
        tft.fillRect(0, center_y_area + 15, tft.width(), 30, COLOR_BG);
    }

    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);

    tft.drawString(getText(StringID::STR_RFID_PROMPT), tft.width() / 2, center_y_area - 15);

    if (strlen(card_id) > 0) {
        char display_text[40]; // "ID Cartao: XX XX XX XX"
        snprintf(display_text, sizeof(display_text), getText(StringID::STR_RFID_ID_PREFIX), card_id);
        tft.setTextSize(1); // ID menor
        tft.drawString(display_text, tft.width() / 2, center_y_area + 25);
    }

    tft.setTextDatum(TL_DATUM); // Reset datum
    tft.setTextSize(1);
}

// ---- New Lock Screen Drawing Function ----
static void ui_draw_content_locked(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int content_h = screen_height - content_y; // Lock screen uses full content area below header
    int center_x = screen_width / 2;
    int center_y_area = content_y + content_h / 2;

    if (full_redraw) {
        tft.fillRect(0, content_y, screen_width, content_h, COLOR_BG);
    }

    // --- Draw Padlock Icon ---
    uint8_t ram_icon_data[ICON_PADLOCK_WIDTH * ICON_PADLOCK_HEIGHT / 8];
    for (int k = 0; k < sizeof(ram_icon_data); ++k) {
        ram_icon_data[k] = reverse_byte_bits(pgm_read_byte(&image_padlock_icon_bits[k]));
    }
    int icon_x_pos = center_x - ICON_PADLOCK_WIDTH / 2;
    int icon_y_pos = center_y_area - ICON_PADLOCK_HEIGHT - 10; // Position above text
    tft.drawXBitmap(icon_x_pos, icon_y_pos, ram_icon_data, ICON_PADLOCK_WIDTH, ICON_PADLOCK_HEIGHT, COLOR_FG);

    // --- Draw Prompt Text ---
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2); // Larger text for prompt
    
    // Handle multi-line prompt from i18n
    char prompt_buf[100];
    strncpy(prompt_buf, getText(StringID::STR_LOCKED_PROMPT), sizeof(prompt_buf) -1);
    prompt_buf[sizeof(prompt_buf)-1] = '\0';

    char* line1 = strtok(prompt_buf, "\n");
    char* line2 = strtok(NULL, "\n");
    int text_y_pos = center_y_area + 15; // Position below icon

    if (line1 && line2) {
        tft.drawString(line1, center_x, text_y_pos);
        tft.drawString(line2, center_x, text_y_pos + tft.fontHeight(2) + 2);
    } else if (line1) {
        tft.drawString(line1, center_x, text_y_pos);
    }

    tft.setTextDatum(TL_DATUM); // Reset datum
    tft.setTextSize(1);
}

// ---- New Card Management Screen Drawing Functions ----
static void ui_draw_content_manage_cards(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int item_h = UI_MENU_ITEM_HEIGHT;
    int item_spacing = UI_MENU_ITEM_SPACING;
    int total_item_h = item_h + item_spacing;
    int footer_h = tft.fontHeight(1) + UI_PADDING * 2; // Approximate height for footer
    int available_draw_height = screen_height - content_y - footer_h;
    int max_visible_items = available_draw_height / total_item_h;
    if (max_visible_items <= 0) max_visible_items = 1;

    if (full_redraw) {
        tft.fillRect(0, content_y, screen_width, screen_height - content_y - footer_h, COLOR_BG); // Clear content area above footer
    }

    if (authorized_card_count == 0) {
        tft.setTextColor(COLOR_DIM_TEXT, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        char* line1 = strtok((char*)getText(StringID::STR_CARD_LIST_EMPTY), "\n");
        char* line2 = strtok(NULL, "\n");
        int text_center_y = content_y + available_draw_height / 2;
        if (line1 && line2) {
            tft.drawString(line1, screen_width / 2, text_center_y - tft.fontHeight(2)/2 - 2);
            tft.drawString(line2, screen_width / 2, text_center_y + tft.fontHeight(2)/2 + 2);
        } else if (line1) {
            tft.drawString(line1, screen_width / 2, text_center_y);
        }
    } else {
        tft.setTextSize(1); // Smaller text for card UIDs
        int draw_y = content_y + UI_PADDING;
        
        // Ensure manage_cards_top_visible_index is valid
        if (manage_cards_top_visible_index < 0) manage_cards_top_visible_index = 0;
        if (manage_cards_top_visible_index > authorized_card_count - max_visible_items && authorized_card_count > max_visible_items) {
            manage_cards_top_visible_index = authorized_card_count - max_visible_items;
        } else if (authorized_card_count <= max_visible_items) {
            manage_cards_top_visible_index = 0;
        }

        for (int i = 0; i < max_visible_items; ++i) {
            int actual_card_index = manage_cards_top_visible_index + i;
            if (actual_card_index >= authorized_card_count) break; // Don't draw beyond available cards

            bool is_selected = (actual_card_index == current_manage_card_index);
            uint16_t item_fg_color = is_selected ? COLOR_HIGHLIGHT_FG : COLOR_FG;
            uint16_t item_bg_color = is_selected ? COLOR_HIGHLIGHT_BG : COLOR_BG;

            int current_item_draw_y = draw_y + i * total_item_h;

            // Clear item area before drawing (especially if not full_redraw)
            if (!full_redraw) {
                 tft.fillRect(UI_PADDING, current_item_draw_y, screen_width - 2 * UI_PADDING - (authorized_card_count > max_visible_items ? 10 : 0), item_h, COLOR_BG);
            }
            if (is_selected) { // Draw highlight for selected item
                 tft.fillRect(UI_PADDING, current_item_draw_y, screen_width - 2 * UI_PADDING - (authorized_card_count > max_visible_items ? 10 : 0), item_h, item_bg_color);
            }

            tft.setTextColor(item_fg_color, item_bg_color);
            tft.setTextDatum(ML_DATUM);
            // Truncate card ID if too long for the display area
            char display_card_id[MAX_CARD_ID_LEN + 4]; // +4 for "..."
            int max_text_width = screen_width - (UI_PADDING * 6) - (authorized_card_count > max_visible_items ? 10 : 0);
            if (tft.textWidth(authorized_card_ids[actual_card_index]) > max_text_width) {
                int len = strlen(authorized_card_ids[actual_card_index]);
                int cutoff = len;
                while(cutoff > 0 && tft.textWidth(String(authorized_card_ids[actual_card_index]).substring(0, cutoff) + "...") > max_text_width) {
                    cutoff--;
                }
                snprintf(display_card_id, sizeof(display_card_id), "%.*s...", cutoff, authorized_card_ids[actual_card_index]);
            } else {
                strncpy(display_card_id, authorized_card_ids[actual_card_index], sizeof(display_card_id) -1);
                display_card_id[sizeof(display_card_id)-1] = '\0';
            }
            tft.drawString(display_card_id, UI_PADDING * 3, current_item_draw_y + item_h / 2);
        }

        // Draw scrollbar if needed
        if (authorized_card_count > max_visible_items) {
            int scrollbar_x = screen_width - UI_PADDING - 6;
            int scrollbar_w = 4;
            int scrollbar_track_h = max_visible_items * total_item_h - item_spacing;
            int scrollbar_track_y = content_y + UI_PADDING;

            tft.fillRect(scrollbar_x, scrollbar_track_y, scrollbar_w, scrollbar_track_h, COLOR_DIM_TEXT); // Scrollbar track

            int thumb_h = max(10, scrollbar_track_h * max_visible_items / authorized_card_count);
            int thumb_max_y_offset = scrollbar_track_h - thumb_h;
            int thumb_y_offset = 0;
            if (authorized_card_count - max_visible_items > 0) {
                thumb_y_offset = round((float)thumb_max_y_offset * manage_cards_top_visible_index / (authorized_card_count - max_visible_items));
            }
            thumb_y_offset = constrain(thumb_y_offset, 0, thumb_max_y_offset);
            tft.fillRect(scrollbar_x, scrollbar_track_y + thumb_y_offset, scrollbar_w, thumb_h, COLOR_ACCENT); // Scrollbar thumb
        }
    }
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(1);
}

static void ui_draw_content_add_card_wait(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int content_h = screen_height - content_y - (tft.fontHeight(1) + UI_PADDING);
    if (full_redraw) {
        tft.fillRect(0, content_y, screen_width, content_h, COLOR_BG);
        tft.setTextColor(COLOR_FG, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        char* line1 = strtok((char*)getText(StringID::STR_ADD_CARD_PROMPT), "\n");
        char* line2 = strtok(NULL, "\n");
        if (line1 && line2) {
            tft.drawString(line1, screen_width / 2, content_y + content_h / 2 - tft.fontHeight(2)/2 - 2);
            tft.drawString(line2, screen_width / 2, content_y + content_h / 2 + tft.fontHeight(2)/2 + 2);
        } else if (line1) {
            tft.drawString(line1, screen_width / 2, content_y + content_h / 2);
        }
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
    }
}

static void ui_draw_content_add_card_confirm(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int content_h = screen_height - content_y - (tft.fontHeight(1) + UI_PADDING);
    if (full_redraw) {
        tft.fillRect(0, content_y, screen_width, content_h, COLOR_BG);
        tft.setTextColor(COLOR_ACCENT, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);

        char confirm_prompt[100];
        snprintf(confirm_prompt, sizeof(confirm_prompt), getText(StringID::STR_CONFIRM_ADD_CARD_PROMPT_FMT), temp_card_id);
        
        char* line1 = strtok(confirm_prompt, "\n");
        char* line2 = strtok(NULL, "\n");
        char* line3 = strtok(NULL, "\n"); // For potentially long UIDs split over two lines by the format string

        int base_y = content_y + content_h / 2;
        int line_height = tft.fontHeight(2) + 2;

        if (line1 && line2 && line3) {
             tft.drawString(line1, screen_width / 2, base_y - line_height);
             tft.drawString(line2, screen_width / 2, base_y);
             tft.drawString(line3, screen_width / 2, base_y + line_height);
        } else if (line1 && line2) {
            tft.drawString(line1, screen_width / 2, base_y - line_height / 2);
            tft.drawString(line2, screen_width / 2, base_y + line_height / 2);
        } else if (line1) {
            tft.drawString(line1, screen_width / 2, base_y);
        }

        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(COLOR_FG, COLOR_BG);
        tft.setTextSize(1);
    }
}

static void ui_draw_content_delete_card_confirm(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int content_h = screen_height - content_y - (tft.fontHeight(1) + UI_PADDING);
    if (full_redraw) {
        tft.fillRect(0, content_y, screen_width, content_h, COLOR_BG);
        tft.setTextColor(COLOR_ERROR, COLOR_BG);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);

        char confirm_prompt[100];
        // Ensure current_manage_card_index is valid before accessing authorized_card_ids
        const char* card_to_delete_id = "????";
        if (current_manage_card_index >= 0 && current_manage_card_index < authorized_card_count) {
            card_to_delete_id = authorized_card_ids[current_manage_card_index];
        }
        snprintf(confirm_prompt, sizeof(confirm_prompt), getText(StringID::STR_CONFIRM_DELETE_CARD_PROMPT_FMT), card_to_delete_id);

        char* line1 = strtok(confirm_prompt, "\n");
        char* line2 = strtok(NULL, "\n");
        char* line3 = strtok(NULL, "\n");

        int base_y = content_y + content_h / 2;
        int line_height = tft.fontHeight(2) + 2;

        if (line1 && line2 && line3) {
             tft.drawString(line1, screen_width / 2, base_y - line_height);
             tft.drawString(line2, screen_width / 2, base_y);
             tft.drawString(line3, screen_width / 2, base_y + line_height);
        } else if (line1 && line2) {
            tft.drawString(line1, screen_width / 2, base_y - line_height / 2);
            tft.drawString(line2, screen_width / 2, base_y + line_height / 2);
        } else if (line1) {
            tft.drawString(line1, screen_width / 2, base_y);
        }

        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(COLOR_FG, COLOR_BG);
        tft.setTextSize(1);
    }
}

// ---- New Config Submenu Drawing Function ----
static void ui_draw_content_config_menu(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int item_h = UI_MENU_ITEM_HEIGHT;
    int item_spacing = UI_MENU_ITEM_SPACING;
    int total_item_h = item_h + item_spacing;
    // For simplicity, this menu won't scroll for now, similar to language select.
    // It will display all NUM_CONFIG_MENU_OPTIONS.

    if (full_redraw) {
        tft.fillRect(0, content_y, screen_width, screen_height - content_y, COLOR_BG);
    }

    tft.setTextSize(2); // Standard menu item text size
    int draw_y = content_y + UI_PADDING + item_spacing; // Start a bit lower for visual balance

    for (int i = 0; i < NUM_CONFIG_MENU_OPTIONS; ++i) {
        bool is_selected = (i == current_config_menu_index);
        uint16_t item_fg_color = is_selected ? COLOR_HIGHLIGHT_FG : COLOR_FG;
        uint16_t item_bg_color = is_selected ? COLOR_HIGHLIGHT_BG : COLOR_BG;

        // Draw background for selection highlight
        if (is_selected) {
            tft.fillRect(UI_PADDING, draw_y, screen_width - 2 * UI_PADDING, item_h, item_bg_color);
        }

        tft.setTextColor(item_fg_color, item_bg_color); // Set text color with appropriate background
        tft.setTextDatum(ML_DATUM); // Middle-Left for consistent text alignment
        tft.drawString(getText(configMenuOptionIDs[i]), UI_PADDING * 3, draw_y + item_h / 2);
        
        draw_y += total_item_h;
    }

    tft.setTextDatum(TL_DATUM); // Reset datum
    tft.setTextColor(COLOR_FG, COLOR_BG); // Reset colors
    tft.setTextSize(1); // Reset text size
}

// ---- New Adjust Lock Timeout Screen Drawing Function ----
static void ui_draw_content_adjust_lock_timeout(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int content_h = screen_height - content_y - (tft.fontHeight(1) + UI_PADDING); // Desconta footer
    int center_y_area = content_y + content_h / 2;

    if (full_redraw) {
        tft.fillRect(0, content_y, screen_width, content_h, COLOR_BG); // Limpa área de conteúdo
    } else {
        // Limpa apenas a área do valor do timeout para atualizações parciais
        tft.fillRect(0, center_y_area - tft.fontHeight(3)/2 - 5, screen_width, tft.fontHeight(3) + 10, COLOR_BG);
    }

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(3); // Tamanho grande para o valor

    char display_str[30];
    snprintf(display_str, sizeof(display_str), getText(StringID::STR_LOCK_TIMEOUT_LABEL_FMT), temp_lock_timeout_minutes);
    
    // Quebrar manualmente se for muito longo (improvável para este formato)
    char* line1 = strtok(display_str, "\n"); 
    char* line2 = strtok(NULL, "\n");

    if (line1 && line2) {
        tft.drawString(line1, screen_width / 2, center_y_area - tft.fontHeight(3)/2 - 2);
        tft.drawString(line2, screen_width / 2, center_y_area + tft.fontHeight(3)/2 + 2);
    } else if (line1) {
        tft.drawString(line1, screen_width / 2, center_y_area);
    }

    tft.setTextDatum(TL_DATUM); // Reset datum
    tft.setTextSize(1);
}

// ---- New Brightness Adjustment Screen Drawing Functions ----
static void ui_draw_content_brightness_menu(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int item_h = UI_MENU_ITEM_HEIGHT;
    int item_spacing = UI_MENU_ITEM_SPACING;
    int total_item_h = item_h + item_spacing;
    
    if (full_redraw) {
        tft.fillRect(0, content_y, screen_width, screen_height - content_y, COLOR_BG);
    }

    tft.setTextSize(2);
    int draw_y = content_y + UI_PADDING + item_spacing;

    const StringID brightness_menu_options[] = {
        StringID::STR_MENU_BRIGHTNESS_USB,
        StringID::STR_MENU_BRIGHTNESS_BATTERY,
        StringID::STR_MENU_BRIGHTNESS_DIMMED
    };
    int num_brightness_options = sizeof(brightness_menu_options) / sizeof(brightness_menu_options[0]);

    // current_config_menu_index is reused for this small menu for simplicity of input handling logic
    // or a new index specific to this menu could be introduced if it grows.
    for (int i = 0; i < num_brightness_options; ++i) {
        // Assuming current_brightness_setting_to_adjust can be mapped to an index 0,1,2 for selection highlight
        // This might need a dedicated index like current_brightness_menu_index if current_config_menu_index is not suitable.
        // For now, let's use current_config_menu_index as a temporary measure for selection.
        bool is_selected = (i == current_config_menu_index); 
        uint16_t item_fg_color = is_selected ? COLOR_HIGHLIGHT_FG : COLOR_FG;
        uint16_t item_bg_color = is_selected ? COLOR_HIGHLIGHT_BG : COLOR_BG;

        if (is_selected) {
            tft.fillRect(UI_PADDING, draw_y, screen_width - 2 * UI_PADDING, item_h, item_bg_color);
        }

        tft.setTextColor(item_fg_color, item_bg_color);
        tft.setTextDatum(ML_DATUM);
        tft.drawString(getText(brightness_menu_options[i]), UI_PADDING * 3, draw_y + item_h / 2);
        
        draw_y += total_item_h;
    }

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(1);
}

static void ui_draw_content_adjust_brightness_value(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int content_h = screen_height - content_y - (tft.fontHeight(1) + UI_PADDING);
    int center_y_area = content_y + content_h / 2;

    if (full_redraw) {
        tft.fillRect(0, content_y, screen_width, content_h, COLOR_BG);
    } else {
        tft.fillRect(0, center_y_area - tft.fontHeight(3)/2 - 10, screen_width, tft.fontHeight(3) + 20, COLOR_BG);
    }

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(2); // Slightly smaller than timeout value, or use 3 and adjust layout

    char display_str[50];
    const char* brightness_type_str = "Error";
    switch(current_brightness_setting_to_adjust) {
        case BrightnessSettingToAdjust::USB: brightness_type_str = getText(StringID::STR_MENU_BRIGHTNESS_USB); break;
        case BrightnessSettingToAdjust::BATTERY: brightness_type_str = getText(StringID::STR_MENU_BRIGHTNESS_BATTERY); break;
        case BrightnessSettingToAdjust::DIMMED: brightness_type_str = getText(StringID::STR_MENU_BRIGHTNESS_DIMMED); break;
    }
    // Using STR_TITLE_ADJUST_BRIGHTNESS_VALUE_FMT which is "Brightness (%s): %d"
    snprintf(display_str, sizeof(display_str), getText(StringID::STR_TITLE_ADJUST_BRIGHTNESS_VALUE_FMT), 
             brightness_type_str, temp_brightness_value);
    
    // Manual line break if needed, though the format string might be short enough
    char* line1 = strtok(display_str, "\n"); 
    char* line2 = strtok(NULL, "\n");

    if (line1 && line2) {
        tft.drawString(line1, screen_width / 2, center_y_area - tft.fontHeight(2)/2 - 2);
        tft.drawString(line2, screen_width / 2, center_y_area + tft.fontHeight(2)/2 + 2);
    } else if (line1) {
        tft.drawString(line1, screen_width / 2, center_y_area);
    }

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
}

// ---- New First Card Setup Screen Drawing Function ----
static void ui_draw_content_setup_first_card(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    // This screen might not have a traditional footer, using more of the screen for the prompt.
    int content_h = screen_height - content_y; 
    int center_x = screen_width / 2;
    int center_y_area = content_y + content_h / 2;

    if (full_redraw) {
        tft.fillRect(0, content_y, screen_width, content_h, COLOR_BG);
    }

    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    
    char prompt_buf[120];
    strncpy(prompt_buf, getText(StringID::STR_PROMPT_SETUP_FIRST_CARD), sizeof(prompt_buf) -1);
    prompt_buf[sizeof(prompt_buf)-1] = '\0';

    char* line1 = strtok(prompt_buf, "\n");
    char* line2 = strtok(NULL, "\n");
    int text_y_pos = center_y_area;

    if (line1 && line2) {
        tft.drawString(line1, center_x, text_y_pos - tft.fontHeight(2)/2 - 2);
        tft.drawString(line2, center_x, text_y_pos + tft.fontHeight(2)/2 + 2);
    } else if (line1) {
        tft.drawString(line1, center_x, text_y_pos);
    }
    
    // Optionally, add a small visual indicator that RFID is active (e.g., a small RFID icon)
    // For now, the text prompt is the main indicator.

    tft.setTextDatum(TL_DATUM); // Reset datum
    tft.setTextSize(1);
}

// ... rest of ui_manager.cpp ...
