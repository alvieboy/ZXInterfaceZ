;
; Widget with window + simple menu (indexed)
;

MenuwindowIndexed__menuinstance_OFFSET	EQU	Window__SIZE+0

MenuwindowIndexed__SIZE			EQU	Window__SIZE + IndexedMenu__SIZE

MenuwindowIndexed__CTOR:
        CALL	Window__CTOR
        PUSH	IX
        
        ; Initialise menu
        LD	BC, MenuwindowIndexed__menuinstance_OFFSET
        ADD	IX, BC
        PUSH	IX
        CALL	IndexedMenu__CTOR
        POP	HL ; HL contains now the Menu pointer
        POP	IX
        CALL	Bin__setChild	; TAILCALL
            

	RET
        
MenuwindowIndexed__setEntries:
        PUSH	IX
        LD	BC, MenuwindowIndexed__menuinstance_OFFSET
        ADD	IX, BC
	CALL	Menu__setEntries
        POP	IX
        RET

; DE: User pointer.
MenuwindowIndexed__setFunctionHandler:
        PUSH	IX
        LD	BC, MenuwindowIndexed__menuinstance_OFFSET
        ADD	IX, BC
	CALL	IndexedMenu__setFunctionHandler
        POP	IX
        RET
