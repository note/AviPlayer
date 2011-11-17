#ifndef __LCD_H__
#define __LCD_H__


#define	LCD_X_SIZE					132
#define	LCD_Y_SIZE					132
#define	LCD_SHADOW_SIZE				(132*132)

#define LCD_COLOR_RED				0xF800
#define LCD_COLOR_GREEN				0x07E0
#define LCD_COLOR_BLUE				0x001F


void lcdWriteRgb555(uint16_t *rgb555);
void lcdUpdate(void);
void lcdInit(void);



#endif

