#include <Arduino.h>
#include <TFT_eSPI.h>
#include "OneButton.h"
#include "pin_config.h"
#include <TimeLib.h> // Funções: now(), hour(), minute(), second(), setTime()
#include <TOTP.h>
#include <Base32-Decode.h>
#include <Preferences.h>

// ---------------------------------------------------------------------------
// CONFIGURAÇÃO DOS SERVIÇOS (NVS)
// Suporte a até MAX_SERVICES
#define MAX_SERVICES 300

struct TOTPService
{
  char name[21];   // nome do serviço (até 20 caracteres)
  char secret[21]; // chave em base32 (até 20 caracteres)
};

TOTPService services[MAX_SERVICES];
int serviceCount = 0;        // Quantidade de serviços cadastrados
int currentServiceIndex = 0; // Índice do serviço ativo

// Variáveis temporárias para criação de novo serviço
char tempServiceName[21];
char tempServiceSecret[21];

// ---------------------------------------------------------------------------
// CONSTANTES E VARIÁVEIS GLOBAIS

TFT_eSPI tft = TFT_eSPI();
OneButton btnDec(PIN_BUTTON_0, true);
OneButton btnInc(PIN_BUTTON_1, true);

uint8_t currentBrightness = 8;
const uint8_t maxBrightness = 16;
const uint32_t TOTP_INTERVAL = 30; // intervalo TOTP em segundos

// Estados das telas
enum ScreenState
{
  SCREEN_TOTP,
  SCREEN_MENU,
  SCREEN_CREATE_SERVICE,
  SCREEN_CREATE_SERVICE_CONFIRM,
  SCREEN_EDIT_TIME,
  SCREEN_DELETE_CONFIRM
};

ScreenState currentScreen = SCREEN_MENU; // Inicia no MENU principal

// Variáveis para controle do TOTP
unsigned char binKey[20];
TOTP totp(binKey, 10);
char lastTOTP[7] = "";         // código TOTP atual
uint32_t lastTOTPInterval = 0; // now()/TOTP_INTERVAL do último código gerado
uint32_t lastUpdateTime = 0;   // controle de atualização do display

// Menu principal
const int NUM_MENU_OPTIONS = 3;
const char *menuOptions[NUM_MENU_OPTIONS] = {
    "Criar novo servico",
    "Ver codigos",
    "Editar hora"};
int currentMenuIndex = 0;

// Variáveis para edição de hora
int editHour, editMinute, editSecond;
int editField = 0; // 0 - hora, 1 - minuto, 2 - segundo

// Variável para economia de energia (inatividade)
uint32_t lastInteractionTime = 0;

// NVS Preferences
Preferences preferences;

// ---------------------------------------------------------------------------

float readBatteryVoltage()
{
  int adcValue = analogRead(PIN_BAT_VOLT);
  float voltage = (adcValue / 4095.0) * 4.2; // ajuste conforme seu divisor de tensão
  return voltage;
}

bool isBatteryMode()
{
  return (readBatteryVoltage() < 4.10); // Se tensão < 4.10V, está em bateria.
}

void drawBatteryIcon(int x, int y)
{
  float voltage = readBatteryVoltage();
  Serial.print("voltage: ");
  Serial.println(voltage);
  bool plugged = (voltage >= 4.10); // Se ≥ 4.10V, assume plugado.
  if (plugged)
  {
    // Ícone de plug: desenho simples
    tft.drawRect(x, y, 20, 10, TFT_WHITE);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("USB", x + 2, y + 1);
  }
  else
  {
    int iconWidth = 20, iconHeight = 10;
    tft.drawRect(x, y, iconWidth, iconHeight, TFT_WHITE);
    float level = (voltage - 3.0) / (4.0 - 3.0);
    if (level < 0)
      level = 0;
    if (level > 1)
      level = 1;
    int fillWidth = (int)(iconWidth * level);
    tft.fillRect(x + 1, y + 1, fillWidth - 2, iconHeight - 2, TFT_WHITE);
  }
}

