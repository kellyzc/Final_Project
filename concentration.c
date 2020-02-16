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
char cursor_solid;
char delay_loops;
const char board_r1[] = "0123456789ABCDEF";
const char board_r2[] = "THIS IS ROW 2...";
char cursor_move_delay_count;
char cursor_movable;
char cursor_fast;
char recieved_char;

void joystick_init(void);
void time_init(void);
void update_gameboard(void);
void get_current_char(void);
void toggle_cursor(void);
void update_cursor(char);
void gameboard_init(char);
void update_gameboard_from_input(void);

void main(void) {
    //Clock Setup
    SCS = 0;
    //LCD Setup
    TRISD = 0;
    TRISA = 0;
    ANSEL = 0;
    lcd_init(gameboard);
    lcd_init(scoreboard);
    //Joystick Setup
    joystick_init();
    gameboard_init(0x45);
    //Main Loop
    recieved_char = 0x00;
    TRISC = 0x80;
    TXEN = 1;
//    TX9 = 1;
    CREN = 1;
//    RX9 = 1;
    RCIE = 1;
    SYNC = 0;
    BRGH = 1;
    BRG16 = 0;
    SPBRG = 10; //115.2k Baud rate
//    SPEN = 1;
    while(1) {
//        if(TXIF) {
//            TXREG = 'X';
//        }
//        DelayMs(1000);
        update_gameboard_from_input();
    }
}

void gameboard_init(char cursor_init_pos) {
    time_init();
    lcd_clear(gameboard);
    lcd_puts(board_r1, gameboard);
    lcd_goto(ROW_2, gameboard);
    lcd_puts(board_r2, gameboard);
    cursor_pos = cursor_init_pos;
    cursor_solid = 0;
    get_current_char();
    delay_loops = 0;
    cursor_movable = 1;
    lcd_goto(cursor_pos, gameboard);
    cursor_move_delay_count = 255;
    PR2 = 250; //TMR2IF triggered after 8ms
    TMR2IF = 0;
    TMR2IE = 1;
    T2CON = 0x56;   //Post-scaler = 1:10, pre-scaler = 1:16,TMR2ON = 1
}

void update_gameboard_from_input(void) {
    //Start a new joystick reading if the previous one has finished
    if(GO == 0) {
        GO = 1;
    }
    //Blink timing
    if(CCP1IF == 1) {
        if(delay_loops == 0) {
            delay_loops = 25;
            toggle_cursor();
        } else {
            delay_loops--;
        }
        CCPR1 = TMR1+50000;
        CCP1IF = 0;
    }
    //Move the cursor based on joystick input
    if(joystick_x_pos > 900) {
        if(cursor_movable) {
            cursor_pos--;
            update_cursor(30);
        }
    } else if(joystick_x_pos < 100) {
        if(cursor_movable) {
            cursor_pos++;
            update_cursor(30);
        }
    } else if(joystick_x_pos > 600) {
        if(cursor_movable) {
            cursor_pos--;
            update_cursor(60);
        }
    } else if(joystick_x_pos < 400) {
        if(cursor_movable) {
            cursor_pos++;
            update_cursor(60);
        }
    } else if(joystick_y_pos > 700) {
        if(cursor_movable) {
            cursor_pos ^= 0x40;
            update_cursor(62);
        }
    } else if(joystick_y_pos < 300) {
        if(cursor_movable) {
            cursor_pos ^= 0x40;
            update_cursor(62);
        }
    } else {
        cursor_movable = 1;
    }
}

void update_cursor(char move_delay_count) {
    cursor_movable = 0;
    cursor_move_delay_count = move_delay_count;
    switch(cursor_pos) {
        case 0x10:
            cursor_pos = 0x00;
            break;
        case 0x50:
            cursor_pos = 0x40;
            break;
        case 0xFF:
            cursor_pos = 0x0F;
            break;
        case 0x3F:
            cursor_pos = 0x4F;
            break;
    }
    lcd_putch(current_char, gameboard);
    lcd_goto(cursor_pos, gameboard);
    get_current_char();
    if(cursor_solid == 1) {
        lcd_putch(0xFF, gameboard);
        lcd_goto(cursor_pos, gameboard);
    }
}

void toggle_cursor(void) {
    if(cursor_solid == 0) {
        lcd_putch(0xFF, gameboard);
        cursor_solid = 1;
    } else {
        lcd_putch(current_char, gameboard);
        cursor_solid = 0;
    }
    lcd_goto(cursor_pos, gameboard);
}

void get_current_char(void) {
    if((cursor_pos>>4) == 4) {
        current_char = board_r2[cursor_pos&0x0F];
    } else {
        current_char = board_r1[cursor_pos&0x0F];
    }
}

//Initialization functions

void time_init(void) {
    CCP1M3 = 1;
    CCP1M2 = 0;
    CCP1M1 = 1;
    CCP1M0 = 0;
    TMR1CS = 0;
    T1CKPS0 = 0;
    T1CKPS1 = 0;
    TMR1GE = 0;
    TMR1ON = 1;
}

void joystick_init(void) {
    joystick_x_pos = 512;
    joystick_y_pos = 512;
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
    if(TMR2IF) {
        cursor_move_delay_count--;
        if(cursor_move_delay_count == 0) {
            cursor_movable = 1;
        }
        TMR2IF = 0;
    }
    if(RCIF) {
        recieved_char = RCREG;
        lcd_putch(recieved_char, gameboard);
    }
}
