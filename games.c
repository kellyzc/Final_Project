/* Concentration
 * ECE230-02 Final Project
 * Written by Zach Kelly and Tanner Brammeier
 * 2/23/20
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

// <editor-fold defaultstate="collapsed" desc="defines and function declarations">
#define JOYSTICK_X 1
#define JOYSTICK_Y 0
#define JOYSTICK_BUTTON_1 RB5
#define JOYSTICK_BUTTON_2 RB2
#define PRESSED 0
#define RELEASED 1
#define ROW_2 0x40
#define VERTICAL 1
#define LEFT 2
#define RIGHT 3
#define PLAYER_1 0
#define PLAYER_2 1
#define JX1 0x2C
#define JX2 0x28
#define JY1 0x24
#define JY2 0x30
#define PERIOD_5HZ 10000
#define PERIOD_1KHZ 5000
#define C2SHARP 36075
#define F6SHARP 1689 
#define A6 1420
#define D6SHARP 2009
#define E6 1896
#define G6 1594
#define C6 2389
#define D3 17026
#define D4 8513
#define D5 4257
#define D6 2128
#define LED_RED RC5
#define LED_GREEN RC6
#define LED_BLUE RC7
#define RED 0
#define GREEN 1
#define BLUE 2
#define YELLOW 3
#define CYAN 4
#define MAGENTA 5
#define WHITE 6
#define CONCENTRATION 0x00
#define SIMON_SAYS 0x40

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
char event_delay_count;
char event_enabled;
char received_char;
char p1_score;
char p2_score;
char selected_tile;
char joystick_pressed;
char id_num;
char my_turn = 1;
char current_player;
char game_over;
int tone_delay;
char simon_says_pattern[25];

void joystick_init(void);
void time_init(void);
void update_gameboard(void);
void get_current_char(void);
void toggle_cursor(void);
void update_cursor(char, char);
void concentration_gameboard_init(void);
void concentration_game_loop(void);
void make_custom_chars(void);
void randomize_gameboard(void);
void display_concentration_gameboard(void);
char get_cursor_index(char);
void concentration_title_screen(void);
void display_concentration_scoreboard(void);
void check_for_match(char);
void concentration_game_end(void);
void play_tone(unsigned int,char);
void turn_on_led(char);
void simon_says_game_loop(void);
void countdown(void);
void display_simon_says_scoreboard(void);
void simon_says_title_screen(void);
void generate_pattern(void);
void play_pattern(void);
char simon_says_get_input(void);
void simon_says_game_end(void);
void simon_says_check_input(void);
void end_screen(const char *r1, char *r2);
// </editor-fold>

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
    //LED Setup
    TRISC &= 0x1F;
    PORTC = 0;
    //Timer Initialization
    time_init();
    //Variable Initialization
    current_player = PLAYER_1;
    //Game Selection
    lcd_puts(" Game Selection", scoreboard);
    lcd_puts("  Concentration", gameboard);
    lcd_goto(ROW_2, gameboard);
    lcd_puts("  Simon Says", gameboard);
    lcd_goto(0, gameboard);
    lcd_putch(0x7E, gameboard);
    char game_selection = 0xFF;
    char selector_pos = CONCENTRATION;
    GO = 1;
    while(game_selection == 0xFF) {
        if(GO == 0) {
            GO = 1;
        }
        if(event_enabled) {
            if(JOYSTICK_BUTTON_1 == PRESSED) {
                game_selection = selector_pos;
                DelayMs(8);
                while(JOYSTICK_BUTTON_1 == PRESSED);
                break;
            }
            if((joystick_y_pos < 100)||(joystick_y_pos > 900)) {
                lcd_goto(selector_pos, gameboard);
                lcd_putch(0x20, gameboard);
                switch(selector_pos) {
                    case CONCENTRATION:
                        selector_pos = SIMON_SAYS;
                        break;
                    case SIMON_SAYS:
                        selector_pos = CONCENTRATION;
                        break;
                }
                lcd_goto(selector_pos, gameboard);
                lcd_putch(0x7E, gameboard);
                event_enabled = 0;
                event_delay_count = 62;
            }
        }
    }
    game_over = 0;
    lcd_clear(gameboard);
    lcd_clear(scoreboard);
    if(game_selection == CONCENTRATION) {
        concentration_gameboard_init();
        concentration_title_screen();
        while(!game_over) {
            concentration_game_loop();
        }
        concentration_game_end();
    } else {
        simon_says_title_screen();
        while(!game_over) {
            simon_says_game_loop();
        }
        simon_says_game_end();
    }
}

void simon_says_game_end(void) {
    if(p1_score == 0) {
        end_screen("   Game Over!", "   You stink!");
    } else if(p1_score <= 5) {
        end_screen("   Game Over!", "Not very good...");
    } else if(p1_score <= 10) {
        end_screen("   Game Over!", "    Not bad.");
    } else if(p1_score <= 15) {
        end_screen("   Game Over!", "   Great job!");
    } else if(p1_score < 20) {
        end_screen("   Game Over!", " Almost there!!");
    } else {
        end_screen("   Game Over!", "   You win!!!");
    }
}

void simon_says_game_loop(void) {
    display_simon_says_scoreboard();
    countdown();
    generate_pattern();
    play_pattern();
    simon_says_check_input();
}

void simon_says_check_input(void) {
    lcd_puts("    Now it's", gameboard);
    lcd_goto(ROW_2, gameboard);
    lcd_puts("   your turn!", gameboard);
    char i;
    char current_option;
    for(i=0;i<(p1_score+1);i++) {
        switch(i%4) {
            case 0:
                current_option = (simon_says_pattern[i/4])&0x03;
                break;
            case 1:
                current_option = (((simon_says_pattern[i/4])&0x0C)>>2);
                break;
            case 2:
                current_option = (((simon_says_pattern[i/4])&0x30)>>4);
                break;
            case 3:
                current_option = (((simon_says_pattern[i/4])&0xC0)>>6);
                break;
        }
        if(current_option != simon_says_get_input()) {
            game_over = 1;
            break;
        }
    }
    lcd_clear(gameboard);
    if(game_over == 0) {
        p1_score++;
    }
    if(p1_score == 20) {
        game_over = 1;
    }
}

char simon_says_get_input(void) {
    char current_input;
    while(1) {
        if(GO == 0) {
            GO = 1;
        }
        if(joystick_x_pos > 1000) {
            if(CCP2IE == 0) {
                tone_delay = D3;
                CCPR2 = TMR1+D3;
                CCP2IF = 0;
                CCP2IE = 1;
                turn_on_led(RED);
            }
            if(JOYSTICK_BUTTON_1 == PRESSED) {
                current_input = 0;
                break;
            }
        } else if(joystick_x_pos < 20) {
            if(CCP2IE == 0) {
                tone_delay = D4;
                CCPR2 = TMR1+D4;
                CCP2IF = 0;
                CCP2IE = 1;
                turn_on_led(GREEN);
            }
            if(JOYSTICK_BUTTON_1 == PRESSED) {
                current_input = 1;
                break;
            }
        } else if(joystick_y_pos > 1000) {
            if(CCP2IE == 0) {
                tone_delay = D5;
                CCPR2 = TMR1+D5;
                CCP2IF = 0;
                CCP2IE = 1;
                turn_on_led(BLUE);
            }
            if(JOYSTICK_BUTTON_1 == PRESSED) {
                current_input = 2;
                break;
            }
        } else if(joystick_y_pos < 20) {
            if(CCP2IE == 0) {
                tone_delay = D6;
                CCPR2 = TMR1+D6;
                CCP2IF = 0;
                CCP2IE = 1;
                turn_on_led(MAGENTA);
            }
            if(JOYSTICK_BUTTON_1 == PRESSED) {
                current_input = 3;
                break;
            }
        } else {
            CCP2IE = 0;
            PORTC &= 0x1F;
        }
    }
    CCP2IE = 0;
    PORTC &= 0x1F;
    DelayMs(8);
    while(JOYSTICK_BUTTON_1 == PRESSED);
    GO = 1;
    DelayMs(500);
    return current_input;
}

void play_pattern(void) {
    lcd_puts(" Simon Says....", gameboard);
    char i;
    char current_option;
    for(i=0;i<(p1_score+1);i++) {
        switch(i%4) {
            case 0:
                current_option = (simon_says_pattern[i/4])&0x03;
                break;
            case 1:
                current_option = (((simon_says_pattern[i/4])&0x0C)>>2);
                break;
            case 2:
                current_option = (((simon_says_pattern[i/4])&0x30)>>4);
                break;
            case 3:
                current_option = (((simon_says_pattern[i/4])&0xC0)>>6);
                break;
        }
        switch(current_option) {
        case 0:
            turn_on_led(RED);
            play_tone(D3, 63);
            break;
        case 1:
            turn_on_led(GREEN);
            play_tone(D4, 63);
            break;
        case 2:
            turn_on_led(BLUE);
            play_tone(D5, 63);
            break;
        case 3:
            turn_on_led(MAGENTA);
            play_tone(D6, 63);
            break;
        }
        PORTC &= 0x1F;
        DelayMs(250);
    }
    lcd_clear(gameboard);
}

void generate_pattern(void) {
    char current_byte;
    DelayMs(TMR2>>(2+(TMR1%4)));
    current_byte = simon_says_pattern[p1_score/4];
    switch(p1_score%4) {
        case 0:
            current_byte &= 0xFC;
            current_byte |= TMR1%4;
            break;
        case 1:
            current_byte &= 0xF3;
            current_byte |= ((TMR1%4)<<2);
            break;
        case 2:
            current_byte &= 0xCF;
            current_byte |= ((TMR1%4)<<4);
            break;
        case 3:
            current_byte &= 0x3F;
            current_byte |= ((TMR1%4)<<6);
            break;
    }
    simon_says_pattern[p1_score/4] = current_byte;
}

void countdown(void) {
    lcd_clear(gameboard);
    signed char i;
    for(i=3;i>=0;i--) {
        lcd_goto(0x07, gameboard);
        lcd_putch(i+0x30, gameboard);
        DelayMs(1000);
    }
    lcd_clear(gameboard);
}

void display_simon_says_scoreboard(void) {
    lcd_clear(scoreboard);
    lcd_puts("     Score:", scoreboard);
    lcd_goto(0x43, scoreboard);
    lcd_putch(((p1_score%100)/10)+0x30, scoreboard);
    lcd_putch((p1_score%10)+0x30, scoreboard);
    lcd_puts("  Rounds", scoreboard);
}

void simon_says_title_screen(void) {
    lcd_puts("   Simon Says", scoreboard);
    lcd_puts("Press the button", gameboard);
    lcd_goto(ROW_2, gameboard);
    lcd_puts("    to start", gameboard);
    while(JOYSTICK_BUTTON_1 == RELEASED);
    DelayMs(8);
    while(JOYSTICK_BUTTON_1 == PRESSED);
    DelayMs(8);
    lcd_clear(scoreboard);
    lcd_clear(gameboard);
    p1_score = 0;
}

void turn_on_led(char color) {
    PORTC &= 0x1F;
    switch(color) {
        case RED:
            LED_RED = 1;
            break;
        case GREEN:
            LED_GREEN = 1;
            break;
        case BLUE:
            LED_BLUE = 1;
            break;
        case YELLOW:
            LED_RED = 1;
            LED_GREEN = 1;
            break;
        case CYAN:
            LED_GREEN = 1;
            LED_BLUE = 1;
            break;
        case MAGENTA:
            LED_BLUE = 1;
            LED_RED = 1;
            break;
        case WHITE:
            LED_RED = 1;
            LED_GREEN = 1;
            LED_BLUE = 1;
            break;
    }
}

void end_screen(const char *first_row, char *second_row) {
    joystick_pressed = RELEASED;
    char colors[] = {RED,YELLOW,GREEN,CYAN,BLUE,MAGENTA};
    char color_index = 0;
    char i;
    for(i=0;i<3;i++) {
        turn_on_led(colors[color_index]);
        color_index++;
        color_index %= 6;
        play_tone(D6SHARP, 30);
        turn_on_led(colors[color_index]);
        color_index++;
        color_index %= 6;
        play_tone(E6, 30);
        turn_on_led(colors[color_index]);
        color_index++;
        color_index %= 6;
        play_tone(G6, 30);
        turn_on_led(colors[color_index]);
        color_index++;
        color_index %= 6;
        DelayMs(250);
    }
    turn_on_led(colors[color_index]);
    color_index++;
    color_index %= 6;
    play_tone(C6, 60);
    while(joystick_pressed==RELEASED) {
        lcd_clear(gameboard);
        lcd_puts(first_row,gameboard);
        event_enabled = 0;
        event_delay_count = 120;
        while(event_enabled == 0) {
            if((JOYSTICK_BUTTON_1 == PRESSED)||(JOYSTICK_BUTTON_2 == PRESSED)) {
                joystick_pressed = PRESSED;
            }
        }
        turn_on_led(colors[color_index]);
        color_index++;
        color_index %= 6;
        lcd_clear(gameboard);
        lcd_goto(ROW_2,gameboard);
        lcd_puts(second_row,gameboard);
        event_enabled = 0;
        event_delay_count = 120;
        while(event_enabled == 0) {
            if((JOYSTICK_BUTTON_1 == PRESSED)||(JOYSTICK_BUTTON_2 == PRESSED)) {
                joystick_pressed = PRESSED;
            }
        }
        turn_on_led(colors[color_index]);
        color_index++;
        color_index %= 6;
    }
}

void concentration_game_end(void) {
    DelayMs(1000);
    if(p1_score>p2_score) {
        end_screen("    WINNER!!","    PLAYER 1");
    } else if(p1_score<p2_score) {
        end_screen("    WINNER!!","    PLAYER 2");
    } else {
        end_screen("   NO WINNER!","      TIED");
    }
    DelayMs(1000);
}

void concentration_title_screen(void) {
    lcd_puts(" Concentration!", scoreboard);
    lcd_puts("Press the button", gameboard);
    lcd_goto(ROW_2, gameboard);
    lcd_puts("    to start", gameboard);
    while(JOYSTICK_BUTTON_1);
    DelayMs(8);
    while(!JOYSTICK_BUTTON_1);
    DelayMs(8);
    lcd_clear(gameboard);
    lcd_clear(scoreboard);
    p1_score = 0;
    p2_score = 0;
    display_concentration_scoreboard();
    display_concentration_gameboard();
}

void display_concentration_scoreboard(void) {
    lcd_puts("     Score:", scoreboard);
    lcd_goto(ROW_2, scoreboard);
    lcd_puts(" P1: ", scoreboard);
    lcd_putch(((p1_score%100)/10)+0x30, scoreboard);
    lcd_putch((p1_score%10)+0x30, scoreboard);
    lcd_puts("  P2: ", scoreboard);
    lcd_putch(((p2_score%100)/10)+0x30, scoreboard);
    lcd_putch((p2_score%10)+0x30, scoreboard);
}

void display_concentration_gameboard(void) {
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

void concentration_gameboard_init(void) {
    selected_tile = 0xFF;
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
    lcd_goto(cursor_pos, gameboard);
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
            turn_on_led(GREEN);
            play_tone(F6SHARP, 15);
            LED_GREEN = 0;
            play_tone(A6, 5);
            turn_on_led(GREEN);
            play_tone(F6SHARP, 15);
            LED_GREEN = 0;
            play_tone(A6, 5);
            //TODO change back
            if((p1_score+p2_score)==2) {
                game_over = 1;
            }
            display_concentration_scoreboard();
            
        } else {
            display_concentration_gameboard();
            turn_on_led(RED);
            play_tone(C2SHARP, 30);
            LED_RED = 0;
            DelayMs(100);
            turn_on_led(RED);
            play_tone(C2SHARP, 30);
            LED_RED = 0;
            visible[get_cursor_index(selected_tile)] = 0xFF;
            visible[get_cursor_index(cursor_pos)] = 0xFF;
            current_char = 0xFF;
            current_player ^= 0x01; //Switch players
        }
        selected_tile = 0xFF;
        joystick_pressed = RELEASED;
        DelayMs(100);
    }
}

void concentration_game_loop(void) {
    //Start a new joystick reading if the previous one has finished
    if(GO == 0) {
        GO = 1;
    }
    if((((JOYSTICK_BUTTON_1 == PRESSED)&&(current_player == PLAYER_1))||
            ((JOYSTICK_BUTTON_2 == PRESSED)&&(current_player == PLAYER_2)))
            &&(joystick_pressed == RELEASED)) {
        DelayMs(6);
        joystick_pressed = PRESSED;
        if(visible[get_cursor_index(cursor_pos)] == 0xFF) {
            visible[get_cursor_index(cursor_pos)] = board[get_cursor_index(cursor_pos)];
            current_char = board[get_cursor_index(cursor_pos)];
            check_for_match(current_player);
            display_concentration_gameboard();
        }
    } else if((((JOYSTICK_BUTTON_1 == RELEASED)&&(current_player == PLAYER_1))||
            ((JOYSTICK_BUTTON_2 == RELEASED)&&(current_player == PLAYER_2)))) {
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
        if(event_enabled) {
            update_cursor(30, LEFT);
        }
    } else if(joystick_x_pos < 100) {
        if(event_enabled) {
            update_cursor(30, RIGHT);
        }
    } else if(joystick_x_pos > 800) {
        if(event_enabled) {
            update_cursor(60, LEFT);
        }
    } else if(joystick_x_pos < 200) {
        if(event_enabled) {
            update_cursor(60, RIGHT);
        }
    } else if(joystick_y_pos == 1021) {
        if(event_enabled) {
            update_cursor(100, VERTICAL);
        }
    } else if(joystick_y_pos == 0) {
        if(event_enabled) {
            update_cursor(100, VERTICAL);
        }
    } else {
        event_enabled = 1;
    }
}

void update_cursor(char move_delay_count, char direction) {
    event_enabled = 0;
    event_delay_count = move_delay_count;
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

void play_tone(unsigned int tone_period, char duration_8ms) {
    tone_delay = tone_period;
    CCPR2 = TMR1+tone_period;
    CCP2IF = 0;
    CCP2IE = 1;
    event_delay_count = duration_8ms;
    event_enabled = 0;
    while(event_enabled == 0);
    CCP2IE = 0;
}

//Initialization functions

void time_init(void) {
    event_delay_count = 255;
    event_enabled = 1;
    PR2 = 250; //TMR2IF triggered after 8ms
    TMR2IF = 0;
    TMR2IE = 1;
    T2CON = 0x56;   //Post-scaler = 1:10, pre-scaler = 1:16,TMR2ON = 1
    
    CCP1M3 = 1;
    CCP1M2 = 0;
    CCP1M1 = 1;
    CCP1M0 = 0;
    TMR1CS = 0;
    T1CKPS0 = 0;
    T1CKPS1 = 0;
    TMR1GE = 0;
    TMR1ON = 1;
    
    CCP2M3 = 1;
    CCP2M2 = 0;
    CCP2M1 = 0;
    CCP2M0 = 0;
    TRISC &= 0xFD;
}

void joystick_init(void) {
    joystick_x_pos = 512;
    joystick_y_pos = 512;
    PORTB = 0;
    nRBPU = 0;
    WPUB = 0x24;
    TRISB = 0x3F;
    ANSELH = 0x1E;
    GIE = 1;
    PEIE = 1;
    ADIF = 0;
    ADIE = 1;
    ADCON1 = 0x80;  //ADFM = 1, VCFG = 00
    ADCON0 = 0xA5;  //ADCS = 10 , CHS = 9, GO = 0, ADON = 1
    joystick_pressed = RELEASED;
}

void __interrupt() interrupt_handler(void) {
    if((CCP2IF==1) && (CCP2IE==1)) {
        CCPR2 += tone_delay;
        if(CCP2M0 == 1) {
            CCP2M0 = 0;
        } else {
            CCP2M0 = 1;
        }
        CCP2IF = 0;
    }
    if(ADIF) {
        char current_analog = ADCON0&0x3C;
        ADCON0 &= 0xC3; //Clear analog select
        if(((current_analog) == JX1)||((current_analog) == JX2)) {
            joystick_x_pos = (((int)ADRESH)<<8)+ADRESL;
            switch(current_player) {
                case PLAYER_1:
                    ADCON0 |= JY1;
                    break;
                case PLAYER_2:
                    ADCON0 |= JY2;
                    break; 
            }
        } else {
            joystick_y_pos = (((int)ADRESH)<<8)+ADRESL;
            switch(current_player) {
                case PLAYER_1:
                    ADCON0 |= JX1;
                    break;
                case PLAYER_2:
                    ADCON0 |= JX2;
                    break; 
            }
        }
        ADIF = 0;
    }
    if(TMR2IF) {
        event_delay_count--;
        if(event_delay_count == 0) {
            event_enabled = 1;
        }
        TMR2IF = 0;
    }
}
