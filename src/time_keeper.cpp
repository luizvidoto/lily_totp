// time_keeper.cpp
#include "time_keeper.h"
#include "globals.h"    // Acesso a rtc, gmt_offset, edit_time_field, etc.
#include "config.h"     // Acesso a NVS_NAMESPACE, NVS_TZ_OFFSET_KEY
#include "ui_manager.h" // Para ui_queue_message e ui_queue_message_fmt
#include "i18n.h"       // Para getText e StringID
#include <Arduino.h>    // Para Serial, setTime, now, etc.

// ---- Inicialização ----
bool time_init_rtc() {
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL); // Inicializa I2C
    Serial.print("[Time] Inicializando RTC DS3231... ");
    if (!rtc.begin()) {
        Serial.println("FALHOU!");
        // Não podemos usar ui_queue_message aqui ainda, pois a UI pode não estar pronta.
        // O setup principal pode lidar com a exibição de erro se isso retornar false.
        return false;
    }
    Serial.println("OK.");

    if (rtc.lostPower()) {
        Serial.println("[Time] RTC perdeu energia (bateria pode estar fraca ou ausente). A hora pode estar incorreta.");
        // Poderia forçar o usuário a ajustar a hora aqui, ou definir uma hora padrão.
        // Ex: rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Define para hora da compilação
        // setTime(23, 59, 50, 31, 12, 2023); // Exemplo: 31/12/2023 23:59:50 UTC
        // time_sync_to_rtc();
        // Serial.println("[Time] RTC ajustado para hora padrão/compilação devido à perda de energia.");
    }

    time_sync_from_rtc(); // Sincroniza TimeLib com RTC na inicialização
    time_load_gmt_offset(); // Carrega fuso horário salvo
    return true;
}

// ---- Sincronização ----
void time_sync_from_rtc() {
    if (rtc.now().unixtime() > 0) { // Verifica se o RTC tem uma hora válida
        setTime(rtc.now().unixtime()); // Ajusta TimeLib para UTC do RTC
        // Serial.println("[Time] TimeLib sincronizado com RTC.");
    } else {
        Serial.println("[Time] RTC retornou hora inválida (0). Não sincronizando TimeLib.");
    }
}

void time_sync_to_rtc() {
    // TimeLib (now()) já está em UTC.
    DateTime dt_to_set(year(), month(), day(), hour(), minute(), second());
    rtc.adjust(dt_to_set);
    Serial.println("[Time] RTC HW atualizado com hora do sistema (UTC).");
}

// ---- Obtenção de Tempo ----
uint64_t time_now_utc() {
    return now(); // now() de TimeLib retorna Unix timestamp UTC
}

time_t time_local_now() {
    return time_now_utc() + (gmt_offset * 3600L); // Aplica offset para hora local
}

// ---- Ajuste Manual de Hora ----
void time_start_manual_edit() {
    time_t current_utc_time = time_now_utc();
    edit_hour = hour(current_utc_time);
    edit_minute = minute(current_utc_time);
    edit_second = second(current_utc_time);
    edit_time_field = 0; // Começa editando a hora
}

void time_adjust_manual_field(int delta) {
    switch (edit_time_field) {
        case 0: // Hora
            edit_hour = (edit_hour + delta + 24) % 24;
            break;
        case 1: // Minuto
            edit_minute = (edit_minute + delta + 60) % 60;
            break;
        case 2: // Segundo
            edit_second = (edit_second + delta + 60) % 60;
            break;
    }
}

bool time_next_manual_edit_field() {
    edit_time_field++;
    if (edit_time_field > 2) { // Passou dos segundos, hora de salvar
        // Define a hora do sistema (TimeLib) com os valores editados (que são UTC)
        setTime(edit_hour, edit_minute, edit_second, day(), month(), year()); // Mantém data atual
        time_sync_to_rtc(); // Atualiza o hardware do RTC
        edit_time_field = 0; // Reseta campo de edição
        return true; // Indica que a hora foi salva
    }
    return false; // Indica que apenas mudou de campo
}

// ---- Ajuste de Hora via JSON ----
void time_set_from_json(JsonDocument& doc) {
    // Validação dos campos obrigatórios e seus tipos
    if (!doc.containsKey("year") || !doc["year"].is<int>() ||
        !doc.containsKey("month") || !doc["month"].is<int>() ||
        !doc.containsKey("day") || !doc["day"].is<int>() ||
        !doc.containsKey("hour") || !doc["hour"].is<int>() ||
        !doc.containsKey("minute") || !doc["minute"].is<int>() ||
        !doc.containsKey("second") || !doc["second"].is<int>()) {
        ui_queue_message(getText(StringID::STR_ERROR_JSON_INVALID_TIME), COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN);
        return;
    }

    int year = doc["year"].as<int>();
    int month = doc["month"].as<int>();
    int day = doc["day"].as<int>();
    int hour_val = doc["hour"].as<int>();
    int minute_val = doc["minute"].as<int>();
    int second_val = doc["second"].as<int>();

    // Validação básica dos valores
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour_val < 0 || hour_val > 23 || minute_val < 0 || minute_val > 59 || second_val < 0 || second_val > 59) {
        ui_queue_message(getText(StringID::STR_ERROR_INVALID_DATETIME), COLOR_ERROR, 3000, ScreenState::SCREEN_MENU_MAIN);
        return;
    }

    // Se tudo OK, define a hora do sistema (TimeLib) como UTC e atualiza o RTC
    setTime(hour_val, minute_val, second_val, day, month, year);
    time_sync_to_rtc();

    // Mostra mensagem de sucesso com a hora LOCAL ajustada
    time_t local_t = time_local_now();
    ui_queue_message_fmt(StringID::STR_TIME_ADJUSTED_FMT, COLOR_SUCCESS, 2500, ScreenState::SCREEN_MENU_MAIN,
                         hour(local_t), minute(local_t), second(local_t));
}

// ---- Fuso Horário (GMT Offset) ----
void time_load_gmt_offset() {
    preferences.begin(NVS_NAMESPACE, true); // Abre NVS para leitura
    gmt_offset = preferences.getInt(NVS_TZ_OFFSET_KEY, 0); // Padrão GMT+0 se não existir
    preferences.end();
    // Garante que o offset esteja dentro de um intervalo razoável, ex: -12 a +14
    gmt_offset = constrain(gmt_offset, -12, 14);
    Serial.printf("[Time] GMT Offset carregado: %+d\n", gmt_offset);
}

void time_adjust_gmt_offset(int delta) {
    gmt_offset += delta;
    // Wrap around: se passar de +14 vai para -12, se passar de -12 (negativo) vai para +14
    if (gmt_offset > 14) gmt_offset = -12;
    if (gmt_offset < -12) gmt_offset = 14;
}

void time_save_gmt_offset() {
    preferences.begin(NVS_NAMESPACE, false); // Abre NVS para escrita
    preferences.putInt(NVS_TZ_OFFSET_KEY, gmt_offset);
    preferences.end();
    Serial.printf("[Time] GMT Offset salvo: %+d\n", gmt_offset);
}