// ---------------------------------------------------------------------------
// FUNÇÃO PARA ATUALIZAR APENAS O RELÓGIO NO MENU
void updateMenuClock()
{
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", hour(now()), minute(now()), second(now()));
  int textWidth = tft.textWidth(timeStr);
  tft.fillRect(tft.width() - (textWidth + 50), 0, textWidth + 50, 30, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(timeStr, tft.width() - textWidth - 10, 10);
  drawBatteryIcon(tft.width() - textWidth - 40, 10);
}

// ---------------------------------------------------------------------------
// FUNÇÃO setBrightness (para ajustar o brilho)
void setBrightness(uint8_t value)
{
  static uint8_t level = 0;
  static uint8_t steps = 16;
  if (value == 0)
  {
    digitalWrite(PIN_LCD_BL, LOW);
    delay(3);
    level = 0;
    currentBrightness = 0;
    return;
  }
  if (level == 0)
  {
    digitalWrite(PIN_LCD_BL, HIGH);
    level = steps;
    delayMicroseconds(30);
  }
  int from = steps - level;
  int to = steps - value;
  int num = (steps + to - from) % steps;
  for (int i = 0; i < num; i++)
  {
    digitalWrite(PIN_LCD_BL, LOW);
    digitalWrite(PIN_LCD_BL, HIGH);
  }
  level = value;
  currentBrightness = value;
}

// ---------------------------------------------------------------------------
// FUNÇÕES DE INTERFACE
// Função para desenhar a parte estática da tela TOTP (chamada uma vez ao entrar na tela)
// void drawTOTPUI()
// {
//   tft.fillScreen(TFT_BLACK);

//   // Desenha o nome do serviço (parte estática)
//   tft.setTextColor(TFT_WHITE, TFT_BLACK);
//   tft.setTextSize(2);
//   tft.drawString(services[currentServiceIndex].name, 10, 10);

//   // Desenha o contorno da barra de tempo (posição fixa)
//   tft.drawRect(10, 100, 200, 10, TFT_WHITE);

//   // Desenha o rótulo de instrução
//   tft.setTextSize(1);
//   tft.setTextColor(TFT_WHITE, TFT_BLACK);
//   tft.drawString("Long press para deletar", 10, tft.height() - 15);
// }

// // Função para atualizar os elementos dinâmicos da tela TOTP (chamada a cada 1 segundo)
// void updateTOTPDynamicUI()
// {
//   // Atualiza o relógio no canto superior direito
//   char timeStr[9];
//   sprintf(timeStr, "%02d:%02d:%02d", hour(now()), minute(now()), second(now()));
//   int textWidth = tft.textWidth(timeStr);
//   tft.fillRect(tft.width() - (textWidth + 50), 0, textWidth + 50, 30, TFT_BLACK);
//   tft.setTextColor(TFT_WHITE, TFT_BLACK);
//   tft.setTextSize(2);
//   tft.drawString(timeStr, tft.width() - textWidth - 10, 10);
//   drawBatteryIcon(tft.width() - textWidth - 40, 10);

//   // Verifica se o código TOTP precisa ser atualizado
//   long nowTime = now();
//   uint32_t currentInterval = nowTime / TOTP_INTERVAL;
//   if (lastTOTP[0] == '\0' || currentInterval != lastTOTPInterval)
//   {
//     const char *code = totp.getCode(nowTime);
//     strncpy(lastTOTP, code, sizeof(lastTOTP) - 1);
//     lastTOTP[sizeof(lastTOTP) - 1] = '\0';
//     lastTOTPInterval = currentInterval;
//     // Atualiza a área do código TOTP (limpa região e desenha novo código)
//     tft.fillRect(60, 50, 150, 40, TFT_BLACK);
//     tft.setTextColor(TFT_WHITE, TFT_BLACK);
//     tft.setTextSize(3);
//     tft.drawString(lastTOTP, 60, 50);
//   }

//   // Atualiza a barra de tempo (somente a parte interna, deixando o contorno inalterado)
//   int barX = 10, barY = 100, barW = 200, barH = 10;
//   tft.fillRect(barX + 1, barY + 1, barW - 2, barH - 2, TFT_BLACK);
//   uint32_t secondsElapsed = nowTime % TOTP_INTERVAL;
//   uint32_t secondsRemaining = TOTP_INTERVAL - secondsElapsed;
//   int progress = constrain(map(secondsRemaining, 0, TOTP_INTERVAL, 0, barW), 0, barW);
//   tft.fillRect(barX + 1, barY + 1, progress - 2, barH - 2, TFT_WHITE);
// }

static const unsigned char PROGMEM plugged_icon[] = {0x00, 0x00, 0x00, 0x0f, 0xff, 0xfe, 0x10, 0x00, 0x01, 0x10, 0x01, 0x01, 0x70, 0x07, 0x01, 0x80, 0x3d, 0xf1, 0x80, 0x61, 0x01, 0x87, 0xc1, 0x01, 0x80, 0x61, 0x01, 0x80, 0x3d, 0xf1, 0x70, 0x07, 0x01, 0x10, 0x01, 0x01, 0x10, 0x00, 0x01, 0x0f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void drawTOTPUI()
{
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(2);
  tft.setFreeFont();
  tft.drawString("00:05:57", 225, 156);

  tft.drawBitmap(295, 0, plugged_icon, 24, 16, 0xFFFF);

  tft.drawString("GotHub", 3, 3);

  tft.drawRoundRect(39, 99, 243, 15, 2, 0xFFFF);

  tft.setTextSize(3);
  tft.drawString("987654", 108, 65);

  tft.setTextSize(1);
  tft.drawString("Long press para deletar", 4, 159);

  tft.fillRect(41, 101, 187, 11, 0xFFFF);
}

void drawMenuUI()
{
  tft.fillScreen(TFT_BLACK);

  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", hour(now()), minute(now()), second(now()));
  int textWidth = tft.textWidth(timeStr);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(timeStr, tft.width() - textWidth - 10, 10);
  drawBatteryIcon(tft.width() - textWidth - 40, 10);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Menu", 10, 10);

  int startY = 40;
  int lineHeight = 30;
  for (int i = 0; i < NUM_MENU_OPTIONS; i++)
  {
    if (i == currentMenuIndex)
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
    else
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString(menuOptions[i], 20, startY + i * lineHeight);
  }

  char status[20];
  sprintf(status, "Servicos: %d/%d", serviceCount, MAX_SERVICES);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(status, 10, tft.height() - 15);
}

void drawCreateServiceUI()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Criar Servico", 10, 10);
  tft.setTextSize(1);
  tft.drawString("Aguardando JSON via Serial:", 10, 40);
  tft.drawString("{\"name\":\"...\",", 10, 55);
  tft.drawString(" \"secret\":\"...\"}", 10, 70);
  tft.drawString("Double-click para menu", 10, tft.height() - 15);
}

void drawCreateServiceConfirmUI(const char *serviceName)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Adicionar:", 10, 30);
  tft.drawString(serviceName, 10, 60);
  tft.setTextSize(1);
  tft.drawString("Btn0: Cancelar / Btn1: Confirmar", 10, 90);
  tft.drawString("Double-click para menu", 10, tft.height() - 15);
}

