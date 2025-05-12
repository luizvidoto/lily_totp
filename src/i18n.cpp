// i18n.cpp
#include "i18n.h"
#include "globals.h" // Para current_language, current_strings_ptr, preferences
#include "config.h"  // Para NVS_LANG_KEY, NVS_NAMESPACE
#include <Arduino.h> // Para Serial

// ---- Definições das Strings ----
// PT-BR
const LanguageStrings strings_pt_br = {{
    "Codigo TOTP", "Menu Principal", "Adicionar Servico", "Ler Cartao RFID", "Confirmar Adicao",
    "Ajustar Hora", "Confirmar Exclusao", "Ajustar Fuso", "Selecionar Idioma",
    "Adicionar Servico", "Ler Cartao RFID", "Ver Codigos TOTP", "Ajustar Hora/Data",
    "Ajustar Fuso Horario", "Idioma",
    "Portugues (BR)", "Ingles (US)",
    "Aguardando JSON", "via Serial...", "Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}",
    "Ex JSON Hora: {'y':ANO,'mo':MES,'d':DIA,'h':HH,'m':MM,'s':SS}",
    "Servicos: %d/%d",
    "Ant/Prox | LP: Sel | DblClick: Menu", // STR_FOOTER_GENERIC_NAV
    "Ant: Canc | Prox: Conf",             // STR_FOOTER_CONFIRM_NAV
    "Ant/Prox: +/- | LP: Campo/Salvar | Dbl: Menu", // STR_FOOTER_TIME_EDIT_NAV
    "Ant/Prox: +/- | LP: Salvar | Dbl: Menu",       // STR_FOOTER_TIMEZONE_NAV
    "Ant/Prox | LP: Salvar | Dbl: Menu",            // STR_FOOTER_LANG_NAV
    "(Fuso atual: GMT %+d)", "Ou envie JSON via Serial (UTC):", "GMT %+d",
    "Adicionar Servico:", "Deletar '%s'?",
    "Servico Adicionado!", "Servico Deletado!", "Hora Ajustada:\n%02d:%02d:%02d (Local)",
    "Fuso Salvo:\nGMT %+d", "Idioma Salvo!",
    "Erro RTC! Verifique conexao/bateria.", "Nenhum servico\ncadastrado!", "Erro JSON:\n%s",
    "JSON Invalido:\nFalta 'name' ou 'secret'", "JSON Invalido:\nFaltam campos Y/M/D h:m:s",
    "Nome Servico\nInvalido!", "Segredo Invalido!", "Segredo Base32\ncontem chars invalidos!",
    "Valores Data/Hora\nInvalidos!", "Erro ao Salvar\nServico!", "Erro ao Deletar\nServico!",
    "[NVS] Erro ao carregar config!", "[NVS] Erro ao salvar config!",
    "Falha ao decodificar\nchave Base32!", "Maximo de servicos\natingido (%d)!",
    "------", "Codigo TOTP",
    "Aproxime o cartao\nRFID do leitor...", "ID Cartao: %s",
    "Iniciando..."
}};

