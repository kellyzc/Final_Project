//Testing Functions
//TODO delete later
#define JOYSTICK_BUTTON RB5
#define PRESSED 0

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