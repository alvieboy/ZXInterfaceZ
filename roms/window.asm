;	class Window: public Bin {
;	  public:
;	    Window();
;	    // Virtual methods
;           virtual void realise();
;	}

Window__titleptr_OFFSET		EQU	Bin__SIZE
Window__screen_OFFSET		EQU	Bin__SIZE+2
Window__SIZE			EQU	Bin__SIZE+4


; Virtual method indexes

; VTable
Window__VTABLE:
	DEFW	Window__DTOR
        DEFW    Bin__draw             	; Widget::draw
        DEFW	Widget_V__setVisible
        DEFW 	Bin__resize
        DEFW	Window__drawImpl	; Widget::drawImpl
        DEFW	Bin__handleEvent     	; Widget::handleEvent
        DEFW	Bin__removeChild	; WidgetGroup::removeChild
        DEFW	Window__resizeEvent     ; WidgetGroup::resizeEvent

; Constructor
Window__CTOR:
        ; Save screen pointer.
        DEBUGSTR "Window CTOR "
        DEBUGHEXIX
        DEBUGSTR "Window screen "
        DEBUGHEXHL
        
        LD	(IX+Window__screen_OFFSET), L
        LD	(IX+Window__screen_OFFSET+1), H
	
	; Windows have no parent widget
        PUSH 	HL
        LD	HL, 0
	CALL	Widget__CTOR
        POP	HL

        CALL	Window__showScreenPtr
        
        LD	(IX), 	LOW(Window__VTABLE)
        LD	(IX+1), HIGH(Window__VTABLE)
        ; Store size
        LD	(IX+Widget__width_OFFSET), D
        LD	(IX+Widget__height_OFFSET), E
        ;
        LD	(IX+Window__titleptr_OFFSET), 0
        LD	(IX+Window__titleptr_OFFSET+1), 0
        
        CALL	Window__showScreenPtr
       
        RET

Window__setTitle:
        LD	(IX+Window__titleptr_OFFSET), L
        LD	(IX+Window__titleptr_OFFSET+1), H
        RET


Window__realise:
	RET
 
       
Window_P__fillHeaderLine:
        ; Prepare header attributes
        PUSH 	DE
	PUSH	BC
        
 	LD	B, (IX+Widget__width_OFFSET)
_header:
        LD	A, $07
        LD	(DE), A
        INC	DE
        DJNZ 	_header
        ; Last one is black
        LD	A, $0
        LD	(DE), A
        POP	BC
	POP 	DE
        RET

; Draw a window
Window__drawImpl:

	LD	A, $78
	CALL	Window__setBackground

	LD	B, (IX+Widget__height_OFFSET)  	; Number of entries
        LD	E, (IX+Widget__screenptr_OFFSET)       ; Screen..
        LD	D, (IX+Widget__screenptr_OFFSET+1)       ; pointer.
       	
        DEBUGSTR "Window height "
        DEBUGHEXB

        DEBUGSTR "Screen "
        DEBUGHEXDE

        DEBUGSTR "Win width " 
        DEBUG8 (IX+Widget__width_OFFSET)
        
        CALL	MOVEDOWN
 
_drawiter:
	PUSH 	BC
        PUSH	DE
        
  	LD	HL, LEFTVERTICAL
        CALL	DRAWCHAR
        
        LD	A, E
        ADD	A, (IX+Widget__width_OFFSET) ; Width of menu

        ;DEC 	A
        LD	E, A
        JR	NC, _noover
	INC	D
_noover:
	LD	HL, RIGHTVERTICAL
	CALL	DRAWCHAR
	POP	DE

        CALL 	MOVEDOWN
        POP	BC

        ; Repeat
        DJNZ	_drawiter
        
        LD	B, (IX+Widget__width_OFFSET)
        INC	B
        INC	B
        LD	A, $FF
        CALL	DRAWHLINE

        ; Prepare header attributes
        LD	E, (IX+Widget__attrptr_OFFSET)
        LD	D, (IX+Widget__attrptr_OFFSET+1)

	CALL	Window_P__fillHeaderLine
        
        LD	E, (IX+Widget__screenptr_OFFSET)
        LD	D, (IX+Widget__screenptr_OFFSET+1)
        ; We need a space first
        LD	A, ' '
        CALL	PRINTCHAR
        ;INC	DE
        LD	L, (IX+Window__titleptr_OFFSET)
        LD	H, (IX+Window__titleptr_OFFSET+1)
        PUSH	DE
        ; Compute amount of space so we can "pad" the string
        LD	A, (IX+Widget__width_OFFSET)
        SUB	4
        CALL	PRINTSTRINGPAD
        
        POP	DE
        LD	A, E
        ADD	A, (IX+Widget__width_OFFSET)
        SUB	5      ; Adjust for logo space
        LD	E, A
        
        ; Place the Logo chars
        LD	HL, HH
        CALL 	DRAWCHAR
        LD	HL, HH
        CALL 	DRAWCHAR
        LD	HL, HH
        CALL 	DRAWCHAR
        LD	HL, HH
        CALL 	DRAWCHAR
        LD	HL, HH
        CALL 	DRAWCHAR
        
        ; Place logo attributes
        LD	E, (IX+Widget__attrptr_OFFSET)
        LD	D, (IX+Widget__attrptr_OFFSET+1)

        LD	A, E
        ADD	A, (IX+Widget__width_OFFSET)
        SUB 	4    ; 4 logo entries
        LD 	E, A
        
        
        LD	A, %01000010   ; Bright, black+red
        LD	(DE), A
	INC 	DE
        LD	A, %01010110   ; Bright, red+yellow
        LD	(DE), A
	INC 	DE
        LD	A, %01110100   	; Bright, yellow+green
        LD	(DE), A
	INC 	DE
        LD	A, %01100101   	; Bright, green+cyan
        LD	(DE), A
	INC 	DE
        LD	A, %01101000   	; Bright, cyan+black
        LD	(DE), A
	INC 	DE
        XOR 	A;
        LD 	(DE), A
        RET

