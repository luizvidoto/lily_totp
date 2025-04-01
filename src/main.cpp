#include <Arduino.h>
#include <TFT_eSPI.h>
#include "OneButton.h"
#include <TimeLib.h>
#include <Preferences.h>
#include <Wire.h>
#include <RTClib.h>
#include "mbedtls/md.h"

// --- CONSTANTES E CONFIGURAÇÕES ---
// Limites para serviços e chaves
#define MAX_SERVICES 300
#define MAX_SECRET_B32_LENGTH 64    // tamanho máximo da chave em Base32
#define MAX_SECRET_BIN_LENGTH 40    // tamanho máximo da chave decodificada

// Intervalos e tempos
const uint32_t TOTP_INTERVAL = 30;            // intervalo TOTP em segundos
const uint32_t CLOCK_UPDATE_INTERVAL = 1000;    // atualização do relógio (1s)
const uint32_t SYNC_INTERVAL = 60000;           // sincronização RTC a cada 60s
const uint32_t INACTIVITY_TIMEOUT = 30000;      // 30 segundos para reduzir brilho

// --- Pinos de hardware ---
// Definidos no arquivo "pin_config.h"
#include "pin_config.h"

// --- ESTRUTURAS ---
// Estrutura para armazenar serviço TOTP
struct TOTPService {
    char name[21];                       // nome do serviço (20 caracteres + '\0')
    char secret[MAX_SECRET_B32_LENGTH];  // chave em Base32
};

// --- VARIÁVEIS GLOBAIS ---
TFT_eSPI tft = TFT_eSPI();
OneButton btnDec(PIN_BUTTON_0, true);
OneButton btnInc(PIN_BUTTON_1, true);
Preferences preferences;
RTC_DS3231 rtc;

// Serviços e controle TOTP
TOTPService services[MAX_SERVICES];
int serviceCount = 0;
int currentServiceIndex = 0;
char lastTOTP[7] = "";     // código TOTP atual (6 dígitos + '\0')
uint32_t currentTOTPInterval = 0;

// Buffers temporários para criação de serviço
char tempServiceName[21];
char tempServiceSecret[MAX_SECRET_B32_LENGTH];

// Estados da tela
enum ScreenState {
    SCREEN_TOTP,
    SCREEN_MENU,
    SCREEN_CREATE_SERVICE,
    SCREEN_CREATE_SERVICE_CONFIRM,
    SCREEN_EDIT_TIME,
    SCREEN_DELETE_CONFIRM
};
ScreenState currentScreen = SCREEN_MENU;

// Menu principal
const int NUM_MENU_OPTIONS = 3;
const char* menuOptions[NUM_MENU_OPTIONS] = {
    "Criar novo servico",
    "Ver codigos",
    "Editar hora"
};
int currentMenuIndex = 0;

// Variáveis para edição de hora
int editHour, editMinute, editSecond;
int editField = 0; // 0: hora, 1: minuto, 2: segundo

// Controle de tempo e interação
unsigned long lastUpdateTime = 0;
unsigned long lastMenuUpdateTime = 0;
unsigned long lastSyncTime = 0;
unsigned long lastInteractionTime = 0;
uint8_t currentBrightness = 8;
const uint8_t maxBrightness = 16;

// --- FUNÇÕES DE UTILIDADE ---
// Decodifica string Base32 para binário; retorna o tamanho decodificado.
// Foram adicionadas verificações para ignorar caracteres inválidos.
int base32decode(const char *encoded, byte *result, int bufSize) {
    int buffer = 0, bitsLeft = 0, count = 0;
    const char *ptr = encoded;
    while (*ptr && count < bufSize) {
        uint8_t ch = *ptr++;
        if (ch >= 'A' && ch <= 'Z') ch -= 'A';
        else if (ch >= '2' && ch <= '7') ch = ch - '2' + 26;
        else if (ch >= 'a' && ch <= 'z') ch -= 'a';
        else continue; // ignora caracteres inválidos
        buffer = (buffer << 5) | ch;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            result[count++] = (byte)(buffer >> (bitsLeft - 8));
            bitsLeft -= 8;
        }
    }
    return count;
}