void drawEditTimeUI()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Editar Hora", 10, 10);

  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", editHour, editMinute, editSecond);
  tft.setTextSize(3);
  tft.drawString(timeStr, 20, 50);

  int markerY = 90;
  int fieldWidth = 30;
  tft.fillRect(20 + editField * (fieldWidth + 10), markerY, fieldWidth, 3, TFT_WHITE);

  tft.setTextSize(1);
  tft.drawString("Btn1: + / Long: proximo campo", 10, 120);
  tft.drawString("Btn0: -  / Double: menu", 10, 135);
  tft.drawString("Ou envie JSON:", 10, tft.height() - 30);
  tft.drawString("{\"hour\":..,\"minute\":..,\"second\":..}", 10, tft.height() - 15);
}

void drawDeleteConfirmUI()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  char msg[30];
  snprintf(msg, sizeof(msg), "Deletar %s?", services[currentServiceIndex].name);
  tft.drawString(msg, 10, 40);
  tft.setTextSize(1);
  tft.drawString("Btn0: Cancelar", 10, 80);
  tft.drawString("Btn1: Confirmar", 10, 95);
  tft.drawString("Double-click para menu", 10, tft.height() - 15);
}

// Tela "Hora Atualizada": espera 5s ou qualquer botão pressionado
void showTimeUpdateSuccessScreen()
{
  uint32_t startTime = millis();
  while (millis() - startTime < 5000)
  {
    btnDec.tick();
    btnInc.tick();
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", hour(now()), minute(now()), second(now()));
    tft.fillRect(10, 60, tft.width() - 20, 30, TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("Hora atualizada", 10, 30);
    tft.drawString(timeStr, 10, 60);
    if (digitalRead(PIN_BUTTON_0) == LOW || digitalRead(PIN_BUTTON_1) == LOW)
      break;
    delay(200);
  }
}

// Tela de erro genérica
void showErrorScreen(const char *msg)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(msg, 10, 50);
  delay(5000);
}

