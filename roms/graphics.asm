; Draw a simple H line.
; DE: start address
;  B: size
; Corrupts: A
DRAWHLINE:
	LD	(DE), A
	LD	A, $FF
        INC	DE
        DJNZ	DRAWHLINE
        RET

;
; Get screen start for XY location
; Inputs:
;	C: X coordinate
;	D: Y coordinate
; Returns:
;       HL: Screen pointer

GETXYSCREENSTART:
        LD	A, D
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
        RET

;
; Get attribute start for XY location
; Inputs:
;	C: X coordinate
;	D: Y coordinate
; Returns:
;       HL: Attribute pointer

GETXYATTRSTART:
	LD	A, D
	LD      L, A ; L= start line
	LD      H, 0
	PUSH	BC
        
	LD      B, $58
	
	ADD	HL,HL		; multiply
        ADD	HL,HL		; by
	ADD	HL,HL		; thirty two
	ADD	HL,HL		; to give count of attribute
	ADD	HL,HL		; cells to end of display.
	ADD     HL, BC
	
        POP	BC
	RET


MOVEDOWN:
	LD	A,E
	CP 	%11100000 	      	; 0xE0, last entry of the band	
	JR	NC, NEXTBAND$           ; Yes, move to next band
	ADD	A,$20                   ; Othewise just increment
	LD	E, A
	LD	A, D
	AND 	%01011000               ; Make sure we don't overflow
	LD	D,A 
	RET        

NEXTBAND$:
	AND 	%00011111		; Blanks out y coords of LSB
	LD	E,A			; copy back to destination LSB
	LD	A,D			; get destination MSB
	ADD 	A,$08	                ; increment band bit - this will push us into attribute space if we're already in the last band
	AND 	%01011000		; 0101 1000 - ensures 010 prefix is maintained, wiping out any carry. Also wipes Y pixel offset.
	LD 	D,A			; copy back into destination MSB	
	RET

MOVENEXTLINE:
	CALL	MOVEDOWN
        LD	A, E
        AND	%11100000
        LD	E, A
        RET
        

	;
        ; Draw char pointed by HL
        ; Inputs:	
        ;	HL: pointer to char bitmap
        ; Clobbers: A
DRAWCHAR:
	PUSH	BC
        PUSH	DE
	LD	B, 8
PCLOOP2:
	LD	A, (HL)
        LD	(DE), A
        INC	D
        INC 	HL
        DJNZ 	PCLOOP2
        POP	DE
        INC 	DE
        POP	BC
        RET

	; Move DE one pixel line below
PMOVEDOWN:
	INC 	D				; Go down onto the next pixel line
	LD 	A, D				; Check if we have gone onto next character boundary
	AND	7
	RET 	NZ				; No, so skip the next bit
	LD 	A,E				; Go onto the next character line
	ADD 	A, 32
	LD 	E,A
	RET 	C				; Check if we have gone onto next third of screen
	LD 	A,D				; Yes, so go onto next third
	SUB 	8
	LD 	D,A
	RET

;	In: 	HL: pointer to bitmap structure
;		DE: screen address
;
;		Bitmap structure contains:
;			DB 0	// Width in bytes
;			DB 0	// Height in lines
;			Db 0..  // Bitmap data.
DISPLAYBITMAP:
        LD	C, (HL)  ; Width in bytes
        INC	HL
        LD	A, (HL)	 ; Number of lines
        INC	HL
DOLINE:
        LD	B, $0
	PUSH	DE
        PUSH	BC
        LDIR
        POP	BC
        POP	DE
        LD	B, A      ; PMOVEDOWN corrupts A, save it.
	CALL	PMOVEDOWN
        LD	A, B
        DEC	A
        JR	NZ, DOLINE
        RET
