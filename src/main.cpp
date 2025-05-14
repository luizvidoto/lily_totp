/*
 * TOTP Authenticator - Refatorado e Otimizado
 *
 * Descrição: Implementa um autenticador TOTP para ESP32 com display TFT,
 *            RTC DS3231, leitor RFID MFRC522, gerenciamento de múltiplos
 *            serviços, persistência NVS, interface gráfica otimizada,
 *            gerenciamento de energia e internacionalização.
 *
 * Estrutura: Código modularizado em múltiplos arquivos (.h/.cpp) para
 *            melhor organização e manutenção.
 *
 * Autor: [Seu Nome/Apelido Aqui] - Baseado no código original fornecido.
 * Versão: 1.0 (Refatorada)
 * Data: [Data Atual]
 */

 #include <Arduino.h>
 #include "config.h"         // Configurações globais e constantes
 #include "types.h"          // Estruturas de dados e enums
 #include "globals.h"        // Variáveis globais (extern)
 #include "i18n.h"           // Internacionalização
 #include "power_utils.h"    // Gerenciamento de energia e brilho
 #include "time_keeper.h"    // Gerenciamento de tempo e RTC
 #include "service_storage.h"// Armazenamento de serviços (NVS)
 #include "totp_core.h"      // Lógica TOTP
 #include "ui_manager.h"     // Gerenciamento da Interface Gráfica (TFT)
 #include "input_handler.h"  // Gerenciamento de Entradas (Botões, Serial)
 #include "rfid_reader.h"    // Leitura RFID
 #include "rfid_auth.h"      // Autenticação RFID
 
 // ---- SETUP ----
 void setup() {
     Serial.begin(115200);
     while (!Serial && millis() < 2000); // Espera Serial ou timeout
     Serial.println("\n\n[SETUP] Iniciando TOTP Authenticator Otimizado...");
 
     // 1. Inicializa Gerenciamento de Energia (inclui pinos ADC, PWM Backlight)
     power_init(); // Configura pinos e PWM/LEDC para backlight
 
     // 2. Inicializa Display e Sprites
     ui_init_display();
     ui_init_sprites();
     
     // 3. Inicializa Internacionalização (carrega idioma salvo) - MOVED EARLIER
     i18n_init(); // Define current_strings_ptr

     // Mostra mensagem inicial enquanto carrega o resto - NOW USES INITIALIZED STRINGS
     ui_change_screen(ScreenState::SCREEN_STARTUP, true);
 
     // 4. Inicializa RTC e Tempo (sincroniza TimeLib, carrega GMT offset)
     if (!time_init_rtc()) {
         // Erro crítico no RTC! Mostra mensagem e trava.
         tft.fillScreen(COLOR_ERROR);
         tft.setTextColor(TFT_WHITE); tft.setTextDatum(MC_DATUM); tft.setTextSize(2);
         tft.drawString(getText(StringID::STR_ERROR_RTC_INIT), tft.width() / 2, tft.height() / 2);
         while (1) { delay(1000); }
     }
 
     // 5. Inicializa Armazenamento e Carrega Serviços
     if (!service_storage_load_all()) {
         // Erro ao carregar do NVS, mensagem já exibida por load_all()
         // Continua com service_count = 0
     }
 
     // 6. Inicializa Leitor RFID
     rfid_init();
     rfid_auth_init(); // Carrega cartões autorizados e estado de bloqueio do NVS
 
     // 7. Inicializa Gerenciador de Entradas (configura botões)
     input_init();
 
     // 8. Prepara estado inicial pós-carregamento
     if (service_count > 0) {
         current_service_index = 0; // Começa no primeiro serviço
         if (!totp_decode_current_service_key()) {
             // Erro ao decodificar a primeira chave (mensagem já exibida/logada)
             // A tela TOTP mostrará o placeholder de erro.
         }
         // Define a tela inicial como a de visualização TOTP se houver serviços
          ui_change_screen(ScreenState::SCREEN_TOTP_VIEW, true);
     } else {
         // Se não houver serviços, vai para o menu principal
         current_totp.valid_key = false; // Garante que não há chave válida
         snprintf(current_totp.code, sizeof(current_totp.code), "%s", getText(StringID::STR_ERROR_NO_SERVICES));
         ui_change_screen(ScreenState::SCREEN_MENU_MAIN, true);
     }
 
     // 9. Inicializa timers do loop principal
     last_interaction_time = millis();
     last_rtc_sync_time = millis();
     last_screen_update_time = 0; // Força atualização na primeira iteração do loop

     input_init(); // Configura callbacks dos botões

     last_interaction_time = millis(); 
     last_lock_check_time = millis();  

     // Define a tela inicial baseada no estado de bloqueio e se há cartões configurados
     if (authorized_card_count == 0 && MAX_AUTHORIZED_CARDS > 0) { // MAX_AUTHORIZED_CARDS > 0 ensures card feature is enabled
         Serial.println("[Setup] Nenhum cartão RFID autorizado. Indo para configuração do primeiro cartão.");
         ui_change_screen(ScreenState::SCREEN_SETUP_FIRST_CARD);
     } else if (is_locked) {
         ui_change_screen(ScreenState::SCREEN_LOCKED);
     } else if (service_count > 0) {
         ui_change_screen(ScreenState::SCREEN_TOTP_VIEW);
     } else {
         ui_change_screen(ScreenState::SCREEN_MENU_MAIN);
     }
     Serial.println("[Setup] Concluído.");
 }
 
 // ---- LOOP PRINCIPAL ----
 void loop() {
     uint32_t loop_start_millis = millis(); // Sample at loop start for general timing
     bool needs_dynamic_update = false; // Flag para indicar que elementos dinâmicos precisam ser redesenhados
 
     // 1. Processa Entradas (Botões, Serial, Timer da Tela de Mensagem)
     input_tick(); // This can call ui_change_screen, which updates last_interaction_time
 
     // 2. Leitura RFID (Apenas na tela específica)
     if (current_screen == ScreenState::SCREEN_LOCKED || 
         current_screen == ScreenState::SCREEN_READ_RFID || 
         current_screen == ScreenState::SCREEN_ADD_CARD_WAIT ||
         current_screen == ScreenState::SCREEN_SETUP_FIRST_CARD) { // Enable RFID scanning for first card setup
         if (rfid_read_card()) {
             // rfid_on_card_present is called from within rfid_read_card
             // which can call ui_change_screen, updating last_interaction_time
         }
     }
 
     // 3. Atualizações Periódicas baseadas em Tempo
     // Use loop_start_millis for these checks to be consistent within the iteration
     if (loop_start_millis - last_screen_update_time >= SCREEN_UPDATE_INTERVAL_MS) {
         needs_dynamic_update = true; 
         power_update_battery_status(); // Reads ADC, relatively quick
         if (current_screen == ScreenState::SCREEN_TOTP_VIEW) {
             totp_update_current_code(); 
         }
     }
 
     // 4. Sincronização com RTC (menos frequente)
     if (loop_start_millis - last_rtc_sync_time >= RTC_SYNC_INTERVAL_MS) {
         last_rtc_sync_time = loop_start_millis;
         time_sync_from_rtc(); 
     }
 
     // 5. Atualiza Brilho da Tela (verifica inatividade using ::last_interaction_time)
     power_update_target_brightness(); 

     // --- Lock Check --- 
     uint32_t check_time_millis = millis(); // Re-sample millis specifically for the lock check
     uint32_t current_lock_timeout_ms = (uint32_t)temp_lock_timeout_minutes * 60 * 1000;
     // Standard unsigned subtraction handles millis() rollover correctly for positive elapsed time.
     uint32_t time_since_last_interaction = check_time_millis - last_interaction_time; 

     if (!is_locked && (time_since_last_interaction > current_lock_timeout_ms)) {
         // Add a small guard against extremely small current_lock_timeout_ms if it could be zero or near zero.
         // However, temp_lock_timeout_minutes is constrained from 1 to 60, so timeout_ms will be at least 60000.
         Serial.printf("[Lock] Inactivity Lock Triggered! check_time: %lu, last_interaction: %lu, diff: %lu, timeout: %lu\n", 
                         check_time_millis, last_interaction_time, time_since_last_interaction, current_lock_timeout_ms);
         is_locked = true;
         rfid_auth_save_lock_state(true); 
         ui_change_screen(ScreenState::SCREEN_LOCKED); // This will update last_interaction_time again
     } else if (!is_locked) {
         // Optional: Print when NOT locking to see the values periodically
         // if (loop_start_millis % 5000 < LOOP_DELAY_MS) { // Print roughly every 5 seconds
         //     Serial.printf("[Lock] Check (Not Locking): check_time: %lu, last_interaction: %lu, diff: %lu, timeout: %lu, is_locked: %d\n", 
         //                 check_time_millis, last_interaction_time, time_since_last_interaction, current_lock_timeout_ms, is_locked);
         // }
     }
 
     // 6. Desenho da Tela
     // needs_dynamic_update is set if SCREEN_UPDATE_INTERVAL_MS has passed
     if (is_menu_animating || needs_dynamic_update) {
         if (!is_locked || 
             current_screen == ScreenState::SCREEN_LOCKED || 
             current_screen == ScreenState::SCREEN_MESSAGE || 
             current_screen == ScreenState::SCREEN_SETUP_FIRST_CARD) { // Allow setup screen even if technically locked
             ui_draw_screen(false); 
         } else {
             // If locked but not on an allowed screen (lock, message, setup), force to lock screen.
             ui_change_screen(ScreenState::SCREEN_LOCKED);
         }
         last_screen_update_time = loop_start_millis; // Update after drawing, using loop_start_millis for consistency
     }
 
     // 7. Pequeno Delay
     // Cede tempo para outras tarefas (se houver) e evita consumo excessivo de CPU.
     // Ajuda a suavizar a animação do menu.
     delay(LOOP_DELAY_MS);
 }