/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation

 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 * 
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the disclaimer below in the documentation and/or
 * other materials provided with the distribution. 
 * 
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission. 
 * 
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
 /*
 modified by RABW, 2008
	- removed accesses to sections: .prerelocate and .postrelocate
	- .posrelocate section is now .relocate section - simply
	- copying .data section contents from flash image to RAM is done AFTER a call to AT91F_LowLevelInit
		=> thus don't use any variables occupying .data section in AT91F_LowLevelInit (they will not be initialized yet!)
		   in AT91F_LowLevelInit the stack already is initialized, so any automatic variables and constatns (.rodata) can be used
	- added a few more or less stupid comments to assembly lines
	- removed some dead calls just after resetHandler label
	- added simple support for Data Abort and Prefetch Abort exceptions
	
 */

//------------------------------------------------------------------------------
//         Definitions
//------------------------------------------------------------------------------

#define IRQ_STACK_SIZE   8*3*4		//saving 3 registers to IRQ stack for each of 8 priorities
#define FIQ_STACK_SIZE   0x100		//FIQ uses its own stack

#define ARM_MODE_ABT     0x17
#define ARM_MODE_FIQ     0x11
#define ARM_MODE_IRQ     0x12
#define ARM_MODE_SVC     0x13

#define I_BIT            0x80
#define F_BIT            0x40

#define AT91C_BASE_AIC   0xFFFFF000
#define AIC_IVR          0x100
#define AIC_FVR          0x104
#define AIC_EOICR        0x130

//------------------------------------------------------------------------------
//         Startup routine
//------------------------------------------------------------------------------

            .align      4
            .arm
        
/* Exception vectors
 *******************/
            .section    .vectors, "a", %progbits

resetVector:
        ldr     pc, =resetHandler       /* Reset */
undefVector:
        b       undefVector             /* Undefined instruction */
swiVector:
        b       swiVector               /* Software interrupt */
prefetchAbortVector:
        b       prefetchAbortHandler     /* Prefetch abort */
dataAbortVector:
        b       dataAbortHandler         /* Data abort */
reservedVector:
        b       reservedVector          /* Reserved for future use */
irqVector:
        b       irqHandler              /* Interrupt */
fiqVector:
                                        /* Fast interrupt */
//------------------------------------------------------------------------------
/// Handles a fast interrupt request by branching to the address defined in the
/// AIC.
//------------------------------------------------------------------------------

/*- Set FIQ Base */
		ldr         r8, =AT91C_BASE_AIC	//using banked reg. r8

