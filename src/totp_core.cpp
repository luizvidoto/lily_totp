// totp_core.cpp
#include "totp_core.h"
#include "globals.h"    // Acesso a services, current_service_index, current_totp, etc.
#include "config.h"     // Acesso a TOTP_INTERVAL, MAX_SECRET_BIN_LEN
#include "time_keeper.h"// Para time_now_utc()
#include "i18n.h"       // Para getText, StringID
#include "ui_manager.h" // Para ui_queue_message (em caso de erro grave na decodificação)
#include <mbedtls/md.h> // Para HMAC-SHA1
#include <Arduino.h>    // Para Serial, snprintf, strlen

// ---- Funções Principais TOTP ----

bool totp_decode_current_service_key() {
    if (service_count <= 0 || current_service_index < 0 || current_service_index >= service_count) {
        current_totp.valid_key = false;
        snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(StringID::STR_TOTP_CODE_PLACEHOLDER));
        // Serial.println("[TOTP] totp_decode_current_service_key: Índice de serviço inválido ou sem serviços.");
        return false;
    }

    const char* secret_b32 = services[current_service_index].secret_b32;
    // Serial.printf("[TOTP] Decodificando chave para '%s': %s\n", services[current_service_index].name, secret_b32);

    // Validação prévia dos caracteres Base32
    if (!totp_validate_base32_string(secret_b32)) {
        current_totp.valid_key = false;
        current_totp.key_bin_len = 0;
        snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(StringID::STR_ERROR_B32_DECODE)); // Ou uma msg mais específica
        Serial.printf("[TOTP ERROR] Segredo Base32 para '%s' contém caracteres inválidos.\n", services[current_service_index].name);
        // Considerar ui_queue_message para um erro mais visível se isso acontecer após adição.
        // ui_queue_message_fmt(StringID::STR_ERROR_SECRET_B32_CHARS, COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN);
        return false;
    }


    int decoded_len = totp_base32_decode((const uint8_t*)secret_b32, strlen(secret_b32),
                                         current_totp.key_bin, MAX_SECRET_BIN_LEN);

    if (decoded_len > 0 && decoded_len <= MAX_SECRET_BIN_LEN) {
        current_totp.key_bin_len = decoded_len;
        current_totp.valid_key = true;
        current_totp.last_generated_interval = 0; // Força geração de novo código
        totp_update_current_code(); // Gera o primeiro código para a nova chave
        // Serial.printf("[TOTP] Chave para '%s' decodificada com sucesso (%d bytes).\n", services[current_service_index].name, decoded_len);
        return true;
    } else {
        current_totp.valid_key = false;
        current_totp.key_bin_len = 0;
        snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(StringID::STR_ERROR_B32_DECODE));
        Serial.printf("[TOTP ERROR] Falha ao decodificar Base32 para '%s'. Decoded len: %d\n", services[current_service_index].name, decoded_len);
        // Se o erro for por buffer insuficiente (decoded_len > MAX_SECRET_BIN_LEN), é um problema sério.
        // A validação de tamanho do segredo na adição deveria pegar isso.
        return false;
    }
}

void totp_update_current_code() {
    if (!current_totp.valid_key) {
        // Se a chave não é válida, o placeholder de erro já deve estar em current_totp.code
        // (definido por totp_decode_current_service_key ou se não houver serviços).
        // Apenas garante que está lá.
        if (service_count == 0) {
             snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(StringID::STR_ERROR_NO_SERVICES));
        } else if (strlen(current_totp.code) == 0 || strcmp(current_totp.code, "000000") == 0) {
            // Se por algum motivo o código está vazio ou zerado e a chave é inválida, põe o placeholder
            snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(StringID::STR_TOTP_CODE_PLACEHOLDER));
        }
        return;
    }

    uint64_t current_unix_time = time_now_utc();
    uint32_t current_interval_count = current_unix_time / TOTP_INTERVAL;

    if (current_interval_count != current_totp.last_generated_interval) {
        uint32_t totp_val = totp_generate_code(current_totp.key_bin, current_totp.key_bin_len,
                                               current_unix_time, TOTP_INTERVAL);
        snprintf(current_totp.code, sizeof(current_totp.code), "%06lu", totp_val);
        current_totp.last_generated_interval = current_interval_count;
        // Serial.printf("[TOTP] Novo código para '%s': %s (intervalo %lu)\n", services[current_service_index].name, current_totp.code, current_interval_count);
    }
    // Se o intervalo não mudou, o código existente em current_totp.code é mantido.
}


// ---- Funções Auxiliares ----

