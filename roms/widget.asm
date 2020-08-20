; Each menu stacking entry needs to set up a structure like this
;
; Widget class structure
;
; Off Size 
; 0   2 	Init function       (return instance in HL)
; 2   2         Idle function  	    (param: instance in HL)
; 4   2		Keyboard handler function (param: instance in HL, key in A)
; 6   2         Redraw function     (param: instance in HL)
; 8   2		Get rect            (param: instance in HL, return 
;			BC=origin (x=B, y=C), DE=width/height)


; Input: 
;	HL: Pointer to widget class structure (pCLASS)
WIDGET__DISPLAY:
	
        ; STACKINDEX++;
        
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
        LD	C, $6
        JP	CALLWIDGETINDEXEDFUNCTION 	; TAILCALL

        ;RET
        
WIDGET__CLOSE:  
	; Close current widget, go back to previous
	LD	A, (STACKINDEX)
        DEC	A
        LD	(STACKINDEX), A

        ; Redraw previous widget
	LD	C, $06
        JR	CALLWIDGETINDEXEDFUNCTION  ; TAILCALL

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
        JR	NZ, _l3
        POP	AF
        RET 	; Last widget
_l3:
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
	LD	A, $FF
        LD	(STACKINDEX), A
        RET


WIDGET__LOOP:
	LD	A, (STACKINDEX)
;        CALL 	DEBUGHEXA

        CP	$FF
        RET	Z	       	; No more widgets
        ; Keyboard scan
        
	CALL	KEYBOARD
        CALL	_WIDGET_INPUT

	LD	A, (STACKINDEX)
        CP	$FF
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

WIDGETROOT_INIT:
	RET
WIDGET__IDLE:
	RET
WIDGETROOT_KEYBOARD:
	RET
WIDGETROOT_REDRAW:
	RET



WIDGETROOT:
	DEFW	WIDGETROOT_INIT
        DEFW	WIDGET__IDLE
        DEFW	WIDGETROOT_KEYBOARD
        DEFW	WIDGETROOT_REDRAW

WIDGET__INIT:
	LD	A, $FF
        LD	(STACKINDEX), A
        LD	HL, WIDGETROOT
        RET 	;JP 	WIDGET__DISPLAY ; TAILCALL
        
