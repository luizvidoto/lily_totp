// include Serial
#include <Arduino.h>

#include "input.h"
#include "globals.h"
#include "config.h"
#include "i18n.h"
#include "ui.h"
#include "types.h"
#include "totp.h"

// Storage (NVS)
void loadServices();
bool storage_saveServiceList();
bool storage_saveService(const char *, const char *);
bool storage_deleteService(int);

void loadServices() {
    if (!preferences.begin("totp-app", true)) { // Abre NVS no modo somente leitura
        Serial.println(getText(STR_ERROR_NVS_LOAD));
        service_count = 0; // Zera contagem se não conseguir ler
        return;
    }
    service_count = preferences.getInt("svc_count", 0); // Lê contador, default 0
    // Validações básicas do contador
    if (service_count < 0) service_count = 0;
    if (service_count > MAX_SERVICES) {
        Serial.printf("[WARN] Contador NVS (%d) > MAX (%d). Limitando.\n", service_count, MAX_SERVICES);
        service_count = MAX_SERVICES;
    }

    int valid_count = 0; // Contador para serviços válidos encontrados
    for (int i = 0; i < service_count; i++) {
        char name_key[16]; char secret_key[16];
        snprintf(name_key, sizeof(name_key), "svc_%d_name", i);
        snprintf(secret_key, sizeof(secret_key), "svc_%d_secret", i);

        String name_str = preferences.getString(name_key, "");
        String secret_str = preferences.getString(secret_key, "");

        // Verifica se os dados carregados são válidos (não vazios e dentro dos limites)
        if (name_str.length() > 0 && name_str.length() <= MAX_SERVICE_NAME_LEN &&
            secret_str.length() > 0 && secret_str.length() <= MAX_SECRET_B32_LEN)
        {
            // Copia para a posição correta no array 'services', compactando a lista
            strncpy(services[valid_count].name, name_str.c_str(), MAX_SERVICE_NAME_LEN);
            services[valid_count].name[MAX_SERVICE_NAME_LEN] = '\0';
            strncpy(services[valid_count].secret_b32, secret_str.c_str(), MAX_SECRET_B32_LEN);
            services[valid_count].secret_b32[MAX_SECRET_B32_LEN] = '\0';
            valid_count++; // Incrementa apenas se o serviço for válido
        } else {
            Serial.printf("[WARN] Serviço %d inválido/ausente no NVS. Pulando.\n", i);
            // Não incrementa valid_count
        }
    }
    preferences.end(); // Fecha NVS

    // Se encontramos menos serviços válidos do que o contador indicava, ajusta
    if (valid_count != service_count) {
        Serial.printf("Serviços compactados de %d para %d.\n", service_count, valid_count);
        service_count = valid_count;
        // Opcional: Salvar a lista compactada de volta no NVS
        // storage_saveServiceList();
    }
    Serial.printf("[NVS] %d serviços válidos carregados.\n", service_count);
}


// ---- Funções de Armazenamento (NVS) ----
bool storage_saveServiceList() {
    if(!preferences.begin("totp-app", false)) { // Abre NVS para leitura/escrita
        Serial.println(getText(STR_ERROR_NVS_SAVE));
        return false;
    }
    int old_count = preferences.getInt("svc_count", 0); // Lê contador antigo
    // Remove chaves de serviços que não existem mais (se lista diminuiu)
    for(int i = service_count; i < old_count; ++i) {
        char name_key[16], secret_key[16];
        snprintf(name_key,sizeof(name_key),"svc_%d_name",i);
        snprintf(secret_key,sizeof(secret_key),"svc_%d_secret",i);
        preferences.remove(name_key);
        preferences.remove(secret_key);
    }
    // Salva o novo contador de serviços
    preferences.putInt("svc_count", service_count);
    bool success = true;
    // Salva cada serviço atual no NVS
    for(int i = 0; i < service_count; i++) {
        char name_key[16], secret_key[16];
        snprintf(name_key,sizeof(name_key),"svc_%d_name",i);
        snprintf(secret_key,sizeof(secret_key),"svc_%d_secret",i);
        if(!preferences.putString(name_key, services[i].name)) success = false;
        if(!preferences.putString(secret_key, services[i].secret_b32)) success = false;
    }
    preferences.end(); // Fecha NVS
    if (!success) Serial.println(getText(STR_ERROR_NVS_SAVE));
    return success;
}

bool storage_saveService(const char *name, const char *secret_b32) {
    if(service_count >= MAX_SERVICES){
        ui_showTemporaryMessage(getText(STR_ERROR_MAX_SERVICES), COLOR_ERROR);
        return false;
    }
    // Adiciona ao array em memória
    strncpy(services[service_count].name, name, MAX_SERVICE_NAME_LEN);
    services[service_count].name[MAX_SERVICE_NAME_LEN] = '\0';
    strncpy(services[service_count].secret_b32, secret_b32, MAX_SECRET_B32_LEN);
    services[service_count].secret_b32[MAX_SECRET_B32_LEN] = '\0';
    service_count++; // Incrementa contador
    // Salva a lista inteira atualizada no NVS
    return storage_saveServiceList();
}

bool storage_deleteService(int index) {
    if(index < 0 || index >= service_count) {
        Serial.printf("[ERROR] Tentativa de deletar índice inválido: %d\n", index);
        return false;
    }
    Serial.printf("[NVS] Deletando '%s' (idx %d)\n", services[index].name, index);
    // Desloca os elementos seguintes para cobrir o espaço removido
    for(int i = index; i < service_count - 1; i++) {
        services[i] = services[i+1];
    }
    service_count--; // Decrementa o contador
    memset(&services[service_count], 0, sizeof(TOTPService)); // Limpa a última posição (agora vazia)

    // Ajusta o índice do serviço atual, se necessário
    if(current_service_index >= service_count && service_count > 0) {
        current_service_index = service_count - 1; // Seleciona o último item se o deletado era o último ou além
    } else if (service_count == 0) {
        current_service_index = 0; // Zera se não houver mais serviços
    }
    // Se deletou um item *antes* do atual, o índice atual continua apontando para o mesmo serviço (que subiu uma posição)
    // Se deletou o item *atual*, o índice agora aponta para o próximo item (que tomou o lugar do deletado)

    bool suc = storage_saveServiceList(); // Salva a nova lista (menor) no NVS

    // Recarrega a chave do serviço que agora está no índice atual
    if(service_count > 0) {
        decodeCurrentServiceKey();
    } else { // Se não houver mais serviços
        current_totp.valid_key_loaded = false;
        snprintf(current_totp.code, sizeof(current_totp.code),"%s",getText(STR_TOTP_CODE_ERROR));
    }
    return suc;
}


