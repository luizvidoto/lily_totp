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
     // Mostra mensagem inicial enquanto carrega o resto
     ui_change_screen(ScreenState::SCREEN_STARTUP, true);
 
     // 3. Inicializa Internacionalização (carrega idioma salvo)
     i18n_init(); // Define current_strings_ptr
 
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
 
     Serial.println("[SETUP] Inicialização concluída.");
 }
 
 // ---- LOOP PRINCIPAL ----
 void loop() {
     uint32_t current_millis = millis();
     bool needs_dynamic_update = false; // Flag para indicar que elementos dinâmicos precisam ser redesenhados
 
     // 1. Processa Entradas (Botões, Serial, Timer da Tela de Mensagem)
     input_tick();
 
     // 2. Leitura RFID (Apenas na tela específica)
     if (current_screen == ScreenState::SCREEN_READ_RFID) {
         if (rfid_read_card()) {
             // Cartão lido com sucesso, UID está em card_id.
             // Força redesenho da tela RFID para mostrar o ID.
             ui_draw_screen(false); // Redesenho parcial é suficiente
         }
     }
 
     // 3. Atualizações Periódicas baseadas em Tempo
     if (current_millis - last_screen_update_time >= SCREEN_UPDATE_INTERVAL_MS) {
         last_screen_update_time = current_millis;
         needs_dynamic_update = true; // Indica que precisamos redesenhar elementos dinâmicos
 
         // Atualiza status da bateria
         power_update_battery_status();
 
         // Atualiza código TOTP se estiver na tela correspondente
         if (current_screen == ScreenState::SCREEN_TOTP_VIEW) {
             totp_update_current_code(); // Gera novo código se necessário
         }
     }
 
     // 4. Sincronização com RTC (menos frequente)
     if (current_millis - last_rtc_sync_time >= RTC_SYNC_INTERVAL_MS) {
         last_rtc_sync_time = current_millis;
         time_sync_from_rtc(); // Atualiza TimeLib a partir do RTC
     }
 
     // 5. Atualiza Brilho da Tela (verifica inatividade)
     power_update_target_brightness(); // Aplica brilho DIMMED/BATTERY/USB
 
     // 6. Desenho da Tela
     // Redesenha se houver animação de menu OU se uma atualização dinâmica ocorreu
     if (is_menu_animating || needs_dynamic_update) {
         // ui_draw_screen(false) é chamado para atualizar apenas o conteúdo
         // e os sprites dinâmicos (header, código totp, barra progresso).
         // A função ui_draw_screen internamente chama as funções de atualização
         // dos sprites necessários antes de desenhá-los.
         ui_draw_screen(false);
     }
 
     // 7. Pequeno Delay
     // Cede tempo para outras tarefas (se houver) e evita consumo excessivo de CPU.
     // Ajuda a suavizar a animação do menu.
     delay(LOOP_DELAY_MS);
 }