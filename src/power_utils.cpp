// power_utils.cpp
#include "power_utils.h"
#include "globals.h"    // Acesso a battery_info, last_interaction_time, current_target_brightness
#include "config.h"     // Acesso a constantes de pino, brilho, bateria, PWM
#include <Arduino.h>    // Para pinMode, digitalWrite, analogRead, millis, ledc

// Flag para saber se o LEDC foi configurado
static bool ledc_backlight_configured = false;

// ---- Inicialização ----
void power_init() {
    // Pino para manter o dispositivo ligado (se aplicável, como no T-Display S3 LDO_EN)
    #ifdef PIN_POWER_ON
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    Serial.println("[Power] Pino POWER_ON ativado.");
    #endif

    // Pino ADC para leitura da tensão da bateria
    #ifdef PIN_BAT_VOLT
    pinMode(PIN_BAT_VOLT, INPUT); // Ou ANALOG se a biblioteca ADC exigir
    // adcAttachPin(PIN_BAT_VOLT); // Para ESP32, pode ser necessário configurar a atenuação do ADC
    // analogSetAttenuation(ADC_11db); // Exemplo: atenuação para ler até ~3.3V (ou mais com divisor)
    Serial.println("[Power] Pino BAT_VOLT configurado para leitura ADC.");
    #endif

     #ifdef PIN_LCD_BL
        // Tenta configurar PWM de hardware (ledc)
        // ledcSetup retorna a frequência real obtida, ou 0 em caso de erro.
        double actual_freq = ledcSetup(BL_PWM_CHANNEL, BL_PWM_FREQ, BL_PWM_RESOLUTION);

        if (actual_freq == 0) { // Verifica se ledcSetup falhou
             Serial.printf("[Power ERROR] Falha ao configurar LEDC para backlight no canal %d.\n", BL_PWM_CHANNEL);
             // Fallback para controle ON/OFF simples se o PWM falhar
             pinMode(PIN_LCD_BL, OUTPUT);
             digitalWrite(PIN_LCD_BL, HIGH); // Liga por padrão
             ledc_backlight_configured = false;
        } else {
            // ledcSetup funcionou (retornou frequência > 0)
             Serial.printf("[Power] LEDC Canal %d configurado com Freq %.2f Hz (solicitado %d Hz).\n", BL_PWM_CHANNEL, actual_freq, BL_PWM_FREQ);

            // Agora, anexa o pino ao canal configurado.
            // ledcAttachPin retorna void, então apenas chamamos a função.
            ledcAttachPin(PIN_LCD_BL, BL_PWM_CHANNEL);

            // Assumimos que o attach funcionou porque o setup funcionou.
            Serial.printf("[Power] Backlight LCD (pino %d) anexado ao LEDC (Canal %d).\n",
                          PIN_LCD_BL, BL_PWM_CHANNEL);
            ledc_backlight_configured = true;
            power_set_screen_brightness(BRIGHTNESS_BATTERY); // Define um brilho inicial
        }
    #else
        Serial.println("[Power WARN] Pino PIN_LCD_BL não definido. Controle de brilho desabilitado.");
    #endif
    power_update_battery_status(); // Leitura inicial
}

// ---- Leitura da Bateria ----
void power_update_battery_status() {
    #ifdef PIN_BAT_VOLT
    int adc_value = analogRead(PIN_BAT_VOLT);

    // --- IMPORTANTE: AJUSTE ESTA FÓRMULA PARA SEU HARDWARE ESPECÍFICO ---
    // A fórmula depende do divisor de tensão (se houver) e da referência do ADC.
    // Exemplo para T-Display S3 (divisor por 2, Vref ~3.3V, ADC 12-bit):
    // Voltagem no pino ADC = (adc_value / 4095.0) * 3.3;
    // Voltagem real da bateria = Voltagem no pino ADC * 2.0; (se R1=R2 no divisor)
    // Para simplificar, vamos assumir que a leitura direta já é a tensão da bateria
    // ou que o divisor e Vref resultam em uma conversão mais direta.
    // Este é um ponto CRÍTICO para precisão.
    battery_info.voltage = (adc_value / 4095.0f) * 3.3f * 2.0f; // Exemplo comum T-Display S3 (divisor 100k/100k)

    // Determina se está alimentado por USB
    battery_info.is_usb_powered = (battery_info.voltage >= USB_THRESHOLD_VOLTAGE);

    if (battery_info.is_usb_powered) {
        battery_info.level_percent = 100; // Assume 100% se no USB (ou tensão de carga completa)
    } else {
        // Mapeamento linear da tensão para percentual
        // (BATTERY_MIN_VOLTAGE é o 0%, BATTERY_MAX_VOLTAGE é o 100%)
        float mapped_level = map(battery_info.voltage * 100.0f,
                                 BATTERY_MIN_VOLTAGE * 100.0f,
                                 BATTERY_MAX_VOLTAGE * 100.0f,
                                 0.0f, 100.0f);
        battery_info.level_percent = constrain((int)mapped_level, 0, 100);
    }
    // Serial.printf("[Power] Bateria: %.2fV, USB: %d, Nível: %d%%\n",
                //   battery_info.voltage, battery_info.is_usb_powered, battery_info.level_percent);
    #else
    // Se não houver pino de leitura de bateria, assume sempre USB e 100%
    battery_info.voltage = 5.0; // Valor simbólico
    battery_info.is_usb_powered = true;
    battery_info.level_percent = 100;
    #endif
}

// ---- Controle de Brilho ----
void power_set_screen_brightness(uint8_t level) {
    #ifdef PIN_LCD_BL
    // Garante que o nível esteja dentro dos limites (0-BRIGHTNESS_MAX_LEVEL)
    // BRIGHTNESS_MAX_LEVEL é 255 para resolução de 8 bits do PWM.
    uint8_t actual_level = constrain(level, BRIGHTNESS_OFF, BRIGHTNESS_MAX_LEVEL);

    if (ledc_backlight_configured) {
        // Mapeia o nível (0-BRIGHTNESS_MAX_LEVEL) para o duty cycle do LEDC
        // Se BRIGHTNESS_MAX_LEVEL já for o máximo da resolução (ex: 255 para 8 bits),
        // o mapeamento é direto.
        // uint32_t duty_cycle = map(actual_level, 0, BRIGHTNESS_MAX_LEVEL, 0, (1 << BL_PWM_RESOLUTION) - 1);
        // Como BRIGHTNESS_MAX_LEVEL é 255 e BL_PWM_RESOLUTION é 8, o duty_cycle é o próprio actual_level.
        ledcWrite(BL_PWM_CHANNEL, actual_level);
    } else {
        // Fallback: Controle ON/OFF simples se o PWM não estiver configurado
        digitalWrite(PIN_LCD_BL, (actual_level > BRIGHTNESS_OFF) ? HIGH : LOW);
    }
    // Serial.printf("[Power] Brilho definido para: %d (HW PWM: %s)\n", actual_level, ledc_backlight_configured ? "SIM" : "NAO");
    #endif
}

void power_update_target_brightness() {
    uint8_t new_target_brightness;

    if (battery_info.is_usb_powered) {
        new_target_brightness = BRIGHTNESS_USB;
    } else {
        if (millis() - last_interaction_time > INACTIVITY_TIMEOUT_MS) {
            new_target_brightness = BRIGHTNESS_DIMMED;
        } else {
            new_target_brightness = BRIGHTNESS_BATTERY;
        }
    }

    if (new_target_brightness != current_target_brightness) {
        current_target_brightness = new_target_brightness;
        power_set_screen_brightness(current_target_brightness);
    }
}