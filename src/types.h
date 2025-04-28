#pragma once // Include guard

#include "config.h" // Necessário para constantes como MAX_SERVICE_NAME_LEN

// ============================================================================
// === ENUMERAÇÕES ===
// ============================================================================

// --- Estados da Aplicação (Telas) ---
// enum class ScreenState : uint8_t {
//     BOOTING,                // Tela inicial de carregamento/status
//     TOTP_VIEW,              // Exibição do código TOTP atual
//     MENU_MAIN,              // Menu principal de opções
//     SERVICE_ADD_WAIT,       // Aguardando JSON via Serial para novo serviço
//     SERVICE_ADD_CONFIRM,    // Confirmação antes de salvar novo serviço
//     TIME_EDIT,              // Edição de Hora/Minuto/Segundo
//     SERVICE_DELETE_CONFIRM, // Confirmação antes de deletar um serviço
//     TIMEZONE_EDIT,          // Edição do fuso horário (GMT Offset)
//     LANGUAGE_SELECT,        // Seleção do idioma da interface
//     READ_RFID,              // Tela para leitura de cartão RFID
//     MESSAGE                 // Tela genérica para exibir mensagens temporárias
// };

// ---- Estados da Aplicação ----
enum ScreenState {
  SCREEN_TOTP_VIEW,
  SCREEN_MENU_MAIN,
  SCREEN_SERVICE_ADD_WAIT,
  SCREEN_SERVICE_ADD_CONFIRM,
  SCREEN_TIME_EDIT,
  SCREEN_SERVICE_DELETE_CONFIRM,
  SCREEN_TIMEZONE_EDIT,
  SCREEN_LANGUAGE_SELECT,
  SCREEN_MESSAGE,
  SCREEN_READ_RFID
};

// --- Idiomas Suportados ---
enum class Language : uint8_t {
    PT_BR,      // Português (Brasil)
    EN_US,      // English (United States)
    // Adicione mais idiomas aqui e atualize i18n.cpp e StringID
};

// --- Identificadores Únicos para Textos Traduzíveis ---
// NOTA: Mantenha sincronizado com as definições em i18n.cpp!
// enum class StringID : uint8_t {
//     // Títulos de Tela
//     TITLE_TOTP_CODE, TITLE_MAIN_MENU, TITLE_ADD_SERVICE, TITLE_READ_RFID,
//     TITLE_CONFIRM_ADD, TITLE_ADJUST_TIME, TITLE_CONFIRM_DELETE, TITLE_ADJUST_TIMEZONE,
//     TITLE_SELECT_LANGUAGE,

//     // Itens do Menu Principal
//     MENU_ADD_SERVICE, MENU_READ_RFID, MENU_VIEW_CODES, MENU_ADJUST_TIME,
//     MENU_ADJUST_TIMEZONE, MENU_SELECT_LANGUAGE,

//     // Nomes de Idioma (para tela de seleção)
//     LANG_PORTUGUESE, LANG_ENGLISH,

//     // Textos Diversos UI
//     AWAITING_JSON, VIA_SERIAL, EXAMPLE_JSON_SERVICE, EXAMPLE_JSON_TIME,
//     SERVICES_STATUS_FMT, // Ex: "Serviços: 3/50"
//     RFID_PROMPT,         // Ex: "Aproxime o cartão"
//     RFID_READING,        // Ex: "Lendo..."
//     CARD_READ_FMT,       // Ex: "ID: 1A2B3C4D"

//     // Textos de Rodapé (Instruções de Navegação)
//     FOOTER_GENERIC_NAV, FOOTER_CONFIRM_NAV, FOOTER_TIME_EDIT_NAV,
//     FOOTER_TIMEZONE_NAV, FOOTER_LANG_NAV,

//     // Textos Específicos de Telas
//     TIME_EDIT_INFO_FMT, // Ex: "(Fuso atual: GMT %+d)"
//     TIME_EDIT_JSON_HINT,// Ex: "Ou envie JSON via Serial (UTC):"
//     TIMEZONE_LABEL_FMT, // Ex: "GMT %+d"
//     CONFIRM_ADD_PROMPT, // Ex: "Adicionar Serviço:"
//     CONFIRM_DELETE_PROMPT, // Ex: "Deletar serviço:"

//     // Mensagens de Status/Sucesso
//     SERVICE_ADDED, SERVICE_DELETED, TIME_ADJUSTED_FMT, TIMEZONE_SAVED_FMT,
//     LANG_SAVED,

//     // Mensagens de Erro
//     ERROR_RTC_FAILED, ERROR_NO_SERVICES, ERROR_JSON_PARSE_FMT, ERROR_JSON_INVALID_SERVICE,
//     ERROR_JSON_INVALID_TIME, ERROR_SERVICE_NAME_INVALID, ERROR_SECRET_INVALID,
//     ERROR_SECRET_B32_INVALID, ERROR_TIME_VALUES_INVALID, ERROR_SAVING_NVS, ERROR_DELETING,
//     ERROR_NVS_LOAD, ERROR_NVS_SAVE, ERROR_B32_DECODE, ERROR_MAX_SERVICES,
//     ERROR_TOTP_GENERATION, ERROR_RFID_READ,

//     // Mensagens de Status da Tela de Boot
//     STATUS_CONNECTING_RTC, STATUS_LOADING_SERVICES, STATUS_READY,

//     // Placeholders
//     PLACEHOLDER_NO_CODE,         // Ex: "------"
//     PLACEHOLDER_NO_SERVICE_TITLE,// Ex: "Sem Serviço"

