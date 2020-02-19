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
#define VERTICAL 1
#define LEFT 2
#define RIGHT 3
#define PLAYER_1 0
#define PLAYER_2 1

char *gameboard = &PORTA;
char *scoreboard = &PORTD;
int joystick_x_pos;
int joystick_y_pos;
char cursor_pos;
char current_char;
char cursor_solid;
char delay_loops;
char board[32];
char visible[32];
char cursor_move_delay_count;
char cursor_movable;
char recieved_char;
char p1_score;
char p2_score;
char selected_tile;
char joystick_pressed;

void joystick_init(void);
void time_init(void);
void update_gameboard(void);
void get_current_char(void);
void toggle_cursor(void);
void update_cursor(char, char);
void gameboard_init(void);
void update_gameboard_from_input(void);
void make_custom_chars(void);
void randomize_gameboard(void);
void display_gameboard(void);
char get_cursor_index(char);
void startup(void);
void display_scoreboard(void);
void serial_init(void);
void check_for_match(char);

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
    gameboard_init();
    serial_init();
    startup();
    //Main loop
    while(1) {
        update_gameboard_from_input();
    }
}

void serial_init(void) {
    recieved_char = 0x00;
    TRISC = 0x80;
    TXEN = 1;
    TX9 = 1;
    CREN = 1;
    RX9 = 1;
    RCIE = 1;
    SYNC = 0;
    BRGH = 1;
    BRG16 = 0;
    SPBRG = 10; //115.2k Baud rate
    SPEN = 1;
}

void startup(void) {
    lcd_clear(gameboard);
    lcd_clear(scoreboard);
    lcd_puts(" Concentration!", scoreboard);
    lcd_puts("Press the button", gameboard);
    lcd_goto(ROW_2, gameboard);
    lcd_puts("    to start", gameboard);
    while(JOYSTICK_BUTTON);
    DelayMs(8);
    while(!JOYSTICK_BUTTON);
    DelayMs(8);
    lcd_clear(gameboard);
    lcd_clear(scoreboard);
    p1_score = 0;
    p2_score = 0;
    display_scoreboard();
    display_gameboard();
}

void display_scoreboard(void) {
    lcd_puts("     Score:", scoreboard);
    lcd_goto(ROW_2, scoreboard);
    lcd_puts(" P1: ", scoreboard);
    lcd_putch(((p1_score%100)/10)+0x30, scoreboard);
    lcd_putch((p1_score%10)+0x30, scoreboard);
    lcd_puts("  P2: ", scoreboard);
    lcd_putch(((p2_score%100)/10)+0x30, scoreboard);
    lcd_putch((p2_score%10)+0x30, scoreboard);
}

void display_gameboard(void) {
    lcd_clear(gameboard);
    char i;
    for(i=0;i<32;i++) {
        if(i == 16) {
            lcd_goto(ROW_2, gameboard);
        }
        lcd_putch(visible[i], gameboard);
    }
    lcd_goto(cursor_pos, gameboard);
}

void make_custom_chars(void) {
    const char smile[] = {0x00, 0x0A, 0x0A, 0x00, 0x15, 0x11, 0x0E, 0x00};
    lcd_set_custom_char(smile, 0x00, gameboard);
    const char diamond[] = {0x00, 0x04, 0x0E, 0x1F, 0x0E, 0x04, 0x00, 0x00};
    lcd_set_custom_char(diamond, 0x01, gameboard);
    const char heart[] = {0x00, 0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00};
    lcd_set_custom_char(heart, 0x02, gameboard);
    const char spade[] = {0x00, 0x04, 0x0E, 0x1F, 0x1F, 0x0E, 0x04, 0x0E};
    lcd_set_custom_char(spade, 0x03, gameboard);
    const char club[] = {0x00, 0x0E, 0x0E, 0x1F, 0x1F, 0x1F, 0x04, 0x0E};
    lcd_set_custom_char(club, 0x04, gameboard);
    const char skull[] = {0x0E, 0x15, 0x15, 0x1F, 0x0A, 0x0E, 0x0E, 0x00};
    lcd_set_custom_char(skull, 0x05, gameboard);
    const char RH[] = {0x1C, 0x14, 0x18, 0x14, 0x00, 0x05, 0x07, 0x05};
    lcd_set_custom_char(RH, 0x06, gameboard);
    const char PIC[] = {0x18, 0x18, 0x14, 0x04, 0x04, 0x03, 0x02, 0x03};
    lcd_set_custom_char(PIC, 0x07, gameboard);
}

