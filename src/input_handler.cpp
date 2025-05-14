// input_handler.cpp
#include "types.h"          // Ensure types are defined first
#include "globals.h"        // Then global variable declarations
#include "input_handler.h"
#include "config.h"         // Constantes (MAX_SERVICES, JSON sizes)
#include "ui_manager.h"     // Para ui_change_screen, ui_queue_message*
#include "service_storage.h"// Para service_storage_add_from_json, service_storage_delete
#include "time_keeper.h"    // Para time_set_manual, time_save_manual_to_rtc, time_set_from_json, time_adjust_gmt_offset, time_save_gmt_offset
#include "i18n.h"           // Para i18n_cycle_language_selection, i18n_save_selected_language, getText
#include "totp_core.h"      // Para totp_decode_current_service_key
#include "rfid_reader.h"    // Para rfid_read_card (embora a leitura possa ser movida para o loop principal)
#include "rfid_auth.h"      // Added in a previous step
#include "power_utils.h"    // Add missing include for power_save_brightness_setting
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
    if (::current_screen == ScreenState::SCREEN_SERVICE_ADD_WAIT || ::current_screen == ScreenState::SCREEN_TIME_EDIT) {
        input_process_serial();
    }

    // Dentro de input_handler.cpp -> input_tick()
    // Gerenciamento da tela de mensagem (timer)
    if (::current_screen == ScreenState::SCREEN_MESSAGE) {
        static uint32_t message_start_time = 0; 
        static bool timer_started_for_this_message = false; // Flag auxiliar

        // Inicia o timer apenas uma vez por mensagem
        if (!timer_started_for_this_message) {
             message_start_time = millis();
             timer_started_for_this_message = true; // Marca que o timer iniciou
             // Serial.printf("[UI Message] Timer iniciado para %lu ms.\n", message_duration_ms);
        }

        // Verifica se a duração da mensagem expirou E se a duração é válida
        if (::message_duration_ms > 0 && (millis() - message_start_time >= ::message_duration_ms)) {
            // Serial.println("[UI Message] Timer expirou.");
            ScreenState next = ::message_next_screen; // Guarda a próxima tela
            // --- IMPORTANTE: Zera a flag ANTES de mudar de tela ---
            timer_started_for_this_message = false;
            // --- Zera a duração APÓS guardar a próxima tela, mas ANTES de chamar change_screen
            // --- para que a próxima verificação não entre aqui novamente por engano.
            ::message_duration_ms = 0;
            // --- Muda para a próxima tela ---
            ui_change_screen(next, true);
            ::last_interaction_time = millis(); // Update interaction time AFTER returning from message screen
            // NÃO zere message_duration_ms aqui novamente.
        }

        // Verifica se a duração da mensagem expirou E se a duração não foi zerada por um clique
        if (::message_duration_ms > 0 && (millis() - message_start_time >= ::message_duration_ms)) {
            // Serial.println("[UI Message] Timer expirou.");
            ui_change_screen(::message_next_screen, true); // Muda para a próxima tela
            ::message_duration_ms = 0; // Reseta para evitar re-trigger imediato
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
        ::last_interaction_time = millis(); // Entrada serial conta como interação

        // Determina o tamanho do documento JSON com base na tela
        size_t json_doc_size = (::current_screen == ScreenState::SCREEN_SERVICE_ADD_WAIT)
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
        if (::current_screen == ScreenState::SCREEN_SERVICE_ADD_WAIT) {
            // Tenta adicionar o serviço a partir do JSON. A função interna
            // cuidará da validação, cópia para temp_vars e mudança para tela de confirmação.
            service_storage_prepare_add_from_json(doc);
        } else if (::current_screen == ScreenState::SCREEN_TIME_EDIT) {
            // Tenta ajustar a hora a partir do JSON. A função interna
            // cuidará da validação, ajuste do TimeLib/RTC e exibição de mensagem.
            time_set_from_json(doc);
        }
    }
}


// ---- Callbacks dos Botões ----

