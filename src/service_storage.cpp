// service_storage.cpp
#include "service_storage.h"
#include "globals.h"    // Acesso a preferences, services, service_count, temp_vars, etc.
#include "config.h"     // Acesso a NVS_*, MAX_SERVICES, MAX_NAME_LEN, MAX_SECRET_LEN
#include "ui_manager.h" // Para ui_change_screen, ui_queue_message*
#include "i18n.h"       // Para getText, StringID
#include "totp_core.h"  // Para totp_decode_current_service_key, totp_validate_base32_string
#include <Arduino.h>    // Para Serial, snprintf, strncpy

// ---- Carregamento e Salvamento ----

bool service_storage_load_all() {
    if (!preferences.begin(NVS_NAMESPACE, true)) { // true = read-only
        Serial.printf("[Storage] Erro ao abrir NVS namespace '%s' para leitura.\n", NVS_NAMESPACE);
        ui_queue_message(getText(StringID::STR_ERROR_NVS_LOAD), COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN); // Ou uma tela de erro fatal
        service_count = 0;
        return false;
    }

    service_count = preferences.getInt(NVS_SVC_COUNT_KEY, 0);
    if (service_count < 0 || service_count > MAX_SERVICES) {
        Serial.printf("[Storage] Contador de serviços NVS inválido (%d). Resetando para 0.\n", service_count);
        service_count = 0;
        // Considerar corrigir o contador no NVS aqui se ele estiver corrompido.
        // preferences.remove(NVS_SVC_COUNT_KEY); // ou putInt(0) se abrir para escrita
    }

    int valid_services_loaded = 0;
    for (int i = 0; i < service_count; ++i) {
        char name_key[20]; // "svc_XX_n"
        char secret_key[20]; // "svc_XX_s"
        snprintf(name_key, sizeof(name_key), "%s%d%s", NVS_SVC_NAME_PREFIX, i, NVS_SVC_NAME_SUFFIX);
        snprintf(secret_key, sizeof(secret_key), "%s%d%s", NVS_SVC_NAME_PREFIX, i, NVS_SVC_SECRET_SUFFIX);

        String name_str = preferences.getString(name_key, "");
        String secret_str = preferences.getString(secret_key, "");

        if (name_str.length() > 0 && name_str.length() <= MAX_SERVICE_NAME_LEN &&
            secret_str.length() > 0 && secret_str.length() <= MAX_SECRET_B32_LEN)
        {
            // Copia para a posição correta no array 'services', compactando a lista se houver buracos
            strncpy(services[valid_services_loaded].name, name_str.c_str(), MAX_SERVICE_NAME_LEN);
            services[valid_services_loaded].name[MAX_SERVICE_NAME_LEN] = '\0'; // Garante terminação nula

            strncpy(services[valid_services_loaded].secret_b32, secret_str.c_str(), MAX_SECRET_B32_LEN);
            services[valid_services_loaded].secret_b32[MAX_SECRET_B32_LEN] = '\0';

            valid_services_loaded++;
        } else {
            Serial.printf("[Storage] Serviço NVS no índice %d inválido ou ausente. Pulando.\n", i);
            // Não incrementa valid_services_loaded, efetivamente removendo este "buraco"
        }
    }
    preferences.end();

    if (valid_services_loaded != service_count) {
        Serial.printf("[Storage] Carregados %d serviços válidos de %d esperados. Contador ajustado.\n", valid_services_loaded, service_count);
        service_count = valid_services_loaded;
        // Opcional: Se o contador foi ajustado, salvar a lista compactada de volta no NVS
        // para limpar entradas órfãs. Isso requer abrir NVS para escrita.
        // service_storage_save_all(); // Cuidado com chamadas recursivas ou loops
    }

    Serial.printf("[Storage] %d serviços carregados do NVS.\n", service_count);
    return true;
}

