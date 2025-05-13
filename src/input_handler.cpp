// input_handler.cpp
#include "input_handler.h"
#include "globals.h"        // Acesso a botões, estado atual, índices, etc.
#include "config.h"         // Constantes (MAX_SERVICES, JSON sizes)
#include "ui_manager.h"     // Para ui_change_screen, ui_queue_message*
#include "service_storage.h"// Para service_storage_add_from_json, service_storage_delete
#include "time_keeper.h"    // Para time_set_manual, time_save_manual_to_rtc, time_set_from_json, time_adjust_gmt_offset, time_save_gmt_offset
#include "i18n.h"           // Para i18n_cycle_language_selection, i18n_save_selected_language, getText
#include "totp_core.h"      // Para totp_decode_current_service_key
#include "rfid_reader.h"    // Para rfid_read_card (embora a leitura possa ser movida para o loop principal)
#include <ArduinoJson.h>    // Para parsing de JSON
#include <Arduino.h>        // Para Serial, millis

// Protótipos dos callbacks dos botões (static para limitar escopo a este arquivo)
static void btn_prev_click();
static void btn_next_click();
static void btn_next_double_click();
static void btn_next_long_press_start();

// ---- Inicialização ----
void input_init() {
    btn_prev.attachClick(btn_prev_click);
    btn_next.attachClick(btn_next_click);
    btn_next.attachDoubleClick(btn_next_double_click);
    btn_next.attachLongPressStart(btn_next_long_press_start);
    Serial.println("[Input] Callbacks dos botões anexados.");
}

// ---- Processamento Principal de Entradas ----
void input_tick() {
    // Processa eventos dos botões
    btn_prev.tick();
    btn_next.tick();

    // Processa entrada Serial apenas em telas que a esperam
    if (current_screen == ScreenState::SCREEN_SERVICE_ADD_WAIT || current_screen == ScreenState::SCREEN_TIME_EDIT) {
        input_process_serial();
    }

    // Dentro de input_handler.cpp -> input_tick()
    // Gerenciamento da tela de mensagem (timer)
    if (current_screen == ScreenState::SCREEN_MESSAGE) {
        static uint32_t message_start_time = 0; 
        static bool timer_started_for_this_message = false; // Flag auxiliar

        // Inicia o timer apenas uma vez por mensagem
        if (!timer_started_for_this_message) {
             message_start_time = millis();
             timer_started_for_this_message = true; // Marca que o timer iniciou
             // Serial.printf("[UI Message] Timer iniciado para %lu ms.\n", message_duration_ms);
        }

        // Verifica se a duração da mensagem expirou E se a duração é válida
        if (message_duration_ms > 0 && (millis() - message_start_time >= message_duration_ms)) {
            // Serial.println("[UI Message] Timer expirou.");
            ScreenState next = message_next_screen; // Guarda a próxima tela
            // --- IMPORTANTE: Zera a flag ANTES de mudar de tela ---
            timer_started_for_this_message = false;
            // --- Zera a duração APÓS guardar a próxima tela, mas ANTES de chamar change_screen
            // --- para que a próxima verificação não entre aqui novamente por engano.
            message_duration_ms = 0;
            // --- Muda para a próxima tela ---
            ui_change_screen(next, true);
            // NÃO zere message_duration_ms aqui novamente.
        }

        // Verifica se a duração da mensagem expirou E se a duração não foi zerada por um clique
        if (message_duration_ms > 0 && (millis() - message_start_time >= message_duration_ms)) {
            // Serial.println("[UI Message] Timer expirou.");
            ui_change_screen(message_next_screen, true); // Muda para a próxima tela
            message_duration_ms = 0; // Reseta para evitar re-trigger imediato
        }
    } else {
        // Se não estamos na tela de mensagem, garante que a flag do timer seja resetada
        // para a próxima vez que entrarmos na tela de mensagem.
        // static bool timer_started_for_this_message = false; // Declaração movida para dentro do if
        // A linha acima não é necessária aqui se a declaração static estiver dentro do if.
        // Se a declaração static for fora do if, descomente a linha abaixo:
        // timer_started_for_this_message = false;
        // -> A melhor abordagem é manter a declaração static DENTRO do if.
    }
}

