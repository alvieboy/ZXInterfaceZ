		
;                OUTPUT	"INTZ.ROM"

; System variables definitions

		include	"interfacez-sysvars.asm"

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
        

        CALL 	SETATTRS

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

ENDLESS:
	JR 	ENDLESS

SETATTRS:
        LD	HL, ATTR
        LD	D, $3
        LD	B, $00
        LD	A, $38
ALOOP:
        LD	(HL), A
	INC	HL
        DJNZ	ALOOP
        DEC 	D
        JR 	NZ, ALOOP
        RET
