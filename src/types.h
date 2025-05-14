#ifndef TYPES_H
#define TYPES_H

#include <stddef.h> // Para size_t
#include <stdint.h> // Para uintX_t
#include "config.h" // Para constantes de tamanho

// ---- Estruturas de Dados ----
struct TOTPService {
  char name[MAX_SERVICE_NAME_LEN + 1];
  char secret_b32[MAX_SECRET_B32_LEN + 1];
};

struct CurrentTOTPInfo {
  char code[7]; // "XXXXXX\0"
  uint32_t last_generated_interval;
  uint8_t key_bin[MAX_SECRET_BIN_LEN];
  size_t key_bin_len;
  bool valid_key;
};

struct BatteryInfo {
  float voltage;
  bool is_usb_powered;
  int level_percent; // 0-100
};

// ---- Estados da Aplicação ----
enum class ScreenState { // Usando enum class para melhor type safety
  SCREEN_TOTP_VIEW,
  SCREEN_MENU_MAIN,
  SCREEN_SERVICE_ADD_WAIT,
  SCREEN_SERVICE_ADD_CONFIRM,
  SCREEN_TIME_EDIT,
  SCREEN_SERVICE_DELETE_CONFIRM,
  SCREEN_TIMEZONE_EDIT,
  SCREEN_LANGUAGE_SELECT,
  SCREEN_MESSAGE,
  SCREEN_READ_RFID, // Existing screen, might be repurposed or used as part of card management
  SCREEN_STARTUP,
  SCREEN_LOCKED,                // Nova tela de bloqueio
  SCREEN_MANAGE_CARDS,          // Nova tela para gerenciar cartões RFID
  SCREEN_ADD_CARD_WAIT,         // Nova tela para aguardar cartão ao adicionar
  SCREEN_ADD_CARD_CONFIRM,      // Nova tela para confirmar adição de cartão
  SCREEN_DELETE_CARD_CONFIRM,   // Nova tela para confirmar remoção de cartão
  SCREEN_CONFIG_MENU,           // Nova tela para o submenu de Configurações
  SCREEN_ADJUST_LOCK_TIMEOUT,   // Nova tela para ajustar o timeout de bloqueio
  SCREEN_BRIGHTNESS_MENU,       // Nova tela para selecionar qual brilho ajustar
  SCREEN_ADJUST_BRIGHTNESS_VALUE, // Nova tela para ajustar um valor de brilho específico
  SCREEN_SETUP_FIRST_CARD       // Nova tela para configurar o primeiro cartão RFID
};

// ---- Internacionalização (i18n) ----
enum class Language { // Usando enum class
  LANG_PT_BR,
  LANG_EN_US,
  NUM_LANGUAGES // Deve ser o último
};

// IDs para todas as strings traduzíveis
enum class StringID {
    STR_TOTP_CODE_TITLE,            // "Codigo TOTP"
    STR_TITLE_MAIN_MENU,            // "Menu Principal"
    STR_TITLE_ADD_SERVICE,          // "Adicionar Servico"
    STR_TITLE_READ_RFID,            // "Ler Cartao RFID"
    STR_TITLE_CONFIRM_ADD,          // "Confirmar Adicao"
    STR_TITLE_ADJUST_TIME,          // "Ajustar Hora"
    STR_TITLE_CONFIRM_DELETE,       // "Confirmar Exclusao"
    STR_TITLE_ADJUST_TIMEZONE,      // "Ajustar Fuso"
    STR_TITLE_SELECT_LANGUAGE,      // "Selecionar Idioma"

    // Menu items (actions, can also be used as labels if appropriate)
    STR_MENU_ADD_SERVICE,           // "Adicionar Servico" (Action text)
    STR_MENU_READ_RFID_OPTION,      // "Ler Cartao RFID" (Action text)
    STR_MENU_VIEW_CODES,            // "Ver Codigos TOTP" (Action text)
    STR_MENU_ADJUST_TIME,           // "Ajustar Hora/Data" (Action text)
    STR_MENU_ADJUST_TIMEZONE,       // "Ajustar Fuso Horario" (Action text)
    STR_MENU_SELECT_LANGUAGE,       // "Idioma" (Action text)

    // New labels for the redesigned main menu items (as added before)
    STR_LABEL_CODES,
    STR_LABEL_CONFIG,
    STR_LABEL_ZONE,
    STR_LABEL_RFID,
    STR_LABEL_LANG,
    STR_LABEL_DATE,

