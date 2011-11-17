#include "common.h"
#include "dbgu.h"

//set to nonzero for full implementation (more code)
#define FULL						1

//which port do you want to use?
#define USE_DBGU
//#define USE_USART0
//#define USE_USART1


#if defined USE_DBGU
	#define DBGU_BASE				AT91C_BASE_DBGU
	#define DBGU_PIO				AT91C_BASE_PIOB
	#define DBGU_RX_MASK			AT91C_PB14_DRXD
	#define DBGU_TX_MASK			AT91C_PB15_DTXD
	#define DBGU_PERIPH				0
#elif defined USE_USART0
	#define DBGU_BASE				AT91C_BASE_US0
	#define DBGU_ID					AT91C_ID_US0
	#define DBGU_PIO				AT91C_BASE_PIOB
	#define DBGU_RX_MASK			AT91C_PB4_TXD0
	#define DBGU_TX_MASK			AT91C_PB5_RXD0
	#define DBGU_PERIPH				0
#elif defined USE_USART1
	#define DBGU_BASE				AT91C_BASE_US1
	#define DBGU_ID					AT91C_ID_US1
	#define DBGU_PIO				AT91C_BASE_PIOB
	#define DBGU_RX_MASK			AT91C_PB6_TXD1
	#define DBGU_TX_MASK			AT91C_PB7_RXD1
	#define DBGU_PERIPH				0
#else
	#error "Debug serial port name: no match!"
#endif

#define DBGU_BAUDRATE			115200
AT91PS_DBGU 					pDbgu;

//private tools
static uint8_t debug_dec2hex(uint8_t val);
#if FULL
static uint8_t debug_char_is_hex(uint8_t chr);
static uint8_t debug_hex2dec(uint8_t val);
#endif

void debug_init_default(void)
{
	debug_init((AT91PS_DBGU)DBGU_BASE, DBGU_PIO, DBGU_RX_MASK | DBGU_TX_MASK, DBGU_PERIPH);
}

//initializes pDbgu port for simple UART operation on 115200 baud (platform dependent)
//requires:
//paramDbgu: base address of DBGU,
//paramPio: base address of PIO pins used for RX and TX signals
//pinMask: mask for RX and TX pins
//periph: zero for peripheral A, non-zero for peripheral B
void debug_init(AT91PS_DBGU paramDbgu, AT91PS_PIO paramPio, uint32_t pinMask, uint8_t periph)
{
	//copy base address of USART to the global variable pDbgu
	pDbgu = paramDbgu;
	//disable PIO on pDbgu pins
	paramPio->PIO_PDR = pinMask;
	if(periph==0)
	{
		//select peripheral A on pDbgu pins
		paramPio->PIO_ASR = pinMask;
	}
	else
	{
		//select peripheral B on pDbgu pins
		paramPio->PIO_BSR = pinMask;
	}
	
	//enable selected USART's clock (DBGU is clocked continuously)
	#if !defined USE_DBGU
	AT91C_BASE_PMC->PMC_PCER = (1<<DBGU_ID);
	#endif
	
	//reset and disable transmitter and receiver
	pDbgu->DBGU_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS;
	//set mode
	pDbgu->DBGU_MR = AT91C_US_CHRL_8_BITS | AT91C_US_PAR_NONE;
	//set baud rate
	pDbgu->DBGU_BRGR = AT91F_US_Baudrate(MCK,DBGU_BAUDRATE);
	//enable transmitter and receiver
	pDbgu->DBGU_CR = AT91C_US_RXEN | AT91C_US_TXEN;
}


//send chr via pDbgu UART (platform dependent)
void debug_chr(char chr)
{
	while ( !(pDbgu->DBGU_CSR & AT91C_US_TXRDY) );	//Wait for empty TX buffer
	pDbgu->DBGU_THR = chr;								//Transmit char
}

//returns ascii value of last char received
//returns 0 if no char was received since last debug_inkey call
//(platform dependent)
char debug_inkey(void)
{
	if( pDbgu->DBGU_CSR & AT91C_US_RXRDY )
	{
		return( pDbgu->DBGU_RHR );
	}
	else
	{
		return(0);
	}
}

//halts program/task execution until char is received
//(platform dependent)
char debug_waitkey(void)
{
	while( (pDbgu->DBGU_CSR & AT91C_US_RXRDY) == 0 );
	return( pDbgu->DBGU_RHR );
}


