#include <ArduinoJson.h>
#include <TimeLib.h>
#include "globals.h"
#include "types.h"
#include "ui.h"
#include "totp.h"
#include "i18n.h"
#include "hardware.h"


// ---- Callbacks dos Botões ----
void btn_prev_click() {
    last_interaction_time = millis(); // Reseta inatividade
    bool needs_full_redraw = false; // Flag para redesenho completo da tela atual

    switch (current_screen) {
        case SCREEN_TOTP_VIEW:
            if (service_count > 0) {
                current_service_index = (current_service_index - 1 + service_count) % service_count; // Volta para serviço anterior
                if (!decodeCurrentServiceKey()) { 
                    // Decodificação falhou, exibe mensagem de erro
                    ui_showTemporaryMessage(getText(StringID::STR_ERROR_B32_DECODE), COLOR_ERROR);
                }
                // Sempre atualiza a tela TOTP, independentemente do resultado
                changeScreen(SCREEN_TOTP_VIEW);
            }
            // Não precisa de redraw extra aqui, changeScreen cuida
            break;
        case SCREEN_MENU_MAIN:
            if (NUM_MENU_OPTIONS > 0) {
                int old_visible_pos = current_menu_index - menu_top_visible_index;
                int old_highlight_y = UI_MENU_START_Y + old_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
                current_menu_index = (current_menu_index - 1 + NUM_MENU_OPTIONS) % NUM_MENU_OPTIONS;

                // Lógica de rolagem para cima
                if (current_menu_index < menu_top_visible_index) {
                    menu_top_visible_index = current_menu_index;
                } else if (current_menu_index == NUM_MENU_OPTIONS - 1 && menu_top_visible_index > 0) {
                     menu_top_visible_index = max(0, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS); // Mostra últimos itens
                }
                menu_top_visible_index = max(0, menu_top_visible_index); // Garante não negativo

                // Calcula posição alvo e inicia animação
                int new_visible_pos = current_menu_index - menu_top_visible_index;
                int new_highlight_y = UI_MENU_START_Y + new_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
                if (menu_highlight_y_current != new_highlight_y) {
                     if (menu_highlight_y_current == -1) menu_highlight_y_current = old_highlight_y;
                     menu_highlight_y_target = new_highlight_y;
                     menu_animation_start_time = millis();
                     is_menu_animating = true;
                }
            }
            // O loop cuidará da atualização da animação
            break;
        case SCREEN_TIME_EDIT: // Decrementa valor do campo
            if(edit_time_field == 0) edit_hour = (edit_hour - 1 + 24) % 24;
            else if(edit_time_field == 1) edit_minute = (edit_minute - 1 + 60) % 60;
            else if(edit_time_field == 2) edit_second = (edit_second - 1 + 60) % 60;
            needs_full_redraw = true; // Precisa redesenhar a tela de edição
            break;
        case SCREEN_TIMEZONE_EDIT: // Decrementa offset GMT
            gmt_offset_hours = (gmt_offset_hours - 1 < -12) ? 14 : gmt_offset_hours - 1; // Wrap -12 a +14
            needs_full_redraw = true; // Precisa redesenhar a tela de fuso
            break;
        case SCREEN_LANGUAGE_SELECT: // Navega para idioma anterior
            current_language_menu_index = (current_language_menu_index - 1 + NUM_LANGUAGES) % NUM_LANGUAGES;
            needs_full_redraw = true; // Precisa redesenhar a lista de idiomas
            break;
        case SCREEN_SERVICE_DELETE_CONFIRM: // Cancela exclusão
        case SCREEN_SERVICE_ADD_CONFIRM:    // Cancela adição
            changeScreen(SCREEN_MENU_MAIN); // Volta para o menu
            // changeScreen já faz o redraw
            break;
        default: break; // Nenhuma ação padrão
    }
    // Redesenha a tela atual completamente se a flag foi setada
    if (needs_full_redraw) ui_drawScreen(true);
}