// Gera código TOTP usando HMAC-SHA1, seguindo as especificações do algoritmo.
uint32_t generateTOTP(const uint8_t *key, size_t keyLength, uint64_t timestamp, uint32_t interval = TOTP_INTERVAL) {
    uint64_t counter = timestamp / interval;
    uint8_t counterBytes[8];
    for (int i = 7; i >= 0; i--) {
        counterBytes[i] = counter & 0xFF;
        counter >>= 8;
    }
    uint8_t hash[20];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
    mbedtls_md_hmac_starts(&ctx, key, keyLength);
    mbedtls_md_hmac_update(&ctx, counterBytes, 8);
    mbedtls_md_hmac_finish(&ctx, hash);
    mbedtls_md_free(&ctx);
    int offset = hash[19] & 0x0F;
    uint32_t binaryCode = ((hash[offset] & 0x7F) << 24) |
                          ((hash[offset + 1] & 0xFF) << 16) |
                          ((hash[offset + 2] & 0xFF) << 8)  |
                          (hash[offset + 3] & 0xFF);
    return binaryCode % 1000000;  // código de 6 dígitos
}

// --- GESTÃO DE ENERGIA ---
// Leitura da tensão da bateria (ajuste conforme divisor de tensão)
float readBatteryVoltage() {
    int adcValue = analogRead(PIN_BAT_VOLT);
    return (adcValue / 4095.0) * 4.2;
}

// Retorna verdadeiro se estiver alimentado por bateria (tensão inferior a 4.10V)
bool isBatteryMode() {
    return (readBatteryVoltage() < 4.10);
}

// Ajusta o brilho do display; aqui pode ser aprimorado para uma transição mais suave.
void setBrightness(uint8_t value) {
    if (value > maxBrightness) value = maxBrightness;
    currentBrightness = value;
    if (currentBrightness == 0) {
        digitalWrite(PIN_LCD_BL, LOW);
    } else {
        digitalWrite(PIN_LCD_BL, HIGH);
    }
}

// --- ARMAZENAMENTO (NVS) ---
// Salva a quantidade de serviços cadastrados.
void saveServiceCount() {
    preferences.putInt("svcCount", serviceCount);
}

// Carrega os serviços do NVS, validando se os dados existem.
void loadServices() {
    preferences.begin("totp", false);
    serviceCount = preferences.getInt("svcCount", 0);
    for (int i = 0; i < serviceCount; i++) {
        char keyName[20], keySecret[20];
        sprintf(keyName, "svc%d_name", i);
        sprintf(keySecret, "svc%d_secret", i);
        String name = preferences.getString(keyName, "");
        String secret = preferences.getString(keySecret, "");
        if (name.length() > 0 && secret.length() > 0) {
            strncpy(services[i].name, name.c_str(), sizeof(services[i].name) - 1);
            services[i].name[sizeof(services[i].name) - 1] = '\0';
            strncpy(services[i].secret, secret.c_str(), sizeof(services[i].secret) - 1);
            services[i].secret[sizeof(services[i].secret) - 1] = '\0';
        }
    }
    preferences.end();
}

// Salva um novo serviço no NVS.
void saveNewService(const char *name, const char *secret) {
    preferences.begin("totp", false);
    char keyName[20], keySecret[20];
    sprintf(keyName, "svc%d_name", serviceCount);
    sprintf(keySecret, "svc%d_secret", serviceCount);
    preferences.putString(keyName, name);
    preferences.putString(keySecret, secret);
    serviceCount++;
    preferences.putInt("svcCount", serviceCount);
    preferences.end();
}

// Remove o serviço atual e atualiza os dados armazenados.
void deleteCurrentService() {
    if (serviceCount == 0) return;
    for (int i = currentServiceIndex; i < serviceCount - 1; i++) {
        services[i] = services[i + 1];
    }
    serviceCount--;
    preferences.begin("totp", false);
    preferences.putInt("svcCount", serviceCount);
    for (int i = 0; i < serviceCount; i++) {
        char keyName[20], keySecret[20];
        sprintf(keyName, "svc%d_name", i);
        sprintf(keySecret, "svc%d_secret", i);
        preferences.putString(keyName, services[i].name);
        preferences.putString(keySecret, services[i].secret);
    }
    preferences.end();
    if (serviceCount > 0)
        currentServiceIndex %= serviceCount;
    else
        lastTOTP[0] = '\0';
}

