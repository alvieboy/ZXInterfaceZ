Settings__show:
        LD	IX, SettingsWindow_INST
        LD	DE, $1807 ; width=28, height=7
        LD	HL, Screen_INST
        CALL	Menuwindow__CTOR
        
        LD	HL, SETTINGS_TITLE
        CALL	Window__setTitle
        
        ; Set menu entries
        LD	A,  5
        LD	HL, SETTINGS_ENTRIES
        CALL	Menuwindow__setEntries
        
        LD	HL, SETTINGS_CALLBACK_TABLE
        CALL	Menuwindow__setCallbackTable

        SWAP_IX_HL
        LD	IX, Screen_INST
        CALL 	Screen__addWindowCentered
        SWAP_IX_HL
        
        VCALL	Widget__draw

        RET

SETTINGS_CALLBACK_TABLE:
	DEFW Widget__destroyParent  ; Wifi
        DEFW Widget__destroyParent  ; Bluetooth
        DEFW Widget__destroyParent  ; USB
        DEFW Widget__destroyParent  ; Video mode
        DEFW Widget__destroyParent 

SETTINGS_TITLE:
	DB 	"Settings", 0

SETTINGSENTRY1: DB	"Wifi...", 0
SETTINGSENTRY2: DB	"Bluetooth...", 0
SETTINGSENTRY3: DB	"USB...", 0
SETTINGSENTRY4: DB	"Video...", 0
SETTINGSENTRY5: DB	"Back", 0

SETTINGS_ENTRIES:
	DB 	0
        DEFW	SETTINGSENTRY1
	DB 	0
        DEFW	SETTINGSENTRY2
	DB 	0
        DEFW	SETTINGSENTRY3
        DB 	0
        DEFW	SETTINGSENTRY4
        DB 	0
        DEFW	SETTINGSENTRY5



        



