/* Concentration
 * ECE230-02 Final Project
 * Written by Zach Kelly and Tanner Brammeier
 * 2/11/20
 * 
 * Description:
 * This program allows two players using separate boards to play the game
 * concentration using serial communication.
 */

#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RE3/MCLR pin function select bit (RE3/MCLR pin function is MCLR)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)
#pragma config BOR4V = BOR21V   // Brown-out Reset Selection bit (Brown-out Reset set to 2.1V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

#include <xc.h>
#include "lcd4bits.h"

#define JOYSTICK_X 1
#define JOYSTICK_Y 0
#define JOYSTICK_BUTTON RB5
#define PRESSED 0
#define RELEASED 1
#define ROW_2 0x40

char *gameboard = &PORTD;
char *scoreboard = &PORTA;
int joystick_x_pos;
int joystick_y_pos;
char cursor_pos = 0x00;
char current_char;

void joystick_init(void);

void main(void) {
    //Clocl Setup
    SCS = 0;
    //LCD Setup
    TRISD = 0;
    TRISA = 0;
    ANSEL = 0;
    lcd_init(gameboard);
    lcd_init(scoreboard);
    joystick_init();
    //Main Loop
    const char board_r1[] = "0123456789ABCDEF";
    const char board_r2[] = "THIS IS ROW 2...";
    lcd_clear(gameboard);
    lcd_puts(board_r1, gameboard);
    lcd_goto(ROW_2, gameboard);
    lcd_puts(board_r2, gameboard);
    cursor_pos = 0x41;
    while(1) {
        //TODO difference between cursor and goto
        lcd_goto(cursor_pos, gameboard);
        if((cursor_pos>>4) == 4) {
            current_char = board_r2[cursor_pos&0x0F];
        } else {
            current_char = board_r1[cursor_pos&0x0F];
        }
        lcd_putch(0xFF, gameboard);
        DelayMs(250);
        lcd_goto(cursor_pos, gameboard);
        lcd_putch(current_char, gameboard);
        DelayMs(250);
        cursor_pos++;
        switch(cursor_pos) {
            case 0x10:
                cursor_pos = 0x40;
                break;
            case 0x50:
                cursor_pos = 0x00;
                break;
        }
    }
}

void joystick_init(void) {
    joystick_x_pos = 0;
    joystick_y_pos = 0;
    PORTB = 0;
    nRBPU = 0;
    WPUB = 0x20;
    TRISB = 0x38;
    ANSELH = 0x0A;
    GIE = 1;
    PEIE = 1;
    ADIF = 0;
    ADIE = 1;
    ADCON1 = 0x80;  //ADFM = 1, VCFG = 00
    ADCON0 = 0xA5;  //ADCS = 10 , CHS = 9, GO = 0, ADON = 1
}

void __interrupt() interrupt_handler(void) {
    if(ADIF) {
        if(CHS1 == JOYSTICK_X) {
            joystick_x_pos = (((int)ADRESH)<<8)+ADRESL;
            CHS1 = JOYSTICK_Y;
        } else {
            joystick_y_pos = (((int)ADRESH)<<8)+ADRESL;
            CHS1 = JOYSTICK_X;
        }
        ADIF = 0;
    }
}

//Testing Functions
//TODO delete later

void display_joystick_values(void) {
    while(JOYSTICK_BUTTON == PRESSED) {
        lcd_clear(gameboard);
        lcd_putch('P', gameboard);
        DelayMs(250);
    }
    GO = 1;
    while(GO == 1);
    lcd_clear(gameboard);
    lcd_putch(0x30+(joystick_x_pos/1000), gameboard);
    lcd_putch(0x30+((joystick_x_pos%1000)/100), gameboard);
    lcd_putch(0x30+((joystick_x_pos%100)/10), gameboard);
    lcd_putch(0x30+((joystick_x_pos%10)/1), gameboard);
    lcd_clear(scoreboard);
    lcd_putch(0x30+(joystick_y_pos/1000), scoreboard);
    lcd_putch(0x30+((joystick_y_pos%1000)/100), scoreboard);
    lcd_putch(0x30+((joystick_y_pos%100)/10), scoreboard);
    lcd_putch(0x30+((joystick_y_pos%10)/1), scoreboard);
    DelayMs(125);
}