static void btn_prev_click() {
    if (::current_screen == ScreenState::SCREEN_MESSAGE) {
        ScreenState next = ::message_next_screen;
        // --- Resetar estado da mensagem antes de mudar ---
        ::message_duration_ms = 0;
        // timer_started_for_this_message = false; // Resetar a flag do timer também
        // -> Para resetar a flag static, precisamos acessá-la. É mais fácil
        // -> deixar a lógica em input_tick() resetá-la quando sair da tela de msg.
        // -> Apenas zerar a duração é suficiente para parar o timer.
        // --- Fim do reset ---
        ui_change_screen(next, true);
        ::last_interaction_time = millis();
        return;
    }

    ::last_interaction_time = millis();
    bool needs_redraw = false; // Flag se a ação requer redesenho parcial/imediato

    switch (::current_screen) {
        case ScreenState::SCREEN_TOTP_VIEW:
            if (::service_count > 0) {
                ::current_service_index = (::current_service_index - 1 + ::service_count) % ::service_count;
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
                ::current_menu_index = (::current_menu_index - 1 + NUM_MENU_OPTIONS) % NUM_MENU_OPTIONS;
                ui_draw_screen(false); // Redraw to show new selection
            }
            break; // Animação cuidará do redraw

        case ScreenState::SCREEN_TIME_EDIT:
            time_adjust_manual_field(-1); // Decrementa valor do campo atual
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_TIMEZONE_EDIT:
            // Modifica a variável temporária
            ::temp_gmt_offset += -1;
            if (::temp_gmt_offset < -12) ::temp_gmt_offset = 14;
            if (::temp_gmt_offset > 14) ::temp_gmt_offset = -12; // Garante wrap-around
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_LANGUAGE_SELECT:
            i18n_cycle_language_selection(false); // Navega para idioma anterior
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_MANAGE_CARDS:
            if (::authorized_card_count > 0) {
                int old_idx = ::current_manage_card_index;
                ::current_manage_card_index = (::current_manage_card_index - 1 + ::authorized_card_count) % ::authorized_card_count;
                
                // Scrolling logic for manage cards screen - UI will determine max_visible_items
                // This logic assumes VISIBLE_MENU_ITEMS can be a proxy or that ui_manager handles clamping.
                // A more robust way is for ui_manager to expose how many items it can show, or for input_handler
                // to not care about the visual aspect, only the index.
                // For now, simplified: if current index is less than top, make current top.
                // If wrapped around, go to bottom.
                int max_visible_items_proxy = VISIBLE_MENU_ITEMS; // Using as a general idea of visible items

                if (::current_manage_card_index < ::manage_cards_top_visible_index) {
                    ::manage_cards_top_visible_index = ::current_manage_card_index;
                } else if (old_idx == 0 && ::current_manage_card_index == ::authorized_card_count - 1) { 
                    ::manage_cards_top_visible_index = max(0, ::authorized_card_count - max_visible_items_proxy);
                }
                // Ensure top visible index is always valid - ui_manager will also clamp this.
                if (::manage_cards_top_visible_index < 0) ::manage_cards_top_visible_index = 0;
                if (::manage_cards_top_visible_index > ::authorized_card_count - max_visible_items_proxy && ::authorized_card_count > max_visible_items_proxy) {
                     ::manage_cards_top_visible_index = ::authorized_card_count - max_visible_items_proxy;
                } else if (::authorized_card_count <= max_visible_items_proxy) {
                     ::manage_cards_top_visible_index = 0;
                }
                needs_redraw = true;
            }
            break;

        case ScreenState::SCREEN_ADD_CARD_CONFIRM:
        case ScreenState::SCREEN_DELETE_CARD_CONFIRM:
            // Prev click cancels confirmation
            ui_change_screen(ScreenState::SCREEN_MANAGE_CARDS);
            break;

        case ScreenState::SCREEN_CONFIG_MENU:
            if (NUM_CONFIG_MENU_OPTIONS > 0) {
                ::current_config_menu_index = (::current_config_menu_index - 1 + NUM_CONFIG_MENU_OPTIONS) % NUM_CONFIG_MENU_OPTIONS;
                needs_redraw = true;
            }
            break;

        case ScreenState::SCREEN_ADJUST_LOCK_TIMEOUT:
            ::temp_lock_timeout_minutes--;
            if (::temp_lock_timeout_minutes < 1) ::temp_lock_timeout_minutes = 60; // Min 1 min, max 60 min, wraps around
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_BRIGHTNESS_MENU:
            // Reusing current_config_menu_index for this small menu
            ::current_config_menu_index = (::current_config_menu_index - 1 + 3) % 3; // 3 options: USB, Battery, Dimmed
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_ADJUST_BRIGHTNESS_VALUE:
            ::temp_brightness_value -= 5; // Adjust by 5 for quicker changes
            if (::temp_brightness_value < 0) ::temp_brightness_value = BRIGHTNESS_MAX_LEVEL;
            ::temp_brightness_value = constrain(::temp_brightness_value, 0, BRIGHTNESS_MAX_LEVEL);
            power_set_screen_brightness(::temp_brightness_value); // Live preview
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM: // Cancela exclusão
        case ScreenState::SCREEN_SERVICE_ADD_CONFIRM:    // Cancela adição
            ui_change_screen(ScreenState::SCREEN_MENU_MAIN); // Volta para o menu
            break; // ui_change_screen cuida do redraw

        case ScreenState::SCREEN_SETUP_FIRST_CARD: 
            // No action for prev click on this screen
            break;

        default: break;
    }

    if (needs_redraw) {
        ui_draw_screen(false); // Força redesenho parcial da tela atual
    }
}

