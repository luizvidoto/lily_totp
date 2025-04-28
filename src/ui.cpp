#include "ui.h"
#include "globals.h"
#include "config.h"
#include "i18n.h"
#include "types.h" // Já incluído via outros, mas explícito
#include "hardware.h" // Para powerUp/DownRFID e battery_info
#include "totp.h"     // Para TOTP_INTERVAL_SECONDS
#include <TimeLib.h>  // Para now(), hour(), minute(), second()

// ============================================================================
// === DEFINIÇÕES INTERNAS E VARIÁVEIS ESTÁTICAS ===
// ============================================================================

// // Mapeamento dos índices do menu principal para as chaves JSON
// const char* mainMenuKeys[] = {
//     "MENU_ADD_SERVICE",
//     "MENU_READ_RFID",
//     "MENU_VIEW_CODES",
//     "MENU_ADJUST_TIME",
//     "MENU_ADJUST_TIMEZONE",
//     "MENU_SELECT_LANGUAGE"
// };
// const int NUM_MAIN_MENU_OPTIONS = sizeof(mainMenuKeys) / sizeof(mainMenuKeys[0]);

// Últimos valores desenhados (para otimização, evitar redesenho desnecessário)
static char last_drawn_clock_str[9] = "";
static int last_drawn_batt_level = -1;
static bool last_drawn_usb_state = false;
static char last_drawn_totp_code[7] = "";
static uint32_t last_drawn_totp_interval = 0; // Usado para progresso

// ============================================================================
// === INICIALIZAÇÃO DA UI ===
// ============================================================================

void initSprites() {
    // --- Sprite do Relógio no Header ---
    spr_header_clock.setColorDepth(8); // 8 bits (256 cores) deve ser suficiente
    // Calcula largura baseada na string máxima HH:MM:SS e fonte
    uint16_t clockW = tft.textWidth("00:00:00", FONT_SIZE_HEADER);
    uint16_t clockH = tft.fontHeight(FONT_SIZE_HEADER);
    spr_header_clock.createSprite(clockW, clockH);
    spr_header_clock.setTextDatum(MR_DATUM); // Middle Right alignment
    spr_header_clock.setTextColor(COLOR_HEADER_FG, COLOR_HEADER_BG);
    spr_header_clock.setTextSize(FONT_SIZE_HEADER);

    // --- Sprite da Bateria no Header ---
    spr_header_batt.setColorDepth(8);
    spr_header_batt.createSprite(UI_BATT_WIDTH, UI_BATT_HEIGHT);
    // Não precisa de datum, desenhamos manualmente

    // --- Sprite do Código TOTP ---
    spr_totp_code.setColorDepth(16); // Mais cores para antialiasing da fonte grande
    uint16_t totpW = tft.textWidth("000000", FONT_SIZE_TOTP_CODE) + 10; // Largura + margem
    uint16_t totpH = tft.fontHeight(FONT_SIZE_TOTP_CODE) + 4;      // Altura + margem
    spr_totp_code.createSprite(totpW, totpH);
    spr_totp_code.setTextDatum(MC_DATUM); // Middle Center alignment
    spr_totp_code.setTextColor(COLOR_FG, COLOR_BG);
    spr_totp_code.setTextSize(FONT_SIZE_TOTP_CODE);

    // --- Sprite da Barra de Progresso ---
    spr_progress_bar.setColorDepth(8);
    // Sprite um pouco menor que a área do contorno para caber dentro
    uint16_t progW = tft.width() - 2 * (UI_PADDING + 2) - 20; // Ajuste conforme necessário
    uint16_t progH = UI_PROGRESS_BAR_HEIGHT;
    spr_progress_bar.createSprite(progW, progH);
    // Não precisa de datum

    Serial.println("[UI] Sprites inicializados.");
}

// ============================================================================
// === CONTROLE DE TELA E MENSAGENS ===
// ============================================================================

void changeScreen(ScreenState new_screen) {
    // Ações de saída da tela *anterior*
    if (current_screen == ScreenState::SCREEN_READ_RFID && new_screen != ScreenState::SCREEN_READ_RFID) {
        powerDownRFID(); // Desliga antena ao sair da tela RFID
    }
    if (current_screen == ScreenState::SCREEN_MENU_MAIN || current_screen == ScreenState::SCREEN_LANGUAGE_SELECT) {
        main_menu_state.is_animating = false; // Para animação se sair do menu
        lang_menu_state.is_animating = false;
    }

    // Definir telas
    previous_screen = current_screen;
    current_screen = new_screen;
    Serial.printf("[UI] Mudando para tela: %d\n", (int)current_screen);

    // Resetar estados relevantes
    message_end_time = 0;        // Cancela qualquer mensagem temporária
    request_full_redraw = true; // Força redesenho completo da nova tela

    // Ações de entrada na tela *nova*
    if (new_screen == ScreenState::SCREEN_MENU_MAIN) {
        resetMenuState(main_menu_state);
        // Centralizar visão no item atual
        if (NUM_MENU_OPTIONS > 0) {
             main_menu_state.top_visible_index = max(0, min(main_menu_state.current_index - VISIBLE_MENU_ITEMS / 2, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS));
        }
    } else if (new_screen == ScreenState::SCREEN_LANGUAGE_SELECT) {
        resetMenuState(lang_menu_state);
        // Definir índice inicial baseado no idioma atual
        lang_menu_state.current_index = (int)getCurrentLanguage();
        lang_menu_state.top_visible_index = max(0, min(lang_menu_state.current_index - VISIBLE_MENU_ITEMS / 2, NUM_LANGUAGES - VISIBLE_MENU_ITEMS));
    } else if (new_screen == ScreenState::SCREEN_READ_RFID) {
        powerUpRFID(); // Liga antena ao entrar na tela RFID
        temp_data.rfid_card_id[0] = '\0'; // Limpa ID anterior
    } else if (new_screen == ScreenState::SCREEN_TIME_EDIT) {
        // Carrega hora atual para edição (em UTC)
        time_t t_utc = now();
        temp_data.edit_hour = hour(t_utc);
        temp_data.edit_minute = minute(t_utc);
        temp_data.edit_second = second(t_utc);
        temp_data.edit_time_field = 0; // Começa editando hora
    } else if (new_screen == ScreenState::SCREEN_TIMEZONE_EDIT) {
        temp_data.edit_gmt_offset = gmt_offset_hours; // Carrega fuso atual para edição
    }

    // O redesenho ocorrerá no próximo ciclo do loop principal devido a request_full_redraw = true;
}

