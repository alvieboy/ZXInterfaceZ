
DEBUGCHR:
	PUSH	BC
	LD	C, PORT_SCRATCH0
        OUT	(C), A
        POP	BC
        RET

DEBUGHEX:
	PUSH 	BC
	LD 	B, A
        SRL     A
        SRL     A
        SRL     A
        SRL     A
        CALL	DEBUGNIBBLE
        LD	A, B
        AND	$F
        CALL 	DEBUGNIBBLE
        POP	BC
        RET
DEBUGNIBBLE:
        CP	10
        JR	NC, _l1
	ADD 	A, '0'
        JR	DEBUGCHR
_l1:   ADD	A, 'A'-10
        JR 	DEBUGCHR


DEBUGHEXDE:
	EX	DE, HL
        CALL DEBUGHEXHL
        EX 	DE, HL
        RET

DEBUGHEXSP:
	PUSH	HL
        LD	HL, 2
        ADD	HL, SP
        CALL DEBUGHEXHL
        POP	HL
        RET

; Debug HL
DEBUGHEXHL:
	PUSH	DE
        PUSH 	BC
        PUSH	AF
        
        ;LD	DE, LINE21
        PUSH	HL
        LD	A, L
        LD	B, A
        LD	A, H
        CALL	DEBUGHEX
        LD	A, B
        CALL 	DEBUGHEX
        LD	A, $0A
        CALL 	DEBUGCHR
        POP	HL
        POP	AF
        POP	BC
        POP	DE
        RET

PRINTHEXHL:
        LD	A, L
        LD	B, A
        LD	A, H
        CALL	PRINTHEX
        LD	A, B
        JP 	PRINTHEX

; Debug A
DEBUGHEXA:
	PUSH	DE
        PUSH 	BC
        PUSH	AF
        PUSH	HL
        ;LD	DE, LINE21
        CALL	DEBUGHEX
        LD	A, $0A
        CALL 	DEBUGCHR
        POP	HL
	POP 	AF
        POP	BC
        POP	DE
        RET

DEBUGSTRING:
	PUSH	DE
        PUSH 	BC
        PUSH	AF
        PUSH	HL
_printloop:

        LD	A, (HL)
        INC	HL
	CP 	$0
        JR	Z, _endprint
        CALL	DEBUGCHR
	JR	_printloop
_endprint
        POP	HL
	POP 	AF
        POP	BC
        POP	DE
        RET

DEBUG8 MACRO what
	PUSH	AF
        LD	A, what
        CALL	DEBUGHEXA
        POP	AF
ENDM
