ALLOC__INIT:
	; Switch to bank 7
        LD	C, PORT_MEMSEL
        LD	A, 7
        OUT	(C), A

	LD	HL, HEAP
        LD	(HL), LOW(ALLOC_ENDPTR)
        INC	HL
        LD	(HL), HIGH(ALLOC_ENDPTR)
        RET


; 	Allocate DE bytes in heap
;	Return: HL: pointer or 0000 if cannot alloc.
;		Flags: TBD
;
MALLOC:
       	; Check for space. TBD
        LD	HL, (ALLOC_ENDPTR)
        ; Store size 
        LD	(HL), E
        INC	HL
        LD	(HL), D
        INC 	HL
        PUSH	HL
        
        ADD	HL, DE
        LD	(ALLOC_ENDPTR), HL
	POP	HL
        DEBUGSTR	" Malloc size= "
        DEBUGHEXDE
        DEBUGSTR	"  Ptr= "
        DEBUGHEXHL
        RET
        

;	
;	Example:
;		Alloc 16: return 0x2002, endptr=0x2012

FREE:
	DEBUGSTR	" Free "
        DEBUGHEXHL
        
        DEC	HL
        DEC	HL
        PUSH	HL 	; Save it here (0x2000 in example)
        
        LD	E, (HL)
        INC	HL
        LD	D, (HL)
        INC 	HL ; 0x2002 in example. DE should contain 0x0010
        ; Ensure this is the last block to be freed.
        ADD	HL, DE
        EX	DE, HL
        LD	HL, (ALLOC_ENDPTR)
        ; Compare with DE
        OR	A; Reset carry
	SBC	HL, DE
        POP	HL	; Get saved new head 
        JR	NZ, _invalidptr
        ; 	Good to go
        LD	(ALLOC_ENDPTR), HL
        RET
_invalidptr:
	JP	INTERNALERROR
        
        
        