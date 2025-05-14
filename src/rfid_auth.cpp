#include "rfid_auth.h"
#include "globals.h"      // Ensure globals.h is included for global variables
#include "config.h"
#include "ui_manager.h" // For ui_queue_message_fmt
#include "i18n.h"       // For StringID
#include <Arduino.h>    // For Serial, strncpy, etc.

void rfid_auth_init() {
    rfid_auth_load_cards(); // Load cards first
    rfid_auth_load_lock_state(); // Load lock state
    rfid_auth_load_lock_timeout(); // Load lock timeout setting
}

// ---- Lock State Management ----
void rfid_auth_save_lock_state(bool locked) {
    preferences.begin(NVS_NAMESPACE, false); // Read-write
    if (preferences.putBool(NVS_LOCK_STATE_KEY, locked)) {
        is_locked = locked; // Update global state only on successful save
        Serial.printf("[Auth] Lock state saved: %s\n", is_locked ? "LOCKED" : "UNLOCKED");
    } else {
        Serial.println("[Auth ERROR] Failed to save lock state to NVS!");
        // Optionally, queue a UI message if this is critical
    }
    preferences.end();
}

void rfid_auth_load_lock_state() {
    preferences.begin(NVS_NAMESPACE, true); // Read-only
    // Default to locked if not found or on first boot
    is_locked = preferences.getBool(NVS_LOCK_STATE_KEY, true); 
    preferences.end();
    Serial.printf("[Auth] Lock state loaded: %s\n", is_locked ? "LOCKED" : "UNLOCKED");
}

// ---- Authorized Card Management ----
void rfid_auth_load_cards() {
    preferences.begin(NVS_NAMESPACE, true); // Read-only
    authorized_card_count = preferences.getInt(NVS_AUTH_CARD_COUNT_KEY, 0);
    if (authorized_card_count < 0 || authorized_card_count > MAX_AUTHORIZED_CARDS) {
        Serial.printf("[Auth] Invalid authorized card count (%d) from NVS. Resetting to 0.\n", authorized_card_count);
        authorized_card_count = 0;
        // Consider correcting NVS if it was invalid by opening for write and setting to 0.
    }

    Serial.printf("[Auth] Loading %d authorized card(s)...\n", authorized_card_count);
    for (int i = 0; i < authorized_card_count; ++i) {
        char card_key[20];
        snprintf(card_key, sizeof(card_key), "%s%d", NVS_AUTH_CARD_PREFIX, i);
        String card_uid_str = preferences.getString(card_key, "");
        if (card_uid_str.length() > 0 && card_uid_str.length() < MAX_CARD_ID_LEN) {
            strncpy(authorized_card_ids[i], card_uid_str.c_str(), MAX_CARD_ID_LEN -1);
            authorized_card_ids[i][MAX_CARD_ID_LEN -1] = '\0'; // Ensure null termination
            Serial.printf("[Auth]  Loaded card %d: %s\n", i, authorized_card_ids[i]);
        } else {
            Serial.printf("[Auth WARN] Invalid or empty UID for card key %s. Clearing slot %d.\n", card_key, i);
            memset(authorized_card_ids[i], 0, MAX_CARD_ID_LEN);
            // This might lead to a gap. Consider compacting the list or re-saving if this happens.
        }
    }
    preferences.end();
}

bool rfid_auth_save_cards() {
    preferences.begin(NVS_NAMESPACE, false); // Read-write
    if (!preferences.putInt(NVS_AUTH_CARD_COUNT_KEY, authorized_card_count)) {
        Serial.println("[Auth ERROR] Failed to save authorized card count to NVS!");
        preferences.end();
        return false;
    }

    Serial.printf("[Auth] Saving %d authorized card(s)...\n", authorized_card_count);
    for (int i = 0; i < authorized_card_count; ++i) {
        char card_key[20];
        snprintf(card_key, sizeof(card_key), "%s%d", NVS_AUTH_CARD_PREFIX, i);
        if (strlen(authorized_card_ids[i]) > 0) { // Only save if ID is not empty
            if (!preferences.putString(card_key, authorized_card_ids[i])) {
                Serial.printf("[Auth ERROR] Failed to save card UID for key %s!\n", card_key);
                preferences.end();
                return false; // Abort on first error
            }
            Serial.printf("[Auth]  Saved card %d: %s\n", i, authorized_card_ids[i]);
        } else {
            // If an ID is empty, remove its key from NVS to keep things clean
            preferences.remove(card_key);
            Serial.printf("[Auth]  Slot %d was empty, removed NVS key %s.\n", i, card_key);
        }
    }
    
    // If the number of cards decreased, remove old NVS entries
    int old_nvs_count = preferences.getInt("old_auth_c_cnt_temp", authorized_card_count); // Need a way to know previous NVS count
    // This part is tricky without knowing the previous NVS count. 
    // A simpler approach is to always clear keys from authorized_card_count up to MAX_AUTHORIZED_CARDS.
    for (int i = authorized_card_count; i < MAX_AUTHORIZED_CARDS; ++i) {
        char card_key[20];
        snprintf(card_key, sizeof(card_key), "%s%d", NVS_AUTH_CARD_PREFIX, i);
        if (preferences.isKey(card_key)) {
            preferences.remove(card_key);
            Serial.printf("[Auth]  Removed unused NVS key %s.\n", card_key);
        }
    }

    preferences.end();
    return true;
}

