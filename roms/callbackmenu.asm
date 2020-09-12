; Menu with callback functions

CallbackMenu__VTABLE:
	DEFW	Widget_V__DTOR
	DEFW	Widget_V__draw
        DEFW	Widget_V__setVisible
        DEFW	Widget_PV__resize
        DEFW	Menu__drawImpl
        DEFW	Menu__handleEvent     	; Widget::handleEvent
        ; Menu virtual functions
        DEFW	CallbackMenu_V__activateEntry 	; Menu::activateEntry

CallbackMenu__cbtable_OFFSET	EQU	Menu__SIZE
CallbackMenu__SIZE 		EQU	Menu__SIZE+2


CallbackMenu_V__activateEntry:
	DEBUGSTR "ACTIVATE\n"
        LD	A, (IX+Menu__selectedEntry_OFFSET)
        ADD	A, A ; *2
        LD	C, A
        LD	B, 0
        ; Load pointer
        LD	L, (IX+CallbackMenu__cbtable_OFFSET)
        LD	H, (IX+CallbackMenu__cbtable_OFFSET+1)
        ADD	HL, BC ; Add offset
        LD	A, (HL)
        INC	HL
        LD	H, (HL)
        LD	L, A
        ; Jump
        DEBUGSTR	"Jump to "
        DEBUGHEXHL

        JP	(HL)
        
CallbackMenu__CTOR:
	;LD 	(IX+CallbackMenu__cbtable_OFFSET),  0
        ;LD 	(IX+CallbackMenu__cbtable_OFFSET+1),0  ; NULL the callback table
        LD	(IX), LOW(CallbackMenu__VTABLE)
        LD	(IX+1), HIGH(CallbackMenu__VTABLE)
	JP	Menu_P__CTOR

CallbackMenu__setCallbackTable:
	LD 	(IX+CallbackMenu__cbtable_OFFSET),  L
        LD 	(IX+CallbackMenu__cbtable_OFFSET+1),H
        RET