void ui_showTemporaryMessage(const char *msg, uint16_t color) {
    strncpy(message_buffer, msg, sizeof(message_buffer) - 1);
    message_buffer[sizeof(message_buffer) - 1] = '\0'; // Garante terminação nula
    message_color = color;
    message_end_time = millis() + TEMPORARY_MESSAGE_DURATION_MS;
    Serial.printf("[UI] Exibindo mensagem: %s\n", message_buffer);
    changeScreen(ScreenState::SCREEN_MESSAGE); // Muda para a tela de mensagem
}

// Verifica e processa expiração da mensagem temporária
// Retorna true se a mensagem expirou e a tela foi trocada, false caso contrário
bool ui_updateTemporaryMessage() {
    if (current_screen == ScreenState::SCREEN_MESSAGE && message_end_time != 0 && millis() >= message_end_time) {
        message_end_time = 0; // Desativa timer

        // Decide para qual tela voltar
        ScreenState targetScreen = ScreenState::SCREEN_MENU_MAIN; // Padrão é voltar ao menu

        // Voltar para TOTP se estava vendo códigos (exceto após erro grave ou ação de menu)
        if (previous_screen == ScreenState::SCREEN_TOTP_VIEW &&
            message_color != COLOR_ERROR) { // Não volta para TOTP se foi erro
             // Poderia adicionar mais exceções: não voltar se a msg foi de add/delete/save
            targetScreen = ScreenState::SCREEN_TOTP_VIEW;
        }
        // Caso especial: Após adicionar serviço com sucesso, ir para TOTP
        else if (previous_screen == ScreenState::SCREEN_SERVICE_ADD_CONFIRM && message_color == COLOR_SUCCESS) {
            targetScreen = ScreenState::SCREEN_TOTP_VIEW;
        }
        // Se a tela anterior era uma que logicamente leva de volta ao menu
        else if ( previous_screen == ScreenState::SCREEN_SERVICE_ADD_WAIT ||
                  previous_screen == ScreenState::SCREEN_SERVICE_DELETE_CONFIRM ||
                  previous_screen == ScreenState::SCREEN_TIME_EDIT ||
                  previous_screen == ScreenState::SCREEN_TIMEZONE_EDIT ||
                  previous_screen == ScreenState::SCREEN_LANGUAGE_SELECT ||
                  previous_screen == ScreenState::SCREEN_READ_RFID)
        {
             targetScreen = ScreenState::SCREEN_MENU_MAIN;
        }


        Serial.printf("[UI] Mensagem expirou, voltando para tela %d\n", (int)targetScreen);
        changeScreen(targetScreen);
        return true; // Tela foi trocada
    }
    return false; // Nenhuma mudança
}

// Processa a animação do menu (chamada antes de desenhar o menu)
void ui_updateMenuAnimation(MenuState& menu) {
    if (!menu.is_animating) return;

    unsigned long elapsed = millis() - menu.animation_start_time;
    if (elapsed >= MENU_ANIMATION_DURATION_MS) {
        menu.highlight_y_current = menu.highlight_y_target;
        menu.is_animating = false; // Termina animação
    } else {
        // Interpolação linear simples (pode usar easing functions para mais suavidade)
        float progress = (float)elapsed / MENU_ANIMATION_DURATION_MS;
        // Evita overshoot movendo da posição *alvo* para a *atual* com (1-progress)
        menu.highlight_y_current = menu.highlight_y_target + (int)((menu.highlight_y_current - menu.highlight_y_target) * (1.0f - progress));
         // Necessário redesenhar parcialmente durante a animação
         // request_full_redraw = false; // Garante que não será full redraw
    }
}

void resetMenuState(MenuState& menu) {
    // menu.current_index = 0; // Não reseta o índice selecionado
    menu.top_visible_index = 0;
    menu.highlight_y_current = -1; // Posição inválida para forçar cálculo inicial
    menu.highlight_y_target = -1;
    menu.animation_start_time = 0;
    menu.is_animating = false;
}

// ============================================================================
// === ATUALIZAÇÃO DOS SPRITES ===
// ============================================================================

void ui_updateHeaderClockSprite() {
    time_t t_local = now() + gmt_offset_hours * 3600; // Calcula hora local
    char current_time_str[9];
    snprintf(current_time_str, sizeof(current_time_str), "%02d:%02d:%02d",
             hour(t_local), minute(t_local), second(t_local));

    // Otimização: Só redesenha o sprite se o texto mudou
    if (strcmp(last_drawn_clock_str, current_time_str) != 0) {
        strcpy(last_drawn_clock_str, current_time_str);
        spr_header_clock.fillSprite(COLOR_HEADER_BG); // Limpa com a cor de fundo do header
        spr_header_clock.drawString(current_time_str, spr_header_clock.width(), spr_header_clock.height() / 2);
    }
    // Empurra o sprite para a tela (mesmo que não tenha mudado, para cobrir área antiga se header foi redesenhado)
    int x_pos = tft.width() - spr_header_clock.width() - UI_BATT_WIDTH - UI_PADDING * 2;
    int y_pos = (UI_HEADER_HEIGHT - spr_header_clock.height()) / 2;
    spr_header_clock.pushSprite(x_pos, y_pos);
}

