#include <TimeLib.h>
#include "totp.h"
#include "globals.h"
#include "i18n.h"
#include "ui.h"
#include "types.h"
#include <mbedtls/md.h>

// ---- Funções de Decodificação Base32 e TOTP ----
int base32_decode(const uint8_t *encoded, size_t encodedLength, uint8_t *result, size_t bufSize) {
    int buffer = 0, bitsLeft = 0, count = 0;
    for (size_t i = 0; i < encodedLength && count < bufSize; i++) {
        uint8_t ch = encoded[i], value = 255;
        if (ch >= 'A' && ch <= 'Z') value = ch - 'A';
        else if (ch >= 'a' && ch <= 'z') value = ch - 'a';
        else if (ch >= '2' && ch <= '7') value = ch - '2' + 26;
        else if (ch == '=') continue; // Ignora padding
        else continue; // Ignora caracteres inválidos
        buffer = (buffer << 5) | value;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            result[count++] = (buffer >> (bitsLeft - 8)) & 0xFF;
            bitsLeft -= 8;
        }
    }
    return count;
}

uint32_t generateTOTP(const uint8_t *key, size_t keyLength, uint64_t timestamp, uint32_t interval = TOTP_INTERVAL_SECONDS) {
    if (!key || keyLength == 0) {
        Serial.printf("[ERROR] generateTOTP: Chave inválida (len=%d)\n", keyLength);
        return 0;
    }
    uint64_t counter = timestamp / interval;
    uint8_t counterBytes[8];
    // Converte counter para Big-Endian byte array
    for (int i = 7; i >= 0; i--) {
        counterBytes[i] = counter & 0xFF;
        counter >>= 8;
    }

    uint8_t hash[20]; // SHA-1 output é 20 bytes
    mbedtls_md_context_t ctx;
    mbedtls_md_info_t const *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (!info) {
        Serial.println("[ERROR] generateTOTP: mbedtls_md_info_from_type falhou");
        return 0; // Não conseguiu obter info SHA1
    }

    mbedtls_md_init(&ctx);
    // Configura para HMAC-SHA1
    if (mbedtls_md_setup(&ctx, info, 1) != 0) { // 1 = use HMAC
        Serial.println("[ERROR] generateTOTP: mbedtls_md_setup falhou");
        mbedtls_md_free(&ctx); return 0;
    }
    if (mbedtls_md_hmac_starts(&ctx, key, keyLength) != 0) {
        Serial.println("[ERROR] generateTOTP: mbedtls_md_hmac_starts falhou");
        mbedtls_md_free(&ctx); return 0;
    }
    if (mbedtls_md_hmac_update(&ctx, counterBytes, 8) != 0) {
        Serial.println("[ERROR] generateTOTP: mbedtls_md_hmac_update falhou");
        mbedtls_md_free(&ctx); return 0;
    }
    if (mbedtls_md_hmac_finish(&ctx, hash) != 0) {
        Serial.println("[ERROR] generateTOTP: mbedtls_md_hmac_finish falhou");
        mbedtls_md_free(&ctx); return 0;
    }
    mbedtls_md_free(&ctx); // Libera contexto

    // Extração dinâmica (RFC 4226)
    int offset = hash[19] & 0x0F; // Último nibble do hash define o offset (0-15)
    // Extrai 4 bytes a partir do offset, zera o bit mais significativo do primeiro byte
    uint32_t binaryCode =
        ((hash[offset] & 0x7F) << 24) |
        ((hash[offset + 1] & 0xFF) << 16) |
        ((hash[offset + 2] & 0xFF) << 8)  |
        (hash[offset + 3] & 0xFF);

    // Retorna os últimos 6 dígitos
    return binaryCode % 1000000;
}

// ---- Funções TOTP ----
bool decodeCurrentServiceKey(){
    if(service_count <= 0 || current_service_index >= service_count || current_service_index < 0){
        current_totp.valid_key_loaded = false;
        snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(STR_TOTP_CODE_ERROR));
        // Serial.println("[TOTP] Índice inválido ou sem serviços.");
        return false;
    }
    const char* secret_b32 = services[current_service_index].secret_b32;
    // Serial.printf("[TOTP] Decodificando chave para '%s': %s\n", services[current_service_index].name, secret_b32);
    int decoded_len = base32_decode((const uint8_t*)secret_b32, strlen(secret_b32), current_totp.key_bin, MAX_SECRET_BIN_LEN);

    if(decoded_len > 0 && decoded_len <= MAX_SECRET_BIN_LEN){
        current_totp.key_bin_len = decoded_len;
        current_totp.valid_key_loaded = true;
        current_totp.last_generated_interval = 0; // Força geração inicial
        updateCurrentTOTP(); // Gera o primeiro código para a nova chave
        // Serial.printf("[TOTP] Chave decodificada com sucesso (%d bytes).\n", decoded_len);
        return true;
    } else {
        current_totp.valid_key_loaded = false;
        current_totp.key_bin_len = 0;
        snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(STR_ERROR_B32_DECODE));
        Serial.printf("[ERROR] Falha ao decodificar Base32 para '%s'. Len: %d\n", services[current_service_index].name, decoded_len);
        return false;
    }
}

void updateCurrentTOTP(){
    // Se não houver chave válida carregada, mostra erro e sai
    if (!current_totp.valid_key_loaded) {
        snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(STR_TOTP_CODE_ERROR));
        return;
    }
    uint64_t current_unix_time_utc = now(); // Usa tempo UTC do TimeLib
    uint32_t current_interval = current_unix_time_utc / TOTP_INTERVAL_SECONDS;

    // Gera um novo código apenas se o intervalo de tempo mudou
    if (current_interval != current_totp.last_generated_interval) {
        uint32_t totp_code_val = generateTOTP(current_totp.key_bin, current_totp.key_bin_len, current_unix_time_utc);
        snprintf(current_totp.code, sizeof(current_totp.code), "%06lu", totp_code_val); // Formata com 6 dígitos
        current_totp.last_generated_interval = current_interval; // Atualiza último intervalo gerado
        // Serial.printf("[TOTP] Novo código gerado: %s\n", current_totp.code);
    }
    // Se o intervalo não mudou, o código existente em current_totp.code é mantido
}