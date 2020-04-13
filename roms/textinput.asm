; EQU's for text input widget
TEXTINPUT_OFF_MAX_LEN			EQU     $08
TEXTINPUT_OFF_STRINGPTR			EQU     $0A

TEXTINPUT__INIT:
	PUSH	HL
        POP	IX
        JP	FRAMEINIT
        

TEXTINPUT__DRAWCONTENT:
	CALL	MOVEDOWN ; Move to next line.
        LD	L, (IX+TEXTINPUT_OFF_STRINGPTR)
        LD	H, (IX+TEXTINPUT_OFF_STRINGPTR+1)
        ; Print out value
        CALL	PRINTSTRING
        ; Print cursor
        LD	A, 'C'
        CALL	PRINTCHAR
        RET
        
TEXTINPUT__DRAW:
	PUSH  	HL
        POP	IX
	CALL	DRAWFRAME
        ; Draw contents
        LD	E, (IX+MENU_OFF_SCREENPTR)
        LD	D, (IX+MENU_OFF_SCREENPTR+1)
        INC	DE
        JP	TEXTINPUT__DRAWCONTENT

	
	