static void btn_next_click() {
    if (::current_screen == ScreenState::SCREEN_MESSAGE) {
        ScreenState next = ::message_next_screen;
        // --- Resetar estado da mensagem antes de mudar ---
        ::message_duration_ms = 0;
        // timer_started_for_this_message = false; // Resetar a flag do timer também
        // -> Para resetar a flag static, precisamos acessá-la. É mais fácil
        // -> deixar a lógica em input_tick() resetá-la quando sair da tela de msg.
        // -> Apenas zerar a duração é suficiente para parar o timer.
        // --- Fim do reset ---
        ui_change_screen(next, true);
        ::last_interaction_time = millis();
        return;
    }

    ::last_interaction_time = millis();
    bool needs_redraw = false;

    switch (::current_screen) {
        case ScreenState::SCREEN_TOTP_VIEW:
            if (::service_count > 0) {
                ::current_service_index = (::current_service_index + 1) % ::service_count;
                if (totp_decode_current_service_key()) {
                    ui_change_screen(ScreenState::SCREEN_TOTP_VIEW);
                } else {
                    ui_change_screen(ScreenState::SCREEN_TOTP_VIEW); // Força redraw mesmo com erro
                }
            }
            break;

        case ScreenState::SCREEN_MENU_MAIN:
            if (NUM_MENU_OPTIONS > 0) {
                ::current_menu_index = (::current_menu_index + 1) % NUM_MENU_OPTIONS;
                ui_draw_screen(false); // Redraw to show new selection
            }
            break;

        case ScreenState::SCREEN_TIME_EDIT:
            time_adjust_manual_field(+1); // Incrementa valor do campo
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_TIMEZONE_EDIT:
            // Modifica a variável temporária
            ::temp_gmt_offset += 1;
            if (::temp_gmt_offset > 14) ::temp_gmt_offset = -12;
            if (::temp_gmt_offset < -12) ::temp_gmt_offset = 14; // Garante wrap-around
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_LANGUAGE_SELECT:
            i18n_cycle_language_selection(true); // Navega para próximo idioma
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_MANAGE_CARDS:
            if (::authorized_card_count > 0) {
                int old_idx = ::current_manage_card_index;
                ::current_manage_card_index = (::current_manage_card_index + 1) % ::authorized_card_count;

                // Scrolling logic for manage cards screen - UI will determine max_visible_items
                int max_visible_items_proxy = VISIBLE_MENU_ITEMS; // Using as a general idea

                if (::current_manage_card_index >= ::manage_cards_top_visible_index + max_visible_items_proxy) {
                    ::manage_cards_top_visible_index = ::current_manage_card_index - max_visible_items_proxy + 1;
                } else if (old_idx == ::authorized_card_count - 1 && ::current_manage_card_index == 0) { 
                    ::manage_cards_top_visible_index = 0;
                }
                // Ensure top visible index is always valid - ui_manager will also clamp this.
                if (::manage_cards_top_visible_index < 0) ::manage_cards_top_visible_index = 0;
                if (::manage_cards_top_visible_index > ::authorized_card_count - max_visible_items_proxy && ::authorized_card_count > max_visible_items_proxy) {
                     ::manage_cards_top_visible_index = ::authorized_card_count - max_visible_items_proxy;
                } else if (::authorized_card_count <= max_visible_items_proxy) {
                     ::manage_cards_top_visible_index = 0;
                }
                needs_redraw = true;
            }
            break;

        case ScreenState::SCREEN_ADD_CARD_CONFIRM:
            if (rfid_auth_add_card(::temp_card_id)) {
                ui_queue_message(getText(StringID::STR_CARD_ADDED), COLOR_SUCCESS, 1500, ScreenState::SCREEN_MANAGE_CARDS);
            } else {
                // Error message (e.g., max cards, already exists) should be handled by rfid_auth_add_card or shown here
                // For now, assume rfid_auth_add_card might queue its own specific error or we rely on its Serial log.
                // A more robust solution would be for rfid_auth_add_card to return a specific error code.
                ui_queue_message(getText(StringID::STR_ERROR_SAVING_CARD), COLOR_ERROR, 2000, ScreenState::SCREEN_MANAGE_CARDS); // Generic error
            }
            break;

        case ScreenState::SCREEN_DELETE_CARD_CONFIRM:
            if (rfid_auth_delete_card(::current_manage_card_index)) {
                if (::authorized_card_count == 0) {
                    // Last card deleted, force setup for a new first card
                    ::is_locked = true; // Ensure device is locked
                    rfid_auth_save_lock_state(true);
                    ui_queue_message(getText(StringID::STR_LAST_CARD_DELETED_SETUP_NEW), COLOR_ACCENT, 3000, ScreenState::SCREEN_SETUP_FIRST_CARD);
                } else {
                    ui_queue_message(getText(StringID::STR_CARD_DELETED), COLOR_SUCCESS, 1500, ScreenState::SCREEN_MANAGE_CARDS);
                }
            } else {
                ui_queue_message(getText(StringID::STR_ERROR_DELETING_CARD), COLOR_ERROR, 2000, ScreenState::SCREEN_MANAGE_CARDS);
            }
            // Reset index after deletion, as list size might change or we might be leaving the screen
            ::current_manage_card_index = 0;
            ::manage_cards_top_visible_index = 0;
            break;

        case ScreenState::SCREEN_CONFIG_MENU:
            if (NUM_CONFIG_MENU_OPTIONS > 0) {
                ::current_config_menu_index = (::current_config_menu_index + 1) % NUM_CONFIG_MENU_OPTIONS;
                needs_redraw = true;
            }
            break;

        case ScreenState::SCREEN_ADJUST_LOCK_TIMEOUT:
            ::temp_lock_timeout_minutes++;
            if (::temp_lock_timeout_minutes > 60) ::temp_lock_timeout_minutes = 1; // Max 60 min, min 1 min, wraps around
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_BRIGHTNESS_MENU:
            ::current_config_menu_index = (::current_config_menu_index + 1) % 3;
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_ADJUST_BRIGHTNESS_VALUE:
            ::temp_brightness_value += 5;
            if (::temp_brightness_value > BRIGHTNESS_MAX_LEVEL) ::temp_brightness_value = 0;
            ::temp_brightness_value = constrain(::temp_brightness_value, 0, BRIGHTNESS_MAX_LEVEL);
            power_set_screen_brightness(::temp_brightness_value); // Live preview
            needs_redraw = true;
            break;

        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM: // Confirma exclusão
            if (service_storage_delete(::current_service_index)) {
                // Mensagem de sucesso e vai para TOTP (se houver) ou Menu
                ui_queue_message(getText(StringID::STR_SERVICE_DELETED), COLOR_SUCCESS, 1500,
                                 ::service_count > 0 ? ScreenState::SCREEN_TOTP_VIEW : ScreenState::SCREEN_MENU_MAIN);
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

        case ScreenState::SCREEN_SETUP_FIRST_CARD: 
            // No action for next click on this screen
            break;

        default: break;
    }

    if (needs_redraw) {
        ui_draw_screen(false);
    }
}

// ... btn_next_double_click() remains the same, but ensure it handles SCREEN_CONFIG_MENU by going to SCREEN_MENU_MAIN ...
static void btn_next_double_click() {
    ::last_interaction_time = millis();
    // Double click sempre volta ao menu principal, exceto se já estiver nele.
    // Ou se estiver em uma tela de confirmação de baixo nível, volta para a tela anterior relevante.
    switch (::current_screen) {
        case ScreenState::SCREEN_ADD_CARD_CONFIRM:
        case ScreenState::SCREEN_DELETE_CARD_CONFIRM:
            ui_change_screen(ScreenState::SCREEN_MANAGE_CARDS);
            break;
        case ScreenState::SCREEN_ADJUST_LOCK_TIMEOUT: // Double click from adjust timeout goes back to config menu
            ui_change_screen(ScreenState::SCREEN_CONFIG_MENU);
            break;
        case ScreenState::SCREEN_BRIGHTNESS_MENU: // Double click from brightness menu goes back to config menu
            ui_change_screen(ScreenState::SCREEN_CONFIG_MENU);
            break;
        case ScreenState::SCREEN_ADJUST_BRIGHTNESS_VALUE: // Double click from adjust brightness value goes back to brightness menu
            // Restore original brightness before going back if not saved
            power_load_brightness_settings(); // Reload from NVS to discard temp changes
            power_update_target_brightness(); // Apply the NVS-loaded brightness
            ui_change_screen(ScreenState::SCREEN_BRIGHTNESS_MENU);
            break;
        case ScreenState::SCREEN_SERVICE_ADD_CONFIRM:
        case ScreenState::SCREEN_SERVICE_DELETE_CONFIRM:
            ui_change_screen(ScreenState::SCREEN_MENU_MAIN);
            break;
        case ScreenState::SCREEN_LOCKED: // No double click action on locked screen
            break;
        case ScreenState::SCREEN_SETUP_FIRST_CARD: 
            // No action for double click on this screen
            break;
        default:
            if (::current_screen != ScreenState::SCREEN_MENU_MAIN) {
                ui_change_screen(ScreenState::SCREEN_MENU_MAIN);
            }
            break;
    }
}

static void btn_next_long_press_start() {
    ::last_interaction_time = millis(); // Reseta inatividade
    bool change_screen_handled = false;

    if (::is_locked && ::current_screen != ScreenState::SCREEN_LOCKED) {
        // Se estiver bloqueado, mas não na tela de bloqueio (ex: mensagem sobreposta),
        // qualquer interação de botão deve levar de volta à tela de bloqueio.
        ui_change_screen(ScreenState::SCREEN_LOCKED);
        return;
    }

    switch (::current_screen) {
        case ScreenState::SCREEN_LOCKED:
            // Botões não fazem nada na tela de bloqueio por enquanto
            change_screen_handled = true;
            break;
        case ScreenState::SCREEN_TOTP_VIEW:
            // Long press na tela TOTP inicia a exclusão do serviço atual
            if (::service_count > 0) {
                ui_change_screen(ScreenState::SCREEN_SERVICE_DELETE_CONFIRM);
            }
            break;

        case ScreenState::SCREEN_MENU_MAIN:
            // Long press no menu seleciona a opção atual
            if (::current_menu_index >= 0 && ::current_menu_index < NUM_MENU_OPTIONS) {
                change_screen_handled = true; // Assume que a ação do menu lidará com a tela
                switch (menuOptionIDs[::current_menu_index]) {
                    case StringID::STR_MENU_VIEW_CODES:
                        if (::service_count > 0) {
                            // Seleciona o primeiro serviço ao entrar na tela TOTP vindo do menu
                            ::current_service_index = 0;
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
                    case StringID::STR_ACTION_CONFIG: 
                        // TODO: Implementar tela de configurações real.
                        // Por enquanto, vamos adicionar uma opção de "Gerenciar Cartões" aqui.
                        // Esta é uma simplificação. Idealmente, STR_ACTION_CONFIG levaria a um SUBMENU.
                        // Para este exemplo, vamos direto para Gerenciar Cartões.
                        if (::authorized_card_count == 0 && MAX_AUTHORIZED_CARDS > 0) {
                            // Se não há cartões, e podemos adicionar, talvez ir direto para adicionar?
                            // Ou mostrar a lista vazia com opção de adicionar.
                            // Por agora, vamos para a tela de gerenciamento que mostrará a lista (vazia).
                            ::current_manage_card_index = 0; // Reseta índice da lista de cartões
                            memset(::temp_card_id, 0, sizeof(::temp_card_id)); // Limpa ID temporário
                            ui_change_screen(ScreenState::SCREEN_MANAGE_CARDS);
                        } else if (MAX_AUTHORIZED_CARDS > 0) {
                            ::current_manage_card_index = 0; // Reseta índice da lista de cartões
                            memset(::temp_card_id, 0, sizeof(::temp_card_id)); // Limpa ID temporário
                            ui_change_screen(ScreenState::SCREEN_MANAGE_CARDS);
                        } else {
                            ui_queue_message(getText(StringID::STR_NOT_IMPLEMENTED_YET), COLOR_ACCENT, 1500, ScreenState::SCREEN_MENU_MAIN);
                        }
                        break;
                    case StringID::STR_MENU_ADD_SERVICE: // This case might be removed if "Add Service" is no longer a direct main menu item
                        ui_change_screen(ScreenState::SCREEN_SERVICE_ADD_WAIT);
                        break;
                    case StringID::STR_MENU_READ_RFID_OPTION:
                         // Limpa ID anterior antes de ir para a tela
                         memset(::card_id, 0, sizeof(::card_id));
                         ui_change_screen(ScreenState::SCREEN_MANAGE_CARDS);
                         break;
                    case StringID::STR_MENU_ADJUST_TIME:
                        time_start_manual_edit(); // Prepara variáveis de edição com hora atual
                        ui_change_screen(ScreenState::SCREEN_TIME_EDIT);
                        break;
                    case StringID::STR_MENU_ADJUST_TIMEZONE:
                        ::temp_gmt_offset = ::gmt_offset; // Sincroniza temp com o valor atual
                        ui_change_screen(ScreenState::SCREEN_TIMEZONE_EDIT);
                        break;
                    case StringID::STR_MENU_SELECT_LANGUAGE:
                        // Sincroniza o índice do menu de idiomas com o idioma atual ao entrar
                        ::current_language_menu_index = static_cast<int>(::current_language);
                        ui_change_screen(ScreenState::SCREEN_LANGUAGE_SELECT);
                        break;
                    default:
                        change_screen_handled = false; // Ação desconhecida
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
            ::gmt_offset = ::temp_gmt_offset;
            time_save_gmt_offset();
            ui_queue_message_fmt(StringID::STR_TIMEZONE_SAVED_FMT, COLOR_SUCCESS, 1500, ScreenState::SCREEN_MENU_MAIN, ::gmt_offset);
            break;

        case ScreenState::SCREEN_LANGUAGE_SELECT:
            // Long press salva o idioma selecionado
            i18n_save_selected_language();
            // Mostra mensagem apenas se o idioma realmente mudou
            if (static_cast<Language>(::current_language_menu_index) == ::current_language) {
                 ui_queue_message(getText(StringID::STR_LANG_SAVED), COLOR_SUCCESS, 1500, ScreenState::SCREEN_MENU_MAIN);
            } else {
                 // Se não mudou, apenas volta ao menu sem mensagem
                 ui_change_screen(ScreenState::SCREEN_MENU_MAIN);
            }
            break;

        case ScreenState::SCREEN_MANAGE_CARDS:
            // Long press: Add if list is empty or no item selected in a way that implies 'add'.
            // Or, if an item is selected, it could mean 'delete selected'.
            // The footer STR_FOOTER_MANAGE_CARDS_NAV is "Prox | LP: Add/Del | Dbl: Menu"
            // So, LP should either add or delete based on context.
            if (::authorized_card_count < MAX_AUTHORIZED_CARDS) { // If there's space, LP can mean Add
                memset(::temp_card_id, 0, sizeof(::temp_card_id)); // Clear any previous temp ID
                ui_change_screen(ScreenState::SCREEN_ADD_CARD_WAIT);
            } else if (::authorized_card_count > 0) { // If list is full, but has items, LP on selected means Delete
                 // Ensure ::current_manage_card_index is valid
                if (::current_manage_card_index >= 0 && ::current_manage_card_index < ::authorized_card_count) {
                    ui_change_screen(ScreenState::SCREEN_DELETE_CARD_CONFIRM);
                } else {
                    // Index out of bounds, perhaps go to add if possible, or do nothing
                    if (::authorized_card_count < MAX_AUTHORIZED_CARDS) {
                        ui_change_screen(ScreenState::SCREEN_ADD_CARD_WAIT);
                    } else {
                        ui_queue_message(getText(StringID::STR_ERROR_MAX_CARDS), COLOR_ERROR, 2000, ScreenState::SCREEN_MANAGE_CARDS);
                    }
                }
            } else { // List is empty and full (MAX_AUTHORIZED_CARDS = 0), do nothing or show error
                 ui_queue_message(getText(StringID::STR_ERROR_MAX_CARDS), COLOR_ERROR, 2000, ScreenState::SCREEN_MANAGE_CARDS);
            }
            change_screen_handled = true;
            break;

        case ScreenState::SCREEN_ADD_CARD_WAIT:
            // Long press cancels adding a card
            ui_change_screen(ScreenState::SCREEN_MANAGE_CARDS);
            change_screen_handled = true;
            break;

        case ScreenState::SCREEN_CONFIG_MENU:
            if (::current_config_menu_index >= 0 && ::current_config_menu_index < NUM_CONFIG_MENU_OPTIONS) {
                change_screen_handled = true;
                switch (configMenuOptionIDs[::current_config_menu_index]) {
                    case StringID::STR_MENU_CONFIG_MANAGE_CARDS:
                        ::current_manage_card_index = 0; 
                        ::manage_cards_top_visible_index = 0; // Reset top visible index when entering
                        memset(::temp_card_id, 0, sizeof(::temp_card_id));
                        ui_change_screen(ScreenState::SCREEN_MANAGE_CARDS);
                        break;
                    case StringID::STR_MENU_CONFIG_LOCK_TIMEOUT:
                        // Load current NVS value into ::temp_lock_timeout_minutes before editing
                        rfid_auth_load_lock_timeout(); // This ensures temp_ is fresh from NVS
                        ui_change_screen(ScreenState::SCREEN_ADJUST_LOCK_TIMEOUT);
                        break;
                    case StringID::STR_MENU_CONFIG_BRIGHTNESS:
                        ::current_config_menu_index = 0; // Reset index for brightness type menu
                        ui_change_screen(ScreenState::SCREEN_BRIGHTNESS_MENU);
                        break;
                    default:
                        change_screen_handled = false;
                        break;
                }
            }
            break;

        case ScreenState::SCREEN_ADJUST_LOCK_TIMEOUT:
            rfid_auth_save_lock_timeout(::temp_lock_timeout_minutes);
            ui_queue_message_fmt(StringID::STR_LOCK_TIMEOUT_SAVED_FMT, COLOR_SUCCESS, 1500, ScreenState::SCREEN_CONFIG_MENU, ::temp_lock_timeout_minutes);
            change_screen_handled = true;
            break;

        case ScreenState::SCREEN_BRIGHTNESS_MENU:
            // ::current_config_menu_index (0,1,2) maps to USB, Battery, Dimmed
            if (::current_config_menu_index == 0) { 
                ::current_brightness_setting_to_adjust = BrightnessSettingToAdjust::USB;
                ::temp_brightness_value = ::brightness_usb_level;
            } else if (::current_config_menu_index == 1) {
                ::current_brightness_setting_to_adjust = BrightnessSettingToAdjust::BATTERY;
                ::temp_brightness_value = ::brightness_battery_level;
            } else { // Index 2
                ::current_brightness_setting_to_adjust = BrightnessSettingToAdjust::DIMMED;
                ::temp_brightness_value = ::brightness_dimmed_level;
            }
            ui_change_screen(ScreenState::SCREEN_ADJUST_BRIGHTNESS_VALUE);
            change_screen_handled = true;
            break;

        case ScreenState::SCREEN_ADJUST_BRIGHTNESS_VALUE:
        { // Create a new scope for this case
            power_save_brightness_setting(::current_brightness_setting_to_adjust, ::temp_brightness_value);
            // Prepare message for which brightness was saved
            const char* saved_brightness_type_str = "Unknown"; // Declaration now within a block
            switch(::current_brightness_setting_to_adjust) {
                case BrightnessSettingToAdjust::USB: saved_brightness_type_str = getText(StringID::STR_MENU_BRIGHTNESS_USB); break;
                case BrightnessSettingToAdjust::BATTERY: saved_brightness_type_str = getText(StringID::STR_MENU_BRIGHTNESS_BATTERY); break;
                case BrightnessSettingToAdjust::DIMMED: saved_brightness_type_str = getText(StringID::STR_MENU_BRIGHTNESS_DIMMED); break;
            }
            ui_queue_message_fmt(StringID::STR_BRIGHTNESS_SAVED_FMT, COLOR_SUCCESS, 1500, ScreenState::SCREEN_BRIGHTNESS_MENU, saved_brightness_type_str, ::temp_brightness_value);
            change_screen_handled = true;
            break;
        } // End of scope for this case

        case ScreenState::SCREEN_SETUP_FIRST_CARD: 
            // No action for long press on this screen
            change_screen_handled = true; // Prevent default actions if any
            break;

        default:
            // Long press não faz nada nas outras telas por padrão
            break;
    }
}

void rfid_on_card_present(const char* uid_str) {
    Serial.printf("[RFID] Cartão apresentado: %s\n", uid_str);
    strncpy(::card_id, uid_str, MAX_CARD_ID_LEN -1); // Copia para card_id global
    ::card_id[MAX_CARD_ID_LEN -1] = '\0';

    switch (::current_screen) {
        case ScreenState::SCREEN_LOCKED: 
        { // Add braces for scope
            if (rfid_auth_is_card_authorized(uid_str)) {
                Serial.println("[Auth] Cartão autorizado! Desbloqueando...");
                ::is_locked = false;
                rfid_auth_save_lock_state(false);
                // Decide para qual tela ir após desbloquear
                ScreenState next_screen = (::service_count > 0 ? ScreenState::SCREEN_TOTP_VIEW : ScreenState::SCREEN_MENU_MAIN);
                ui_change_screen(next_screen);
                ::last_interaction_time = millis(); // Update interaction time AFTER screen change
            } else {
                Serial.println("[Auth] Cartão NÃO autorizado!");
                ui_queue_message(getText(StringID::STR_ERROR_CARD_UNAUTHORIZED), COLOR_ERROR, 1500, ScreenState::SCREEN_LOCKED);
                ::last_interaction_time = millis(); // Update interaction time even on failed unlock attempt
            }
            break;
        }
        case ScreenState::SCREEN_READ_RFID: 
        { // Add braces for scope
            ui_draw_screen(false); 
            ::last_interaction_time = millis(); // Update interaction time on RFID read screen
            break;
        }
        case ScreenState::SCREEN_ADD_CARD_WAIT:
        { // Add braces for scope
            strncpy(::temp_card_id, uid_str, MAX_CARD_ID_LEN -1);
            ::temp_card_id[MAX_CARD_ID_LEN-1] = '\0';
            ui_change_screen(ScreenState::SCREEN_ADD_CARD_CONFIRM);
            ::last_interaction_time = millis(); // Update interaction time after scanning for add
            break;
        }
        case ScreenState::SCREEN_SETUP_FIRST_CARD:
        { 
            Serial.printf("[Setup] Tentando salvar o primeiro cartão: %s\n", uid_str);
            if (rfid_auth_add_card(uid_str)) {
                ::is_locked = false; // Desbloqueia o dispositivo
                rfid_auth_save_lock_state(false); // Salva o estado desbloqueado
                ScreenState next_screen = (::service_count > 0 ? ScreenState::SCREEN_TOTP_VIEW : ScreenState::SCREEN_MENU_MAIN);
                ui_queue_message(getText(StringID::STR_FIRST_CARD_SAVED), COLOR_SUCCESS, 2500, next_screen);
                // ui_queue_message handles screen change, and it updates last_interaction_time upon entering MESSAGE screen.
                // We might need an extra update AFTER the message screen transitions back.
                // For now, let's rely on the message screen handling interaction time.
            } else {
                // rfid_auth_add_card() já mostra mensagens de erro específicas (max cards, already exists)
                // Se chegou aqui, foi um erro genérico de salvamento.
                ui_queue_message(getText(StringID::STR_ERROR_SAVING_FIRST_CARD), COLOR_ERROR, 2500, ScreenState::SCREEN_SETUP_FIRST_CARD);
                 ::last_interaction_time = millis(); // Update interaction time even on failed save attempt
            }
            break;
        }
        default:
        { // Add braces for scope
            Serial.println("[RFID] Cartão lido, mas nenhuma ação para a tela atual.");
            ::last_interaction_time = millis(); // Update interaction time on other screens too
            break;
        }
    }
}