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

//TODO
char *gameboard = &PORTD;
char *scoreboard = &PORTA;
int joystick_x_pos = 0;
int joystick_y_pos  =0;
char cursor_pos = 0x00;

void update_board(void);

void main(void) {
    //Clocl Setup
    SCS = 0;
    //LCD Setup
    TRISD = 0;
    TRISA = 0;
    ANSEL = 0;
    lcd_init(gameboard);
    lcd_init(scoreboard);
    //Joystick Setup
    PORTB = 0;
    nRBPU = 0;
    WPUB = 0x31;
    TRISB = 0x31;
    ANSELH = 0x18;  //RB3 (y) and RB4 (x) are analog inputs
    ADCON0 = 0xA5;  //ADCS = 10 , CHS = 9, GO = 0, ADON = 1
    ADCON1 = 0x80;  //ADFM = 1, VCFG = 00
    GIE = 1;
    PEIE = 1;
    ADIF = 0;
    ADIE = 1;
    //Main Loop
    lcd_putch('X', gameboard);
    lcd_putch('X', scoreboard);
    DelayMs(1000);
    while(1) {
        update_board();
    }
}

void update_board(void) {
    //TODO figure out range of joystick voltage values
    GO = 1;
    DelayMs(500);
    lcd_clear(gameboard);
    lcd_putch(0x30+(joystick_x_pos/1000), gameboard);
    lcd_putch(0x30+((joystick_x_pos%1000)/100), gameboard);
    lcd_putch(0x30+((joystick_x_pos%100)/10), gameboard);
    lcd_putch(0x30+((joystick_x_pos%10)/1), gameboard);
    lcd_clear(scoreboard);
    lcd_putch(0x30+(joystick_x_pos/1000), scoreboard);
    lcd_putch(0x30+((joystick_x_pos%1000)/100), scoreboard);
    lcd_putch(0x30+((joystick_x_pos%100)/10), scoreboard);
    lcd_putch(0x30+((joystick_x_pos%10)/1), scoreboard);
}

void __interrupt() interrupt_handler(void) {
    if(ADIF) {
        if(CHS1 == JOYSTICK_X) {
            joystick_x_pos = (ADRESH<<8)+ADRESL;
        } else {
            joystick_y_pos = (ADRESH<<8)+ADRESL;
        }
        ADIF = 0;
    }
}
