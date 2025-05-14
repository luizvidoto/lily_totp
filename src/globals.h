#ifndef GLOBALS_H
#define GLOBALS_H

#include <RTClib.h>
#include <TFT_eSPI.h>
#include "OneButton.h"
#include <Preferences.h>
#include <MFRC522.h>
#include "types.h" // Inclui nossas structs e enums

// ---- Objetos de Hardware e Bibliotecas ----
extern RTC_DS3231 rtc;
extern TFT_eSPI tft;
extern Preferences preferences;
extern MFRC522 mfrc522;

// Sprites para otimizar o desenho na tela
extern TFT_eSprite spr_totp_code;
extern TFT_eSprite spr_progress_bar;
extern TFT_eSprite spr_header_clock; // Sprite para o relógio no header
extern TFT_eSprite spr_header_battery; // Sprite para o ícone de bateria no header

// Botões
extern OneButton btn_prev;
extern OneButton btn_next;

// ---- Estado Global da Aplicação ----
extern TOTPService services[MAX_SERVICES];
extern int service_count;
extern int current_service_index; // Índice do serviço TOTP atualmente selecionado
extern CurrentTOTPInfo current_totp; // Informações do TOTP atualmente exibido
extern ScreenState current_screen;
extern ScreenState previous_screen; // Para saber de onde viemos (útil para mensagens)

// Informações da bateria e energia
extern BatteryInfo battery_info;
extern uint8_t current_target_brightness; // Nível de brilho alvo (0-255 ou 0-16)
extern uint32_t last_interaction_time;

// Lock state
extern bool is_locked;
extern uint32_t last_lock_check_time; // Added extern

// Configurações de tempo e idioma
extern int gmt_offset; // Deslocamento GMT em horas (+/-)
extern Language current_language;
extern const LanguageStrings *current_strings_ptr; // Ponteiro para as strings do idioma atual

// Variáveis temporárias para operações (adição de serviço, edição de tempo)
extern int temp_gmt_offset;
extern char temp_service_name[MAX_SERVICE_NAME_LEN + 1];
extern char temp_service_secret[MAX_SECRET_B32_LEN + 1];
extern int edit_time_field; // 0: hora, 1: minuto, 2: segundo
extern int edit_hour, edit_minute, edit_second;

// Estado do Menu
extern int current_menu_index;         // Índice do item de menu atualmente selecionado
extern int menu_top_visible_index;     // Índice do primeiro item de menu visível (para rolagem)
extern int menu_highlight_y_current;   // Posição Y atual do destaque do menu (para animação)
extern int menu_highlight_y_target;    // Posição Y alvo do destaque do menu
extern int menu_highlight_y_anim_start; // Posição Y no início da animação
extern unsigned long menu_animation_start_time;
extern bool is_menu_animating;
extern const StringID menuOptionIDs[]; 
extern const int NUM_MENU_OPTIONS;     

// ---- Novo: Opções do Submenu de Configurações ----
extern const StringID configMenuOptionIDs[]; // Added extern
extern const int NUM_CONFIG_MENU_OPTIONS;   // Added extern
extern int current_config_menu_index;       // Added extern

// Buffer para mensagens temporárias na tela
extern char message_buffer[120]; // Aumentado um pouco para mensagens de erro mais longas
extern uint16_t message_color;
extern uint32_t message_duration_ms;
extern ScreenState message_next_screen; // Para onde ir após a mensagem

// RFID
extern char card_id[16]; // Armazena o ID do último cartão RFID lido "XX XX XX XX"

// RFID Card Management - ADD MISSING EXTERN DECLARATIONS
extern char authorized_card_ids[MAX_AUTHORIZED_CARDS][MAX_CARD_ID_LEN];
extern int authorized_card_count;
extern int current_manage_card_index;
extern int manage_cards_top_visible_index;
// extern char temp_card_id[MAX_CARD_ID_LEN]; // This is already present

// Timers
extern uint32_t last_rtc_sync_time;
extern uint32_t last_screen_update_time;

// Variável para editar o timeout de bloqueio
extern int temp_lock_timeout_minutes;

// ---- Novas Globais para Configuração de Brilho ----
extern uint8_t brightness_usb_level;
extern uint8_t brightness_battery_level;
extern uint8_t brightness_dimmed_level;
extern int temp_brightness_value;
extern BrightnessSettingToAdjust current_brightness_setting_to_adjust;

extern char temp_card_id[MAX_CARD_ID_LEN]; // Ensure this extern declaration is present

#endif // GLOBALS_H