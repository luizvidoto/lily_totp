// service_storage.h
#ifndef SERVICE_STORAGE_H
#define SERVICE_STORAGE_H

#include <ArduinoJson.h> // Para JsonDocument

// ---- Funções de Gerenciamento de Serviços ----

// Carrega todos os serviços salvos do NVS para o array 'services' em memória.
// Atualiza 'service_count'.
// Retorna true se bem-sucedido, false em caso de erro ao acessar NVS.
bool service_storage_load_all();

// Salva a lista completa de serviços (do array 'services') no NVS.
// Remove chaves NVS de serviços que não existem mais.
// Atualiza 'svc_count' no NVS.
// Retorna true se bem-sucedido, false em caso de erro.
bool service_storage_save_all();

// Prepara um novo serviço para adição a partir de um JsonDocument.
// Valida os dados ('name', 'secret'), e se válidos, copia para
// 'temp_service_name' e 'temp_service_secret'.
// Em seguida, muda a tela para SCREEN_SERVICE_ADD_CONFIRM.
// Mostra mensagens de erro na UI se a validação falhar.
void service_storage_prepare_add_from_json(JsonDocument& doc);

// Confirma e salva o serviço que está em 'temp_service_name' e 'temp_service_secret'.
// Adiciona ao array 'services', incrementa 'service_count', e chama service_storage_save_all().
// Decodifica a chave do novo serviço e o define como o atual.
// Retorna true se bem-sucedido, false caso contrário (ex: MAX_SERVICES atingido, erro NVS).
// Mostra mensagens de erro/sucesso na UI.
bool service_storage_commit_add();

// Deleta o serviço no 'index' especificado.
// Remove do array 'services', decrementa 'service_count', e chama service_storage_save_all().
// Ajusta 'current_service_index' se necessário.
// Recarrega a chave do serviço que se tornou o atual (se houver).
// Retorna true se bem-sucedido, false caso contrário (ex: índice inválido, erro NVS).
// Mostra mensagens de erro/sucesso na UI.
bool service_storage_delete(int index_to_delete);

#endif // SERVICE_STORAGE_H