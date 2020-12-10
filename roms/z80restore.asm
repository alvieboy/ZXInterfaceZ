
RESTOREZ80VX:
        LD	A, $10		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        LD	A, $C0          ; hSB
        OUT	(PORT_RAM_ADDR_1), A
        LD	A, $01          ; MSB
        OUT	(PORT_RAM_ADDR_2), A    ; $01C010
	; Load page information into area
        DEBUGSTR "Loading map \n"
        LD	HL, HEAP
        LD	C, PORT_RAM_DATA
        REPT 	9 
	  INI
        ENDM
        
        
        LD	HL, HEAP

        LD	A, $00		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        LD	A, $00          ; hSB
        OUT	(PORT_RAM_ADDR_1), A
        LD	A, $03          ; MSB
        OUT	(PORT_RAM_ADDR_2), A    ; $030000


_nextpage:
        LD	BC, $7FFD
        LD	A, (HL)
        DEBUGSTR "Page index ptr "
        DEBUGHEXHL
        DEBUGSTR "Page val "
        DEBUGHEXA
        
        INC	HL
        CP	$FF
        JP	Z, _nomorepages
        ; Load page number
        OUT	(C), A
        ; Finally, load page data (16K)
        EX	DE, HL

        LD	C, PORT_RAM_DATA
        LD	HL, $C000
        ; For 48K compatiblity, use low addresses for the three pages involved.
        CP	$05
        JR	NZ, _check02
        LD	HL, $4000
        JR	_precopy
_check02:
        CP	$01
        JR	NZ, _precopy
        LD	HL, $8000
        ; Other pages use C000.
_precopy:
        DEBUGSTR "Target area"
        DEBUGHEXHL
        LD	A, $00
_copy:
        REPT 	64
        	INI
        ENDM
        DEC	A
        JP	NZ, _copy
        EX	DE, HL
        JP	_nextpage
_nomorepages:

        LD	A, $20		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        LD	A, $C0          ; hSB
        OUT	(PORT_RAM_ADDR_1), A
        LD	A, $01          ; MSB
        OUT	(PORT_RAM_ADDR_2), A    ; $01C020
        IN	A, (PORT_RAM_DATA)
        LD	BC, $7FFD
	OUT 	(C), A		; Restore active page.
        ; 
        ; Restore YM registers.
        ;
        LD	A, $00		; LSB 
        OUT	(PORT_RAM_ADDR_0), A ; $01C000

        ; C001 reg idx, 8001 data
        LD	C, $01
        LD	D, 0
_nextreg:
        LD	B, $C0
        OUT	(C), D  		; Select register number
        IN	A, (PORT_RAM_DATA)      ; Read value from memory
        LD	B, $80
        OUT	(C), A                  ; Write register value
        INC	D
        LD	A, D
        CP	$10
        JR	NZ, _nextreg
        ; Restore last YM register
        LD	B, $C0
        LD	A, $1A		; LSB 
        OUT	(PORT_RAM_ADDR_0), A ; $01C01A
        OUT	(C), A
        
	JP	RESTOREREGS_NOROM
