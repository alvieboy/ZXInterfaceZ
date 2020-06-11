include "port_defs.asm"
		
;                OUTPUT	"INTZ.ROM"

; System variables definitions

		include	"snaloader-sysvars.asm"

;*****************************************
;** Part 1. RESTART ROUTINES AND TABLES **
;*****************************************

;------
; Start
;------
; At switch on, the Z80 chip is in interrupt mode 0.
; This location can also be 'called' to reset the machine.
; Typically with PRINT USR 0.

	ORG	$0000
			       
START:	DI			; disable interrupts.
	JP	RAM_DONE


	ORG	$0008
		      
RST8:	JP 	RST8


	ORG	$0066
NMIH:	RETN

	ORG 	$0080
	
;        Offset   Size   Description
;   ------------------------------------------------------------------------
;   0        1      byte   I                                      Check
;   1        8      word   HL',DE',BC',AF'                        Check
;   9        10     word   HL,DE,BC,IY,IX                         Check
;   19       1      byte   Interrupt (bit 2 contains IFF2, 1=EI/0=DI)  Check
;   20       1      byte   R                                      Check
;   21       4      words  AF,SP                                  Check
;   25       1      byte   IntMode (0=IM0/1=IM1/2=IM2)            Check
;   26       1      byte   BorderColor (0..7, not used by Spectrum 1.7)  Check
;   27       49152  bytes  RAM dump 16384..65535
;   ------------------------------------------------------------------------

include "snarestore.asm"
        
RAM_DONE:
	LD	HL, $5F00
        LD	SP, HL  	; TBD: do we need stack?
        
        LD	HL, $4000
        LD	DE, $BFFF
        LD	B, %00000001
LOADER:
	IN	A, (PORT_RESOURCE_FIFO_STATUS)
        OR	A
        JR 	NZ, LOADER
        IN	A, (PORT_RESOURCE_FIFO_DATA)
        LD	(HL), A
	INC	HL
	DEC	DE
        LD	A, B
        XOR	%00000011
        OUT	($FE), A
        LD	B, A
    	LD	A, D
        OR 	E
    	JR	NZ, LOADER
        JP	RESTOREREGS

