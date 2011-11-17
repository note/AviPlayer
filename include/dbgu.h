#ifndef __DBGU_H__
#define __DBGU_H__

#include "AT91SAM9260.h"

void debug_init_default(void);
void debug_init(AT91PS_DBGU paramDbgu, AT91PS_PIO paramPio, uint32_t pinMask, uint8_t periph);
void debug_chr(char chr);
char debug_inkey(void);
char debug_waitkey(void);
void debug_ascii(uint8_t b);
void debug_msg(char *str);
void debug_txt(char *str);
void debug_txt_limit(char *str, uint8_t len);
void debug8_t(uint8_t val);
void debug16_t (uint16_t val);
void debug32_t (uint32_t val);
void debug_value( const char* s, uint32_t value);
void debug_array( void *array, uint16_t size );
void debug_dump(void *address, uint16_t len);
uint8_t debug_input_string(char * pBuf, uint8_t BufSize);
uint32_t debug_input_hex(uint32_t input);

/* added from FreeRTOS */
int debug_test(void);

#endif


