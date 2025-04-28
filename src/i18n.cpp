#include "i18n.h"
#include "globals.h" // Para acessar/modificar current_language
#include "config.h"  // Se alguma constante for usada aqui
#include <ArduinoJson.h>
#include <pgmspace.h> // Para PROGMEM e funções relacionadas

// ============================================================================
// === DEFINIÇÕES INTERNAS E STRINGS JSON ===
// ============================================================================

// --- Strings JSON Armazenadas na Memória Flash (PROGMEM) ---

// NOTA IMPORTANTE: Mantenha as CHAVES consistentes entre os idiomas!
// Use Raw String Literals R"(...)" para facilitar a escrita do JSON.

const char json_pt_br[] PROGMEM = R"({
  "LANGUAGE_NAME": "Portugues (BR)",
  "TITLE_TOTP_CODE": "Codigo TOTP",
  "TITLE_MAIN_MENU": "Menu Principal",
  "TITLE_ADD_SERVICE": "Adicionar Servico",
  "TITLE_READ_RFID": "Ler Cartao RFID",
  "TITLE_CONFIRM_ADD": "Confirmar Adicao",
  "TITLE_ADJUST_TIME": "Ajustar Hora",
  "TITLE_CONFIRM_DELETE": "Confirmar Exclusao",
  "TITLE_ADJUST_TIMEZONE": "Ajustar Fuso",
  "TITLE_SELECT_LANGUAGE": "Selecionar Idioma",
  "MENU_ADD_SERVICE": "Adicionar Servico",
  "MENU_READ_RFID": "Ler Cartao RFID",
  "MENU_VIEW_CODES": "Ver Codigos TOTP",
  "MENU_ADJUST_TIME": "Ajustar Hora/Data",
  "MENU_ADJUST_TIMEZONE": "Ajustar Fuso Horario",
  "MENU_SELECT_LANGUAGE": "Idioma",
  "LANG_PORTUGUESE": "Portugues (BR)",
  "LANG_ENGLISH": "Ingles (US)",
  "AWAITING_JSON": "Aguardando JSON",
  "VIA_SERIAL": "via Serial...",
  "EXAMPLE_JSON_SERVICE": "Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}",
  "EXAMPLE_JSON_TIME": "{'y':..,'mo':..,'d':..,'h':..,'m':..,'s':..}",
  "SERVICES_STATUS_FMT": "Servicos: %d/%d",
  "RFID_PROMPT": "Aproxime Cartao",
  "RFID_READING": "Lendo...",
  "CARD_READ_FMT": "ID: %s",
  "FOOTER_GENERIC_NAV": "Prev/Next | LP: Sel | DblClick: Menu",
  "FOOTER_CONFIRM_NAV": "Prev: Canc | Next: Conf",
  "FOOTER_TIME_EDIT_NAV": "Prev/Next: +/- | LP: Campo/Salvar | Dbl: Menu",
  "FOOTER_TIMEZONE_NAV": "Prev/Next: +/- | LP: Salvar | Dbl: Menu",
  "FOOTER_LANG_NAV": "Prev/Next | LP: Salvar | Dbl: Menu",
  "TIME_EDIT_INFO_FMT": "(Fuso atual: GMT %+d)",
  "TIME_EDIT_JSON_HINT": "Ou envie JSON via Serial (UTC):",
  "TIMEZONE_LABEL_FMT": "GMT %+d",
  "CONFIRM_ADD_PROMPT": "Adicionar Servico:",
  "CONFIRM_DELETE_PROMPT": "Deletar servico:",
  "SERVICE_ADDED": "Servico Adicionado!",
  "SERVICE_DELETED": "Servico Deletado",
  "TIME_ADJUSTED_FMT": "Hora Ajustada:\n%02d:%02d:%02d",
  "TIMEZONE_SAVED_FMT": "Fuso Salvo:\nGMT %+d",
  "LANG_SAVED": "Idioma Salvo!",
  "ERROR_RTC_FAILED": "Erro RTC!",
  "ERROR_NO_SERVICES": "Nenhum servico\ncadastrado!",
  "ERROR_JSON_PARSE_FMT": "Erro JSON:\n%s",
  "ERROR_JSON_INVALID_SERVICE": "JSON Invalido:\nFalta 'name'/'secret'",
  "ERROR_JSON_INVALID_TIME": "JSON Invalido:\nFaltam campos Y/M/D h:m:s",
  "ERROR_SERVICE_NAME_INVALID": "Nome Servico\nInvalido!",
  "ERROR_SECRET_INVALID": "Segredo Invalido!",
  "ERROR_SECRET_B32_INVALID": "Segredo Base32\nInvalido!",
  "ERROR_TIME_VALUES_INVALID": "Valores Data/Hora\nInvalidos!",
  "ERROR_SAVING_NVS": "Erro ao Salvar NVS!",
  "ERROR_DELETING": "Erro ao Deletar",
  "ERROR_NVS_LOAD": "[NVS] Erro ao carregar",
  "ERROR_NVS_SAVE": "[NVS] Erro ao salvar",
  "ERROR_B32_DECODE": "Falha chave B32!",
  "ERROR_MAX_SERVICES": "Max Servicos!",
  "ERROR_TOTP_GENERATION": "Erro Geracao TOTP",
  "ERROR_RFID_READ": "Erro Leitura RFID",
  "STATUS_CONNECTING_RTC": "Conectando RTC...",
  "STATUS_LOADING_SERVICES": "Carregando Dados...",
  "STATUS_READY": "Pronto!",
  "PLACEHOLDER_NO_CODE": "------",
  "PLACEHOLDER_NO_SERVICE_TITLE": "Sem Servico",
  "PLACEHOLDER_UNKNOWN_KEY": "?KEY?",
  "NONE": ""
})";

