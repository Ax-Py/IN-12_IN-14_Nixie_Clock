/*
 * UART.h
 *
 * Created: 1/6/2023 8:24:44 AM
 *  Author: aparady
 */ 


#ifndef UART_H_
#define UART_H_

void USART_Init(){
    // Set to 4800 bps
	UBRR0H = 0;
	UBRR0L = 12;

	UCSR0B = (1<<RXEN)|(1<<TXEN); // Enable receive and transmit
	UCSR0A = (1 << U2X); // Enable double speed transfers
}

void USART_Transmit(char Data){
	while (!(UCSR0A & (1<<UDRE))); // Wait for transmit buffer to be empty so more data can be put into it.
	UDR0 = Data; // Put data into buffer and send it.
}

void USART_Transmit_Hex(uint8_t Value){
	char ASCII_Table[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'}; // Lookup table for conversion from hex to ASCII char
	
	// Convert nibble to its ASCII hex equivalent
	char HighNibble = ASCII_Table[Value >> 4];
	char LowNibble = ASCII_Table[Value & 0x0F];

	// Print the characters
	USART_Transmit(HighNibble);
	USART_Transmit(LowNibble);
	USART_Transmit('$');
}

#endif /* UART_H_ */