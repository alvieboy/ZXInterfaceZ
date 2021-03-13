; This needs to be copied into RAM prior to execution
;
; At this point, command and ROM number must already be inside FIFO
;
ROMCRC_ROM:
	LD	A, $02
        OUT	(PORT_NMIREASON), A 	; Temporarly disable ROMCS

        LD	HL, $0000 ; Start ROM address
        LD	B, 0      ; Bytes to load: 256
	; Load ROM data.
_next:
	LD	D, (HL)
        ; Need to advance 64 bytes.
        LD	A, 64
        ADD	A, L
    	LD	L, A   ; L = A+L
    	ADC	A, H   ; A = A+L+H+carry
    	SUB	L      ; A = H+carry
    	LD	H, A   ; H = H+carry
        
        ; Send out ROM byte
_w2:    IN 	A, (PORT_CMD_FIFO_STATUS)
        OR	A
        JR	NZ, _w2
        LD	A, D
        OUT	(PORT_CMD_FIFO_DATA), A

        DJNZ	_next

        LD	A, $00
        OUT	(PORT_NMIREASON), A
        RET ; Return to ROM

ROMCRC_SIZE EQU $ - ROMCRC_ROM
