#include "common.h"
#include "lcd.h"



#define LCD_SPI			AT91C_BASE_SPI1
#define LCD_SPI_ID		AT91C_ID_SPI1
#define LCD_PIO			AT91C_BASE_PIOB
#define LCD_SPI_ABSR	PIO_ASR
#define LCD_SPI_MASK	( AT91C_PB1_SPI1_MOSI | AT91C_PB2_SPI1_SPCK | AT91C_PB3_SPI1_NPCS0 )
#define LCD_RST_MASK	AT91C_PIO_PB0
#define LCD_PIO_MASK	( LCD_RST_MASK )

#define LCD_RST_0		LCD_PIO->PIO_CODR = LCD_RST_MASK
#define LCD_RST_1		LCD_PIO->PIO_SODR = LCD_RST_MASK

#define DISON       0xAF
#define DISOFF      0xAE
#define DISNOR      0xA6
#define DISINV      0xA7
#define SLPIN       0x95
#define SLPOUT      0x94
#define COMSCN      0xBB
#define DISCTL      0xCA
#define PASET       0x75
#define CASET       0x15
#define DATCTL      0xBC
#define RGBSET8     0xCE
#define RAMWR       0x5C
#define RAMRD       0x5D
#define PTLIN       0xA8
#define PTLOUT      0xA9
#define RMWIN       0xE0
#define RMWOUT      0xEE
#define ASCSET      0xAA
#define SCSTART     0xAB
#define OSCON       0xD1
#define OSCOFF      0xD2
#define PWRCTR      0x20
#define VOLCTR      0x81
#define VOLUP       0xD6
#define VOLDOWN     0xD7
#define TMPGRD      0x82
#define EPCTIN      0xCD
#define EPCOUT      0xCC
#define EPMWR       0xFC
#define EPMRD       0xFD
#define EPSRRD1     0x7C
#define EPSRRD2     0x7D
#define NOP         0x25


#define	NOPP		0x00
#define	BSTRON		0x03
#define SLEEPIN     0x10
#define	SLEEPOUT	0x11
#define	NORON		0x13
#define	INVOFF		0x20
#define INVON      	0x21
#define	SETCON		0x25
#define DISPOFF     0x28
#define DISPON      0x29
#define CASETP      0x2A
#define PASETP      0x2B
#define RAMWRP      0x2C
#define RGBSET	    0x2D
#define	MADCTL		0x36
#define	COLMOD		0x3A
#define DISCTR      0xB9
#define	EC			0xC0

#define EPSON		1 //EPSON driver: True or False? ;-)
#define PHILIPS		0 //PHILIPS driver: True or False? ;-)
#define ENDPAGE     132
#define ENDCOL      132


static uint16_t *lcdShadow;



static void lcdSendData(uint8_t data);
static void lcdSendCommand(uint8_t cmd);
static void lcdPinsInit(void);
static void lcdSpiInit(void);



uint32_t lcdIOUpdate(void *shadow)
{
	uint16_t *rgb565 = shadow;
	//debug_txt("refsh*");
	lcdSendCommand(PASET);   // page start/end ram
	lcdSendData(0);
	lcdSendData(ENDPAGE-1);
	
	lcdSendCommand(CASET);   // column start/end ram
	lcdSendData(0);
	lcdSendData(ENDCOL-1);
	
	lcdSendCommand(RAMWR);    // write
	
	uint32_t i = ENDPAGE*ENDCOL/2;
	while(i--)
	{
		//*rgb565 = 0x03E0;
		uint8_t next3[3];
		next3[0] = (((*rgb565)>>8) & 0xF0) | ( ((*rgb565)>>7) & 0x0F );	//R1 R1 R1 R1 G1 G1 G1 G1
		next3[1] = ((*rgb565)<<3) & 0xF0;	//B1 B1 B1 B1 00 00 00 00
		rgb565++;
		//*rgb565 = 0x03E0;
		next3[1] |= ((*rgb565)>>12) & 0x0F;	//00 00 00 00 R2 R2 R2 R2
		next3[2] = (((*rgb565)>>3)&0xF0) | (((*rgb565)>>1)&0x0F);	//G2 G2 G2 G2 B2 B2 B2 B2
		lcdSendData(next3[0]);
		lcdSendData(next3[1]);
		lcdSendData(next3[2]);
		rgb565++;
	}
	
	return(OK);
}



static void lcdSendData(uint8_t data)
{
	uint16_t temp = 0x100 | data;
	while((LCD_SPI->SPI_SR & AT91C_SPI_TDRE)==0);
	LCD_SPI->SPI_TDR = temp;
}


static void lcdSendCommand(uint8_t cmd)
{
	uint16_t temp = cmd;
	while((LCD_SPI->SPI_SR & AT91C_SPI_TDRE)==0);
	LCD_SPI->SPI_TDR = temp;
}


