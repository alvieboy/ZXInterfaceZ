
Menu__drawFunc_OFFSET		EQU	Widget__SIZE+0   ; 2
Menu__displayOffset_OFFSET 	EQU	Widget__SIZE+2   ; 1
Menu__selectedEntry_OFFSET 	EQU	Widget__SIZE+3   ; 1
Menu__dataEntries_OFFSET 	EQU	Widget__SIZE+4   ; 1
Menu__data_OFFSET 		EQU	Widget__SIZE+5   ; 2
Menu__SIZE			EQU	Widget__SIZE+7

; VTABLE

Menu__activateEntry		EQU	Widget__NUM_VTABLE_ENTRIES	; Pure virtual
Menu__NUM_VTABLE_ENTRIES	EQU	Widget__NUM_VTABLE_ENTRIES+1

Menu__handleEvent:
	; Check if keyboard event
        CP	0
        RET	NZ	; No, not keyboard event
        DEBUGSTR "Menu KBD event\n"
        LD	A, E
        CP	$26 	; A key
        JP	Z, Menu__chooseNext
        CP	$25     ; Q key
        JP	Z, Menu__choosePrev
        CP	$21     ; ENTER key
        JR	NZ, _n3
        DEBUGSTR "Activate entry\n"
        LD	A, (IX+Menu__selectedEntry_OFFSET)
        VCALL	Menu__activateEntry
_n3:    
	LD	DE,(CURKEY) ; Don't think is needed.
        LD	A, D
        CP	$27
        RET	NZ
        LD	A, E	
        CP	$20
        RET	NZ
        ; Cancel pressed.
        LD	A, $FF
        VCALL	Menu__activateEntry
        
	RET

Menu_V__activateEntry:
	RET

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

Menu_P__CTOR:
	CALL	Widget__CTOR

	LD	(IX+Menu__drawFunc_OFFSET), LOW(MENU__DRAWITEMDEFAULT)
        LD	(IX+Menu__drawFunc_OFFSET+1), HIGH(MENU__DRAWITEMDEFAULT)
        LD	(IX+Menu__displayOffset_OFFSET), 0
        LD	(IX+Menu__selectedEntry_OFFSET), 0
        DEBUGSTR "Menu created "
        DEBUGHEXIX
        
        RET

;
; A:  Number of entries
; HL: Pointer to entry table

Menu__setEntries:
	LD	(IX+Menu__dataEntries_OFFSET), A
        LD	(IX+Menu__data_OFFSET), L
        LD	(IX+Menu__data_OFFSET+1), H
        RET

Menu__clear:
	;JP  	FRAME__CLEAR

Menu__drawImpl:
	DEBUGSTR	"Redrawing menu\n"
        CALL 	MENU__UPDATESELECTION
        JP	MENU__DRAWCONTENTS_NO_DE

FILLSLINE:     
	PUSH	BC
	PUSH	DE
        LD	B, (IX+Widget__width_OFFSET)  	; Load width of menu
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
        
        ; Get pointer to Nth entry, where Nth=display Offset.
        
	LD	L, (IX+Menu__data_OFFSET)
        LD	H, (IX+Menu__data_OFFSET+1)
        LD	C, (IX+Menu__displayOffset_OFFSET)
        LD	B, 0
        ADD	HL, BC
        ADD	HL, BC
        ADD	HL, BC
        
        LD	E, (IX+Widget__attrptr_OFFSET)
        LD	D, (IX+Widget__attrptr_OFFSET+1)
        
        LD	B, (IX+Widget__height_OFFSET) 	; Number of entries

	DEBUGSTR " > Menu lines: "
        DEBUGHEXB


        LD	C, (IX+Menu__displayOffset_OFFSET)         ; Compare against this number

L6:
        LD	A, E            ; Move to next attribute line
        ADD	A, $20
        LD	E, A
        JR	NC, L7
        INC	D
L7:                           	
	LD	A, (IX+Menu__selectedEntry_OFFSET)	; Get active entry
        CP	C		; Compare
        JR	NZ,  L5
        
        ; load item flags.
        BIT 	0, (HL);IY+Menu__firstEntry_OFFSET); TODO: Do NOT use IY!
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
        DEBUGSTR "Update sel done\n"
        RET


; Draw menu contents
; Inputs: 	IX : 	pointer to menu structure
;   		DE : 	Ptr to screen header location

MENU__DRAWCONTENTS_NO_DE:
	LD	E, (IX+Widget__screenptr_OFFSET)
        LD	D, (IX+Widget__screenptr_OFFSET+1)
        ;INC	DE
;MENU__DRAWCONTENTS:   	; Call this instead if you already have DE pointing to the correct place.


	DEBUGSTR "Drawing menu contents ";
        DEBUGHEXIX
        
	LD	A, (IX+Menu__dataEntries_OFFSET) ; Get number of entries
        LD	B, (IX+Widget__height_OFFSET) ; Get number of lines
	CP	B
        JR	NC, _s
        LD	B, A
_s:     
	DEBUGSTR " > will draw lines="
        DEBUGHEXB
        
        LD	L, (IX+Menu__data_OFFSET)
        LD	H, (IX+Menu__data_OFFSET+1)
        
	; HL now contains the first entry.
        ; Move past offset if we have one. This is used for scrolling.
        PUSH	BC
        LD	A, (IX+Menu__displayOffset_OFFSET)
        LD	C, A
        LD	B, 0
        ADD	HL, BC
        ADD	HL, BC
        ADD	HL, BC
        POP	BC
        
	DEBUGSTR " > first ptr "
        DEBUGHEXHL
	DEBUGSTR " > DE "
        DEBUGHEXDE

