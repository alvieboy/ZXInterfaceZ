; Each menu stacking entry needs to set up a structure like this
;
; Widget class structure
;
; Off Size 
; 0   2 	Init function       (return instance in HL)
; 2   2         Idle function  	    (param: instance in HL)
; 4   2		Keyboard handler function (param: instance in HL, key in A)
; 6   2         Redraw function     (param: instance in HL)
; 8   2		Get bounds            (param: instance in HL, return 
;			BC=origin (x=B, y=C), DE=target (x=D, y=E)


; Input: 
;	HL: Pointer to widget class structure (pCLASS)
WIDGET__DISPLAY:
	CALL	WIDGET__DISPLAYNODRAW
        LD	C, $6
        JP	CALLWIDGETINDEXEDFUNCTION 	; TAILCALL

        ; STACKINDEX++;
WIDGET__DISPLAYNODRAW:

	LD	A, (STACKINDEX)
        INC	A
        LD	(STACKINDEX), A
	; STACKDATA(32)[ STACKINDEX ].class = pCLASS;
        
        PUSH	HL
        LD	HL, STACKDATA
        ADD	A, A
        ADD	A, A 			; Multiply by 4.
        ADD_HL_A
        
        ; HL now points to STACKDATA.
        POP	DE
        LD	(HL), E ; Store class structure pointer inside STACKDATA
        INC	HL
	LD	(HL), D
        INC	HL
        PUSH	HL 	; Store STACKDATA
        
        ; pINSTANCE = pCLASS->INIT();
        
        EX	DE, HL


        LD	E, (HL) 	; Load init function pointer into HL
        INC	HL
        LD	D, (HL) 	; Load init function pointer into HL
        LD	HL, _l1
        PUSH	HL
        PUSH	DE
        RET			; Call init function
_l1:    ; HL now points to instance class.
        EX	DE, HL
        POP	HL

        LD	(HL), E 	; Store class structure pointer inside STACKDATA
        INC	HL
	LD	(HL), D
        INC	HL              ; Maybe not needed.
        ;
        ; At this point we have: STACKDATA[ STACKINDEX ].class = pCLASS;
        ;                        STACKDATA[ STACKINDEX ].instance = pINSTANCE
	; Call redraw function
	RET
        ;RET
        
WIDGET__CLOSE:  
	; Close current widget, go back to previous
	;LD	A, (STACKINDEX)
        ;DEC	A
        ;LD	(STACKINDEX), A

        ; Redraw previous widget
	;LD	C, $06
        ;JR	CALLWIDGETINDEXEDFUNCTION  ; TAILCALL
	JR	WIDGET__DAMAGE
        
WIDGET__GETCURRENTINSTANCE:
	LD	A, (STACKINDEX)
        LD	HL, STACKDATA
        ADD	A, A
        ADD	A, A 			; Multiply by 4.
        ADD_HL_A
        INC	HL
        INC	HL
        LD	A, (HL)
        INC	HL
        LD	H, (HL)
        LD	L, A
        RET

WIDGET__CLOSENOREDRAW:  
	; Close current widget, go back to previous but do not redraw
	LD	A, (STACKINDEX)
        DEC	A
        LD	(STACKINDEX), A
        RET
        
CALLWIDGETINDEXEDFUNCTION:
        PUSH	AF

	LD	A, (STACKINDEX)
        
        CP 	$FF
        JR	NZ, CW3
        POP	AF
        RET 	; Last widget
CALLWIDGETINDEXEDFUNCTION_INDEXED:
	PUSH	AF
CW3:
        LD	HL, STACKDATA
        ADD	A, A
        ADD	A, A 			; Multiply by 4.

        ADD_HL_A


        ; Load class pointer into DE
        LD	E, (HL)
        INC	HL
        LD	D, (HL)
        INC	HL
        ; Save it so we can fetch the instance pointer later
        PUSH	HL
        
        EX	DE, HL

        ; HL now points to class structure
        LD	A, C ; Offset of function
        ADD_HL_A
        
        ; Load function into DE
        LD	E, (HL)
        INC	HL
        LD	D, (HL)

        ; We still need to fetch instance pointer.
        POP	HL     ; HL points to instance pointer
        ; Load instance pointer
        LD	A, (HL)
        INC	HL
        LD	H, (HL)
        LD	L, A
        ; Instance pointer now in HL
        ; Tailcall optimization
        POP	AF	; Restore A
        PUSH	DE
	RET

WIDGET__CLOSEALL:
	LD	A, 0
        LD	(STACKINDEX), A
        RET


WIDGET__LOOP:
	LD	A, (STACKINDEX)
;        CALL 	DEBUGHEXA

        CP	0
        RET	Z	       	; No more widgets
        ; Keyboard scan
        
	CALL	KEYBOARD
        CALL	_WIDGET_INPUT

	LD	A, (STACKINDEX)
        CP	0
        RET	Z	       	; No more widgets

        ; Idle function call
        LD	C, $2
        CALL	CALLWIDGETINDEXEDFUNCTION
        
        JR 	WIDGET__LOOP	; Continue

_WIDGET_INPUT:	
	BIT	5,(IY+(FLAGS-IYBASE))	; test FLAGS  - has a new key been pressed ?
	RET	Z		; return if not.
        ;CALL	DEBUGHEXA
	LD	DE, (CURKEY)
	LD	A, D
	RES	5,(IY+(FLAGS-IYBASE))	; update FLAGS  - reset the new key flag.
        DEC	A
        RET 	Z 	; Modifier key applied, skip
        LD	A, E
	LD	C, $04
        JP	CALLWIDGETINDEXEDFUNCTION ; Call keyboard handler


WIDGET__DAMAGE:
	; Get this widget bounds
        ;DEBUGSTR "Entering damage\n"
	LD	C, $08
        CALL	CALLWIDGETINDEXEDFUNCTION 
        
        ; BC, DE
        PUSH	AF ; Room in stack
        PUSH	DE
        PUSH	BC
        
        LD	IX, 0
        
        ADD	IX, SP
        ;	IX+0: 	C (y1)
        ;	IX+1: 	B (x1)
        ;	IX+2: 	E (y2)
        ;	IX+3: 	D (x2)

        ;DEBUGSTR "x1 "
        ;DEBUG8 (IX+1)
        ;DEBUGSTR "y1 "
        ;DEBUG8 (IX+0)
        ;DEBUGSTR "x2 "
        ;DEBUG8 (IX+3)
        ;DEBUGSTR "y2 "
        ;DEBUG8 (IX+2)

	
        LD	A, (STACKINDEX)
        DEC	A
        LD	(STACKINDEX), A
        LD	(IX+4), A ; Save in LOCALINDEX
        ;DEBUGSTR "Localindex "
        ;DEBUG8 (IX+4)
        
	; Check parent widget
_containercheckloop:
        LD	A, (IX+4)
        ; Add sanity check if we went OOB
	PUSH	IX
        LD	C, $08
        CALL	CALLWIDGETINDEXEDFUNCTION_INDEXED
	POP	IX

        ;DEBUGSTR "Localindex check "
        ;DEBUG8 (IX+4)

        ; BC, DE of parent widget 
        
        ; If parent x1 (px1) greater than x1, then not contained
        LD	A, (IX+1)    ; x1
        SUB	B            ; x1 -= px1. If negative, not contained
        JR	C, _notcontained
        ; If parent y1 (py1) greater than y1, then not contained
        LD	A, (IX+0)    ; y1
        SUB	C            ; y1 -= py1. If negative,
        JR	C, _notcontained
        ; If x2 greater than parent x2 (px2) then not contained
        LD	A, D
        SUB	(IX+3)
        JR	C, _notcontained
        ; If y2 greater than parent y2 (py2) then not contained
        LD	A, E
        SUB     (IX+2)
        JR	C, _notcontained
        ; Fully contained.
        ;DEBUGSTR "Fully contained\n"

_stackredraw:
	LD	A, (IX+4)
        ;DEBUGSTR "Redraw index "
        ;DEBUG8	(IX+4)
        
        PUSH	IX
        LD	C, $06
        CALL	CALLWIDGETINDEXEDFUNCTION_INDEXED ; Redraw widget
        POP	IX
        
        LD	A, (STACKINDEX)
        CP	(IX+4)
        JR	Z, _exit1 	; No more widgets to redraw
        INC	(IX+4)
        JR	_stackredraw
_notcontained:
	DEC	(IX+4)
        JP	_containercheckloop
_exit1:
	POP	BC
        POP	DE
        POP	AF
        RET

WIDGETROOT_INIT:
	RET
WIDGET__IDLE:
	RET
WIDGETROOT_KEYBOARD:
	RET
WIDGETROOT_REDRAW:
	;DEBUGSTR "Redrawing screen"
	JP	RESTORESCREENAREA
WIDGETROOT_GETBOUNDS:
	LD	BC, 0
        LD	DE, $FFFF
	RET



WIDGETROOT:
	DEFW	WIDGETROOT_INIT
        DEFW	WIDGET__IDLE
        DEFW	WIDGETROOT_KEYBOARD
        DEFW	WIDGETROOT_REDRAW
        DEFW	WIDGETROOT_GETBOUNDS

WIDGET__INIT:
	LD	A, $FF
        LD	(STACKINDEX), A
        LD	HL, WIDGETROOT
        JP 	WIDGET__DISPLAYNODRAW ; TAILCALL
        