const char json_en_us[] PROGMEM = R"({
  "LANGUAGE_NAME": "English (US)",
  "TITLE_TOTP_CODE": "TOTP Code",
  "TITLE_MAIN_MENU": "Main Menu",
  "TITLE_ADD_SERVICE": "Add Service",
  "TITLE_READ_RFID": "Read RFID Card",
  "TITLE_CONFIRM_ADD": "Confirm Addition",
  "TITLE_ADJUST_TIME": "Adjust Time",
  "TITLE_CONFIRM_DELETE": "Confirm Deletion",
  "TITLE_ADJUST_TIMEZONE": "Adjust Timezone",
  "TITLE_SELECT_LANGUAGE": "Select Language",
  "MENU_ADD_SERVICE": "Add Service",
  "MENU_READ_RFID": "Read RFID Card",
  "MENU_VIEW_CODES": "View TOTP Codes",
  "MENU_ADJUST_TIME": "Adjust Time/Date",
  "MENU_ADJUST_TIMEZONE": "Adjust Timezone",
  "MENU_SELECT_LANGUAGE": "Language",
  "LANG_PORTUGUESE": "Portuguese (BR)",
  "LANG_ENGLISH": "English (US)",
  "AWAITING_JSON": "Awaiting JSON",
  "VIA_SERIAL": "via Serial...",
  "EXAMPLE_JSON_SERVICE": "Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}",
  "EXAMPLE_JSON_TIME": "{'y':..,'mo':..,'d':..,'h':..,'m':..,'s':..}",
  "SERVICES_STATUS_FMT": "Services: %d/%d",
  "RFID_PROMPT": "Approach Card",
  "RFID_READING": "Reading...",
  "CARD_READ_FMT": "ID: %s",
  "FOOTER_GENERIC_NAV": "Prev/Next | LP: Sel | DblClick: Menu",
  "FOOTER_CONFIRM_NAV": "Prev: Canc | Next: Conf",
  "FOOTER_TIME_EDIT_NAV": "Prev/Next: +/- | LP: Field/Save | Dbl: Menu",
  "FOOTER_TIMEZONE_NAV": "Prev/Next: +/- | LP: Save | Dbl: Menu",
  "FOOTER_LANG_NAV": "Prev/Next | LP: Save | Dbl: Menu",
  "TIME_EDIT_INFO_FMT": "(Current Zone: GMT %+d)",
  "TIME_EDIT_JSON_HINT": "Or send JSON via Serial (UTC):",
  "TIMEZONE_LABEL_FMT": "GMT %+d",
  "CONFIRM_ADD_PROMPT": "Add Service:",
  "CONFIRM_DELETE_PROMPT": "Delete service:",
  "SERVICE_ADDED": "Service Added!",
  "SERVICE_DELETED": "Service Deleted",
  "TIME_ADJUSTED_FMT": "Time Adjusted:\n%02d:%02d:%02d",
  "TIMEZONE_SAVED_FMT": "Zone Saved:\nGMT %+d",
  "LANG_SAVED": "Language Saved!",
  "ERROR_RTC_FAILED": "RTC Error!",
  "ERROR_NO_SERVICES": "No services\nregistered!",
  "ERROR_JSON_PARSE_FMT": "JSON Error:\n%s",
  "ERROR_JSON_INVALID_SERVICE": "Invalid JSON:\nMissing 'name'/'secret'",
  "ERROR_JSON_INVALID_TIME": "Invalid JSON:\nMissing Y/M/D h:m:s",
  "ERROR_SERVICE_NAME_INVALID": "Invalid Service\nName!",
  "ERROR_SECRET_INVALID": "Invalid Secret!",
  "ERROR_SECRET_B32_INVALID": "Invalid Base32\nSecret!",
  "ERROR_TIME_VALUES_INVALID": "Invalid Date/Time\nValues!",
  "ERROR_SAVING_NVS": "NVS Save Error!",
  "ERROR_DELETING": "Error Deleting",
  "ERROR_NVS_LOAD": "[NVS] Load error",
  "ERROR_NVS_SAVE": "[NVS] Save error",
  "ERROR_B32_DECODE": "B32 Key Error!",
  "ERROR_MAX_SERVICES": "Max Services!",
  "ERROR_TOTP_GENERATION": "TOTP Gen Error",
  "ERROR_RFID_READ": "RFID Read Error",
  "STATUS_CONNECTING_RTC": "Connecting RTC...",
  "STATUS_LOADING_SERVICES": "Loading Data...",
  "STATUS_READY": "Ready!",
  "PLACEHOLDER_NO_CODE": "------",
  "PLACEHOLDER_NO_SERVICE_TITLE": "No Service",
  "PLACEHOLDER_UNKNOWN_KEY": "?KEY?",
  "NONE": ""
})";


