; EQU's for text input widget
TEXTINPUT_OFF_MAX_LEN			EQU     $08
TEXTINPUT_OFF_STRINGPTR			EQU     $0A

TEXTINPUT__INIT:
	PUSH	HL
        POP	IX
        JP	FRAME__INIT
        

TEXTINPUT__DRAWCONTENT:
	CALL	MOVEDOWN ; Move to next line.
        LD	L, (IX+TEXTINPUT_OFF_STRINGPTR)
        LD	H, (IX+TEXTINPUT_OFF_STRINGPTR+1)
        ; Print out value (and count chars)
        CALL	PRINTSTRINGCNT
        ; Print cursor
        LD	A, 'C'
        CALL	PRINTCHAR
        ; In "C" we still have the number of chars printed.
        LD	A,C
        CALL	DEBUGHEXA
        ENDLESS
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

	
	
