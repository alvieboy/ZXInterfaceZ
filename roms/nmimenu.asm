NMIMENU__HANDLEKEY:
	LD	HL, NMI_MENU
	
        CP	$26 	; A key
        JR	NZ, _n1
        JP	MENU__CHOOSENEXT
_n1:
        CP	$25     ; Q key
        JR	NZ, _n2
        JP	MENU__CHOOSEPREV
_n2:
        CP	$21     ; ENTER key
        JR	NZ, _n3
        JP	MENU__ACTIVATE
_n3:
	RET
        

NMIENTRY3HANDLER:

NMIENTRY4HANDLER:

NMIENTRY5HANDLER:
        RET
        
NMIENTRY6HANDLER:
	DI
        LD	DE, 0
        PUSH	DE
        RETN

NMIENTRY7HANDLER:
        LD	A,1
        LD	(NMIEXIT), A
        RET

        
NMIMENU__SETUP:
       	LD	IX, NMI_MENU
        LD	(IX + FRAME_OFF_WIDTH), 28 ; Menu width 24
        LD	(IX + FRAME_OFF_NUMBER_OF_LINES), 7 ; Menu visible entries
        LD	(IX + MENU_OFF_DATA_ENTRIES), 7 ; Menu actual entries 
        LD 	(IX + MENU_OFF_SELECTED_ENTRY), 0 ; Selected entry
        LD	(IX+FRAME_OFF_TITLEPTR), LOW(NMIMENUTITLE)
        LD	(IX+FRAME_OFF_TITLEPTR+1), HIGH(NMIMENUTITLE)
        ; Entry 1
        LD	(IX+MENU_OFF_FIRST_ENTRY), 0 ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+1), LOW(NMIENTRY1)
        LD	(IX+MENU_OFF_FIRST_ENTRY+2), HIGH(NMIENTRY1);

        LD	(IX+MENU_OFF_FIRST_ENTRY+3), 0 ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+4), LOW(NMIENTRY2)
        LD	(IX+MENU_OFF_FIRST_ENTRY+5), HIGH(NMIENTRY2);
        
        LD	(IX+MENU_OFF_FIRST_ENTRY+6), 0 ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+7), LOW(NMIENTRY3)
        LD	(IX+MENU_OFF_FIRST_ENTRY+8), HIGH(NMIENTRY3)
        
        LD	(IX+MENU_OFF_FIRST_ENTRY+9), 0 ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+10), LOW(NMIENTRY4)
        LD	(IX+MENU_OFF_FIRST_ENTRY+11), HIGH(NMIENTRY4)

        LD	(IX+MENU_OFF_FIRST_ENTRY+12), 0 ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+13), LOW(NMIENTRY5)
        LD	(IX+MENU_OFF_FIRST_ENTRY+14), HIGH(NMIENTRY5)

        LD	(IX+MENU_OFF_FIRST_ENTRY+15), 0 ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+16), LOW(NMIENTRY6)
        LD	(IX+MENU_OFF_FIRST_ENTRY+17), HIGH(NMIENTRY6)

        LD	(IX+MENU_OFF_FIRST_ENTRY+18), 0 ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+19), LOW(NMIENTRY7)
        LD	(IX+MENU_OFF_FIRST_ENTRY+20), HIGH(NMIENTRY7)

	LD	(IX+MENU_OFF_CALLBACKPTR), LOW(NMIMENUCALLBACKTABLE)
        LD	(IX+MENU_OFF_CALLBACKPTR+1), HIGH(NMIMENUCALLBACKTABLE)

        LD 	(IX+MENU_OFF_DISPLAY_OFFSET), 0
        LD 	(IX+MENU_OFF_SELECTED_ENTRY), 0
        RET

MOVE_HL_PAST_NULL:
	LD	A,(HL)
        OR	A
        INC	HL
        JR	NZ, MOVE_HL_PAST_NULL
        RET

SETUP_FILEMENU:
	LD	HL, HEAP
        ; Load directory
	LD	A, RESOURCE_ID_DIRECTORY
        CALL	LOADRESOURCE
        JP	Z, INTERNALERROR
	; HL points to "free" area after listing resource. 
        ; Use it to set up menu
        PUSH	HL
        POP	IX
        LD	(SDMENU), IX
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
        ;LD	(IX+FRAME_OFF_TITLEPTR), LOW(SDMENUTITLE)
        ;LD	(IX+FRAME_OFF_TITLEPTR+1), HIGH(SDMENUTITLE)
        ; Now, copy entries. Number is still in B
	; TODO: check for zero entries
        ;DEC	B
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
        
	RET