// ---- Processamento de Entrada Serial ----
void input_process_serial() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() == 0) return;

        Serial.printf("[Input] Serial RX: %s\n", input.c_str());
        last_interaction_time = millis(); // Entrada serial conta como interação

        // Determina o tamanho do documento JSON com base na tela
        size_t json_doc_size = (current_screen == ScreenState::SCREEN_SERVICE_ADD_WAIT)
                               ? JSON_DOC_SIZE_SERVICE
                               : JSON_DOC_SIZE_TIME;
        DynamicJsonDocument doc(json_doc_size); // Usa Dynamic para flexibilidade, mas Static é mais eficiente em memória se o tamanho for bem definido

        DeserializationError error = deserializeJson(doc, input);

        if (error) {
            char error_buf[80];
            snprintf(error_buf, sizeof(error_buf), getText(StringID::STR_ERROR_JSON_PARSE_FMT), error.c_str());
            ui_queue_message(error_buf, COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN); // Mostra erro e volta ao menu
            return;
        }

        // Delega o processamento baseado na tela atual
        if (current_screen == ScreenState::SCREEN_SERVICE_ADD_WAIT) {
            // Tenta adicionar o serviço a partir do JSON. A função interna
            // cuidará da validação, cópia para temp_vars e mudança para tela de confirmação.
            service_storage_prepare_add_from_json(doc);
        } else if (current_screen == ScreenState::SCREEN_TIME_EDIT) {
            // Tenta ajustar a hora a partir do JSON. A função interna
            // cuidará da validação, ajuste do TimeLib/RTC e exibição de mensagem.
            time_set_from_json(doc);
        }
    }
}


// ---- Callbacks dos Botões ----

