// Created by Hi-Tech Software Modified by Jianjian Song to work with PIC16F887, August 13, 2009
// Modified by Keith Hoover for easier use in ECE230, July 15, 2011
// Modified by Chris Miller for easier use in ECE230, September, 2014
// Modified by Zach Kelly and Tanner Brammeier 1/29/20
/*
 *	"lcd4bits.c" LCD Display Panel Driver Routines
 *  Reference:  Refer to the Optrex LCD User Manual for all details and command formats
 *              used in these routines "Optrex LCD Manual Dmcman_full-user manual.pdf" 
 *
 *	All necessary LCD and delay functions have been bundled into this file (lcd8bit.c),
 *  and the necessary function prototype statements are bundled into the 
 *  companion header file, lcd4bit.h.
 *
 *	This code will interface to a wide variety of B/W LCD Display Controllers
 *	like the Hitachi HD44780. It uses the 4-bit transfer mode, and it assumes that the
 *      pins of PORT D (RD7:4) are connected to the LCD panel, with
 *	the hardware connected as follows, assuming the standard 16 pin LCD pinout:
 
 *    GND to VSS Pin of LCD Pin 1
 *    +5V to VDD Pin of LCD Pin 2
 *     ~  to V0 Pin of LCD Pin 3
 *    GND to R/W* Pin of LCD Pin 5 (This means that the software can only write to the LCD panel, and never
 *                                  read from it.)
 *	  RD0:7 are connected to the LCD data bits 0-7 LCD Pins 7-14
 *	  RC4 is connected to the LCD RS input (register select) LCD Pin 4
 *  	  RC5 is connected to the LCD EN bit (enable)  LCD Pin 6
 *	
 *	The available routines in this file are:
 *
 *    1.  lcd_init( ) 
 *        Always call lcd_init() first, which follows the manufacturer's
 *        directions for initializing the LCD display panel into 8-bit transfer mode.
 *        Then you may call any of the other routines, as needed.  Note that this
 *        initialization routine also makes RD7:0 all outputs, as required to drive
 *		  the LCD panel connected to RD7:0.  It also selects 8 MHz internal clock.
 *
 *    2.  lcd_clear( )
 *        Clears LCD display and homes the cursor
 *
 *    3.  lcd_puts(const char s*)
 *        writes a constant character string to the lcd panel
 *
 *    4.  lcd_putch(char s)
 *        writes a single character to lcd panel
 *
 *    5.  lcd_goto(unsigned char pos)
 *        Moves cursor to desired position.  For 16 x 2 LCD display panel, 
 *        the columns of Row 1 are 0x00....0x10
 *        the columns of Row 2 are 0x40....0x50
 *
 *    6.  DelayMs(unsigned int NrMs)
 *		  Delays for NrMs milliseconds.
 *
 */

#include	<xc.h>

#include "lcd4bits.h"

#define CMD_MODE                0x00        //(0 for command mode)
#define DTA_MODE                0x10        //(1 for data mode)
                                            //RS is bit 4 of an LCD port
//rename RD2 	-LCD command/data 
#define LCD_EN                  0x20        //Enable is bit 5 of an LCD port
//rename RD3 	-LCD Enable (clocks data into LCD on falling edge)
#define LCDCMD_ClearDisplay     0x01    //clear display: clear, move cursor home
#define LCDCMD_EMS              0x06    //entry mode set: auto increment cursor after each char sent
#define LCDCMD_DisplaySettings  0x0C    //display ON/OFF control: display on, cursor off, blink off
#define LCDCMD_FunctionSet      0x28    //function set: 4-bit mode, 2 lines, 5x7 dots
#define LCDCMD_CursorShiftL     0x10    //Shift cursor to the left by 1

void tmr0_init() {
    // T0CS = 0 -- Fosc/4 clock source
    // PSA = 0 -- prescalar set to TMR0
    // PS = 0 -- 1:256
    OPTION_REG &= 0xC0;
    // PS = 4 -- 1:32
    OPTION_REG |= 0x04;
}

/*
 * Delay for indicated number of milliseconds
 *  (256 - 9)*8 + 24 instruction cycles => 2000 instruction cycles (1ms)
 *  assumes PS 1:8, 8MHz clock
 */
void DelayMs(unsigned int millis) {
    while (millis != 0) {
        TMR0 = 100;
        T0IF = 0;
        while(T0IF == 0);
        millis--;
    }
}

/*
 * Provides delay of ~20us
 *  assumes 20MHz clock
 */
void Delay20us() {
    unsigned char next;
    for (next = 0; next < 8; next++);
}

/*
 * lcd_write function ---writes a byte to the LCD in 4-bit mode.
 * Note that the "mode" argument is set to either CMD_MODE (=0) or DTA_MODE (=1), 
 * so that the LCD panel knows whether an instruction byte is being written to it 
 * or an ASCII code is being written to it that is to be displayed on the panel.
 */ 
void lcd_write(unsigned char mode, unsigned char CmdChar, char *port) {
    *port = (mode|((CmdChar>>4)+LCD_EN));    //Sets port to send lower nibble, mode, and enable
    Delay20us();
    *port = (*port)&(~LCD_EN);  //Clears enable
    //TODO make sure data is upper 4 bits
    *port = (mode|((CmdChar&0x0F)+LCD_EN));  //Sets port to send upper nibble, mode, and enable
    Delay20us();
    *port = (*port)&(~LCD_EN);  //Clears enable
}

/*
 * Clear the LCD and go to home position
 */
void lcd_clear(char *port) {
    lcd_write(CMD_MODE, LCDCMD_ClearDisplay, port);
    DelayMs(2);
}

/* Write a string of chars to the LCD */
void lcd_puts(const char *string, char *port) {
    while (*string != 0) { // Last character in a C-language string is alway "0" (ASCII NULL character)
        lcd_write(DTA_MODE, *string++, port);
    }
}

/* Write one character to the LCD */
void lcd_putch(char character, char *port) {
    lcd_write(DTA_MODE, character, port);
}

/*
 * Moves cursor to desired position.  
 * For 16 x 2 LCD display panel, 
 *     the columns of Row 1 are 0x00....0x10
 *     the columns of Row 2 are 0x40....0x50
 */
void lcd_goto(unsigned char position, char *port) {
    lcd_write(CMD_MODE, 0x80 + position, port); // The "cursor move" command is indicated by MSB=1 (0x80)
    // followed by the panel position address (0x00- 0x7F)
    DelayMs(2);
}

/*
 * Initialize the LCD - put into 8 bit mode
 */
void lcd_init(char *port) //See Section 2.2.2.2 of the Optrex LCD DMCman User Manual
{
    tmr0_init();
    *port = 0;
    DelayMs(15); // wait 15mSec after power applied,
    lcd_write(CMD_MODE, LCDCMD_FunctionSet, port); // function set: 4-bit mode, 2 lines, 5x7 dots
    lcd_write(CMD_MODE, LCDCMD_DisplaySettings, port); // display ON/OFF control: display on, cursor off, blink off
    lcd_clear(port); // Clear screen
    lcd_write(CMD_MODE, LCDCMD_EMS, port); // Set entry Mode
}
