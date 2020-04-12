include "menu_defs.asm"
;  Functions in this file
;
;  MENU__INIT       : Initialise a menu
;  MENU__DRAW       : Draw the menu (call once)
;  MENU__CHOOSENEXT : Choose next entry in the menu
;  MENU__CHOOSEPREV : Choose previous entry in menu
;  MENU__ACTIVATE   : Activate current menu entry (execute callback)



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
	LD      A, (IX + MENU_OFF_WIDTH)
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

        LD	(IX+MENU_OFF_SCREENPTR), L
        LD	(IX+MENU_OFF_SCREENPTR+1), H
	
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
        LD	(IX+MENU_OFF_ATTRPTR), L
        LD	(IX+MENU_OFF_ATTRPTR+1), H
        ; Add some default settings.
	XOR	A

	POP     HL
        RET

; Clear area used by menu.
; Attribute used in A
MENU__CLEAR:

        PUSH	AF		; Save attribute
	PUSH  	HL
        POP	IX
	
        LD	A, (IX+MENU_OFF_MAX_VISIBLE_ENTRIES)  	; Number of entries

        INC	A		; Include header
        SLA	A
        SLA	A
        SLA	A               ; Multiply by 8
        INC	A		; Include (one line) footer
        LD	L, (IX+MENU_OFF_SCREENPTR)       ; Screen..
        LD	H, (IX+MENU_OFF_SCREENPTR+1)     ; pointer.
        PUSH	HL
        POP	DE
        
        LD	C, A            ; Rows to process

CLRNEXT:
        XOR	A
        LD	B, (IX+MENU_OFF_WIDTH)         ; Width of menu
        INC	B
        INC	B
CLRLINE:
        LD	(HL), A
        INC	HL
        DJNZ	CLRLINE
        CALL	PMOVEDOWN
        PUSH	DE
        POP	HL
        
        DEC	C
        XOR	A
        OR	C		; A is still 0
	JR	NZ, CLRNEXT
        
        
        POP	AF
        ;RET
        
        
        
        
        ; Now, clear attributes
        LD	L, (IX+MENU_OFF_ATTRPTR)
        LD	H, (IX+MENU_OFF_ATTRPTR+1)

        LD	C, (IX+MENU_OFF_MAX_VISIBLE_ENTRIES)       ; Attribute lines to clear
        INC	C               ; Include header
        INC	C               ; And footer
        
        LD	D, A	; Save attribute in D
        
CLRATTRNEXT:
        LD	B, (IX+MENU_OFF_WIDTH)         ; Width of menu
        INC	B
        INC	B
        PUSH	HL
CLRATTR:
        LD	(HL), A
        INC 	HL
        DJNZ	CLRATTR
        POP	HL
        
        LD	A, 32
        ADD_HL_A ; Add A to HL
        XOR	A

        DEC	C
        OR	C
        LD	A, D ; Restore attibute
        JR	NZ, CLRATTRNEXT
        
        RET
        

MENU__DRAW:
	PUSH  	HL
        POP	IX
	LD	B, (IX+MENU_OFF_MAX_VISIBLE_ENTRIES)  	; Number of entries
        LD	E, (IX+MENU_OFF_SCREENPTR)       ; Screen..
        LD	D, (IX+MENU_OFF_SCREENPTR+1)       ; pointer.
       	CALL	MOVEDOWN
 