//platform independent funcs


//prints text starting at str
//adds new line first
void debug_msg(char *str)
{
	debug_chr('\r');
	debug_chr('\n');
	debug_txt(str);
}

//prints text starting at str
void debug_txt(char *str)
{
	while(*str) debug_chr(*str++);
}

#if FULL
//prints text starting at str
//prints exactly len chars
void debug_txt_limit(char *str, uint8_t len)
{
	while(len)
	{
		debug_ascii(*str);
		str++;
		len--;
	}
}
#endif


#if FULL
//sends char b over pDbgu UART. Replaces values that can change cursor pos. on terminal
void debug_ascii(uint8_t b)
{
	switch(b)
	{
		case 0:
		{
			debug_chr('.');	//replace 0 with dot
			break;
		}
		case 8:
		case 9:
		case 10:
		case 13:
		{
			//avoid other chars that modify terminal cursor
			//replace them with space
			debug_chr(' ');
			break;
		}
		default:
		{
			debug_chr(b);
		}
	}//switch(chr)
}
#endif

//displays hex number from val in format FF
void debug8_t(uint8_t val)
{
	debug_chr(debug_dec2hex(val>>4));
	debug_chr(debug_dec2hex(val&0x0F));
}


//displays hex number from val in format FFFF
void debug16_t (uint16_t val)
{
	debug8_t(val>>(1*8));
	debug8_t(val>>(0*8));
}

//displays hex number from val in format FFFFFFFF
void debug32_t (uint32_t val)
{
	debug8_t(val>>(3*8));
	debug8_t(val>>(2*8));
	debug8_t(val>>(1*8));
	debug8_t(val>>(0*8));
}



//converts decimal digit from into ascii hex digit
//returns ascii char of hex digit
static uint8_t debug_dec2hex(uint8_t val)
{
	if(val<10) val+='0';
	else val=val-10+'A';
	return(val);
}

//prints text from *s and value
void debug_value( const char* s, uint32_t value)
{
    debug_txt("\n\r");
    debug_txt((char*)s);
    debug_txt("0x");
    if(value>0x00FFFFFF) debug8_t(value>>(3*8));
    if(value>0x0000FFFF) debug8_t(value>>(2*8));
    if(value>0x000000FF) debug8_t(value>>(1*8));
    debug8_t(value>>(0*8));
}

//prints array contents byte-by-byte in ASCII hex format
//provide starting address in *array and number of items to display in size
void debug_array( void *array, uint16_t size )
{
	uint8_t *s = array;
    debug_txt("\n\r");
	debug_txt("Length="); debug16_t(size); debug_txt(", Contents:\n\r");
	do
	{
		debug_chr(' '); debug8_t(*s++);
		size--;
	}while(size);
    debug_txt(" end");
}

#if FULL
void debug_dump(void *address, uint16_t len)
{
	uint8_t *buf = address;
	const uint16_t bytesInLine = 16;
	const uint16_t spaceBetweenDumpAndASCII = 4;
	uint16_t i, counter=len;
	
	//header
	debug_msg("Memory dump, starting address = "); debug32_t((uint32_t)buf); debug_txt(", length = "); debug16_t(len);
	debug_msg("Address  Offs:   Data");
	//contents
	while(1)
	{
		//insert last line (may be shorter than full line)
		if(counter < bytesInLine)
		{
			debug_txt("\r\n"); debug32_t((uint32_t)buf); debug_txt(" "); debug16_t(len-counter); debug_txt(":   ");
			
			//contents in hex
			for(i=0;i<bytesInLine;i++)
			{
				if(i<counter)
				{
					debug8_t(buf[i]);
					debug_chr(' ');
				}
				else
				{
					debug_txt("   ");
				}
				if(i%8==7) debug_chr(' ');
			}
			
			//space
			for(i=0;i<spaceBetweenDumpAndASCII;i++)
			{
				debug_chr(' ');
			}
			
			//contents in ASCII
			for(i=0;i<bytesInLine;i++)
			{
				if(i<counter)
				{
					debug_ascii(buf[i]);
				}
				else
				{
					debug_chr(' ');
				}
			}
			
			break;
		}
		
		debug_txt("\r\n"); debug32_t((uint32_t)buf); debug_txt(" "); debug16_t(len-counter); debug_txt(":   ");
		
		for(i=0;i<bytesInLine;i++)
		{
			debug8_t(buf[i]); debug_chr(' ');
			if(i%8==7) debug_chr(' ');
		}
		
		//space
		for(i=0;i<spaceBetweenDumpAndASCII;i++)
		{
			debug_chr(' ');
		}
		
		//contents in ASCII
		for(i=0;i<bytesInLine;i++)
		{
			debug_ascii(buf[i]);
		}
		
		buf += bytesInLine;
		if(counter >= bytesInLine)
		{
			counter -= bytesInLine;
		}
		
		if(counter == 0) break;
		
	}	//while(counter)
	//footer
	debug_msg("End of dump, last byte @ address = "); debug32_t((uint32_t)buf);
}
#endif


