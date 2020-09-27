SNARAM:
	DI
        ; External RAM address: 0x010000
        LD	A, $00		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        LD	A, $00          ; hSB
        OUT	(PORT_RAM_ADDR_1), A
        LD	A, $01          ; MSB
        OUT	(PORT_RAM_ADDR_2), A

	; Restore all RAM from external RAM (whole 48K)
        LD	B, $00
        LD	D, $C0 ;$C0
        LD	C, PORT_RAM_DATA
	LD	HL, SCREEN
_l2:
        INIR
        DEC	D
        JR	NZ, _l2
        ; ESP32 shall have perform a ROM patch. Jump into it.
        JP	ROM_PATCHED_SNALOAD

