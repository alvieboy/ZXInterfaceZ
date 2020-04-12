
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

; Debug A
DEBUGHEXA:
	PUSH	DE
        PUSH 	BC
        LD	DE, LINE21
        CALL	PRINTHEX
        POP	BC
        POP	DE
        RET
