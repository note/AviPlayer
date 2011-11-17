#include "common.h"
#include "sd.h"

/*
this is a simple SD card interface module written by RABW in 2008/2009
SDHC support is an experimental option with some dumb detection and still quite slow writes
use this file at your own risk and don't blame me for a mess ;]
*/

//general macros
#define MCI_PIO			AT91C_BASE_PIOA		//PIO base address (where MCI pins are muxed)
#define MCI_PERIPH		PIO_BSR				//peripheral select register A or B
#define MCI_PINS_MASK	( \
	AT91C_PA0_MCDB0 | AT91C_PA1_MCCDB | AT91C_PA3_MCDB3 |\
	AT91C_PA4_MCDB2 | AT91C_PA5_MCDB1 | AT91C_PA8_MCCK )	//pins used by MCI controller
#define MCI				AT91C_BASE_MCI		//Base address of MCI to use
#define MCI_ID			AT91C_ID_MCI		//ID of MCI to use

//define which leds blink during write and read
//if you don't want any leds, just leave these 4 defs empty
#define LED_WR_ON			LED_R_ON
#define LED_WR_OFF			LED_R_OFF
#define LED_RD_ON			LED_Y_ON
#define LED_RD_OFF			LED_Y_OFF

/* macro for SD card block size. Cards may have different blocks sizes
however 512 bytes block is often used independently */
#define BLOCK_SIZE			512

//max number of blocks to be written/read in one call of sdWriteBlocksDma or sdReadBlocksDma
//keep MAX_DMA_BLOCKS * BLOCK_SIZE < 0x10000.
#define MAX_DMA_BLOCKS		127

//set it to nonzero if you wanna use DMA transfers for data exchange
//be careful cause DMA may interfere with cache!
#define USE_DMA				0

//define error masks
#define MCI_ERRORS_MASK	(AT91C_MCI_RINDE | AT91C_MCI_RDIRE   \
		| AT91C_MCI_RENDE | AT91C_MCI_RTOE | AT91C_MCI_DCRCE \
		| AT91C_MCI_DTOE | AT91C_MCI_OVRE | AT91C_MCI_UNRE)


//don't care argument
#define ARG_DONT_CARE		0x00000000

//commands
#define CMD_SEND_IF_COND			8
#define ACMD_SET_BUS_WIDTH			6
#define ACMD_OP_COND				41
#define CMD_APP_CMD					55
#define	CMD_GO_IDLE_STATE			0
#define CMD_ALL_SEND_CID			2
#define CMD_SET_RELATIVE_ADDR		3
#define CMD_SELECT_CARD				7
#define CMD_SEND_CSD				9
#define CMD_SEND_CID				10
#define CMD_STOP_TRANSMISSION		12
#define CMD_SEND_STATUS				13
#define CMD_READ_SINGLE_BLOCK		17
#define CMD_READ_MULTIPLE_BLOCK		18
#define CMD_WRITE_BLOCK				24
#define CMD_WRITE_MULTIPLE_BLOCK	25

//Relative Card Address (RCA)
static uint32_t rca;

//number of blocks on card
static uint32_t numberOfBlocks;

//actual block size
static uint32_t actualBlockSize;

static uint32_t sdhcFlag;

//private functions
static uint32_t mciInit(void);
static uint32_t cardInit(void);
static uint32_t sdCommand(uint32_t command, uint32_t argument);
static uint32_t sdResponse48(uint32_t *buf);
static uint32_t sdResponse136(uint32_t *buf);
static void sdDispResponse(uint32_t *pResp) __attribute__((unused));
static uint32_t sdBusyWait(void);
static uint32_t sdReadStatus(void);
static uint32_t sdWriteBlock(void *buffer, uint32_t lba) __attribute__((unused));
static uint32_t sdWriteBlocksDma(void *buffer, uint32_t lba, uint32_t blocksToWrite) __attribute__((unused));
static uint32_t sdReadBlock(void *buffer, uint32_t lba) __attribute__((unused));
static uint32_t sdReadBlocksDma(void *buffer, uint32_t lba, uint16_t blocksToRead) __attribute__((unused));


