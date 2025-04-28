#pragma once // Include guard

#include "types.h" // Para enum Language (StringID não é mais necessário aqui)
#include <ArduinoJson.h> // Incluir para declarar JsonDocument se necessário

// ============================================================================
// === FUNÇÕES PÚBLICAS DO MÓDULO DE INTERNACIONALIZAÇÃO (JSON Incorporado) ===
// ============================================================================

/**
 * @brief Inicializa o sistema i18n carregando e parseando o JSON
 *        do idioma especificado da PROGMEM para a RAM.
 *        Deve ser chamado no setup() após loadSettings() ter definido o idioma inicial
 *        e sempre que o idioma for alterado.
 * @param lang O idioma a ser carregado.
 * @return true se o JSON foi parseado com sucesso, false caso contrário.
 */
bool initI18N(Language lang);

/**
 * @brief Obtém o texto traduzido para a chave fornecida, consultando o JSON
 *        atualmente carregado na RAM.
 * @param key A chave da string desejada (ex: "TITLE_MAIN_MENU").
 * @return Ponteiro constante para a string traduzida no JSON em RAM.
 *         Retorna a própria chave (ou um placeholder como "?KEY?") se a chave não for encontrada.
 */
const char *getText(StringID key);
// const char* getText(const char* key);

/**
 * @brief Define o idioma atual da aplicação, carregando o JSON correspondente.
 *        Chama initI18N() internamente.
 * @param lang O novo idioma a ser definido.
 * @return true se o novo idioma foi carregado com sucesso, false caso contrário.
 */
bool setLanguage(Language lang);

/**
 * @brief Obtém o idioma atualmente ativo.
 * @return O enum Language do idioma atual.
 */
Language getCurrentLanguage();



/**
 * @brief Obtém o nome do idioma na lista (para menu de seleção)
 * @param index O índice do idioma na enum Language
 * @return Ponteiro constante para o nome do idioma (ex: "Português (BR)")
 */
const char* getLanguageNameByIndex(Language index); // Nova função para o menu

// (Opcional) Se você quiser manter a verificação em tempo de compilação para
// as chaves, pode definir macros aqui, mas torna o código mais verboso:
// #define KEY_TITLE_TOTP_CODE "TITLE_TOTP_CODE"
// #define KEY_TITLE_MAIN_MENU "TITLE_MAIN_MENU"
// // ... etc
// E então chamar getText(KEY_TITLE_MAIN_MENU)