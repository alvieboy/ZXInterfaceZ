Screen__MAXWINDOWS		EQU	8

Screen__CTOR:
	DEBUGSTR "Screen CTOR "
        DEBUGHEXIX
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
        
        DEBUGSTR "addWindow off "
        DEBUGHEXIX
        
        LD	(IX), L
        LD	(IX+1), H ; Store window pointer.
        POP	IX
        
        DEBUGSTR "Addind window "
        DEBUGHEXHL

        ; Move window into place
        SWAP_IX_HL
        EX	DE, HL

        DEBUGSTR "Window base 1 "
        DEBUGHEXIX
        
        PUSH	DE
        LD	E, $FF	; Do not change width/height
        VCALL	Widget__resize
        POP	DE
        
        EX	DE, HL
        SWAP_IX_HL
        
        DEBUGSTR "Window moved\n"
	; Increment number of windows


        DEBUGSTR "Screen base"
        DEBUGHEXIX
        
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
        DEBUGHEXA
        POP	HL
        SWAP_IX_HL

        DEBUGSTR "WinC Width "
        DEBUGHEXD
        DEBUGSTR "WinC Height"
        DEBUGHEXE
        
        
	JP 	Screen__addWindow ; TAILCALL
        

;Screen__destroyWindow: ; Window in HL
;        RET

Screen__redraw:
	PUSH 	IX
        PUSH	BC

        LD	BC, Screen__windows_OFFSET
        ADD	IX, BC
        
        DEBUGSTR "Windows pointer "
        DEBUGHEXIX
        
	LD	B, (IX+Screen__numwindows_OFFSET-Screen__windows_OFFSET)
        DEBUGSTR "Num windows "
        DEBUGHEXB
_drawnext:
        LD	L, (IX)
        LD	H, (IX+1)
        PUSH	BC
        
        SWAP_IX_HL
        
        DEBUGSTR "Call redraw "
        DEBUGHEXIX

        VCALL	Widget__draw
        SWAP_IX_HL
        POP	BC
        
        INC	IX
        INC	IX
        DEBUGSTR "Redraw done\n";
        DJNZ	_drawnext
        DEBUGSTR "All widgets redrawn\n"
        POP	BC
        POP	IX
        RET

Screen__hasKbdEvent:
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
        
        ; Keyboard scan
        
	CALL	KEYBOARD
                  
	LD	HL, FLAGS

	BIT	5, (HL)   ; test FLAGS  - has a new key been pressed ?
	JR	Z, Screen__eventLoop
                
	LD	DE, (CURKEY)
	LD	A, D
        LD	HL, FLAGS
	RES	5, (HL)	; update FLAGS  - reset the new key flag.
        DEC	A
        JR	Z, Screen__eventLoop 	; Modifier key applied, skip
        LD	A, E

	PUSH 	IX
        PUSH	BC
        LD	BC, Screen__windows_OFFSET
        LD	A, (IX+Screen__numwindows_OFFSET)
        DEC	A
        ADD	A, A ; * 2
        ADD	A, C
        JR	NC, _noinc
        INC	B
_noinc:
	LD	C, A
        ADD	IX, BC
       	LD	B, (IX)
        LD	A, (IX+1)
        LD	IXL, B
        LD	IXH, A
        
        ;SWAP_IX_HL
        
	LD	HL, (CURKEY)
        XOR	A
        VCALL	Widget__handleEvent
        ;SWAP_IX_HL
        
        POP	BC
        POP	IX

        JP	Screen__eventLoop

Screen__removeWindow:
	
        DEBUGSTR "Removing window "
        DEBUGHEXHL
	; TODO: check if correct window is being removed.

	DEBUGSTR "Current windows "
        LD	A, (IX+Screen__numwindows_OFFSET)
        DEBUGHEXA

	DEC 	(IX+Screen__numwindows_OFFSET)

        PUSH	HL
        CALL	Screen__DAMAGE
	POP	HL
	RET




; Check if coordinates in IX widget (WA) are fully contained (enclosed) in IY widget (WB)
;
; if (WA.x() < WB.x())
;	return false;
;
; if (WA.y() > WB.y())
;	return false;
;
; if (WA.x2() > WB.x2())
;	return false;
;
; Returns carry set if false, carry clear if true
; 

Screen__checkIXIYenclosed:
	LD	A, (IX+Widget__x_OFFSET)
        SUB	(IY+Widget__x_OFFSET)
        RET	C
	LD	A, (IY+Widget__y_OFFSET)    
        SUB	(IX+Widget__x_OFFSET)       
        RET	C

        ; Compute x2
	LD	A, (IX+Widget__x_OFFSET)
        ADD	A, (IX+Widget__width_OFFSET)
	LD	C, A ; B holds WA x2
        LD	A, (IY+Widget__x_OFFSET)
        ADD	A, (IY+Widget__width_OFFSET)
        ; A holds WB x2
        SUB	C
        RET	C

        ; Compute x2
	LD	A, (IY+Widget__x_OFFSET)
        ADD	A, (IY+Widget__width_OFFSET)
	LD	C, A ; B holds WA x2
        LD	A, (IX+Widget__x_OFFSET)
        ADD	A, (IX+Widget__width_OFFSET)
        ; A holds WB x2
        SUB	C
        RET	
        

; Apply damage by widget @HL
Screen__DAMAGE:
        DEBUGSTR "Damage "
        DEBUGHEXIX

	PUSH	IX
        PUSH 	HL
        
	LD	A, (IX+Screen__numwindows_OFFSET)
        ; Save in B
        DEC	A
        LD	B, A        
        JR	C, _redrawscreen ; No windows, redraw whole screen.
        
        
        PUSH	BC
        LD	BC, Screen__windows_OFFSET
        ADD	A, C
;        JR	NC, _noinc
;        INC	B
;_noinc:
	LD	C, A

        PUSH	IX
        POP	HL
        ADD	HL, BC
        POP	BC
        
        
        
        DEBUGSTR "HL pointer "
        DEBUGHEXHL
        
        
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
        
        CP 	(IX+Screen__numwindows_OFFSET)
        RET	Z ; Nothing to do

        ADD	A, A ; *2
        LD	DE, Screen__windows_OFFSET
        ADD	A, E
        LD	E, A
        ; TBD: overflow
        
        PUSH	IX
        POP	HL
        ADD	HL, DE 	; Get pointer to widget
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
        VCALL	Widget__draw
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


