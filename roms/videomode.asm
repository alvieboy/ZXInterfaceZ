       
VIDEOMODE__REDRAW:
;        LD	HL, VIDEOMODE_MENU
        LD	D, 8
        CALL	MENU__INIT
        JP 	MENU__DRAW	; TAILCALL

VIDEOMODE__KEYHANDLER:
;	LD	HL, VIDEOMODE_MENU
	
        CP	$26 	; A key
        JR	NZ, _n1
        JP	MENU__CHOOSENEXT
_n1:
        CP	$25     ; Q key
        JR	NZ, _n2
        JP	MENU__CHOOSEPREV
_n2:
        CP	$21     ; ENTER key
        JR	Z, SENDSETVIDEOMODE
_n3:
	; Check for cancel
       	LD	DE,(CURKEY) ; Don't think is needed.
        LD	A, D
        CP	$27
        RET	NZ
        LD	A, E	
        CP	$20
        JR	Z, EXITVIDEOMODE
	RET
        
SENDSETVIDEOMODE:
        LD	A, CMD_SETVIDEOMODE
        CALL	WRITECMDFIFO
        LD	IX, VIDEOMODE_MENU
        LD	A, (IX+MENU_OFF_SELECTED_ENTRY)
        CALL	WRITECMDFIFO

EXITVIDEOMODE:
	JP	WIDGET__CLOSE 	; TAILCALL
        
        
VIDEOMODE__INIT:
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
        PUSH	IX
        POP	HL ; Return class pointer in HL
        RET

VIDEOMODE__CLASSDEF:
	DEFW	VIDEOMODE__INIT
        DEFW	WIDGET__IDLE
        DEFW	VIDEOMODE__KEYHANDLER
        DEFW	VIDEOMODE__REDRAW

VIDEOMODE__SHOW:
	LD	HL, VIDEOMODE__CLASSDEF
        JP	WIDGET__DISPLAY		; TAILCALL

VIDEOMENUTITLE:	DB	"Video mode", 0
VIDEOENTRY1:	DB	"Expanded display"  ,0
VIDEOENTRY2:	DB	"Wide-screen display"  ,0
VIDEOENTRY3:	DB	"Normal display"  ,0