static void btn_prev_click() {
    if (current_screen == ScreenState::SCREEN_MESSAGE) {
        ScreenState next = message_next_screen;
        // --- Resetar estado da mensagem antes de mudar ---
        message_duration_ms = 0;
        // timer_started_for_this_message = false; // Resetar a flag do timer também
        // -> Para resetar a flag static, precisamos acessá-la. É mais fácil
        // -> deixar a lógica em input_tick() resetá-la quando sair da tela de msg.
        // -> Apenas zerar a duração é suficiente para parar o timer.
        // --- Fim do reset ---
        ui_change_screen(next, true);
        last_interaction_time = millis();
        return;
    }

    last_interaction_time = millis();
    bool needs_redraw = false; // Flag se a ação requer redesenho parcial/imediato

    switch (current_screen) {
        case ScreenState::SCREEN_TOTP_VIEW:
            if (service_count > 0) {
                current_service_index = (current_service_index - 1 + service_count) % service_count;
                if (totp_decode_current_service_key()) {
                    ui_change_screen(ScreenState::SCREEN_TOTP_VIEW); // Redesenho completo com novo serviço
                } else {
                    // Erro ao decodificar já mostra msg via ui_queue_message em totp_core
                    // A tela TOTP mostrará o placeholder de erro. Força redraw.
                    ui_change_screen(ScreenState::SCREEN_TOTP_VIEW);
                }
            }
            break; // ui_change_screen cuida do redraw

        case ScreenState::SCREEN_MENU_MAIN:
            if (NUM_MENU_OPTIONS > 0) {
                int old_menu_index = current_menu_index; // Guarda o índice antes da mudança
                int old_top_visible = menu_top_visible_index; // Guarda o topo visível antes

                current_menu_index = (current_menu_index - 1 + NUM_MENU_OPTIONS) % NUM_MENU_OPTIONS;

                // --- LÓGICA DE ROLAGEM PARA CIMA (REVISADA) ---
                if (old_menu_index == 0 && current_menu_index == NUM_MENU_OPTIONS - 1) {
                    // Caso especial: Estava no primeiro e foi para o último (wrap around para cima)
                    // Queremos que o último item fique visível, idealmente na última posição da janela.
                    menu_top_visible_index = max(0, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS);
                } else if (current_menu_index < menu_top_visible_index) {
                    // O item selecionado está acima da janela visível, rola para cima
                    menu_top_visible_index = current_menu_index;
                }
                // Não precisa de mais lógica aqui, pois se o item já está visível,
                // menu_top_visible_index não deve mudar, a menos que seja o wrap around.
                // A linha menu_top_visible_index = max(0, menu_top_visible_index); já garante não ser negativo.

                // Garante que menu_top_visible_index não seja negativo e não exceda o limite máximo
                menu_top_visible_index = max(0, menu_top_visible_index);
                if (NUM_MENU_OPTIONS > VISIBLE_MENU_ITEMS) { // Só limita se houver mais itens que o visível
                    menu_top_visible_index = min(menu_top_visible_index, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS);
                } else {
                    menu_top_visible_index = 0; // Se todos os itens cabem, o topo é sempre 0
                }


                // --- LÓGICA DE ANIMAÇÃO (EXISTENTE, MAS VERIFICAR POSIÇÃO INICIAL) ---
                // Calcula a posição Y do item que estava selecionado ANTES da mudança,
                // relativo ao menu_top_visible_index ANTES da mudança.
                int old_visible_pos_rel_to_old_top = old_menu_index - old_top_visible;
                int old_highlight_y = UI_MENU_START_Y + old_visible_pos_rel_to_old_top * (UI_MENU_ITEM_HEIGHT + UI_MENU_ITEM_SPACING);

                // Calcula a nova posição Y do item AGORA selecionado,
                // relativo ao NOVO menu_top_visible_index.
                int new_visible_pos_rel_to_new_top = current_menu_index - menu_top_visible_index;
                int new_highlight_y = UI_MENU_START_Y + new_visible_pos_rel_to_new_top * (UI_MENU_ITEM_HEIGHT + UI_MENU_ITEM_SPACING);

                if (menu_highlight_y_current != new_highlight_y || old_top_visible != menu_top_visible_index) { // Anima se o Y ou o topo mudou
                     if (menu_highlight_y_current == -1 || old_top_visible != menu_top_visible_index) {
                         // Se o topo mudou, ou é a primeira vez, a "posição antiga" para animação
                         // deve ser recalculada ou forçada para uma transição suave.
                         // Para o wrap around, a animação pode ser de uma posição "fora da tela".
                         // Vamos simplificar: se o topo mudou, a animação começa da borda.
                         if (old_menu_index == 0 && current_menu_index == NUM_MENU_OPTIONS - 1) { // Wrap para cima
                            menu_highlight_y_anim_start = UI_MENU_START_Y - (UI_MENU_ITEM_HEIGHT + UI_MENU_ITEM_SPACING); // "Acima" da primeira posição
                         } else if (current_menu_index < old_menu_index) { // Movendo para cima
                            menu_highlight_y_anim_start = old_highlight_y;
                         } else { // Caso inesperado, usa a posição atual
                            menu_highlight_y_anim_start = new_highlight_y;
                         }
                     } else {
                        menu_highlight_y_anim_start = menu_highlight_y_current;
                     }

                     menu_highlight_y_target = new_highlight_y;
                     menu_animation_start_time = millis();
                     is_menu_animating = true;
                }
            }
            break; // Animação cuidará do redraw

        case ScreenState::SCREEN_TIME_EDIT:
            time_adjust_manual_field(-1); // Decrementa valor do campo atual
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_TIMEZONE_EDIT:
            time_adjust_gmt_offset(-1); // Decrementa offset GMT
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_LANGUAGE_SELECT:
            i18n_cycle_language_selection(false); // Navega para idioma anterior
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM: // Cancela exclusão
        case ScreenState::SCREEN_SERVICE_ADD_CONFIRM:    // Cancela adição
            ui_change_screen(ScreenState::SCREEN_MENU_MAIN); // Volta para o menu
            break; // ui_change_screen cuida do redraw

        default: break;
    }

    if (needs_redraw) {
        ui_draw_screen(false); // Força redesenho parcial da tela atual
    }
}