/*- Save r0 in R9 (banked) */
		mov         r9,r0
		ldr         r0 , [r8, #AIC_FVR]

/*- Save scratch/used registers and LR in FIQ Stack */
		stmfd       sp!, { r1-r3, lr}

/*- Branch to the routine pointed by the AIC_FVR */
		mov         lr, pc
		bx          r0

/*- Restore scratch/used registers and LR from FIQ Stack */
		ldmia       sp!, { r1-r3, lr}

/*- Restore the R0 register */
		mov         r0,r9

/*- Restore the Program Counter using the LR_fiq directly in the PC */
		subs        pc,lr,#4



dataAbortHandler:
	//change state to supervisor, disable IRQ and FIQ
        msr     CPSR_c, #ARM_MODE_SVC | I_BIT | F_BIT
	
	//save registers to supervisor stack
        stmfd   sp!, {r0-r12, lr}
	
	//jump to high level abort handler
	    ldr     r0, =defaultDataAbortHandler
        mov     lr, pc
        bx      r0

prefetchAbortHandler:
	//change state to supervisor, disable IRQ and FIQ
        msr     CPSR_c, #ARM_MODE_SVC | I_BIT | F_BIT
	
	//save registers to supervisor stack
        stmfd   sp!, {r0-r12, lr}
	
	//jump to high level abort handler
	    ldr     r0, =defaultPrefetchAbortHandler
        mov     lr, pc
        bx      r0

	
//------------------------------------------------------------------------------
/// Handles incoming interrupt requests by branching to the corresponding
/// handler, as defined in the AIC. Supports interrupt nesting.
//------------------------------------------------------------------------------
irqHandler:

/* Save interrupt context on the stack to allow nesting */
        sub     lr, lr, #4
        stmfd   sp!, {lr}
        mrs     lr, SPSR
        stmfd   sp!, {r0, lr}

/* Write in the IVR to support Protect Mode */
        ldr     lr, =AT91C_BASE_AIC
        ldr     r0, [lr, #AIC_IVR]
        str     lr, [lr, #AIC_IVR]

/* Branch to interrupt handler in Supervisor mode */
        msr     CPSR_c, #ARM_MODE_SVC
        stmfd   sp!, {r1-r3, r12, lr}
        mov     lr, pc
        bx      r0
		
/* Restore scratch/used registers and LR from User Stack */
        ldmia   sp!, {r1-r3, r12, lr}
		
/* Disable Interrupt and switch back to IRQ mode */
        msr     CPSR_c, #ARM_MODE_IRQ | I_BIT
		
/* Acknowledge interrupt */
        ldr     lr, =AT91C_BASE_AIC
        str     lr, [lr, #AIC_EOICR]
		
/* Restore interrupt context and branch back to calling code */
        ldmia   sp!, {r0, lr}
        msr     SPSR_cxsf, lr
        ldmia   sp!, {pc}^ /* A4-40, ^ = SPSR of the current mode is to be copied to the CPSR */


//------------------------------------------------------------------------------
/// Initializes the chip and branches to the main() function.
//------------------------------------------------------------------------------
            .section    .text
            .global     entry

entry:
resetHandler:

/* Perform low-level initialization of the chip using AT91F_LowLevelInit() */
        ldr     sp, =_sstack
	    ldr     r0, =AT91F_LowLevelInit
        mov     lr, pc
        bx      r0

/* Copy .relocate section from image to its VMA */
        ldr     r0, =_efixed
        ldr     r1, =_srelocate
        ldr     r2, =_erelocate
1:
        cmp     r1, r2			//compare r1 - r2
        ldrcc   r3, [r0], #4	//if C flag is clear, copy next word from _efixed pointed by r0 to r3
        strcc   r3, [r1], #4	//copy r3 to the address pointed by r1 (in RAM)
        bcc     1b				//jump one label "1:" backward (1b). "1f" means 1 label forward (simply loop)

/* Clear the zero segment */
	    ldr     r0, =_szero
        ldr     r1, =_ezero
        mov     r2, #0			//value to be written (0)
1:
        cmp     r0, r1
        strcc   r2, [r0], #4
        bcc     1b

/* Setup stacks
 **************/
/* FIQ mode */
        msr     CPSR_c, #ARM_MODE_FIQ | I_BIT | F_BIT	//move control flags (_c) to CPSR: mode=FIQ, mask IRQ and FIQ
        ldr     sp, =_sstack							//sp = _sstack (see .lds file)
        sub     r4, sp, #FIQ_STACK_SIZE					//r4 = sp - FIQ_STACK_SIZE;

/* IRQ mode */
        msr     CPSR_c, #ARM_MODE_IRQ | I_BIT | F_BIT	//move control flags (_c) to CPSR: mode=IRQ, mask IRQ and FIQ
        mov     sp, r4									//sp (IRQ) = r4 = _sstack - FIQ_STACK_SIZE
        sub     r4, sp, #IRQ_STACK_SIZE					//r4 = sp - IRQ_STACK_SIZE;
		
/* Supervisor mode (interrupts enabled) */
        msr     CPSR_c, #ARM_MODE_SVC					//back to supervisor mode, FIQ still masked
        mov     sp, r4									//supervisor stack @ address = _sstack - IRQ_STACK_SIZE (=r4)

/* Branch to main()
 ******************/
        ldr     r0, =main	//load address of main to r0
        mov     lr, pc		//move value in PC into link register
        bx      r0			//branch to main

/* Loop indefinitely when program is finished */
1:
        b       1b