d1$:
	PUSH 	BC
        PUSH	DE
        
  	LD	HL, LEFTVERTICAL
        CALL	DRAWCHAR
        
        LD	A, E
        ADD	A, (IX+MENU_OFF_WIDTH) ; Width of menu
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

        
        LD	B, (IX+MENU_OFF_WIDTH)
        INC	B
        INC	B
        LD	A, $FF
        CALL	DRAWHLINE

        ; Prepare header attributes
        LD	E, (IX+MENU_OFF_ATTRPTR)
        LD	D, (IX+MENU_OFF_ATTRPTR+1)

	CALL	FILLHEADERLINE
        CALL 	MENU__UPDATESELECTION
        
        LD	E, (IX+MENU_OFF_SCREENPTR)
        LD	D, (IX+MENU_OFF_SCREENPTR+1)

        INC	DE
        LD	L, (IX+MENU_OFF_MENU_TITLE)
        LD	H, (IX+MENU_OFF_MENU_TITLE+1)
        PUSH	DE
        CALL	PRINTSTRING
        POP	DE
        LD	A, E
        ADD	A, (IX+MENU_OFF_WIDTH)
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
        LD	E, (IX+MENU_OFF_ATTRPTR)
        LD	D, (IX+MENU_OFF_ATTRPTR+1)

        LD	A, E
        ADD	A, (IX+MENU_OFF_WIDTH)
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

        ; Draw contents
        LD	E, (IX+MENU_OFF_SCREENPTR)
        LD	D, (IX+MENU_OFF_SCREENPTR+1)
        INC	DE
        JR 	MENU__DRAWCONTENTS

FILLSLINE:     
	PUSH	BC
	PUSH	DE
        LD	B, (IX+MENU_OFF_WIDTH)  	; Load width of menu
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
 	LD	B, (IX+MENU_OFF_WIDTH)
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
        
        PUSH	IX
        EX	(SP), IY  ; IY now in stack, IY=IX
        ; We need to offset IY to the "offset" we are looking at the menu.
        ; There is for sure a better way to do this.
        LD	A, (IX+MENU_OFF_DISPLAY_OFFSET)
        LD	C, A
        ADD	A, A ; *2
        ADD	A, C ; *3
        ADD_IY_A ; macro
        
        LD	E, (IX+MENU_OFF_ATTRPTR)
        LD	D, (IX+MENU_OFF_ATTRPTR+1)
        
        LD	B, (IX+MENU_OFF_MAX_VISIBLE_ENTRIES) 	; Number of entries
        ;LD	C, $0           ; Start at 0 for "active" comparison
        LD	C, (IX+MENU_OFF_DISPLAY_OFFSET)
L6:
        LD	A, E            ; Move to next attribute line
        ADD	A, $20
        LD	E, A
        JR	NC, L7
        INC	D
L7:
	LD	A, (IX+MENU_OFF_SELECTED_ENTRY)	; Get active entry
        CP	C		; Compare
        JR	NZ,  L5
        
        ; load item flags.
        BIT 	0, (IY+MENU_OFF_FIRST_ENTRY)
        JR	NZ, L9 ; Disabled
        LD	A, MENU_COLOR_SELECTED ; Cyan, for selected
	JR	L8
L9:	LD	A, MENU_COLOR_DISABLED
	JR 	L8
L5:
        LD	A, MENU_COLOR_NORMAL ; Normal color
L8:
        INC	IY
        INC	IY
        INC	IY
        INC	C		; Next entry
        CALL	FILLSLINE
        DJNZ	L6
        POP	IY
	POP	DE
        POP 	BC
        RET


; Draw menu contents
; Inputs: 	IX : 	pointer to menu structure
;   		DE : 	Ptr to screen header location

MENU__DRAWCONTENTS_NO_DE:
	LD	E, (IX+MENU_OFF_SCREENPTR)
        LD	D, (IX+MENU_OFF_SCREENPTR+1)
        INC	DE
MENU__DRAWCONTENTS:
	LD	B, (IX+MENU_OFF_MAX_VISIBLE_ENTRIES) ; Get number of entries
        PUSH	IX
        POP	HL
        
        ; Move HL pointer to correct offset
        LD	A, L
        ADD	A, MENU_OFF_FIRST_ENTRY
        LD	L, A
        JR	NC, MD1
        INC	H
MD1:    ; HL now contains the first entry.
        ; Move past offset if we have one. This is used for scrolling.
        LD	A, (IX+MENU_OFF_DISPLAY_OFFSET)
        LD	C, A
        ADD	A, C ; Multiply by 2.
        ADD	A, C ; Multiply by 3.
        ADD_HL_A  ; Add to HL offset
        

