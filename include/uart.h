#ifndef _UART_H_
#define _UART_H_

/*UART0 Pins To ARM*/
#define  GPA0CON  	(*(volatile unsigned int *) 0xE0200000)
#define  GPA1CON  	(*(volatile unsigned int *) 0xE0200020)

/*UART0 Pins*/
#define  ULCON0  	  	(*(volatile unsigned int *) 0xE2900000)
#define  UCON0  	  	(*(volatile unsigned int *) 0xE2900004)
#define  UFCON0  		(*(volatile unsigned int *) 0xE2900008)
#define  UMCON0  		(*(volatile unsigned int *) 0xE290000c)
#define  UTRSTAT0  		(*(volatile unsigned int *) 0xE2900010)
#define  UTXH0  		(*(volatile unsigned char*) 0xE2900020)
#define  URXH0  		(*(volatile unsigned char*) 0xE2900024)
#define  UBRDIV0 		(*(volatile unsigned int *) 0xE2900028)
#define  UDIVSLOT0  (*(volatile unsigned int *) 0xE290002c)

void uart_init();

void putc(unsigned char c);
unsigned char getc();
#endif //_UART_H_
