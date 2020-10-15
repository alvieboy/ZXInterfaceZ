
include "command_defs.asm"

ENTERSTATE:
	HALT
        

WIFICONFIG__STATUSCHANGED:

	BIT	2, (IX+1)                 	; Bit 2 is WIFI Scanning
        JP	NZ, WIFISCANNINGSTATUSCHANGED

	RET
        
WIFISCANNINGSTATUSCHANGED:
	BIT	2,(IX)	; Scan ended ?
        JR	Z, SCANFINSIHED
        RET
        
SCANFINSIHED:
	LD	A, $38
	CALL	TEXTMESSAGE__CLEAR
	; Load access points.
        LD	A, $05
        LD	HL, HEAP
        CALL	LOADRESOURCE
        JP	Z, INTERNALERROR
        

        LD	A, 1 
        LD	(WIFIFLAGS), A ; 2 - menu entry
        ; Clear password
        LD	DE, WIFIPASSWD
        XOR	A
        LD	(DE), A
        
	; HL points to "free" area after listing resource. 
        ; Use it to set up menu
        PUSH	HL
        POP	IX

        LD	HL, HEAP
        LD	A, (HL)
        CP	0	; Any access point found?
        JR	Z, NOACCESSPOINTFOUND
        ; Create selection menu
        
        LD	A, (HL) 	; Get number of dir. entries
        LD	B, A		; Save for later
        CP	15
        JR 	C, _l1
    	LD	A, 15
_l1:
        LD	C, A
        ; Ready to set up
        
        INC	HL
        ; Now, setup menu at IX
        LD	(WIFIAPMENU), IX
        LD	A, 30
        LD	(IX+FRAME_OFF_WIDTH), A
        LD	(IX+FRAME_OFF_NUMBER_OF_LINES), C 	; Max visible entries
        ; TBD LD	(IX+MENU_OFF_DATA_ENTRIES), B 		; Total number of entries
        LD	(IX+FRAME_OFF_TITLEPTR), LOW(WIFIAPMENUTITLE)
        LD	(IX+FRAME_OFF_TITLEPTR+1), HIGH(WIFIAPMENUTITLE)

_l4:
	INC	HL ;Move past entry flags for now
        LD	A, 0 ; Entry attribute
        ; TBD WIDGET LD	(IX+MENU_OFF_FIRST_ENTRY), A
	; TBD WIDGET LD	(IX+MENU_OFF_FIRST_ENTRY+1), L
        ; TBD WIDGET LD	(IX+MENU_OFF_FIRST_ENTRY+2), H
        INC	IX
        INC	IX
        INC	IX
        ; Now, scan for NULL termination
_l2:    LD	A, (HL)
        OR	A
        INC 	HL
        JR 	NZ, _l2
        DJNZ	_l4

        ; Show menu
        LD	HL, (WIFIAPMENU)
        LD	D, 5 ; line to display menu at.
        ; TBD WIDGET CALL	MENU__INIT
        ; TBD WIDGET CALL	MENU__DRAW
        
        RET
        
NOACCESSPOINTFOUND:
	RET
        
; Flags:
;  	00: Scanning
;	01: Menu
;	10: Password
        
WIFICONFIG__HANDLEKEY:
	; TODO: check for "no access point found" message close 
	LD	C, A
        LD	A, (WIFIFLAGS)
        BIT	1, A
        JR 	Z, _n5
        LD	HL, PASSWDENTRY
        
        CALL	TEXTINPUT__HANDLEKEY
        ; Check output. 0: Enter, 1:cancel, 0xff: continue
        CP	0
        JR	Z, PASSWDFINISHED
        CP	1
        JR	Z, PASSWDCANCEL
        RET
        
_n5:
 	BIT	0, A
        RET	Z  ; Still in "Scanning"
_n4:
        LD	A, C
	LD	HL, (WIFIAPMENU)
        
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
       	JP	WIFIACTIVATE
_n3:
	; Check for cancel
        LD	DE,(CURKEY) ; Don't think is needed.
        LD	A, D
        CP	$27
        RET	NZ ; No caps shift
        LD	A, E	
        CP	$20
        RET	NZ
        ; Cancelled.
        
        ; TBD: this should be handled by state machine
        LD	A, $38
        LD	HL, (WIFIAPMENU)
        ; TBD WIDGET CALL	MENU__CLEAR
        
        LD	A, STATE_MAINMENU
        JP	ENTERSTATE

PASSWDCANCEL:
	LD	HL, PASSWDENTRY
        LD	A, $38
        CALL	FRAME__CLEAR
        
	LD	A, 1
        LD	(WIFIFLAGS), A
        
        LD	HL, (WIFIAPMENU)
        ; TBD WIDGET JP 	MENU__DRAW

PASSWDFINISHED:

        LD	HL, PASSWDENTRY
        LD	A, $38
        CALL	FRAME__CLEAR
        
WIFIACTIVATE:
	; Now, we need to iterate and find the entry "flags", which is one-byte before the 
        ; AP name.
        LD	IX,(WIFIAPMENU)
        ; TBD WIDGET LD	A, (IX+MENU_OFF_SELECTED_ENTRY)
        LD	C, A
        ; Mul by 3
        ADD	A, A
        ADD	A, C
        
        ADD_IX_A
        ; Ix is now offsetted.
        ; Load string pointer
        ; TBD WIDGET LD	L, (IX+MENU_OFF_FIRST_ENTRY+1)
        ; TBD WIDGET LD	H, (IX+MENU_OFF_FIRST_ENTRY+2)

        ; Go back one byte
        DEC	HL
        ; And load attibutes.
        LD	A, (HL)

        ; Bit '0' means if we need or not a password for AP.
        BIT	0, A
        JR	Z, _nopwdneeded
        ; We need a password. Check if we already have one.
        LD	DE, WIFIPASSWD
        LD	A, (DE)
        CP	0
        JR	Z, WIFIASKPASSWORD
        
