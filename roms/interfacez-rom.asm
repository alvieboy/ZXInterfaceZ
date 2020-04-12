		
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
        CALL 	PRINTSTRINGN


	LD	HL, COPYRIGHT
        LD	DE, LINE22
        CALL 	PRINTSTRING


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
        LD	(IX+MENU_OFF_FIRST_ENTRY),A ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+1), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+2), H

        LD	HL, ENTRY2
        LD	A,1
        LD	(IX+MENU_OFF_FIRST_ENTRY+3),A ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+4), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+5), H
        
        XOR	A
        LD	HL, ENTRY3
        LD	(IX+MENU_OFF_FIRST_ENTRY+6),A ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+7), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+8), H
        
        LD	HL, ENTRY4
        LD	(IX+MENU_OFF_FIRST_ENTRY+9),A ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+10), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+11), H

	LD	(IX+MENU_OFF_CALLBACKPTR), low(MENUCALLBACKTABLE)
        LD	(IX+MENU_OFF_CALLBACKPTR+1), high(MENUCALLBACKTABLE)
        RET



        

        include "menu.asm"
        include "keyboard.asm"
	include	"charmap.asm"
        include	"resource.asm"
        include	"graphics.asm"
        include	"print.asm"
        include	"debug.asm"
               ; 00000000001111111111222222222233
               ; 01234567890123456789012345678901
COPYRIGHT:DB	"ZX Interface Z (C) Alvieboy 2020", 0

MENUTITLE:
	DB 	"ZX Interface Z", 0
ENTRY1: DB	"Configure WiFI", 0
ENTRY2: DB	"Load from SD Card", 0
ENTRY3: DB	"Show version", 0
ENTRY4: DB	"Goto BASIC", 0
