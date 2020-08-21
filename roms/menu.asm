include "menu_defs.asm"
; 
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
;   Clobbers: IX, DE, BC, A

MENU__INIT:
        PUSH	HL
	PUSH	HL
        POP	IX
        
        CALL	FRAME__INIT
        ; Add some default settings.
	XOR	A
        ;LD	(IX+MENU_OFF_SELECTED_ENTRY), A
        LD	(IX+MENU_OFF_DISPLAY_OFFSET), A

	LD	(IX+MENU_OFF_DRAWFUNC), LOW(MENU__DRAWITEMDEFAULT)
        LD	(IX+MENU_OFF_DRAWFUNC+1), HIGH(MENU__DRAWITEMDEFAULT)
	POP     HL
        RET

MENU__CLEAR:
	PUSH  	HL
        POP	IX
	JP  	FRAME__CLEAR


MENU__DRAW:
	PUSH  	HL
        POP	IX
	CALL	FRAME__DRAW
        CALL 	MENU__UPDATESELECTION
        ; Draw contents
        LD	E, (IX+FRAME_OFF_SCREENPTR)
        LD	D, (IX+FRAME_OFF_SCREENPTR+1)
        INC	DE
        JP	MENU__DRAWCONTENTS

FILLSLINE:     
	PUSH	BC
	PUSH	DE
        LD	B, (IX+FRAME_OFF_WIDTH)  	; Load width of menu
        INC	B               ; Add +2 for extra borders.
        INC	B
LP1:    LD	(DE), A
        INC 	DE
        DJNZ	LP1
        POP	DE
        POP	BC
	RET

        
MENU__UPDATESELECTION:
	PUSH	BC
        PUSH	DE
        
        PUSH	IX
        POP	HL
        
        ; This sucks. Can we make this faster ?
        ; HL = HL + (MENU_OFF_FIRST_ENTRY + (MENU_OFF_DISPLAY_OFFSET*3)
        LD	A, (IX+MENU_OFF_DISPLAY_OFFSET)
        ADD_HL_A
        LD	A, (IX+MENU_OFF_DISPLAY_OFFSET)
        ADD_HL_A
        LD	A, (IX+MENU_OFF_DISPLAY_OFFSET)
        ADD_HL_A ; 3 times.
        LD	A, MENU_OFF_FIRST_ENTRY
        ADD_HL_A
                
        LD	E, (IX+FRAME_OFF_ATTRPTR)
        LD	D, (IX+FRAME_OFF_ATTRPTR+1)
        
        LD	B, (IX+FRAME_OFF_NUMBER_OF_LINES) 	; Number of entries
        LD	C, (IX+MENU_OFF_DISPLAY_OFFSET)         ; Compare against this number

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
        BIT 	0, (HL);IY+MENU_OFF_FIRST_ENTRY); TODO: Do NOT use IY!
        JR	NZ, L9 ; Disabled
        LD	A, MENU_COLOR_SELECTED ; Cyan, for selected
	JR	L8
L9:	LD	A, MENU_COLOR_DISABLED
	JR 	L8
L5:
        LD	A, MENU_COLOR_NORMAL ; Normal color
L8:
        INC	HL
        INC	HL
        INC	HL
        INC	C		; Next entry
        CALL	FILLSLINE
        DJNZ	L6
        
        ;POP	IY
	POP	DE
        POP 	BC
        RET


; Draw menu contents
; Inputs: 	IX : 	pointer to menu structure
;   		DE : 	Ptr to screen header location

MENU__DRAWCONTENTS_NO_DE:
	LD	E, (IX+FRAME_OFF_SCREENPTR)
        LD	D, (IX+FRAME_OFF_SCREENPTR+1)
        INC	DE
MENU__DRAWCONTENTS:   	; Call this instead if you already have DE pointing to the correct place.
	LD	A, (IX+MENU_OFF_DATA_ENTRIES) ; Get number of entries
        LD	B, (IX+FRAME_OFF_NUMBER_OF_LINES) ; Get number of lines
	CP	B
        JR	NC, _s
        LD	B, A
