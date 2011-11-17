/*linker script for programs that use SDRAM and internal SRAM memories*/

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(entry)

MEMORY
{
    sram0 (W!RX) : ORIGIN = 0x000000, LENGTH = 0x1000
    sram1 (W!RX) : ORIGIN = 0x300000, LENGTH = 0x1000
    sdram_cachable (W!RX) : ORIGIN = 0x20000000, LENGTH = 0x0FC000
	/*0x200FC000 to 0x20100000 (16KB) reserved for TLB*/
    sdram_noncachable (W!RX) : ORIGIN = 0x20100000, LENGTH = 0x1F00000
}


SECTIONS
{  
    .fixed :
    {
        . = ALIGN(4);
        _sfixed = .;
        *(.text*)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.data)
        . = ALIGN(4);
        _efixed = .;
    } >sdram_cachable

    .relocate : AT (_efixed)
    {
        . = ALIGN(4);
        _srelocate = .;
        *(.vectors);
        . = ALIGN(4);
		*(.fast_data_0);
        . = ALIGN(4);
        _erelocate = .;
    } >sram0
	
    .zero (NOLOAD) : {
        _szero = .;
        *(.bss)
        . = ALIGN(4);
		*(COMMON)
        . = ALIGN(4);
        _ezero = .;
    } >sdram_cachable
	
	_sheap_cachable = .;		/*cachable heap starts after bss and COMMON*/
	
	.buffers (NOLOAD) : {
		_sbuffers = .;
		*(.buffers)
        . = ALIGN(4);
		_ebuffers = .;
	} >sdram_noncachable
    
	_sheap_noncachable = .;		/*noncachable heap starts after the buffers*/
	
	.fast_sram1 (NOLOAD) : {
		_sfast_data = .;
		*(.fast_data_1)
        . = ALIGN(4);
		_efast_data = .;
	} >sram1
	
    _sstack = 0x200FC000;
	
	_end_of_user_ram = 0x22000000;
}
end = .;
