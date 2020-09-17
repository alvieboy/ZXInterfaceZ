
MultiWidget__MAXIMUM_CHILDS 		EQU	4

MultiWidget__NUM_VTABLE_ENTRIES		EQU	WidgetGroup__NUM_VTABLE_ENTRIES+1

MultiWidget__numchilds_OFFSET		EQU	WidgetGroup__SIZE
MultiWidget__childs_OFFSET		EQU	WidgetGroup__SIZE+1

MultiWidget__SIZE			EQU	WidgetGroup__SIZE+1+(MultiWidget__MAXIMUM_CHILDS*2)



MultiWidget__resize:
	CALL	Widget_PV__resize
        VCALL	WidgetGroup__resizeEvent
        RET

MultiWidget_P__getWidgetPointer:
	; Returns IX + MultiWidget__childs_OFFSET + (2* (A) ) in HL
        ; Corrupts BC
        LD	L, A
        LD	H, 0
        ADD	HL, HL
        LD	BC, MultiWidget__childs_OFFSET
        ADD	HL, BC
        PUSH	IX
        POP	BC
        ADD	HL, BC
        OR	A 	; Clear carry
        RET

MultiWidget_P__getStartWidgetPointer:
	PUSH	IX
        LD	BC, MultiWidget__childs_OFFSET
	ADD	IX, BC
        EX	(SP), IX
        POP	HL
        RET

MultiWidget__addChild:
	; Child in HL
        EX	DE, HL
        LD	A, (IX+MultiWidget__numchilds_OFFSET)
        CALL	MultiWidget_P__getWidgetPointer
        LD	(HL), E
        INC	HL
        LD	(HL), D
        EX	DE, HL
        INC	(IX+MultiWidget__numchilds_OFFSET)
        ; Notify child
        SWAP_IX_HL
        CALL	Widget__setParent
        SWAP_IX_HL
        VCALL	WidgetGroup__resizeEvent
        RET

MultiWidget_P__foreachVcallC:
        PUSH 	IX	; save "this" pointer

        PUSH	HL      ; HL in stack
        
        EX	AF, AF'
	XOR	A
        CALL	MultiWidget_P__getStartWidgetPointer
        EX	AF, AF'
	LD	B, (IX+MultiWidget__numchilds_OFFSET)
        
        ; At this point:
        ; DE: Param 2
        ; HL: pointer to widget table
        ; (SP): original HL
	
        
        PUSH	DE	; place it in stack (was HL)
_vloop:       
        EX	AF, AF'
        LD	A, (HL)
        LD	IXL, A
	INC	HL
        LD	A, (HL)
        LD	IXH, A
        INC 	HL
        EX	AF, AF'

        ; At this point:
        ; DE: Param 2
        ; HL: pointer to widget table (next entry)
        ; (SP): original HL

        EX	(SP), HL
        
        ; At this point:
        ; DE: Param 2
        ; HL: Param 1
        ; (SP): pointer to widget table (next entry)
        PUSH	BC
        PUSH	HL
        PUSH    DE
        PUSH	AF
        VCALL_C
        POP	AF
        POP	DE
        POP	HL  
	POP	BC
        ; Swap back
        EX	(SP), HL
        DJNZ	_vloop

	POP	HL
        POP	IX
        RET

MultiWidget__handleEvent:

        LD	C, Widget__handleEvent
        JP 	MultiWidget_P__foreachVcallC

MultiWidget__draw:
        
        VCALL Widget__drawImpl
        
        LD	C, Widget__draw
        JP 	MultiWidget_P__foreachVcallC
        
MultiWidget__DTOR:
        LD	C, Widget__DTOR
        CALL 	MultiWidget_P__foreachVcallC
        JP	Widget_V__DTOR


MultiWidget__removeChild:
	UNIMPLEMENTED
        RET
