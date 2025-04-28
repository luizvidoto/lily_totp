#include "hardware.h"
#include "globals.h" // Acessa objetos tft, rtc, mfrc522, battery_info, etc.
#include "config.h"  // Acessa pinos, constantes de brilho, bateria, etc.
#include <Wire.h>    // Para I2C (RTC)
#include <SPI.h>     // Para SPI (RFID)
#include <driver/adc.h> // Para configurar atenuação do ADC (melhora leitura de bateria)
#include <TimeLib.h> // Para TimeLib (RTC, hora atual, etc.)

// ============================================================================
// === DEFINIÇÕES INTERNAS E VARIÁVEIS ESTÁTICAS ===
// ============================================================================

// --- Controle de Brilho (Software PWM - Fallback) ---
#ifndef HW_PWM_BL // Compila este bloco apenas se HW_PWM_BL não estiver definido
static uint8_t current_sw_pwm_level = 0;
const uint8_t MAX_SW_PWM_LEVEL = 16; // Nível máximo para simulação SW

// Função auxiliar para simular PWM por software (pode causar flicker)
static void pulsePinLed() {
    if (current_sw_pwm_level == 0) {
        digitalWrite(PIN_LCD_BL, LOW);
    } else if (current_sw_pwm_level >= MAX_SW_PWM_LEVEL) {
        digitalWrite(PIN_LCD_BL, HIGH);
    } else {
        // Cria pulsos para escurecer
        digitalWrite(PIN_LCD_BL, LOW);
        delayMicroseconds(2); // Pequeno tempo desligado
        digitalWrite(PIN_LCD_BL, HIGH);
        // O tempo ligado é controlado pela frequência de chamada desta função
        // e pelo 'duty cycle' implícito dado por current_sw_pwm_level / MAX_SW_PWM_LEVEL
        // Uma implementação mais robusta usaria timers.
    }
}
#else // Compila este bloco se HW_PWM_BL estiver definido
// --- Controle de Brilho (Hardware PWM - LEDC) ---
#include <driver/ledc.h>
const int LEDC_CHANNEL_BL = 0;      // Canal LEDC para o backlight (0-7 para high speed)
const int LEDC_TIMER_BIT = 8;       // Resolução do PWM (8 bits = 0-255)
const int LEDC_BASE_FREQ = 5000;    // Frequência do PWM (Hz)
const uint32_t LEDC_MAX_DUTY = (1 << LEDC_TIMER_BIT) - 1; // 255 para 8 bits
#endif // HW_PWM_BL

// ============================================================================
// === INICIALIZAÇÃO DE HARDWARE ===
// ============================================================================

void initBaseHardware() {
    // Configura pinos de controle essenciais
#ifdef PIN_POWER_ON
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH); // Garante alimentação (se aplicável)
    Serial.println("[HW] Power Latch ON");
#endif

    // Inicializa I2C para RTC (e talvez outros sensores)
    // Use os pinos definidos em pin_config.h
    bool i2c_ok = Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    if (i2c_ok) {
        Serial.printf("[HW] I2C inicializado (SDA: %d, SCL: %d)\n", PIN_IIC_SDA, PIN_IIC_SCL);
    } else {
        Serial.println("[HW][ERRO] Falha ao iniciar I2C!");
        // Pode ser necessário tratar esse erro de forma mais robusta
    }

    // Inicializa SPI para RFID (e talvez display ou SD)
    // Use os pinos definidos em pin_config.h
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, -1); // CS é gerenciado pela lib MFRC522 ou outra
    Serial.printf("[HW] SPI inicializado (SCK: %d, MISO: %d, MOSI: %d)\n", SCK_PIN, MISO_PIN, MOSI_PIN);
}

void initDisplay() {
    tft.init();
    tft.setRotation(SCREEN_ROTATION);
    tft.fillScreen(COLOR_BG); // Limpa tela inicial
    tft.setTextColor(COLOR_FG, COLOR_BG); // Define cores padrão
    tft.setTextSize(1); // Define tamanho padrão
    tft.setTextDatum(TL_DATUM); // Define datum padrão (Top Left)
    Serial.println("[HW] Display TFT inicializado.");

    // Backlight precisa ser inicializado separadamente
    initBrightnessControl();
}

bool initRTC() {
    if (!rtc.begin()) {
        Serial.println("[HW][ERRO] RTC DS3231 não encontrado!");
        rtc_available = false;
        return false;
    }

    if (rtc.lostPower()) {
        Serial.println("[HW][WARN] RTC perdeu energia (bateria fraca/ausente?). Hora pode estar incorreta.");
        // Opcional: Forçar ajuste de hora aqui ou definir uma hora padrão.
        // Ex: rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Ajusta para hora da compilação
    } else {
        // Verifica se a hora do RTC é razoável (ex: ano > 2023)
        DateTime now = rtc.now();
        if (now.year() < 2023) {
             Serial.println("[HW][WARN] Hora do RTC parece inválida. Ajuste necessário.");
             // Poderia retornar false ou tentar ajustar para hora da compilação
        }
    }

    rtc.disable32K(); // Desativa saída de 32KHz não usada
    rtc.clearAlarm(1); // Limpa flags de alarme
    rtc.clearAlarm(2);
    rtc.writeSqwPinMode(DS3231_OFF); // Desliga saída SQW

    Serial.println("[HW] RTC DS3231 inicializado.");
    rtc_available = true;
    return true;
}

