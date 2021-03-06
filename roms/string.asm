	; STRLEN: return NULL-terminated string length
	; HL: string
	; Returns
	;	A: Length of string
        ; Corrupts
        ;	C HL
STRLEN:
	LD	C, 0
_l1:
        LD	A, (HL)
        CP	0
        JR	Z, _l2
        INC 	HL
        INC	C
        JR	_l1
_l2:	LD	A, C
	RET

STRAPPENDCHAR:
	PUSH 	AF
	CALL	STRLEN
	POP	AF
        LD	(HL), A
	INC	HL
	XOR 	A
        LD	(HL), A
        RET

STRREMOVELASTCHAR:
	CALL	STRLEN
	OR 	A
        RET	Z ; Empty string
        DEC	HL
        XOR 	A
        LD	(HL), A
        RET
