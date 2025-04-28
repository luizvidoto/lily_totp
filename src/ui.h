#pragma once // Include guard

// 1. Includes necessários para os tipos usados nas declarações abaixo
#include "types.h"   // Para ScreenState, MenuState, etc.
#include <stdint.h> // Para tipos como uint16_t (usado em message_color)

// 2. Declarações (Protótipos) das Funções Públicas do Módulo UI
//    (Sem o corpo da função " { ... } ")

// --- Inicialização ---
void initSprites(); // Configura os objetos TFT_eSprite

// --- Funções de Desenho Principais ---
void ui_drawScreen(bool full_redraw); // Função principal que orquestra o desenho da tela atual
void ui_drawHeader(StringID title); // Desenha a parte estática do header
void ui_updateHeaderDynamic(); // Atualiza e desenha partes dinâmicas do header (relógio, bateria) via sprites

// --- Funções de Desenho Específicas por Tela (Chamadas por ui_drawScreen) ---
// (Você PODE omitir estas do .h se NENHUM outro módulo as chamar diretamente,
//  mas incluí-las aqui documenta melhor a estrutura interna do módulo UI)
void ui_drawScreenBootingContent(bool full_redraw);
void ui_drawScreenTOTPContent(bool full_redraw);
void ui_drawScreenMenuContent(bool full_redraw);
void ui_drawScreenServiceAddWaitContent(bool full_redraw);
void ui_drawScreenServiceAddConfirmContent(bool full_redraw);
void ui_drawScreenTimeEditContent(bool full_redraw);
void ui_drawScreenServiceDeleteConfirmContent(bool full_redraw);
void ui_drawScreenTimezoneEditContent(bool full_redraw);
void ui_drawScreenLanguageSelectContent(bool full_redraw);
void ui_drawScreenReadRFIDContent(bool full_redraw);
void ui_drawScreenMessage(bool full_redraw); // Desenha a tela de mensagem temporária

// --- Funções de Controle da UI ---
void changeScreen(ScreenState new_screen); // Muda para uma nova tela
void ui_showTemporaryMessage(const char *msg, uint16_t color); // Configura e exibe uma mensagem temporária
bool ui_updateTemporaryMessage(); // Verifica se a mensagem temporária deve expirar (chamada no loop?) - ALTERNATIVA: Verificação pode ser no loop principal.
void ui_updateMenuAnimation(); // Processa a animação de scroll do menu (chamada no loop?) - ALTERNATIVA: Verificação pode ser no loop principal.
void resetMenuState(MenuState& menu); // Reseta o estado de um menu específico

// --- Funções de Atualização de Sprites (Chamadas internamente ou por lógica de atualização) ---
// (Também podem ser omitidas do .h se forem estritamente internas ao ui.cpp)
void ui_updateTotpCodeSprite();
void ui_updateProgressBarSprite(uint64_t current_timestamp_utc);
void ui_updateHeaderClockSprite();
void ui_updateHeaderBatterySprite();


// NÃO inclua aqui:
// - O corpo das funções { ... }
// - Variáveis globais (vão em globals.h/cpp)
// - Funções auxiliares estáticas (definidas apenas em ui.cpp)