void initBatteryMonitor() {
    // Configura o pino ADC
    pinMode(PIN_BAT_VOLT, INPUT);

    // Configura a atenuação do ADC para permitir leitura de até ~3.3V (ou mais dependendo do Vref)
    // ADC_ATTEN_DB_11: Faixa de ~150mV a 2450mV (Verificar documentação ESP32 para seu Vref)
    // Se seu divisor de tensão resultar em mais que ~2.45V na entrada ADC com bateria cheia,
    // pode ser necessário ajustar o Vref ou usar outra atenuação.
    // Para o T-Display S3 (ESP32-S3), a faixa com 11dB é maior, até ~2.5V tipicamente.
    // Verifique a documentação específica do seu ESP32.
    #if CONFIG_IDF_TARGET_ESP32 // Para ESP32 original
        adc1_config_width(ADC_WIDTH_BIT_12); // Resolução de 12 bits
        adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // Exemplo: GPIO34 é ADC1_CH6
    #elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
        // S2/S3 podem usar adc_oneshot_config_channel ou equivalente. Simplificando:
        adc1_config_width(ADC_WIDTH_BIT_12);
        // A função adc1_config_channel_atten pode precisar de ajustes para S2/S3 ou usar API mais nova.
        // Exemplo genérico (VERIFICAR CANAL CORRETO PARA PIN_BAT_VOLT):
        // adc_atten_t atten = ADC_ATTEN_DB_11;
        // adc_unit_t unit = ADC_UNIT_1; // Ou ADC_UNIT_2 dependendo do pino
        // int channel = adc_channel_io_map[unit][PIN_BAT_VOLT]; // Mapeamento pino->canal (complexo)
        // Simplificação (assumindo ADC1 e atenuação 11dB funciona):
         if (digitalPinToAnalogChannel(PIN_BAT_VOLT) != -1) {
            adc1_config_channel_atten((adc1_channel_t)digitalPinToAnalogChannel(PIN_BAT_VOLT), ADC_ATTEN_DB_12);
         } else {
             Serial.println("[HW][ERRO] Pino de bateria não é um canal ADC válido!");
         }
    #else
         // Configuração para outras variantes do ESP32, se necessário
    #endif

    Serial.printf("[HW] Monitor de Bateria inicializado (Pino: %d, Atenuação: 11dB)\n", PIN_BAT_VOLT);
    updateBatteryStatus(); // Faz a primeira leitura
}

void initBrightnessControl() {
    Serial.print("[HW] Inicializando controle de brilho... ");
#ifndef HW_PWM_BL
    // --- Software PWM ---
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, LOW); // Começa desligado
    current_sw_pwm_level = 0;
    Serial.println("Modo: Software PWM (Fallback).");