static void btn_next_click() {
    if (current_screen == ScreenState::SCREEN_MESSAGE) {
        ScreenState next = message_next_screen;
        // --- Resetar estado da mensagem antes de mudar ---
        message_duration_ms = 0;
        // timer_started_for_this_message = false; // Resetar a flag do timer também
        // -> Para resetar a flag static, precisamos acessá-la. É mais fácil
        // -> deixar a lógica em input_tick() resetá-la quando sair da tela de msg.
        // -> Apenas zerar a duração é suficiente para parar o timer.
        // --- Fim do reset ---
        ui_change_screen(next, true);
        last_interaction_time = millis();
        return;
    }

    last_interaction_time = millis();
    bool needs_redraw = false;

    switch (current_screen) {
        case ScreenState::SCREEN_TOTP_VIEW:
            if (service_count > 0) {
                current_service_index = (current_service_index + 1) % service_count;
                if (totp_decode_current_service_key()) {
                    ui_change_screen(ScreenState::SCREEN_TOTP_VIEW);
                } else {
                    ui_change_screen(ScreenState::SCREEN_TOTP_VIEW); // Força redraw mesmo com erro
                }
            }
            break;

        case ScreenState::SCREEN_MENU_MAIN:
            if (NUM_MENU_OPTIONS > 0) {
                int old_visible_pos = current_menu_index - menu_top_visible_index;
                int old_highlight_y = UI_MENU_START_Y + old_visible_pos * (UI_MENU_ITEM_HEIGHT + UI_MENU_ITEM_SPACING);

                current_menu_index = (current_menu_index + 1) % NUM_MENU_OPTIONS;

                // Lógica de rolagem para baixo
                if (current_menu_index >= menu_top_visible_index + VISIBLE_MENU_ITEMS) {
                    // Rola para baixo para manter o item selecionado visível (na última posição)
                    menu_top_visible_index = current_menu_index - VISIBLE_MENU_ITEMS + 1;
                } else if (current_menu_index == 0 && menu_top_visible_index != 0) {
                    menu_top_visible_index = 0; // Voltou ao início
                }
                 menu_top_visible_index = min(menu_top_visible_index, max(0, NUM_MENU_OPTIONS - VISIBLE_MENU_ITEMS)); // Garante limite

                // Inicia animação
                int new_visible_pos = current_menu_index - menu_top_visible_index;
                int new_highlight_y = UI_MENU_START_Y + new_visible_pos * (UI_MENU_ITEM_HEIGHT + UI_MENU_ITEM_SPACING);
                 if (menu_highlight_y_current != new_highlight_y) {
                     if (menu_highlight_y_current == -1) menu_highlight_y_current = old_highlight_y;
                     menu_highlight_y_anim_start = menu_highlight_y_current;
                     menu_highlight_y_target = new_highlight_y;
                     menu_animation_start_time = millis();
                     is_menu_animating = true;
                 }
            }
            break;

        case ScreenState::SCREEN_TIME_EDIT:
            time_adjust_manual_field(+1); // Incrementa valor do campo
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_TIMEZONE_EDIT:
            time_adjust_gmt_offset(+1); // Incrementa offset GMT
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_LANGUAGE_SELECT:
            i18n_cycle_language_selection(true); // Navega para próximo idioma
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM: // Confirma exclusão
            if (service_storage_delete(current_service_index)) {
                // Mensagem de sucesso e vai para TOTP (se houver) ou Menu
                ui_queue_message(getText(StringID::STR_SERVICE_DELETED), COLOR_SUCCESS, 1500,
                                 service_count > 0 ? ScreenState::SCREEN_TOTP_VIEW : ScreenState::SCREEN_MENU_MAIN);
            } else {
                // Mensagem de erro (já exibida por storage_delete) e volta ao Menu
                 ui_queue_message(getText(StringID::STR_ERROR_DELETING_SERVICE), COLOR_ERROR, 2000, ScreenState::SCREEN_MENU_MAIN);
            }
            break; // ui_queue_message cuida da transição

        case ScreenState::SCREEN_SERVICE_ADD_CONFIRM: // Confirma adição
            if (service_storage_commit_add()) { // Tenta salvar o serviço preparado
                 // Mensagem de sucesso e vai para a tela TOTP ver o novo código
                 ui_queue_message(getText(StringID::STR_SERVICE_ADDED), COLOR_SUCCESS, 1500, ScreenState::SCREEN_TOTP_VIEW);
            } else {
                 // Mensagem de erro (já exibida por storage_commit_add) e volta ao Menu
                 ui_queue_message(getText(StringID::STR_ERROR_SAVING_SERVICE), COLOR_ERROR, 2000, ScreenState::SCREEN_MENU_MAIN);
            }
            break; // ui_queue_message cuida da transição

        default: break;
    }

    if (needs_redraw) {
        ui_draw_screen(false);
    }
}

static void btn_next_double_click() {
    last_interaction_time = millis();
    // Double click sempre volta ao menu principal, exceto se já estiver nele.
    if (current_screen != ScreenState::SCREEN_MENU_MAIN) {
        ui_change_screen(ScreenState::SCREEN_MENU_MAIN);
    }
}

