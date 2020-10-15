;
; Menu with strings which can return the selected string
;

StringMenu__VTABLE:
	DEFW	Widget_V__DTOR
	DEFW	Widget_V__draw
        DEFW	Widget_V__show
        DEFW	Widget_PV__resize
        DEFW	Menu__drawImpl
        DEFW	Menu__handleEvent     	; Widget::handleEvent
        ; Menu virtual functions
        DEFW	StringMenu_V__activateEntry 	; Menu::activateEntry

StringMenu__selectfun_OFFSET	EQU	Menu__SIZE
StringMenu__SIZE 		EQU	Menu__SIZE+2


StringMenu_V__activateEntry:
       ; UNIMPLEMENTED
	RET
        
StringMenu__CTOR:
	;LD 	(IX+StringMenu__cbtable_OFFSET),  0
        ;LD 	(IX+StringMenu__cbtable_OFFSET+1),0  ; NULL the callback table
        LD	(IX), LOW(StringMenu__VTABLE)
        LD	(IX+1), HIGH(StringMenu__VTABLE)
	JP	Menu_P__CTOR

StringMenu__setFunctionHandler:
	LD 	(IX+StringMenu__selectfun_OFFSET),  L
        LD 	(IX+StringMenu__selectfun_OFFSET+1),H
        RET

