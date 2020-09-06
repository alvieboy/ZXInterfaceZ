


FileDialog__directoryList_OFFSET	EQU Window__SIZE 	; Pointer
FileDialog__menuEntries_OFFSET		EQU Window__SIZE+2 	; Pointer
FileDialog__menuinstance_OFFSET		EQU Window__SIZE+4      ; StringMenu object
FileDialog__SIZE 			EQU (Window__SIZE + StringMenu__SIZE + 4)


FileDialog__allocateDirectory:
	CALL	MALLOC
        ; Store pointer
        LD (IX+FileDialog__directoryList_OFFSET), L
        LD (IX+FileDialog__directoryList_OFFSET+1), H

        RET

FileDialog__CTOR:
	; Init Window
        
        CALL	Window__CTOR

	PUSH 	IX
        ; Initialise menu
        LD	BC, FileDialog__menuinstance_OFFSET
        ADD	IX, BC
        PUSH	IX
        CALL	StringMenu__CTOR
        POP	HL ; HL contains now the Menu pointer
        POP	IX
        CALL	Bin__setChild	; TAILCALL

        POP	IX	; Get back our own pointer

        ; Load directory
	LD	A, RESOURCE_ID_DIRECTORY
        LD	HL, FileDialog__allocateDirectory
        
        CALL	LOADRESOURCE_ALLOCFUN
        
        JP	Z, INTERNALERROR
	
        LD	L, (IX+FileDialog__directoryList_OFFSET)
        LD	H, (IX+FileDialog__directoryList_OFFSET+1)
        LD	A, (HL) 	; Get number of dir. entries
        LD	B, A
        PUSH	BC		; Sabe entries in stack
        
        
        INC	HL		; HL now points to directory name
        ; Set window title
        CALL 	Window__setTitle
        
        CALL	MOVE_HL_PAST_NULL ; 	Move until we get the null

	POP	BC
        PUSH	BC
        
        ; Each entry is 3 bytes. 
        PUSH	HL	; Save entries pointer
        LD	E, B
        LD	D, 0
        LD	HL, 0
        ADD	HL, DE
        ADD	HL, DE
        ADD	HL, DE ; * 3
        EX	DE, HL
        ; DE now contains the memory we need for entries.
        ; Allocate memory
        CALL	MALLOC
        ; Store the pointer 
        LD	(IX+FileDialog__menuEntries_OFFSET), L
        LD	(IX+FileDialog__menuEntries_OFFSET+1), H
        ; Save it in DE
        EX	DE, HL
        POP	HL	; Get back the directory entries
        
        POP	BC	; Restore number of entries
        PUSH	BC	; Keep it in stack, though
        
        ; HL points to 1st entry.
_l4:
	LD	A, (HL)
        INC	HL 
        ; Setup attribute in bits 7-1. Bit 0 reserved for "active"
        SLA	A
        
        LD	(DE), A
	INC 	DE
        LD	A, L
        LD	(DE), A
        INC	DE
        LD	A, H
	LD 	(DE), A
        INC 	DE
        
        ; Now, scan for NULL termination
_l2:    LD	A, (HL)
        OR	A
        INC 	HL
        JR 	NZ, _l2
        ; We are past NULL now.

        DJNZ	_l4

	POP	BC
        
	; We now correcly set up the menu entries
        ; Set up menu accordingly
        LD	A, B	; Number of entries
        LD	L, (IX+FileDialog__menuEntries_OFFSET)
        LD	H, (IX+FileDialog__menuEntries_OFFSET+1)
        
        PUSH	IX
        LD	BC, FileDialog__menuinstance_OFFSET
        ADD	IX, BC
	CALL	Menu__setEntries
        POP	IX

        RET
        
        
FileDialog__destroy:
	; Delete the computed memory area
        LD	L, (IX+FileDialog__menuEntries_OFFSET)
        LD	H, (IX+FileDialog__menuEntries_OFFSET+1)
        CALL	FREE
        ; Delete the allocated memory for directory
        LD	L, (IX+FileDialog__directoryList_OFFSET)
        LD	H, (IX+FileDialog__directoryList_OFFSET+1)
        CALL	FREE
	RET
        
FileDialog__setFunctionHandler:
        PUSH	IX
        LD	BC, FileDialog__menuinstance_OFFSET
        ADD	IX, BC
	CALL	StringMenu__setFunctionHandler
        POP	IX
        RET

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
