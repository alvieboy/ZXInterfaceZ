Screen__MAXWINDOWS		EQU	8

Screen__CTOR:
        LD	(IX+Screen__numwindows_OFFSET), 0
	RET

Screen__numwindows_OFFSET	EQU	Object__SIZE+0
Screen__windows_OFFSET		EQU	Object__SIZE+1
Screen__SIZE 			EQU 	Object__SIZE+1+(Screen__MAXWINDOWS*2)

; HL: Window pointer
; DE: Location on screen (xy)

Screen__addWindow:
        LD	A, (IX+Screen__numwindows_OFFSET)
        PUSH	IX
        
        ADD	A, A	; *2
        LD	BC, Screen__windows_OFFSET
        ADD	A, C
        LD	C, A
        ; BC now holds offset
        ADD	IX, BC
        
        LD	(IX), L
        LD	(IX+1), H ; Store window pointer.
        POP	IX
        
        ; Move window into place
        SWAP_IX_HL
        EX	DE, HL

        PUSH	DE
        LD	E, $FF	; Do not change width/height
        VCALL	Widget__resize
        POP	DE
        
        EX	DE, HL
        SWAP_IX_HL
        
	; Increment number of windows
        
        INC	(IX+Screen__numwindows_OFFSET)

        RET

Screen__addWindowCentered:

        SWAP_IX_HL
        PUSH 	HL
        ;CALL Widget__getWidth
        LD	A, (IX+Widget__width_OFFSET)
        ; Divide by 2
        LD	D, -16
        SRA 	A
        ADD	A, D
        NEG     
        LD	D, A

        
        ; Calculate height
        LD	A, (IX+Widget__height_OFFSET)
        
       
        LD	E, -12
        SRA	A
        ADD	A, E
        NEG
        LD	E, A
        POP	HL
        SWAP_IX_HL

	JP 	Screen__addWindow ; TAILCALL
        

;Screen__destroyWindow: ; Window in HL
;        RET

Screen__redraw:
	PUSH 	IX
        PUSH	BC

        LD	BC, Screen__windows_OFFSET
        ADD	IX, BC
        
	LD	B, (IX+Screen__numwindows_OFFSET-Screen__windows_OFFSET)
_drawnext:
        LD	L, (IX)
        LD	H, (IX+1)
        PUSH	BC
        
        SWAP_IX_HL
        
        VCALL	Widget__draw
        SWAP_IX_HL
        POP	BC
        
        INC	IX
        INC	IX
        DJNZ	_drawnext
        POP	BC
        POP	IX
        RET

Screen_SP__hasKbdEvent:
	CALL	KEYBOARD
                  
	LD	HL, FLAGS

	BIT	5, (HL)   ; test FLAGS  - has a new key been pressed ?
	RET	Z
                
	LD	DE, (CURKEY)
	LD	A, D
        LD	HL, FLAGS
	RES	5, (HL)	; update FLAGS  - reset the new key flag.
        DEC	A
        RET	Z 	; Modifier key applied, skip
        LD	A, E
        RET ; NZ

Screen__eventLoop:
        LD	A, (IX+Screen__numwindows_OFFSET)
	CP	0
        RET	Z	       	; No more widgets
        
	PUSH 	IX
        PUSH	BC
        CALL	Screen_P__loadIXWithTopWindow
        
        ; If wiget is not visible, then we should exit main loop
        BIT    	WIDGET_FLAGS_VISIBLE_BIT, (IX+Widget__flags_OFFSET)
        JR	Z, _nowidget
        
        CALL	Screen_SP__hasKbdEvent
        JR	Z, _noevent
        
	LD	HL, (CURKEY)
        XOR	A
        DEBUGSTR "sendin handleEvent to "
        DEBUGHEXIX
        
        VCALL	Widget__handleEvent

_noevent:
        POP	BC
        POP	IX
        JR	Screen__eventLoop
_nowidget:
	POP	BC
        POP	IX
        RET

Screen__removeWindow:
	
	; TODO: check if correct window is being removed.

        LD	A, (IX+Screen__numwindows_OFFSET)
	DEC 	(IX+Screen__numwindows_OFFSET)

        PUSH	HL
        CALL	Screen__DAMAGE
	POP	HL
	RET




; Check if coordinates in IX widget (WA) are fully contained (enclosed) in IY widget (WB)
; IY should be the parent widget (on back of IX)
;
; if (WA.x() < WB.x())
;	return false;
;
; if (WA.y() < WB.y())
;	return false;
;
; if (WA.x2() > WB.x2())
;	return false;
;
; Returns carry set if false, carry clear if true
; 

