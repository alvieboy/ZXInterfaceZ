include "textinput_defs.asm"

TextInput__maxlen_OFFSET	EQU	Widget__SIZE
TextInput__strptr_OFFSET	EQU	Widget__SIZE+1
TextInput__callback_OFFSET	EQU	Widget__SIZE+1
TextInput__SIZE			EQU	Widget__SIZE+5

TextInput__VTABLE:
	DEFW	Widget_V__DTOR
	DEFW	Widget_V__draw
        DEFW	Widget_V__setVisible
        DEFW	Widget_PV__resize
        DEFW	TextInput__drawImpl
        DEFW	TextInput__handleEvent     	; Widget::handleEvent

TextInput__CTOR:
	; Inputs: A: max len, HL: string pointer
        LD	(IX+TextInput__maxlen_OFFSET),A
        LD	(IX+TextInput__strptr_OFFSET), L
        LD	(IX+TextInput__strptr_OFFSET+1), H
        JP	Widget__CTOR
        
TextInput__setCallbackFunction:
        LD	(IX+TextInput__callback_OFFSET), L
        LD	(IX+TextInput__callback_OFFSET+1), H
	RET
        
TextInput__drawImpl:
        LD	E, (IX+Widget__screenptr_OFFSET)
        LD	D, (IX+Widget__screenptr_OFFSET+1)
        
        LD	L, (IX+TextInput__strptr_OFFSET)
        LD	H, (IX+TextInput__strptr_OFFSET+1)
        ; Print out value (and count chars)
        CALL	PRINTSTRINGCNT
        ; Print cursor
        LD	A, 'L'
        CALL	PRINTCHAR
        ; Clear everything up to end of line
        PUSH	BC
        LD	A, (IX+TextInput__maxlen_OFFSET)
        SUB	C
        DEC	A
        DEC	A
        LD	HL, EMPTYSTRING
        CALL	PRINTSTRINGPAD
        POP	BC
        ; In the eventuality of backspace, clear next char as well
        LD	A, ' '
        CALL	PRINTCHAR
        
        ; In "C" we still have the number of chars printed.
        ;LD	A,C
        LD	L, (IX+Widget__attrptr_OFFSET)
        LD	H, (IX+Widget__attrptr_OFFSET+1)
        
        ; Fill everything regular color, except cursor.
        LD	A, 32
        ADD_HL_A ; Add A to HL

	LD	B, (IX+TextInput__maxlen_OFFSET)
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
        
TextInput__handleEvent:
	CP	0
        RET	Z
        
        LD	DE, (CURKEY)
        CALL	KEYTOASCII
        
        CP	$FF
        RET	Z

        LD	L, (IX+TextInput__strptr_OFFSET)
        LD	H, (IX+TextInput__strptr_OFFSET+1)
        
        CP	$08 ; Backspace
        JR	Z, _backspace
        CP	$0D ; Enter
        JR	Z, _enter
        CP	$1B ; ESCAPE/Break
        JR	Z, _cancel
        
        ;
        ; Append if we still have space.
        ;
        LD	D, A
        LD	B, (IX+TextInput__maxlen_OFFSET)
        ;PUSH	HL
        CALL	STRLEN
        ;POP	HL
        CP	B
        LD	A, $FF
        RET	Z 

	LD	(HL), D
        XOR	A
        INC	HL
        LD	(HL), A
_update:
        ; Redraw - could be faster
        CALL 	TextInput__drawImpl
        RET
_backspace:
        CALL	STRREMOVELASTCHAR
        JR	_update
_enter:	LD	A, 0
	JR	_docallback
_cancel:
	LD	A, 1
_docallback:
        LD	L, (IX+TextInput__callback_OFFSET)
        LD	H, (IX+TextInput__callback_OFFSET+1)
        JP	(HL)