//     // Marcador Final para Contagem
//     COUNT // Deve ser o último para saber o número total de strings
// };

enum StringID {
  STR_TITLE_TOTP_CODE,
  STR_TITLE_MAIN_MENU,
  STR_TITLE_ADD_SERVICE,
  STR_TITLE_CONFIRM_ADD,
  STR_TITLE_ADJUST_TIME,
  STR_TITLE_CONFIRM_DELETE,
  STR_TITLE_ADJUST_TIMEZONE,
  STR_TITLE_SELECT_LANGUAGE,
  STR_TITLE_READ_RFID,
  STR_MENU_ADD_SERVICE,
  STR_MENU_READ_RFID,
  STR_MENU_VIEW_CODES,
  STR_MENU_ADJUST_TIME,
  STR_MENU_ADJUST_TIMEZONE,
  STR_MENU_SELECT_LANGUAGE,
  STR_LANG_PORTUGUESE,
  STR_LANG_ENGLISH,
  STR_AWAITING_JSON,
  STR_VIA_SERIAL,
  STR_EXAMPLE_JSON_SERVICE,
  STR_EXAMPLE_JSON_TIME,
  STR_SERVICES_STATUS_FMT,
  STR_FOOTER_GENERIC_NAV,
  STR_FOOTER_CONFIRM_NAV,
  STR_FOOTER_TIME_EDIT_NAV,
  STR_FOOTER_TIMEZONE_NAV,
  STR_FOOTER_LANG_NAV,
  STR_TIME_EDIT_INFO_FMT,
  STR_TIME_EDIT_JSON_HINT,
  STR_TIMEZONE_LABEL,
  STR_CONFIRM_ADD_PROMPT,
  STR_CONFIRM_DELETE_PROMPT,
  STR_SERVICE_ADDED,
  STR_SERVICE_DELETED,
  STR_TIME_ADJUSTED_FMT,
  STR_TIMEZONE_SAVED_FMT,
  STR_LANG_SAVED,
  STR_ERROR_RTC,
  STR_ERROR_NO_SERVICES,
  STR_ERROR_JSON_PARSE_FMT,
  STR_ERROR_JSON_INVALID_SERVICE,
  STR_ERROR_JSON_INVALID_TIME,
  STR_ERROR_SERVICE_NAME_INVALID,
  STR_ERROR_SECRET_INVALID,
  STR_ERROR_TIME_VALUES_INVALID,
  STR_ERROR_SAVING,
  STR_ERROR_DELETING,
  STR_ERROR_NVS_LOAD,
  STR_ERROR_NVS_SAVE,
  STR_ERROR_B32_DECODE,
  STR_ERROR_MAX_SERVICES,
  STR_TOTP_CODE_ERROR,
  STR_TOTP_NO_SERVICE_TITLE,
  STR_CARD_READ_FMT,
  STR_RFID_PROMPT,
  NONE,
  NUM_STRINGS // Deve ser o último
};

// ============================================================================
// === ESTRUTURAS DE DADOS ===
// ============================================================================

// --- Representação de um Serviço TOTP Armazenado ---
struct TOTPService {
  char name[MAX_SERVICE_NAME_LEN + 1];      // Nome do serviço (visível ao usuário)
  char secret_b32[MAX_SECRET_B32_LEN + 1];  // Segredo em formato Base32
  // A chave binária não é armazenada aqui para economizar RAM; é decodificada sob demanda.
};

// --- Informações sobre o Código TOTP sendo Exibido ---
struct CurrentTOTPInfo {
  char code[7];                       // Código formatado ("123456" ou placeholder)
  uint32_t last_generated_interval;   // Otimização: último intervalo de tempo para o qual o código foi gerado
  uint8_t key_bin[MAX_SECRET_BIN_LEN];
  size_t key_bin_len;
  bool valid_key_loaded;              // Indica se a chave do serviço atual foi decodificada com sucesso
};

// --- Informações da Bateria e Alimentação ---
struct BatteryInfo {
  float voltage;        // Tensão lida (após conversão do ADC)
  bool is_usb_powered;  // True se a tensão indicar alimentação USB
  int level_percent;    // Nível estimado da bateria (0-100)
};

// --- Dados Temporários para Telas de Edição/Adição ---
struct TempData {
    // Para Adição de Serviço
    char service_name[MAX_SERVICE_NAME_LEN + 1];
    char service_secret[MAX_SECRET_B32_LEN + 1];
    // Para Edição de Hora
    int edit_time_field; // 0=hora, 1=minuto, 2=segundo
    int edit_hour;
    int edit_minute;
    int edit_second;
    // Para Edição de Fuso Horário
    int edit_gmt_offset;
    // Para Seleção de Idioma
    Language edit_language_index; // Índice do idioma sendo destacado no menu
    // Para Leitura RFID
    char rfid_card_id[16]; // Armazena o último ID lido como string HEX
};

// --- Estado de um Menu com Rolagem e Animação ---
struct MenuState {
    int current_index;              // Índice do item atualmente selecionado
    int top_visible_index;          // Índice do primeiro item visível na tela (para scroll)
    int highlight_y_current;        // Posição Y atual (pixels) da barra de destaque (para animação)
    int highlight_y_target;         // Posição Y alvo (pixels) da barra de destaque
    unsigned long animation_start_time; // Tempo (millis) em que a animação começou
    bool is_animating;              // Flag indicando se o menu está atualmente animando o scroll
};