// Implementação de Base32 Decode (RFC 4648)
// Adaptação da função original, com algumas melhorias de robustez.
int totp_base32_decode(const uint8_t *encoded, size_t encodedLength, uint8_t *result, size_t bufSize) {
    if (!encoded || !result || bufSize == 0) {
        return -1; // Argumentos inválidos
    }

    int buffer = 0;
    int bitsLeft = 0;
    int count = 0;

    for (size_t i = 0; i < encodedLength; i++) {
        uint8_t ch = encoded[i];
        uint8_t value;

        if (ch >= 'A' && ch <= 'Z') {
            value = ch - 'A';
        } else if (ch >= 'a' && ch <= 'z') { // Suporta minúsculas também
            value = ch - 'a';
        } else if (ch >= '2' && ch <= '7') {
            value = ch - '2' + 26;
        } else if (ch == '=') { // Padding
            // Verifica se o padding é válido (ocorre no final e em blocos corretos)
            // Para simplificar, vamos apenas parar aqui. Uma validação mais estrita
            // verificaria se os bits restantes em 'buffer' são zero.
            break;
        } else {
            // Caractere inválido na string Base32
            // Serial.printf("[TOTP B32 Decode] Caractere inválido '%c' (0x%02X) na posição %d.\n", ch, ch, i);
            return -2; // Erro de caractere inválido
        }

        buffer = (buffer << 5) | value;
        bitsLeft += 5;

        if (bitsLeft >= 8) {
            if (count >= bufSize) {
                // Serial.println("[TOTP B32 Decode] Buffer de resultado insuficiente.");
                return -3; // Buffer de resultado cheio
            }
            result[count++] = (buffer >> (bitsLeft - 8)) & 0xFF;
            bitsLeft -= 8;
        }
    }
    // RFC 4648: "Qualquer bit restante no final do processamento é ignorado."
    // No entanto, se bitsLeft > 0 e esses bits não são zero devido a um caractere
    // de padding ausente ou dados truncados, isso pode ser um problema.
    // Para esta aplicação, se chegou aqui sem erro, consideramos ok.

    return count; // Retorna o número de bytes decodificados
}

bool totp_validate_base32_string(const char* b32_string) {
    if (!b32_string) return false;
    for (size_t i = 0; i < strlen(b32_string); ++i) {
        char ch = b32_string[i];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '2' && ch <= '7') || ch == '=')) {
            return false; // Caractere inválido encontrado
        }
    }
    return true;
}


// Implementação da geração TOTP (baseada em HMAC-SHA1)
uint32_t totp_generate_code(const uint8_t *key, size_t keyLength, uint64_t timestamp, uint32_t interval) {
    if (!key || keyLength == 0) {
        Serial.printf("[TOTP ERROR] totp_generate_code: Chave inválida ou comprimento zero (len=%zu)\n", keyLength);
        return 0; // Ou um código de erro específico
    }
    if (interval == 0) {
        Serial.println("[TOTP ERROR] totp_generate_code: Intervalo não pode ser zero.");
        return 0;
    }

    uint64_t counter = timestamp / interval;
    uint8_t counterBytes[8];

    // Converte o contador para um array de bytes Big-Endian
    for (int i = 7; i >= 0; i--) {
        counterBytes[i] = counter & 0xFF;
        counter >>= 8;
    }

    uint8_t hash[MBEDTLS_MD_MAX_SIZE]; // SHA-1 é 20 bytes, mas usamos o max size para segurança
    size_t hash_len;

    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);

    if (!info) {
        Serial.println("[TOTP ERROR] mbedtls_md_info_from_type(SHA1) falhou.");
        return 0;
    }

    mbedtls_md_init(&ctx);

    if (mbedtls_md_setup(&ctx, info, 1) != 0) { // 1 = use HMAC
        Serial.println("[TOTP ERROR] mbedtls_md_setup falhou.");
        mbedtls_md_free(&ctx);
        return 0;
    }
    if (mbedtls_md_hmac_starts(&ctx, key, keyLength) != 0) {
        Serial.println("[TOTP ERROR] mbedtls_md_hmac_starts falhou.");
        mbedtls_md_free(&ctx);
        return 0;
    }
    if (mbedtls_md_hmac_update(&ctx, counterBytes, sizeof(counterBytes)) != 0) {
        Serial.println("[TOTP ERROR] mbedtls_md_hmac_update falhou.");
        mbedtls_md_free(&ctx);
        return 0;
    }
    if (mbedtls_md_hmac_finish(&ctx, hash) != 0) {
        Serial.println("[TOTP ERROR] mbedtls_md_hmac_finish falhou.");
        mbedtls_md_free(&ctx);
        return 0;
    }
    mbedtls_md_free(&ctx); // Libera o contexto mbedTLS

    hash_len = mbedtls_md_get_size(info); // Deve ser 20 para SHA1

    // Extração dinâmica do código (RFC 4226)
    int offset = hash[hash_len - 1] & 0x0F; // Último nibble do hash define o offset (0-15)

    // Verifica se o offset + 4 bytes não excede o tamanho do hash
    if (offset + 3 >= hash_len) {
        Serial.printf("[TOTP ERROR] Offset (%d) inválido para hash_len (%zu).\n", offset, hash_len);
        return 0;
    }

    uint32_t binaryCode =
        ((hash[offset] & 0x7F) << 24) |     // Zera o bit mais significativo do primeiro byte
        ((hash[offset + 1] & 0xFF) << 16) |
        ((hash[offset + 2] & 0xFF) << 8) |
        (hash[offset + 3] & 0xFF);

    return binaryCode % 1000000; // Retorna os últimos 6 dígitos
}