LOADSNAPSHOT:
	; Set up filtering
        LD	A, CMD_SET_FILE_FILTER
        CALL	WRITECMDFIFO
        LD	A, FILE_FILTER_SNAPSHOTS
        CALL	WRITECMDFIFO

        CALL	LOADFILE
        CP	$00
        JR	Z, _validsnap
	; Cancelled or error
        CALL	RESTORESCREENAREA
        LD	HL, NMI_MENU
        JP	MENU__DRAW
_validsnap:
       	JP	REQUEST_SNAPSHOT

LOADTAPE:
	; Set up filtering
        LD	A, CMD_SET_FILE_FILTER
        CALL	WRITECMDFIFO
        LD	A, FILE_FILTER_TAPES
        CALL	WRITECMDFIFO

        CALL	LOADFILE
        CP	$00
        JR	Z, _validtape
	; Cancelled or error
        
        CALL	RESTORESCREENAREA
        LD	HL, NMI_MENU
        JP	MENU__DRAW
_validtape:
       	JP	REQUEST_TAPE
        

LOADFILE:        
	CALL 	SETUP_FILEMENU
        LD	HL, (SDMENU)
        LD	D, 5 ; line to display menu at.
        CALL	MENU__INIT
        ; Set up custom draw function. IX is still set up to point to SDMENU
        ; TBD
        
        ;LD	(IX+MENU_OFF_DRAWFUNC), LOW(MENU_DRAW_FILENAME)
        
	CALL	MENU__DRAW
_fm:
	CALL	KEYBOARD
        CALL	CHECKKEY
        JR	Z, _fm

	LD	HL, (SDMENU)
        CP	$26 	; A key
        JR	NZ, _n1
        CALL	MENU__CHOOSENEXT
        JR	_fm
_n1:
        CP	$25     ; Q key
        JR	NZ, _n2
        CALL	MENU__CHOOSEPREV
        JR	_fm
_n2:
        CP	$21     ; ENTER key
        JR	NZ, _n3
       	JR	FILESELECTED
_n3:
	; Check for cancel
       	LD	DE,(CURKEY) ; Don't think is needed.
        LD	A, D
        CP	$27
        JR	NZ, _fm
        LD	A, E	
        CP	$20
        JR	NZ, _fm
	LD	A, $FF
        RET

FILESELECTED:
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
        
        ; TODO: improve this for better screen handling
        ;CALL	RESTORESCREENAREA
        ;LD	HL, NMI_MENU
        ;CALL	MENU__DRAW
        ; END TODO
        
        JP	LOADFILE ; Do everything again!
        
_entryisfile:
	LD	A, 0
        RET


REQUEST_SNAPSHOT:
	; Request load snapshot
        LD 	A, CMD_LOAD_SNA
        CALL	WRITECMDFIFO
        ; String still in HL
        CALL	WRITECMDSTRING
        ;
        ; Wait for completion
_wait:
        LD	HL, NMICMD_RESPONSE
	LD	A, RESOURCE_ID_OPERATION_STATUS
        CALL	LOADRESOURCE
        JR	Z, _error1
        ; Get operation status
        LD	A, (NMICMD_RESPONSE)
        CP      STATUS_INPROGRESS
        JR	Z, _wait
        CP	STATUS_OK
        JP	Z, SNARAM
        
        LD	HL, NMICMD_RESPONSE
        INC	HL	
        JP	SHOWOPERRORMSG

        
       ; CALL	RESTORESCREENAREA


        ;LD	HL, NMI_MENU
        ;JP	MENU__DRAW
	;RET
_error1:
	JP	INTERNALERROR
	ENDLESS


REQUEST_TAPE:
        LD 	A, CMD_PLAY_TAPE
        CALL	WRITECMDFIFO
        ; String still in HL
        CALL	WRITECMDSTRING
	LD	A, 1
	LD	(NMIEXIT), A
	RET	
        



NMIMENUCALLBACKTABLE:
	DEFW LOADSNAPSHOT
        DEFW ASKFILENAME
        DEFW LOADTAPE
        DEFW NMIENTRY4HANDLER
        DEFW VIDEOMODE
        DEFW NMIENTRY6HANDLER
        DEFW NMIENTRY7HANDLER

NMIMENUTITLE:
	DB 	"ZX Interface Z", 0
NMIENTRY1: DB	"Load snapshot..", 0
NMIENTRY2: DB	"Save snapshot..", 0
NMIENTRY3: DB	"Play tape...", 0
NMIENTRY4: DB	"Poke...",0
NMIENTRY5: DB	"Video...", 0
NMIENTRY6: DB	"Reset", 0
NMIENTRY7: DB	"Exit", 0