MD2:
        PUSH	BC
        CALL	MOVEDOWN
	PUSH	DE
        ; Load entry attributes
        LD	A, (HL)
        INC 	HL
        LD	C, (HL) ; Load entry..
        INC	HL
        LD	B, (HL) ; pointer
        INC	HL
        PUSH	HL
        PUSH	BC
        POP	HL
        LD	A, (IX+MENU_OFF_WIDTH)
        CALL 	PRINTSTRINGPAD
        POP	HL
        POP	DE
        POP	BC
        
        DJNZ	MD2

;	Check if we need up/down arrows.

	LD	A, (IX+MENU_OFF_SCREENPTR)
        LD	D, (IX+MENU_OFF_SCREENPTR+1)
        INC	A
        ADD	A, (IX+MENU_OFF_WIDTH)
        LD	E,A
        
        CALL	MOVEDOWN

        ; Up arrow needed if offset is not zero
        XOR	A
        CP	(IX+MENU_OFF_DISPLAY_OFFSET)
        JR	Z, nouparrow
        LD	HL, UPARROW
        JR	f1
nouparrow:  
	LD	HL, RIGHTVERTICAL ; Empty (space) with vertical bar on right
f1:
        CALL	DRAWCHAR
        DEC	DE



        ; Draw down arrow
        LD	B, (IX+MENU_OFF_MAX_VISIBLE_ENTRIES)
        DEC	B
POSITION_DOWN_ARROW:
        CALL	MOVEDOWN
        DJNZ	POSITION_DOWN_ARROW

	LD	A, (IX+MENU_OFF_DISPLAY_OFFSET)
        ADD 	A, (IX+MENU_OFF_MAX_VISIBLE_ENTRIES)
        CP	(IX+MENU_OFF_DATA_ENTRIES)
        JR	NC, nodownarrow
        LD	HL, DOWNARROW
	JR 	f2
nodownarrow:
	LD	HL, RIGHTVERTICAL ; Empty (space) with vertical bar on right
f2:
        CALL	DRAWCHAR


	RET

MENU__CHOOSENEXT:
        PUSH	HL
        POP	IX
	LD	A, (IX+MENU_OFF_SELECTED_ENTRY)
        INC	A
        CP	(IX+MENU_OFF_DATA_ENTRIES)
        RET	Z
	LD	(IX+MENU_OFF_SELECTED_ENTRY), A
        ; Check if we need to scroll

        LD	A, (IX+MENU_OFF_MAX_VISIBLE_ENTRIES)
        ADD	A, (IX+MENU_OFF_DISPLAY_OFFSET)
        ; Example:
        ;	- Max visible entries is 4
        ;       - Current display offset is 0
        ;	- Selected entry is 4
        CP	(IX+MENU_OFF_SELECTED_ENTRY)

        JR	NZ, NOSCROLL2
        ;	
        INC	(IX+MENU_OFF_DISPLAY_OFFSET)
        CALL	MENU__DRAWCONTENTS_NO_DE
        

;	endless2: jr endless2
	;
NOSCROLL2:

	JP   	MENU__UPDATESELECTION

MENU__CHOOSEPREV:
        PUSH	HL
        POP	IX
	LD	A, (IX+MENU_OFF_SELECTED_ENTRY)
        SUB	1
        RET	C
	LD	(IX+MENU_OFF_SELECTED_ENTRY), A
        ; If we are moving before offset, decrement offset

        CP	(IX+MENU_OFF_DISPLAY_OFFSET)
	JR	NC, NOSCROLL1
        DEC     (IX+MENU_OFF_DISPLAY_OFFSET)
        ; Redraw contents
        CALL	MENU__DRAWCONTENTS_NO_DE
NOSCROLL1:
	JP   	MENU__UPDATESELECTION

MENU__ACTIVATE:
	PUSH	HL
        POP	IX
        ; Load active entry
        LD	A, (IX+MENU_OFF_SELECTED_ENTRY)
        SLA	A ; Multiply by 2
        ; Load HL with base callback pointer
        LD	L, (IX+MENU_OFF_CALLBACKPTR)
        LD	H, (IX+MENU_OFF_CALLBACKPTR+1)
        ADD_HL_A

        LD	E, (HL)
        INC 	HL
        LD	D, (HL)
        PUSH	DE
        RET			; Jump to function handler. Unclear why JP (HL) does not work.
        