// Tela de sucesso genérica
void showSuccessScreen(const char *msg)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(msg, 10, 50);
  delay(5000);
}

// ---------------------------------------------------------------------------
// FUNÇÕES DE ARMAZENAMENTO (NVS)

void saveServiceCount()
{
  preferences.putInt("svcCount", serviceCount);
}

void loadServices()
{
  preferences.begin("totp", false);
  serviceCount = preferences.getInt("svcCount", 0);
  for (int i = 0; i < serviceCount; i++)
  {
    char keyNome[20], keySecret[20];
    sprintf(keyNome, "svc%d_nome", i);
    sprintf(keySecret, "svc%d_secret", i);
    String nome = preferences.getString(keyNome, "");
    String secret = preferences.getString(keySecret, "");
    if (nome.length() > 0 && secret.length() > 0)
    {
      strncpy(services[i].name, nome.c_str(), sizeof(services[i].name) - 1);
      services[i].name[sizeof(services[i].name) - 1] = '\0';
      strncpy(services[i].secret, secret.c_str(), sizeof(services[i].secret) - 1);
      services[i].secret[sizeof(services[i].secret) - 1] = '\0';
    }
  }
  preferences.end();
}

void saveNewService(const char *nome, const char *secret)
{
  preferences.begin("totp", false);
  char keyNome[20], keySecret[20];
  sprintf(keyNome, "svc%d_nome", serviceCount);
  sprintf(keySecret, "svc%d_secret", serviceCount);
  preferences.putString(keyNome, nome);
  preferences.putString(keySecret, secret);
  serviceCount++;
  preferences.putInt("svcCount", serviceCount);
  preferences.end();
}

// ---------------------------------------------------------------------------
// FUNÇÕES DE SERVIÇOS

void setCurrentService(int index)
{
  if (serviceCount == 0)
    return;
  currentServiceIndex = index;
  base32decode(services[index].secret, binKey, sizeof(binKey));
  totp = TOTP(binKey, 10);
  lastTOTP[0] = '\0';
}

void addNewService(const char *nome, const char *secret)
{
  if (serviceCount < MAX_SERVICES)
  {
    strncpy(services[serviceCount].name, nome, sizeof(services[serviceCount].name) - 1);
    services[serviceCount].name[sizeof(services[serviceCount].name) - 1] = '\0';
    strncpy(services[serviceCount].secret, secret, sizeof(services[serviceCount].secret) - 1);
    services[serviceCount].secret[sizeof(services[serviceCount].secret) - 1] = '\0';
    saveNewService(nome, secret);
    currentServiceIndex = serviceCount - 1;
    setCurrentService(currentServiceIndex);
  }
}