_nopwdneeded
        ; No password needed. Just set up AP.
        INC	HL ; Get HL back to string pointer.

	; HL input is SSID.
WIFISENDCONFIG:
        ; Send in AP name.
        LD	A, CMD_SETUP_WIFI ; Setup wifi
        CALL	WRITECMDFIFO
        LD	A, $01 ; STA mode. 
        CALL	WRITECMDFIFO
        ; HL has SSID
        CALL	WRITECMDSTRING
        ; Check password, send if needed
        LD	HL, WIFIPASSWD
        CALL	WRITECMDSTRING
        ; Clear PWD entry: TODO
	; Go back to main menu
        LD	HL, (WIFIAPMENU)
        LD	A, $38
        ; TBD WIDGET CALL	MENU__CLEAR
        
        LD	A, STATE_MAINMENU
	JP	ENTERSTATE
;	RET
        
WIFIASKPASSWORD:
	; Close menu.
        LD	HL, (WIFIAPMENU)
        LD	A, $38
        ; TBD WIDGET CALL	MENU__CLEAR
	; Create password input.
        LD	A, 2 
        LD	(WIFIFLAGS), A ; 2 - password entry
        
	LD	IX, PASSWDENTRY
        ; Limit passwd size to 28. Sorry about that.
	LD	(IX+TEXTINPUT_OFF_MAX_LEN), 28
        LD	(IX+FRAME_OFF_WIDTH), 30
        LD      (IX+FRAME_OFF_TITLEPTR), LOW(PASSWDTITLE)
        LD      (IX+FRAME_OFF_TITLEPTR+1), HIGH(PASSWDTITLE)

	LD	HL, WIFIPASSWD

	LD	(IX+TEXTINPUT_OFF_STRINGPTR), L
        LD	(IX+TEXTINPUT_OFF_STRINGPTR+1), H
        XOR	A
        LD	(HL), A ; Clear password

       	;CALL	WIFIPASSWORD__SETUP	
        LD	HL, PASSWDENTRY
        LD	D, 14 ; line to display menu at.
        CALL	TEXTINPUT__INIT
        LD	HL, PASSWDENTRY
	JP	TEXTINPUT__DRAW
	RET
        
        
        
        
WIFIAPMENUTITLE:
	DB "Choose AP", 0
PASSWDTITLE: 
	DB "WiFi password", 0

WIFICONFIG__KEYHANDLER:
	RET

WIFITITLE:
	DB "WiFi configuration", 0
       
       
WIFICONFIG__REDRAW:
	PUSH	HL
        POP	IX
        LD	A, $78
        CALL	FRAME__CLEAR
        CALL	FRAME__DRAW
        ; Show relevant information
        
        LD	E, (IX+FRAME_OFF_SCREENPTR)       ; Screen..
        LD	D, (IX+FRAME_OFF_SCREENPTR+1)     ; Screen..
        INC 	DE
        INC 	DE
        CALL	MOVEDOWN
        CALL	MOVEDOWN
	CALL	PRINTAPMODE
        
        RET
        
PRINTAPMODE:
        LD	HL, APMODESTR
        PUSH	DE
        CALL	PRINTSTRING
	POP	DE
        INC	DE
        CALL	MOVEDOWN
        
        PUSH	DE
        LD	HL, CHANNELSTR
        CALL	PRINTSTRING
        LD	HL, TBDSTR
        CALL 	PRINTSTRING
        POP 	DE
        CALL	MOVEDOWN
        
        
        PUSH	DE
        LD	HL, SSIDSTR
        CALL	PRINTSTRING
        LD	HL, TBDSTR
        CALL 	PRINTSTRING
        POP 	DE
        CALL	MOVEDOWN

        PUSH	DE
        LD	HL, PWDSTR
        CALL	PRINTSTRING
        LD	HL, TBDSTR
        CALL 	PRINTSTRING
        POP 	DE
        CALL	MOVEDOWN
        
        RET
        
        
APMODESTR:	DB 	"Access point mode",0
CHANNELSTR:	DB	"Channel : ",0
SSIDSTR:	DB	"SSID    : ",0
PWDSTR:		DB	"Password: ",0
TBDSTR:		DB	"TBD",0		    
       
WIFICONFIG__INIT:
	LD	HL,WIFI_MENU
        PUSH	HL
        PUSH	HL
        POP	IX
        LD	(IX+FRAME_OFF_WIDTH), 30
        LD      (IX+FRAME_OFF_TITLEPTR), LOW(WIFITITLE)
        LD      (IX+FRAME_OFF_TITLEPTR+1), HIGH(WIFITITLE)
      
      	LD	(IX+FRAME_OFF_NUMBER_OF_LINES), 14
        LD	D, 4
        CALL	FRAME__INIT
        POP	HL
        RET

WIFICONFIG__SHOW:
	LD	HL, WIFICONFIG__CLASSDEF
        JP	WIDGET__DISPLAY

WIFICONFIG__CLASSDEF:
	DEFW	WIFICONFIG__INIT
        DEFW	WIDGET__IDLE
        DEFW	WIFICONFIG__KEYHANDLER
        DEFW	WIFICONFIG__REDRAW
        DEFW	FRAME__GETBOUNDS

