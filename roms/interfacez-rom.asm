; System variables definitions

include	"interfacez-sysvars.asm"
include "macros.asm"

	ORG	$0000
			       
START:	DI			; disable interrupts.
	;LD	DE,$FFFF	; top of possible physical RAM.
	;JP	ROM_CHECK	; jump forward to common code at START_NEW.
        JP	DELAY
        ;JP	REGTEST

ORG $0006
	DW	FREEAREA
ORG	$0008
		      
RST8:	EX	(SP), HL	; Get return address in HL
	LD	A, (HL)		; Load syscall
        SUB	$C0
        JR	C, _std8handler
        ; Valid syscall number
        ; Fix return address
        INC	HL
        EX	(SP), HL
        JR	handlesyscall
_std8handler:
        EX	(SP), HL	; Place it back
        LD	HL, CH_ADD
        RET_TO_ROM_AT	$000B
        
	; Original ROM:	LD	HL,(CH_ADD)	; fetch the character address from CH_ADD.
	;		LD	(X_PTR),HL	; copy it to the error pointer X_PTR.
	;   		JR	ERROR_2		; forward to continue at ERROR_2.
handlesyscall:
	
        ; Check bounds: TODO
        
        PUSH	HL
        
        LD	HL, SYSCALLTABLE
        ADD	A, A ; Two bytes per sysall
        ADD_HL_A
        ; Load it into stack
        LD	A, (HL)
        INC	HL
        LD	H, (HL)
        LD	L, A
        
        
        
        EX	(SP), HL ; Restores HL
        
        RET	     ; Jump into syscall

ORG	$0038
	EI
        RET


	ORG	$0066

NMIH:	PUSH 	AF
	JP	NMIHANDLER_FROM_NMIREQUEST


	ORG 	$0080
DELAY:
	; Delay for 200ms, because the reset might be asserted quick enough.
        LD	BC, $4E20
_dlyloop: ; (T=35), 10us
        NOP                     ; T=4
        DEC 	BC              ; T=6
        LD 	A,B             ; T=9
        OR 	C               ; T=4
        JR 	NZ, _dlyloop    ; T=12

	; Check for special command
;        IN	A, (PORT_MISCCTRL)
;        OR	A
;        JR	NZ, _bootcommand
        JR	DETECT
	;; 128K reset hook. This area is never executed in 48K mode.
ORG $00C7
	LD	A, CMD_128RESET
        CALL	WRITECMDFIFO	; Notify firmware we are about to reset.
        LD   	BC,$7FFD     	;
        LD   	A, $30       	; Force ROM to 48K, block any further changes
        DI                	; Disable interrupts whilst switching ROMs.
        OUT  (C),A        	; Switch to the other ROM.
        EI                	;
        RET_TO_ROM_AT $0000	; Reset! !


DETECT:
	; Notify CRC
	LD	A, CMD_ROMCRC
        CALL 	WRITECMDFIFO
        LD	A, 0
        CALL 	WRITECMDFIFO

	; Perform a CRC on current ROM.
        LD	DE, ROMCRC_RAM
        LD	HL, ROMCRC_ROM
        LD	BC, ROMCRC_SIZE
        LDIR
        CALL	ROMCRC_RAM
        ; Next ROM
        LD	BC, $7FFD
        LD      A, $10 ; Bit 4 set (48K ROM).
        OUT	(C), A

	LD	A, CMD_ROMCRC
	CALL   	WRITECMDFIFO
        LD	A, 1
        CALL 	WRITECMDFIFO

        CALL	ROMCRC_RAM
        LD	BC, $7FFD
        LD      A, $00 ; Bit 4 reset
        OUT	(C), A

	; Attempt to detect machine type.
 	LD	A, CMD_SPECTRUMDETECT
        CALL	WRITECMDFIFO

        LD	BC, $7FFD
        XOR	A
        OUT	(C), A              	; 0x7FFD: select page 0.
        LD	A, $AA
        LD	(RAM128PAGESTART), A    ; Write 0xAA to @C000 (page 0)
        LD	A, $01 			; 0x7FFD: select page 1
        OUT	(C), A
        LD	A, $55                  ; Write 0x55 to @C000 (page 1)
        LD	(RAM128PAGESTART), A
        XOR	A
        OUT	(C), A                  ; 0x7FFD: select page 0
        LD	A, (RAM128PAGESTART)    ; Read out @C000.
        CP	$55                     ; If we read back 0x55, we don't have paging 
        JR	Z, _is_16k_48k
        CP	$AA	                ; Paging enabled, Check if page1 OK
        JR	NZ, _memoryerror
        ; at this point, a 128K or similar
        LD	A, $01
        CALL 	WRITECMDFIFO
        JR	DETECT_YM
_is_16k_48k:
        LD	A, $00
        CALL 	WRITECMDFIFO
        JR	DETECT_YM
_memoryerror:
        LD	A, $FF
        CALL 	WRITECMDFIFO
        CALL 	WRITECMDFIFO
        ENDLESS



DETECT_YM:
	LD	BC, $C001
        LD	A, $07	; Volume register
        OUT	(C), A
        LD	B, $80
        LD	A, $3F  ; Channel audio disabled, IO ports as input.
        OUT	(C), A
        IN	D, (C)  ; For +2A and beyond, this will also report the register contents
        LD	B, $C0
        IN	A, (C)  ; Read back
        CP	$3F
        LD	A, $01
        JR	Z, _ymfound
        DEC	A
_ymfound:
	LD	C, A
        ; Now, check if D also holds 3F
        LD	A, D
        CP	$3F
        LD	A, C
        JR	NZ, _2plus
        ; Same value from both ports, it's a 2A+, 3 series
        SET	1, A
_2plus:

	CALL	WRITECMDFIFO

        ENDLESS

        
	
;INTERNALERROR:
;	DEBUGHEXA
;	LD	DE, LINE23
;        LD	HL, INTERNALERRORSTR
;        LD	A, 32
;        CALL	PRINTSTRINGPAD
;	LD	DE, LINE22
;        LD	HL, EMPTYSTRING
;        LD	A, 32
;        CALL	PRINTSTRINGPAD
_endl1: HALT
	JR _endl1


        
        

        include "syscall.asm"
	include "debug.asm"
        include "string.asm"
        include "keyboard.asm"
        ;include	"graphics.asm"
;        include	"print.asm"
        include "utils.asm"
        ; WARNING WARNING -  this does need correct placement in ROM
        ; Make sure it does not overlap with other routines.
        include "savepatch.asm"
        include "loadpatch.asm"
	include "kinject.asm"      	

        include	"resource.asm"
        include "romcrc.asm"
	include "nmihandler.asm"
;	include	"charmap.asm"
;	include "keybtest.asm"
	include "snaram.asm"
        include "z80restore.asm"

               ; 00000000001111111111222222222233
               ; 01234567890123456789012345678901
INTERNALERRORSTR: DB "Internal ERROR, aborting" ; Fallback to EMPTYSYRING
EMPTYSTRING:	DB 0

FREEAREA	EQU	$
	LD	HL, 1234
        LD	(HL), $22

ORG 	ROM_PATCHED_SNALOAD - 64
	include "version.asm"

; THIS MUST BE THE LAST ENTRY!!!!
	include "snarestore.asm"