void deleteCurrentService()
{
  if (serviceCount == 0)
    return;
  for (int i = currentServiceIndex; i < serviceCount - 1; i++)
  {
    services[i] = services[i + 1];
  }
  serviceCount--;
  preferences.begin("totp", false);
  preferences.putInt("svcCount", serviceCount);
  for (int i = 0; i < serviceCount; i++)
  {
    char keyNome[20], keySecret[20];
    sprintf(keyNome, "svc%d_nome", i);
    sprintf(keySecret, "svc%d_secret", i);
    preferences.putString(keyNome, services[i].name);
    preferences.putString(keySecret, services[i].secret);
  }
  preferences.end();
  if (serviceCount > 0)
    setCurrentService(currentServiceIndex % serviceCount);
  else
    lastTOTP[0] = '\0';
}

// ---------------------------------------------------------------------------
// PROCESSAMENTO DE JSON VIA SERIAL

void processNewServiceJSON()
{
  if (Serial.available() > 0)
  {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0)
      return;
    if (input.startsWith("{") && input.endsWith("}"))
    {
      int nameIndex = input.indexOf("\"name\"");
      int secretIndex = input.indexOf("\"secret\"");
      if (nameIndex != -1 && secretIndex != -1)
      {
        int colonIndex = input.indexOf(":", nameIndex);
        int commaIndex = input.indexOf(",", nameIndex);
        if (commaIndex == -1)
          commaIndex = input.indexOf("}", nameIndex);
        String nameValue = input.substring(colonIndex + 1, commaIndex);
        nameValue.replace("\"", "");
        nameValue.trim();

        colonIndex = input.indexOf(":", secretIndex);
        int endIndex = input.indexOf("}", secretIndex);
        String secretValue = input.substring(colonIndex + 1, endIndex);
        secretValue.replace("\"", "");
        secretValue.trim();

        if (nameValue.length() > 0 && secretValue.length() > 0)
        {
          nameValue.toCharArray(tempServiceName, sizeof(tempServiceName));
          secretValue.toCharArray(tempServiceSecret, sizeof(tempServiceSecret));
          currentScreen = SCREEN_CREATE_SERVICE_CONFIRM;
          drawCreateServiceConfirmUI(tempServiceName);
          lastInteractionTime = millis();
        }
        else
        {
          showErrorScreen("JSON invalido");
          drawCreateServiceUI();
        }
      }
      else
      {
        showErrorScreen("JSON invalido");
        drawCreateServiceUI();
      }
    }
  }
}
void processEditTimeJSON()
{
  if (Serial.available() > 0)
  {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0)
      return;
    if (input.startsWith("{") && input.endsWith("}"))
    {
      // Se o JSON contiver "day", assume data completa
      bool hasFullDate = (input.indexOf("\"day\"") != -1);
      if (hasFullDate)
      {
        int dayIndex = input.indexOf("\"day\"");
        int monthIndex = input.indexOf("\"month\"");
        int yearIndex = input.indexOf("\"year\"");
        int hourIndex = input.indexOf("\"hour\"");
        int minuteIndex = input.indexOf("\"minute\"");
        int secondIndex = input.indexOf("\"second\"");
        if (dayIndex != -1 && monthIndex != -1 && yearIndex != -1 && hourIndex != -1 && minuteIndex != -1 && secondIndex != -1)
        {
          // Extração dos valores
          int colonIndex = input.indexOf(":", dayIndex);
          int commaIndex = input.indexOf(",", dayIndex);
          String dayStr = input.substring(colonIndex + 1, commaIndex);
          dayStr.trim();
          int newDay = dayStr.toInt();

          colonIndex = input.indexOf(":", monthIndex);
          commaIndex = input.indexOf(",", monthIndex);
          String monthStr = input.substring(colonIndex + 1, commaIndex);
          monthStr.trim();
          int newMonth = monthStr.toInt();

          colonIndex = input.indexOf(":", yearIndex);
          commaIndex = input.indexOf(",", yearIndex);
          if (commaIndex == -1)
            commaIndex = input.indexOf("}", yearIndex);
          String yearStr = input.substring(colonIndex + 1, commaIndex);
          yearStr.trim();
          int newYear = yearStr.toInt();

          colonIndex = input.indexOf(":", hourIndex);
          commaIndex = input.indexOf(",", hourIndex);
          String hourStr = input.substring(colonIndex + 1, commaIndex);
          hourStr.trim();
          int newHour = hourStr.toInt();

          colonIndex = input.indexOf(":", minuteIndex);
          commaIndex = input.indexOf(",", minuteIndex);
          String minuteStr = input.substring(colonIndex + 1, commaIndex);
          minuteStr.trim();
          int newMinute = minuteStr.toInt();

          colonIndex = input.indexOf(":", secondIndex);
          int endIndex = input.indexOf("}", secondIndex);
          String secondStr = input.substring(colonIndex + 1, endIndex);
          secondStr.trim();
          int newSecond = secondStr.toInt();

          // Validação simples dos valores (pode ser ampliada)
          if (newDay >= 1 && newDay <= 31 && newMonth >= 1 && newMonth <= 12 &&
              newYear >= 2000 && newYear <= 2100 &&
              newHour >= 0 && newHour < 24 && newMinute >= 0 && newMinute < 60 && newSecond >= 0 && newSecond < 60)
          {
            setTime(newHour, newMinute, newSecond, newDay, newMonth, newYear);
            showTimeUpdateSuccessScreen();
            currentScreen = SCREEN_MENU;
            drawMenuUI();
            lastInteractionTime = millis();
          }
          else
          {
            showErrorScreen("Valores invalidos");
            drawEditTimeUI();
          }
        }
        else
        {
          showErrorScreen("JSON invalido");
          drawEditTimeUI();
        }
      }
      else
      {
        // Caso contrário, trata somente hora, minuto e segundo
        int hourIndex = input.indexOf("\"hour\"");
        int minuteIndex = input.indexOf("\"minute\"");
        int secondIndex = input.indexOf("\"second\"");
        if (hourIndex != -1 && minuteIndex != -1 && secondIndex != -1)
        {
          int colonIndex = input.indexOf(":", hourIndex);
          int commaIndex = input.indexOf(",", hourIndex);
          if (commaIndex == -1)
            commaIndex = input.indexOf("}", hourIndex);
          String hourStr = input.substring(colonIndex + 1, commaIndex);
          hourStr.trim();
          int newHour = hourStr.toInt();

          colonIndex = input.indexOf(":", minuteIndex);
          commaIndex = input.indexOf(",", minuteIndex);
          if (commaIndex == -1)
            commaIndex = input.indexOf("}", minuteIndex);
          String minuteStr = input.substring(colonIndex + 1, commaIndex);
          minuteStr.trim();
          int newMinute = minuteStr.toInt();

          colonIndex = input.indexOf(":", secondIndex);
          int endIndex = input.indexOf("}", secondIndex);
          String secondStr = input.substring(colonIndex + 1, endIndex);
          secondStr.trim();
          int newSecond = secondStr.toInt();

          if (newHour >= 0 && newHour < 24 && newMinute >= 0 && newMinute < 60 && newSecond >= 0 && newSecond < 60)
          {
            setTime(newHour, newMinute, newSecond, day(), month(), year());
            showTimeUpdateSuccessScreen();
            currentScreen = SCREEN_MENU;
            drawMenuUI();
            lastInteractionTime = millis();
          }
          else
          {
            showErrorScreen("Valores invalidos");
            drawEditTimeUI();
          }
        }
        else
        {
          showErrorScreen("JSON invalido");
          drawEditTimeUI();
        }
      }
    }
  }
}

