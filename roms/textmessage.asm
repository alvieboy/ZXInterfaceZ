TEXTMESSAGE_COLOR_NORMAL EQU %01111000

TEXTMESSAGE_OFF_STRINGPTR EQU FRAME_OFF_MAX


TEXTMESSAGE__CLEAR:
        LD	IX, TEXTMESSAGEAREA
        JP	FRAME__CLEAR
        
TEXTMESSAGE__SHOW:
	CALL TEXTMESSAGE__INIT ; Sets up IX
        CALL FRAME__DRAW
        JR   TEXTMESSAGE__DRAWCONTENT

	; HL: Message        
TEXTMESSAGE__INIT:
        LD	IX, TEXTMESSAGEAREA
        LD	(IX+TEXTMESSAGE_OFF_STRINGPTR), L
        LD	(IX+TEXTMESSAGE_OFF_STRINGPTR+1), H
        
        LD	(IX+FRAME_OFF_NUMBER_OF_LINES), 3
        ; Compute size
        CALL	STRLEN
        INC	A
        INC	A ; Add two spaces
        ; At least 12 
        CP	12
        JR	NC, _l1
    	LD	A, 12
_l1:
        LD	(IX+FRAME_OFF_WIDTH), A

        LD	(IX+FRAME_OFF_TITLEPTR), LOW(TEXTMESSAGE_TITLE)
        LD	(IX+FRAME_OFF_TITLEPTR+1), HIGH(TEXTMESSAGE_TITLE)
        LD	D, 12 ; Line to display at
        JP	FRAME__INIT

TEXTMESSAGE__DRAWCONTENT:
        LD	E, (IX+FRAME_OFF_SCREENPTR)
        LD	D, (IX+FRAME_OFF_SCREENPTR+1)
	CALL	MOVEDOWN ; Move to next line.
        CALL	MOVEDOWN ; Move to next line.
        INC	DE
        LD	L, (IX+TEXTMESSAGE_OFF_STRINGPTR)
        LD	H, (IX+TEXTMESSAGE_OFF_STRINGPTR+1)
        
        CALL	PRINTSTRING

        LD	L, (IX+FRAME_OFF_ATTRPTR)
        LD	H, (IX+FRAME_OFF_ATTRPTR+1)
        
        ; Fill everything regular color, except cursor.
        LD	A, 32
        ADD_HL_A ; Add A to HL

	LD	C, (IX+FRAME_OFF_WIDTH)
        INC	C  ; First space
        INC	C  ; Last space

        LD	B, 3
_l2:
	PUSH 	BC
        LD	B, C
        PUSH	HL
        ; Fill in top attributes.
        LD	A, TEXTMESSAGE_COLOR_NORMAL
_l1: 	LD	(HL), A
        INC 	HL
        DJNZ	_l1
        POP	HL
        LD	A, 32
        ADD_HL_A
        POP	BC
        DJNZ    _l2
        RET
        
TEXTMESSAGE_TITLE:
	DB "Message", 0