// --- Array de ponteiros para os JSONs em PROGMEM ---
const char* const language_jsons[static_cast<int>(Language::NUM_LANGUAGES)] PROGMEM = {
    json_pt_br,
    json_en_us
    // Adicione ponteiros para outros JSONs aqui, na mesma ordem do enum Language
};

// --- Documento JSON para manter o idioma atual na RAM ---
// Ajuste o tamanho conforme necessário! Calcule usando ArduinoJson Assistant:
// https://arduinojson.org/v6/assistant/
// Considere o maior JSON de idioma + um pouco de folga.
// Se der erro de compilação ou falha no parse, aumente o tamanho.
// Usar DynamicJsonDocument é uma alternativa se a RAM permitir e a fragmentação
// não for um problema crítico, mas Static é geralmente preferível no ESP32.
constexpr size_t JSON_DOC_SIZE = 4096; // Ajuste este valor!
StaticJsonDocument<JSON_DOC_SIZE> langDoc;

// Placeholder para chave não encontrada
const char* unknownKeyPlaceholder = "?KEY?";

// ============================================================================
// === IMPLEMENTAÇÃO DAS FUNÇÕES PÚBLICAS ===
// ============================================================================

bool initI18N(Language lang) {
    if (static_cast<int>(lang) >= static_cast<int>(Language::NUM_LANGUAGES)) {
        Serial.printf("[i18n ERROR] Tentativa de carregar idioma inválido: %d\n", (int)lang);
        return false; // Índice de idioma fora dos limites
    }

    // Seleciona o ponteiro PROGMEM correto
    // Precisamos copiar o ponteiro de PROGMEM para a RAM antes de usar
    const char* json_progmem_ptr = (const char*)pgm_read_ptr(&language_jsons[static_cast<int>(lang)]);

    // Faz o parse do JSON da PROGMEM para o documento na RAM
    DeserializationError error = deserializeJson(langDoc, json_progmem_ptr);

    if (error) {
        Serial.printf("[i18n ERROR] Falha ao parsear JSON para idioma %d: %s\n", (int)lang, error.c_str());
        langDoc.clear(); // Garante que o documento esteja vazio em caso de falha
        // Poderia tentar carregar um idioma padrão aqui como fallback?
        return false;
    }

    // Se deu certo, atualiza o idioma global
    current_language = lang;
    Serial.printf("[i18n] Idioma %d carregado com sucesso.\n", (int)lang);
    return true;
}

