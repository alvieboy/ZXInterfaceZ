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
        ; At this point, we already have full ROM active
        PUSH	BC
        
        LD	A, CMD_FASTLOAD
        CALL	WRITECMDFIFO

        EX	AF, AF'
	LD	B, A
        EX	AF, AF'
        
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
        IN	E, (C)
        IN	D, (C)
	; This should match DE. TBD.
        
        ; Copy to IX
_loopcopy:
        IN	A, (C)
        LD	(IX), A
        INC	IX
        DEC 	DE
        LD	A, E
        OR	D
        JR 	NZ, _loopcopy
        ; All done.
        ; Signal we are returning to regular ROM
        SCF
FASTLOADEXIT:
	; Go back to Spectrum, LD_MARKER caller
        POP	BC
	JP	RETTOMAINROM
LOAD_ERROR:
	POP	BC
	CCF
	JP	RETTOMAINROM


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
        JR	Z, _goback
        CALL	NMIHANDLER_FROM_ROM

	PUSH	BC
	CALL	READRESFIFO
        ;CP	$FF
        ; STATUS is ignored for now, will be used in the future
	POP	BC

_goback
	; Go back to Spectrum.
	POP	AF 
	RET_TO_ROM_AT RET0767 ; After PUSH IX
_cancel_load:
	POP	AF
        CCF
	RET_TO_ROM_AT $0552 ; REPORT_DA