void btn_next_click() {
    last_interaction_time = millis();
    bool needs_full_redraw = false;

    switch (current_screen) {
        case SCREEN_TOTP_VIEW:
            if (service_count > 0) {
                current_service_index = (current_service_index + 1) % service_count; // Avança para próximo serviço
                if (!decodeCurrentServiceKey()) { 
                    // Decodificação falhou, exibe mensagem de erro
                    ui_showTemporaryMessage(getText(StringID::STR_ERROR_B32_DECODE), COLOR_ERROR);
                }
                changeScreen(SCREEN_TOTP_VIEW); // Força redraw com novo título/código ou com erro
            }
            break;
        case SCREEN_MENU_MAIN:
             if (NUM_MENU_OPTIONS > 0) {
                 int old_visible_pos = current_menu_index - menu_top_visible_index;
                 int old_highlight_y = UI_MENU_START_Y + old_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
                 current_menu_index = (current_menu_index + 1) % NUM_MENU_OPTIONS;

                 // Lógica de rolagem para baixo
                 if (current_menu_index >= menu_top_visible_index + VISIBLE_MENU_ITEMS) {
                     menu_top_visible_index = current_menu_index - VISIBLE_MENU_ITEMS + 1;
                 } else if (current_menu_index == 0 && menu_top_visible_index != 0) {
                     menu_top_visible_index = 0; // Wrap around
                 }
                 menu_top_visible_index = min(menu_top_visible_index, max(0, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS)); // Garante limite

                 // Calcula posição alvo e inicia animação
                 int new_visible_pos = current_menu_index - menu_top_visible_index;
                 int new_highlight_y = UI_MENU_START_Y + new_visible_pos * (UI_MENU_ITEM_HEIGHT + 5);
                 if (menu_highlight_y_current != new_highlight_y) {
                     if (menu_highlight_y_current == -1) menu_highlight_y_current = old_highlight_y;
                     menu_highlight_y_target = new_highlight_y;
                     menu_animation_start_time = millis();
                     is_menu_animating = true;
                 }
             }
             break; // Loop cuida da animação
        case SCREEN_TIME_EDIT: // Incrementa valor do campo
            if(edit_time_field == 0) edit_hour = (edit_hour + 1) % 24;
            else if(edit_time_field == 1) edit_minute = (edit_minute + 1) % 60;
            else if(edit_time_field == 2) edit_second = (edit_second + 1) % 60;
            needs_full_redraw = true;
            break;
        case SCREEN_TIMEZONE_EDIT: // Incrementa offset GMT
            gmt_offset_hours = (gmt_offset_hours + 1 > 14) ? -12 : gmt_offset_hours + 1; // Wrap -12 a +14
            needs_full_redraw = true;
            break;
        case SCREEN_LANGUAGE_SELECT: // Navega para próximo idioma
            current_language_menu_index = (current_language_menu_index + 1) % NUM_LANGUAGES;
            needs_full_redraw = true;
            break;
        case SCREEN_SERVICE_DELETE_CONFIRM: // Confirma exclusão
            if(storage_deleteService(current_service_index)) {
                ui_showTemporaryMessage(getText(STR_SERVICE_DELETED), COLOR_SUCCESS);
                // A mensagem chamará changeScreen para TOTP_VIEW ou MENU
            } else {
                ui_showTemporaryMessage(getText(STR_ERROR_DELETING), COLOR_ERROR);
                // A mensagem chamará changeScreen para MENU
            }
            break;
        case SCREEN_SERVICE_ADD_CONFIRM: // Confirma adição
             if(storage_saveService(temp_service_name, temp_service_secret)){
                 current_service_index = service_count - 1; // Seleciona o recém-adicionado
                 decodeCurrentServiceKey(); // Decodifica chave
                 ui_showTemporaryMessage(getText(STR_SERVICE_ADDED), COLOR_SUCCESS); // Mostra sucesso
                 // A mensagem chamará changeScreen(SCREEN_MENU_MAIN), precisamos ir para TOTP
                 // TODO: Melhorar lógica pós-mensagem para ir para tela correta
                 // Por enquanto, a mensagem sempre volta ao menu. O usuário pode navegar para TOTP.
             } else {
                 // Erro ao salvar já mostra mensagem em storage_saveService
                 changeScreen(SCREEN_MENU_MAIN); // Volta ao menu em caso de erro
             }
             break;
        default: break;
    }
    if (needs_full_redraw) ui_drawScreen(true);
}

void btn_next_double_click() {
    last_interaction_time = millis();
    if (current_screen != SCREEN_MENU_MAIN) {
        changeScreen(SCREEN_MENU_MAIN); // Double click sempre volta ao menu
    }
}

