#include "common.h"
#include "lcd.h"
#include "term_io.h"
#include "sd.h"

typedef struct {
	char list[4];
	int size;
	char fourCC[4];
	char * data; // contains Lists and Chunks
} AVIList;

typedef struct {
	int microSecPerFrame; // frame display rate (or 0)
	int maxBytesPerSec; // max. transfer rate
	int paddingGranularity; // pad to multiples of this
	// size;
	int flags; // the ever-present flags
	int totalFrames; // # frames in file
	int initialFrames;
	int streams;
	int suggestedBufferSize;
	int width;
	int height;
	int reserved[4];
} MainAVIHeader;

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


int main(){
	char * buff = malloc(10);
	FILE * f = fopen("final.avi", "r");
	MainAVIHeader mainHeader;
	loadMainAVIHeader(f, &mainHeader);
	printf("Height: %d\nWidth: %d\n", mainHeader.height, mainHeader.width);
	printf("movi offset: %d\n", getMoviOffset(f));
	fclose(f);
}
