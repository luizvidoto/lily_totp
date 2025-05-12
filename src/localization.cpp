#include "localization.h"
#include <Preferences.h>

// Language string tables
const LanguageStrings strings_pt_br = {{
    "Codigo TOTP",
    "Menu Principal",
    "Adicionar Servico",
    "Confirmar Adicao",
    "Ajustar Hora",
    "Confirmar Exclusao",
    "Ajustar Fuso",
    "Selecionar Idioma",
    "Adicionar Servico",
    "Ler Cartao RFID",
    "Ver Codigos TOTP",
    "Ajustar Hora/Data",
    "Ajustar Fuso Horario",
    "Idioma",
    "Portugues (BR)",
    "Ingles (US)",
    "Aguardando JSON",
    "via Serial...",
    "Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}",
    "{'y':..,'mo':..,'d':..,'h':..,'m':..,'s':..}",
    "Servicos: %d/%d",
    "Prev/Next | LP: Sel | DblClick: Menu",
    "Prev: Canc | Next: Conf",
    "Prev/Next: +/- | LP: Campo/Salvar | Dbl: Menu",
    "Prev/Next: +/- | LP: Salvar | Dbl: Menu",
    "Prev/Next | LP: Salvar | Dbl: Menu",
    "(Fuso atual: GMT %+d)",
    "Ou envie JSON via Serial (UTC):",
    "GMT %+d",
    "Adicionar Servico:",
    "Deletar servico:",
    "Servico Adicionado!",
    "Servico Deletado",
    "Hora Ajustada:\n%02d:%02d:%02d",
    "Fuso Salvo:\nGMT %+d",
    "Idioma Salvo!",
    "Erro RTC!",
    "Nenhum servico\ncadastrado!",
    "Erro JSON:\n%s",
    "JSON Invalido:\nFalta 'name'/'secret'",
    "JSON Invalido:\nFaltam campos Y/M/D h:m:s",
    "Nome Servico\nInvalido!",
    "Segredo Invalido!",
    "Valores Data/Hora\nInvalidos!",
    "Erro ao Salvar!",
    "Erro ao Deletar",
    "[NVS] Erro ao carregar",
    "[NVS] Erro ao salvar",
    "Falha chave B32!",
    "Max Servicos!",
    "------",
    "Codigo TOTP",
}};

const LanguageStrings strings_en_us = {{
    "TOTP Code",
    "Main Menu",
    "Add Service",
    "Confirm Addition",
    "Adjust Time",
    "Confirm Deletion",
    "Adjust Timezone",
    "Select Language",
    "Add Service",
    "Read RFID Card",
    "View TOTP Codes",
    "Adjust Time/Date",
    "Adjust Timezone",
    "Language",
    "Portuguese (BR)",
    "English (US)",
    "Awaiting JSON",
    "via Serial...",
    "Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}",
    "{'y':..,'mo':..,'d':..,'h':..,'m':..,'s':..}",
    "Services: %d/%d",
    "Prev/Next | LP: Sel | DblClick: Menu",
    "Prev: Canc | Next: Conf",
    "Prev/Next: +/- | LP: Field/Save | Dbl: Menu",
    "Prev/Next: +/- | LP: Save | Dbl: Menu",
    "Prev/Next | LP: Save | Dbl: Menu",
    "(Current Zone: GMT %+d)",
    "Or send JSON via Serial (UTC):",
    "GMT %+d",
    "Add Service:",
    "Delete service:",
    "Service Added!",
    "Service Deleted",
    "Time Adjusted:\n%02d:%02d:%02d",
    "Zone Saved:\nGMT %+d",
    "Language Saved!",
    "RTC Error!",
    "No services\nregistered!",
    "JSON Error:\n%s",
    "Invalid JSON:\nMissing 'name'/'secret'",
    "Invalid JSON:\nMissing Y/M/D h:m:s",
    "Invalid Service\nName!",
    "Invalid Secret!",
    "Invalid Date/Time\nValues!",
    "Error Saving!",
    "Error Deleting",
    "[NVS] Load error",
    "[NVS] Save error",
    "B32 Key Error!",
    "Max Services!",
    "------",
    "TOTP Code",
}};

const LanguageStrings *languages[NUM_LANGUAGES] = {&strings_pt_br, &strings_en_us};
Language current_language = LANG_PT_BR;
const LanguageStrings *current_strings = languages[current_language];

const char* getText(StringID id) {
    if (id < NUM_STRINGS && current_strings != nullptr) {
        const char* text = current_strings->strings[id];
        return (text != nullptr) ? text : "";
    }
    Serial.printf("[ERROR] getText: ID inválido %d ou ponteiro nulo\n", id);
    return "?ERR?";
}

bool saveLanguagePreference(Language lang) {
    if (lang >= NUM_LANGUAGES) return false;
    
    Preferences preferences;
    if (!preferences.begin("totp-app", false)) return false;
    
    bool success = preferences.putInt("language", (int)lang);
    preferences.end();
    
    if (success) {
        current_language = lang;
        current_strings = languages[current_language];
    }
    
    return success;
}