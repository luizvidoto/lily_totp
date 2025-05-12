// time_keeper.h
#ifndef TIME_KEEPER_H
#define TIME_KEEPER_H

#include <stdint.h> // Para uint64_t
#include <TimeLib.h> // Para time_t
#include <ArduinoJson.h> // Para JsonDocument

// ---- Inicialização ----
// Inicializa o RTC e sincroniza o TimeLib. Retorna false em caso de falha do RTC.
bool time_init_rtc();

// ---- Sincronização ----
// Atualiza o TimeLib com a hora do RTC.
void time_sync_from_rtc();
// Atualiza o hardware do RTC com a hora do TimeLib (UTC).
void time_sync_to_rtc();

// ---- Obtenção de Tempo ----
// Retorna o tempo Unix UTC atual do TimeLib.
uint64_t time_now_utc();
// Retorna o tempo local atual (considerando gmt_offset).
time_t time_local_now();

// ---- Ajuste Manual de Hora (para tela SCREEN_TIME_EDIT) ----
// Prepara as variáveis edit_hour, edit_minute, edit_second com a hora UTC atual.
void time_start_manual_edit();
// Ajusta o valor do campo de edição atual (edit_time_field) por 'delta' (+1 ou -1).
void time_adjust_manual_field(int delta);
// Avança para o próximo campo de edição (H->M->S). Se estiver no último (S),
// salva a hora editada no TimeLib e RTC, e retorna true. Caso contrário, retorna false.
bool time_next_manual_edit_field();

// ---- Ajuste de Hora via JSON (para entrada Serial) ----
// Valida e define a hora do sistema (UTC) e RTC a partir de um JsonDocument.
// Mostra mensagem de sucesso/erro.
void time_set_from_json(JsonDocument& doc);

// ---- Fuso Horário (GMT Offset) ----
// Carrega o GMT offset do NVS.
void time_load_gmt_offset();
// Ajusta o gmt_offset global por 'delta' (+1 ou -1), com wrap around.
void time_adjust_gmt_offset(int delta);
// Salva o gmt_offset atual no NVS.
void time_save_gmt_offset();

#endif // TIME_KEEPER_H