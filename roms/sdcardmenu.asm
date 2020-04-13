
SDCARDMENU__STATUSCHANGED:
	BIT	3, A                    ; Bit 3 is SD Card connected flag.
        RET	Z
	BIT 	3, (IX) ; See if SDCARD is connected.
        RET 	NZ
	; SD card disconnected. Go back to main menu
        LD	A, STATE_MAINMENU
        JP	ENTERSTATE
        
SDCARDMENU__HANDLEKEY:
	LD	HL, (SDMENU)
        
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
       	;JP	MENU__ACTIVATE
_n3:
	RET


SDCARDMENU__SETUP:
	LD	HL, HEAP
        ; Load SD card root
        LD	A, $03 ; Id: DIRECTORYRESOURCE
        CALL	LOADRESOURCE
        JR	NZ, _rok
	; Could not load resource...
        
        ; TBD
        JP	INTERNALERROR
_rok:
	; HL points to "free" area after listing resource. 
        ; Use it to set up menu
        PUSH	HL
        POP	IX
        LD	HL, HEAP
        
        ; Use at most 16 entries.
        LD	A, (HL) 	; Get number of dir. entries
        LD	B, A		; Save for later
        
        CP	15
        JR 	C, _l1
    	LD	A, 15
_l1:
        LD	C, A
        
        INC	HL
        ; Now, setup menu at IX
        LD	(SDMENU), IX
        LD	A, 24
        LD	(IX+FRAME_OFF_WIDTH), A
        LD	(IX+FRAME_OFF_NUMBER_OF_LINES), C 	; Max visible entries
        LD	(IX+MENU_OFF_DATA_ENTRIES), B 		; Total number of entries
        LD	A, LOW(SDMENUTITLE)
        LD	(IX+FRAME_OFF_TITLEPTR), A
        LD	A, HIGH(SDMENUTITLE)
        LD	(IX+FRAME_OFF_TITLEPTR+1), A
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
        ;INC HL
        ;LD 	A,(HL)
        ;call DEBUGHEXA
        ;_lend: jr _lend

        DJNZ	_l4
        
	RET

DUMMY: DB "Dummy", 0
SDMENUTITLE:
	DB "Open file from SD", 0
        