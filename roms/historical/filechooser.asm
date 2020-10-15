


FileDialog__directoryList_OFFSET	EQU Dialog__SIZE 	; Pointer
FileDialog__menuEntries_OFFSET		EQU Dialog__SIZE+2 	; Pointer
FileDialog__menuinstance_OFFSET		EQU Dialog__SIZE+4      ; IndexedMenu object
FileDialog__SIZE 			EQU (Dialog__SIZE + IndexedMenu__SIZE + 4)

FileDialog__getSelectionString:
        
        LD	B, 0
        LD	C, (IX+FileDialog__menuinstance_OFFSET + Menu__selectedEntry_OFFSET)
        
        ;	Get entry from the directory list.
        LD	L, (IX+FileDialog__menuEntries_OFFSET)
        LD	H, (IX+FileDialog__menuEntries_OFFSET+1)
        ADD	HL, BC
        ADD	HL, BC
        ADD	HL, BC   
        
        DEBUGSTR "Entry HL "
        DEBUGHEXHL
        
        INC	HL
        LD	A, (HL)
        INC	HL
        LD	H, (HL)
        LD	L, A
        DEBUGSTR "STR HL "
        DEBUGHEXHL
        RET

FileDialog__allocateDirectory:

	CALL	MALLOC
        ; Store pointer
        LD (IX+FileDialog__directoryList_OFFSET), L
        LD (IX+FileDialog__directoryList_OFFSET+1), H
	DEBUGSTR "Allocated bytes at "
        DEBUGHEXHL
        RET

FileDialog__CTOR:
	; Init Window
        DEBUGSTR "Enter FileChooser__CTOR "
        DEBUGHEXSP
        
        CALL	Window__CTOR

	PUSH 	IX
        ; Save in DE, needed for setFunctionHandler
        POP	DE
        PUSH	DE
        
         ; Initialise menu
         LD	BC, FileDialog__menuinstance_OFFSET
         ADD	IX, BC
         PUSH	IX
          CALL	IndexedMenu__CTOR
          LD	HL, FileDialog__entrySelectedWrapper
          ; original IX in DE
          CALL	IndexedMenu__setFunctionHandler


         POP	HL ; HL contains now the Menu pointer
        POP	IX
        CALL	Bin__setChild	; TAILCALL

        ;POP	IX	; Get back our own pointer

FileDialog__loadDirectory_LABEL:
        ; Load directory
	LD	A, RESOURCE_ID_DIRECTORY
        LD	HL, FileDialog__allocateDirectory
        
        CALL	LOADRESOURCE_ALLOCFUN
        
        JP	Z, INTERNALERROR
	
        LD	L, (IX+FileDialog__directoryList_OFFSET)
        LD	H, (IX+FileDialog__directoryList_OFFSET+1)
        DEBUGSTR "Number of entries "
        LD	A, (HL) 	; Get number of dir. entries

	DEBUGHEXA
        
        LD	B, A
        PUSH	BC		; Sabe entries in stack
        
        
        INC	HL		; HL now points to directory name
        ; Set window title
        PUSH	HL
        CALL 	Window__setTitle
        POP	HL
        
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

	DEBUGSTR "Entry "
        DEBUGHEXHL
        
        ; Now, scan for NULL termination
_l2:    LD	A, (HL)
        OR	A
        INC 	HL
        JR 	NZ, _l2
        ; We are past NULL now.

        DJNZ	_l4

	POP	BC
        DEBUGSTR	"Adding entries\n"
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

        DEBUGSTR	"FileChooser__CTOR done "
        DEBUGHEXSP
        
        RET
        
        
FileDialog__destroy:
	; Fallback 
FileDialog__freeAreas:
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
	CALL	IndexedMenu__setFunctionHandler
        POP	IX
        RET

FileDialog__entrySelectedWrapper:
	DEBUGSTR "entrySelectedWrapper "
        DEBUGHEXA
	PUSH	HL
        POP	IX
        ; If $FF, then we "canceled"
        CP	$FF
        JR	Z, FileDialog__cancel
        JR	FileDialog__fileSelected

; User canceled load.
FileDialog__cancel:
	LD	A, $FF
	LD	(IX+Dialog__result_OFFSET), A
        LD	A, 0
        VCALL	Widget__setVisible
        RET
        

FileDialog__fileSelected:
	; IX is dialog!
        
        ; 	Find entry.
        ;LD	A, (IX+Menu__selectedEntry_OFFSET)
        DEBUGSTR "Entry "
        DEBUGHEXA
        
        LD	B, 0
        LD	C, A
        
        ;	Get entry from the directory list.
        LD	L, (IX+FileDialog__menuEntries_OFFSET)
        LD	H, (IX+FileDialog__menuEntries_OFFSET+1)
        ADD	HL, BC
        ADD	HL, BC
        ADD	HL, BC
        DEBUGSTR "Check entry "
        DEBUGHEXHL
        
	; 	Load this entry flags
        LD	A, (HL)
        DEBUGHEXA
	SRL	A	; Get flags (shift right)
        
_entryfound:
	; See if it is a directory or a file. 01: dir, 00: file
        INC	HL ; Move to entry name
        JR	Z, _entryisfile
        ; Entry is a directory.

	; Free up space for the menu entries
        PUSH	HL
        CALL	FileDialog__freeAreas
	POP	HL
        
        ; HL is a pointer.

	LD	A,(HL)
        INC	HL
        LD	H,(HL)
        LD	L, A
        
	DEBUGSTR "Chdir to "
        DEBUGHEXHL
        DEBUGSTRHL
        
        LD	A, CMD_CHDIR
        CALL	WRITECMDFIFO
        CALL	WRITECMDSTRING

        ; Re-init listing widget. Note that this might call the idle function
        ; with a wrong instance pointer.

        
	CALL	FileDialog__loadDirectory_LABEL
        ; Force redraw
        VCALL	Widget__draw
        RET
        
_entryisfile:
        ; Hide
        LD	A, 0
	LD	(IX+Dialog__result_OFFSET), A
        ; A used here as 0 too
        VCALL	Widget__setVisible
        DEBUGSTR "Dialog hidden\n"
        RET		; JP (DE)


FileDialog__setFilter:
        PUSH	AF
	LD	A, CMD_SET_FILE_FILTER
        CALL	WRITECMDFIFO
        POP	AF
        JP	WRITECMDFIFO	; TAILCALL

FileDialog__DTOR:
	DEBUGSTR "Filedialog__DTOR "
        DEBUGHEXIX
        
	CALL	FileDialog__destroy
        JP	Window__DTOR

