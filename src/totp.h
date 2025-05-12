#pragma once

#include "types.h"
#include "globals.h"
#include <Base32.h>

// Function declarations
bool initTOTPServices();
bool saveTOTPServices();
bool addTOTPService(const char* name, const char* secret_b32);
bool deleteTOTPService(int index);
void generateTOTPCode(uint64_t timestamp_utc);
bool decodeBase32Secret(const char* secret_b32, uint8_t* key_bin, size_t* key_bin_len);
uint32_t calculateTOTPInterval(uint64_t timestamp_utc);
uint32_t generateTOTP(const uint8_t* key, size_t keyLength, uint64_t timestamp, uint32_t interval);