void ui_updateHeaderBatterySprite() {
    // Otimização: Só redesenha o sprite se o nível ou estado USB mudou
    if (last_drawn_batt_level != battery_info.level_percent || last_drawn_usb_state != battery_info.is_usb_powered) {
        last_drawn_batt_level = battery_info.level_percent;
        last_drawn_usb_state = battery_info.is_usb_powered;

        spr_header_batt.fillSprite(COLOR_HEADER_BG); // Limpa com fundo do header

        // Coordenadas relativas ao sprite (0,0)
        int icon_w = UI_BATT_WIDTH - 2, icon_h = UI_BATT_HEIGHT - 2;
        int icon_x = 1, icon_y = 1; // Dentro do sprite
        int term_w = 2, term_h = icon_h / 2;
        int term_x = icon_x + icon_w, term_y = icon_y + (icon_h - term_h) / 2;
        uint16_t fg_color = COLOR_HEADER_FG;
        uint16_t bg_color = COLOR_HEADER_BG;

        if (battery_info.is_usb_powered) { // Ícone USB/Raio
            spr_header_batt.drawRect(icon_x, icon_y, icon_w, icon_h, fg_color); // Contorno
            spr_header_batt.fillRect(term_x, term_y, term_w, term_h, fg_color); // Terminal
            // Raio (simplificado)
            int bolt_x = icon_x + icon_w / 2 - 3, bolt_y = icon_y + 1;
            int bolt_h_half = (icon_h > 2) ? (icon_h - 2) / 2 : 0;
            int bolt_y_mid = bolt_y + bolt_h_half;
            int bolt_y_end = (icon_h > 2) ? (icon_y + icon_h - 2) : bolt_y;
            spr_header_batt.drawLine(bolt_x + 3, bolt_y, bolt_x, bolt_y_mid, fg_color);
            spr_header_batt.drawLine(bolt_x, bolt_y_mid, bolt_x + 3, bolt_y_mid, fg_color);
            spr_header_batt.drawLine(bolt_x + 3, bolt_y_mid, bolt_x, bolt_y_end, fg_color);
        } else { // Ícone Bateria Normal
            spr_header_batt.drawRect(icon_x, icon_y, icon_w, icon_h, fg_color); // Contorno
            spr_header_batt.fillRect(term_x, term_y, term_w, term_h, fg_color); // Terminal
            int fill_w_max = (icon_w > 2) ? (icon_w - 2) : 0;
            int fill_h = (icon_h > 2) ? (icon_h - 2) : 0;
            int fill_w = fill_w_max * battery_info.level_percent / 100;
            fill_w = constrain(fill_w, 0, fill_w_max);
            uint16_t fill_color = COLOR_SUCCESS;
            if (battery_info.level_percent < 50) fill_color = COLOR_WARNING;
            if (battery_info.level_percent < 20) fill_color = COLOR_ERROR;
            if (fill_w > 0) spr_header_batt.fillRect(icon_x + 1, icon_y + 1, fill_w, fill_h, fill_color);
            if (fill_w < fill_w_max) spr_header_batt.fillRect(icon_x + 1 + fill_w, icon_y + 1, fill_w_max - fill_w, fill_h, bg_color);
        }
    }
    // Empurra o sprite para a tela
    int x_pos = tft.width() - UI_BATT_WIDTH - UI_PADDING;
    int y_pos = (UI_HEADER_HEIGHT - UI_BATT_HEIGHT) / 2;
    spr_header_batt.pushSprite(x_pos, y_pos);
}

void ui_updateTotpCodeSprite() {
    // Otimização: Só redesenha se o código mudou
    if (strcmp(last_drawn_totp_code, current_totp.code) != 0) {
        strcpy(last_drawn_totp_code, current_totp.code);
        spr_totp_code.fillSprite(COLOR_BG); // Limpa com fundo geral
        spr_totp_code.drawString(current_totp.code, spr_totp_code.width() / 2, spr_totp_code.height() / 2);
    }
    // Empurra o sprite para a tela
    int x_pos = (tft.width() - spr_totp_code.width()) / 2;
    // Calcula Y para centralizar verticalmente na área de conteúdo disponível
    int footer_height = tft.fontHeight(FONT_SIZE_SMALL) + UI_PADDING;
    int content_y_start = UI_HEADER_HEIGHT;
    int content_height = tft.height() - content_y_start - footer_height - UI_PROGRESS_BAR_HEIGHT - 20; // Subtrai espaço da barra e margens
    int content_center_y = content_y_start + content_height / 2;
    int y_pos = content_center_y - spr_totp_code.height() / 2;
    spr_totp_code.pushSprite(x_pos, y_pos);
}

void ui_updateProgressBarSprite(uint64_t current_timestamp_utc) {
    uint32_t current_interval = current_timestamp_utc / TOTP_INTERVAL_SECONDS;
    // Otimização: Só redesenha se o intervalo (segundos restantes) mudou
    if (last_drawn_totp_interval != current_interval) {
        last_drawn_totp_interval = current_interval;
        uint32_t seconds_remaining = TOTP_INTERVAL_SECONDS - (current_timestamp_utc % TOTP_INTERVAL_SECONDS);
        int bar_width = spr_progress_bar.width();
        int bar_height = spr_progress_bar.height();
        int progress_w = map(seconds_remaining, 0, TOTP_INTERVAL_SECONDS, 0, bar_width);
        progress_w = constrain(progress_w, 0, bar_width);

        spr_progress_bar.fillSprite(COLOR_BAR_BG); // Fundo da barra
        if (progress_w > 0) {
            spr_progress_bar.fillRect(0, 0, progress_w, bar_height, COLOR_BAR_FG); // Preenchimento
        }
    }
    // Empurra o sprite para a tela
    int x_pos = (tft.width() - spr_progress_bar.width()) / 2;
    // Calcula Y para ficar abaixo do código TOTP
    int footer_height = tft.fontHeight(FONT_SIZE_SMALL) + UI_PADDING;
    int content_y_start = UI_HEADER_HEIGHT;
    int content_height = tft.height() - content_y_start - footer_height - UI_PROGRESS_BAR_HEIGHT - 20;
    int content_center_y = content_y_start + content_height / 2;
    int totp_code_bottom_y = content_center_y + spr_totp_code.height() / 2;
    int y_pos = totp_code_bottom_y + 10; // Espaço abaixo do código
    spr_progress_bar.pushSprite(x_pos, y_pos);
}


// ============================================================================
// === FUNÇÕES DE DESENHO DA UI ===
// ============================================================================

// Helper para obter a chave JSON do título da tela
const char* getScreenTitleKey(ScreenState state) {
    if (state == ScreenState::SCREEN_TOTP_VIEW) {
        return (service_count > 0 && current_service_index >= 0 && current_service_index < service_count)
               ? services[current_service_index].name // Usa o nome do serviço como "chave" direta (não traduzível por padrão)
               : getText(StringID::NONE); // Chave para "Sem Serviço"
    }
    switch(state){
        // case ScreenState::BOOTING:              return getText(StringID::NONE); // Sem título padrão
        case ScreenState::SCREEN_MENU_MAIN:              return getText(StringID::STR_TITLE_MAIN_MENU);
        case ScreenState::SCREEN_SERVICE_ADD_WAIT:       return getText(StringID::STR_TITLE_ADD_SERVICE);
        case ScreenState::SCREEN_SERVICE_ADD_CONFIRM:    return getText(StringID::STR_TITLE_CONFIRM_ADD);
        case ScreenState::SCREEN_TIME_EDIT:              return getText(StringID::STR_TITLE_ADJUST_TIME);
        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM: return getText(StringID::STR_TITLE_CONFIRM_DELETE);
        case ScreenState::SCREEN_TIMEZONE_EDIT:          return getText(StringID::STR_TITLE_ADJUST_TIMEZONE);
        case ScreenState::SCREEN_LANGUAGE_SELECT:        return getText(StringID::STR_TITLE_SELECT_LANGUAGE);
        case ScreenState::SCREEN_READ_RFID:              return getText(StringID::STR_TITLE_READ_RFID);
        case ScreenState::SCREEN_MESSAGE:                return getText(StringID::NONE); // Sem título padrão
        default:                                          return getText(StringID::NONE); // Sem título para desconhecido
    }
}

