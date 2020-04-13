; System variables definitions

include	"interfacez-sysvars.asm"
include "macros.asm"


; Main state machine
STATE_MAINMENU 		EQU 0
STATE_WIFICONFIG 	EQU 1
STATE_SDCARDMENU 	EQU 2
STATE_UNKNOWN  		EQU $FF

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


	;LD	HL,$0523	; The keyboard repeat and delay values
	;LD	(REPDEL),HL	; are loaded to REPDEL and REPPER.
        ;LD	A, $FF
        ;LD	(KSTATE_0),A	; set KSTATE_0 to $FF.
        ;LD	(KSTATE_4),A	; set KSTATE_0 to $FF.

	; Setup default state.
        LD	A, STATE_UNKNOWN
        LD	(STATE), A ; State STATE_UNKNOWN

  	IM 	1
        LD	IY, IYBASE
        EI
        
        LD	A, STATE_MAINMENU
        CALL	ENTERSTATE
        
        
ENDLESS:
	CALL	SYSTEM_STATUS
	CALL	KEY_INPUT
        HALT
       	JR 	ENDLESS 


STATUSCHANGEDHANDLERTABLE:
	DEFW	MAINMENU__STATUSCHANGED
        DEFW	WIFICONFIG__STATUSCHANGED
        DEFW	SDCARDMENU__STATUSCHANGED

HANDLEKEYANDLERTABLE:
	DEFW	MAINMENU__HANDLEKEY
        DEFW    WIFICONFIG__HANDLEKEY
        DEFW	SDCARDMENU__HANDLEKEY

	; Enter new state in A
ENTERSTATE:
	; Ignore if we are already on that state.
        ;CP	(STATE)
	PUSH 	AF
        ;
        ; Check previous state.
	;
        LD	A, (STATE)
        CP	STATE_MAINMENU  ; Coming from main menu?
        JR	NZ, _nl1        ; No.
        
        LD	HL, MENU1    	; Yes. Coming from main menu, clear it.
        LD	A, $38
        CALL	MENU__CLEAR
        JR	_nl2
_nl1:   CP	STATE_SDCARDMENU  ; Coming from SD card menu?
        JR	NZ, _nl2        ; No.

        LD	HL, (SDMENU)  	; Yes. Coming from SD menu, clear it.
        LD	A, $38
        CALL	MENU__CLEAR


        
_nl2:
        POP	AF
        LD	(STATE), A ; Set up state.
        ;
        ;
        ; Check new state
        CP	STATE_MAINMENU
        JR 	NZ, _l1
	; ENTER STATE: MAINMENU

	CALL	MAINMENU__SETUP
	LD	HL, MENU1
        LD	D, 6 ; line to display menu at.
        CALL	MENU__INIT
	CALL	MENU__DRAW
        
        JR	 _lend
_l1:   	CP	 STATE_SDCARDMENU
	JR	NZ, _l1
       	CALL	SDCARDMENU__SETUP
	
        LD	HL, (SDMENU)
        LD	D, 5 ; line to display menu at.
        CALL	MENU__INIT
        
	CALL	MENU__DRAW


        JR	_lend
_lend:
	RET
        
SYSTEM_STATUS:
	LD	A, $2           ; Load resource #2 (status flag)
        LD	HL, STATUSBUF        ; Into our STATUSBUF area
        CALL	LOADRESOURCE
        CP 	$FF             ; If not valid (can this happen?) exits.
        RET 	Z
        LD	IX, STATUSBUF        ; Check system status flags. Use IX for dereferencing with BIT later on
        LD	A, (PREVSTATUS)
        XOR	(IX)            ; Compute differences between this status and prev. status
        CP	0
        CALL	NZ, PROCESSSTATUSCHANGE	; Process status change if anything changed

        LD	A, (STATUSBUF) 	; Store status for next check	
	LD	(PREVSTATUS), A
        
        RET

PROCESSSTATUSCHANGE:
	LD	HL, STATUSCHANGEDHANDLERTABLE
	JR	CALLSTATUSFUN
HANDLEKEY:
	LD	HL, HANDLEKEYANDLERTABLE
        ; Fallback
CALLSTATUSFUN:
	LD	B, A
	LD	A, (STATE)
        ADD	A, A ; a = a*2
        ADD_HL_A
	LD	A, (HL)
        INC 	HL
	LD	H, (HL)
	LD	L, A
        PUSH	HL
        LD	A, B
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

INTERNALERROR:
	LD	DE, LINE23
        LD	HL, INTERNALERRORSTR
        LD	A, 32
        CALL	PRINTSTRINGPAD
	LD	DE, LINE22
        LD	HL, EMPTYSYRING
        LD	A, 32
        CALL	PRINTSTRINGPAD
_endl1: HALT
	JR _endl1

	include "menu_defs.asm"
        include "menu.asm"
        include "mainmenu.asm"
        include "wifimenu.asm"
        include "sdcardmenu.asm"
        include "keyboard.asm"
	include	"charmap.asm"
        include	"resource.asm"
        include	"graphics.asm"
        include	"print.asm"
        include	"debug.asm"

               ; 00000000001111111111222222222233
               ; 01234567890123456789012345678901
INTERNALERRORSTR: DB "Internal ERROR, aborting" ; Fallback to EMPTYSYRING
EMPTYSYRING:	DB 0
COPYRIGHT:DB	"ZX Interface Z (C) Alvieboy 2020", 0

MENUTITLE:
	DB 	"ZX Interface Z", 0
ENTRY1: DB	"Configure WiFI", 0
ENTRY2: DB	"Load from SD Card", 0
ENTRY3: DB	"Show version", 0
ENTRY4: DB	"Goto BASIC", 0
ENTRY5: DB	"Hidden entry", 0
