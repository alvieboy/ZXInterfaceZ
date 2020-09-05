; System variables definitions

include	"interfacez-sysvars.asm"

include "macros.asm"

; Main state machine
STATE_UNKNOWN  		EQU 0
STATE_MAINMENU 		EQU 1
STATE_WIFICONFIGAP 	EQU 2
STATE_SDCARDMENU 	EQU 3
STATE_WIFIPASSWORD 	EQU 4
STATE_WIFICONFIGSTA 	EQU 5

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
        ;JP	REGTEST

	ORG	$0008
		      
RST8:	JP 	RST8

	ORG	$0038
					;;;$0038
MASK_INT:	PUSH	AF		; save the registers.
		PUSH	HL		
		LD	HL,(FRAMES1)	; fetch two bytes at FRAMES1.
		INC	HL		; increment lowest two bytes of counter.
	        LD	(FRAMES1),HL	; place back in FRAMES1.
		LD	A,H		; test if the result
		OR	L		; was zero.
		JR	NZ,KEY_INT	; forward to KEY_INT if not.

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
NMIH:	PUSH 	AF
	JP	NMIHANDLER
	;POP AF
        
	;RETN

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
        HALT
IGNORE:	
	RET


GRAPHICS_SDON:
	PUSH 	IX
	LD	IX, ATTR
        LD	(IX+1), %01100000
        LD	(IX+33), %01100000
	POP 	IX
        RET

GRAPHICS_SDOFF:
	PUSH 	IX
	LD	IX, ATTR
        LD	(IX+1), %00111000
        LD	(IX+33), %00111000
	POP	IX
        RET
        
GRAPHICS_WIFION:
        LD	A, %01111001
GR1:
	PUSH 	IX
	LD	IX, ATTR        
        LD	(IX+64), A
        LD	(IX+65), A
        LD	(IX+66), A
        LD	(IX+96), A
        LD	(IX+97), A
        LD	(IX+98), A
        POP 	IX
        RET

GRAPHICS_WIFIOFF:
	LD	A, %01111000
        CALL	GR1
        ; Write "disconnected" area
        LD	HL, DISCONNECTED
	LD	DE, LINE3 + 3
        LD	A, 23
	CALL	PRINTSTRINGPAD
        RET
;	A: 0 (STA) or 1(AP) mode
;	HL:	SSID

GRAPHICS_WIFIPRINT:
	LD	DE, LINE3 + 2
        LD	C, %01111000 ; Normal color
	CP	0
        LD	A, 23 ; for string padding
        JR	Z, _stamode
        PUSH	HL
        LD	A, 'A'
        CALL	PRINTCHAR
        LD	A, 'P'
        CALL	PRINTCHAR
        INC	DE
        POP	HL
        LD	C, %00001110 ; Color for AP mode text
        LD	A, 20 ; For string padding
_stamode:
	CALL	PRINTSTRINGPAD
        PUSH 	IX
        LD	IX, ATTR+96+3
        LD	A, C
        LD	(IX), A
        LD	(IX+1), A
        POP 	IX
        RET

GETSSID:
	LD	HL, SSID
        LD	A, $04
        CALL	LOADRESOURCE
        JR	Z, INTERNALERROR
        LD	(HL), 0 ; NULL-terminate the string.
        RET

CALLTABLE:
        ADD	A, A ; a = a*2
        ADD_HL_A
	LD	A, (HL)
        INC 	HL
	LD	H, (HL)
	LD	L, A
        PUSH	HL
        RET
    


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

INTERNALERROR:
	DEBUGHEXA
	LD	DE, LINE23
        LD	HL, INTERNALERRORSTR
        LD	A, 32
        CALL	PRINTSTRINGPAD
	LD	DE, LINE22
        LD	HL, EMPTYSTRING
        LD	A, 32
        CALL	PRINTSTRINGPAD
_endl1: HALT
	JR _endl1

        include	"debug.asm"
	include "snaram.asm"
	include "menu_defs.asm"
        include "frame.asm"
        include "textinput.asm"
        include "textmessage.asm"
        include "string.asm"
        include "alloc.asm"
        ;include "mainmenu.asm"
        ;include "wifimenu.asm"
        ;include "sdcardmenu.asm"
        ;include "nmimenu.asm"
        include "keyboard.asm"
	include	"charmap.asm"
        include	"resource.asm"
        include	"graphics.asm"
        ;include "videomode.asm"
        include	"print.asm"
        include	"regdebug.asm"
	include "keybtest.asm"
        ;include "widget.asm"
        include "object.asm"
        include "widget_class.asm"
        include "widgetgroup.asm"
        include "bin.asm"
        include "menu.asm"
        include "callbackmenu.asm"
        include "window.asm"
        include "menuwindow.asm"
        include "screen.asm"
        include "utils.asm"
        include "stringmenu.asm"
        include "filechooser.asm"
        
	include "nmihandler.asm"
        include "settingsmenu.asm"

       ; include "filechooser.asm"
       
               ; 00000000001111111111222222222233
               ; 01234567890123456789012345678901
INTERNALERRORSTR: DB "Internal ERROR, aborting" ; Fallback to EMPTYSYRING
EMPTYSTRING:	DB 0
COPYRIGHT:DB	"ZX Interface Z (C) Alvieboy 2020", 0

PASSWDTMP: DB "Spectrum", 0
DISCONNECTED:	DB	"Disconnected", 0
SCANNING:	DB	"Scanning...", 0


; THIS MUST BE THE LAST ENTRY!!!!
	ORG ROM_PATCHED_SNALOAD
	include "snarestore.asm"
