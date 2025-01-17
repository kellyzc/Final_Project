/*
 *	LCD interface header file
 *	See lcd8bits.c for more info
 */

extern void Display( unsigned int );

/* delay for indicated number of milliseconds */
extern void DelayMs(unsigned int);

/* write a byte to the LCD in 8 bit mode */
extern void lcd_write(unsigned char, unsigned char, char *c);

/* Clear and home the LCD */
extern void lcd_clear(char *);

/* write a string of characters to the LCD */
extern void lcd_puts(const char * s, char *c);

/* Go to the specified position */
extern void lcd_goto(unsigned char pos, char *c);
	
/* intialize the LCD - call before anything else */
extern void lcd_init(char *);

extern void lcd_putch(char, char *c);

extern void lcd_set_custom_char(const char *d, char, char* p);
