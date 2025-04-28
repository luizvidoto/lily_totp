#pragma once // Include guard

#include <stdint.h> // Para uint8_t

// ============================================================================
// === FUNÇÕES PÚBLICAS DO MÓDULO HARDWARE ===
// ============================================================================

// --- Funções de Inicialização (Chamadas no Setup) ---

/**
 * @brief Inicializa hardware base: pinos GPIO básicos, I2C (Wire), SPI.
 */
void initBaseHardware();

/**
 * @brief Inicializa o display TFT (tft.init, rotação, limpeza inicial).
 */
void initDisplay();

/**
 * @brief Inicializa o módulo RTC DS3231.
 * @return true se o RTC foi encontrado e inicializado com sucesso, false caso contrário.
 */
bool initRTC();

/**
 * @brief Configura o pino ADC para leitura da voltagem da bateria.
 */
void initBatteryMonitor();

/**
 * @brief Configura o controle de brilho (PWM por Software ou Hardware - ledc).
 */
void initBrightnessControl();

/**
 * @brief Inicializa o leitor RFID MFRC522.
 */
void initRFID();


// --- Funções de Controle e Leitura (Chamadas no Loop ou sob Demanda) ---

/**
 * @brief Lê a hora atual do RTC e atualiza o relógio interno do sistema (TimeLib).
 */
void updateTimeFromRTC();

/**
 * @brief Grava a hora atual do sistema (TimeLib) no hardware RTC.
 */
void updateRTCFromSystem();

/**
 * @brief Lê a voltagem da bateria, calcula o percentual e atualiza a struct global battery_info.
 */
void updateBatteryStatus();

/**
 * @brief Define o nível de brilho da tela (implementação SW ou HW PWM).
 * @param level Nível de brilho (0 a MAX definido em config.h, ou 0-255 se usando HW PWM).
 */
void setScreenBrightness(uint8_t level);

/**
 * @brief Verifica o estado de alimentação e inatividade para ajustar o brilho automaticamente.
 *        Chama setScreenBrightness() com o nível apropriado.
 */
void updateScreenBrightness();

/**
 * @brief Tenta ler um cartão RFID. Se um novo cartão for lido com sucesso,
 *        armazena seu UID na struct global temp_data.rfid_card_id e
 *        sinaliza para redesenhar a tela (request_full_redraw = true).
 * @return true se um novo cartão foi lido com sucesso, false caso contrário.
 */
bool readRFIDCard();

/**
 * @brief Desliga a antena do módulo RFID para economizar energia.
 */
void powerDownRFID();

/**
 * @brief Liga a antena do módulo RFID.
 */
void powerUpRFID(); // Função complementar a powerDownRFID