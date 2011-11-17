#ifndef __SD_H__
#define __SD_H__

uint8_t sdInit(void);
uint8_t sdReadBlocks(void *buffer, uint32_t lba, uint32_t blocksToRead);
uint8_t sdWriteBlocks(void *buffer, uint32_t lba, uint32_t blocksToWrite);
uint8_t sdGetSizeInfo(uint32_t *availableBlocks, uint16_t *sizeOfBlock);

#endif

