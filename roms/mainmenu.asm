MAINMENU__HANDLEKEY:
	LD	HL, MENU1
	
        CP	$26 	; A key
        JR	NZ, N1
        JP	MENU__CHOOSENEXT
N1:
        CP	$25     ; Q key
        JR	NZ, N2
        JP	MENU__CHOOSEPREV
N2:
        CP	$21     ; ENTER key
        JR	NZ, N3
        JP	MENU__ACTIVATE
N3:
	RET
        
MAINMENU__STATUSCHANGED:
	BIT	1, A                 	; Bit 1 is WIFI Connected flag
        JR	NZ, WIFISTATUSCHANGED
	BIT	3, A                    ; Bit 3 is SD Card connected flag.
        JR	NZ, SDCARDSTATUSCHANGED
	RET


WIFISTATUSCHANGED:
	RET

SDCARDSTATUSCHANGED:
	BIT 	3, (IX) ; See if SDCARD is connected.
        JR	NZ, SDCARDCONNECTED
	
        ; SD card disconnected. Update menu to disable entry

        LD	HL, MENU1
	PUSH	HL
        POP	IX
        SET	0, (IX+MENU_OFF_FIRST_ENTRY+3) ; 3 == second entry (each has 3 bytes)
        JP	MENU__UPDATESELECTION

SDCARDCONNECTED:
	; Activate menu entry for SDcard
        LD	HL, MENU1
	PUSH	HL
        POP	IX
        RES	0, (IX+MENU_OFF_FIRST_ENTRY+3) ; 3 == second entry (each has 3 bytes)
        JP	MENU__UPDATESELECTION

ENTRY1HANDLER:
        LD	A, STATE_WIFICONFIG
        JP	ENTERSTATE

ENTRY2HANDLER:
	LD	A, STATE_SDCARDMENU
        JP	ENTERSTATE

ENTRY3HANDLER:
	RET

ENTRY4HANDLER:
	JP 0

ENTRY5HANDLER:
	RET
        
MAINMENU__SETUP:
       	LD	IX, MENU1
        LD	A, 28  			; Menu width 24
        LD	(IX + MENU_OFF_WIDTH), A
        LD	A, 4                    ; Menu visible entries
        LD	(IX + MENU_OFF_MAX_VISIBLE_ENTRIES), A
        LD	A, 5                    ; Menu actual entries
        LD	(IX + MENU_OFF_DATA_ENTRIES), A
        XOR	A
        LD 	(IX+ MENU_OFF_SELECTED_ENTRY), A		; Selected entry
        LD	HL, MENUTITLE
        LD	(IX+MENU_OFF_MENU_TITLE), L
        LD	(IX+MENU_OFF_MENU_TITLE+1), H
        ; Entry 1
        LD	HL, ENTRY1
        LD	(IX+MENU_OFF_FIRST_ENTRY),A ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+1), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+2), H

        LD	HL, ENTRY2
        LD	A,1
        LD	(IX+MENU_OFF_FIRST_ENTRY+3),A ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+4), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+5), H
        
        XOR	A
        LD	HL, ENTRY3
        LD	(IX+MENU_OFF_FIRST_ENTRY+6),A ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+7), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+8), H
        
        LD	HL, ENTRY4
        LD	(IX+MENU_OFF_FIRST_ENTRY+9),A ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+10), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+11), H

        LD	HL, ENTRY5
        LD	(IX+MENU_OFF_FIRST_ENTRY+12),A ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+13), L
        LD	(IX+MENU_OFF_FIRST_ENTRY+14), H

	LD	(IX+MENU_OFF_CALLBACKPTR), low(MENUCALLBACKTABLE)
        LD	(IX+MENU_OFF_CALLBACKPTR+1), high(MENUCALLBACKTABLE)

	LD	A, 0 ; TEST ONLY
        LD 	(IX+MENU_OFF_DISPLAY_OFFSET), A
        LD 	(IX+MENU_OFF_SELECTED_ENTRY), A
        RET

MENUCALLBACKTABLE:
	DEFW ENTRY1HANDLER
        DEFW ENTRY2HANDLER
        DEFW ENTRY3HANDLER
        DEFW ENTRY4HANDLER
        DEFW ENTRY5HANDLER