// --- INTERFACE GRÁFICA (UI) ---
// Definição de regiões fixas para atualização parcial (minimiza flicker)
#define CLOCK_X 225
#define CLOCK_Y 156
#define CLOCK_WIDTH 96
#define CLOCK_HEIGHT 17

#define TOTP_X 108
#define TOTP_Y 44
#define TOTP_WIDTH 109
#define TOTP_HEIGHT 25

#define PROGRESS_BAR_X 41
#define PROGRESS_BAR_Y 101
#define PROGRESS_BAR_WIDTH 239
#define PROGRESS_BAR_HEIGHT 11

// Atualiza somente a área do relógio na tela
void updateClockUI() {
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", hour(now()), minute(now()), second(now()));
    tft.startWrite();
    tft.fillRect(CLOCK_X - 10, CLOCK_Y - 10, CLOCK_WIDTH + 20, CLOCK_HEIGHT + 20, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString(timeStr, CLOCK_X, CLOCK_Y);
    tft.endWrite();
}

// Desenha a tela TOTP; se fullRedraw==true desenha a tela completa (nome, contorno, instruções)
void drawTOTPUI(bool fullRedraw) {
    tft.startWrite();
    if(fullRedraw) {
        tft.fillScreen(TFT_BLACK);
        // Nome do serviço
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if(serviceCount > 0)
            tft.drawString(services[currentServiceIndex].name, 3, 3);
        else
            tft.drawString("Sem servicos", 3, 3);
        // Contorno da barra de tempo
        tft.drawRoundRect(PROGRESS_BAR_X - 2, PROGRESS_BAR_Y - 2, PROGRESS_BAR_WIDTH + 4, PROGRESS_BAR_HEIGHT + 4, 2, TFT_WHITE);
        // Instrução para deletar
        tft.setTextSize(1);
        tft.drawString("Long press para deletar", 4, tft.height()-15);
    }
    // Atualiza o código TOTP
    tft.fillRect(TOTP_X - 2, TOTP_Y - 2, TOTP_WIDTH + 4, TOTP_HEIGHT + 4, TFT_BLACK);
    tft.setTextSize(3);
    tft.drawString(lastTOTP, TOTP_X, TOTP_Y);
    // Atualiza a barra de progresso do tempo
    long nowTime = now();
    uint32_t secondsElapsed = nowTime % TOTP_INTERVAL;
    uint32_t secondsRemaining = TOTP_INTERVAL - secondsElapsed;
    int progress = map(secondsRemaining, 0, TOTP_INTERVAL, 0, PROGRESS_BAR_WIDTH);
    tft.fillRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, TFT_BLACK);
    tft.fillRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, progress, PROGRESS_BAR_HEIGHT, TFT_WHITE);
    // Atualiza relógio na área inferior direita
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", hour(now()), minute(now()), second(now()));
    tft.fillRect(CLOCK_X, CLOCK_Y, CLOCK_WIDTH, CLOCK_HEIGHT, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString(timeStr, CLOCK_X, CLOCK_Y);
    tft.endWrite();
}

// Desenha a tela de menu principal
void drawMenuUI() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    // Título e relógio
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Menu", 10, 10);
    updateClockUI();
    // Opções do menu
    int startY = 40, lineHeight = 30;
    for (int i = 0; i < NUM_MENU_OPTIONS; i++) {
        if (i == currentMenuIndex)
            tft.setTextColor(TFT_BLACK, TFT_WHITE);
        else
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(2);
        tft.drawString(menuOptions[i], 20, startY + i * lineHeight);
    }
    // Exibe status de serviços
    char status[20];
    sprintf(status, "Servicos: %d/%d", serviceCount, MAX_SERVICES);
    tft.setTextSize(1);
    tft.drawString(status, 10, tft.height()-15);
    tft.endWrite();
}

// Tela para criação de serviço via JSON
void drawCreateServiceUI() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Criar Servico", 10, 10);
    tft.setTextSize(1);
    tft.drawString("Aguardando JSON via Serial:", 10, 40);
    tft.drawString("{\"name\":\"...\",", 10, 55);
    tft.drawString(" \"secret\":\"...\"}", 10, 70);
    tft.drawString("Double-click para menu", 10, tft.height()-15);
    tft.endWrite();
}