// Desenha a parte estática do header (fundo e título)
void ui_drawHeader(const char* title_text) {
    tft.fillRect(0, 0, tft.width(), UI_HEADER_HEIGHT, COLOR_HEADER_BG); // Fundo

    uint8_t prev_datum = tft.getTextDatum();
    tft.setTextDatum(ML_DATUM); // Middle Left
    tft.setTextColor(COLOR_HEADER_FG, COLOR_HEADER_BG);
    tft.setTextSize(FONT_SIZE_HEADER);

    // Truncamento simples se necessário (considerando largura da tela menos espaço para relógio e bateria)
    int max_title_width = tft.width() - UI_PADDING * 3 - spr_header_clock.width() - UI_BATT_WIDTH;
    int current_title_width = tft.textWidth(title_text, FONT_SIZE_HEADER);
    char truncated_title[MAX_SERVICE_NAME_LEN + 4]; // Buffer para truncar

    if (current_title_width > max_title_width) {
        int len = strlen(title_text);
        int cutoff = len;
        const int ellipsis_width = tft.textWidth("...", FONT_SIZE_HEADER);
        while (tft.textWidth(title_text, FONT_SIZE_HEADER) > max_title_width - ellipsis_width && cutoff > 0) {
            cutoff--;
        }
        strncpy(truncated_title, title_text, cutoff);
        strcpy(truncated_title + cutoff, "...");
        title_text = truncated_title;
    }

    tft.drawString(title_text, UI_PADDING, UI_HEADER_HEIGHT / 2);
    tft.setTextDatum(prev_datum); // Restaura datum
    // Cores e tamanho serão restaurados pela próxima função que desenhar
}

// Atualiza as partes dinâmicas do header (chamado sempre)
void ui_updateHeaderDynamic() {
    ui_updateHeaderClockSprite();
    ui_updateHeaderBatterySprite();
}

// --- Função Principal de Desenho ---
void ui_drawScreen(bool full_redraw) {
    // 1. Processar expiração de mensagem temporária PRIMEIRO
    if (ui_updateTemporaryMessage()) {
        return; // Tela mudou, não precisa desenhar a tela antiga/mensagem
    }

    // 2. Lógica específica para telas que não usam header/footer padrão
    if (current_screen == ScreenState::SCREEN_MESSAGE) {
        ui_drawScreenMessage(full_redraw); // Só desenha se for full_redraw (a mensagem não muda)
        return;
    }
    // if (current_screen == ScreenState::BOOTING) {
    //     // A tela de boot é geralmente estática ou atualizada por funções específicas
    //     // ui_drawScreenBootingContent(full_redraw); // Implementar se necessário
    //     return;
    // }

    // 3. Processar animação do menu (se aplicável)
    if (current_screen == ScreenState::SCREEN_MENU_MAIN) {
        ui_updateMenuAnimation(main_menu_state);
        // Se a animação estiver ocorrendo, força uma atualização parcial
        if (main_menu_state.is_animating) full_redraw = false;
    } else if (current_screen == ScreenState::SCREEN_LANGUAGE_SELECT) {
        ui_updateMenuAnimation(lang_menu_state);
        if (lang_menu_state.is_animating) full_redraw = false;
    }


    // 4. Desenhar Header
    if (full_redraw) {
        // Obtém a chave ou nome para o título
        const char* title_key = getScreenTitleKey(current_screen);
        ui_drawHeader(title_key);
    }
    // Sempre atualiza relógio e bateria (via sprites)
    ui_updateHeaderDynamic();

    // 5. Desenhar Conteúdo Específico da Tela
    int content_y = UI_CONTENT_Y_START;
    int content_h = tft.height() - content_y; // Altura disponível para conteúdo + footer

    // Limpar área de conteúdo apenas em full_redraw (exceto para menu que limpa internamente)
    if (full_redraw && current_screen != ScreenState::SCREEN_MENU_MAIN && current_screen != ScreenState::SCREEN_LANGUAGE_SELECT) {
        tft.fillRect(0, content_y, tft.width(), content_h, COLOR_BG);
    }

    switch (current_screen) {
        case ScreenState::SCREEN_TOTP_VIEW:              ui_drawScreenTOTPContent(full_redraw); break;
        case ScreenState::SCREEN_MENU_MAIN:              ui_drawScreenMenuContent(full_redraw); break;
        case ScreenState::SCREEN_SERVICE_ADD_WAIT:       ui_drawScreenServiceAddWaitContent(full_redraw); break;
        case ScreenState::SCREEN_SERVICE_ADD_CONFIRM:    ui_drawScreenServiceAddConfirmContent(full_redraw); break;
        case ScreenState::SCREEN_TIME_EDIT:              ui_drawScreenTimeEditContent(full_redraw); break;
        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM: ui_drawScreenServiceDeleteConfirmContent(full_redraw); break;
        case ScreenState::SCREEN_TIMEZONE_EDIT:          ui_drawScreenTimezoneEditContent(full_redraw); break;
        case ScreenState::SCREEN_LANGUAGE_SELECT:        ui_drawScreenLanguageSelectContent(full_redraw); break;
        case ScreenState::SCREEN_READ_RFID:              ui_drawScreenReadRFIDContent(full_redraw); break;
        default:
             if(full_redraw) tft.fillRect(0, content_y, tft.width(), content_h, COLOR_ERROR); // Tela desconhecida
             break;
    }

    // Resetar estado para próxima iteração
    // request_full_redraw = false; // Movido para o loop principal
}

// --- Desenho de Conteúdo Específico por Tela ---