bool rfid_auth_is_card_authorized(const char* card_uid) {
    if (!card_uid || strlen(card_uid) == 0) return false;
    for (int i = 0; i < authorized_card_count; ++i) {
        if (strcmp(authorized_card_ids[i], card_uid) == 0) {
            return true;
        }
    }
    return false;
}

bool rfid_auth_add_card(const char* card_uid) {
    if (!card_uid || strlen(card_uid) == 0 || strlen(card_uid) >= MAX_CARD_ID_LEN) {
        Serial.println("[Auth ERROR] Attempt to add invalid or too long card UID.");
        // This error is more for internal/dev, UI should prevent this if possible.
        return false;
    }
    if (rfid_auth_is_card_authorized(card_uid)) {
        Serial.printf("[Auth WARN] Card %s already authorized.\n", card_uid);
        ui_queue_message(getText(StringID::STR_ERROR_CARD_ALREADY_EXISTS), COLOR_ACCENT, 2000, ScreenState::SCREEN_MANAGE_CARDS);
        return false; 
    }
    if (authorized_card_count >= MAX_AUTHORIZED_CARDS) {
        Serial.println("[Auth ERROR] Maximum number of authorized cards reached.");
        ui_queue_message(getText(StringID::STR_ERROR_MAX_CARDS), COLOR_ERROR, 2000, ScreenState::SCREEN_MANAGE_CARDS);
        return false;
    }

    strncpy(authorized_card_ids[authorized_card_count], card_uid, MAX_CARD_ID_LEN -1);
    authorized_card_ids[authorized_card_count][MAX_CARD_ID_LEN -1] = '\0';
    authorized_card_count++;
    
    if (rfid_auth_save_cards()) {
        Serial.printf("[Auth] Card %s added and list saved.\n", card_uid);
        return true;
    } else {
        Serial.printf("[Auth ERROR] Failed to save card list after adding %s. Reverting add.\n", card_uid);
        authorized_card_count--; 
        memset(authorized_card_ids[authorized_card_count], 0, MAX_CARD_ID_LEN);
        ui_queue_message(getText(StringID::STR_ERROR_SAVING_CARD), COLOR_ERROR, 2000, ScreenState::SCREEN_MANAGE_CARDS);
        return false;
    }
}

bool rfid_auth_delete_card(int index) {
    if (index < 0 || index >= authorized_card_count) {
        Serial.printf("[Auth ERROR] Attempt to delete invalid card index: %d\n", index);
        // This is an internal error, should not happen with proper UI checks.
        return false;
    }
    Serial.printf("[Auth] Deleting card at index %d: %s\n", index, authorized_card_ids[index]);

    // Shift remaining cards to fill the gap
    for (int i = index; i < authorized_card_count - 1; ++i) {
        strncpy(authorized_card_ids[i], authorized_card_ids[i+1], MAX_CARD_ID_LEN);
    }
    // Clear the last (now duplicate) entry
    memset(authorized_card_ids[authorized_card_count - 1], 0, MAX_CARD_ID_LEN);
    authorized_card_count--;

    if (rfid_auth_save_cards()) {
        Serial.println("[Auth] Card deleted and list saved.");
        return true;
    } else {
        Serial.println("[Auth ERROR] Failed to save card list after deleting. Consider reloading from NVS to restore state.");
        ui_queue_message(getText(StringID::STR_ERROR_DELETING_CARD), COLOR_ERROR, 2000, ScreenState::SCREEN_MANAGE_CARDS);
        // Attempt to reload cards to revert to a consistent state if save failed.
        rfid_auth_load_cards(); 
        return false;
    }
}

// ---- Lock Timeout Management ----
void rfid_auth_load_lock_timeout() {
    preferences.begin(NVS_NAMESPACE, true); // Read-only
    int stored_minutes = preferences.getInt(NVS_LOCK_TIMEOUT_KEY, 5); // Default 5 minutes
    preferences.end();
    
    // Ensure the loaded value is within reasonable bounds (e.g., 1 to 60 minutes)
    if (stored_minutes < 1) stored_minutes = 1;
    if (stored_minutes > 60) stored_minutes = 60; // Max 1 hour for this example
    
    ::temp_lock_timeout_minutes = stored_minutes; // Update the global variable used for editing
    Serial.printf("[Auth] Lock timeout loaded from NVS: %d minutes. Constrained to: %d minutes.\n", stored_minutes, ::temp_lock_timeout_minutes);
}

void rfid_auth_save_lock_timeout(int timeout_minutes) {
    Serial.printf("[Auth] Attempting to save lock timeout: %d minutes.\n", timeout_minutes);
    // Ensure the value to save is within reasonable bounds
    if (timeout_minutes < 1) timeout_minutes = 1;
    if (timeout_minutes > 60) timeout_minutes = 60;

    preferences.begin(NVS_NAMESPACE, false); // Read-write
    if (preferences.putInt(NVS_LOCK_TIMEOUT_KEY, timeout_minutes)) {
        ::temp_lock_timeout_minutes = timeout_minutes; // Update global on successful save
        Serial.printf("[Auth] Lock timeout saved to NVS: %d minutes.\n", ::temp_lock_timeout_minutes);
    } else {
        Serial.println("[Auth ERROR] Failed to save lock timeout to NVS!");
    }
    preferences.end();
}

// This function might not be strictly necessary if temp_lock_timeout_minutes is always kept up-to-date
// and used as the source of truth for the current setting.
int rfid_auth_get_lock_timeout_minutes() {
    // Could re-read from NVS or just return the global temp_lock_timeout_minutes
    // For simplicity, assume temp_lock_timeout_minutes is authoritative after load/save.
    return temp_lock_timeout_minutes;
}