// Tela de confirmação para novo serviço
void drawCreateServiceConfirmUI(const char *serviceName) {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Adicionar:", 10, 30);
    tft.drawString(serviceName, 10, 60);
    tft.setTextSize(1);
    tft.drawString("Btn0: Cancelar / Btn1: Confirmar", 10, 90);
    tft.drawString("Double-click para menu", 10, tft.height()-15);
    tft.endWrite();
}

// Tela de edição de hora
void drawEditTimeUI() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Editar Hora", 10, 10);
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", editHour, editMinute, editSecond);
    tft.setTextSize(3);
    tft.drawString(timeStr, 20, 50);
    // Marcador para campo em edição
    int markerY = 90, fieldWidth = 30;
    tft.fillRect(20 + editField * (fieldWidth + 10), markerY, fieldWidth, 3, TFT_WHITE);
    tft.setTextSize(1);
    tft.drawString("Btn1: + / Long: proximo campo", 10, 120);
    tft.drawString("Btn0: -  / Double: menu", 10, 135);
    tft.drawString("Ou envie JSON:", 10, tft.height()-30);
    tft.drawString("{\"hour\":..,\"minute\":..,\"second\":..}", 10, tft.height()-15);
    tft.endWrite();
}

// Tela de confirmação para deleção
void drawDeleteConfirmUI() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    char msg[30];
    snprintf(msg, sizeof(msg), "Deletar %s?", services[currentServiceIndex].name);
    tft.drawString(msg, 10, 40);
    tft.setTextSize(1);
    tft.drawString("Btn0: Cancelar", 10, 80);
    tft.drawString("Btn1: Confirmar", 10, 95);
    tft.drawString("Double-click para menu", 10, tft.height()-15);
    tft.endWrite();
}

// Exibe uma mensagem (sucesso ou erro) com uma animação simples; 
// permite saída antecipada ao pressionar um botão.
void showMessageScreen(const char *msg, uint16_t color) {
    unsigned long startTime = millis();
    while (millis() - startTime < 3000) { // exibe por 3s
        btnDec.tick();
        btnInc.tick();
        tft.startWrite();
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(color, TFT_BLACK);
        tft.drawString(msg, 10, tft.height()/2 - 10);
        tft.endWrite();
        if (digitalRead(PIN_BUTTON_0) == LOW || digitalRead(PIN_BUTTON_1) == LOW)
            break;
        delay(200);
    }
}

// --- PROCESSAMENTO DE JSON ---
// Processa JSON recebido para criação de serviço, com validação e sanitização.
void processNewServiceJSON() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() == 0) return;
        if (input.startsWith("{") && input.endsWith("}")) {
            int nameIndex = input.indexOf("\"name\"");
            int secretIndex = input.indexOf("\"secret\"");
            if (nameIndex != -1 && secretIndex != -1) {
                // Extrai valores delimitados por aspas para maior robustez
                int nameStart = input.indexOf("\"", nameIndex + 6);
                int nameEnd = input.indexOf("\"", nameStart + 1);
                int secretStart = input.indexOf("\"", secretIndex + 7);
                int secretEnd = input.indexOf("\"", secretStart + 1);
                if (nameStart != -1 && nameEnd != -1 && secretStart != -1 && secretEnd != -1) {
                    String nameValue = input.substring(nameStart + 1, nameEnd);
                    String secretValue = input.substring(secretStart + 1, secretEnd);
                    nameValue.trim();
                    secretValue.trim();
                    if (nameValue.length() > 0 && secretValue.length() > 0 &&
                        secretValue.length() < MAX_SECRET_B32_LENGTH) {
                        nameValue.toCharArray(tempServiceName, sizeof(tempServiceName));
                        secretValue.toCharArray(tempServiceSecret, sizeof(tempServiceSecret));
                        currentScreen = SCREEN_CREATE_SERVICE_CONFIRM;
                        drawCreateServiceConfirmUI(tempServiceName);
                        lastInteractionTime = millis();
                    } else {
                        showMessageScreen("JSON invalido", TFT_RED);
                        drawCreateServiceUI();
                    }
                } else {
                    showMessageScreen("JSON invalido", TFT_RED);
                    drawCreateServiceUI();
                }
            } else {
                showMessageScreen("JSON invalido", TFT_RED);
                drawCreateServiceUI();
            }
        }
    }
}