Window__show:

	CALL	Widget_V__setVisible
        ; Show child
        LD	A, (IX+Bin__mainwidget_OFFSET)
        OR	(IX+Bin__mainwidget_OFFSET+1)
        RET	Z
        LD	L, (IX+Bin__mainwidget_OFFSET)
        LD	H, (IX+Bin__mainwidget_OFFSET+1)
        SWAP_IX_HL
        VCALL	Widget__setVisible
        SWAP_IX_HL

        RET
        
Window__resizeEvent:
	DEBUGSTR "Window resize event "
        DEBUGHEXIX

        LD	L, (IX+Bin__mainwidget_OFFSET)
	LD	A, (IX+Bin__mainwidget_OFFSET+1)
	OR	L
        RET	Z	; No child widget
        LD	H, (IX+Bin__mainwidget_OFFSET+1)
        ; Update child sizes for child in HL
        PUSH	IX

        PUSH	HL
        
        LD	H, (IX+Widget__x_OFFSET)
        INC	H
        LD	L, (IX+Widget__y_OFFSET)
        INC	L

        LD	D, (IX+Widget__width_OFFSET)
        DEC	D
        DEC	D
        LD	E, (IX+Widget__height_OFFSET)
        DEC	E
        DEC	E
        
        POP	IX	; IX is child now
        DEBUGSTR "Resizing child "
        DEBUGHEXIX

        VCALL	Widget__resize
        POP	IX
        
        RET
        
Window_P_fillHeaderLine:
        ; Prepare header attributes
        PUSH 	DE
	PUSH	BC
 	LD	B, (IX+Widget__width_OFFSET)
_l1:
        LD	A, $07
        LD	(DE), A
        INC	DE
        DJNZ 	_l1
        ; Last one is black
        XOR	A
        LD	(DE), A
        POP	BC
	POP 	DE
        RET

; set window area
Window__setBackground:
        PUSH	AF		; Save attribute
	
        LD	A, (IX+Widget__height_OFFSET)
        INC	A		; Include header
        SLA	A
        SLA	A
        SLA	A               ; Multiply by 8
        INC	A		; Include (one line) footer
        LD	L, (IX+Widget__screenptr_OFFSET)       ; Screen..
        LD	H, (IX+Widget__screenptr_OFFSET+1)     ; pointer.
        PUSH	HL
        POP	DE
        
        LD	C, A            ; Rows to process

_CLRNEXT:
        XOR	A
        LD	B, (IX+Widget__width_OFFSET)         ; Width of menu
        INC	B
        INC	B
_CLRLINE:
        LD	(HL), A
        INC	HL
        DJNZ	_CLRLINE
        CALL	PMOVEDOWN
        PUSH	DE
        POP	HL
        
        DEC	C
        XOR	A
        OR	C		; A is still 0
	JR	NZ, _CLRNEXT
        
        POP	AF
        ; Now, clear attributes
        LD	L, (IX+Widget__attrptr_OFFSET)
        LD	H, (IX+Widget__attrptr_OFFSET+1)

        LD	C, (IX+Widget__height_OFFSET)       ; Attribute lines to clear
        INC	C               ; Include header
       ; INC	C               ; And footer
        
        LD	D, A	; Save attribute in D
        
_CLRATTRNEXT:
        LD	B, (IX+Widget__width_OFFSET)         ; Width of menu
        INC	B
        INC	B
        PUSH	HL
_CLRATTR:
        LD	(HL), A
        INC 	HL
        DJNZ	_CLRATTR
        POP	HL
        
        LD	A, 32
        ADD_HL_A ; Add A to HL
        XOR	A

        DEC	C
        OR	C
        LD	A, D ; Restore attibute
        JR	NZ, _CLRATTRNEXT
        
        RET

Window__DTOR:
	DEBUGSTR "Destroy window\n"
	; Remove ourselves from the screen list.
        LD	L, (IX+Window__screen_OFFSET)
        LD	H, (IX+Window__screen_OFFSET+1)
        DEBUGSTR "Screen "
        DEBUGHEXHL
        
        SWAP_IX_HL
        CALL	Screen__removeWindow
        SWAP_IX_HL
	JP	Bin__DTOR

Window__showScreenPtr:
	PUSH 	HL
        LD	L, (IX+Window__screen_OFFSET)
        LD	H, (IX+Window__screen_OFFSET+1)
        DEBUGSTR "Window::showScreenPtr() = "
        DEBUGHEXHL
        POP	HL
        RET
        
