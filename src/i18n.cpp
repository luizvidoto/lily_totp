// i18n.cpp
#include "i18n.h"
#include "globals.h" // Para current_language, current_strings_ptr, preferences
#include "config.h"  // Para NVS_LANG_KEY, NVS_NAMESPACE
#include <Arduino.h> // Para Serial

// ---- Definições das Strings ----
// As strings DEVEM estar na mesma ordem que no enum StringID em types.h

const LanguageStrings strings_pt_br = {
    /* STR_TOTP_CODE_TITLE */           "Codigo TOTP",
    /* STR_TITLE_MAIN_MENU */           "Menu Principal",
    /* STR_TITLE_ADD_SERVICE */         "Adicionar Servico",
    /* STR_TITLE_READ_RFID */           "Ler Cartao RFID",
    /* STR_TITLE_CONFIRM_ADD */         "Confirmar Adicao",
    /* STR_TITLE_ADJUST_TIME */         "Ajustar Hora",
    /* STR_TITLE_CONFIRM_DELETE */      "Confirmar Exclusao",
    /* STR_TITLE_ADJUST_TIMEZONE */     "Ajustar Fuso",
    /* STR_TITLE_SELECT_LANGUAGE */     "Selecionar Idioma",
    /* STR_MENU_ADD_SERVICE */          "Adicionar Servico",
    /* STR_MENU_READ_RFID_OPTION */     "Ler Cartao RFID",
    /* STR_MENU_VIEW_CODES */           "Ver Codigos TOTP",
    /* STR_MENU_ADJUST_TIME */          "Ajustar Hora/Data",
    /* STR_MENU_ADJUST_TIMEZONE */      "Ajustar Fuso Horario",
    /* STR_MENU_SELECT_LANGUAGE */      "Idioma",
    /* STR_LABEL_CODES */               "Codes",
    /* STR_LABEL_CONFIG */              "Config",
    /* STR_LABEL_ZONE */                "Zone",
    /* STR_LABEL_RFID */                "RFID",
    /* STR_LABEL_LANG */                "Lang",
    /* STR_LABEL_DATE */                "Date",
    /* STR_ACTION_CONFIG */             "Configuracoes",
    /* STR_LANG_PORTUGUESE */           "Portugues (BR)",
    /* STR_LANG_ENGLISH */              "Ingles (US)",
    /* STR_AWAITING_JSON */             "Aguardando JSON",
    /* STR_VIA_SERIAL */                "via Serial...",
    /* STR_EXAMPLE_JSON_SERVICE */      "Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}",
    /* STR_EXAMPLE_JSON_TIME */         "Ex JSON Hora: {'y':ANO,'mo':MES,'d':DIA,'h':HH,'m':MM,'s':SS}",
    /* STR_SERVICES_STATUS_FMT */       "Servicos: %d/%d",
    /* STR_FOOTER_GENERIC_NAV */        "Ant/Prox | LP: Sel | DblClick: Menu",
    /* STR_FOOTER_CONFIRM_NAV */        "Ant: Canc | Prox: Conf",
    /* STR_FOOTER_TIME_EDIT_NAV */      "Ant/Prox: +/- | LP: Campo/Salvar | Dbl: Menu",
    /* STR_FOOTER_TIMEZONE_NAV */       "Ant/Prox: +/- | LP: Salvar | Dbl: Menu",
    /* STR_FOOTER_LANG_NAV */           "Ant/Prox | LP: Salvar | Dbl: Menu",
    /* STR_TIME_EDIT_INFO_FMT */        "(Fuso atual: GMT %+d)",
    /* STR_TIME_EDIT_JSON_HINT */       "Ou envie JSON via Serial (UTC):",
    /* STR_TIMEZONE_LABEL */            "GMT %+d",
    /* STR_CONFIRM_ADD_PROMPT */        "Adicionar Servico:",
    /* STR_CONFIRM_DELETE_PROMPT_FMT */ "Deletar '%s'?",
    /* STR_SERVICE_ADDED */             "Servico Adicionado!",
    /* STR_SERVICE_DELETED */           "Servico Deletado!",
    /* STR_TIME_ADJUSTED_FMT */         "Hora Ajustada:\n%02d:%02d:%02d (Local)",
    /* STR_TIMEZONE_SAVED_FMT */        "Fuso Salvo:\nGMT %+d",
    /* STR_LANG_SAVED */                "Idioma Salvo!",
    /* STR_ERROR_RTC */                 "Erro RTC! Verifique conexao/bateria.",
    /* STR_ERROR_RTC_INIT */            "Falha Init RTC!",
    /* STR_ERROR_NO_SERVICES */         "Nenhum servico\ncadastrado!",
    /* STR_ERROR_JSON_PARSE_FMT */      "Erro JSON:\n%s",
    /* STR_ERROR_JSON_INVALID_SERVICE */"JSON Invalido:\nFalta 'name' ou 'secret'",
    /* STR_ERROR_JSON_INVALID_TIME */   "JSON Invalido:\nFaltam campos Y/M/D h:m:s",
    /* STR_ERROR_INVALID_SVC_NAME */    "Nome Servico\nInvalido!",
    /* STR_ERROR_INVALID_SECRET */      "Segredo Invalido!",
    /* STR_ERROR_INVALID_B32_SECRET */  "Segredo Base32\ncontem chars invalidos!",
    /* STR_ERROR_INVALID_DATETIME */    "Valores Data/Hora\nInvalidos!",
    /* STR_ERROR_SAVING_SERVICE */      "Erro ao Salvar\nServico!",
    /* STR_ERROR_DELETING_SERVICE */    "Erro ao Deletar\nServico!",
    /* STR_ERROR_NVS_LOAD_CONFIG */     "[NVS] Erro ao carregar config!",
    /* STR_ERROR_NVS_SAVE_CONFIG */     "[NVS] Erro ao salvar config!",
    /* STR_ERROR_B32_DECODE */          "Falha ao decodificar\nchave Base32!",
    /* STR_ERROR_MAX_SERVICES_FMT */    "Maximo de servicos\natingido (%d)!",
    /* STR_TOTP_NO_SERVICE_TITLE */     "------",
    /* STR_TOTP_CODE_PLACEHOLDER */     "Codigo TOTP",
    /* STR_RFID_PROMPT */               "Aproxime o cartao\nRFID do leitor...",
    /* STR_RFID_ID_PREFIX */            "ID Cartao: %s",
    /* STR_STARTUP_MSG */               "Iniciando...",
    /* STR_NOT_IMPLEMENTED_YET */       "Nao implementado!",
    /* STR_TITLE_LOCKED */              "Bloqueado",
    /* STR_TITLE_MANAGE_CARDS */        "Gerenciar Cartoes",
    /* STR_TITLE_ADD_CARD */            "Adicionar Cartao",
    /* STR_TITLE_CONFIRM_ADD_CARD */    "Confirmar Cartao",
    /* STR_TITLE_CONFIRM_DELETE_CARD */ "Remover Cartao",
    /* STR_LOCKED_PROMPT */             "Aproxime o cartao\npara desbloquear",
    /* STR_MENU_MANAGE_CARDS */         "Gerenciar Cartoes",
    /* STR_CARD_LIST_EMPTY */           "Nenhum cartao\nautorizado.",
    /* STR_ADD_CARD_PROMPT */           "Aproxime novo cartao...",
    /* STR_SCAN_CARD_PROMPT */          "Lendo cartao...",
    /* STR_CONFIRM_ADD_CARD_PROMPT_FMT */ "Adicionar cartao\nID: %s?",
    /* STR_CONFIRM_DELETE_CARD_PROMPT_FMT */ "Remover cartao\nID: %s?",
    /* STR_CARD_ADDED */                "Cartao adicionado!",
    /* STR_CARD_DELETED */              "Cartao removido!",
    /* STR_ERROR_MAX_CARDS */           "Max. de cartoes\natingido!",
    /* STR_ERROR_CARD_ALREADY_EXISTS */ "Cartao ja existe!",
    /* STR_ERROR_SAVING_CARD */         "Erro ao salvar\ncartao!",
    /* STR_ERROR_DELETING_CARD */       "Erro ao remover\ncartao!",
    /* STR_FOOTER_MANAGE_CARDS_NAV */   "Prox | LP: Add/Del | Dbl: Menu",
    /* STR_FOOTER_ADD_CARD_NAV */       "LP: Canc | Dbl: Menu",
    /* STR_FOOTER_CONFIRM_CARD_NAV */   "Ant: Canc | Prox: Conf | Dbl:Menu",
    /* STR_TITLE_CONFIG_MENU */         "Configuracoes",
    /* STR_MENU_CONFIG_MANAGE_CARDS */  "Gerenciar Cartoes",
    /* STR_MENU_CONFIG_LOCK_TIMEOUT */  "Timeout Bloqueio",
    /* STR_MENU_CONFIG_BRIGHTNESS */    "Ajustar Brilho",
    /* STR_FOOTER_CONFIG_SUBMENU_NAV */ "Ant/Prox | LP: Sel | Dbl: Menu",
    /* STR_ERROR_CARD_UNAUTHORIZED */   "Cartao nao\nautorizado!",
    /* STR_TITLE_ADJUST_LOCK_TIMEOUT */ "Timeout Bloqueio",
    /* STR_LOCK_TIMEOUT_LABEL_FMT */    "Bloquear apos: %d min",
    /* STR_LOCK_TIMEOUT_SAVED_FMT */    "Timeout salvo: %d min",
    /* STR_FOOTER_ADJUST_VALUE_NAV */   "Ant/Prox: +/- | LP: Salvar | Dbl: Menu",
    /* STR_TITLE_BRIGHTNESS_MENU */         "Ajustar Brilho",
    /* STR_MENU_BRIGHTNESS_USB */           "Brilho USB",
    /* STR_MENU_BRIGHTNESS_BATTERY */       "Brilho Bateria",
    /* STR_MENU_BRIGHTNESS_DIMMED */        "Brilho Reduzido",
    /* STR_TITLE_ADJUST_BRIGHTNESS_VALUE_FMT */ "Brilho (%s): %d",
    /* STR_BRIGHTNESS_SAVED_FMT */            "Brilho (%s) salvo: %d",
    /* STR_TITLE_SETUP_FIRST_CARD */    "Configurar Cartao",
    /* STR_PROMPT_SETUP_FIRST_CARD */   "Aproxime o 1º cartao\npara registrar",
    /* STR_FIRST_CARD_SAVED */          "Primeiro cartao\nsalvo! Desbloqueado.",
    /* STR_ERROR_SAVING_FIRST_CARD */   "Erro ao salvar\nprimeiro cartao!",

    // Nova String para quando o último cartão é deletado
    /* STR_LAST_CARD_DELETED_SETUP_NEW */ "Ultimo cartao removido!\nAproxime novo cartao."
};

