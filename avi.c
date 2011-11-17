#include "avi.h"
#include "lcd.h"

static uint16_t img[LCD_Y_SIZE][LCD_X_SIZE];

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
	return 28+aviList.size+junkSize+20;
}

void playMovie(FIL * file){	
	MainAVIHeader mainHeader;
	loadMainAVIHeader(file, &mainHeader);
	xprintf("Width: %d\nHeight: %d\n", mainHeader.width, mainHeader.height);
	uint16_t dummy;
	uint32_t offset = getMoviOffset(file);
	f_lseek(file, offset);
	
	for(int i = 0; i < mainHeader.totalFrames; i++){
		f_read(file,img,mainHeader.width*mainHeader.height*2,&dummy);
		lcdWriteRgb555(img);
		lcdUpdate();
		f_lseek(file, offset+(mainHeader.width*mainHeader.height*2+8)*(i+1));
	}
}