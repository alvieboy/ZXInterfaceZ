		
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

RESTOREREGS:
; This is the register restore routine.
        LD	HL, (P_TEMP)	; Save P_TEMP
        LD	DE, $FFFF 	; FILL register: AF'
        LD	(P_TEMP), DE
        LD	SP, P_TEMP      ; 
        LD	BC, $FFFF	; FILL register: BC' 
        LD	DE, $FFFF       ; FILL register: DE'
        LD	HL, $FFFF       ; FILL register: HL'
        POP	AF
        EX	AF, AF'
        EXX
        LD	A, $FF          ; FILL register: I
        LD	I, A
        LD	DE, $FFFF 	; FILL register : AF
        LD	(P_TEMP), DE
        LD	BC, $FFFF       ; FILL register : BC
        
        LD	SP, P_TEMP      ; TBD: check if SP points to actual word
        LD	A, $FF          ; FILL register: R
        LD	R, A
        LD	A, $FF		; FILL border
        OUT	($FE), A
        POP	AF
        LD	DE, $FFFF       ; FILL register : DE
        LD	IX, $FFFF       ; FILL register: IX
        LD	IY, $FFFF       ; FILL register: IY
        LD	(P_TEMP), HL    ; Restore P_TEMP
        LD	SP, $FFFF       ; FILL register : SP
	LD	HL, $FFFF       ; FILL register : HL
        IM	0		; FILL IM X
        EI			; FILL EI or DI, depending. We should time this so that no interrupt can come before JP
RETINST:RETN
        
RAM_DONE:
	LD	HL, $5F00
        LD	SP, HL
        
        LD	HL, $4000
        LD	DE, $BFFF
        LD	B, %00000001
LOADER:
	IN	A, ($0B)
        OR	A
        JR 	NZ, LOADER
        IN	A, ($0D)
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