    // New action ID for Config (as added before)
    STR_ACTION_CONFIG,

    // Language names
    STR_LANG_PORTUGUESE,            // "Portugues (BR)"
    STR_LANG_ENGLISH,               // "Ingles (US)"

    // UI Messages & Prompts
    STR_AWAITING_JSON,              // "Aguardando JSON"
    STR_VIA_SERIAL,                 // "via Serial..."
    STR_EXAMPLE_JSON_SERVICE,       // "Ex: {\"name\":\"Git\",\"secret\":\"ABC...\"}"
    STR_EXAMPLE_JSON_TIME,          // "Ex JSON Hora: {'y':ANO,'mo':MES,'d':DIA,'h':HH,'m':MM,'s':SS}"
    STR_SERVICES_STATUS_FMT,        // "Servicos: %d/%d"

    // Footer navigation texts
    STR_FOOTER_GENERIC_NAV,         // "Ant/Prox | LP: Sel | DblClick: Menu"
    STR_FOOTER_CONFIRM_NAV,         // "Ant: Canc | Prox: Conf"
    STR_FOOTER_TIME_EDIT_NAV,       // "Ant/Prox: +/- | LP: Campo/Salvar | Dbl: Menu"
    STR_FOOTER_TIMEZONE_NAV,        // "Ant/Prox: +/- | LP: Salvar | Dbl: Menu"
    STR_FOOTER_LANG_NAV,            // "Ant/Prox | LP: Salvar | Dbl: Menu"

    // Other UI texts
    STR_TIME_EDIT_INFO_FMT,         // "(Fuso atual: GMT %+d)"
    STR_TIME_EDIT_JSON_HINT,        // "Ou envie JSON via Serial (UTC):"
    STR_TIMEZONE_LABEL,             // "GMT %+d"
    STR_CONFIRM_ADD_PROMPT,         // "Adicionar Servico:"
    STR_CONFIRM_DELETE_PROMPT_FMT,  // "Deletar '%s'?"

    // Success/Error/Info Messages
    STR_SERVICE_ADDED,              // "Servico Adicionado!"
    STR_SERVICE_DELETED,            // "Servico Deletado!"
    STR_TIME_ADJUSTED_FMT,          // "Hora Ajustada:\n%02d:%02d:%02d (Local)"
    STR_TIMEZONE_SAVED_FMT,         // "Fuso Salvo:\nGMT %+d"
    STR_LANG_SAVED,                 // "Idioma Salvo!"
    STR_ERROR_RTC,                  // "Erro RTC! Verifique conexao/bateria."
    STR_ERROR_RTC_INIT,             // "Erro RTC Init!"
    STR_ERROR_NO_SERVICES,          // "Nenhum servico\ncadastrado!"
    STR_ERROR_JSON_PARSE_FMT,       // "Erro JSON:\n%s"
    STR_ERROR_JSON_INVALID_SERVICE, // "JSON Invalido:\nFalta 'name' ou 'secret'"
    STR_ERROR_JSON_INVALID_TIME,    // "JSON Invalido:\nFaltam campos Y/M/D h:m:s"
    STR_ERROR_INVALID_SVC_NAME,     // "Nome Servico\nInvalido!"
    STR_ERROR_INVALID_SECRET,       // "Segredo Invalido!"
    STR_ERROR_INVALID_B32_SECRET,   // "Segredo Base32\ncontem chars invalidos!"
    STR_ERROR_INVALID_DATETIME,     // "Valores Data/Hora\nInvalidos!"
    STR_ERROR_SAVING_SERVICE,       // "Erro ao Salvar\nServico!"
    STR_ERROR_DELETING_SERVICE,     // "Erro ao Deletar\nServico!"
    STR_ERROR_NVS_LOAD_CONFIG,      // "[NVS] Erro ao carregar config!"
    STR_ERROR_NVS_SAVE_CONFIG,      // "[NVS] Erro ao salvar config!"
    STR_ERROR_B32_DECODE,           // "Falha ao decodificar\nchave Base32!"
    STR_ERROR_MAX_SERVICES_FMT,     // "Maximo de servicos\natingido (%d)!"
    STR_TOTP_NO_SERVICE_TITLE,      // "------" (Placeholder when no service for TOTP view title)
    STR_TOTP_CODE_PLACEHOLDER,      // "Codigo TOTP" (Placeholder for code area if error)
    STR_RFID_PROMPT,                // "Aproxime o cartao\nRFID do leitor..."
    STR_RFID_ID_PREFIX,             // "ID Cartao: %s"
    STR_STARTUP_MSG,                // "Iniciando..."
    STR_NOT_IMPLEMENTED_YET,