//===================================================================================================
//							INITIALIZATION/STATUS FUNCS
//===================================================================================================


static uint32_t mciInit(void)
{
	//enable MCI's clock
	AT91C_BASE_PMC->PMC_PCER = (1<<MCI_ID);
	
	//configure PIOs to be controlled by MCI
	MCI_PIO->PIO_PDR = MCI_PINS_MASK;
	
	//select peripheral for MCI pins
	MCI_PIO->MCI_PERIPH = MCI_PINS_MASK;
	
	//enable internal pullupz
	MCI_PIO->PIO_PPUER = MCI_PINS_MASK;
	
	//configure MCI regs
	
	//perform software reset
	MCI->MCI_CR = AT91C_MCI_SWRST;
	
	//enable MCI, disable power saving mode
	MCI->MCI_CR = AT91C_MCI_MCIEN;
	
	const uint8_t CLKDIV = 0xFF;
	const uint32_t MCI_CLKDIV_MASK = (uint32_t)CLKDIV;
	const uint32_t MCI_BLKLEN_MASK = (uint32_t)(BLOCK_SIZE << 16);
	
	//set MCI Mode Register
	#if USE_DMA
	MCI->MCI_MR =
		MCI_BLKLEN_MASK | MCI_CLKDIV_MASK |
		AT91C_MCI_PDCFBYTE | AT91C_MCI_RDPROOF | AT91C_MCI_WRPROOF;	
	#else
	MCI->MCI_MR =
		MCI_BLKLEN_MASK | MCI_CLKDIV_MASK |
		AT91C_MCI_RDPROOF | AT91C_MCI_WRPROOF;	
	#endif
	
	debug_msg("MCI_MR (1) = "); debug32_t(MCI->MCI_MR);
	
	//select slot B...
	const uint8_t SDCSEL_SLOT_B = 1;
	//...in MCI SDCR register
	MCI->MCI_SDCR = SDCSEL_SLOT_B | AT91C_MCI_SCDBUS;		//AT91C_MCI_SCDBUS means 4-bit bus
	
	//set timeout stuff
	const uint8_t DTOCYC = 0x0F;
	MCI->MCI_DTOR = DTOCYC | AT91C_MCI_DTOMUL_1048576;
	
	return(OK);
}


/*	this init func is not 100% compatilbe with SDHC specification!!!
	It is simple and dumb: SDHC cards are detected by checking CSS bit in busy wait func.
	CMD8 is issued just for compatibility and it doesnt change anything in the program.
	The init func supports neither SDIO nor MMC cards. See Figure 7-2 in SDHC Simplified spec
	for more details about standard init procedure. */
