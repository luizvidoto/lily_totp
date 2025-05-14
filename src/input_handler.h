// input_handler.h
#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

// Inicializa os botões e configura callbacks
void input_init();

// Processa eventos de botão e entrada serial (deve ser chamado no loop principal)
void input_tick();

// Processa dados recebidos via Serial (chamado por input_tick quando apropriado)
void input_process_serial();

void rfid_on_card_present(const char* uid_str);

#endif // INPUT_HANDLER_H