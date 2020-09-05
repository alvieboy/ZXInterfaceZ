;
; Widget with window + simple menu (callback)
;

Menuwindow__menuinstance_OFFSET	EQU	Window__SIZE+0

Menuwindow__SIZE		EQU	Window__SIZE + CallbackMenu__SIZE

Menuwindow__CTOR:
	DEBUGSTR "Init menu window "
        DEBUGHEXIX
	
        CALL	Window__CTOR

        ;CALL	Window__showScreenPtr

        PUSH	IX
        
        ; Initialise menu
        LD	BC, Menuwindow__menuinstance_OFFSET
        ADD	IX, BC
        PUSH	IX
        CALL	CallbackMenu__CTOR
        POP	HL ; HL contains now the Menu pointer
        POP	IX
        CALL	Bin__setChild	; TAILCALL
            

	RET
        
Menuwindow__setEntries:
        PUSH	IX
        LD	BC, Menuwindow__menuinstance_OFFSET
        ADD	IX, BC
	CALL	Menu__setEntries
        POP	IX
        RET

Menuwindow__setCallbackTable:
        PUSH	IX
        LD	BC, Menuwindow__menuinstance_OFFSET
        ADD	IX, BC
	CALL	CallbackMenu__setCallbackTable
        POP	IX
        RET
