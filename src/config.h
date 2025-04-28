#pragma once // Include guard

#include <Arduino.h>
#include <TFT_eSPI.h>     // Para definições de cores TFT_*

// ============================================================================
// === PIN CONFIGURATION (EXTERNAL) ===
// ============================================================================
// Inclui o arquivo de configuração de pinos separado do usuário.
// Certifique-se de que este arquivo define todos os pinos necessários
// listados nos comentários do arquivo original ou na estrutura proposta.
// Ex: PIN_LCD_BL, PIN_BUTTON_0, PIN_BUTTON_1, PIN_IIC_SDA, PIN_IIC_SCL,
//     SCK_PIN, MISO_PIN, MOSI_PIN, SDA_PIN (RFID CS), RST_PIN (RFID RST),
//     PIN_BAT_VOLT, PIN_POWER_ON (se existir)
#include "pin_config.h"

// ============================================================================
// === CORE SETTINGS ===
// ============================================================================
constexpr uint32_t TOTP_INTERVAL_SECONDS = 30;  // Intervalo padrão TOTP (segundos)
constexpr int MAX_SERVICES = 50;                // Número máximo de serviços armazenáveis
constexpr size_t MAX_SERVICE_NAME_LEN = 20;     // Comprimento máx. nome serviço (sem '\0')
constexpr size_t MAX_SECRET_B32_LEN = 64;       // Comprimento máx. segredo Base32 (sem '\0')
constexpr size_t MAX_SECRET_BIN_LEN = 40;       // Comprimento máx. segredo binário (bytes)

// ============================================================================
// === UI BEHAVIOR ===
// ============================================================================
constexpr int VISIBLE_MENU_ITEMS = 3;             // Quantos itens do menu são visíveis de uma vez
constexpr uint32_t INACTIVITY_TIMEOUT_MS = 30000; // Tempo (ms) para escurecer tela em bateria
constexpr uint32_t SCREEN_UPDATE_INTERVAL_MS = 500;// Intervalo (ms) para atualizações regulares (relógio, progresso)
constexpr uint32_t RTC_SYNC_INTERVAL_MS = 60 * 1000;// Intervalo (ms) para sincronizar TimeLib com RTC
constexpr int MENU_ANIMATION_DURATION_MS = 120;   // Duração (ms) da animação de scroll do menu
constexpr uint32_t TEMPORARY_MESSAGE_DURATION_MS = 2000; // Duração padrão (ms) das mensagens temporárias
constexpr uint32_t LOOP_DELAY_MS = 10;            // Delay (ms) no final do loop principal

// ============================================================================
// === POWER MANAGEMENT ===
// ============================================================================
// Níveis de Brilho (ajuste conforme necessário; 0=desligado)
// Se usar PWM por hardware (ledc), estes podem ir até 255 e setScreenBrightness precisa ser ajustado.
constexpr uint8_t BRIGHTNESS_USB = 14;            // Brilho quando alimentado por USB
constexpr uint8_t BRIGHTNESS_BATTERY = 10;        // Brilho normal em bateria
constexpr uint8_t BRIGHTNESS_DIMMED = 2;          // Brilho reduzido após inatividade em bateria

// Limiares e Fator de Conversão da Bateria (AJUSTE PARA SEU HARDWARE!)
constexpr float BATT_VOLTAGE_USB_THRESHOLD = 4.15f; // Tensão acima da qual considera-se USB
constexpr float BATT_VOLTAGE_MAX = 4.10f;           // Tensão considerada 100%
constexpr float BATT_VOLTAGE_MIN = 3.20f;           // Tensão considerada 0%
// Exemplo para T-Display S3 com divisor 100k/100k (x2) e Vref=3.3V, ADC 12bit (4096)
constexpr float BATT_ADC_CONVERSION_FACTOR = (3.3f * 2.0f) / 4095.0f;

// ============================================================================
// === UI APPEARANCE ===
// ============================================================================
#define SCREEN_ROTATION 3 // Rotação do Display: 0, 1, 2, 3 (ajuste)

// --- Language ---
#define NUM_LANGUAGES 2 // Número de idiomas suportados (atualize se adicionar mais)