void ui_drawScreenBootingContent(bool full_redraw) {
    // Chamada por funções específicas de boot, não por ui_drawScreen padrão
    // Exemplo: ui_drawBootScreenMessage(getText("STATUS_LOADING_SERVICES"));
     if (full_redraw) {
         tft.fillScreen(COLOR_BG);
         tft.setTextColor(COLOR_FG, COLOR_BG);
         tft.setTextSize(FONT_SIZE_MESSAGE);
         tft.setTextDatum(MC_DATUM);
         tft.drawString(message_buffer, tft.width() / 2, tft.height() / 2);
         tft.setTextDatum(TL_DATUM); // Reset datum
     }
}


void ui_drawScreenTOTPContent(bool full_redraw) {
    int footer_height = tft.fontHeight(FONT_SIZE_SMALL) + UI_PADDING;
    int content_y_start = UI_HEADER_HEIGHT;
    int content_height = tft.height() - content_y_start - footer_height;

    if (full_redraw) {
        // Limpa área de conteúdo (exceto header)
        tft.fillRect(0, content_y_start, tft.width(), content_height + footer_height, COLOR_BG);

        // Desenha contorno da barra de progresso (se houver serviço)
        if (service_count > 0 && current_service_index != -1) {
             int prog_bar_outline_w = spr_progress_bar.width() + 4;
             int prog_bar_outline_h = spr_progress_bar.height() + 4;
             int prog_bar_outline_x = (tft.width() - prog_bar_outline_w) / 2;
             // Calcula Y baseado na posição do sprite da barra
             int content_center_y = content_y_start + (content_height - UI_PROGRESS_BAR_HEIGHT - 10) / 2;
             int totp_code_bottom_y = content_center_y + spr_totp_code.height() / 2;
             int prog_bar_sprite_y = totp_code_bottom_y + 10;
             int prog_bar_outline_y = prog_bar_sprite_y - 2;

             tft.drawRoundRect(prog_bar_outline_x, prog_bar_outline_y, prog_bar_outline_w, prog_bar_outline_h, UI_PROGRESS_BAR_CORNER_RADIUS, COLOR_FG);
        } else {
            // Mostra mensagem de "Sem Serviços"
             tft.setTextColor(COLOR_FG, COLOR_BG);
             tft.setTextSize(FONT_SIZE_MESSAGE);
             tft.setTextDatum(MC_DATUM);
             tft.drawString(getText(StringID::STR_ERROR_NO_SERVICES), tft.width()/2, content_y_start + content_height / 2);
             tft.setTextDatum(TL_DATUM); // Reset
        }

        // Desenha rodapé
        tft.setTextColor(COLOR_DIM_TEXT, COLOR_BG);
        tft.setTextSize(FONT_SIZE_SMALL);
        tft.setTextDatum(BC_DATUM); // Bottom Center
        tft.drawString(getText(StringID::STR_FOOTER_GENERIC_NAV), tft.width() / 2, UI_FOOTER_TEXT_Y);
        tft.setTextDatum(TL_DATUM); // Reset datum
    }

    // Atualiza e desenha sprites (se houver serviço)
    if (service_count > 0 && current_service_index != -1) {
        ui_updateTotpCodeSprite();
        ui_updateProgressBarSprite(now()); // Passa tempo atual
    }
}


void ui_drawScreenMenuContent(bool full_redraw) {
    MenuState& menu = main_menu_state; // Atalho
    int item_y_start = UI_MENU_START_Y;
    int item_spacing = UI_MENU_ITEM_SPACING;
    int item_total_height = UI_MENU_ITEM_HEIGHT + item_spacing;
    int menu_area_height = tft.height() - item_y_start; // Altura da área do menu + footer

    // --- Cálculo inicial da posição do highlight (se ainda não definido) ---
    if (menu.highlight_y_current == -1 && NUM_MENU_OPTIONS > 0) {
         int initial_visible_pos = menu.current_index - menu.top_visible_index;
         menu.highlight_y_current = item_y_start + initial_visible_pos * item_total_height;
         menu.highlight_y_target = menu.highlight_y_current; // Evita animação inicial
    }

    // --- Limpeza e Desenho ---
    int scrollbar_x = tft.width() - UI_PADDING - UI_MENU_SCROLLBAR_WIDTH;
    int menu_items_width = scrollbar_x - UI_PADDING * 2;

    if (full_redraw) {
        // Limpa toda a área de conteúdo abaixo do header
        tft.fillRect(0, UI_HEADER_HEIGHT, tft.width(), tft.height() - UI_HEADER_HEIGHT, COLOR_BG);
        // Desenha rodapé estático com status
        tft.setTextColor(COLOR_DIM_TEXT, COLOR_BG); tft.setTextSize(FONT_SIZE_SMALL);
        tft.setTextDatum(BL_DATUM); // Bottom Left
        char status_buf[40];
        snprintf(status_buf, sizeof(status_buf), getText(StringID::STR_SERVICES_STATUS_FMT), service_count, MAX_SERVICES);
        tft.drawString(status_buf, UI_PADDING, UI_FOOTER_TEXT_Y);
        // Instrução de navegação no rodapé
        tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(StringID::STR_FOOTER_GENERIC_NAV), tft.width()/2, UI_FOOTER_TEXT_Y);
        tft.setTextDatum(TL_DATUM); // Reset
    } else {
         // Limpeza parcial: Apenas a área dos itens visíveis + highlight
         // (A animação acontece aqui)
         tft.fillRect(UI_PADDING, item_y_start, menu_items_width, VISIBLE_MENU_ITEMS * item_total_height, COLOR_BG);
    }

    // --- Desenha Highlight ---
     if (menu.highlight_y_current != -1 && NUM_MENU_OPTIONS > 0) {
        tft.fillRect(UI_PADDING, menu.highlight_y_current, menu_items_width, UI_MENU_ITEM_HEIGHT, COLOR_HIGHLIGHT_BG);
     }

    // --- Desenha Itens Visíveis ---
    tft.setTextSize(FONT_SIZE_MENU_ITEM);
    tft.setTextDatum(ML_DATUM); // Middle Left
    int draw_count = 0;
    for (int i = menu.top_visible_index; i < NUM_MENU_OPTIONS && draw_count < VISIBLE_MENU_ITEMS; ++i) {
        int current_item_draw_y = item_y_start + draw_count * item_total_height;
        // Define cor: invertido se for o item com highlight (mesmo durante animação)
        tft.setTextColor((i == menu.current_index) ? COLOR_HIGHLIGHT_FG : COLOR_FG, COLOR_BG); // BG transparente
        // Ajusta cor de fundo se for o item destacado (para cobrir texto antigo na animação)
         if (i == menu.current_index) {
            tft.setTextColor(COLOR_HIGHLIGHT_FG, COLOR_HIGHLIGHT_BG);
         } else {
            tft.setTextColor(COLOR_FG, COLOR_BG);
         }

        // Desenha texto do item
        tft.drawString(getText(menuOptionIDs[i]), UI_PADDING * 3, current_item_draw_y + UI_MENU_ITEM_HEIGHT / 2);
        draw_count++;
    }
    //  tft.setTextbgcolor(COLOR_BG); // Reseta BG do texto

    // --- Desenha Barra de Rolagem ---
    if (NUM_MENU_OPTIONS > VISIBLE_MENU_ITEMS) {
        int scrollbar_h = VISIBLE_MENU_ITEMS * item_total_height - item_spacing; // Altura da área visível do menu
        // Limpa área da barra de rolagem (necessário em updates parciais)
        tft.fillRect(scrollbar_x, item_y_start, UI_MENU_SCROLLBAR_WIDTH, scrollbar_h, COLOR_BAR_BG); // Trilho

        int thumb_h = max(5, scrollbar_h * VISIBLE_MENU_ITEMS / NUM_MAIN_MENU_OPTIONS); // Altura proporcional
        int thumb_max_y = scrollbar_h - thumb_h; // Deslocamento máximo
        int thumb_y = item_y_start;
        if (NUM_MAIN_MENU_OPTIONS - VISIBLE_MENU_ITEMS > 0) { // Evita divisão por zero
             thumb_y += round((float)thumb_max_y * menu.top_visible_index / (NUM_MAIN_MENU_OPTIONS - VISIBLE_MENU_ITEMS));
        }
        thumb_y = constrain(thumb_y, item_y_start, item_y_start + thumb_max_y); // Garante limites

        tft.fillRect(scrollbar_x, thumb_y, UI_MENU_SCROLLBAR_WIDTH, thumb_h, COLOR_ACCENT); // Indicador
    }

    // --- Reset Texto ---
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(1); // Default size
}