static uint32_t lcdIOInit(void)
{
	//initialize pio controller for LCD
	lcdPinsInit();
	
	//initialize SPI interface for LCD
	//LCD uses SPI1 as the only slave
	lcdSpiInit();
	
	LCD_RST_0;
	delay_ms(100);
	LCD_RST_1;
	delay_ms(100);
	
	// Display vontrol
	lcdSendCommand(DISCTL);
	lcdSendData(0x0C);   	// 12 = 1100 - CL dividing ratio [don't divide] switching period 8H (default)
	lcdSendData(0x20);
	lcdSendData(0x02);
	
	lcdSendCommand(COMSCN);
	lcdSendData(0x01);  // Scan 1-80
	
	// Internal oscilator ON
	lcdSendCommand(OSCON);
	
	// wait aproximetly 100ms
	delay_ms(200);
	
	// Sleep out
	lcdSendCommand(SLPOUT);
	
	// Voltage control
	lcdSendCommand(VOLCTR);
	lcdSendData(0x1F); // middle value of V1
	lcdSendData(0x03); // middle value of resistance value
	
	// Temperature gradient
	lcdSendCommand(TMPGRD);
	lcdSendData(0x00); // default
	
	// Power control
	lcdSendCommand(PWRCTR);
	lcdSendData(0x0f);   // referance voltage regulator on, circuit voltage follower on, BOOST ON
	
  // Normal display
  lcdSendCommand(DISNOR);

  // Inverse display
  lcdSendCommand(DISINV);

  // Partial area off
  lcdSendCommand(PTLOUT);

	
	// Data control
	lcdSendCommand(DATCTL);
	lcdSendData(0x00); // all inversions off, column direction
	lcdSendData(0x00);   	// normal RGB arrangement
	lcdSendData(0x02);
	
	lcdSendCommand(NOP);  	// nop(EPSON)
	lcdSendCommand(DISON);
	
	return(OK);
}


static void lcdPinsInit(void)
{
	//disable PIO controller on LCD LCD_SPI pins
	LCD_PIO->PIO_PDR = LCD_SPI_MASK;
	
	//select A or B peripheral
	LCD_PIO->LCD_SPI_ABSR = LCD_SPI_MASK;
	
	//enable pio controller on LCD RESET pin
	LCD_PIO->PIO_PER = LCD_PIO_MASK;
	
	//set RESET to hi level
	LCD_PIO->PIO_SODR = LCD_PIO_MASK;
	
	//enable output on these bitz
	LCD_PIO->PIO_OER = LCD_PIO_MASK;
	
	delay_ms(100);
	
	LCD_RST_0;
	
	delay_ms(100);
	
	LCD_RST_1;
	
	delay_ms(100);
}


static void lcdSpiInit(void)
{
	//enable LCD_SPI clock
	AT91C_BASE_PMC->PMC_PCER = ( 1 << LCD_SPI_ID );
	
	//LCD_SPI enable and reset
	LCD_SPI->SPI_CR = AT91C_SPI_SWRST;
	
	const uint8_t DELAY_BETWEEN_CHIP_SELECTS = 0xFF;
	const uint32_t DLYBCS_MASK	= (uint32_t)DELAY_BETWEEN_CHIP_SELECTS << 24;
	
	//LCD_SPI Mode Register: Fault detection disable, fixed periph. chip select
    LCD_SPI->SPI_MR  = DLYBCS_MASK | AT91C_SPI_MSTR | AT91C_SPI_MODFDIS;
	
	LCD_SPI->SPI_CR = AT91C_SPI_SPIEN;
	
	const uint8_t BAUD_DIVIDER = 0x10;
	const uint8_t DELAY_BEFORE_SPCK = 0;
	const uint8_t DELAY_BETWEEN_CONSECUTIVE_TRANSFERS = 0;
	
	//masks for SPI_CSR registers
	const uint32_t SCBR_MASK = (uint32_t)BAUD_DIVIDER << 8;
	const uint32_t DLYBS_MASK = (uint32_t)DELAY_BEFORE_SPCK << 16;
	const uint32_t DLYBCT_MASK = (uint32_t)DELAY_BETWEEN_CONSECUTIVE_TRANSFERS << 24;
	
	//initialize NPCS0 to "Mode 0"
	LCD_SPI->SPI_CSR[0] = AT91C_SPI_BITS_9 | SCBR_MASK | DLYBS_MASK | DLYBCT_MASK | AT91C_SPI_CPOL;
}



static void lcdWriteShadow(uint16_t *img)
{
	memcpy(lcdShadow,img,LCD_X_SIZE*LCD_Y_SIZE*sizeof(uint16_t));
}


//converts format from (xrrrrrgggggbbbbb from sd) to (rrrrrgggggXbbbbb driver compatible!!!)
void lcdWriteRgb555(uint16_t *rgb555)
{
	uint32_t i = LCD_X_SIZE*LCD_Y_SIZE;
	lcdShadow = rgb555;
	while(i--)
	{
		int32_t col = *rgb555;
		int32_t rMask = (col << 1) & 0xF800;
		int32_t gMask = (col << 1) & 0x07C0;
		int32_t bMask = col & 0x001F;
		*rgb555++ = rMask | gMask | bMask;
	}
}





//copies shadow to LCD display
void lcdUpdate(void)
{
	lcdIOUpdate((void*)lcdShadow);
}



void lcdInit(void)
{
	lcdIOInit();
	lcdUpdate();
}

