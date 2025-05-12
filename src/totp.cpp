#include "totp.h"
#include <Preferences.h>
#include <mbedtls/md.h>

TOTPService services[MAX_SERVICES];
int num_services = 0;
int current_service_index = 0;
CurrentTOTPInfo current_totp;

bool initTOTPServices() {
    Preferences preferences;
    if (!preferences.begin("totp-app", true)) return false;
    
    num_services = preferences.getInt("num_services", 0);
    if (num_services > MAX_SERVICES) num_services = 0;
    
    for (int i = 0; i < num_services; i++) {
        char key[16];
        snprintf(key, sizeof(key), "svc_name_%d", i);
        String name = preferences.getString(key, "");
        snprintf(key, sizeof(key), "svc_secret_%d", i);
        String secret = preferences.getString(key, "");
        
        if (name.length() > 0 && secret.length() > 0) {
            strlcpy(services[i].name, name.c_str(), sizeof(services[i].name));
            strlcpy(services[i].secret_b32, secret.c_str(), sizeof(services[i].secret_b32));
        }
    }
    
    preferences.end();
    return true;
}

bool saveTOTPServices() {
    Preferences preferences;
    if (!preferences.begin("totp-app", false)) return false;
    
    preferences.clear();
    preferences.putInt("num_services", num_services);
    
    for (int i = 0; i < num_services; i++) {
        char key[16];
        snprintf(key, sizeof(key), "svc_name_%d", i);
        preferences.putString(key, services[i].name);
        snprintf(key, sizeof(key), "svc_secret_%d", i);
        preferences.putString(key, services[i].secret_b32);
    }
    
    preferences.end();
    return true;
}

bool addTOTPService(const char* name, const char* secret_b32) {
    if (num_services >= MAX_SERVICES) return false;
    if (strlen(name) > MAX_SERVICE_NAME_LEN) return false;
    if (strlen(secret_b32) > MAX_SECRET_B32_LEN) return false;
    
    uint8_t test_bin[MAX_SECRET_BIN_LEN];
    size_t test_bin_len;
    if (!decodeBase32Secret(secret_b32, test_bin, &test_bin_len)) return false;
    
    strlcpy(services[num_services].name, name, sizeof(services[num_services].name));
    strlcpy(services[num_services].secret_b32, secret_b32, sizeof(services[num_services].secret_b32));
    num_services++;
    
    return saveTOTPServices();
}

bool deleteTOTPService(int index) {
    if (index < 0 || index >= num_services) return false;
    
    for (int i = index; i < num_services - 1; i++) {
        memcpy(&services[i], &services[i + 1], sizeof(TOTPService));
    }
    num_services--;
    
    if (current_service_index >= num_services) {
        current_service_index = num_services - 1;
        if (current_service_index < 0) current_service_index = 0;
    }
    
    return saveTOTPServices();
}

bool decodeBase32Secret(const char* secret_b32, uint8_t* key_bin, size_t* key_bin_len) {
    Base32 b32;
    size_t input_len = strlen(secret_b32);
    *key_bin_len = b32.fromBase32((const uint8_t*)secret_b32, input_len, key_bin);
    return (*key_bin_len > 0 && *key_bin_len <= MAX_SECRET_BIN_LEN);
}

uint32_t calculateTOTPInterval(uint64_t timestamp_utc) {
    return timestamp_utc / TOTP_INTERVAL;
}

void generateTOTPCode(uint64_t timestamp_utc) {
    if (num_services == 0 || current_service_index < 0 || current_service_index >= num_services) {
        strcpy(current_totp.code, "------");
        current_totp.valid_key = false;
        return;
    }
    
    TOTPService& service = services[current_service_index];
    
    // Decode Base32 secret if needed
    if (current_totp.last_generated_interval == 0 || !current_totp.valid_key) {
        current_totp.valid_key = decodeBase32Secret(service.secret_b32, 
                                                   current_totp.key_bin, 
                                                   &current_totp.key_bin_len);
        if (!current_totp.valid_key) {
            strcpy(current_totp.code, "------");
            return;
        }
    }
    
    uint32_t interval = calculateTOTPInterval(timestamp_utc);
    
    // Only regenerate if interval changed
    if (interval == current_totp.last_generated_interval) return;
    current_totp.last_generated_interval = interval;
    
    // Convert interval to big-endian bytes
    uint8_t counter[8] = {0};
    for (int i = 7; i >= 0; i--) {
        counter[i] = interval & 0xFF;
        interval >>= 8;
    }
    
    // Calculate HMAC-SHA1
    uint8_t hmac[20];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
    mbedtls_md_hmac_starts(&ctx, current_totp.key_bin, current_totp.key_bin_len);
    mbedtls_md_hmac_update(&ctx, counter, sizeof(counter));
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);
    
    // Get offset and truncate
    int offset = hmac[19] & 0x0F;
    uint32_t truncated = ((hmac[offset] & 0x7F) << 24) |
                        ((hmac[offset + 1] & 0xFF) << 16) |
                        ((hmac[offset + 2] & 0xFF) << 8) |
                        (hmac[offset + 3] & 0xFF);
    
    // Generate 6-digit code
    truncated = truncated % 1000000;
    snprintf(current_totp.code, sizeof(current_totp.code), "%06d", truncated);
}