static uint32_t cardInit(void)
{
	
	//universal buffer for responses
	uint32_t response[5];
	
	//initialization command (74 clock cycles for initialization sequence)
	sdCommand(AT91C_MCI_SPCMD_INIT, ARG_DONT_CARE);
	
	sdCommand(CMD_GO_IDLE_STATE,ARG_DONT_CARE);	//Issue CMD0
	
	//Issue CMD8 with 10101010 arg and VHS=1
	if(sdCommand(CMD_SEND_IF_COND | AT91C_MCI_MAXLAT | AT91C_MCI_RSPTYP_48,0x000001AA)==OK)
	{
		sdResponse48(response);
		debug_msg("response to CMD8: ");
		sdDispResponse(response);
	}
	else
	{
		debug_msg("CMD8 failed");
	}
	
	if(sdBusyWait()!=OK)
	{
		debug_msg("sdBusyWait returned FAIL");
		return(FAIL);
	}
	
	sdCommand(CMD_ALL_SEND_CID | AT91C_MCI_MAXLAT | AT91C_MCI_RSPTYP_136, ARG_DONT_CARE);
	sdResponse136(response);
	
	sdCommand(CMD_SET_RELATIVE_ADDR | AT91C_MCI_MAXLAT | AT91C_MCI_RSPTYP_48, ARG_DONT_CARE);
	
	//response R6
	sdResponse48(response);
	
	//extract relative card address and copy it to a global variable
	rca = (response[0] >> 16) & 0xFFFF;
	
	//read CSD
	sdCommand(CMD_SEND_CSD | AT91C_MCI_MAXLAT | AT91C_MCI_RSPTYP_136, rca<<16);
	sdResponse136(response);
	
	//compute block length and number of blockz
	//example of returned CSD stuff (SD 256MB)
	//005E0032 1F5983CF EDB6BF87 964000B5 005E0032
	//            ^ ^^^ ^
	//      bl.len| |||C_SIZE
	/*
	  4GB SDHC:
	  400E0032 5B590000 1DE77F80 0A4000D5 400E0032
	  bl.len is fixed to 0x9 = 512B/block
	*/
	
	uint8_t *pResp = (uint8_t*)response;
	
	debug_msg("send CSD response:");
	sdDispResponse(response);
	
	if( (response[0] & 0x40000000) == 0x40000000 )
	{
		//compute number of blocks
		uint32_t c_size = ((response[1]<<16) | (response[2]>>16)) & 0x3FFFFF;
		numberOfBlocks = (c_size + 1) * 1024;
		
		if( (response[1] & 0x00090000) == 0x00090000 )
		{
			actualBlockSize = BLOCK_SIZE;
		}
		else
		{
			debug_msg("cardInit: wrong SDHC block size");
			return(FAIL);
		}
	}
	else
	{
		//compute number of blocks
		uint32_t size;
		size = pResp[5] & 0x03;
		size <<= 10;
		size |= (uint16_t)pResp[4]<<2;
		size |= pResp[11]>>6;
		uint16_t sizeMultiplier = 1 << (2+ ( ( (pResp[10] & 0x03)<<1 ) | (pResp[9] >> 7) ) );
		numberOfBlocks=sizeMultiplier*(size+1);
		
		//compute block size
		actualBlockSize = 1 << ((response[1]>>16) & 0x0F);
		debug_msg("actual block size: "); debug16_t(actualBlockSize);
	}
	
	//when all (just one in this case) CSD regs are read, we may set the full speed (yeah!)
	const uint8_t CLKDIV = 0x01;
	const uint32_t MCI_CLKDIV_MASK = (uint32_t)CLKDIV;
	
	//zero CLKDIV field
	MCI->MCI_MR &= ~AT91C_MCI_CLKDIV;
	
	//set MCI Mode Register
	MCI->MCI_MR |= MCI_CLKDIV_MASK;
	
	debug_msg("MCI_MR (2) = "); debug32_t(MCI->MCI_MR);
	
	//select card (put it into the transfer state)
	sdCommand(CMD_SELECT_CARD | AT91C_MCI_RSPTYP_48 | AT91C_MCI_MAXLAT, rca<<16);
	sdResponse48(response);
	
	//set 4-bit bus width
	sdCommand(CMD_APP_CMD | AT91C_MCI_MAXLAT | AT91C_MCI_RSPTYP_48, rca<<16);
	sdResponse48(response);
	
	sdCommand(ACMD_SET_BUS_WIDTH | AT91C_MCI_MAXLAT | AT91C_MCI_RSPTYP_48, 2);
	sdResponse48(response);
	
	/*sdCommand(16 | AT91C_MCI_MAXLAT | AT91C_MCI_RSPTYP_48, 512);
	sdResponse48(response);*/
	
	return(OK);
}


static uint32_t sdReadStatus(void)
{
	sdCommand(CMD_SEND_STATUS | AT91C_MCI_MAXLAT | AT91C_MCI_RSPTYP_48, rca<<16);
	uint32_t response[2];
	sdResponse48(response);
	return(response[0]);
}