void btn_next_long_press_start() {
    last_interaction_time = millis();
    bool change_screen_handled = false; // Flag para controle de redraw/changeScreen

    switch(current_screen) {
        case SCREEN_TOTP_VIEW:
            if (service_count > 0) {
                changeScreen(SCREEN_SERVICE_DELETE_CONFIRM);
                change_screen_handled = true;
            }
            break;
        case SCREEN_MENU_MAIN:
            change_screen_handled = true; // Ação do menu sempre lida com a tela
            switch(menuOptionIDs[current_menu_index]){ // Seleciona ação baseada no ID
                case STR_MENU_ADD_SERVICE:      changeScreen(SCREEN_SERVICE_ADD_WAIT); break;
                case STR_MENU_READ_RFID:        changeScreen(SCREEN_READ_RFID); break;
                case STR_MENU_VIEW_CODES:       if(service_count > 0) changeScreen(SCREEN_TOTP_VIEW); else ui_showTemporaryMessage(getText(STR_ERROR_NO_SERVICES), COLOR_ACCENT, 2000); break;
                case STR_MENU_ADJUST_TIME:      edit_hour=hour(); edit_minute=minute(); edit_second=second(); edit_time_field=0; changeScreen(SCREEN_TIME_EDIT); break;
                case STR_MENU_ADJUST_TIMEZONE:  changeScreen(SCREEN_TIMEZONE_EDIT); break;
                case STR_MENU_SELECT_LANGUAGE:  current_language_menu_index = current_language; changeScreen(SCREEN_LANGUAGE_SELECT); break;
            }
            break;
        case SCREEN_TIME_EDIT: // Avança campo ou salva
            edit_time_field++;
            if (edit_time_field > 2) { // Passou dos segundos, salvar
                setTime(edit_hour, edit_minute, edit_second, day(), month(), year()); // Salva UTC
                updateRTCFromSystem(); // Atualiza RTC
                time_t local_t = now() + gmt_offset_hours * 3600; // Calcula hora local para msg
                snprintf(message_buffer, sizeof(message_buffer), getText(STR_TIME_ADJUSTED_FMT), hour(local_t), minute(local_t), second(local_t));
                ui_showTemporaryMessage(message_buffer, COLOR_SUCCESS);
                change_screen_handled = true; // Mensagem cuida da transição
            } else {
                ui_drawScreen(true); // Apenas redesenha para mostrar novo campo ativo
            }
            break;
        case SCREEN_TIMEZONE_EDIT: // Salva fuso horário
            preferences.begin("totp-app", false); // Abre NVS para escrita
            preferences.putInt(NVS_KEY_TZ_OFFSET, gmt_offset_hours);
            preferences.end();
            Serial.printf("[NVS] Fuso salvo: GMT%+d\n", gmt_offset_hours);
            snprintf(message_buffer, sizeof(message_buffer), getText(STR_TIMEZONE_SAVED_FMT), gmt_offset_hours);
            ui_showTemporaryMessage(message_buffer, COLOR_SUCCESS);
            change_screen_handled = true; // Mensagem cuida da transição
            break;
        case SCREEN_LANGUAGE_SELECT: // Salva idioma selecionado
             change_screen_handled = true; // Sempre lida com a tela
             if (current_language_menu_index != current_language) { // Se mudou
                 current_language = (Language)current_language_menu_index;
                 //current_strings = languages[current_language]; // Atualiza ponteiro global
                 preferences.begin("totp-app", false);
                 preferences.putInt(NVS_KEY_LANGUAGE, (int)current_language);
                 preferences.end();
                 Serial.printf("[LANG] Idioma salvo: %d\n", current_language);
                 // Mostra msg de sucesso e força redraw completo da tela anterior (menu)
                 // A função de mensagem chamará changeScreen(SCREEN_MENU_MAIN)
                 ui_showTemporaryMessage(getText(STR_LANG_SAVED), COLOR_SUCCESS);

             } else {
                 changeScreen(SCREEN_MENU_MAIN); // Volta se não mudou
             }
             break;
        default: break;
    }
    // Evita redesenho extra desnecessário se uma ação já mudou a tela ou mostrou mensagem
    // if (!change_screen_handled) ui_drawScreen(true);
}


// ---- Funções de Entrada Serial ----
void processSerialInput() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() == 0) return; // Ignora linhas vazias

        Serial.printf("[SERIAL] RX: %s\n", input.c_str());
        last_interaction_time = millis(); // Considera entrada serial como interação

        // Tenta parsear o JSON
        StaticJsonDocument<256> doc; // Ajustar tamanho se necessário
        DeserializationError error = deserializeJson(doc, input);

        if (error) {
            // Mostra erro de parsing
            snprintf(message_buffer, sizeof(message_buffer), getText(STR_ERROR_JSON_PARSE_FMT), error.c_str());
            ui_showTemporaryMessage(message_buffer, COLOR_ERROR);
            // Volta ao menu em caso de erro nas telas de entrada
            if (current_screen == SCREEN_SERVICE_ADD_WAIT || current_screen == SCREEN_TIME_EDIT) {
                changeScreen(SCREEN_MENU_MAIN);
            }
            return;
        }

        // Delega o processamento baseado na tela atual
        if (current_screen == SCREEN_SERVICE_ADD_WAIT) {
            processServiceAdd(doc);
        } else if (current_screen == SCREEN_TIME_EDIT) {
            processTimeSet(doc);
        }
    }
}

