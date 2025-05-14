#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>  
#include <stddef.h> 


#include "pin_config.h" // Inclui as definições de pinos específicas do hardware

// ---- Constantes e Configurações do Aplicativo ----
constexpr uint32_t TOTP_INTERVAL = 30;           // Intervalo de validade do código TOTP em segundos
constexpr int MAX_SERVICES = 20;                 // Número máximo de serviços TOTP suportados (reduzido para economizar RAM)
constexpr size_t MAX_SERVICE_NAME_LEN = 20;      // Comprimento máximo do nome do serviço
constexpr size_t MAX_SECRET_B32_LEN = 64;        // Comprimento máximo do segredo Base32
constexpr size_t MAX_SECRET_BIN_LEN = 40;        // Comprimento máximo do segredo binário (após decodificação Base32)
constexpr size_t JSON_DOC_SIZE_SERVICE = 256;    // Tamanho do buffer JSON para adicionar serviço
constexpr size_t JSON_DOC_SIZE_TIME = 256;       // Tamanho do buffer JSON para ajustar hora

// ---- Configurações da Interface Gráfica (UI) ----
constexpr int VISIBLE_MENU_ITEMS = 3;            // Número de itens de menu visíveis na tela
constexpr uint32_t SCREEN_UPDATE_INTERVAL_MS = 1000; // Intervalo base para atualização da tela (relógio, bateria)
constexpr int MENU_ANIMATION_DURATION_MS = 120;  // Duração da animação de transição do menu (reduzido para mais rápido)
constexpr uint32_t LOOP_DELAY_MS = 10;           // Delay principal do loop (reduzido para maior responsividade)

// Cores da UI (formato RGB565)
#define COLOR_BG TFT_BLACK
#define COLOR_FG TFT_WHITE
#define COLOR_ACCENT TFT_CYAN
#define COLOR_SUCCESS TFT_GREEN
#define COLOR_ERROR TFT_RED
#define COLOR_BAR_FG TFT_WHITE
#define COLOR_BAR_BG TFT_DARKGREY
#define COLOR_HIGHLIGHT_BG TFT_WHITE
#define COLOR_HIGHLIGHT_FG TFT_BLACK
#define COLOR_DIM_TEXT TFT_DARKGREY

// Dimensões e Posições da UI
#define UI_PADDING 5
#define UI_HEADER_HEIGHT 25
#define UI_CLOCK_WIDTH_ESTIMATE 85
#define UI_BATT_WIDTH 24
#define UI_BATT_HEIGHT 16
#define UI_CONTENT_Y_START (UI_HEADER_HEIGHT + UI_PADDING)
#define UI_TOTP_CODE_FONT_SIZE 3
#define UI_PROGRESS_BAR_HEIGHT 10 // Reduzido
// !!! CORREÇÃO AQUI: UI_PROGRESS_BAR_WIDTH e _X dependem de tft.width()
// !!! Essas macros não podem ser definidas diretamente aqui, pois 'tft' não é conhecido.
// !!! Elas devem ser calculadas dinamicamente onde são usadas ou definidas após a inicialização do tft.
// !!! Vamos removê-las daqui por enquanto e calcular em ui_manager.cpp
// #define UI_PROGRESS_BAR_WIDTH (tft.width() - 2 * UI_PADDING - 20) // <<< REMOVER ou calcular depois
// #define UI_PROGRESS_BAR_X ((tft.width() - UI_PROGRESS_BAR_WIDTH) / 2) // <<< REMOVER ou calcular depois
#define UI_FOOTER_TEXT_Y (170 - UI_PADDING / 2) // <<< Usar valor fixo ou calcular depois (ex: 170 para T-Display S3)
#define UI_MENU_ITEM_HEIGHT 30
#define UI_MENU_ITEM_SPACING 5
#define UI_MENU_START_Y UI_CONTENT_Y_START

// ---- Configurações de Gerenciamento de Energia e Bloqueio ----
constexpr uint32_t INACTIVITY_TIMEOUT_MS = 20000; // Tempo para escurecer a tela (reduzido)
constexpr uint32_t LOCK_INACTIVITY_TIMEOUT_MS = 5 * 60 * 1000; // 5 minutos para bloqueio automático
constexpr uint32_t RTC_SYNC_INTERVAL_MS = 3600000; // Sincronizar com RTC a cada hora
constexpr float USB_THRESHOLD_VOLTAGE = 4.15f;
constexpr float BATTERY_MAX_VOLTAGE = 4.10f;
constexpr float BATTERY_MIN_VOLTAGE = 3.20f;

// ---- Configurações de RFID ----
constexpr int MAX_AUTHORIZED_CARDS = 5;     // Máximo de cartões RFID autorizados
constexpr int MAX_CARD_ID_LEN = 16;         // Comprimento máximo da string do ID do cartão (incluindo \0)

// Níveis de Brilho (0-255 para PWM de hardware, ou 0-16 para fallback de software)
// Ajuste BRIGHTNESS_MAX_LEVEL conforme a resolução do PWM (e.g., 255 para 8 bits)
constexpr uint8_t BRIGHTNESS_MAX_LEVEL = 255; // Para PWM de hardware com 8 bits de resolução
constexpr uint8_t BRIGHTNESS_USB = 200;       // Brilho quando conectado ao USB
constexpr uint8_t BRIGHTNESS_BATTERY = 120;   // Brilho normal na bateria
constexpr uint8_t BRIGHTNESS_DIMMED = 40;     // Brilho reduzido após inatividade
constexpr uint8_t BRIGHTNESS_OFF = 0;         // Tela desligada

// Configurações do PWM do Backlight (para ledc - PWM de hardware)
#define BL_PWM_CHANNEL 0     // Canal LEDC (0-7 para alta velocidade, 0-15 total)
#define BL_PWM_FREQ 5000     // Frequência do PWM em Hz
#define BL_PWM_RESOLUTION 8  // Resolução do PWM em bits (8 bits = 0-255)

// ---- Chaves NVS (Non-Volatile Storage) ----
extern const char* NVS_NAMESPACE;           
extern const char* NVS_LANG_KEY;            
extern const char* NVS_TZ_OFFSET_KEY;       
extern const char* NVS_SVC_COUNT_KEY;       
extern const char* NVS_SVC_NAME_PREFIX;     
extern const char* NVS_SVC_NAME_SUFFIX;     
extern const char* NVS_SVC_SECRET_SUFFIX;   
// Novas chaves NVS para RFID e estado de bloqueio
extern const char* NVS_LOCK_STATE_KEY;        // Chave para estado de bloqueio (true/false)
extern const char* NVS_AUTH_CARD_COUNT_KEY; // Chave para número de cartões autorizados
extern const char* NVS_AUTH_CARD_PREFIX;    // Prefixo para chaves de cartões autorizados (ex: "authcard_0")
extern const char* NVS_LOCK_TIMEOUT_KEY;    // Chave para o valor do timeout de bloqueio em minutos
// Novas chaves NVS para níveis de brilho
extern const char* NVS_BRIGHTNESS_USB_KEY;
extern const char* NVS_BRIGHTNESS_BATTERY_KEY;
extern const char* NVS_BRIGHTNESS_DIMMED_KEY;

#endif // CONFIG_H