const LanguageStrings strings_en_us = {
    /* STR_TOTP_CODE_TITLE */           "TOTP Code",
    /* STR_TITLE_MAIN_MENU */           "Main Menu",
    /* STR_TITLE_ADD_SERVICE */         "Add Service",
    /* STR_TITLE_READ_RFID */           "Read RFID Card",
    /* STR_TITLE_CONFIRM_ADD */         "Confirm Addition",
    /* STR_TITLE_ADJUST_TIME */         "Adjust Time",
    /* STR_TITLE_CONFIRM_DELETE */      "Confirm Deletion",
    /* STR_TITLE_ADJUST_TIMEZONE */     "Adjust Timezone",
    /* STR_TITLE_SELECT_LANGUAGE */     "Select Language",
    /* STR_MENU_ADD_SERVICE */          "Add Service",
    /* STR_MENU_READ_RFID_OPTION */     "Read RFID Card",
    /* STR_MENU_VIEW_CODES */           "View TOTP Codes",
    /* STR_MENU_ADJUST_TIME */          "Adjust Time/Date",
    /* STR_MENU_ADJUST_TIMEZONE */      "Adjust Timezone",
    /* STR_MENU_SELECT_LANGUAGE */      "Language",
    /* STR_LABEL_CODES */               "Codes",
    /* STR_LABEL_CONFIG */              "Config",
    /* STR_LABEL_ZONE */                "Zone",
    /* STR_LABEL_RFID */                "RFID",
    /* STR_LABEL_LANG */                "Lang",
    /* STR_LABEL_DATE */                "Date",
    /* STR_ACTION_CONFIG */             "Settings",
    /* STR_LANG_PORTUGUESE */           "Portuguese (BR)",
    /* STR_LANG_ENGLISH */              "English (US)",
    /* STR_AWAITING_JSON */             "Awaiting JSON",
    /* STR_VIA_SERIAL */                "via Serial...",
    /* STR_EXAMPLE_JSON_SERVICE */      "Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}",
    /* STR_EXAMPLE_JSON_TIME */         "Ex Time JSON: {'y':YR,'mo':MON,'d':DAY,'h':HH,'m':MM,'s':SS}",
    /* STR_SERVICES_STATUS_FMT */       "Services: %d/%d",
    /* STR_FOOTER_GENERIC_NAV */        "Prev/Next | LP: Sel | DblClick: Menu",
    /* STR_FOOTER_CONFIRM_NAV */        "Prev: Canc | Next: Conf",
    /* STR_FOOTER_TIME_EDIT_NAV */      "Prev/Next: +/- | LP: Field/Save | Dbl: Menu",
    /* STR_FOOTER_TIMEZONE_NAV */       "Prev/Next: +/- | LP: Save | Dbl: Menu",
    /* STR_FOOTER_LANG_NAV */           "Prev/Next | LP: Save | Dbl: Menu",
    /* STR_TIME_EDIT_INFO_FMT */        "(Current Zone: GMT %+d)",
    /* STR_TIME_EDIT_JSON_HINT */       "Or send JSON via Serial (UTC):",
    /* STR_TIMEZONE_LABEL */            "GMT %+d",
    /* STR_CONFIRM_ADD_PROMPT */        "Add Service:",
    /* STR_CONFIRM_DELETE_PROMPT_FMT */ "Delete '%s'?",
    /* STR_SERVICE_ADDED */             "Service Added!",
    /* STR_SERVICE_DELETED */           "Service Deleted!",
    /* STR_TIME_ADJUSTED_FMT */         "Time Adjusted:\n%02d:%02d:%02d (Local)",
    /* STR_TIMEZONE_SAVED_FMT */        "Zone Saved:\nGMT %+d",
    /* STR_LANG_SAVED */                "Language Saved!",
    /* STR_ERROR_RTC */                 "RTC Error! Check connection/battery.",
    /* STR_ERROR_RTC_INIT */            "RTC Init Fail!",
    /* STR_ERROR_NO_SERVICES */         "No services\nregistered!",
    /* STR_ERROR_JSON_PARSE_FMT */      "JSON Error:\n%s",
    /* STR_ERROR_JSON_INVALID_SERVICE */"Invalid JSON:\nMissing 'name' or 'secret'",
    /* STR_ERROR_JSON_INVALID_TIME */   "Invalid JSON:\nMissing Y/M/D h:m:s fields",
    /* STR_ERROR_INVALID_SVC_NAME */    "Invalid Service\nName!",
    /* STR_ERROR_INVALID_SECRET */      "Invalid Secret!",
    /* STR_ERROR_INVALID_B32_SECRET */  "Base32 Secret\nhas invalid chars!",
    /* STR_ERROR_INVALID_DATETIME */    "Invalid Date/Time\nValues!",
    /* STR_ERROR_SAVING_SERVICE */      "Error Saving\nService!",
    /* STR_ERROR_DELETING_SERVICE */    "Error Deleting\nService!",
    /* STR_ERROR_NVS_LOAD_CONFIG */     "[NVS] Error loading config!",
    /* STR_ERROR_NVS_SAVE_CONFIG */     "[NVS] Error saving config!",
    /* STR_ERROR_B32_DECODE */          "Failed to decode\nBase32 key!",
    /* STR_ERROR_MAX_SERVICES_FMT */    "Maximum services\nreached (%d)!",
    /* STR_TOTP_NO_SERVICE_TITLE */     "------",
    /* STR_TOTP_CODE_PLACEHOLDER */     "TOTP Code",
    /* STR_RFID_PROMPT */               "Approach RFID card\nto the reader...",
    /* STR_RFID_ID_PREFIX */            "Card ID: %s",
    /* STR_STARTUP_MSG */               "Initializing...",
    /* STR_NOT_IMPLEMENTED_YET */       "Not implemented!",
    /* STR_TITLE_LOCKED */              "Locked",
    /* STR_TITLE_MANAGE_CARDS */        "Manage Cards",
    /* STR_TITLE_ADD_CARD */            "Add Card",
    /* STR_TITLE_CONFIRM_ADD_CARD */    "Confirm Card",
    /* STR_TITLE_CONFIRM_DELETE_CARD */ "Delete Card",
    /* STR_LOCKED_PROMPT */             "Present card\nto unlock",
    /* STR_MENU_MANAGE_CARDS */         "Manage Cards",
    /* STR_CARD_LIST_EMPTY */           "No authorized\ncards found.",
    /* STR_ADD_CARD_PROMPT */           "Present new card...",
    /* STR_SCAN_CARD_PROMPT */          "Scanning card...",
    /* STR_CONFIRM_ADD_CARD_PROMPT_FMT */ "Add card\nID: %s?",
    /* STR_CONFIRM_DELETE_CARD_PROMPT_FMT */ "Delete card\nID: %s?",
    /* STR_CARD_ADDED */                "Card added!",
    /* STR_CARD_DELETED */              "Card removed!",
    /* STR_ERROR_MAX_CARDS */           "Max cards\nreached!",
    /* STR_ERROR_CARD_ALREADY_EXISTS */ "Card already exists!",
    /* STR_ERROR_SAVING_CARD */         "Error saving\ncard!",
    /* STR_ERROR_DELETING_CARD */       "Error deleting\ncard!",
    /* STR_FOOTER_MANAGE_CARDS_NAV */   "Next | LP: Add/Del | Dbl: Menu",
    /* STR_FOOTER_ADD_CARD_NAV */       "LP: Canc | Dbl: Menu",
    /* STR_FOOTER_CONFIRM_CARD_NAV */   "Prev: Canc | Next: Conf | Dbl:Menu",
    /* STR_TITLE_CONFIG_MENU */         "Settings",
    /* STR_MENU_CONFIG_MANAGE_CARDS */  "Manage Cards",
    /* STR_MENU_CONFIG_LOCK_TIMEOUT */  "Lock Timeout",
    /* STR_MENU_CONFIG_BRIGHTNESS */    "Adjust Brightness",
    /* STR_FOOTER_CONFIG_SUBMENU_NAV */ "Prev/Next | LP: Sel | Dbl: Menu",
    /* STR_ERROR_CARD_UNAUTHORIZED */   "Card not\nauthorized!",
    /* STR_TITLE_ADJUST_LOCK_TIMEOUT */ "Lock Timeout",
    /* STR_LOCK_TIMEOUT_LABEL_FMT */    "Lock after: %d min",
    /* STR_LOCK_TIMEOUT_SAVED_FMT */    "Timeout saved: %d min",
    /* STR_FOOTER_ADJUST_VALUE_NAV */   "Prev/Next: +/- | LP: Save | Dbl: Menu",
    /* STR_TITLE_BRIGHTNESS_MENU */         "Adjust Brightness",
    /* STR_MENU_BRIGHTNESS_USB */           "USB Brightness",
    /* STR_MENU_BRIGHTNESS_BATTERY */       "Battery Brightness",
    /* STR_MENU_BRIGHTNESS_DIMMED */        "Dimmed Brightness",
    /* STR_TITLE_ADJUST_BRIGHTNESS_VALUE_FMT */ "Brightness (%s): %d",
    /* STR_BRIGHTNESS_SAVED_FMT */            "Brightness (%s) saved: %d",
    /* STR_TITLE_SETUP_FIRST_CARD */    "Setup First Card",
    /* STR_PROMPT_SETUP_FIRST_CARD */   "Present 1st card\nto register",
    /* STR_FIRST_CARD_SAVED */          "First card saved!\nUnlocked.",
    /* STR_ERROR_SAVING_FIRST_CARD */   "Error saving\nfirst card!",

    // Nova String para quando o último cartão é deletado
    /* STR_LAST_CARD_DELETED_SETUP_NEW */ "Last card removed!\nPresent new card."
};

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
    int id_val = static_cast<int>(id);
    if (current_strings_ptr != nullptr && id_val >= 0 && id_val < static_cast<int>(StringID::NUM_STRINGS)) {
        const char* text = current_strings_ptr->strings[id_val];
        if (text == nullptr) {
             Serial.printf("[i18n WARN] StringID %d is NULL in current language!\n", id_val);
            return "ERR_STR_NULL";
        }
        return text;
    }
    Serial.printf("[i18n ERROR] Invalid StringID %d or null strings pointer (NUM_STRINGS: %d).\n", id_val, static_cast<int>(StringID::NUM_STRINGS));
    return "ERR_STR_ID";
}