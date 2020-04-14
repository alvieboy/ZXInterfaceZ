	; STRLEN: return NULL-terminated string length
	; HL: string
	; Returns
	;	A: Length of string
        ; Corrupts
        ;	C
STRLEN:
	LD	C, 0
_l1:
        LD	A. (HL)
        INC 	HL
        OR 	A
        RET	Z
        INC	C
        JR	_l1