// EN-US
const LanguageStrings strings_en_us = {{
    "TOTP Code", "Main Menu", "Add Service", "Read RFID Card", "Confirm Addition",
    "Adjust Time", "Confirm Deletion", "Adjust Timezone", "Select Language",
    "Add Service", "Read RFID Card", "View TOTP Codes", "Adjust Time/Date",
    "Adjust Timezone", "Language",
    "Portuguese (BR)", "English (US)",
    "Awaiting JSON", "via Serial...", "Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}",
    "Ex Time JSON: {'y':YR,'mo':MON,'d':DAY,'h':HH,'m':MM,'s':SS}",
    "Services: %d/%d",
    "Prev/Next | LP: Sel | DblClick: Menu", // STR_FOOTER_GENERIC_NAV
    "Prev: Canc | Next: Conf",             // STR_FOOTER_CONFIRM_NAV
    "Prev/Next: +/- | LP: Field/Save | Dbl: Menu", // STR_FOOTER_TIME_EDIT_NAV
    "Prev/Next: +/- | LP: Save | Dbl: Menu",       // STR_FOOTER_TIMEZONE_NAV
    "Prev/Next | LP: Save | Dbl: Menu",            // STR_FOOTER_LANG_NAV
    "(Current Zone: GMT %+d)", "Or send JSON via Serial (UTC):", "GMT %+d",
    "Add Service:", "Delete '%s'?",
    "Service Added!", "Service Deleted!", "Time Adjusted:\n%02d:%02d:%02d (Local)",
    "Zone Saved:\nGMT %+d", "Language Saved!",
    "RTC Error! Check connection/battery.", "No services\nregistered!", "JSON Error:\n%s",
    "Invalid JSON:\nMissing 'name' or 'secret'", "Invalid JSON:\nMissing Y/M/D h:m:s fields",
    "Invalid Service\nName!", "Invalid Secret!", "Base32 Secret\nhas invalid chars!",
    "Invalid Date/Time\nValues!", "Error Saving\nService!", "Error Deleting\nService!",
    "[NVS] Error loading config!", "[NVS] Error saving config!",
    "Failed to decode\nBase32 key!", "Maximum services\nreached (%d)!",
    "------", "TOTP Code",
    "Approach RFID card\nto the reader...", "Card ID: %s",
    "Initializing..."
}};

const LanguageStrings* const languages[] = { // Array de ponteiros para const LanguageStrings
    &strings_pt_br,
    &strings_en_us
};

int current_language_menu_index = 0; // Para a tela de seleção

void i18n_init() {
    preferences.begin(NVS_NAMESPACE, true); // Abre NVS para leitura
    int saved_lang_index = preferences.getInt(NVS_LANG_KEY, static_cast<int>(Language::LANG_PT_BR));
    preferences.end();

    if (saved_lang_index >= 0 && saved_lang_index < static_cast<int>(Language::NUM_LANGUAGES)) {
        current_language = static_cast<Language>(saved_lang_index);
    } else {
        current_language = Language::LANG_PT_BR; // Fallback
        Serial.printf("[i18n] Invalid language index %d from NVS, defaulting to PT_BR.\n", saved_lang_index);
    }
    current_strings_ptr = languages[static_cast<int>(current_language)];
    current_language_menu_index = static_cast<int>(current_language); // Sincroniza seleção do menu
    Serial.printf("[i18n] Language set to: %d\n", static_cast<int>(current_language));
}

void i18n_set_language(Language lang) {
    if (static_cast<int>(lang) >= 0 && static_cast<int>(lang) < static_cast<int>(Language::NUM_LANGUAGES)) {
        current_language = lang;
        current_strings_ptr = languages[static_cast<int>(current_language)];
        current_language_menu_index = static_cast<int>(current_language);
    }
}

Language i18n_get_current_language_selection() {
    return static_cast<Language>(current_language_menu_index);
}

void i18n_cycle_language_selection(bool next) {
    if (next) {
        current_language_menu_index = (current_language_menu_index + 1) % static_cast<int>(Language::NUM_LANGUAGES);
    } else {
        current_language_menu_index = (current_language_menu_index - 1 + static_cast<int>(Language::NUM_LANGUAGES)) % static_cast<int>(Language::NUM_LANGUAGES);
    }
}

void i18n_save_selected_language() {
    Language selected_lang = static_cast<Language>(current_language_menu_index);
    if (selected_lang != current_language) {
        i18n_set_language(selected_lang);
        preferences.begin(NVS_NAMESPACE, false); // Abre NVS para escrita
        preferences.putInt(NVS_LANG_KEY, static_cast<int>(current_language));
        preferences.end();
        Serial.printf("[i18n] Language saved to NVS: %d\n", static_cast<int>(current_language));
        // A mensagem de sucesso e redraw da tela são tratados pelo chamador (input_handler)
    }
}

const char* getText(StringID id) {
    if (current_strings_ptr != nullptr && static_cast<int>(id) < static_cast<int>(StringID::NUM_STRINGS)) {
        const char* text = current_strings_ptr->strings[static_cast<int>(id)];
        return (text != nullptr) ? text : "ERR_STR_NULL";
    }
    Serial.printf("[i18n] Error: Invalid StringID %d or null strings pointer.\n", static_cast<int>(id));
    return "ERR_STR_ID";
}