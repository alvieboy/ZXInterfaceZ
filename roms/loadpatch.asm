; patches for LD-BYTES 
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


ORG	$0767
        PUSH	IX ; Same as 48K ROM
RET0767:
	PUSH	AF


	PUSH	BC
	LD	A, CMD_LOADTRAP
        CALL	WRITECMDFIFO
        ; Wait for acknowledle
_retry1:
	CALL	READRESFIFO
        CP	$FF
	POP	BC

        JR	Z, RETIMMED ; If FF, we need to return immeditaly


	; AF is still in stack. NMI handler expects
        ; return address then AF, cause it will pop AF before returning.
        ; So we need to swap them
        EX	DE, HL
        LD	HL, RET2
        EX 	(SP), HL
        ; HL now contains AF.
        PUSH	HL
        EX	DE, HL
        
        JP	NMIHANDLER
	; Go back to Spectrum.
RETIMMED:
	POP	AF 
RET2:        
        LD	DE, RET0767
        ;PUSH	AF
        ;POP	IX
        ;LD	DE, $0552 ; test for break!
        PUSH	DE

        JP 	RETTOMAINROM
	HALT




ORG	$0970	; SA_CONTRL
	PUSH	HL	; Same as ROM
RET0970:
	PUSH	BC
        
	LD	A, CMD_SAVETRAP
        CALL	WRITECMDFIFO

        ; Push header (17 bytes)
        PUSH	IX
        LD	B, 17
_sendheader:
        LD      A, (IX)
        CALL	WRITECMDFIFO
        INC	IX
        DJNZ    _sendheader
        
        ; start to HL, length to DE.
        POP	IX

	POP	BC
        LD	DE, RET0970
        PUSH	DE
        JP 	RETTOMAINROM

        
        
        DI
        HALT

