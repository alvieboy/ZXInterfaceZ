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

MENU1		equ	$5C20 ; 4 entries: size: 9+(4*2) = 9+16 = 25
	
HEAP		equ	$5D00 ; 4 entries: size: 9+(4*2) = 9+16 = 25


SCREEN		equ	$4000
LINE23		equ 	$50E0
LINE22		equ 	$50C0
LINE21		equ 	$50A0
ATTR		equ	$5800