/*
waits for ready status and checks CSS bit OCR contents
If CSS is 1 after status changed to "ready", it sets global sdhcFlag to 1
otherwise sdhcFlag is set to 0.
*/
static uint32_t sdBusyWait(void)
{
	uint32_t response[2];
	for(uint16_t i=0;i<100;i++)
	{
		delay_ms(10);
		
		//after those commands, status will be in response[0]
		
		if(sdCommand(CMD_APP_CMD | AT91C_MCI_MAXLAT | AT91C_MCI_RSPTYP_48,0)!=OK) return(FAIL);
		sdResponse48(response);
		
		//Issue ACMD41 to get OCR
		//bit 30 is HCS bit and FF8 is voltage mask (see page 55 of SDHC simplified spec)
		if(sdCommand(ACMD_OP_COND | AT91C_MCI_RSPTYP_48, 0x00FF8000 | (1<<30))!=OK) return(FAIL);
		sdResponse48(response);
		
		//debug_msg("busywait resp:");
		//sdDispResponse(response);
		
		//debug_msg("press something...");
		//debug_waitkey();
		
		if(response[0] & (1<<31))	//if ready bit is set
		{
			//set SDHC flag when ready and CSS bits are set
			if(response[0] & (1<<30)) sdhcFlag=1; else sdhcFlag=0;
			return(OK);
		}
	}
	debug_txt("sdhcBusyWait: TIMEOUT!!!");
	return(FAIL);
}



//===================================================================================================
//								COMMAND/RESPONSE
//===================================================================================================



static uint32_t sdCommand(uint32_t command, uint32_t argument)
{
	MCI->MCI_ARGR = argument;
	MCI->MCI_CMDR = command;
	
	uint32_t status;
	
	do
	{
		status = MCI->MCI_SR;
	}while((status & AT91C_MCI_CMDRDY)==0);
	
	if(status & MCI_ERRORS_MASK)
	{
		debug_msg("sdCommand error, CMD="); debug32_t(command);
		debug_txt(", status="); debug32_t(status);
		return(FAIL);
	}
	
	return(OK);
}



static uint32_t sdResponse48(uint32_t *buf)
{
	for(uint8_t i=0;i<2;i++)
	{
		buf[i] = MCI->MCI_RSPR[0];
	}
	return(OK);
}

static uint32_t sdResponse136(uint32_t *buf)
{
	for(uint8_t i=0;i<5;i++)
	{
		buf[i] = MCI->MCI_RSPR[0];
	}
	return(OK);
}


static void sdDispResponse(uint32_t *pResp)
{
	debug_msg("response: ");
	//uint8_t *respByte = (void*)pResp;
	for(uint32_t i=0;i<4;i++)
	{
		//debug_bin(*respByte++);
		debug32_t(*pResp++);
		debug_chr(' ');
	}
}


//delay for less than 1ms
static void shortDelay(void)
{
	delay_us(500);
}


//===================================================================================================
//								INTERNAL R/W FUNCS
//===================================================================================================



static uint32_t sdReadBlocksDma(void *buffer, uint32_t lba, uint16_t blocksToRead)
{
	
	//enable PDC mode
	MCI->MCI_MR |= AT91C_MCI_PDCMODE;
	
	//configure the PDC channel
	MCI->MCI_RPR = (uint32_t)buffer;
	MCI->MCI_RCR = BLOCK_SIZE * blocksToRead;
	MCI->MCI_PTCR = AT91C_PDC_RXTEN;
	
	//send CMD_READ_MULTIPLE_BLOCK (CMD18) command
	if(sdhcFlag)
	{
		sdCommand(CMD_READ_MULTIPLE_BLOCK | AT91C_MCI_MAXLAT | AT91C_MCI_TRDIR | AT91C_MCI_TRCMD_START | AT91C_MCI_TRTYP_MULTIPLE | AT91C_MCI_RSPTYP_48, lba);
	}
	else
	{
		uint32_t byteAddress = lba * BLOCK_SIZE;
		sdCommand(CMD_READ_MULTIPLE_BLOCK | AT91C_MCI_MAXLAT | AT91C_MCI_TRDIR | AT91C_MCI_TRCMD_START | AT91C_MCI_TRTYP_MULTIPLE | AT91C_MCI_RSPTYP_48, byteAddress);
	}
	
	while((MCI->MCI_SR & AT91C_MCI_ENDRX)==0);
	
	sdCommand(CMD_STOP_TRANSMISSION | AT91C_MCI_MAXLAT | AT91C_MCI_TRCMD_STOP | AT91C_MCI_RSPTYP_48, ARG_DONT_CARE);
	
	return(OK);
}



