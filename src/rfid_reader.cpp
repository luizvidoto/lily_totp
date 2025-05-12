// rfid_reader.cpp
#include "rfid_reader.h"
#include "globals.h" // Acesso a mfrc522, card_id
#include "config.h"  // Acesso aos pinos SPI (SCK_PIN, MISO_PIN, MOSI_PIN) - Embora MFRC522 use SPI padrão
#include <SPI.h>     // Necessário para MFRC522
#include <Arduino.h> // Para Serial, sprintf

// ---- Inicialização ----
void rfid_init() {
    Serial.print("[RFID] Inicializando MFRC522... ");
    // A biblioteca MFRC522 gerencia a inicialização do SPI internamente
    // se não for inicializado antes. Mas é boa prática inicializar explicitamente.
    // SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN); // SS_PIN é gerenciado pela lib
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN); // Inicializa SPI sem definir SS globalmente
    mfrc522.PCD_Init(); // Inicializa o MFRC522 (configura pinos SS e RST)

    // Pequena verificação (opcional)
    byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (version == 0x00 || version == 0xFF) {
        Serial.println("FALHA! Verifique conexões.");
        // Poderia sinalizar um erro mais grave aqui se o RFID for essencial.
    } else {
        Serial.print("OK. Versão Firmware: 0x");
        Serial.println(version, HEX);
    }
}

// ---- Leitura do Cartão ----
bool rfid_read_card() {
    // 1. Verifica se há um novo cartão presente
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return false; // Nenhum cartão novo
    }

    // 2. Tenta selecionar o cartão e ler o UID
    if (!mfrc522.PICC_ReadCardSerial()) {
        // Falha na leitura (colisão, cartão removido rápido demais, etc.)
        // A biblioteca já chama PICC_HaltA() em caso de falha aqui.
        return false;
    }

    // 3. Leitura bem-sucedida - Formata e armazena o UID
    // O UID está em mfrc522.uid.uidByte[], e o tamanho em mfrc522.uid.size
    // Vamos formatar os primeiros 4 bytes (comum para MIFARE Classic)
    // para a variável global 'card_id'.
    if (mfrc522.uid.size >= 4) {
        snprintf(card_id, sizeof(card_id), "%02X %02X %02X %02X",
                 mfrc522.uid.uidByte[0],
                 mfrc522.uid.uidByte[1],
                 mfrc522.uid.uidByte[2],
                 mfrc522.uid.uidByte[3]);
        Serial.printf("[RFID] Cartão lido. UID: %s\n", card_id);
    } else {
        // Caso o cartão tenha um UID menor que 4 bytes (raro, mas possível)
        strcpy(card_id, "UID<4B"); // Indica UID curto
        Serial.printf("[RFID] Cartão lido com UID curto (size=%d).\n", mfrc522.uid.size);
    }


    // 4. Libera o cartão para permitir nova leitura (importante!)
    mfrc522.PICC_HaltA();
    // 5. Para a criptografia (se estava ativa - não usamos aqui, mas é boa prática)
    mfrc522.PCD_StopCrypto1();

    return true; // Novo cartão lido com sucesso
}