void processServiceAdd(JsonDocument &doc) {
    // Verifica se os campos obrigatórios existem e são do tipo correto
    if(!doc.containsKey("name") || !doc["name"].is<const char*>() || !doc.containsKey("secret") || !doc["secret"].is<const char*>()) {
        ui_showTemporaryMessage(getText(STR_ERROR_JSON_INVALID_SERVICE), COLOR_ERROR);
        changeScreen(SCREEN_MENU_MAIN); return;
    }
    const char* name = doc["name"];
    const char* secret = doc["secret"];

    // Valida comprimento dos dados
    if(strlen(name) == 0 || strlen(name) > MAX_SERVICE_NAME_LEN){
        ui_showTemporaryMessage(getText(STR_ERROR_SERVICE_NAME_INVALID), COLOR_ERROR);
        changeScreen(SCREEN_MENU_MAIN); return;
    }
    if(strlen(secret) == 0 || strlen(secret) > MAX_SECRET_B32_LEN){
        ui_showTemporaryMessage(getText(STR_ERROR_SECRET_INVALID), COLOR_ERROR);
        changeScreen(SCREEN_MENU_MAIN); return;
    }

    // TODO: Adicionar validação dos caracteres do segredo Base32 se desejado

    // Copia dados válidos para variáveis temporárias e vai para confirmação
    strncpy(temp_service_name, name, sizeof(temp_service_name) - 1); temp_service_name[sizeof(temp_service_name) - 1] = '\0';
    strncpy(temp_service_secret, secret, sizeof(temp_service_secret) - 1); temp_service_secret[sizeof(temp_service_secret) - 1] = '\0';
    changeScreen(SCREEN_SERVICE_ADD_CONFIRM);
}

void processTimeSet(JsonDocument &doc) {
    // Verifica campos de data e hora
    if(!doc.containsKey("year")||!doc["year"].is<int>()||!doc.containsKey("month")||!doc["month"].is<int>()||
       !doc.containsKey("day")||!doc["day"].is<int>()||!doc.containsKey("hour")||!doc["hour"].is<int>()||
       !doc.containsKey("minute")||!doc["minute"].is<int>()||!doc.containsKey("second")||!doc["second"].is<int>()) {
        ui_showTemporaryMessage(getText(STR_ERROR_JSON_INVALID_TIME), COLOR_ERROR);
        changeScreen(SCREEN_MENU_MAIN); return;
    }
    int y = doc["year"], m = doc["month"], d = doc["day"], h = doc["hour"], mn = doc["minute"], s = doc["second"];

    // Valida os intervalos dos valores
    if(y < 2023 || y > 2100 || m < 1 || m > 12 || d < 1 || d > 31 || h < 0 || h > 23 || mn < 0 || mn > 59 || s < 0 || s > 59) {
        ui_showTemporaryMessage(getText(STR_ERROR_TIME_VALUES_INVALID), COLOR_ERROR);
        changeScreen(SCREEN_MENU_MAIN); return;
    }

    // Se tudo ok, define o tempo do sistema (UTC) e atualiza o RTC
    setTime(h, mn, s, d, m, y);
    updateRTCFromSystem();

    // Mostra mensagem de sucesso com a hora LOCAL ajustada
    time_t local_adjusted_time = now() + gmt_offset_hours * 3600;
    snprintf(message_buffer, sizeof(message_buffer), getText(STR_TIME_ADJUSTED_FMT), hour(local_adjusted_time), minute(local_adjusted_time), second(local_adjusted_time));
    ui_showTemporaryMessage(message_buffer, COLOR_SUCCESS);
    // changeScreen(SCREEN_MENU_MAIN); // Chamado pela função de mensagem
}


void configureButtonCallbacks(){
    btn_prev.attachClick(btn_prev_click);
    btn_next.attachClick(btn_next_click);
    btn_next.attachDoubleClick(btn_next_double_click);
    btn_next.attachLongPressStart(btn_next_long_press_start);
}