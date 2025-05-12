// power_utils.h
#ifndef POWER_UTILS_H
#define POWER_UTILS_H

#include <stdint.h> // Para uint8_t

// ---- Inicialização ----
// Configura pinos relacionados à energia (ex: POWER_ON, ADC da bateria, PWM do backlight).
void power_init();

// ---- Leitura da Bateria ----
// Lê a tensão da bateria, determina se está alimentado por USB e calcula o percentual.
// Atualiza a struct global 'battery_info'.
void power_update_battery_status();

// ---- Controle de Brilho ----
// Define o brilho da tela (0-BRIGHTNESS_MAX_LEVEL).
// Usa PWM de hardware (ledc) se configurado, senão pode ter um fallback (não ideal).
void power_set_screen_brightness(uint8_t level);

// Determina o nível de brilho alvo com base na alimentação (USB/bateria) e inatividade.
// Chama power_set_screen_brightness() para aplicar.
// Deve ser chamado regularmente no loop.
void power_update_target_brightness();

#endif // POWER_UTILS_H