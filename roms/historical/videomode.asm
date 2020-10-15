Videomode__show:
        LD	IX, VideomodeWindow_INST
        LD	DE, $1806 ; width=28, height=7
        LD	HL, Screen_INST
        CALL	MenuwindowIndexed__CTOR
        
        LD	HL, VIDEOMODE_TITLE
        CALL	Window__setTitle
        
        ; Set menu entries
        LD	A,  4
        LD	HL, VIDEOMODE_ENTRIES
        CALL	MenuwindowIndexed__setEntries
        

        LD	IX, VideomodeWindow_INST
        LD	HL, SENDSETVIDEOMODE
        PUSH	IX
        POP	DE
        DEBUGSTR "Video "
        DEBUGHEXIX
        CALL	MenuwindowIndexed__setFunctionHandler

        SWAP_IX_HL
        LD	IX, Screen_INST
        CALL 	Screen__addWindowCentered
        SWAP_IX_HL
        
        LD	A, 1
        VCALL	Widget__setVisible

        RET

SENDSETVIDEOMODE:
        ; Unless BACK was selected.
        ;LD	IX, VIDEOMODE_MENU
        ;LD	A, (IX+MENU_OFF_SELECTED_ENTRY)
        CP	3
        JR	Z, _exitvideo
        ; Valid video mode.
        PUSH	AF
        LD	A, CMD_SETVIDEOMODE
        CALL	WRITECMDFIFO
        POP	AF
        CALL	WRITECMDFIFO
_exitvideo:
	PUSH	HL
        POP	IX
	JP	Widget__close


VIDEOMODE_TITLE:	DB	"Video mode", 0

VIDEOENTRY1:	DB	"Expanded display"  ,0
VIDEOENTRY2:	DB	"Wide-screen display"  ,0
VIDEOENTRY3:	DB	"Normal display"  ,0
VIDEOENTRY4:	DB	"Back"  ,0

VIDEOMODE_ENTRIES:
	DB 	0
        DEFW	VIDEOENTRY1
	DB 	0
        DEFW	VIDEOENTRY2
	DB 	0
        DEFW	VIDEOENTRY3
	DB 	0
        DEFW	VIDEOENTRY4
