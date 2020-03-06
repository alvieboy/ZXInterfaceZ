		
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
	LD	DE,$FFFF	; top of possible physical RAM.
	JP	ROM_CHECK	; jump forward to common code at START_NEW.


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
	DI
        EXX
        LD	HL, (P_TEMP)	; Save P_TEMP
        LD	DE, $FFF1 	; FILL register: AF'
        LD	(P_TEMP), DE
        LD	SP, P_TEMP      ; TBD: check if SP points to actual word
        LD	BC, $FFFF	; FILL register: BC' 
        LD	DE, $FFFF       ; FILL register: DE'
        LD	HL, $FFFF       ; FILL register: HL'
        POP	AF
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
        LD	A, 1
        LD	DE, RETINST+2   ; Load the last fetch address, Once this address is requested, FPGA will disable the custom ROM
        LD	A, D
        OUT	($07), A 	; Notify FPGA we are now to return (MSB) 
        LD	A, E
        OUT	($09), A 	; Notify FPGA we are now to return (LSB)
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
        
ROM_CHECK:
	LD	A,$07		; select a white border
	OUT	($FE),A		; and set it now.
	LD	A,$3F		; load accumulator with last page in ROM.
	LD	I,A		; set the I register - this remains constant
				; and can't be in range $40 - $7F as 'snow'
				; appears on the screen.
RAM_CHECK:	
	LD	H,D		; transfer the top value to
	LD	L,E		; the HL register pair.
					;;;$11DC
RAM_FILL:
	LD	(HL),$02	; load with 2 - red ink on black paper
	DEC	HL		; next lower
	CP	H		; have we reached ROM - $3F ?
	JR	NZ,RAM_FILL	; back to RAM_FILL if not.
RAM_READ:	
	AND	A		; clear carry - prepare to subtract
	SBC	HL,DE		; subtract and add back setting
	ADD	HL,DE		; carry when back at start.
	INC	HL		; and increment for next iteration.
	JR	NC,RAM_DONE	; forward to RAM_DONE if we've got back to
				; starting point with no errors.
	DEC	(HL)		; decrement to 1.
	JR	Z,RAM_DONE	; forward to RAM_DONE if faulty.

	DEC	(HL)		; decrement to zero.
	JR	Z,RAM_READ	; back to RAM_READ if zero flag was set.

RAM_DONE:
	DEC	HL
        DEC	HL
        LD	SP, HL
        
        LD	HL, $4000
        LD	D, $BF
        LD	B, $FF
LOADER:
	IN	A, ($0B)
        OR	A
        JR 	NZ, LOADER
        IN	A, ($0D)
        LD	(HL), A
	INC	HL
        DJNZ	LOADER
        DEC 	D
        JR 	NZ, LOADER
        JP	RESTOREREGS

