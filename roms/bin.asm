Bin__NUM_VTABLE_ENTRIES	EQU	WidgetGroup__NUM_VTABLE_ENTRIES + 1

Bin__mainwidget_OFFSET	EQU	WidgetGroup__SIZE+0
Bin__SIZE		EQU	WidgetGroup__SIZE+2

Bin__CTOR:
	JP	Widget__CTOR

Bin__resize:
	CALL	Widget_PV__resize
        VCALL	WidgetGroup__resizeEvent
        RET

Bin__setChild:
	; Child in HL
        LD	(IX+Bin__mainwidget_OFFSET), L
        LD	(IX+Bin__mainwidget_OFFSET+1), H
        ; Notify child
        SWAP_IX_HL
        CALL	Widget__setParent
        SWAP_IX_HL
        VCALL	WidgetGroup__resizeEvent
        RET

Bin__handleEvent:
        PUSH	IX
        PUSH	HL
        
        PUSH	HL
        LD	L, (IX+Bin__mainwidget_OFFSET)
        LD	H, (IX+Bin__mainwidget_OFFSET+1)
	EX	(SP), HL
        POP	IX
        
        VCALL	Widget__handleEvent

        POP	HL
        POP	IX
        RET

Bin__draw:
	DEBUGSTR "Bin redraw "
        DEBUGHEXIX
        
        VCALL Widget__drawImpl
        ; Draw child
        LD	L, (IX+Bin__mainwidget_OFFSET)
        LD	A, L
        LD	H, (IX+Bin__mainwidget_OFFSET+1)
        OR	H
        RET	Z ; NULL child

	DEBUGSTR "Bin draw child "
        DEBUGHEXHL
        
        SWAP_IX_HL
        VCALL	Widget__draw
        SWAP_IX_HL
        RET

Bin__DTOR:
        LD	L, (IX+Bin__mainwidget_OFFSET)
        LD	A, L
        LD	H, (IX+Bin__mainwidget_OFFSET+1)
        OR	H
        JR	Z, _nochild
        SWAP_IX_HL
	DEBUGSTR "Bin destroy child\n"        
        VCALL	Widget__DTOR
        SWAP_IX_HL
_nochild:
	DEBUGSTR "Bin destroy\n"
        CALL 	Widget_V__DTOR
        RET


Bin__removeChild:
	; Child in HL
        LD	A, (IX+Bin__mainwidget_OFFSET)
        CP	L
        RET	NZ
        LD	A, (IX+Bin__mainwidget_OFFSET+1)
        CP	H
        RET 	NZ
        ; Clear it. 
        LD	(IX+Bin__mainwidget_OFFSET), 0
        LD	(IX+Bin__mainwidget_OFFSET+1), 0
        RET
