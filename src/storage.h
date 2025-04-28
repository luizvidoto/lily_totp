#pragma once // Include guard

#include <stddef.h> // Para size_t
#include "types.h"  // Para TOTPService, Language

// ============================================================================
// === FUNÇÕES PÚBLICAS DO MÓDULO DE ARMAZENAMENTO (NVS) ===
// ============================================================================

/**
 * @brief Carrega as configurações gerais (idioma, fuso horário) do NVS.
 *        Atualiza as variáveis globais correspondentes (current_language, gmt_offset_hours).
 * @return true se o carregamento foi bem-sucedido (namespace aberto), false caso contrário.
 */
bool loadSettings();

/**
 * @brief Salva as configurações gerais atuais (idioma, fuso horário) no NVS.
 * @return true se a gravação foi bem-sucedida, false caso contrário.
 */
bool saveSettings();

/**
 * @brief Carrega a lista de serviços TOTP do NVS para o array global 'services'.
 *        Atualiza a variável global 'service_count'.
 *        Realiza compactação se encontrar entradas inválidas/removidas.
 */
void loadServices();

/**
 * @brief Salva TODO o array global 'services' atual no NVS.
 *        Sobrescreve ou remove entradas conforme necessário com base em 'service_count'.
 * @return true se a operação foi bem-sucedida, false em caso de erro no NVS.
 */
bool saveServiceList();

/**
 * @brief Adiciona um novo serviço ao final do array 'services' e salva a lista inteira no NVS.
 *        Verifica se o limite MAX_SERVICES foi atingido.
 * @param name Nome do novo serviço.
 * @param secret_b32 Segredo Base32 do novo serviço.
 * @return true se o serviço foi adicionado e salvo com sucesso, false caso contrário (erro NVS ou limite atingido).
 */
bool storage_saveService(const char *name, const char *secret_b32);

/**
 * @brief Remove o serviço no índice especificado do array 'services',
 *        desloca os itens subsequentes e salva a lista atualizada no NVS.
 * @param index O índice do serviço a ser deletado.
 * @return true se o serviço foi deletado e a lista salva com sucesso, false caso contrário (índice inválido ou erro NVS).
 */
bool storage_deleteService(int index);