void ui_drawScreenServiceAddWaitContent(bool full_redraw) {
    if (full_redraw) {
        // Área de conteúdo já limpa por ui_drawScreen
        tft.setTextColor(COLOR_FG); tft.setTextSize(FONT_SIZE_MESSAGE); tft.setTextDatum(MC_DATUM);
        int center_y = UI_HEADER_HEIGHT + (tft.height() - UI_HEADER_HEIGHT) / 2;
        tft.drawString(getText(StringID::STR_AWAITING_JSON), tft.width() / 2, center_y - 30);
        tft.drawString(getText(StringID::STR_VIA_SERIAL), tft.width() / 2, center_y );

        // Desenha exemplo e rodapé
        tft.setTextSize(FONT_SIZE_SMALL); tft.setTextColor(COLOR_DIM_TEXT);
        tft.drawString(getText(StringID::STR_EXAMPLE_JSON_SERVICE), tft.width() / 2, center_y + 40);
        tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(StringID::STR_FOOTER_GENERIC_NAV), tft.width()/2, UI_FOOTER_TEXT_Y);

        // Reset
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); tft.setTextSize(1);
    }
    // Nenhum conteúdo dinâmico nesta tela
}

void ui_drawScreenServiceAddConfirmContent(bool full_redraw) {
     if (full_redraw) {
        // Área de conteúdo já limpa
        tft.setTextColor(COLOR_ACCENT); tft.setTextSize(FONT_SIZE_MESSAGE); tft.setTextDatum(MC_DATUM);
        int center_y = UI_HEADER_HEIGHT + (tft.height() - UI_HEADER_HEIGHT) / 3; // Um pouco mais para cima

        tft.drawString(getText(StringID::STR_CONFIRM_ADD_PROMPT), tft.width() / 2, center_y);

        // Mostra nome do serviço a ser adicionado
        tft.setTextColor(COLOR_FG);
        tft.drawString(temp_data.service_name, tft.width() / 2, center_y + 35);

        // Desenha rodapé
        tft.setTextColor(COLOR_DIM_TEXT); tft.setTextSize(FONT_SIZE_SMALL); tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(StringID::STR_FOOTER_CONFIRM_NAV), tft.width() / 2, UI_FOOTER_TEXT_Y);

        // Reset
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); tft.setTextSize(1);
     }
     // Nenhum conteúdo dinâmico
}


void ui_drawScreenTimeEditContent(bool full_redraw) {
    // Calcula posições centrais
    int content_y_start = UI_HEADER_HEIGHT;
    int footer_height = tft.fontHeight(FONT_SIZE_SMALL) * 3 + UI_PADDING * 2; // Altura estimada do rodapé + hints
    int content_height = tft.height() - content_y_start - footer_height;
    int content_center_y = content_y_start + content_height / 2;

    // --- Desenho Estático (em full_redraw) ---
    if (full_redraw) {
        // Rodapé e Hints
        tft.setTextColor(COLOR_DIM_TEXT); tft.setTextSize(FONT_SIZE_SMALL);
        // Instrução Principal
        tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(StringID::STR_FOOTER_TIME_EDIT_NAV), tft.width() / 2, UI_FOOTER_TEXT_Y);
        // Info Fuso Horário
        char tz_info_buf[40];
        snprintf(tz_info_buf, sizeof(tz_info_buf), getText(StringID::STR_TIME_EDIT_INFO_FMT), gmt_offset_hours);
        tft.drawString(tz_info_buf, tft.width() / 2, UI_FOOTER_TEXT_Y - tft.fontHeight(FONT_SIZE_SMALL) - 2);
        // Hint JSON
         tft.drawString(getText(StringID::STR_TIME_EDIT_JSON_HINT), tft.width() / 2, UI_FOOTER_TEXT_Y - 2*(tft.fontHeight(FONT_SIZE_SMALL) + 2));
         tft.drawString(getText(StringID::STR_EXAMPLE_JSON_TIME), tft.width() / 2, UI_FOOTER_TEXT_Y - 3*(tft.fontHeight(FONT_SIZE_SMALL) + 2));

        tft.setTextDatum(TL_DATUM); // Reset
    }

    // --- Desenho Dinâmico (Hora e Marcador - sempre redesenhado) ---
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(FONT_SIZE_TIME_EDIT);
    int text_height = tft.fontHeight(FONT_SIZE_TIME_EDIT);
    int time_y_pos = content_center_y; // Centraliza verticalmente

    // Calcula geometria dos campos de hora
    int field_width = tft.textWidth("00", FONT_SIZE_TIME_EDIT);
    int separator_width = tft.textWidth(":", FONT_SIZE_TIME_EDIT);
    int total_width = 3 * field_width + 2 * separator_width;
    int start_x = (tft.width() - total_width) / 2;

    // Posição do marcador de campo ativo
    int marker_height = 4; // Altura do sublinhado
    int marker_y = time_y_pos + text_height / 2 + 2; // Abaixo do texto
    int marker_x = start_x;
    if (temp_data.edit_time_field == 1) { // Minuto
        marker_x = start_x + field_width + separator_width;
    } else if (temp_data.edit_time_field == 2) { // Segundo
        marker_x = start_x + 2 * (field_width + separator_width);
    }

    // Limpa área da hora + marcador para evitar sobreposição
    tft.fillRect(start_x - 5, time_y_pos - text_height / 2 - 5, total_width + 10, text_height + marker_height + 10, COLOR_BG);

    // Desenha a hora formatada
    char time_str_buf[12];
    snprintf(time_str_buf, sizeof(time_str_buf), "%02d:%02d:%02d",
             temp_data.edit_hour, temp_data.edit_minute, temp_data.edit_second);
    tft.drawString(time_str_buf, tft.width() / 2, time_y_pos);

    // Desenha o marcador do campo ativo
    tft.fillRect(marker_x, marker_y, field_width, marker_height, COLOR_ACCENT);

    // Reset texto
    tft.setTextDatum(TL_DATUM); tft.setTextSize(1); tft.setTextColor(COLOR_FG, COLOR_BG);
}


