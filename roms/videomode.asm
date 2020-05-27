VIDEOMODE:
	CALL	VIDEOMODE__SETUP
        LD	HL, VIDEOMODE_MENU
        LD	D, 8
        CALL	MENU__INIT
        CALL 	MENU__DRAW
        ; HANDLE KEYS
        
_l1:    CALL	KEYBOARD
	CALL	CHECKKEY
        JR	Z, _l1

	LD	HL, VIDEOMODE_MENU
	
        CP	$26 	; A key
        JR	NZ, _n1
        CALL	MENU__CHOOSENEXT
        JR	_l1
_n1:
        CP	$25     ; Q key
        JR	NZ, _n2
        CALL	MENU__CHOOSEPREV
        JR	_l1
_n2:
        CP	$21     ; ENTER key
        JR	Z, SENDSETVIDEOMODE
_n3:
	; Check for cancel
       	LD	DE,(CURKEY) ; Don't think is needed.
        LD	A, D
        CP	$27
        JR	NZ, _n4
        LD	A, E	
        CP	$20
        JR	Z, EXITVIDEOMODE
_n4:

	JR	_l1
        
SENDSETVIDEOMODE:
        LD	A, CMD_SETVIDEOMODE
        CALL	WRITECMDFIFO
        LD	IX, VIDEOMODE_MENU
        LD	A, (IX+MENU_OFF_SELECTED_ENTRY)
        CALL	WRITECMDFIFO
EXITVIDEOMODE:
	LD	HL, NMI_MENU
        JP	MENU__DRAW

        
        
VIDEOMODE__SETUP:
       	LD	IX, VIDEOMODE_MENU
        LD	(IX + FRAME_OFF_WIDTH), 24 ; Menu width 24
        LD	(IX + FRAME_OFF_NUMBER_OF_LINES), 3 ; Menu visible entries
        LD	(IX + MENU_OFF_DATA_ENTRIES), 3 ; Menu actual entries 

        ; get current video mode
        LD	HL, HEAP
	LD	A, RESOURCE_ID_VIDEOMODE

        CALL	LOADRESOURCE

        JP	Z, INTERNALERROR
        
        LD	HL, HEAP
        LD	A, (HL)

        LD 	(IX + MENU_OFF_SELECTED_ENTRY), A ; Selected entry

        LD	(IX+FRAME_OFF_TITLEPTR), LOW(VIDEOMENUTITLE)
        LD	(IX+FRAME_OFF_TITLEPTR+1), HIGH(VIDEOMENUTITLE)

        ; Entry 1
        LD	(IX+MENU_OFF_FIRST_ENTRY), 0 ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+1), LOW(VIDEOENTRY1)
        LD	(IX+MENU_OFF_FIRST_ENTRY+2), HIGH(VIDEOENTRY1)

        LD	(IX+MENU_OFF_FIRST_ENTRY+3), 0 ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+4), LOW(VIDEOENTRY2)
        LD	(IX+MENU_OFF_FIRST_ENTRY+5), HIGH(VIDEOENTRY2);
        
        LD	(IX+MENU_OFF_FIRST_ENTRY+6), 0 ; Flags
        LD	(IX+MENU_OFF_FIRST_ENTRY+7), LOW(VIDEOENTRY3)
        LD	(IX+MENU_OFF_FIRST_ENTRY+8), HIGH(VIDEOENTRY3)
        
        LD 	(IX+MENU_OFF_DISPLAY_OFFSET), 0
        RET

VIDEOMENUTITLE:	DB	"Video mode", 0
VIDEOENTRY1:	DB	"Expanded display"  ,0
VIDEOENTRY2:	DB	"Wide-screen display"  ,0
VIDEOENTRY3:	DB	"Normal display"  ,0
