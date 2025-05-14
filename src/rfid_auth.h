#ifndef RFID_AUTH_H
#define RFID_AUTH_H

#include "globals.h"
#include "types.h"

// Initialization
void rfid_auth_init(); // Loads authorized cards and lock state from NVS

// Lock State Management
void rfid_auth_save_lock_state(bool locked);
void rfid_auth_load_lock_state();

// Authorized Card Management
bool rfid_auth_is_card_authorized(const char* card_uid);
bool rfid_auth_add_card(const char* card_uid);
bool rfid_auth_delete_card(int index);
bool rfid_auth_save_cards(); // Saves the current authorized_card_ids list to NVS
void rfid_auth_load_cards(); // Loads authorized cards from NVS into authorized_card_ids

// Lock Timeout Management
void rfid_auth_load_lock_timeout();
void rfid_auth_save_lock_timeout(int timeout_minutes);
int rfid_auth_get_lock_timeout_minutes(); // Gets the current value from NVS or default

#endif // RFID_AUTH_H