Screen__checkIXIYenclosed:
	DEBUGSTR "Enclosed:\n"
	LD	A, (IX+Widget__x_OFFSET)
        DEBUGSTR " IX x:"
        DEBUGHEXA
        DEBUGSTR " IY x:"
        DEBUG8 (IY+Widget__x_OFFSET)
        SUB	(IY+Widget__x_OFFSET)
        RET	C
        
        
	LD	A, (IX+Widget__y_OFFSET)
        DEBUGSTR " IX y:"
        DEBUGHEXA
        DEBUGSTR " IY y:"
        DEBUG8 (IY+Widget__y_OFFSET)
        SUB	(IY+Widget__y_OFFSET)
        RET	C

        ; Compute x2


	LD	A, (IX+Widget__x_OFFSET)
        ADD	A, (IX+Widget__width_OFFSET)
	LD	C, A ; B holds WA x2
        LD	A, (IY+Widget__x_OFFSET)
        ADD	A, (IY+Widget__width_OFFSET)
        ; A holds WB x2

        DEBUGSTR " IY x2:"
        DEBUGHEXA
        DEBUGSTR " IX x2:"
        DEBUGHEXC

        SUB	C
        RET	C

        ; Compute x2
	LD	A, (IY+Widget__y_OFFSET)
        ADD	A, (IY+Widget__height_OFFSET)
	LD	C, A ; B holds WA x2
        LD	A, (IX+Widget__y_OFFSET)
        ADD	A, (IX+Widget__height_OFFSET)
        ; A holds WB x2
        SUB	C
        RET	


Screen_P__getWindowPointer:
	; Returns IX + Screen__windows + (2* (A) )
        LD	L, A
        LD	H, 0
        ADD	HL, HL
        LD	BC, Screen__windows_OFFSET
        ADD	HL, BC
        PUSH	IX
        POP	BC
        ADD	HL, BC
        OR	A 	; Clear carry
        RET

Screen_P__getActiveWindowPointer:
	; Returns IX + Screen__windows + (2* (Screen__numwindows-1) )
	LD	A, (IX+Screen__numwindows_OFFSET)
        DEC	A
        RET	C
        JP	Screen_P__getWindowPointer

Screen_P__loadIXWithTopWindow:
        CALL	Screen_P__getActiveWindowPointer
       	LD	B, (HL)
        INC	HL
        LD	A, (HL)
        DEC	HL
        LD	IXL, B
        LD	IXH, A
	RET



; Apply damage by widget @HL
Screen__DAMAGE:
        DEBUGSTR "Damage "
        DEBUGHEXIX

	PUSH	IX
        PUSH 	HL
        
        CALL	Screen_P__getActiveWindowPointer
        JR	C, _redrawscreen ; No windows, redraw whole screen.
        
        
        DEBUGSTR "HL pointer "
        DEBUGHEXHL
        
        ; B should contain number of windows
	LD	B, (IX+Screen__numwindows_OFFSET)
        DEC	B
        
        POP	IX	; IX now holds the "damaging" widget.
_checkdamage:
        ; Get "damaged" widget into IY
        LD	A, (HL)
        LD	IYL, A
	INC	HL
        LD	A, (HL)
        LD	IYH, A
        DEC	HL
        
        DEBUGSTR "IX widget "
        DEBUGHEXIX
        DEBUGSTR "IY widget "
        DEBUGHEXIY
        
        CALL	Screen__checkIXIYenclosed
        JR	NC, _enclosed
        ; Go back to parent. See if we need to redraw screen
        DEC	B
        JR	C, _redrawscreen
        ; Move to parent window, recheck
        DEC	HL
        DEC	HL
        JR 	_checkdamage
_redrawscreen:
	; We need to redraw screen.
        DEBUGSTR "Screen: restore\n"
        CALL	RESTORESCREENAREA
        LD	B, 0	
        ; Fully enclosed.
_enclosed:
        DEBUGSTR "Screen: stack draw\n"	
        ; Start drawing up until we reach the end.

        POP	IX 	; Get back screen pointer
        
        LD	A, B
        
        PUSH	BC	; Save counter.
        CALL	Screen_P__getWindowPointer ; into HL
        POP	BC
        
_redrawone:
        LD	E, (HL)
	INC	HL
        LD	D, (HL)
        INC	HL
        DEBUGSTR " > Widget "
        DEBUGHEXDE
        
        SWAP_IX_DE
        PUSH	BC
        PUSH	DE
        PUSH	HL
        VCALL	Widget__draw
        POP	HL
        POP	DE
        POP	BC
        SWAP_IX_DE
        
        INC	B
        DEBUGSTR "After incr index "
        DEBUGHEXB
        LD A, (IX+Screen__numwindows_OFFSET)
        DEBUGSTR "Limit "
        DEBUGHEXA
        
        

        LD	A, B
        CP 	(IX+Screen__numwindows_OFFSET)
        JR	NZ, _redrawone
        
        RET

Screen__closeAll:
	;LD	A, (IX+Screen__numwindows_OFFSET)
        ;CP	0
        ;RET	Z
        
        ;PUSH 	IX
        ;CALL	Screen_P__loadIXWithTopWindow
	LD	(IX+Screen__numwindows_OFFSET), 0
        
        
        RET
