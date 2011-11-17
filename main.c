#include "common.h"
#include "lcd.h"
#include "term_io.h"
#include "sd.h"
#include "avi.h"

static void (*pt2programStart)(void) = 0x20000000;
static FATFS fs __attribute__((section(".buffers")));
static FIL file __attribute__((section(".buffers")));

void init(void)
{
	// enable peripheral clock for PIO controller
	AT91C_BASE_PMC->PMC_PCER = (1<<AT91C_ID_PIOA) | (1<<AT91C_ID_PIOB) | (1<<AT91C_ID_PIOC);
	LEDS_INIT;
	debug_init_default();
	
	debug_msg("AT91SAM9260 Empty Project");
	xprintf("cos tam");
	
}

void restartIfNeeded(void){
	if(SW4_PRESSED){
		debug_msg("program restart");
		pt2programStart();
	}
}

void showInt16(uint16_t * b){
	for(int i=0; i<16; ++i)
		xprintf("%d,", (*b >> 15-i) & 1);
	xprintf("\n");
}



int main( void )
{
	init();
	
	debug_msg("entering main loop");
		
	if(sdInit()==OK) debug_msg("sdInit OK!"); else debug_msg("sdInit FAILED!");
	
	FRESULT res = f_mount(0,&fs);;						/* Mount/Unmount a logical drive */
	xprintf("montowanie %d\n", res);
	res = f_open (&file, "final.avi", FA_READ);			/* Open or create a file */
	xprintf("otwarcie pliku %d\n", res);
	
	lcdInit();
	while(1) playMovie(&file);
		
	/*for(int i=0; i<LCD_Y_SIZE/3; ++i)
		for(int j=0; j<LCD_X_SIZE; ++j){
			img[i][j] = 0xF800; //red square in the corner (which one?)0xF800
		}
		
	for(int i=LCD_Y_SIZE/3; i<2*LCD_Y_SIZE/3; ++i)
		for(int j=0; j<LCD_X_SIZE; ++j){
			img[i][j] = 0xF000; //red square in the corner (which one?)0x07C0;
		}
	for(int i=2*LCD_Y_SIZE/3; i<LCD_Y_SIZE; ++i)
		for(int j=0; j<LCD_X_SIZE; ++j){
			img[i][j] = 0x7800; //red square in the corner (which one?)
		}
		
	xprintf("czerw: ");
	showInt16(&img[0][0]);
	
	xprintf("ziel: ");
	showInt16(&img[LCD_Y_SIZE/3][0]);
	
	xprintf("niebieski: ");
	showInt16(&img[2*LCD_Y_SIZE/3][0]);
*/
	
	for(;;)
	{
		LED_Y_TOGGLE;
		delay_ms(500);
		restartIfNeeded();
			
		
	}
	return(0);
}

