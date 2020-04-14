include "textinput_defs.asm"

TEXTINPUT__INIT:
	PUSH	HL
        POP	IX
        LD	(IX+FRAME_OFF_NUMBER_OF_LINES), 3
        JP	FRAME__INIT

TEXTINPUT__DRAWCONTENT:
	CALL	MOVEDOWN ; Move to next line.
        CALL	MOVEDOWN ; Move to next line.
        LD	L, (IX+TEXTINPUT_OFF_STRINGPTR)
        LD	H, (IX+TEXTINPUT_OFF_STRINGPTR+1)
        ; Print out value (and count chars)
        CALL	PRINTSTRINGCNT
        ; Print cursor
        LD	A, 'L'
        CALL	PRINTCHAR
        ; In "C" we still have the number of chars printed.
        ;LD	A,C
        LD	L, (IX+FRAME_OFF_ATTRPTR)
        LD	H, (IX+FRAME_OFF_ATTRPTR+1)
        ; Fill everything regular color, except cursor.
        LD	A, 32
        ADD_HL_A ; Add A to HL

	LD	B, (IX+FRAME_OFF_WIDTH)
        INC	B  ; First space
        LD	A, B
        SUB	C
        LD	C, A
        INC	B  ; Last space
        
        PUSH	BC ; We need this for the upcoming lines
        PUSH	HL
        
        ; Fill in top attributes.
        LD	A, TEXTINPUT_COLOR_NORMAL
_topline: LD	(HL), A
        INC 	HL
        DJNZ	_topline

        POP	HL

        LD	A, 32
        ADD_HL_A
        
        
        POP	BC
        PUSH	BC

        PUSH	HL
        
_nextattr:
	LD	A, B
        CP	C
        LD	A, TEXTINPUT_COLOR_NORMAL
        JR 	NZ, _l1
        LD	A, TEXTINPUT_COLOR_CURSOR
_l1:    LD	(HL), A
        INC	HL
        DJNZ   _nextattr
        
        POP	HL
        LD	A, 32
        ADD_HL_A
        
        POP 	BC
        LD	A, TEXTINPUT_COLOR_NORMAL
_bline: LD	(HL), A
        INC 	HL
        DJNZ	_bline
        
        
        
        RET
        
TEXTINPUT__DRAW:
	PUSH  	HL
        POP	IX
	CALL	FRAME__DRAW
        ; Draw contents
        LD	E, (IX+FRAME_OFF_SCREENPTR)
        LD	D, (IX+FRAME_OFF_SCREENPTR+1)
        INC	DE
        JP	TEXTINPUT__DRAWCONTENT



