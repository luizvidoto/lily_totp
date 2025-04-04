// #include <Arduino.h>
// #include <TFT_eSPI.h> // Inclui a biblioteca principal
// #include <RTClib.h>      // Para RTC DS3231

// // --- Definições Específicas T-Display S3 ---
// // Confirme estes pinos com base na sua verificação do User_Setup.h!
// #define PIN_LCD_BL 38 // Pino do Backlight

// TFT_eSPI tft = TFT_eSPI(); // Objeto para a tela principal

// void setup() {
//   Serial.begin(115200);
//   while (!Serial);
//   Serial.println("\n--- Teste Básico Backlight e TFT ---");

//   // 1. Controlar Backlight PRIMEIRO
//   Serial.print("Configurando Pino Backlight (");
//   Serial.print(PIN_LCD_BL);
//   Serial.println(")...");
//   pinMode(PIN_LCD_BL, OUTPUT);

//   Serial.println("Ligando Backlight (digitalWrite HIGH)...");
//   digitalWrite(PIN_LCD_BL, HIGH); // Assume que HIGH liga o backlight

//   // Pequena pausa para garantir que o backlight ligue
//   delay(250);

//   // 2. Inicializar TFT DEPOIS de ligar o backlight
//   Serial.print("Inicializando TFT... ");
//   tft.init();
//   Serial.println("Feito.");

//   // 3. Tentar desenhar algo simples
//   tft.setRotation(3); // Tente rotações diferentes (0, 1, 2, 3) se necessário
//   Serial.println("Preenchendo tela com VERDE...");
//   tft.fillScreen(TFT_GREEN); // Use uma cor brilhante
//   Serial.println("Comando fillScreen enviado.");

//   Serial.println("Setup concluído. A tela deve estar VERDE agora.");
// }

// void loop() {
//   // Apenas mantém o backlight ligado e informa no Serial
//   digitalWrite(PIN_LCD_BL, HIGH); // Garante que continua ligado
//   Serial.println("Loop - Mantendo backlight LIGADO. Tela deve permanecer VERDE.");
//   delay(5000); // Pausa longa
// }