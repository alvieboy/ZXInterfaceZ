		
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

	ORG	$0038
;---------------------------
; Maskable interrupt routine
;---------------------------
; This routine increments the Spectrum's three-byte FRAMES counter
; fifty times a second (sixty times a second in the USA ).
; Both this routine and the called KEYBOARD subroutine use 
; the IY register to access system variables and flags so a user-written
; program must disable interrupts to make use of the IY register.

					;;;$0038
MASK_INT:	PUSH	AF		; save the registers.
		PUSH	HL		; but not IY unfortunately.
		LD	HL,(FRAMES1)	; fetch two bytes at FRAMES1.
		INC	HL		; increment lowest two bytes of counter.
		LD	(FRAMES1),HL	; place back in FRAMES1.
		LD	A,H		; test if the result
		OR	L		; was zero.
		JR	NZ,KEY_INT	; forward to KEY_INT if not.

		INC	(IY+$40)	; otherwise increment FRAMES3 the third byte.

					; now save the rest of the main registers and read and decode the keyboard.

					;;;$0048
KEY_INT:	PUSH	BC		; save the other
		PUSH	DE		; main registers.
		CALL	KEYBOARD	; routine KEYBOARD executes a stage in the process of reading a key-press.
		POP	DE
		POP	BC		; restore registers.
		POP	HL
		POP	AF
		EI			; enable interrupts.
		RET			; return.


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
        
        LD	HL, $4000
        LD	D, $BF
        LD	B, $FF

        CALL 	SETATTRS
	
        LD	A, $1
        OUT	($FE),A

	CALL	SETUP_MENU1
	LD	HL, MENU1
        LD	D, 4 ; line to display menu at.
        CALL	MENU__INIT
        
	CALL	MENU__DRAW



	LD	HL,$0523	; The keyboard repeat and delay values
	LD	(REPDEL),HL	; are loaded to REPDEL and REPPER.
        LD	A, $FF
        LD	(KSTATE_0),A	; set KSTATE_0 to $FF.
        LD	(KSTATE_4),A	; set KSTATE_0 to $FF.

  	IM 	1
        LD	IY, ERR_NR
        EI
        
ENDLESS:CALL	KEY_INPUT
       	JR 	ENDLESS 

MENUSELECTED:
	RET

HANDLEKEY:
	LD	HL, MENU1
	
        CP	$90
        JR	NZ, N1
        JP	MENU__CHOOSENEXT
N1:
        CP	$A0
        JR	NZ, N2
        JP	MENU__CHOOSEPREV
N2:
        CP	$0D
        JR	NZ, N3
        JP	MENUSELECTED
N3:
	RET

KEY_INPUT:	
	BIT	5,(IY+$01)	; test FLAGS  - has a new key been pressed ?
	RET	Z		; return if not.

	LD	A,(LASTK)	; system variable LASTK will hold last key -
					; from the interrupt routine.
	RES	5,(IY+$01)	; update FLAGS  - reset the new key flag.
	JR	HANDLEKEY
        ;RET



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

;
; Print HEX byte in A.
;
; Inputs
; 	DE:	Target screen address

PRINTHEX:
	PUSH 	BC
	LD 	B, A
        SRL     A
        SRL     A
        SRL     A
        SRL     A
        CALL	PUTNIBBLE
        LD	A, B
        AND	$F
        CALL 	PUTNIBBLE
        POP	BC
        RET


; Draw a simple H line.
; DE: start address
;  B: size
; Corrupts: A
DRAWHLINE:
	LD	(DE), A
	LD	A, $FF
        INC	DE
        DJNZ	DRAWHLINE
        RET

PUTSTRING:
	LD 	A,(HL)
        OR	A
        RET 	Z
        PUSH	HL	
        CALL	PUTCHAR
        POP	HL
        INC	HL
        JR 	PUTSTRING
        
PUTNIBBLE:
        CP	10
        JR	NC, NDEC
	ADD 	A, '0'
        JR	PUTCHAR
NDEC:   ADD	A, 'A'-10
        JR 	PUTCHAR
        
;
;      DE: screen address
;	A: char 
PUTCHAR:
	SUB	32
        PUSH 	BC
        PUSH 	DE
        LD	BC, CHAR_SET
        LD	H,$00		; set high byte to 0
	LD	L,A		; character to A
        ADD	HL,HL		; multiply
	ADD	HL,HL		; by
        ADD	HL,HL		; eight
        ADD	HL, BC
        LD	B, 8
PCLOOP1:
	LD	A, (HL)
        LD	(DE), A
        INC	D
        INC 	HL
        DJNZ 	PCLOOP1
        POP	DE
        INC 	DE
        POP	BC
        RET

PUTRAW:
	PUSH	BC
        PUSH	DE
	LD	B, 8
PCLOOP2:
	LD	A, (HL)
        LD	(DE), A
        INC	D
        INC 	HL
        DJNZ 	PCLOOP1
        POP	DE
        INC 	DE
        POP	BC
        RET

MOVEDOWN:
	LD	A,E
	CP 	%11100000 	      	; 0xE0, last entry of the band	
	JR	NC, NEXTBAND$           ; Yes, move to next band
	ADD	A,$20                   ; Othewise just increment
	LD	E, A
	LD	A, D
	AND 	%01011000               ; Make sure we don't overflow
	LD	D,A 
	RET        
NEXTBAND$:
	AND 	%00011111		; Blanks out y coords of LSB
	LD	E,A			; copy back to destination LSB
	LD	A,D			; get destination MSB
	ADD 	A,$08	                ; increment band bit - this will push us into attribute space if we're already in the last band
	AND 	%01011000		; 0101 1000 - ensures 010 prefix is maintained, wiping out any carry. Also wipes Y pixel offset.
	LD 	D,A			; copy back into destination MSB	
	RET

SETUP_MENU1:
       	LD	IX, MENU1
        LD	A, 28  			; Menu width 24
        LD	(IX), A
        LD	A, 4                    ; Menu entries
        LD	(IX+1), A
        XOR	A
        LD 	(IX+2), A		; Selected entry
        ; Leave screen out.
        LD	HL, MENUTITLE
        LD	(IX+7), L
        LD	(IX+8), H
        LD	HL, ENTRY1
        LD	(IX+9), L
        LD	(IX+10), H
        LD	HL, ENTRY2
        LD	(IX+11), L
        LD	(IX+12), H
        LD	HL, ENTRY3
        LD	(IX+13), L
        LD	(IX+14), H
        LD	HL, ENTRY4
        LD	(IX+15), L
        LD	(IX+16), H
        RET

        include "menu.asm"
        include "keyboard.asm"
	include	"charmap.asm"



MENUTITLE:
	DB 	"ZX Interface Z", 0
ENTRY1: DB	"Configure WiFI", 0
ENTRY2: DB	"Load from SD Card", 0
ENTRY3: DB	"Show version", 0
ENTRY4: DB	"Goto BASIC", 0