MD2:
        PUSH	BC
        CALL	MOVEDOWN
	PUSH	DE
        ; Load entry attributes - TBD
        LD	A, (HL)
        DEBUGSTR " > Attr "
        DEBUGHEXA
        
        INC 	HL
        ; We only have BC available here. Use it to load
        ; function pointer
        LD	BC, RETFROMDRAW
        PUSH	BC ; Will be return address.
        
        LD	C, (IX+Menu__drawFunc_OFFSET)
        LD	B, (IX+Menu__drawFunc_OFFSET+1)
        PUSH	BC ; Function to call
        
        
        LD	C, (HL) ; Load entry..
        INC	HL
        LD	B, (HL) ; pointer
        INC	HL
	DEBUGSTR "Entry @"
        DEBUGHEXBC

        RET	; THIS IS A CALL!
        
RETFROMDRAW:       
        DEBUGSTR "Entry redrawn\n"
        POP	DE
        POP	BC
        DEBUGSTR "Loop to go "
        DEBUGHEXB
        
        DJNZ	MD2
        
        ; We may need empty entries if we are short.
	LD	B, (IX+Menu__dataEntries_OFFSET) ; Get number of entries
        LD	A, (IX+Widget__height_OFFSET) ; Get number of lines
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
        LD	A, (IX+Widget__width_OFFSET)
        CALL 	PRINTSTRINGPAD
        POP	DE
        POP	BC
        DJNZ	_fillempty


        
_noempty:



        RET; TEMPORARY!!!!!

;	Check if we need up/down arrows.

	LD	A, (IX+Widget__screenptr_OFFSET)
        LD	D, (IX+Widget__screenptr_OFFSET+1)
        INC	A
        ADD	A, (IX+Widget__width_OFFSET)
        LD	E,A
        
        CALL	MOVEDOWN

        ; Up arrow needed if offset is not zero
        XOR	A
        CP	(IX+Menu__displayOffset_OFFSET)
        JR	Z, nouparrow
        LD	HL, UPARROW
        JR	f1
nouparrow:  
	LD	HL, RIGHTVERTICAL ; Empty (space) with vertical bar on right
f1:
        CALL	DRAWCHAR
        DEC	DE


        ; Draw down arrow
        LD	B, (IX+Widget__height_OFFSET)
        DEC	B
POSITION_DOWN_ARROW:
        CALL	MOVEDOWN
        DJNZ	POSITION_DOWN_ARROW

	LD	A, (IX+Menu__displayOffset_OFFSET)
        DEBUGSTR "displayOffset "
        DEBUGHEXA
        
        ADD 	A, (IX+Widget__height_OFFSET)
        DEBUGSTR " + height "
        DEBUGHEXA
                              
        DEBUGSTR "data entries "
        DEBUG8 (IX+Menu__dataEntries_OFFSET)

        CP	(IX+Menu__dataEntries_OFFSET)
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
        LD	A, (IX+Widget__width_OFFSET)
        CALL 	PRINTSTRINGPAD
        POP	HL
        RET


Menu__chooseNext:
	LD	A, (IX+Menu__selectedEntry_OFFSET)
        INC	A
        CP	(IX+Menu__dataEntries_OFFSET)
        RET	Z
	LD	(IX+Menu__selectedEntry_OFFSET), A
        ; Check if we need to scroll

        LD	A, (IX+Widget__height_OFFSET)
        ADD	A, (IX+Menu__displayOffset_OFFSET)
        ; Example:
        ;	- Max visible entries is 4
        ;       - Current display offset is 0
        ;	- Selected entry is 4
        CP	(IX+Menu__selectedEntry_OFFSET)

        JR	NZ, NOSCROLL2
        ;	
        INC	(IX+Menu__displayOffset_OFFSET)
        CALL	MENU__DRAWCONTENTS_NO_DE
NOSCROLL2:
	JP   	MENU__UPDATESELECTION

Menu__choosePrev:
	LD	A, (IX+Menu__selectedEntry_OFFSET)
        SUB	1
        RET	C
	LD	(IX+Menu__selectedEntry_OFFSET), A
        ; If we are moving before offset, decrement offset

        CP	(IX+Menu__displayOffset_OFFSET)
	JR	NC, NOSCROLL1
        DEC     (IX+Menu__displayOffset_OFFSET)
        ; Redraw contents
        CALL	MENU__DRAWCONTENTS_NO_DE
NOSCROLL1:
	JP   	MENU__UPDATESELECTION

	; A: menu index to check.
        ; IX: 
        ; Clobbers: C flags
MENU__ISDISABLED:
        LD	C, A
        LD	B, 0
        PUSH	HL
        LD	L, (IX+Menu__data_OFFSET)
        LD	H, (IX+Menu__data_OFFSET+1)
        ADD	HL, BC
        ADD	HL, BC
        ADD	HL, BC
        BIT	0, (HL)
	POP	HL
        RET

;MENU__ACTIVATE:
;	PUSH	HL
;        POP	IX
;        LD	A, (IX+Menu__selectedEntry_OFFSET)
;        CALL	MENU__ISDISABLED
;        RET	NZ
;        ; Load active entry
;        LD	A, (IX+Menu__selectedEntry_OFFSET)
;        
;        ADD	A, A ; Multiply by 2
;        ; Load HL with base callback pointer
;        LD	L, (IX+MENU_OFF_CALLBACKPTR)
;        LD	H, (IX+MENU_OFF_CALLBACKPTR+1)
;        ADD_HL_A
;        LD	E, (HL)
;        INC 	HL
;        LD	D, (HL)
;        PUSH	DE
;        RET			; Jump to function handler. 
