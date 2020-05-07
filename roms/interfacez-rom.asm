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
NMIH:	PUSH AF
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
	DEC	HL
        DEC	HL
        LD	SP, HL
        
        LD	HL, $4000
        LD	D, $BF
        LD	B, $FF

        CALL 	SETATTRS
	
        ;
        ; DEBUG ONLY
        ;
        ;JP 	EXTRAMTEST
        JP 	KEYBTEST
        
        LD	A, $7
        OUT	($FE),A


	LD	HL, HEAP
        LD	A, RESOURCE_ID_VERSION
        CALL 	LOADRESOURCE
        JP	Z, 0
        
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
        JP	Z, 0

	LD	HL, HEAP
        LD	DE, SCREEN
        CALL	DISPLAYBITMAP

        ; Attribute settins.
        ; Lines 1 to 4 have bright.
        
        LD	DE, ATTR
        LD	B, 128
        LD	A, %01111000
_a1:
	LD	(DE), A
        INC	DE
        DJNZ    _a1
        
        CALL	GRAPHICS_SDOFF
        CALL	GRAPHICS_WIFIOFF
	
        LD 	HL, DISCONNECTED
        LD	A, 1
        CALL	GRAPHICS_WIFIPRINT
        
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
        XOR 	A  	
        LD	(STATE), A
        LD	A, STATE_MAINMENU
        CALL	ENTERSTATE
        
        
_loop:
	CALL	SYSTEM_STATUS
	CALL	KEY_INPUT
        HALT
       	JR 	_loop


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

STATUSCHANGEDHANDLERTABLE:
        DEFW	IGNORE
	DEFW	MAINMENU__STATUSCHANGED
        DEFW	WIFICONFIG__STATUSCHANGED
        DEFW	SDCARDMENU__STATUSCHANGED
        DEFW	IGNORE
        DEFW	IGNORE

HANDLEKEYANDLERTABLE:
        DEFW	IGNORE
	DEFW	MAINMENU__HANDLEKEY
        DEFW    WIFICONFIG__HANDLEKEY
        DEFW	SDCARDMENU__HANDLEKEY
        DEFW	IGNORE
        DEFW	IGNORE

ENTERSTATEHANDLERTABLE:
        DEFW	IGNORE
	DEFW	ENTER_MAINMENU
        DEFW 	ENTER_WIFICONFIGAP
        DEFW	ENTER_SDCARD
        DEFW	ENTER_WIFICONFIGSTA
        DEFW	IGNORE

LEAVESTATEHANDLERTABLE:
        DEFW	IGNORE
	DEFW	LEAVE_MAINMENU
        DEFW 	LEAVE_WIFICONFIGAP
        DEFW	LEAVE_SDCARD
        DEFW	LEAVE_WIFICONFIGSTA
        DEFW	IGNORE


LEAVE_MAINMENU:
        LD	HL, MENU1
        LD	A, $38
        JP	MENU__CLEAR

LEAVE_SDCARD:
        LD	HL, (SDMENU)
        LD	A, $38
        JP	MENU__CLEAR

ENTER_MAINMENU:
	CALL	MAINMENU__SETUP
	LD	HL, MENU1
        LD	D, 6 ; line to display menu at.
        CALL	MENU__INIT
	JP	MENU__DRAW

ENTER_SDCARD:
       	CALL	SDCARDMENU__SETUP	
        LD	HL, (SDMENU)
        LD	D, 5 ; line to display menu at.
        CALL	MENU__INIT
	JP	MENU__DRAW
        
LEAVE_WIFICONFIGAP:
	RET

ENTER_WIFICONFIGAP:
	LD	HL, SCANNING
	CALL 	TEXTMESSAGE__SHOW
        LD	A, 0 
        LD	(WIFIFLAGS), A
        ; Request scan
        LD	A, $02
        JP	WRITECMDFIFO

ENTER_WIFICONFIGSTA:
	RET
LEAVE_WIFICONFIGSTA:
	RET
        
	; Enter new state in A
ENTERSTATE:
	; Ignore if we are already on that state.
	PUSH 	AF
        LD	HL, LEAVESTATEHANDLERTABLE
        LD	A, (STATE)
        CALL	CALLTABLE
        POP	AF
        
        LD	(STATE), A ; Set up new state.
        LD	HL, ENTERSTATEHANDLERTABLE
        JP	CALLTABLE
        
