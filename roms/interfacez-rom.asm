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

	JR	RAMCOPY	


	ORG	$0066
NMIH:	PUSH 	AF
	JP	NMIHANDLER

RAMCOPY:
	DI
	EX	AF, AF'
        EXX

        LD	C, PORT_RAM_ADDR_0
        IN	D, (C) ; RAM0:D 
        LD	A, $00
        OUT	(C), A

        LD	C, PORT_RAM_ADDR_1
        IN	E, (C) ; RAM1: E
        OUT	(C), A
        LD	C, PORT_RAM_ADDR_2
        IN	L, (C) ; RAM1: C (L temporary)
        LD	A, $02
        OUT	(C), A
        LD	C, L
        LD	HL, SCREEN

IF 0

	;	6912 bytes. 216 blocks of 32 bytes, 108 blocks of 64 bytes, 54 blocks of 128 bytes.
        LD	A, 54
_loop1:
        REPT	1                 	; T16, T2048 total
          INI
        ENDM
        DEC	A     			; T4
        JP	NZ, _loop1              ; T10   (sum 111348)
        

        LD	L, C
        LD	C, PORT_RAM_ADDR_0
        OUT	(C), D ; RAM0:D 
        LD	C, PORT_RAM_ADDR_1
        OUT	(C), E
        LD	C, PORT_RAM_ADDR_2
        IN	L, (C) ; RAM1: L


        OUT	(C), L

ENDIF

	LD	BC, $FB6B
        OUT	(C), A

	EXX
        EX	AF, AF'
	;EI
	RETI                          	; 32ms



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

	include "debug.asm"
	include "menu_defs.asm"
        include "string.asm"
        include "keyboard.asm"
	include	"charmap.asm"
        include	"resource.asm"
        include	"graphics.asm"
        include	"print.asm"
        include	"regdebug.asm"
	include "keybtest.asm"
        include "utils.asm"
	include "nmihandler.asm"
	include "snaram.asm"
       
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