    // Novas Strings para Tela de Bloqueio e Gerenciamento de RFID
    STR_TITLE_LOCKED,
    STR_TITLE_MANAGE_CARDS,
    STR_TITLE_ADD_CARD,
    STR_TITLE_CONFIRM_ADD_CARD,
    STR_TITLE_CONFIRM_DELETE_CARD,
    STR_LOCKED_PROMPT,
    STR_MENU_MANAGE_CARDS,
    STR_CARD_LIST_EMPTY,
    STR_ADD_CARD_PROMPT,
    STR_SCAN_CARD_PROMPT,
    STR_CONFIRM_ADD_CARD_PROMPT_FMT,    // "Adicionar cartao %s?"
    STR_CONFIRM_DELETE_CARD_PROMPT_FMT, // "Remover cartao %s?"
    STR_CARD_ADDED,
    STR_CARD_DELETED,
    STR_ERROR_MAX_CARDS,
    STR_ERROR_CARD_ALREADY_EXISTS,
    STR_ERROR_SAVING_CARD,
    STR_ERROR_DELETING_CARD,
    STR_FOOTER_MANAGE_CARDS_NAV,      // "Ant/Prox | LP: Sel | Dbl: Menu"
    STR_FOOTER_ADD_CARD_NAV,        // "LP: Canc | Dbl: Menu"
    STR_FOOTER_CONFIRM_CARD_NAV,    // "Ant: Canc | Prox: Conf | Dbl:Menu"

    // Novas Strings para o Submenu de Configurações
    STR_TITLE_CONFIG_MENU,
    STR_MENU_CONFIG_MANAGE_CARDS,   // Opção no submenu de config para gerenciar cartões
    STR_MENU_CONFIG_LOCK_TIMEOUT,   // Opção para ajustar timeout de bloqueio (placeholder)
    STR_MENU_CONFIG_BRIGHTNESS,     // Opção para ajustar brilho (placeholder)
    STR_FOOTER_CONFIG_SUBMENU_NAV,  // Footer para o submenu de config

    // Nova String para erro de cartão não autorizado na tela de bloqueio
    STR_ERROR_CARD_UNAUTHORIZED,

    // Novas Strings para Ajuste de Timeout de Bloqueio
    STR_TITLE_ADJUST_LOCK_TIMEOUT,
    STR_LOCK_TIMEOUT_LABEL_FMT,     // "Bloquear apos: %d min"
    STR_LOCK_TIMEOUT_SAVED_FMT,     // "Timeout salvo: %d min"
    STR_FOOTER_ADJUST_VALUE_NAV,    // "Ant/Prox: +/- | LP: Salvar | Dbl: Menu"

    // Novas Strings para Ajuste de Brilho
    STR_TITLE_BRIGHTNESS_MENU,
    STR_MENU_BRIGHTNESS_USB,
    STR_MENU_BRIGHTNESS_BATTERY,
    STR_MENU_BRIGHTNESS_DIMMED,
    STR_TITLE_ADJUST_BRIGHTNESS_VALUE_FMT, // "Brilho (%s): %d"
    STR_BRIGHTNESS_SAVED_FMT,            // "Brilho (%s) salvo: %d"

    // Novas Strings para Configuração do Primeiro Cartão
    STR_TITLE_SETUP_FIRST_CARD,
    STR_PROMPT_SETUP_FIRST_CARD,
    STR_FIRST_CARD_SAVED,
    STR_ERROR_SAVING_FIRST_CARD,

    // Nova String para quando o último cartão é deletado
    STR_LAST_CARD_DELETED_SETUP_NEW,

    NUM_STRINGS // Deve ser o último para contagem
};

// Enum para saber qual brilho está sendo editado (Moved from globals.h)
enum class BrightnessSettingToAdjust {
    USB,
    BATTERY,
    DIMMED
};

struct LanguageStrings {
  const char *strings[static_cast<int>(StringID::NUM_STRINGS)];
};

#endif // TYPES_H