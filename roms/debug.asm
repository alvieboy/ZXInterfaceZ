
; Debug HL
DEBUGHEXHL:
	PUSH	DE
        PUSH 	BC
        PUSH	AF
        
        LD	DE, LINE21
        PUSH	HL
        LD	A, L
        LD	B, A
        LD	A, H
        CALL	PRINTHEX
        LD	A, B
        CALL 	PRINTHEX
        POP	HL
        POP	AF
        POP	BC
        POP	DE
        RET