bool service_storage_save_all() {
    if (!preferences.begin(NVS_NAMESPACE, false)) { // false = read-write
        Serial.printf("[Storage] Erro ao abrir NVS namespace '%s' para escrita.\n", NVS_NAMESPACE);
        ui_queue_message(getText(StringID::STR_ERROR_NVS_SAVE), COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN);
        return false;
    }

    // Lê o contador antigo para saber quantos slots limpar, se a lista diminuiu
    int old_count = preferences.getInt(NVS_SVC_COUNT_KEY, 0);

    // Salva o novo contador de serviços
    if (!preferences.putInt(NVS_SVC_COUNT_KEY, service_count)) {
        Serial.println("[Storage] Erro ao salvar novo contador de serviços no NVS.");
        preferences.end();
        ui_queue_message(getText(StringID::STR_ERROR_NVS_SAVE), COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN);
        return false;
    }

    bool success = true;
    // Salva cada serviço atual no NVS
    for (int i = 0; i < service_count; ++i) {
        char name_key[20];
        char secret_key[20];
        snprintf(name_key, sizeof(name_key), "%s%d%s", NVS_SVC_NAME_PREFIX, i, NVS_SVC_NAME_SUFFIX);
        snprintf(secret_key, sizeof(secret_key), "%s%d%s", NVS_SVC_NAME_PREFIX, i, NVS_SVC_SECRET_SUFFIX);

        if (!preferences.putString(name_key, services[i].name)) success = false;
        if (!preferences.putString(secret_key, services[i].secret_b32)) success = false;

        if (!success) {
            Serial.printf("[Storage] Erro ao salvar serviço %d ('%s') no NVS.\n", i, services[i].name);
            break; // Para a operação se um erro ocorrer
        }
    }

    // Se a lista de serviços diminuiu, remove as chaves NVS dos serviços antigos
    if (success && service_count < old_count) {
        for (int i = service_count; i < old_count; ++i) {
            char name_key[20];
            char secret_key[20];
            snprintf(name_key, sizeof(name_key), "%s%d%s", NVS_SVC_NAME_PREFIX, i, NVS_SVC_NAME_SUFFIX);
            snprintf(secret_key, sizeof(secret_key), "%s%d%s", NVS_SVC_NAME_PREFIX, i, NVS_SVC_SECRET_SUFFIX);
            preferences.remove(name_key);
            preferences.remove(secret_key);
            // Serial.printf("[Storage] Removidas chaves NVS para o slot de serviço antigo %d.\n", i);
        }
    }

    preferences.end();

    if (!success) {
        ui_queue_message(getText(StringID::STR_ERROR_NVS_SAVE), COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN);
    } else {
        Serial.printf("[Storage] %d serviços salvos no NVS.\n", service_count);
    }
    return success;
}


// ---- Adição de Serviço ----

void service_storage_prepare_add_from_json(JsonDocument& doc) {
    // Validação dos campos e tipos
    if (!doc.containsKey("name") || !doc["name"].is<const char*>() ||
        !doc.containsKey("secret") || !doc["secret"].is<const char*>()) {
        ui_queue_message(getText(StringID::STR_ERROR_JSON_INVALID_SERVICE), COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN);
        return;
    }

    const char* name_json = doc["name"];
    const char* secret_json = doc["secret"];

    // Validação de comprimento
    if (strlen(name_json) == 0 || strlen(name_json) > MAX_SERVICE_NAME_LEN) {
        ui_queue_message(getText(StringID::STR_ERROR_SERVICE_NAME_INVALID), COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN);
        return;
    }
    if (strlen(secret_json) == 0 || strlen(secret_json) > MAX_SECRET_B32_LEN) {
        ui_queue_message(getText(StringID::STR_ERROR_SECRET_INVALID), COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN);
        return;
    }

    // Validação de caracteres do segredo Base32
    if (!totp_validate_base32_string(secret_json)) {
        ui_queue_message(getText(StringID::STR_ERROR_SECRET_B32_CHARS), COLOR_ERROR, 3500, ScreenState::SCREEN_MENU_MAIN);
        return;
    }

    // Copia dados válidos para variáveis temporárias
    strncpy(temp_service_name, name_json, MAX_SERVICE_NAME_LEN);
    temp_service_name[MAX_SERVICE_NAME_LEN] = '\0';
    strncpy(temp_service_secret, secret_json, MAX_SECRET_B32_LEN);
    temp_service_secret[MAX_SECRET_B32_LEN] = '\0';

    // Muda para a tela de confirmação
    ui_change_screen(ScreenState::SCREEN_SERVICE_ADD_CONFIRM);
}

