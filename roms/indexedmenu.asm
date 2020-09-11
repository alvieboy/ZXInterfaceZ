;
; Menu with strings which can return the selected string
;

IndexedMenu__VTABLE:
	DEFW	Widget_V__DTOR
	DEFW	Widget_V__draw
        DEFW	Widget_V__show
        DEFW	Widget_PV__resize
        DEFW	Menu__drawImpl
        DEFW	Menu__handleEvent     	; Widget::handleEvent
        ; Menu virtual functions
        DEFW	IndexedMenu_V__activateEntry 	; Menu::activateEntry

IndexedMenu__selectfun_OFFSET	EQU	Menu__SIZE
IndexedMenu__userptr_OFFSET	EQU	Menu__SIZE+2
IndexedMenu__SIZE 		EQU	Menu__SIZE+4


IndexedMenu_V__activateEntry:
	DEBUGSTR "activateEntry "
        DEBUGHEXIX
       	LD	E, (IX+IndexedMenu__selectfun_OFFSET)
        LD	D, (IX+IndexedMenu__selectfun_OFFSET+1)
        LD	L, (IX+IndexedMenu__userptr_OFFSET)
        LD	H, (IX+IndexedMenu__userptr_OFFSET+1)
	DEBUGSTR " userptr "
        DEBUGHEXHL
        PUSH	DE
        RET        ; JP (DE)
        
IndexedMenu__CTOR:
        LD	(IX), LOW(IndexedMenu__VTABLE)
        LD	(IX+1), HIGH(IndexedMenu__VTABLE)
	JP	Menu_P__CTOR

IndexedMenu__setFunctionHandler:
	; HL: function to call
        ; DE: parameter to pass in HL when calling function
	LD 	(IX+IndexedMenu__selectfun_OFFSET),  L
        LD 	(IX+IndexedMenu__selectfun_OFFSET+1),H
	LD 	(IX+IndexedMenu__userptr_OFFSET),  E
        LD 	(IX+IndexedMenu__userptr_OFFSET+1),  D
        DEBUGSTR "IndexedMenu DE "
        DEBUGHEXDE
        RET