void ui_drawScreenServiceDeleteConfirmContent(bool full_redraw) {
     if (full_redraw) {
        // Área limpa
        tft.setTextColor(COLOR_ERROR); tft.setTextSize(FONT_SIZE_MESSAGE); tft.setTextDatum(MC_DATUM);
        int center_y = UI_HEADER_HEIGHT + (tft.height() - UI_HEADER_HEIGHT) / 3; // Mais para cima

        tft.drawString(getText(StringID::STR_CONFIRM_DELETE_PROMPT), tft.width() / 2, center_y);

        // Mostra nome do serviço a ser deletado
        tft.setTextColor(COLOR_FG);
        if (service_count > 0 && current_service_index >= 0 && current_service_index < service_count) {
            tft.drawString(services[current_service_index].name, tft.width() / 2, center_y + 35);
        } else {
            tft.drawString("???", tft.width() / 2, center_y + 35); // Fallback
        }

        // Rodapé
        tft.setTextColor(COLOR_DIM_TEXT); tft.setTextSize(FONT_SIZE_SMALL); tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(StringID::STR_FOOTER_CONFIRM_NAV), tft.width() / 2, UI_FOOTER_TEXT_Y);

        // Reset
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); tft.setTextSize(1);
     }
     // Nenhum conteúdo dinâmico
}


void ui_drawScreenTimezoneEditContent(bool full_redraw) {
    int content_y_start = UI_HEADER_HEIGHT;
    int footer_height = tft.fontHeight(FONT_SIZE_SMALL) + UI_PADDING;
    int content_height = tft.height() - content_y_start - footer_height;
    int content_center_y = content_y_start + content_height / 2;

    if (full_redraw) {
        // Rodapé
        tft.setTextColor(COLOR_DIM_TEXT); tft.setTextSize(FONT_SIZE_SMALL); tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(StringID::STR_FOOTER_TIMEZONE_NAV), tft.width() / 2, UI_FOOTER_TEXT_Y);
        tft.setTextDatum(TL_DATUM); // Reset
    }

    // --- Desenho Dinâmico (Valor do Fuso - sempre redesenhado) ---
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_FG, COLOR_BG);
    tft.setTextSize(FONT_SIZE_TIME_EDIT); // Fonte grande
    int text_h = tft.fontHeight(FONT_SIZE_TIME_EDIT);

    // Limpa área do texto
    tft.fillRect(0, content_center_y - text_h/2 - 5, tft.width(), text_h + 10, COLOR_BG);

    // Formata e desenha o fuso horário sendo editado
    char tz_str[10];
    snprintf(tz_str, sizeof(tz_str), getText(StringID::STR_TIMEZONE_LABEL), temp_data.edit_gmt_offset);
    tft.drawString(tz_str, tft.width() / 2, content_center_y);

    // Reset texto
    tft.setTextDatum(TL_DATUM); tft.setTextSize(1);
}


