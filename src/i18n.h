#ifndef I18N_H
#define I18N_H

#include "types.h"

// Funções de Internacionalização
void i18n_init();
void i18n_set_language(Language lang);
Language i18n_get_current_language_selection();
void i18n_cycle_language_selection(bool next);
void i18n_save_selected_language();
const char* getText(StringID id);

// Para a tela de seleção de idioma
extern int current_language_menu_index; // Índice do idioma selecionado no menu de idiomas

#endif // I18N_H