//keep (blocksToWrite * BLOCK_SIZE) < 0x10000
static uint32_t sdWriteBlocksDma(void *buffer, uint32_t lba, uint32_t blocksToWrite)
{
	
	//enable PDC mode
	MCI->MCI_MR |= AT91C_MCI_PDCMODE;
	
	//set block len in bytes
	MCI->MCI_BLKR = (uint32_t)(BLOCK_SIZE)<<16;
	
	shortDelay();
	
	if(sdhcFlag)
	{
		sdCommand(CMD_WRITE_MULTIPLE_BLOCK | AT91C_MCI_MAXLAT | AT91C_MCI_TRCMD_START | AT91C_MCI_TRTYP_MULTIPLE | AT91C_MCI_RSPTYP_48, lba);
	}
	else
	{
		uint32_t byteAddress = lba * BLOCK_SIZE;
		sdCommand(CMD_WRITE_MULTIPLE_BLOCK | AT91C_MCI_MAXLAT | AT91C_MCI_TRCMD_START | AT91C_MCI_TRTYP_MULTIPLE | AT91C_MCI_RSPTYP_48, byteAddress);
	}
	
	MCI->MCI_TPR = (uint32_t)buffer;
	MCI->MCI_TCR = BLOCK_SIZE * blocksToWrite;
	
	//start DMA transfer
	MCI->MCI_PTCR = AT91C_PDC_TXTEN;
	
	//now poll BLKE bit
	while((MCI->MCI_SR & AT91C_MCI_BLKE) == 0);
	
	//debug_msg("*CMD12");
	sdCommand(CMD_STOP_TRANSMISSION | AT91C_MCI_MAXLAT | AT91C_MCI_TRCMD_STOP | AT91C_MCI_RSPTYP_48, ARG_DONT_CARE);
	
	MCI->MCI_PTCR = AT91C_PDC_TXTDIS;
	
	//debug_msg("waiting for NOTBUSY bit...");
	while((MCI->MCI_SR & AT91C_MCI_NOTBUSY) == 0);
	
	while((sdReadStatus() & (1<<8))==0);
	
	return(OK);
}




static uint32_t sdReadBlock(void *buffer, uint32_t lba)
{
	//disable PDC mode (if enabled)
	MCI->MCI_MR &= ~AT91C_MCI_PDCMODE;
	
	if(sdhcFlag)
	{
		sdCommand(CMD_READ_SINGLE_BLOCK | AT91C_MCI_MAXLAT  | AT91C_MCI_TRDIR | AT91C_MCI_TRCMD_START | AT91C_MCI_RSPTYP_48, lba);
	}
	else
	{
		uint32_t byteAddress = lba * BLOCK_SIZE;
		sdCommand(CMD_READ_SINGLE_BLOCK | AT91C_MCI_MAXLAT  | AT91C_MCI_TRDIR | AT91C_MCI_TRCMD_START | AT91C_MCI_RSPTYP_48, byteAddress);
	}
	
	uint32_t wordsToRead = BLOCK_SIZE / 4;
	uint8_t *pBuf = (uint8_t*)buffer;
	
	while(wordsToRead--)
	{
		while((MCI->MCI_SR & AT91C_MCI_RXRDY)==0);
		uint32_t temp;
		temp = MCI->MCI_RDR;
		for(uint8_t i=0;i<4;i++)
		{
			*pBuf = temp;
			temp = temp>>8;
			pBuf++;
		}
	}
	
	return(OK);
}