#else
    // --- Hardware PWM (LEDC) ---
    ledcSetup(LEDC_CHANNEL_BL, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
    ledcAttachPin(PIN_LCD_BL, LEDC_CHANNEL_BL);
    ledcWrite(LEDC_CHANNEL_BL, 0); // Começa desligado
    Serial.printf("Modo: Hardware LEDC (Canal: %d, Freq: %dHz, Res: %dbit).\n",
                  LEDC_CHANNEL_BL, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
#endif
    current_brightness_level = 0;
}

void initRFID() {
    mfrc522.PCD_Init(); // Inicializa o MFRC522
    delay(4); // Pequeno delay após init recomendado
    mfrc522.PCD_DumpVersionToSerial(); // Mostra versão do firmware no Serial
    powerDownRFID(); // Começa com a antena desligada para economizar energia
    Serial.println("[HW] RFID MFRC522 inicializado (antena desligada).");
}

// ============================================================================
// === CONTROLE E LEITURA DE HARDWARE ===
// ============================================================================

void updateTimeFromRTC() {
    if (rtc_available) {
        setTime(rtc.now().unixtime());
        // Serial.println("[HW] TimeLib sincronizado com RTC.");
    } else {
        // Serial.println("[HW] RTC indisponível, não foi possível sincronizar TimeLib.");
    }
}

void updateRTCFromSystem() {
    if (rtc_available) {
        // TimeLib sempre armazena UTC. DateTime do RTClib também espera UTC.
        rtc.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
        Serial.println("[HW] RTC HW atualizado com hora do sistema (UTC).");
    } else {
        Serial.println("[HW] RTC indisponível, não foi possível ajustar hora.");
    }
}

void updateBatteryStatus() {
    int adcValue = analogRead(PIN_BAT_VOLT);
    battery_info.voltage = adcValue * BATT_ADC_CONVERSION_FACTOR; // Usa fator de config.h

    // Lógica para determinar estado USB e percentual
    battery_info.is_usb_powered = (battery_info.voltage >= BATT_VOLTAGE_USB_THRESHOLD);

    if (battery_info.is_usb_powered) {
        battery_info.level_percent = 100; // Ou um valor especial, como 101, para indicar carregando?
    } else {
        // Mapeamento linear da tensão para percentual
        // Multiplica por 100 antes de converter para int para manter precisão
        int voltage_mv = battery_info.voltage * 1000;
        int min_mv = BATT_VOLTAGE_MIN * 1000;
        int max_mv = BATT_VOLTAGE_MAX * 1000;
        // Evita divisão por zero se min == max
        if (max_mv <= min_mv) {
            battery_info.level_percent = (voltage_mv > min_mv) ? 100 : 0;
        } else {
             battery_info.level_percent = map(voltage_mv, min_mv, max_mv, 0, 100);
        }
        battery_info.level_percent = constrain(battery_info.level_percent, 0, 100); // Garante 0-100%
    }
    // Serial.printf("[BATT] ADC: %d, V: %.2fV, USB: %d, Nivel: %d%%\n", adcValue, battery_info.voltage, battery_info.is_usb_powered, battery_info.level_percent);
}

void setScreenBrightness(uint8_t level) {
    // Garante que o nível esteja dentro dos limites esperados
#ifndef HW_PWM_BL
    level = constrain(level, 0, MAX_SW_PWM_LEVEL);
    if (level == current_brightness_level) return; // Otimização: Sem mudança
    current_sw_pwm_level = level;
    pulsePinLed(); // Aplica o nível SW PWM (pode ser chamado mais frequentemente por um timer)
#else
    level = constrain(level, 0, 255); // Nível 0-255 para HW PWM 8 bits
    if (level == current_brightness_level) return; // Otimização: Sem mudança
    // Mapeia 0-255 para o duty cycle do LEDC (se resolução for diferente de 8 bits)
    // Se LEDC_TIMER_BIT for 8, duty = level. Se for 10, duty = map(level, 0, 255, 0, 1023) etc.
    uint32_t duty = map(level, 0, 255, 0, LEDC_MAX_DUTY);
    ledcWrite(LEDC_CHANNEL_BL, duty);
#endif

    // Serial.printf("[HW] Brilho definido para %d\n", level);
    current_brightness_level = level;
}

void updateScreenBrightness() {
    uint8_t target_brightness;

    if (battery_info.is_usb_powered) {
        target_brightness = BRIGHTNESS_USB;
    } else {
        // Verifica inatividade (somente se em bateria)
        if (millis() - last_interaction_time > INACTIVITY_TIMEOUT_MS) {
            target_brightness = BRIGHTNESS_DIMMED;
        } else {
            target_brightness = BRIGHTNESS_BATTERY;
        }
    }

    // Aplica o brilho apenas se for diferente do atual
    if (target_brightness != current_brightness_level) {
        setScreenBrightness(target_brightness);
    }

#ifndef HW_PWM_BL
    // Se usando SW PWM, precisa chamar pulsePinLed() periodicamente
    // Poderia ser chamado aqui ou em um timer de alta frequência.
    // Chamando aqui pode não ser ideal para flicker.
    pulsePinLed();
#endif
}

bool readRFIDCard() {
    // Verifica se há um novo cartão presente
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return false;
    }

    // Tenta ler o UID do cartão
    if (!mfrc522.PICC_ReadCardSerial()) {
        // Falha na leitura (colisão, cartão removido rápido demais, etc.)
        mfrc522.PICC_HaltA(); // Tenta parar a comunicação atual
        mfrc522.PCD_StopCrypto1();
        return false;
    }

    // UID lido com sucesso, armazena em temp_data
    char* uid_ptr = temp_data.rfid_card_id;
    size_t buffer_size = sizeof(temp_data.rfid_card_id);
    memset(uid_ptr, 0, buffer_size); // Limpa buffer

    for (byte i = 0; i < mfrc522.uid.size && (i * 2 + 2) < buffer_size; i++) {
        snprintf(uid_ptr + (i * 2), 3, "%02X", mfrc522.uid.uidByte[i]);
    }
    Serial.printf("[HW] RFID Card Read: %s\n", temp_data.rfid_card_id);

    // Sinaliza para redesenhar a tela RFID
    request_full_redraw = true;

    // É importante parar a comunicação com o cartão para permitir a leitura de outros
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    return true; // Novo cartão lido com sucesso
}

void powerDownRFID() {
    mfrc522.PCD_AntennaOff(); // Desliga o campo RF da antena
    // Opcional: Colocar o CI MFRC522 em modo de baixo consumo, se suportado e necessário
    // mfrc522.PCD_SoftPowerDown(); // Verificar se sua biblioteca suporta
    Serial.println("[HW] RFID Antenna OFF.");
}

void powerUpRFID() {
    mfrc522.PCD_AntennaOn(); // Liga o campo RF da antena
    // Se usou SoftPowerDown, pode precisar de um SoftPowerUp ou re-init
    delay(2); // Pequeno delay para estabilizar
    Serial.println("[HW] RFID Antenna ON.");
}