// --- Colors ---
#define COLOR_BG            TFT_BLACK       // Fundo principal
#define COLOR_FG            TFT_WHITE       // Texto/elementos principais
#define COLOR_ACCENT        TFT_CYAN        // Cor de destaque (header, marcadores)
#define COLOR_SUCCESS       TFT_GREEN       // Mensagens de sucesso
#define COLOR_ERROR         TFT_RED         // Mensagens de erro
#define COLOR_WARNING       TFT_YELLOW      // Avisos (bateria baixa)
#define COLOR_BAR_FG        TFT_WHITE       // Cor da barra de progresso preenchida
#define COLOR_BAR_BG        TFT_DARKGREY    // Fundo da barra de progresso
#define COLOR_HIGHLIGHT_BG  TFT_WHITE       // Fundo do item de menu selecionado
#define COLOR_HIGHLIGHT_FG  TFT_BLACK       // Texto do item de menu selecionado
#define COLOR_DIM_TEXT      TFT_DARKGREY    // Texto secundário (rodapé, hints)
#define COLOR_HEADER_BG     COLOR_ACCENT    // Fundo do Header
#define COLOR_HEADER_FG     TFT_BLACK       // Texto sobre o Header

// --- Constantes da Interface Gráfica ---
#define SCREEN_WIDTH tft.width()
#define SCREEN_HEIGHT tft.height()
// Posições e Tamanhos
#define UI_PADDING 5
#define UI_HEADER_HEIGHT 25
#define UI_CLOCK_WIDTH_ESTIMATE 85 // Largura estimada para relógio HH:MM:SS
#define UI_BATT_WIDTH 24
#define UI_BATT_HEIGHT 16
#define UI_CONTENT_Y_START (UI_HEADER_HEIGHT + UI_PADDING) // Y inicial para conteúdo abaixo do header
#define UI_TOTP_CODE_FONT_SIZE 3
#define UI_PROGRESS_BAR_HEIGHT 15
#define UI_PROGRESS_BAR_WIDTH (SCREEN_WIDTH - 2 * UI_PADDING - 20)
#define UI_PROGRESS_BAR_X ((SCREEN_WIDTH - UI_PROGRESS_BAR_WIDTH) / 2)
#define UI_FOOTER_TEXT_Y (SCREEN_HEIGHT - UI_PADDING / 2) // Y da linha de base do texto do rodapé
#define UI_MENU_ITEM_HEIGHT 30
#define UI_MENU_START_Y UI_CONTENT_Y_START

// --- Font Sizes (Tamanhos para fontes GFX padrão da TFT_eSPI) ---
#define FONT_SIZE_HEADER 2
#define FONT_SIZE_MENU_ITEM 2
#define FONT_SIZE_TOTP_CODE 3
#define FONT_SIZE_MESSAGE 2
#define FONT_SIZE_SMALL 1     // Para rodapé, hints, exemplos JSON
#define FONT_SIZE_TIME_EDIT 3

// ============================================================================
// === NVS (Preferences) KEYS ===
// ============================================================================
#define NVS_NAMESPACE "totp_auth"             // Namespace para evitar colisões
#define NVS_KEY_SVC_COUNT "svc_count"         // Chave para número de serviços
#define NVS_KEY_SVC_NAME_PREFIX "svc_%d_n"    // Prefixo para nome do serviço (indexado)
#define NVS_KEY_SVC_SECRET_PREFIX "svc_%d_s"  // Prefixo para segredo do serviço (indexado)
#define NVS_KEY_LANGUAGE "lang"               // Chave para idioma salvo
#define NVS_KEY_TZ_OFFSET "tz_offs"           // Chave para fuso horário salvo

// ============================================================================
// === JSON KEYS (for Serial Input) ===
// ============================================================================
#define JSON_KEY_SERVICE_NAME "name"
#define JSON_KEY_SERVICE_SECRET "secret"
#define JSON_KEY_TIME_YEAR "y"
#define JSON_KEY_TIME_MONTH "mo"
#define JSON_KEY_TIME_DAY "d"
#define JSON_KEY_TIME_HOUR "h"
#define JSON_KEY_TIME_MINUTE "m"
#define JSON_KEY_TIME_SECOND "s"