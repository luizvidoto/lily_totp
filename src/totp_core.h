// totp_core.h
#ifndef TOTP_CORE_H
#define TOTP_CORE_H

#include <stdint.h> // Para uintXX_t
#include <stddef.h> // Para size_t

// ---- Funções Principais TOTP ----

// Decodifica o segredo Base32 do serviço atualmente selecionado (current_service_index)
// para binário e armazena em current_totp.key_bin.
// Atualiza current_totp.valid_key e current_totp.key_bin_len.
// Retorna true se a decodificação for bem-sucedida, false caso contrário.
// Em caso de falha, preenche current_totp.code com uma mensagem de erro.
bool totp_decode_current_service_key();

// Gera um novo código TOTP para o serviço atual, se o intervalo de tempo mudou.
// Usa current_totp.key_bin e current_totp.key_bin_len.
// Armazena o código formatado (6 dígitos) em current_totp.code.
// Atualiza current_totp.last_generated_interval.
// Deve ser chamado regularmente (ex: a cada segundo).
void totp_update_current_code();

// ---- Funções Auxiliares (geralmente internas, mas podem ser úteis) ----

// Decodifica uma string Base32 para um buffer binário.
// Parâmetros:
//   encoded: Ponteiro para a string Base32 (null-terminated ou com `encodedLength`).
//   encodedLength: Comprimento da string Base32 a ser decodificada.
//   result: Buffer para armazenar o resultado binário.
//   bufSize: Tamanho do buffer `result`.
// Retorna:
//   O número de bytes decodificados e escritos em `result`.
//   Retorna 0 ou um valor negativo em caso de erro (ex: buffer insuficiente, char inválido).
//   (Nota: A implementação original retornava 0 para erro, vamos manter isso por simplicidade,
//    mas um código de erro negativo seria mais explícito).
int totp_base32_decode(const uint8_t *encoded, size_t encodedLength, uint8_t *result, size_t bufSize);

// Valida se uma string contém apenas caracteres Base32 válidos (A-Z, 2-7, =).
// Retorna true se válido, false caso contrário.
bool totp_validate_base32_string(const char* b32_string);

// Gera um código TOTP (HOTP com contador baseado em tempo).
// Parâmetros:
//   key: Ponteiro para a chave binária.
//   keyLength: Comprimento da chave binária em bytes.
//   timestamp: Timestamp Unix UTC para o qual gerar o código.
//   interval: Intervalo TOTP em segundos (geralmente 30).
// Retorna:
//   O código TOTP de 6 dígitos (0-999999).
//   Retorna um valor específico (ex: 0 ou UINT32_MAX) em caso de erro.
uint32_t totp_generate_code(const uint8_t *key, size_t keyLength, uint64_t timestamp, uint32_t interval);

#endif // TOTP_CORE_H
