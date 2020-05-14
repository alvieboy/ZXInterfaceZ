;**********************************
;** ZX SPECTRUM SYSTEM VARIABLES **
;**********************************
P_RAMT		equ	$5C00  
P_TEMP		equ	$5C02
FRAMES1 	equ	$5C04
; Keyboard variables
KSTATE_0	equ	$5C05
KSTATE_1	equ	$5C06
KSTATE_2	equ	$5C07
KSTATE_3	equ	$5C08
KSTATE_4	equ	$5C09
KSTATE_5	equ	$5C09
KSTATE_6	equ	$5C09
KSTATE_7	equ	$5C09

REPDEL    	equ	$5C0A
REPPER    	equ	$5C0B
LASTK    	equ	$5C0C


; IY-based variables.
IYBASE		equ	$5C0D
ERR_NR		equ	$5C0E
FLAGS		equ	$5C0F
TV_FLAG		equ	$5C10
FLAGS2	        equ	$5C11
MODE		equ	$5C12
PREVKEY		equ     $5C13
CURKEY		equ     $5C15
KFLAG		equ     $5C17

CURRSTATUS	equ 	$5C19 ; Current status
CURRSTATUSXOR	equ 	$5C1A ; Current status XOR previous status

WIFIFLAGS	equ 	$5C1B
SDMENU		equ 	$5C1C ; To store SD card menu location in RAM
WIFIAPMENU	equ 	SDMENU ; Reuse.
STATE		equ 	$5C1E
PREVSTATUS	equ 	$5C1F ; Previous status
MENU1		equ	$5C20 ; 4 entries: size: 9+(4*2) = 9+16 = 25
PASSWDENTRY     equ	$5C40

TEXTMESSAGEAREA	equ	$5CA0 
SSID		equ	$5CB0 

WIFIPASSWD      equ	$5CD0 ; TODO.

STATUSBUF	equ	$5CF0 


HEAP		equ	$5E00 


SCREEN		equ	$4000
LINE1		equ 	$4020
LINE2		equ 	$4040
LINE3		equ 	$4060
LINE4		equ 	$4080
LINE23		equ 	$50E0
LINE22		equ 	$50C0
LINE21		equ 	$50A0
ATTR		equ	$5800
SCREENSIZE	equ     $1B00



; NMI areas.
NMI_SCRATCH	equ 	$5D00
NMIEXIT		equ 	$5D02
NMI_MENU 	equ 	$5D03
SNAFILENAME	equ	$5D40
FILENAMEENTRYWIDGET equ	$5D50
NMICMD_RESPONSE equ	$5D60
NMI_SPVAL	equ 	$5FFE

; Patched ROM (on-the-fly)  - max 256 bytes

ROM_PATCHED_SNALOAD equ $3F00
