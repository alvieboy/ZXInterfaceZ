include "macros.asm"

; SA-LEADER
;
ORG $04D7
	LD	B, A ; As in original ROM

	PUSH	AF
        PUSH	BC
        
	LD	A, CMD_SAVETRAP
        CALL	WRITECMDFIFO
	LD	A, D
        CALL    WRITECMDFIFO
	LD	A, E
        CALL    WRITECMDFIFO
        
        ; We have type in A'
        EX	AF, AF'
        LD	C, A
        EX	AF, AF'
        LD	A, C

        CALL    WRITECMDFIFO
        ; For headers, we immediatly send header data
        CP	$00
        JR	NZ, _nodata
        CALL	SENDTAPEDATAFIFO
_nodata:	
        ; Wait for acknowledle
	CALL	READRESFIFO
        CP	$FF
        POP	BC
        
        ; Enter menu
        CALL    NZ, NMIHANDLER_FROM_ROM

	; Check if we need to send data.
        
        PUSH	BC
        CALL	READRESFIFO
        ; At this point:
        CP	$FF
        POP	BC
        JR	Z, _goback

        ; TBD: Write data to RAM.
	PUSH	IX
        PUSH	DE
        ; Set up pointer
        LD	A, $02
        OUT	(PORT_RAM_ADDR_2), A
        LD	A, $80
        OUT	(PORT_RAM_ADDR_1), A
        XOR	A
        OUT	(PORT_RAM_ADDR_0), A
        
        PUSH	BC
        EX	AF, AF'
        LD	C, A
        EX	AF, AF'
        LD	A, C
        POP	BC
        ; Send block type first
        OUT	(PORT_RAM_DATA), A
	; And skip first byte at (IX)
	INC	IX

_wloop:	LD	A, (IX)
	INC	IX
        OUT	(PORT_RAM_DATA), A
        DEC	DE
        LD	A, E
        OR	D                              
        JR	NZ, _wloop
        POP	DE
        POP	IX
        ; At this point we wrote all required data.
        
	LD	A, CMD_SAVEDATA
        CALL	WRITECMDFIFO
	LD	A, D
        CALL    WRITECMDFIFO
	LD	A, E
        CALL    WRITECMDFIFO


        POP	AF
        
        RET_TO_ROM_AT $053E
_goback:
	POP	AF
        RET_TO_ROM_AT $04D8


SENDTAPEDATAFIFO:
       ; Need to send data to FIFO
        PUSH   IX
        PUSH    DE
_write:
        LD     A, (IX)
        CALL   WRITECMDFIFO
        INC    IX
        DEC    DE
        LD     A, E
        OR     D
        JR     NZ, _write

        POP    DE
        POP     IX
        RET