// Processa JSON para edição do tempo, com validação dos valores recebidos.
void processEditTimeJSON() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() == 0) return;
        if (input.startsWith("{") && input.endsWith("}")) {
            bool hasFullDate = input.indexOf("\"day\"") != -1;
            if (hasFullDate) {
                int dayVal = input.indexOf("\"day\"");
                int monthVal = input.indexOf("\"month\"");
                int yearVal = input.indexOf("\"year\"");
                int hourVal = input.indexOf("\"hour\"");
                int minuteVal = input.indexOf("\"minute\"");
                int secondVal = input.indexOf("\"second\"");
                if (dayVal != -1 && monthVal != -1 && yearVal != -1 && hourVal != -1 && minuteVal != -1 && secondVal != -1) {
                    int newDay = input.substring(input.indexOf(":", dayVal) + 1, input.indexOf(",", dayVal)).toInt();
                    int newMonth = input.substring(input.indexOf(":", monthVal) + 1, input.indexOf(",", monthVal)).toInt();
                    int newYear = input.substring(input.indexOf(":", yearVal) + 1, input.indexOf(",", yearVal)).toInt();
                    int newHour = input.substring(input.indexOf(":", hourVal) + 1, input.indexOf(",", hourVal)).toInt();
                    int newMinute = input.substring(input.indexOf(":", minuteVal) + 1, input.indexOf(",", minuteVal)).toInt();
                    int newSecond = input.substring(input.indexOf(":", secondVal) + 1, input.indexOf("}", secondVal)).toInt();
                    if (newDay >= 1 && newDay <= 31 && newMonth >= 1 && newMonth <= 12 &&
                        newYear >= 2000 && newYear <= 2100 &&
                        newHour >= 0 && newHour < 24 && newMinute >= 0 && newMinute < 60 && newSecond >= 0 && newSecond < 60) {
                        rtc.adjust(DateTime(newYear, newMonth, newDay, newHour, newMinute, newSecond));
                        setTime(rtc.now().unixtime());
                        showMessageScreen("Hora atualizada", TFT_GREEN);
                        currentScreen = SCREEN_MENU;
                        drawMenuUI();
                        lastInteractionTime = millis();
                    } else {
                        showMessageScreen("Valores invalidos", TFT_RED);
                        drawEditTimeUI();
                    }
                } else {
                    showMessageScreen("JSON invalido", TFT_RED);
                    drawEditTimeUI();
                }
            } else {
                // Caso apenas hora, minuto e segundo
                int hourIndex = input.indexOf("\"hour\"");
                int minuteIndex = input.indexOf("\"minute\"");
                int secondIndex = input.indexOf("\"second\"");
                if (hourIndex != -1 && minuteIndex != -1 && secondIndex != -1) {
                    int newHour = input.substring(input.indexOf(":", hourIndex) + 1, input.indexOf(",", hourIndex)).toInt();
                    int newMinute = input.substring(input.indexOf(":", minuteIndex) + 1, input.indexOf(",", minuteIndex)).toInt();
                    int newSecond = input.substring(input.indexOf(":", secondIndex) + 1, input.indexOf("}", secondIndex)).toInt();
                    if (newHour >= 0 && newHour < 24 && newMinute >= 0 && newMinute < 60 && newSecond >= 0 && newSecond < 60) {
                        setTime(newHour, newMinute, newSecond, day(), month(), year());
                        showMessageScreen("Hora atualizada", TFT_GREEN);
                        currentScreen = SCREEN_MENU;
                        drawMenuUI();
                        lastInteractionTime = millis();
                    } else {
                        showMessageScreen("Valores invalidos", TFT_RED);
                        drawEditTimeUI();
                    }
                } else {
                    showMessageScreen("JSON invalido", TFT_RED);
                    drawEditTimeUI();
                }
            }
        }
    }
}

// --- FUNÇÕES DE SERVIÇOS ---
// Define o serviço atual e gera o código TOTP correspondente.
void setCurrentService(int index) {
    if (serviceCount == 0) return;
    currentServiceIndex = index;
    byte binKey[MAX_SECRET_BIN_LENGTH];
    int decodedLen = base32decode(services[index].secret, binKey, sizeof(binKey));
    if (decodedLen <= 0) {
        Serial.println("Erro no decode Base32!");
        showMessageScreen("Erro na chave", TFT_RED);
        return;
    }
    uint32_t novoTOTP = generateTOTP(binKey, decodedLen, rtc.now().unixtime());
    snprintf(lastTOTP, sizeof(lastTOTP), "%06d", novoTOTP);
    currentTOTPInterval = now() / TOTP_INTERVAL;
    Serial.print("Novo TOTP gerado: ");
    Serial.println(lastTOTP);
}