const char* getText(StringID key) {
    // Verifica se a chave existe no documento JSON carregado
    // transforms key into string for lookup
    String keyString = String(key);
    if (langDoc.containsKey(keyString)) {
        // Retorna o ponteiro para a string dentro do JsonDocument
        // ATENÇÃO: O ponteiro é válido apenas enquanto langDoc não for modificado/limpo.
        return langDoc[key].as<const char*>();
    } else {
        // Chave não encontrada, retorna um placeholder
        Serial.printf("[i18n WARN] Chave não encontrada no JSON atual: '%s'\n", key);
        return unknownKeyPlaceholder; // Ou poderia retornar a própria 'key'
    }
}

bool setLanguage(Language lang) {
    if (lang == current_language && !langDoc.isNull()) {
         return true; // Já está carregado e válido
    }
    // Tenta carregar e parsear o novo idioma
    bool success = initI18N(lang);
    // Em caso de sucesso, 'current_language' já foi atualizado dentro de initI18N
    return success;
}

Language getCurrentLanguage() {
    return current_language;
}

// Retorna o nome do idioma baseado no índice (para menu de seleção)
// Lê diretamente do JSON em PROGMEM para evitar parse desnecessário
const char* getLanguageNameByIndex(Language index) {
     if (static_cast<int>(index) >= static_cast<int>(Language::COUNT)) {
        return unknownKeyPlaceholder; // Índice inválido
    }
    // Faz um parse rápido apenas para pegar o nome
    // Isso é menos eficiente, mas evita carregar o JSON inteiro só pelo nome
    // Alternativa: Ter um array `PROGMEM` separado só com os nomes
    const char* json_progmem_ptr = (const char*)pgm_read_ptr(&language_jsons[static_cast<int>(index)]);
    // Usar um JsonDocument temporário e pequeno
    StaticJsonDocument<128> nameDoc; // Pequeno, só para o nome
    DeserializationError error = deserializeJson(nameDoc, json_progmem_ptr, DeserializationOption::Filter(DeserializationOption::Filter::InterningFilter<1>()));
    // Filtro para carregar apenas a chave "LANGUAGE_NAME" (otimização)
    JsonObject root = nameDoc.as<JsonObject>();
    if (error || !root.containsKey("LANGUAGE_NAME")) {
        Serial.printf("[i18n WARN] Não foi possível obter nome para idioma índice %d\n", (int)index);
        return unknownKeyPlaceholder;
    }
    // Retorna o nome - ATENÇÃO: A string está na memória temporária 'nameDoc'!
    // Para usar isso com segurança, você talvez precise copiar a string retornada
    // para um buffer antes que nameDoc saia de escopo.
    // >>> SOLUÇÃO MAIS SEGURA: Criar um array PROGMEM só com os nomes <<<
    static const char* lang_names[] PROGMEM = { "Portugues (BR)", "English (US)" /* ... outros nomes */ };
    if (static_cast<int>(index) < (sizeof(lang_names)/sizeof(lang_names[0]))) {
        // Copia o nome de PROGMEM para um buffer estático (cuidado com multi-threading se aplicável)
        static char name_buffer[32]; // Buffer para copiar o nome
        strcpy_P(name_buffer, (char*)pgm_read_ptr(&(lang_names[static_cast<int>(index)])));
        return name_buffer;
    } else {
         return unknownKeyPlaceholder; // Índice inválido
    }

    // A solução com array PROGMEM separado é preferível à que parseia o JSON.
}