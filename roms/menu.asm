;
; MENU structure definition
; off size meaning
; 00  01   Width of menu
; 01  01   Number of entries
; 02  01   Current selected entry
; 03  02   Screen pointer (start)
; 05  02   Attribute pointer (start)
; 07  02   Ptr to menu title
; 09  02   Ptr to entry
; *   *    Ptr to entry

; 
;  Functions in this file
;
;  MENU__INIT       : Initialise a menu
;  MENU__DRAW       : Draw the menu (call once)
;  MENU__CHOOSENEXT : Choose next entry in the menu
;  MENU__CHOOSEPREV : Choose previous entry in menu



; MENU_INIT - Initialise a new menu
;   Inputs
;       HL:     Pointer to menu structure.
;       D:      Line to display menu at
;   Outputs
;       None
;   Clobbers: IX, DE
MENU__INIT:
        PUSH	HL
	PUSH	HL
        POP	IX
	; Get menu width
	LD      A, (IX)
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

        LD	(IX+3), L
        LD	(IX+4), H
	
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
	
        ;LD	DE, ATTR+64
        LD	(IX+5), L
        LD	(IX+6), H
	POP     HL
        RET

MENU__DRAW:
	PUSH  	HL
        POP	IX
	LD	B, (IX+1)
        LD	E, (IX+3)
        LD	D, (IX+4)
       	CALL	MOVEDOWN
 
d1$:
	PUSH 	BC
        PUSH	DE
        
  	LD	HL, LEFTVERTICAL
        CALL	PUTRAW
        
        LD	A, E
        ADD	A, (IX) ; Width of menu
        ;DEC 	A
        LD	E, A
        JR	NC, L3
	INC	D
L3:	LD	HL, RIGHTVERTICAL
	CALL	PUTRAW
	POP	DE

        CALL 	MOVEDOWN
        POP	BC

        ; Repeat
        DJNZ	d1$

        
        LD	B, (IX)
        INC	B
        INC	B
        LD	A, $FF
        CALL	DRAWHLINE

        ; Prepare header attributes
        LD	E, (IX+5)
        LD	D, (IX+6)

	CALL	FILLHEADERLINE
        CALL 	MENU__UPDATESELECTION
        
        LD	E, (IX+3)
        LD	D, (IX+4)

        ;LD	DE, SCREEN+64
        INC	DE
        LD	L, (IX+7)
        LD	H, (IX+8)
        PUSH	DE
        CALL	PUTSTRING
        POP	DE
        LD	A, E
        ADD	A, (IX)
        SUB	5
        LD	E, A
        
        ; Place the Logo chars
        LD	HL, HH
        CALL 	PUTRAW
        LD	HL, HH
        CALL 	PUTRAW
        LD	HL, HH
        CALL 	PUTRAW
        LD	HL, HH
        CALL 	PUTRAW
        LD	HL, HH
        CALL 	PUTRAW
        ; Place logo attributes
        ;LD	DE, ATTR + 64 
        LD	E, (IX+5)
        LD	D, (IX+6)

        LD	A, E
        ADD	A, (IX)
        SUB 	4
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

        ; Draw contents
        LD	E, (IX+3)
        LD	D, (IX+4)
        INC	DE
        JR 	MENU__DRAWCONTENTS

FILLSLINE:     
	PUSH	BC
	PUSH	DE
        LD	B, (IX)  	; Load width of menu
        INC	B               ; Add +2 for extra borders.
        INC	B               
LP1:    LD	(DE), A
        INC 	DE
        DJNZ	LP1
        POP	DE
        POP	BC
	RET
FILLHEADERLINE:
        ; Prepare header attributes
        PUSH 	DE
	PUSH	BC
 	LD	B, (IX)
HEADER$:
        LD	A, $07
        LD	(DE), A
        INC	DE
        DJNZ 	HEADER$
        ; Last one is black
        LD	A, $0
        LD	(DE), A
        POP	BC
	POP 	DE
        RET
        
MENU__UPDATESELECTION:
	PUSH	BC
        PUSH	DE
        
        LD	E, (IX+5)
        LD	D, (IX+6)
        
        LD	B, (IX+1) 	; Number of entries
        LD	C, $0           ; Start at 0 for "active" comparison
L6:
        LD	A, E            ; Move to next attribute line
        ADD	A, $20
        LD	E, A
        JR	NC, L7
        INC	D
L7:

	LD	A, (IX+2)	; Get active entry
        CP	C		; Compare
        LD	A, %01111000    ; Normal color
        JR	NZ,  L5
        LD	A, %01101000    ; Cyan, for selected
L5:
        INC	C		; Next entry
        CALL	FILLSLINE
        DJNZ	L6
	POP	DE
        POP 	BC
        RET


; Draw menu contents
; Inputs: 	IX : 	pointer to menu structure
;   		DE : 	Ptr to screen header location
MENU__DRAWCONTENTS:
	LD	B, (IX+1) ; Get number of entries
        PUSH	IX
        POP	HL
        LD	A, L
        ADD	A, 9
        LD	L, A
        JR	NC, MD1
        INC	H
MD1:    ; HL now contains the first entry.
        PUSH	BC
        CALL	MOVEDOWN
	PUSH	DE
        LD	C, (HL)
        INC	HL
        LD	B, (HL)
        INC	HL
        PUSH	HL
        PUSH	BC
        POP	HL
        ;LD	A, 'L'
        ;CALL PUTCHAR
        CALL 	PUTSTRING
        POP	HL
        POP	DE
        POP	BC
        
        DJNZ	MD1
	RET

MENU__CHOOSENEXT:
        PUSH	HL
        POP	IX
	LD	A, (IX+2)
        INC	A
        CP	(IX+1)
        RET	Z
	LD	(IX+2), A
	JR   	MENU__UPDATESELECTION

MENU__CHOOSEPREV:
        PUSH	HL
        POP	IX
	LD	A, (IX+2)
        SUB	1
        RET	C
	LD	(IX+2), A
	JR   	MENU__UPDATESELECTION