// ---------------------------------------------------------------------------
// CALLBACKS DOS BOTÕES

void onButtonIncClick()
{
  lastInteractionTime = millis();
  switch (currentScreen)
  {
  case SCREEN_TOTP:
    if (serviceCount > 0)
    {
      currentServiceIndex = (currentServiceIndex + 1) % serviceCount;
      setCurrentService(currentServiceIndex);
      drawTOTPUI();
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
    drawTOTPUI();
    // updateTOTPDynamicUI();
    break;
  case SCREEN_CREATE_SERVICE_CONFIRM:
    addNewService(tempServiceName, tempServiceSecret);
    showSuccessScreen("Servico adicionado!");
    currentScreen = SCREEN_MENU;
    drawMenuUI();
    break;
  default:
    break;
  }
}

void onButtonDecClick()
{
  lastInteractionTime = millis();
  switch (currentScreen)
  {
  case SCREEN_TOTP:
    if (serviceCount > 0)
    {
      currentServiceIndex = (currentServiceIndex - 1 + serviceCount) % serviceCount;
      setCurrentService(currentServiceIndex);
      drawTOTPUI();
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
    drawTOTPUI();
    // updateTOTPDynamicUI();
    break;
  case SCREEN_CREATE_SERVICE_CONFIRM:
    currentScreen = SCREEN_MENU;
    drawMenuUI();
    break;
  default:
    break;
  }
}

void onButtonIncLongPress()
{
  lastInteractionTime = millis();
  switch (currentScreen)
  {
  case SCREEN_TOTP:
    if (serviceCount > 0)
    {
      currentScreen = SCREEN_DELETE_CONFIRM;
      drawDeleteConfirmUI();
    }
    break;
  case SCREEN_MENU:
    if (currentMenuIndex == 0)
    {
      currentScreen = SCREEN_CREATE_SERVICE;
      drawCreateServiceUI();
    }
    else if (currentMenuIndex == 1)
    {
      currentScreen = SCREEN_TOTP;
      drawTOTPUI();
    }
    else if (currentMenuIndex == 2)
    {
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
    if (editField < 2)
    {
      editField++;
      drawEditTimeUI();
    }
    else
    {
      setTime(editHour, editMinute, editSecond, day(), month(), year());
      showTimeUpdateSuccessScreen();
      currentScreen = SCREEN_MENU;
      drawMenuUI();
    }
    break;
  default:
    break;
  }
}

void onButtonIncDoubleClick()
{
  lastInteractionTime = millis();
  currentScreen = SCREEN_MENU;
  drawMenuUI();
}

// ---------------------------------------------------------------------------
// SETUP E LOOP

void setup()
{
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

  tft.init();
  tft.setRotation(3);

  preferences.begin("totp", false);
  loadServices();
  preferences.end();

  if (serviceCount > 0)
    setCurrentService(currentServiceIndex);
  else
    lastTOTP[0] = '\0';

  currentScreen = SCREEN_MENU;
  drawMenuUI();
  lastInteractionTime = millis();

  Serial.begin(115200);
}

uint32_t lastMenuUpdateTime = 0;

void loop()
{
  btnDec.tick();
  btnInc.tick();

  if (currentScreen == SCREEN_CREATE_SERVICE)
  {
    processNewServiceJSON();
  }
  if (currentScreen == SCREEN_EDIT_TIME)
  {
    processEditTimeJSON();
  }

  uint32_t currentMillis = millis();

  if (currentScreen == SCREEN_TOTP && (millis() - lastUpdateTime >= 1000))
  {
    lastUpdateTime = millis();
    // updateTOTPDynamicUI();
  }

  if (currentScreen == SCREEN_MENU && (currentMillis - lastMenuUpdateTime >= 1000))
  {
    lastMenuUpdateTime = currentMillis;
    updateMenuClock();
  }

  if (isBatteryMode())
  {
    if (currentMillis - lastInteractionTime > 30000)
    {
      setBrightness(2);
    }
    else
    {
      setBrightness(currentBrightness);
    }
  }
}