static void btn_next_long_press_start() {
    if (current_screen == ScreenState::SCREEN_MESSAGE) {
        ScreenState next = message_next_screen;
        // --- Resetar estado da mensagem antes de mudar ---
        message_duration_ms = 0;
        // timer_started_for_this_message = false; // Resetar a flag do timer também
        // -> Para resetar a flag static, precisamos acessá-la. É mais fácil
        // -> deixar a lógica em input_tick() resetá-la quando sair da tela de msg.
        // -> Apenas zerar a duração é suficiente para parar o timer.
        // --- Fim do reset ---
        ui_change_screen(next, true);
        last_interaction_time = millis();
        return;
    }

    last_interaction_time = millis();

    switch (current_screen) {
        case ScreenState::SCREEN_TOTP_VIEW:
            // Long press na tela TOTP inicia a exclusão do serviço atual
            if (service_count > 0) {
                ui_change_screen(ScreenState::SCREEN_SERVICE_DELETE_CONFIRM);
            }
            break;

        case ScreenState::SCREEN_MENU_MAIN:
            // Long press no menu seleciona a opção atual
            if (current_menu_index >= 0 && current_menu_index < NUM_MENU_OPTIONS) {
                switch (menuOptionIDs[current_menu_index]) {
                    case StringID::STR_MENU_VIEW_CODES:
                        if (service_count > 0) {
                            // Seleciona o primeiro serviço ao entrar na tela TOTP vindo do menu
                            current_service_index = 0;
                            if(totp_decode_current_service_key()) {
                                ui_change_screen(ScreenState::SCREEN_TOTP_VIEW);
                            } else {
                                // Erro ao decodificar, a tela TOTP mostrará erro
                                ui_change_screen(ScreenState::SCREEN_TOTP_VIEW);
                            }
                        } else {
                            ui_queue_message(getText(StringID::STR_ERROR_NO_SERVICES), COLOR_ACCENT, 2000, ScreenState::SCREEN_MENU_MAIN);
                        }
                        break;
                    case StringID::STR_MENU_ADD_SERVICE:
                        ui_change_screen(ScreenState::SCREEN_SERVICE_ADD_WAIT);
                        break;
                    case StringID::STR_MENU_READ_RFID_OPTION:
                         // Limpa ID anterior antes de ir para a tela
                         memset(card_id, 0, sizeof(card_id));
                         ui_change_screen(ScreenState::SCREEN_READ_RFID);
                         break;
                    case StringID::STR_MENU_ADJUST_TIME:
                        time_start_manual_edit(); // Prepara variáveis de edição com hora atual
                        ui_change_screen(ScreenState::SCREEN_TIME_EDIT);
                        break;
                    case StringID::STR_MENU_ADJUST_TIMEZONE:
                        ui_change_screen(ScreenState::SCREEN_TIMEZONE_EDIT);
                        break;
                    case StringID::STR_MENU_SELECT_LANGUAGE:
                        // Sincroniza o índice do menu de idiomas com o idioma atual ao entrar
                        current_language_menu_index = static_cast<int>(current_language);
                        ui_change_screen(ScreenState::SCREEN_LANGUAGE_SELECT);
                        break;
                }
            }
            break;

        case ScreenState::SCREEN_TIME_EDIT:
            // Long press avança o campo de edição (H->M->S) ou salva se estiver no último campo
            if (time_next_manual_edit_field()) { // Se true, significa que salvou
                // Mensagem de sucesso e volta ao menu
                time_t local_t = time_local_now();
                ui_queue_message_fmt(StringID::STR_TIME_ADJUSTED_FMT, COLOR_SUCCESS, 2000, ScreenState::SCREEN_MENU_MAIN,
                                     hour(local_t), minute(local_t), second(local_t));
            } else {
                // Apenas mudou de campo, redesenha a tela para mostrar o novo marcador
                ui_draw_screen(false); // Redesenho parcial
            }
            break;

        case ScreenState::SCREEN_TIMEZONE_EDIT:
            // Long press salva o fuso horário selecionado
            time_save_gmt_offset();
            ui_queue_message_fmt(StringID::STR_TIMEZONE_SAVED_FMT, COLOR_SUCCESS, 1500, ScreenState::SCREEN_MENU_MAIN, gmt_offset);
            break;

        case ScreenState::SCREEN_LANGUAGE_SELECT:
            // Long press salva o idioma selecionado
            i18n_save_selected_language();
            // Mostra mensagem apenas se o idioma realmente mudou
            if (static_cast<Language>(current_language_menu_index) == current_language) {
                 ui_queue_message(getText(StringID::STR_LANG_SAVED), COLOR_SUCCESS, 1500, ScreenState::SCREEN_MENU_MAIN);
            } else {
                 // Se não mudou, apenas volta ao menu sem mensagem
                 ui_change_screen(ScreenState::SCREEN_MENU_MAIN);
            }
            break;

        default:
            // Long press não faz nada nas outras telas por padrão
            break;
    }
}