#if FULL
//input:
//	BufSize - for buffer limitation
//	pBuf - pointer to string buffer
//returns:
//	length of obtained text (0 means no text)
uint8_t debug_input_string(char * pBuf, uint8_t BufSize)
{
	enum
	{
		BACKSPACE = 8,
		RET = 13,
		ESC = 27
	};
	
	uint8_t Cursor=0;
	uint8_t rxedChar;
	pBuf[0]=0;
	
	for(;;)
	{
		rxedChar = debug_inkey();
		if(rxedChar)
		{
			switch(rxedChar)
			{
				case BACKSPACE:
				{
					if(Cursor)
					{
						Cursor--;
						pBuf[Cursor]=0;
						debug_chr(rxedChar);
					}
					break;
				}
				case RET:
				{
					pBuf[Cursor]=0; //mark end of string
					return(Cursor);
				}
				case ESC:
				{
					return(0); //return zero length
				}
				default:
				{
					pBuf[Cursor]=rxedChar;
					if(Cursor<(BufSize-1))
					{
						Cursor++;
						debug_chr(rxedChar);
					}
				}
			}//switch(rxedChar)
		}//if(rxedChar)
		//vTaskDelay(REFRESH_DELAY);
	}//for(;;)
	
	return(0);
}


//returns 1 if chr is ascii HEX number else returns zero
static uint8_t debug_char_is_hex(uint8_t chr)
{
	if( ((chr>=0x30)&&(chr<=0x39)) || ( (chr>='a')&&(chr<='f') ) || ( (chr>='A')&&(chr<='F') ) ) return(1); else return(0);
}


//provide pointer to hext number in ASCII format *pBuf
//returns value of ascii number in buffer
uint32_t debug_ascii_to_int_hex(char *pBuf)
{
	uint32_t output=0;
	uint8_t pos=strlen(pBuf);
	for(uint8_t i=0;i<8;i++)
	{
		if(pos) pos--; else return(output);
		output+=((debug_hex2dec(pBuf[pos]))<<i*4);
	}
	return(output);
}


//returns input value on fail (no change) and new value on OK
uint32_t debug_input_hex(uint32_t input)
{
	const uint8_t BUFSIZE = 10;
	uint32_t Value=0;
	uint8_t SizeObtained=0;
	
	//allocate memory
	//uint8_t *pBuf;
	//pBuf = (uint8_t *) pvPortMalloc( BUFSIZE*sizeof(uint8_t) );
	uint8_t pBuf[BUFSIZE];
	for(uint8_t i=0;i<BUFSIZE;i++) pBuf[i]=0;
	
	//get string from terminal
	SizeObtained = debug_input_string((char*)pBuf, BUFSIZE);
	
	//check if length > 0
	if(SizeObtained==0) return(input);
	
	//scan buffer if contains digits only
	for(uint8_t i=0;i<SizeObtained;i++)
	{
		if(!debug_char_is_hex(pBuf[i])) return(input);
	}
	
	//convert number string to integer
	Value = debug_ascii_to_int_hex((char*)pBuf);
	
	//free allocated memory region
	//vPortFree(pBuf);
	
	//confirm a new value
	//debug_value("using value = ",Value);
	
	//return a new value
	return(Value);
}

//converts hex digit in ASCII format (val) into integer (return)
static uint8_t debug_hex2dec(uint8_t val)
{
	if((val>='a')&&(val<='f'))
		return(val+10-'a');
	if((val>='A')&&(val<='F'))
		return(val+10-'A');
	return(val-'0');
}

#endif
