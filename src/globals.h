#pragma once // Include guard

#include <Arduino.h>
#include <RTClib.h>       // Para RTC_DS3231
#include <TFT_eSPI.h>     // Para TFT_eSPI, TFT_eSprite
#include "OneButton.h"    // Para OneButton
#include <Preferences.h>  // Para Preferences (NVS)
#include <MFRC522.h>      // Para MFRC522

#include "types.h"        // Nossos enums e structs (ScreenState, TOTPService, etc.)
#include "config.h"       // Pinos e constantes (para inicialização de objetos)

//=============================================================================
// Declarações Extern das Variáveis Globais
// (As definições/inicializações reais estão em globals.cpp)
//=============================================================================

// --- Hardware Objects ---
extern TFT_eSPI tft;                 // Objeto principal do display TFT
extern TFT_eSprite spr_header_clock; // Sprite para relógio no header (anti-flicker)
extern TFT_eSprite spr_header_batt;  // Sprite para bateria no header (anti-flicker)
extern TFT_eSprite spr_totp_code;    // Sprite para exibir o código TOTP grande
extern TFT_eSprite spr_progress_bar; // Sprite para a barra de progresso TOTP
extern RTC_DS3231 rtc;               // Objeto do Real-Time Clock
extern MFRC522 mfrc522;             // Objeto do leitor RFID
extern OneButton btn_prev;           // Botão "Anterior" ou "Esquerda"
extern OneButton btn_next;           // Botão "Próximo" ou "Direita"
extern Preferences preferences;     // Objeto para acesso ao NVS

// --- Application State ---
extern ScreenState current_screen;        // Tela atualmente ativa
extern ScreenState previous_screen;       // Tela anterior (para voltar de mensagens/confirmações)
extern TOTPService services[MAX_SERVICES]; // Array para armazenar os serviços TOTP
extern int service_count;                 // Número de serviços atualmente carregados
extern int current_service_index;         // Índice do serviço TOTP sendo exibido/editado (-1 se nenhum)
extern CurrentTOTPInfo current_totp;      // Informações sobre o código TOTP atual (código, validade)
extern BatteryInfo battery_info;          // Informações sobre a bateria (voltagem, percentual, USB)
extern int gmt_offset_hours;              // Fuso horário em horas (e.g., -3 para GMT-3)
extern Language current_language;         // Idioma atualmente selecionado para a UI

// --- Menu Options ---
extern const StringID menuOptionIDs[]; // Identificadores dos itens do menu principal
extern const int NUM_MENU_OPTIONS;    // Número total de opções no menu principal

// --- UI State ---
extern uint8_t current_brightness_level; // Nível de brilho atual aplicado (0-definido em config.h)
extern MenuState main_menu_state;        // Estado do menu principal (índice, scroll, animação)
extern MenuState lang_menu_state;        // Estado do menu de seleção de idioma
extern TempData temp_data;               // Dados temporários para adição/edição (serviço, hora, idioma, rfid)
bool is_menu_animating;                  // Flag para indicar se o menu está animando (scrolling)

// --- Timers ---
extern uint32_t last_interaction_time;   // Millis() da última interação do usuário (botões, serial)
extern uint32_t last_rtc_sync_time;      // Millis() da última sincronização TimeLib -> RTC
extern uint32_t last_screen_update_time; // Millis() da última atualização regular da tela
extern uint32_t message_end_time;        // Millis() quando a mensagem temporária deve desaparecer (0 se inativa)

// --- Buffers ---
extern char message_buffer[120];          // Buffer para formatar mensagens temporárias e outras strings
extern uint16_t message_color;            // Cor para a mensagem temporária atual

// --- Flags ---
extern bool request_full_redraw;          // Flag para solicitar um redesenho completo da tela no próximo ciclo
extern bool rtc_available;                // Flag indicando se o RTC foi inicializado com sucesso