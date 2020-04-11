		
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

		;INC	(IY+$40)	; otherwise increment FRAMES3 the third byte.

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
        LD	D, 6 ; line to display menu at.
        CALL	MENU__INIT
        
	CALL	MENU__DRAW

	LD	HL, HEAP
        LD	A, $00
        CALL 	LOADRESOURCE
        JR	Z, ENDLESS
        
	LD	HL, HEAP
        LD	DE, LINE23
        ;	Load string lenght
        LD	A,(HL)
        INC	HL
        CALL 	PUTSTRINGN


	LD	HL, COPYRIGHT
        LD	DE, LINE22
        CALL 	PUTSTRING


	; Load image logo

	LD	HL, HEAP
        LD	A, $01
        CALL 	LOADRESOURCE
        CP	$FF
        JR	Z, ENDLESS

	LD	HL, HEAP
        LD	DE, SCREEN
        CALL	DISPLAYBITMAP


	LD	HL,$0523	; The keyboard repeat and delay values
	LD	(REPDEL),HL	; are loaded to REPDEL and REPPER.
        LD	A, $FF
        LD	(KSTATE_0),A	; set KSTATE_0 to $FF.
        LD	(KSTATE_4),A	; set KSTATE_0 to $FF.

  	IM 	1
        LD	IY, IYBASE
        EI
        
ENDLESS:
	CALL	KEY_INPUT
        HALT
       	JR 	ENDLESS 

HANDLEKEY:
	LD	HL, MENU1
	
        CP	$26 	; A key
        JR	NZ, N1
        JP	MENU__CHOOSENEXT
N1:
        CP	$25     ; Q key
        JR	NZ, N2
        JP	MENU__CHOOSEPREV
N2:
        CP	$21     ; ENTER key
        JR	NZ, N3
        JP	MENU__ACTIVATE
N3:
	RET

KEY_INPUT:	
	BIT	5,(IY+(FLAGS-IYBASE))	; test FLAGS  - has a new key been pressed ?
	RET	Z		; return if not.

	LD	DE, (CURKEY)
	LD	A, D
	RES	5,(IY+(FLAGS-IYBASE))	; update FLAGS  - reset the new key flag.
        DEC	A
        RET 	Z 	; Modifier key applied, skip
        LD	A, E
	JR	HANDLEKEY


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

PUTSTRINGN:
	LD	B, A
PS1:
	LD 	A,(HL)
        PUSH	HL
        PUSH	BC
        CALL	PUTCHAR
        POP	BC
        POP	HL
        INC	HL
        DJNZ 	PS1
        RET
        
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

ENTRY1HANDLER:
	LD	HL, MENU1
        LD	A, $38
        CALL	MENU__CLEAR
	RET
ENTRY2HANDLER:
	RET
ENTRY3HANDLER:
	RET
ENTRY4HANDLER:
	JP 0
	RET
        
MENUCALLBACKTABLE:
	DEFW ENTRY1HANDLER
        DEFW ENTRY2HANDLER
        DEFW ENTRY3HANDLER
        DEFW ENTRY4HANDLER

include "menu_defs.asm"

SETUP_MENU1:
       	LD	IX, MENU1
        LD	A, 28  			; Menu width 24
        LD	(IX + MENU_OFF_WIDTH), A
        LD	A, 4                    ; Menu entries
        LD	(IX + MENU_OFF_MAX_VISIBLE_ENTRIES), A
        LD	(IX + MENU_OFF_DATA_ENTRIES), A
        XOR	A
        LD 	(IX+ MENU_OFF_SELECTED_ENTRY), A		; Selected entry
        LD	HL, MENUTITLE
        LD	(IX+MENU_OFF_MENU_TITLE), L
        LD	(IX+MENU_OFF_MENU_TITLE+1), H
        ; Entry 1
        LD	HL, ENTRY1
        LD	(IX+MENU_OFF_FIRST_ENTRY), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+1), H
        LD	HL, ENTRY2
        LD	(IX+MENU_OFF_FIRST_ENTRY+2), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+3), H
        LD	HL, ENTRY3
        LD	(IX+MENU_OFF_FIRST_ENTRY+4), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+5), H
        LD	HL, ENTRY4
        LD	(IX+MENU_OFF_FIRST_ENTRY+6), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+7), H

	LD	(IX+MENU_OFF_CALLBACKPTR), low(MENUCALLBACKTABLE)
        LD	(IX+MENU_OFF_CALLBACKPTR+1), high(MENUCALLBACKTABLE)
        RET

READRESFIFO:
	IN	A, ($0B)
        OR	A
        JR	NZ, READRESFIFO
        ; Grab resoruce data
        IN	A, ($0D)
        RET

;
; Inputs: 	A:  Resource ID
;         	HL: Target memory area
; Returns:
;		A:  Resource type, or $FF if not found
;		DE: Resource size
;		Z:  Zero if resource is invalid.
; Corrupts:	HL, BC, F

LOADRESOURCE:
	LD	C, A
WAITFIFO1:
        IN 	A, ($07)
        OR	A
        JR	NZ, WAITFIFO1
        ; Send resource ID
        LD	A, C
        OUT	($09), A
        
        
        CALL	READRESFIFO
        CP	$FF
        RET	Z	; If invalid, return immediatly
        
        LD	C, A ; Save resource type in C

        CALL	READRESFIFO        
        LD	E, A 	; Resource size LSB

        CALL	READRESFIFO
        LD	D, A 	; Resource size MSB
        
        PUSH	DE	; Save resource size.
	
WAITFIFO3:

        CALL	READRESFIFO
        
        LD	(HL), A
        INC	HL
        DEC	DE
       	LD	A, D
    	OR	E
        JR 	NZ,   WAITFIFO3

        LD	A, C ; Get resource type back in A
	POP 	DE   ; Get resource size back

        CP	$FF
        RET

	; Move DE one pixel line below
PMOVEDOWN:
	INC 	D				; Go down onto the next pixel line
	LD 	A, D				; Check if we have gone onto next character boundary
	AND	7
	RET 	NZ				; No, so skip the next bit
	LD 	A,E				; Go onto the next character line
	ADD 	A, 32
	LD 	E,A
	RET 	C				; Check if we have gone onto next third of screen
	LD 	A,D				; Yes, so go onto next third
	SUB 	8
	LD 	D,A
	RET

;	In: 	HL: pointer to bitmap structure
;		DE: screen address
;
;		Bitmap structure contains:
;			DB 0	// Width in bytes
;			DB 0	// Height in lines
;			Db 0..  // Bitmap data.
DISPLAYBITMAP:
        LD	C, (HL)  ; Width in bytes
        INC	HL
        LD	A, (HL)	 ; Number of lines
        INC	HL
DOLINE:
        LD	B, $0
	PUSH	DE
        PUSH	BC
        LDIR
        POP	BC
        POP	DE
        LD	B, A      ; PMOVEDOWN corrupts A, save it.
	CALL	PMOVEDOWN
        LD	A, B
        DEC	A
        JR	NZ, DOLINE
        RET
        

        include "menu.asm"
        include "keyboard.asm"
	include	"charmap.asm"
               ; 00000000001111111111222222222233
               ; 01234567890123456789012345678901
COPYRIGHT:DB	"ZX Interface Z (C) Alvieboy 2020", 0

MENUTITLE:
	DB 	"ZX Interface Z", 0
ENTRY1: DB	"Configure WiFI", 0
ENTRY2: DB	"Load from SD Card", 0
ENTRY3: DB	"Show version", 0
ENTRY4: DB	"Goto BASIC", 0
