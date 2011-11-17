#include "common.h"
#include "sd.h"


typedef struct {
	char list[4];
	uint32_t size;
	char fourCC[4];
	char * data; // contains Lists and Chunks
} AVIList;

typedef struct {
	uint32_t microSecPerFrame; // frame display rate (or 0)
	uint32_t maxBytesPerSec; // max. transfer rate
	uint32_t paddingGranularity; // pad to multiples of this
	// size;
	uint32_t flags; // the ever-present flags
	uint32_t totalFrames; // # frames in file
	uint32_t initialFrames;
	uint32_t streams;
	uint32_t suggestedBufferSize;
	uint32_t width;
	uint32_t height;
	uint32_t reserved[4];
} MainAVIHeader;

typedef struct {
	char fourCC[4];
	uint32_t size;
	char * data; // contains headers or video/audio data
} AVIChunk;



void loadMainAVIHeader(FIL * file, MainAVIHeader * header);
uint32_t getMoviOffset(FIL * file);