#pragma once // Include guard

#include <stddef.h> // Para size_t
#include <stdint.h> // Para uint8_t, uint32_t, uint64_t
#include "config.h" // Para constantes como TOTP_INTERVAL_SECONDS

// ============================================================================
// === FUNÇÕES PÚBLICAS DO MÓDULO TOTP ===
// ============================================================================

/**
 * @brief Decodifica uma string Base32 em um buffer binário.
 *        Ignora caracteres inválidos e padding ('=').
 * @param encoded Ponteiro para a string Base32 codificada.
 * @param encodedLength Comprimento da string Base32.
 * @param result Buffer onde o resultado binário será armazenado.
 * @param bufSize Tamanho do buffer de resultado.
 * @return O número de bytes decodificados escritos no buffer 'result', ou 0 em caso de erro ou entrada vazia.
 */
int base32_decode(const uint8_t *encoded, size_t encodedLength, uint8_t *result, size_t bufSize);

/**
 * @brief Gera um código TOTP (HMAC-SHA1) para um determinado timestamp.
 * @param key Ponteiro para a chave secreta binária.
 * @param keyLength Comprimento da chave secreta em bytes.
 * @param timestamp Timestamp Unix (UTC) para o qual gerar o código.
 * @param interval Intervalo de tempo TOTP em segundos (padrão de config.h).
 * @return O código TOTP de 6 dígitos, ou 0 em caso de erro.
 */
uint32_t generateTOTP(const uint8_t *key, size_t keyLength, uint64_t timestamp, uint32_t interval = TOTP_INTERVAL_SECONDS);

/**
 * @brief Decodifica a chave secreta Base32 do serviço atualmente selecionado
 *        (current_service_index) e armazena o resultado binário internamente (se necessário).
 *        Atualiza o estado 'current_totp.valid_key_loaded'.
 * @return true se a decodificação foi bem-sucedida, false caso contrário (índice inválido, erro de decodificação).
 */
bool decodeCurrentServiceKey();

/**
 * @brief Verifica se o intervalo de tempo TOTP mudou desde a última geração.
 *        Se sim, gera um novo código TOTP para o serviço atual (usando a chave decodificada)
 *        e atualiza a struct global 'current_totp' (código e último intervalo).
 * @return void
 */
void updateCurrentTOTP();

/**
 * @brief Marca o código TOTP atual como inválido e define o placeholder.
 *        Usado quando não há serviços ou a chave não pôde ser decodificada.
 */
void invalidateCurrentTOTP();