// Adiciona um novo serviço e atualiza o atual.
void addNewService(const char *name, const char *secret) {
    if (serviceCount < MAX_SERVICES) {
        strncpy(services[serviceCount].name, name, sizeof(services[serviceCount].name) - 1);
        services[serviceCount].name[sizeof(services[serviceCount].name) - 1] = '\0';
        strncpy(services[serviceCount].secret, secret, sizeof(services[serviceCount].secret) - 1);
        services[serviceCount].secret[sizeof(services[serviceCount].secret) - 1] = '\0';
        saveNewService(name, secret);
        currentServiceIndex = serviceCount - 1;
        setCurrentService(currentServiceIndex);
    } else {
        showMessageScreen("Maximo de servicos", TFT_RED);
    }
}

// --- CALLBACKS DOS BOTÕES ---
void onButtonIncClick() {
    lastInteractionTime = millis();
    switch (currentScreen) {
        case SCREEN_TOTP:
            if (serviceCount > 0) {
                currentServiceIndex = (currentServiceIndex + 1) % serviceCount;
                setCurrentService(currentServiceIndex);
                drawTOTPUI(false);
            }
            break;
        case SCREEN_MENU:
            currentMenuIndex = (currentMenuIndex + 1) % NUM_MENU_OPTIONS;
            drawMenuUI();
            break;
        case SCREEN_EDIT_TIME:
            if (editField == 0)
                editHour = (editHour + 1) % 24;
            else if (editField == 1)
                editMinute = (editMinute + 1) % 60;
            else if (editField == 2)
                editSecond = (editSecond + 1) % 60;
            drawEditTimeUI();
            break;
        case SCREEN_DELETE_CONFIRM:
            deleteCurrentService();
            currentScreen = SCREEN_TOTP;
            drawTOTPUI(true);
            break;
        case SCREEN_CREATE_SERVICE_CONFIRM:
            addNewService(tempServiceName, tempServiceSecret);
            showMessageScreen("Servico adicionado!", TFT_GREEN);
            currentScreen = SCREEN_MENU;
            drawMenuUI();
            break;
        default:
            break;
    }
}

void onButtonDecClick() {
    lastInteractionTime = millis();
    switch (currentScreen) {
        case SCREEN_TOTP:
            if (serviceCount > 0) {
                currentServiceIndex = (currentServiceIndex - 1 + serviceCount) % serviceCount;
                setCurrentService(currentServiceIndex);
                drawTOTPUI(false);
            }
            break;
        case SCREEN_MENU:
            currentMenuIndex = (currentMenuIndex - 1 + NUM_MENU_OPTIONS) % NUM_MENU_OPTIONS;
            drawMenuUI();
            break;
        case SCREEN_EDIT_TIME:
            if (editField == 0)
                editHour = (editHour - 1 + 24) % 24;
            else if (editField == 1)
                editMinute = (editMinute - 1 + 60) % 60;
            else if (editField == 2)
                editSecond = (editSecond - 1 + 60) % 60;
            drawEditTimeUI();
            break;
        case SCREEN_DELETE_CONFIRM:
            currentScreen = SCREEN_TOTP;
            drawTOTPUI(true);
            break;
        case SCREEN_CREATE_SERVICE_CONFIRM:
            currentScreen = SCREEN_MENU;
            drawMenuUI();
            break;
        default:
            break;
    }
}

