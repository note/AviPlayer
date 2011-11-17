#include "common.h"
#include "delay.h"

#define TIMER			AT91C_BASE_TC1
#define TIMER_ID		AT91C_ID_TC1


#define TC_CLKS_MCK32	0x2
#define TC_CLKS_MCK128	0x3


void delay_us(uint16_t us)
{
	AT91C_BASE_PMC->PMC_PCER = (1<<TIMER_ID);
	
	if(us < 10000)
	{
		TIMER->TC_CMR = AT91C_TC_WAVE  //bit WAVE=1 => Waveform Mode
			|TC_CLKS_MCK32   //selected Timer clock freq.
			|AT91C_TC_CPCDIS
			|AT91C_TC_WAVESEL_UP_AUTO;  //Waveform Selection: UP mode with automatic trigger on RC compare (10)
		
		TIMER->TC_RC = us * ((double)MCK / (double)32000000);
	}
	else
	{
		TIMER->TC_CMR = AT91C_TC_WAVE  //bit WAVE=1 => Waveform Mode
			|TC_CLKS_MCK128   //selected Timer clock freq.
			|AT91C_TC_CPCDIS
			|AT91C_TC_WAVESEL_UP_AUTO;  //Waveform Selection: UP mode with automatic trigger on RC compare (10)
		
		TIMER->TC_RC = us * ((double)MCK / (double)128000000);
	}
	
	uint32_t status = TIMER->TC_SR;
	status = status;
	
	//enable timer's clock
	TIMER->TC_CCR = AT91C_TC_CLKEN;
	
	//start timer
	TIMER->TC_CCR = AT91C_TC_SWTRG ;
	
	//wait for TC event
	while( (TIMER->TC_SR & AT91C_TC_CPCS) == 0 );
	
	//disable timer's clock
	AT91C_BASE_PMC->PMC_PCDR = (1<<TIMER_ID);
}


static void timer_ms_wait(uint16_t ms)
{
	AT91C_BASE_PMC->PMC_PCER = (1<<TIMER_ID);
	
	TIMER->TC_CMR = AT91C_TC_WAVE  //bit WAVE=1 => Waveform Mode
		|TC_CLKS_MCK128   //selected Timer clock freq.
		|AT91C_TC_CPCDIS
		|AT91C_TC_WAVESEL_UP_AUTO;  //Waveform Selection: UP mode with automatic trigger on RC compare (10)
	
	TIMER->TC_RC = ms * MCK / 128 / 1000;
	
	uint32_t status = TIMER->TC_SR;
	status = status;
	
	//enable timer's clock
	TIMER->TC_CCR = AT91C_TC_CLKEN;
	
	//start timer
	TIMER->TC_CCR = AT91C_TC_SWTRG ;
	
	//wait for TC event
	while( (TIMER->TC_SR & AT91C_TC_CPCS) == 0 );
	
	//disable timer's clock
	AT91C_BASE_PMC->PMC_PCDR = (1<<TIMER_ID);
}


void delay_ms(uint16_t ms)
{
	if(ms < 64)
	{
		timer_ms_wait(ms);
	}
	else
	{
		uint16_t full = ms >> 6;
		uint16_t remaining = ms - (full << 6);
		while(full--) timer_ms_wait(1 << 6);
		if(remaining) timer_ms_wait(remaining);
	}
}


void delay_s(uint16_t s)
{
	while(s--) delay_ms(1000);
}

