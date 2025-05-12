// rfid_reader.h
#ifndef RFID_READER_H
#define RFID_READER_H

// ---- Inicialização ----
// Inicializa a comunicação SPI e o módulo MFRC522.
void rfid_init();

// ---- Leitura do Cartão ----
// Verifica se um novo cartão está presente e tenta ler seu UID.
// Se um novo cartão for lido com sucesso:
//   - Formata o UID como string hexadecimal ("XX XX XX XX").
//   - Armazena o UID formatado na variável global 'card_id'.
//   - Retorna true.
// Se nenhum cartão novo for encontrado ou a leitura falhar:
//   - Retorna false.
// NOTA: Esta função deve ser chamada APENAS quando na tela SCREEN_READ_RFID
// para evitar consumo desnecessário de SPI e processamento.
bool rfid_read_card();

#endif // RFID_READER_H