void onButtonIncLongPress() {
    lastInteractionTime = millis();
    switch (currentScreen) {
        case SCREEN_TOTP:
            if (serviceCount > 0) {
                currentScreen = SCREEN_DELETE_CONFIRM;
                drawDeleteConfirmUI();
            }
            break;
        case SCREEN_MENU:
            if (currentMenuIndex == 0) {
                currentScreen = SCREEN_CREATE_SERVICE;
                drawCreateServiceUI();
            } else if (currentMenuIndex == 1) {
                currentScreen = SCREEN_TOTP;
                drawTOTPUI(true);
            } else if (currentMenuIndex == 2) {
                currentScreen = SCREEN_EDIT_TIME;
                time_t t = now();
                editHour = hour(t);
                editMinute = minute(t);
                editSecond = second(t);
                editField = 0;
                drawEditTimeUI();
            }
            break;
        case SCREEN_EDIT_TIME:
            if (editField < 2) {
                editField++;
                drawEditTimeUI();
            } else {
                setTime(editHour, editMinute, editSecond, day(), month(), year());
                showMessageScreen("Hora atualizada", TFT_GREEN);
                currentScreen = SCREEN_MENU;
                drawMenuUI();
            }
            break;
        default:
            break;
    }
}

void onButtonIncDoubleClick() {
    lastInteractionTime = millis();
    currentScreen = SCREEN_MENU;
    drawMenuUI();
}

// --- ATUALIZAÇÃO DO CÓDIGO TOTP ---
// Verifica se o intervalo mudou e atualiza o código se necessário.
void updateTOTPCode() {
    if (serviceCount == 0) return;
    uint32_t nowInterval = now() / TOTP_INTERVAL;
    if (nowInterval != currentTOTPInterval) {
        setCurrentService(currentServiceIndex);
        drawTOTPUI(false);
    }
}

// --- SETUP E LOOP ---
void setup() {
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    Serial.begin(115200);
    delay(500);
    Serial.println("[SETUP] Iniciando...");
    if (!rtc.begin()) {
        Serial.println("RTC não encontrado!");
        showMessageScreen("RTC nao encontrado", TFT_RED);
        while (1);
    }
    setTime(rtc.now().unixtime());
    // Configura os pinos
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    pinMode(PIN_LCD_BL, OUTPUT);
    setBrightness(currentBrightness);
    pinMode(PIN_BUTTON_0, INPUT_PULLUP);
    pinMode(PIN_BUTTON_1, INPUT_PULLUP);
    pinMode(PIN_BAT_VOLT, INPUT);
    // Configura callbacks dos botões
    btnDec.attachClick(onButtonDecClick);
    btnInc.attachClick(onButtonIncClick);
    btnInc.attachDoubleClick(onButtonIncDoubleClick);
    btnInc.attachLongPressStart(onButtonIncLongPress);
    // Inicializa a tela
    tft.init();
    tft.setRotation(3);
    // Carrega serviços do NVS
    preferences.begin("totp", false);
    loadServices();
    preferences.end();
    if (serviceCount > 0) {
        setCurrentService(currentServiceIndex);
    } else {
        lastTOTP[0] = '\0';
    }
    currentScreen = SCREEN_MENU;
    drawMenuUI();
    lastInteractionTime = millis();
    lastSyncTime = millis();
    Serial.print("[SETUP] RTC: ");
    Serial.println(rtc.now().unixtime());
}

void loop() {
    btnDec.tick();
    btnInc.tick();

    // Processa entradas Serial conforme a tela atual
    if (currentScreen == SCREEN_CREATE_SERVICE) {
        processNewServiceJSON();
    }
    if (currentScreen == SCREEN_EDIT_TIME) {
        processEditTimeJSON();
    }
    
    unsigned long currentMillis = millis();
    
    // Atualiza TOTP e UI na tela TOTP
    if (currentScreen == SCREEN_TOTP && currentMillis - lastUpdateTime >= CLOCK_UPDATE_INTERVAL) {
        lastUpdateTime = currentMillis;
        updateTOTPCode();
    }
    // Atualiza o relógio no menu
    if (currentScreen == SCREEN_MENU && currentMillis - lastMenuUpdateTime >= CLOCK_UPDATE_INTERVAL) {
        lastMenuUpdateTime = currentMillis;
        updateClockUI();
    }
    // Ajusta o brilho se em modo bateria e inatividade
    if (isBatteryMode()) {
        if (currentMillis - lastInteractionTime > INACTIVITY_TIMEOUT)
            setBrightness(2);
        else
            setBrightness(currentBrightness);
    }
    // Sincroniza o tempo com o RTC periodicamente
    if (currentMillis - lastSyncTime > SYNC_INTERVAL) {
        setTime(rtc.now().unixtime());
        lastSyncTime = currentMillis;
    }
}