bool service_storage_commit_add() {
    if (service_count >= MAX_SERVICES) {
        ui_queue_message_fmt(StringID::STR_ERROR_MAX_SERVICES, COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN, MAX_SERVICES);
        return false;
    }

    // Adiciona ao array em memória (usando os dados de temp_service_name e temp_service_secret)
    strncpy(services[service_count].name, temp_service_name, MAX_SERVICE_NAME_LEN);
    services[service_count].name[MAX_SERVICE_NAME_LEN] = '\0';
    strncpy(services[service_count].secret_b32, temp_service_secret, MAX_SECRET_B32_LEN);
    services[service_count].secret_b32[MAX_SECRET_B32_LEN] = '\0';

    service_count++;

    // Salva a lista inteira atualizada no NVS
    if (!service_storage_save_all()) {
        // Erro ao salvar no NVS. Desfaz a adição em memória.
        service_count--;
        // A mensagem de erro já foi mostrada por save_all()
        return false;
    }

    // Sucesso. Define o novo serviço como o atual e decodifica sua chave.
    current_service_index = service_count - 1;
    if (!totp_decode_current_service_key()) {
        // Isso seria um erro inesperado se a validação do Base32 foi feita antes.
        // A mensagem de erro de decodificação já foi mostrada por totp_decode.
        // O usuário verá "------" para o código.
    }

    // Limpa variáveis temporárias
    memset(temp_service_name, 0, sizeof(temp_service_name));
    memset(temp_service_secret, 0, sizeof(temp_service_secret));

    return true;
}


// ---- Deleção de Serviço ----

bool service_storage_delete(int index_to_delete) {
    if (index_to_delete < 0 || index_to_delete >= service_count) {
        Serial.printf("[Storage ERROR] Tentativa de deletar índice inválido: %d (total: %d)\n", index_to_delete, service_count);
        // Não mostra mensagem na UI aqui, pois isso não deveria ser acionado pelo usuário.
        return false;
    }

    Serial.printf("[Storage] Deletando serviço '%s' no índice %d.\n", services[index_to_delete].name, index_to_delete);

    // Desloca os elementos seguintes para cobrir o espaço removido
    for (int i = index_to_delete; i < service_count - 1; ++i) {
        services[i] = services[i + 1]; // Cópia de struct
    }
    service_count--;
    // Limpa a última posição (agora formalmente vazia) do array em memória
    if (service_count >=0 && service_count < MAX_SERVICES) { // Checagem de segurança
        memset(&services[service_count], 0, sizeof(TOTPService));
    }


    // Salva a nova lista (menor) no NVS
    if (!service_storage_save_all()) {
        // Erro ao salvar no NVS. Teoricamente, deveríamos tentar reverter a deleção em memória,
        // mas isso complica. Por ora, a memória está inconsistente com NVS.
        // A mensagem de erro já foi mostrada por save_all().
        // Recarregar tudo do NVS seria uma opção para restaurar consistência.
        service_storage_load_all(); // Tenta recarregar para estado consistente
        return false;
    }

    // Ajusta o índice do serviço atual, se necessário
    if (service_count == 0) {
        current_service_index = 0; // Ou -1 se preferir indicar "nenhum selecionado"
        current_totp.valid_key = false;
        snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(StringID::STR_ERROR_NO_SERVICES));
    } else {
        // Se o item deletado estava ANTES ou NO índice atual, o índice precisa ser ajustado
        // ou o item que tomou seu lugar precisa ser selecionado.
        // Se o índice deletado era o current_service_index, o próximo item (ou o anterior se era o último)
        // se torna o current_service_index.
        if (current_service_index >= service_count) { // Se deletou o último ou o índice ficou fora dos limites
            current_service_index = service_count - 1; // Seleciona o novo último item
        }
        // Se deletou um item antes do current_service_index, o current_service_index efetivamente
        // aponta para um item "anterior" na lista original, mas o conteúdo mudou.
        // Se deletou o current_service_index, o item que estava em current_service_index+1
        // agora está em current_service_index.
        // Em todos esses casos, precisamos recodificar a chave para o (novo) current_service_index.
        if (!totp_decode_current_service_key()) {
            // Erro ao decodificar a chave do novo serviço atual.
        }
    }
    return true;
}