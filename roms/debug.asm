DEBUGENABLED 	EQU 1

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


DEBUGHEXDE MACRO
IF	DEBUGENABLED
	EX	DE, HL
        CALL DEBUGHEXHL_INTERNAL
        EX 	DE, HL
ENDIF
ENDM

DEBUGHEXBC MACRO
IF	DEBUGENABLED

        PUSH	HL
	PUSH	BC
        POP	HL

        CALL DEBUGHEXHL_INTERNAL
        POP	HL
ENDIF
ENDM

DEBUGHEXIX MACRO
IF	DEBUGENABLED
	PUSH	HL
        PUSH	IX
        POP	HL
        CALL DEBUGHEXHL_INTERNAL
        POP	HL
ENDIF
ENDM

DEBUGHEXIY MACRO
IF	DEBUGENABLED
	PUSH	HL
        PUSH	IY
        POP	HL
        CALL DEBUGHEXHL_INTERNAL
        POP	HL
ENDIF
ENDM

DEBUGHEXSP MACRO
IF	DEBUGENABLED
	PUSH	HL
        LD	HL, 2
        ADD	HL, SP
        CALL DEBUGHEXHL_INTERNAL
        POP	HL
ENDIF
ENDM


; Debug HL
DEBUGHEXHL MACRO
IF	DEBUGENABLED
	CALL	DEBUGHEXHL_INTERNAL
ENDIF
ENDM

DEBUGHEXHL_INTERNAL:
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
DEBUGHEXA_INTERNAL:
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
        
DEBUGHEXB MACRO
IF DEBUGENABLED
	PUSH	AF
        LD	A, B
        CALL	DEBUGHEXA_INTERNAL
        POP	AF
ENDIF
ENDM

DEBUGHEXA MACRO
IF DEBUGENABLED
	CALL	DEBUGHEXA_INTERNAL
ENDIF
ENDM

DEBUGHEXH MACRO
IF DEBUGENABLED
	PUSH	AF
        LD	A, H
        CALL	DEBUGHEXA_INTERNAL
        POP	AF
ENDIF
ENDM

DEBUGHEXD MACRO
IF DEBUGENABLED
	PUSH	AF
        LD	A, D
        CALL	DEBUGHEXA_INTERNAL
        POP	AF
ENDIF
ENDM
        
DEBUGHEXE MACRO
IF DEBUGENABLED
	PUSH	AF
        LD	A, E
        CALL	DEBUGHEXA_INTERNAL
        POP	AF
ENDIF
ENDM

DEBUGHEXL MACRO
IF DEBUGENABLED
	PUSH	AF
        LD	A, L
        CALL	DEBUGHEXA_INTERNAL
        POP	AF
ENDIF
ENDM

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
IF DEBUGENABLED
	PUSH	AF
        LD	A, what
        CALL	DEBUGHEXA_INTERNAL
        POP	AF
ENDIF
ENDM

DEBUGSP	MACRO
	PUSH 	HL
        LD	HL,0
        ADD	HL, SP
        INC	HL
        INC	HL
        CALL	DEBUGHEXHL_INTERNAL
        POP	HL
ENDM


DEBUGSTR MACRO string
IF DEBUGENABLED
	PUSH	HL
	LD 	HL, _str
        CALL 	DEBUGSTRING
        POP	HL
        JR	_endstr
_str:	DB string, 0
_endstr:
ENDIF
ENDM