static uint32_t sdWriteBlock(void *buffer, uint32_t lba)
{
	//disable PDC mode (if enabled)
	MCI->MCI_MR &= ~AT91C_MCI_PDCMODE;
	
	if(sdhcFlag)
	{
		sdCommand(CMD_WRITE_BLOCK | AT91C_MCI_MAXLAT | AT91C_MCI_TRCMD_START | AT91C_MCI_RSPTYP_48, lba);
	}
	else
	{
		uint32_t byteAddress = lba * BLOCK_SIZE;
		sdCommand(CMD_WRITE_BLOCK | AT91C_MCI_MAXLAT | AT91C_MCI_TRCMD_START | AT91C_MCI_RSPTYP_48, byteAddress);
	}
	
	uint32_t wordsToWrite = BLOCK_SIZE / 4;
	uint8_t *pBuf = (uint8_t*)buffer;
	while(wordsToWrite--)
	{
		while(!(MCI->MCI_SR & AT91C_MCI_TXRDY));
		uint32_t temp;
		uint8_t *pTemp = (uint8_t*)&temp;
		for(uint8_t i=0;i<4;i++)
		{
			*pTemp = *pBuf;
			pBuf++;
			pTemp++;
		}
		MCI->MCI_TDR = temp;
	}
	
	//wait for NOTBUSY bit set
	while((MCI->MCI_SR & AT91C_MCI_NOTBUSY) == 0);
	
	
	return(OK);
}

//===================================================================================================
//								PUBLIC/INTERFACE FUNCTIONS
//===================================================================================================


uint8_t sdInit(void)
{
	//initialize MCI controller and IO pins
	mciInit();
	
	//initialize SD card in slot
	if(cardInit()!=OK)
	{
		return(FAIL);
	}
	
	return(OK);
}



uint8_t sdGetSizeInfo(uint32_t *availableBlocks, uint16_t *sizeOfBlock)
{
	*availableBlocks = numberOfBlocks * (actualBlockSize / BLOCK_SIZE);
	*sizeOfBlock = BLOCK_SIZE;	//return always the same block size
	return(OK);
}



uint8_t sdWriteBlocks(void *buffer, uint32_t lba, uint32_t blocksToWrite)
{
	LED_WR_ON;
	#if USE_DMA
		//debug_txt(" *btw="); debug32_t(blocksToWrite);
		uint8_t *pBuf = (uint8_t*)buffer;
		while(blocksToWrite>MAX_DMA_BLOCKS)
		{
			if(sdWriteBlocksDma(pBuf,lba,MAX_DMA_BLOCKS)!=OK) {LED_WR_OFF; return(FAIL);}
			pBuf+=BLOCK_SIZE*MAX_DMA_BLOCKS;
			blocksToWrite-=MAX_DMA_BLOCKS;
			lba+=MAX_DMA_BLOCKS;
		}
		if(blocksToWrite)
		{
			if(sdWriteBlocksDma(pBuf,lba,blocksToWrite)!=OK) {LED_WR_OFF; return(FAIL);}
		}
	#else
		uint8_t *pBuf = (uint8_t*)buffer;
		while(blocksToWrite--)
		{
			if(sdWriteBlock(pBuf,lba)!=OK) {LED_WR_OFF; return(FAIL);}
			pBuf+=BLOCK_SIZE;
			lba++;
		}
	#endif
	LED_WR_OFF;
	
	return(OK);
}



uint8_t sdReadBlocks(void *buffer, uint32_t lba, uint32_t blocksToRead)
{
	LED_RD_ON;
	#if USE_DMA
		//debug_txt(" *btr="); debug32_t(blocksToRead);
		uint8_t *pBuf = (uint8_t*)buffer;
		while(blocksToRead>MAX_DMA_BLOCKS)
		{
			if(sdReadBlocksDma(pBuf,lba,MAX_DMA_BLOCKS)!=OK) {LED_RD_OFF; return(FAIL);}
			pBuf+=BLOCK_SIZE*MAX_DMA_BLOCKS;
			blocksToRead-=MAX_DMA_BLOCKS;
			lba+=MAX_DMA_BLOCKS;
		}
		if(blocksToRead)
		{
			if(sdReadBlocksDma(pBuf,lba,blocksToRead)!=OK) {LED_RD_OFF; return(FAIL);}
		}
	#else
		uint8_t *pBuf = (uint8_t*)buffer;
		while(blocksToRead--)
		{
			if(sdReadBlock(pBuf,lba)!=OK) {LED_RD_OFF; return(FAIL);}
			pBuf+=BLOCK_SIZE;
			lba++;
		}
	#endif
	LED_RD_OFF;
	
	return(OK);
}
