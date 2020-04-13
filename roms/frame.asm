; Initialize a frame
; Inputs:
;	IX:	pointer to frame structure
; Clobbers: A BC DE HL
;
FRAME__INIT:
        ; Get menu width
	LD      A, (IX + FRAME_OFF_WIDTH)
	SRA     A       ; Divide by 2.
	LD      C, A   
	LD      A, 15
	SUB     C       ; Now A has the start column offset. Place it in L
;	LD      L, A
	LD      C, A    ; Save in C
	LD      A, D
	RRCA			; multiply
	RRCA			; by
	RRCA			; thirty-two.
	AND	$E0		; mask off low bits to make
	ADD     A, C
	LD      L,A
	LD	A,D		; bring back the line to A.
	AND	$18		; now $00, $08 or $10.
        OR	$40		; add the base address of screen.
	LD      H,  A

        LD	(IX+FRAME_OFF_SCREENPTR), L
        LD	(IX+FRAME_OFF_SCREENPTR+1), H
	
	; Pick start line again. C still holds the offset.
	LD      A, D
	LD      L, A ; L= start line
	LD      H, 0
	
	LD      B, $58
	
	ADD	HL,HL		; multiply
        ADD	HL,HL		; by
	ADD	HL,HL		; thirty two
	ADD	HL,HL		; to give count of attribute
	ADD	HL,HL		; cells to end of display.
	ADD     HL, BC
	
        LD	(IX+FRAME_OFF_ATTRPTR), L
        LD	(IX+FRAME_OFF_ATTRPTR+1), H
	RET

	; Draw a menu/whatever frame
        ; Inputs:
        ;	IX: Pointer to frame structure
        ; Clobbers: DE, BC, HL, A
        
FRAME__DRAW:
	LD	B, (IX+FRAME_OFF_NUMBER_OF_LINES)  	; Number of entries
        LD	E, (IX+FRAME_OFF_SCREENPTR)       ; Screen..
        LD	D, (IX+FRAME_OFF_SCREENPTR+1)       ; pointer.
       	CALL	MOVEDOWN
 
d1$:
	PUSH 	BC
        PUSH	DE
        
  	LD	HL, LEFTVERTICAL
        CALL	DRAWCHAR
        
        LD	A, E
        ADD	A, (IX+FRAME_OFF_WIDTH) ; Width of menu
        ;DEC 	A
        LD	E, A
        JR	NC, L3
	INC	D
L3:	LD	HL, RIGHTVERTICAL
	CALL	DRAWCHAR
	POP	DE

        CALL 	MOVEDOWN
        POP	BC

        ; Repeat
        DJNZ	d1$

        
        LD	B, (IX+FRAME_OFF_WIDTH)
        INC	B
        INC	B
        LD	A, $FF
        CALL	DRAWHLINE

        ; Prepare header attributes
        LD	E, (IX+FRAME_OFF_ATTRPTR)
        LD	D, (IX+FRAME_OFF_ATTRPTR+1)

	CALL	FILLHEADERLINE
        CALL 	MENU__UPDATESELECTION
        
        LD	E, (IX+FRAME_OFF_SCREENPTR)
        LD	D, (IX+FRAME_OFF_SCREENPTR+1)

        INC	DE
        LD	L, (IX+FRAME_OFF_TITLEPTR)
        LD	H, (IX+FRAME_OFF_TITLEPTR+1)
        PUSH	DE
        CALL	PRINTSTRING
        POP	DE
        LD	A, E
        ADD	A, (IX+FRAME_OFF_WIDTH)
        SUB	5      ; Adjust for logo space
        LD	E, A
        
        ; Place the Logo chars
        LD	HL, HH
        CALL 	DRAWCHAR
        LD	HL, HH
        CALL 	DRAWCHAR
        LD	HL, HH
        CALL 	DRAWCHAR
        LD	HL, HH
        CALL 	DRAWCHAR
        LD	HL, HH
        CALL 	DRAWCHAR
        
        ; Place logo attributes
        LD	E, (IX+FRAME_OFF_ATTRPTR)
        LD	D, (IX+FRAME_OFF_ATTRPTR+1)

        LD	A, E
        ADD	A, (IX+FRAME_OFF_WIDTH)
        SUB 	4    ; 4 logo entries
        LD 	E, A
        
        
        LD	A, %01000010   ; Bright, black+red
        LD	(DE), A
	INC 	DE
        LD	A, %01010110   ; Bright, red+yellow
        LD	(DE), A
	INC 	DE
        LD	A, %01110100   	; Bright, yellow+green
        LD	(DE), A
	INC 	DE
        LD	A, %01100101   	; Bright, green+cyan
        LD	(DE), A
	INC 	DE
        LD	A, %01101000   	; Bright, cyan+black
        LD	(DE), A
	INC 	DE
        XOR 	A;
        LD 	(DE), A
        RET

        