_s:     
        PUSH	IX  	;
        POP	HL      ; Move IX (menu structure) into HL
        
        ; Move HL pointer to correct offset of first entry ()
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
        ; Load entry attributes - TBD
        LD	A, (HL)
        INC 	HL
        ; We only have BC available here. Use it to load
        ; function pointer
        LD	BC, RETFROMDRAW
        PUSH	BC ; Will be return address.
        
        LD	C, (IX+MENU_OFF_DRAWFUNC)
        LD	B, (IX+MENU_OFF_DRAWFUNC+1)
        PUSH	BC ; Function to call
        
        LD	C, (HL) ; Load entry..
        INC	HL
        LD	B, (HL) ; pointer
        INC	HL
        
        RET	; THIS IS A CALL!
        
RETFROMDRAW:       

        POP	DE
        POP	BC

        DJNZ	MD2
        
        ; We may need empty entries if we are short.
	LD	B, (IX+MENU_OFF_DATA_ENTRIES) ; Get number of entries
        LD	A, (IX+FRAME_OFF_NUMBER_OF_LINES) ; Get number of lines
	SUB	B      ; Example: 2 entries, 14 lines. result: 12

        
        JR	C, _noempty
        JR	Z, _noempty

        ;CALL	DEBUGHEXA
        LD	B, A
 	
        ; Need "B" empty lines.
_fillempty:

        PUSH	BC
        CALL	MOVEDOWN
	PUSH	DE
        LD	HL, EMPTYSTRING
        LD	A, (IX+FRAME_OFF_WIDTH)
        CALL 	PRINTSTRINGPAD
        POP	DE
        POP	BC
        DJNZ	_fillempty


        
_noempty:

;	Check if we need up/down arrows.

	LD	A, (IX+FRAME_OFF_SCREENPTR)
        LD	D, (IX+FRAME_OFF_SCREENPTR+1)
        INC	A
        ADD	A, (IX+FRAME_OFF_WIDTH)
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
        LD	B, (IX+FRAME_OFF_NUMBER_OF_LINES)
        DEC	B
POSITION_DOWN_ARROW:
        CALL	MOVEDOWN
        DJNZ	POSITION_DOWN_ARROW

	LD	A, (IX+MENU_OFF_DISPLAY_OFFSET)
        ADD 	A, (IX+FRAME_OFF_NUMBER_OF_LINES)
        CP	(IX+MENU_OFF_DATA_ENTRIES)
        JR	NC, nodownarrow
        LD	HL, DOWNARROW
	JR 	f2
nodownarrow:
	LD	HL, RIGHTVERTICAL ; Empty (space) with vertical bar on right
f2:
        CALL	DRAWCHAR


	RET

MENU__DRAWITEMDEFAULT:
        PUSH	HL
        PUSH	BC
        POP	HL
        LD	A, (IX+FRAME_OFF_WIDTH)
        CALL 	PRINTSTRINGPAD
        POP	HL
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

        LD	A, (IX+FRAME_OFF_NUMBER_OF_LINES)
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

	; A: menu index to check.
        ; IX: pointer to menu structure
        ; Clobbers: C
MENU__ISDISABLED:
        LD	C, A
        ADD	A, C ; Multiply by 2.
        ADD	A, C ; Multiply by 3.
        PUSH	IX
        ADD_IX_A  ; Add to HL offset
        BIT	0, (IX+MENU_OFF_FIRST_ENTRY)
	POP	IX
        RET

MENU__ACTIVATE:
	PUSH	HL
        POP	IX
        LD	A, (IX+MENU_OFF_SELECTED_ENTRY)
        CALL	MENU__ISDISABLED
        RET	NZ
        ; Load active entry
        LD	A, (IX+MENU_OFF_SELECTED_ENTRY)
        
        ADD	A, A ; Multiply by 2
        ; Load HL with base callback pointer
        LD	L, (IX+MENU_OFF_CALLBACKPTR)
        LD	H, (IX+MENU_OFF_CALLBACKPTR+1)
        ADD_HL_A
        LD	E, (HL)
        INC 	HL
        LD	D, (HL)
        PUSH	DE
        RET			; Jump to function handler. 
        
MENU__GETBOUNDS:
	PUSH	HL
        POP	IX
	JP	FRAME__GETBOUNDS	; TAILCALL	
