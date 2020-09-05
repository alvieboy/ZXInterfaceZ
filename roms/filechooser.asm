; CLASS FileDialog

FileDialog__SIZE 	EQU	Window__SIZE + CallbackMenu__SIZE


include "filechooser_defs.asm"

FILECHOOSER__INIT:
	LD	HL, HEAP
        ; Load directory
	LD	A, RESOURCE_ID_DIRECTORY
        CALL	LOADRESOURCE
        JP	Z, INTERNALERROR
	; HL points to "free" area after listing resource. 
        ; Use it to set up menu
        PUSH	HL
        POP	IX
        
        PUSH	HL ; Save for later
        
        LD	HL, HEAP
        ; Use at most 16 entries.
        LD	A, (HL) 	; Get number of dir. entries
        LD	B, A		; Save for later
        
        LD	C, 15
        
        INC	HL

        ; 	Starts with directory. Use it for title
        LD	(IX+FRAME_OFF_TITLEPTR), L
        LD	(IX+FRAME_OFF_TITLEPTR+1), H
        CALL	MOVE_HL_PAST_NULL ; 	Move until we get the null

	XOR 	A
        LD	(IX+MENU_OFF_SELECTED_ENTRY), A

        ; Now, setup menu at IX
        LD	A, 24
        LD	(IX+FRAME_OFF_WIDTH), A
        LD	(IX+FRAME_OFF_NUMBER_OF_LINES), C 	; Max visible entries
        LD	(IX+MENU_OFF_DATA_ENTRIES), B 		; Total number of entries
        ; HL points to 1st entry.
_l4:
	INC	HL ;Move past entry flags for now
        LD	A, 0 ; Entry attribute
        LD	(IX+MENU_OFF_FIRST_ENTRY), A
	LD	(IX+MENU_OFF_FIRST_ENTRY+1), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+2), H
        INC	IX
        INC	IX
        INC	IX
        ; Now, scan for NULL termination
_l2:    LD	A, (HL)
        OR	A
        INC 	HL
        JR 	NZ, _l2
        ; We are past NULL now.

        DJNZ	_l4

	POP	HL	; Restore menu pointer

        LD	D, 5 ; line to display menu at.
        JP	MENU__INIT	; TAILCALL


FILECHOOSER__HANDLEKEY:
        CP	$26 	; A key
        JP	Z, MENU__CHOOSENEXT
        CP	$25     ; Q key
        JP	Z, MENU__CHOOSEPREV
        CP	$21     ; ENTER key
        JR	Z, FILECHOOSER__FILESELECTED
	; Check for cancel
       	LD	DE,(CURKEY) ; Don't think is needed.
        LD	A, D
        CP	$27
        RET	NZ
        LD	A, E
        CP	$20
        RET	NZ
        ; Cancel choosen.
        JP	WIDGET__CLOSE


FILECHOOSER__FILESELECTED:
	PUSH	HL
        POP	IX
        ; 	Find entry.
        LD	B, (IX+MENU_OFF_SELECTED_ENTRY)
        LD	HL, HEAP
        ; 	Move past number of entries
        INC	HL
        ;	Move past title
        CALL	MOVE_HL_PAST_NULL ; 	Move until we get the null
_scanentry:
	XOR	A
        OR	B
	JR	Z, _entryfound
        DEC	B
        INC	HL ; Move past entry flags
        CALL	MOVE_HL_PAST_NULL ; 	Move until we get the null
        JR	_scanentry
        
_entryfound:
	; See if it is a directory or a file. 01: dir, 00: file
        LD	A,(HL)
        INC	HL ; Move to entry name
        OR	A
        JR	Z, _entryisfile
        ; Entry is a directory.
        ; Send CHDIR command
        LD	A, CMD_CHDIR
        CALL	WRITECMDFIFO
        CALL	WRITECMDSTRING
        
        ; Re-init widget. Note that this might call the idle function
        ; with a wrong instance pointer.

        ; Save local callback function
        LD	E, (IX+FILECHOOSER_OFF_SELECTCB)
        LD	D, (IX+FILECHOOSER_OFF_SELECTCB+1)
        
        ;        
        CALL	WIDGET__CLOSENOREDRAW
        ;
        LD	HL, FILECHOOSER__CLASSDEF
        PUSH	DE
        CALL	WIDGET__DISPLAY
        
        CALL	WIDGET__GETCURRENTINSTANCE

        POP	DE
        PUSH	HL
        POP	IX

        LD	(IX+FILECHOOSER_OFF_SELECTCB),E
        LD	(IX+FILECHOOSER_OFF_SELECTCB+1),D
        RET
        
_entryisfile:
	; Original filename in HL
        LD	E, (IX+FILECHOOSER_OFF_SELECTCB)
        LD	D, (IX+FILECHOOSER_OFF_SELECTCB+1)
        PUSH	DE
        RET		; JP (DE)

FILECHOOSER__SETSELECTCALLBACK:
	; in DE
        PUSH	HL
        POP	IX
        
        LD	(IX+FILECHOOSER_OFF_SELECTCB), E
        LD	(IX+FILECHOOSER_OFF_SELECTCB+1), D

        RET	
        

FILECHOOSER__SETFILTER:
        PUSH	AF
	LD	A, CMD_SET_FILE_FILTER
        CALL	WRITECMDFIFO
        POP	AF
        JP	WRITECMDFIFO	; TAILCALL



FILECHOOSER__CLASSDEF:
	DEFW	FILECHOOSER__INIT	; Init
        DEFW	WIDGET__IDLE	; Idle
        DEFW	FILECHOOSER__HANDLEKEY    ; Keyboard handler
        DEFW	MENU__DRAW	; Draw
        DEFW	MENU__GETBOUNDS