SYSTEM_STATUS:
	LD	A, $2           ; Load resource #2 (status flag)
        LD	HL, STATUSBUF        ; Into our STATUSBUF area
        CALL	LOADRESOURCE
        CP 	RESOURCE_TYPE_INTEGER  ; If not valid (can this happen?) exits.
        JP	NZ, INTERNALERROR
        
        LD	IX, CURRSTATUS   ; Check system status flags. Use IX for dereferencing with BIT later on
        LD	HL, STATUSBUF        ; Into our STATUSBUF area
        LD	A, (HL)
        LD	(IX), A
        LD	A, (PREVSTATUS)
        XOR	(IX)            ; Compute differences between this status and prev. status
        LD	(IX+1), A	; Save STATUSXOR
        CP	0
        CALL	NZ, PROCESSSTATUSCHANGE	; Process status change if anything changed

        LD	A, (IX) 	; Store status for next check
	LD	(PREVSTATUS), A
        RET
        
LOCALSTATUSCHANGED:
	BIT	0, (IX+1)                 	; Bit 1 is WIFI Mode flag
        CALL	NZ, LOCALWIFIMODESTATUSCHANGED
	BIT	1, (IX+1)                 	; Bit 1 is WIFI Connected flag
        CALL	NZ, LOCALWIFICONNECTEDSTATUSCHANGED
	BIT	2, (IX+1)                 	; Bit 2 is WIFI Scanning
        CALL	NZ, LOCALWIFISCANNINGSTATUSCHANGED
	BIT	3, (IX+1)                    ; Bit 3 is SD Card connected flag.
        JP	NZ, LOCALSDCARDSTATUSCHANGED
        RET

LOCALWIFIMODESTATUSCHANGED: ; Wifi mode changed (STA<->AP)
        CALL	GETSSID
        BIT	0, (IX) ; Get WIFI mode
        LD	HL, SSID
        JR	Z, _stamode
        ; AP mode
        XOR	A
        JP	GRAPHICS_WIFIPRINT
_stamode:
	LD	A, 1
        JP	GRAPHICS_WIFIPRINT
        
;	RET

LOCALWIFICONNECTEDSTATUSCHANGED: ; Wifi connection status changed (connected/disconnected)
        BIT	1, (IX) ; Get WIFI connect status
        JP	Z, GRAPHICS_WIFIOFF
        ; On. Check WiFI mode
        CALL	GRAPHICS_WIFION
        BIT	0, (IX+1);  IF Wifi mode changed, ignore
        RET	NZ
        JP	LOCALWIFIMODESTATUSCHANGED
	;RET

LOCALWIFISCANNINGSTATUSCHANGED: ; Wifi scanning status changed (scanning/not scanning)
        BIT	2, (IX) ; Get WIFI connect status
	RET

LOCALSDCARDSTATUSCHANGED: ; SD card status changed (present/not present)
        BIT	3, (IX) ; Get SD
        JP	Z, GRAPHICS_SDOFF
        JP	GRAPHICS_SDON

GETSSID:
	LD	HL, SSID
        LD	A, $04
        CALL	LOADRESOURCE
        JR	Z, INTERNALERROR
        LD	(HL), 0 ; NULL-terminate the string.
        RET

PROCESSSTATUSCHANGE:
	PUSH	AF
        ; Local status change.
        CALL	LOCALSTATUSCHANGED
        POP	AF
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
	
        
        
        ; Call a table entry
        ; HL:	table 
        ; A:	table index
CALLTABLE:
        ADD	A, A ; a = a*2
        ADD_HL_A
	LD	A, (HL)
        INC 	HL
	LD	H, (HL)
	LD	L, A
        PUSH	HL
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
	CALL	DEBUGHEXA
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

	include "menu_defs.asm"
        include "menu.asm"
        include "frame.asm"
        include "textinput.asm"
        include "textmessage.asm"
        include "string.asm"
        include "mainmenu.asm"
        include "wifimenu.asm"
        include "sdcardmenu.asm"
        include "nmimenu.asm"
        include "keyboard.asm"
	include	"charmap.asm"
        include	"resource.asm"
        include	"graphics.asm"
	include "nmihandler.asm"
        include	"print.asm"
        include	"debug.asm"
	include "keybtest.asm"
        
               ; 00000000001111111111222222222233
               ; 01234567890123456789012345678901
INTERNALERRORSTR: DB "Internal ERROR, aborting" ; Fallback to EMPTYSYRING
EMPTYSTRING:	DB 0
COPYRIGHT:DB	"ZX Interface Z (C) Alvieboy 2020", 0

PASSWDTMP: DB "Spectrum", 0
DISCONNECTED:	DB	"Disconnected", 0
SCANNING:	DB	"Scanning...", 0
