include "macros.asm"

ORG $056C
	JR $059F

ORG $059E
	NOP

ORG $05C8
	; Load handler - LD_MARKER
        ; AF'	: block or data
        ; DE 	: block length
        ; IY	: Target memory address

L_WAITFIFO1:
        IN 	A, (PORT_CMD_FIFO_STATUS)
        OR	A
        JR	NZ, L_WAITFIFO1
        
        LD	A, CMD_FASTLOAD
        OUT	(PORT_CMD_FIFO_DATA), A

        ; At this point, we need to wait until the full ROM is activated.
        ; This is because we do not have enough room in LD_BYTES to have a full load routine.
        LD	HL, MARKER
_wait:
        LD	A, (HL)
        CP	$08 ; EX AF, AF' instruction
        JR	NZ, _wait

MARKER:
        EX	AF, AF'
	LD	B, A
        LD	(IX), $FE ; Invalidate block
        ; Clear status flag at 0x028000
        LD	A, $02
        OUT	(PORT_RAM_ADDR_2), A
        LD	A, $80
        OUT	(PORT_RAM_ADDR_1), A
        XOR	A
        OUT	(PORT_RAM_ADDR_0), A
        ;
        OUT	(PORT_RAM_DATA), A ; clear status flag
        
	LD	A, CMD_FASTLOAD_DATA
        CALL	WRITECMDFIFO
        LD	A, B
        CALL	WRITECMDFIFO
        LD	A, E
        CALL	WRITECMDFIFO
        LD	A, D
        CALL	WRITECMDFIFO
        ; Wait for completion
_retry:
        XOR	A
        OUT	(PORT_RAM_ADDR_0), A
        IN	A, (PORT_RAM_DATA)
        CP	0
        JR	Z, _retry
        CP	$FF
        JR	Z, LOAD_ERROR
        ; Data block OK.
        
        ; Read in length
        LD	C, PORT_RAM_DATA
        IN	L, (C)
        IN	H, (C)
	; This should match DE. TBD.
        
        ; Copy to IX
_loopcopy:
        IN	A, (C)
        LD	(IX), A
        INC	IX
        DEC 	HL
        LD	A, L
        OR	H
        JR 	NZ, _loopcopy
        ; All done.
        ; Signal we are returning to regular ROM
        SCF
FASTLOADEXIT:
        LD	A, 1
        OUT	(PORT_NMIREASON), A 	; Reused address.
        RET
LOAD_ERROR:
        LD	A, 1
        OUT	(PORT_NMIREASON), A 	; Reused address.
	CCF
        RET


;
; Hook to capture 'LOAD ""'
;
ORG	$0767
        PUSH	IX ; Same as 48K ROM
RET0767:
	PUSH	AF
	PUSH	BC
	
        LD	A, CMD_LOADTRAP
        CALL	WRITECMDFIFO
        ; Wait for acknowledle

	CALL	READRESFIFO
        CP	$FF
	POP	BC

        CALL	NZ, NMIHANDLER_FROM_ROM

	PUSH 	BC
        CALL	READRESFIFO
        CP	$FF
        POP	BC
        ;JR	Z, _cancel_load


	; Go back to Spectrum.
	POP	AF 
	RET_TO_ROM_AT RET0767 ; After PUSH IX
_cancel_load:
	POP	AF
        CCF
	RET_TO_ROM_AT $0552 ; REPORT_DA