void ui_drawScreenLanguageSelectContent(bool full_redraw) {
    MenuState& menu = lang_menu_state; // Atalho
    int item_y_start = UI_MENU_START_Y;
    int item_spacing = UI_MENU_ITEM_SPACING;
    int item_total_height = UI_MENU_ITEM_HEIGHT + item_spacing;

    // Cálculo inicial do highlight
    if (menu.highlight_y_current == -1 && NUM_LANGUAGES > 0) {
         int initial_visible_pos = menu.current_index - menu.top_visible_index;
         menu.highlight_y_current = item_y_start + initial_visible_pos * item_total_height;
         menu.highlight_y_target = menu.highlight_y_current;
    }

    // Limpeza e Desenho
    int scrollbar_x = tft.width() - UI_PADDING - UI_MENU_SCROLLBAR_WIDTH;
    int menu_items_width = scrollbar_x - UI_PADDING * 2;

    if (full_redraw) {
        tft.fillRect(0, UI_HEADER_HEIGHT, tft.width(), tft.height() - UI_HEADER_HEIGHT, COLOR_BG);
        // Rodapé
        tft.setTextColor(COLOR_DIM_TEXT, COLOR_BG); tft.setTextSize(FONT_SIZE_SMALL);
        tft.setTextDatum(BC_DATUM);
        tft.drawString(getText(StringID::STR_FOOTER_LANG_NAV), tft.width() / 2, UI_FOOTER_TEXT_Y);
        tft.setTextDatum(TL_DATUM);
    } else {
        // Limpeza parcial
        tft.fillRect(UI_PADDING, item_y_start, menu_items_width, VISIBLE_MENU_ITEMS * item_total_height, COLOR_BG);
    }

    // Desenha Highlight
     if (menu.highlight_y_current != -1 && NUM_LANGUAGES > 0) {
        tft.fillRect(UI_PADDING, menu.highlight_y_current, menu_items_width, UI_MENU_ITEM_HEIGHT, COLOR_HIGHLIGHT_BG);
     }

    // Desenha Itens Visíveis
    tft.setTextSize(FONT_SIZE_MENU_ITEM);
    tft.setTextDatum(ML_DATUM);
    int draw_count = 0;
    for (int i = menu.top_visible_index; i < NUM_LANGUAGES && draw_count < VISIBLE_MENU_ITEMS; ++i) {
        int current_item_draw_y = item_y_start + draw_count * item_total_height;
        bool is_selected = (i == menu.current_index);
        bool is_active = ((Language)i == current_language);

        // Define cor do texto
        tft.setTextColor(is_selected ? COLOR_HIGHLIGHT_FG : COLOR_FG, is_selected ? COLOR_HIGHLIGHT_BG : COLOR_BG); // BG transparente

        // Desenha nome do idioma
        tft.drawString(getLanguageNameByIndex((Language)i), UI_PADDING * 3, current_item_draw_y + UI_MENU_ITEM_HEIGHT / 2);

        // Adiciona marcador '*' se for o idioma ATUALMENTE ATIVO (e não selecionado)
        if (is_active && !is_selected) {
            uint16_t prev_fg = tft.textcolor;
            uint16_t prev_bg = tft.textbgcolor;
            tft.setTextColor(COLOR_ACCENT, COLOR_BG); // Cor do marcador
            tft.drawString("*", menu_items_width - UI_PADDING, current_item_draw_y + UI_MENU_ITEM_HEIGHT / 2); // Desenha à direita
            tft.setTextColor(prev_fg, prev_bg); // Restaura cores
        }
        draw_count++;
    }
    // tft.setTextbgcolor(COLOR_BG); // Reseta BG

    // Barra de Rolagem (se necessário)
    if (NUM_LANGUAGES > VISIBLE_MENU_ITEMS) {
       int scrollbar_h = VISIBLE_MENU_ITEMS * item_total_height - item_spacing;
       tft.fillRect(scrollbar_x, item_y_start, UI_MENU_SCROLLBAR_WIDTH, scrollbar_h, COLOR_BAR_BG);
       int thumb_h = max(5, scrollbar_h * VISIBLE_MENU_ITEMS / NUM_LANGUAGES);
       int thumb_max_y = scrollbar_h - thumb_h;
       int thumb_y = item_y_start;
       if (NUM_LANGUAGES - VISIBLE_MENU_ITEMS > 0) {
          thumb_y += round((float)thumb_max_y * menu.top_visible_index / (NUM_LANGUAGES - VISIBLE_MENU_ITEMS));
       }
       thumb_y = constrain(thumb_y, item_y_start, item_y_start + thumb_max_y);
       tft.fillRect(scrollbar_x, thumb_y, UI_MENU_SCROLLBAR_WIDTH, thumb_h, COLOR_ACCENT);
    }

    // Reset Texto
    tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); tft.setTextSize(1);
}


void ui_drawScreenReadRFIDContent(bool full_redraw) {
     if (full_redraw) {
        // Área limpa
        tft.setTextColor(COLOR_FG, COLOR_BG);
        tft.setTextSize(FONT_SIZE_MESSAGE);
        tft.setTextDatum(MC_DATUM);
        int center_y = UI_HEADER_HEIGHT + (tft.height() - UI_HEADER_HEIGHT) / 2;

        // Mostra o prompt ou o ID lido
        if (temp_data.rfid_card_id[0] == '\0') {
            tft.drawString(getText(StringID::STR_RFID_PROMPT), tft.width() / 2, center_y);
        } else {
            char buffer[30];
            snprintf(buffer, sizeof(buffer), getText(StringID::STR_CARD_READ_FMT), temp_data.rfid_card_id);
             // Limpa área antes de desenhar o ID
             tft.fillRect(0, center_y - tft.fontHeight(FONT_SIZE_MESSAGE), tft.width(), tft.fontHeight(FONT_SIZE_MESSAGE)*2, COLOR_BG);
            tft.drawString(buffer, tft.width() / 2, center_y);
        }

         // Rodapé (opcional, pode ser útil ter instrução para voltar)
         tft.setTextColor(COLOR_DIM_TEXT); tft.setTextSize(FONT_SIZE_SMALL); tft.setTextDatum(BC_DATUM);
         tft.drawString(getText(StringID::STR_FOOTER_GENERIC_NAV), tft.width()/2, UI_FOOTER_TEXT_Y);

        // Reset
        tft.setTextDatum(TL_DATUM); tft.setTextColor(COLOR_FG, COLOR_BG); tft.setTextSize(1);
     }
     // Conteúdo é atualizado apenas quando um cartão é lido (via request_full_redraw = true)
}

void ui_drawScreenMessage(bool full_redraw) {
     // Esta tela é sempre redesenhada completamente quando ativada
     if (full_redraw) {
         tft.fillScreen(COLOR_BG); // Limpa TUDO (sem header)
         tft.setTextColor(message_color, COLOR_BG);
         tft.setTextSize(FONT_SIZE_MESSAGE);
         tft.setTextDatum(MC_DATUM);

         // Desenha mensagem (com suporte simples a quebra de linha \n)
         char temp_msg_buffer[sizeof(message_buffer)]; // Cria cópia para strtok
         strncpy(temp_msg_buffer, message_buffer, sizeof(temp_msg_buffer));
         temp_msg_buffer[sizeof(temp_msg_buffer)-1] = '\0';

         char *line1 = strtok(temp_msg_buffer, "\n");
         char *line2 = strtok(NULL, "\n"); // Pega segunda linha se houver

         int y_pos = tft.height() / 2;
         int line_height = tft.fontHeight(FONT_SIZE_MESSAGE) + 5;

         if (line1 && line2) { // Duas linhas
             y_pos -= line_height / 2; // Ajusta Y para centralizar as duas
             tft.drawString(line1, tft.width() / 2, y_pos);
             tft.drawString(line2, tft.width() / 2, y_pos + line_height);
         } else if (line1) { // Apenas uma linha
             tft.drawString(line1, tft.width() / 2, y_pos);
         }

         // Reset
         tft.setTextDatum(TL_DATUM);
         tft.setTextColor(COLOR_FG, COLOR_BG);
         tft.setTextSize(1);
     }
}