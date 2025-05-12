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
        case ScreenState::SCREEN_TOTP_VIEW:                 ui_draw_content_totp_view(full_redraw); break;
        case ScreenState::SCREEN_MENU_MAIN:                 ui_draw_content_menu_main(full_redraw); break;
        case ScreenState::SCREEN_SERVICE_ADD_WAIT:          ui_draw_content_service_add_wait(full_redraw); break;
        case ScreenState::SCREEN_SERVICE_ADD_CONFIRM:       ui_draw_content_service_add_confirm(full_redraw); break;
        case ScreenState::SCREEN_TIME_EDIT:                 ui_draw_content_time_edit(full_redraw); break;
        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM:    ui_draw_content_service_delete_confirm(full_redraw); break;
        case ScreenState::SCREEN_TIMEZONE_EDIT:             ui_draw_content_timezone_edit(full_redraw); break;
        case ScreenState::SCREEN_LANGUAGE_SELECT:           ui_draw_content_language_select(full_redraw); break;
        case ScreenState::SCREEN_READ_RFID:                 ui_draw_content_read_rfid(full_redraw); break;
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
        } else { // Limpa área do footer se não houver um
            // Calcula altura aproximada do footer para limpar
            uint8_t prev_size = tft.textsize;
            tft.setTextSize(1);
            int footer_h_approx = tft.fontHeight() + UI_PADDING;
            tft.setTextSize(prev_size);
            tft.fillRect(0, tft.height() - footer_h_approx, tft.width(), footer_h_approx, COLOR_BG);
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
    if (!footer_text || strlen(footer_text) == 0) return; // Não desenha se texto vazio

    uint8_t prev_datum = tft.getTextDatum();
    uint16_t prev_fg = tft.textcolor;
    uint16_t prev_bg = tft.textbgcolor;
    uint8_t prev_size = tft.textsize;

    tft.setTextDatum(BC_DATUM); // Bottom Center
    tft.setTextColor(COLOR_DIM_TEXT, COLOR_BG);
    tft.setTextSize(1);

    // Calcula a altura do texto para limpar a área do rodapé corretamente
    int text_height = tft.fontHeight();
    int footer_area_y = tft.height() - (text_height + UI_PADDING);
    int footer_area_h = text_height + UI_PADDING;
    tft.fillRect(0, footer_area_y, tft.width(), footer_area_h, COLOR_BG); // Limpa área do rodapé

    tft.drawString(footer_text, tft.width() / 2, UI_FOOTER_TEXT_Y);

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
        case ScreenState::SCREEN_MENU_MAIN:               title_id = StringID::STR_TITLE_MAIN_MENU; break;
        case ScreenState::SCREEN_SERVICE_ADD_WAIT:        title_id = StringID::STR_TITLE_ADD_SERVICE; break;
        case ScreenState::SCREEN_SERVICE_ADD_CONFIRM:     title_id = StringID::STR_TITLE_CONFIRM_ADD; break;
        case ScreenState::SCREEN_TIME_EDIT:               title_id = StringID::STR_TITLE_ADJUST_TIME; break;
        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM:  title_id = StringID::STR_TITLE_CONFIRM_DELETE; break;
        case ScreenState::SCREEN_TIMEZONE_EDIT:           title_id = StringID::STR_TITLE_ADJUST_TIMEZONE; break;
        case ScreenState::SCREEN_LANGUAGE_SELECT:         title_id = StringID::STR_TITLE_SELECT_LANGUAGE; break;
        case ScreenState::SCREEN_READ_RFID:               title_id = StringID::STR_TITLE_READ_RFID; break;
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
        case ScreenState::SCREEN_STARTUP:
        case ScreenState::SCREEN_MESSAGE:
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

static void ui_draw_content_menu_main(bool full_redraw) {
    int item_y_start = UI_MENU_START_Y;
    int item_total_h = UI_MENU_ITEM_HEIGHT + UI_MENU_ITEM_SPACING;
    int menu_area_h = VISIBLE_MENU_ITEMS * item_total_h - UI_MENU_ITEM_SPACING; // Altura total dos itens visíveis

    // --- Animação do Highlight ---
    if (is_menu_animating) {
        unsigned long elapsed = millis() - menu_animation_start_time;
        if (elapsed >= MENU_ANIMATION_DURATION_MS) {
            menu_highlight_y_current = menu_highlight_y_target;
            is_menu_animating = false;
        } else {
            float progress = (float)elapsed / MENU_ANIMATION_DURATION_MS;
            // Ease-out-quad: progress * (2 - progress) ou similar para suavizar
            // progress = progress * (2.0f - progress); // Exemplo de easing
            menu_highlight_y_current = menu_highlight_y_anim_start + (int)((menu_highlight_y_target - menu_highlight_y_anim_start) * progress);
        }
    } else if (menu_highlight_y_current == -1 && NUM_MENU_OPTIONS > 0) {
        // Posição inicial do highlight se ainda não definida
        int initial_visible_pos = current_menu_index - menu_top_visible_index;
        menu_highlight_y_current = item_y_start + initial_visible_pos * item_total_h;
        menu_highlight_y_target = menu_highlight_y_current; // Evita animação no primeiro desenho
    }

    // --- Limpeza e Desenho ---
    if (full_redraw) {
        tft.fillRect(0, UI_CONTENT_Y_START, tft.width(), tft.height() - UI_CONTENT_Y_START, COLOR_BG); // Limpa toda área de conteúdo + footer
    } else {
        // Limpa apenas a área dos itens do menu para atualizações parciais (animação)
        tft.fillRect(UI_PADDING, item_y_start, tft.width() - 2 * UI_PADDING, menu_area_h, COLOR_BG);
    }

    // Desenha o retângulo de destaque (highlight)
    if (menu_highlight_y_current != -1 && NUM_MENU_OPTIONS > 0) {
        tft.fillRect(UI_PADDING, menu_highlight_y_current, tft.width() - 2 * UI_PADDING - (NUM_MENU_OPTIONS > VISIBLE_MENU_ITEMS ? 10 : 0), UI_MENU_ITEM_HEIGHT, COLOR_HIGHLIGHT_BG);
    }

    // Desenha os itens do menu visíveis
    tft.setTextSize(2);
    tft.setTextDatum(ML_DATUM); // Middle Left
    int drawn_items = 0;
    for (int i = menu_top_visible_index; i < NUM_MENU_OPTIONS && drawn_items < VISIBLE_MENU_ITEMS; ++i) {
        int item_render_y = item_y_start + drawn_items * item_total_h;
        tft.setTextColor((i == current_menu_index) ? COLOR_HIGHLIGHT_FG : COLOR_FG,
                         (i == current_menu_index) ? COLOR_HIGHLIGHT_BG : COLOR_BG); // Define BG para anti-aliasing correto

        // Adiciona um preenchimento para o texto não ficar colado na borda do highlight
        tft.drawString(getText(menuOptionIDs[i]), UI_PADDING * 3, item_render_y + UI_MENU_ITEM_HEIGHT / 2);
        drawn_items++;
    }

    // --- Barra de Rolagem ---
    if (NUM_MENU_OPTIONS > VISIBLE_MENU_ITEMS) {
        int scrollbar_x = tft.width() - UI_PADDING - 6;
        int scrollbar_w = 4;
        int scrollbar_track_y = item_y_start;
        int scrollbar_track_h = menu_area_h;

        tft.fillRect(scrollbar_x, scrollbar_track_y, scrollbar_w, scrollbar_track_h, COLOR_DIM_TEXT); // Trilho

        int thumb_h = max(10, scrollbar_track_h * VISIBLE_MENU_ITEMS / NUM_MENU_OPTIONS); // Altura proporcional do thumb
        int thumb_max_y_offset = scrollbar_track_h - thumb_h; // Deslocamento máximo do thumb dentro do trilho

        int thumb_y_offset = 0;
        if (NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS > 0) { // Evita divisão por zero
            thumb_y_offset = round((float)thumb_max_y_offset * menu_top_visible_index / (NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS));
        }
        thumb_y_offset = constrain(thumb_y_offset, 0, thumb_max_y_offset);

        tft.fillRect(scrollbar_x, scrollbar_track_y + thumb_y_offset, scrollbar_w, thumb_h, COLOR_ACCENT); // Thumb
    }

    // Restaura defaults de texto
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

static void ui_draw_content_time_edit(bool full_redraw) {
    int content_y = UI_CONTENT_Y_START;
    int content_h = tft.height() - content_y - (tft.fontHeight(1) + UI_PADDING);
    int center_y_area = content_y + content_h / 2;

    if (full_redraw) {
        tft.fillRect(0, content_y, tft.width(), content_h, COLOR_BG); // Limpa área de conteúdo

        tft.setTextColor(COLOR_DIM_TEXT, COLOR_BG);
        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);

        char tz_info_buf[40];
        snprintf(tz_info_buf, sizeof(tz_info_buf), getText(StringID::STR_TIME_EDIT_INFO_FMT), gmt_offset);
        tft.drawString(tz_info_buf, tft.width() / 2, center_y_area + 45);
        tft.drawString(getText(StringID::STR_TIME_EDIT_JSON_HINT), tft.width() / 2, center_y_area + 60);
        tft.drawString(getText(StringID::STR_EXAMPLE_JSON_TIME), tft.width() / 2, center_y_area + 75);
    } else {
        // Limpa apenas a área da hora e do marcador para atualização parcial
        tft.fillRect(0, center_y_area - 30, tft.width(), 60, COLOR_BG);
    }

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(3); // Tamanho grande para a hora

    char time_str_buf[12];
    snprintf(time_str_buf, sizeof(time_str_buf), "%02d:%02d:%02d", edit_hour, edit_minute, edit_second);
    tft.drawString(time_str_buf, tft.width() / 2, center_y_area);

    // Desenha marcador do campo selecionado
    int field_width_hh = tft.textWidth("00", 3); // Largura de "HH" no tamanho 3
    int separator_width = tft.textWidth(":", 3);
    int total_time_width = 3 * field_width_hh + 2 * separator_width;
    int time_start_x = (tft.width() - total_time_width) / 2;

    int marker_x = time_start_x;
    if (edit_time_field == 1) marker_x = time_start_x + field_width_hh + separator_width;
    else if (edit_time_field == 2) marker_x = time_start_x + 2 * (field_width_hh + separator_width);

    int marker_y = center_y_area - tft.fontHeight(3)/2 - 6; // Acima do texto
    int marker_h = 4;
    tft.fillRect(marker_x, marker_y, field_width_hh, marker_h, COLOR_ACCENT);

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
    snprintf(tz_str, sizeof(tz_str), getText(StringID::STR_TIMEZONE_LABEL), gmt_offset);
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