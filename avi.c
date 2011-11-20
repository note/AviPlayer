#include "avi.h"
#include "lcd.h"

#define AVI_BUF_SIZE 1

static uint16_t img[LCD_Y_SIZE][LCD_X_SIZE];
static uint16_t buf[AVI_BUF_SIZE][LCD_Y_SIZE*LCD_X_SIZE+4]  __attribute__((section(".buffers")));

void loadMainAVIHeader(FIL * file, MainAVIHeader * header){
	uint16_t dummy;
	f_lseek(file, 32);
	f_read(file, header, sizeof(MainAVIHeader), &dummy);
}

uint32_t getMoviOffset(FIL * file){
	uint16_t dummy;
	AVIList aviList;
	uint32_t junkSize;
	f_lseek(file, 12);
	f_read(file, &aviList, sizeof(AVIList), &dummy);
	f_lseek(file, 24 + aviList.size);
	f_read(file, &junkSize, sizeof(junkSize), &dummy);
	return 28+aviList.size+junkSize+20-8;
}

void playMovie(FIL * file){	
	MainAVIHeader mainHeader;
	loadMainAVIHeader(file, &mainHeader);
	//xprintf("Width: %d\nHeight: %d\n", mainHeader.width, mainHeader.height);
	uint32_t dummy;
	uint32_t offset = getMoviOffset(file);
	f_lseek(file, offset);
	int k;
	for(int i = 0; i < mainHeader.totalFrames; i += AVI_BUF_SIZE){
		k = AVI_BUF_SIZE;
		if(mainHeader.totalFrames - i < AVI_BUF_SIZE) k = mainHeader.totalFrames - i;
		f_read(file, buf, (mainHeader.width*mainHeader.height*2+8)*k, &dummy);
		for(int j = 0; j<k; ++j){
			lcdWriteRgb555(buf[j]+4);
			LED_R_ON;
			lcdUpdate();
			LED_R_OFF;
		}
		f_lseek(file, offset+(mainHeader.width*mainHeader.height*2+8)*(i+k));
	}
}