void gameboard_init(void) {
    selected_tile = 0xFF;
    time_init();
    make_custom_chars();
    char i;
    for(i = 0; i < 32; i++) {
        visible[i] = 0xFF;
    }
    lcd_clear(gameboard);
    cursor_pos = 0x00;
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
    randomize_gameboard();
}

char get_cursor_index(char cursor) {
    if(cursor&0x40) {
        return (cursor-0x30);
    }
    return cursor;
}

void randomize_gameboard(void) {
    char used[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    char current = 0;
    char random;
    while(current != 32) {
        DelayMs(TMR2>>(2+(TMR1%4)));
        random = TMR1 % 32;
        if(used[random] == 0) {
            used[random] = 1;
            board[random] = current/4;
            current++;
        }
    }
}

void check_for_match(char player) {
    if(selected_tile == 0xFF) {
        selected_tile = cursor_pos;
    } else {
        if(board[get_cursor_index(selected_tile)] == board[get_cursor_index(cursor_pos)]) {
            if(player == PLAYER_1) {
                p1_score++;
            } else {
                p2_score++;
            }
            display_scoreboard();
        } else {
            display_gameboard();
            DelayMs(1000);
            visible[get_cursor_index(selected_tile)] = 0xFF;
            visible[get_cursor_index(cursor_pos)] = 0xFF;
            current_char = 0xFF;
        }
        selected_tile = 0xFF;
    }
}

void update_gameboard_from_input(void) {
    //Start a new joystick reading if the previous one has finished
    if(GO == 0) {
        GO = 1;
    }
    if((JOYSTICK_BUTTON == PRESSED)&&(joystick_pressed == RELEASED)) {
        DelayMs(6);
        joystick_pressed = PRESSED;
        if(visible[get_cursor_index(cursor_pos)] == 0xFF) {
            visible[get_cursor_index(cursor_pos)] = board[get_cursor_index(cursor_pos)];
            current_char = board[get_cursor_index(cursor_pos)];
            //TODO player number variable
            check_for_match(PLAYER_1);
            display_gameboard();
        }
    } else if(JOYSTICK_BUTTON == RELEASED) {
        joystick_pressed = RELEASED;
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
            update_cursor(30, LEFT);
        }
    } else if(joystick_x_pos < 100) {
        if(cursor_movable) {
            update_cursor(30, RIGHT);
        }
    } else if(joystick_x_pos > 600) {
        if(cursor_movable) {
            update_cursor(60, LEFT);
        }
    } else if(joystick_x_pos < 400) {
        if(cursor_movable) {
            update_cursor(60, RIGHT);
        }
    } else if(joystick_y_pos == 1021) {
        if(cursor_movable) {
            update_cursor(62, VERTICAL);
        }
    } else if(joystick_y_pos == 0) {
        if(cursor_movable) {
            update_cursor(62, VERTICAL);
        }
    } else {
        cursor_movable = 1;
    }
}

void update_cursor(char move_delay_count, char direction) {
    cursor_movable = 0;
    cursor_move_delay_count = move_delay_count;
    switch(direction) {
        case VERTICAL:
            cursor_pos ^= 0x40;
            break;
        case LEFT:
            switch(cursor_pos) {
                case 0x00:
                    cursor_pos = 0x0F;
                    break;
                case 0x40:
                    cursor_pos = 0x4F;
                    break;
                default:
                    cursor_pos--;
            }
            break;
        case RIGHT:
            switch(cursor_pos) {
                case 0x0F:
                    cursor_pos = 0x00;
                    break;
                case 0x4F:
                    cursor_pos = 0x40;
                    break;
                default:
                    cursor_pos++;
            }
            break;
    }
    lcd_putch(current_char, gameboard);
    lcd_goto(cursor_pos, gameboard);
    get_current_char();
    if(cursor_solid == 1) {
        lcd_putch(0x20, gameboard);
        lcd_goto(cursor_pos, gameboard);
    }
}

void toggle_cursor(void) {
    if(cursor_solid == 0) {
        lcd_putch(0x20, gameboard);
        cursor_solid = 1;
    } else {
        lcd_putch(current_char, gameboard);
        cursor_solid = 0;
    }
    lcd_goto(cursor_pos, gameboard);
}

void get_current_char(void) {
    current_char = visible[get_cursor_index(cursor_pos)];
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
    joystick_pressed = RELEASED;
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
