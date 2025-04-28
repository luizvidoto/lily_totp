#pragma once // Include guard

// ============================================================================
// === FUNÇÕES PÚBLICAS DO MÓDULO DE ENTRADA ===
// ============================================================================

/**
 * @brief Configura os callbacks (click, double click, long press) para os botões
 *        usando a biblioteca OneButton.
 *        Deve ser chamado no setup() após a inicialização dos botões.
 */
void attachButtons();

/**
 * @brief Processa os eventos dos botões (chama btn.tick()) e a entrada serial.
 *        Deve ser chamado repetidamente no loop() principal.
 */
void input_tick();

/**
 * @brief Verifica se há dados na Serial, lê uma linha JSON, faz o parse
 *        e chama as funções apropriadas para processar os dados (adição de serviço ou ajuste de hora).
 *        Normalmente chamado por input_tick(), mas pode ser chamado diretamente se necessário.
 */
void processSerialInput();

// Nota: As funções de callback específicas dos botões (ex: void btn_next_click_handler())
// são geralmente definidas como 'static' dentro de input.cpp e não precisam ser
// declaradas aqui, pois são chamadas internamente pela biblioteca OneButton.


void configureButtonCallbacks(); // Configura os callbacks dos botões (OneButton)