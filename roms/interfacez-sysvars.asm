;**********************************
;** ZX SPECTRUM SYSTEM VARIABLES **
;**********************************
P_RAMT		equ	$5C00  
P_TEMP		equ	$5C02
FRAMES1 	equ	$5C04
; Keyboard variables
KSTATE_0	equ	$5C05
KSTATE_4	equ	$5C06
REPDEL    	equ	$5C07
LASTK    	equ	$5C08
REPPER    	equ	$5C09

ERR_NR		equ	$5C0A
FLAGS		equ	$5C0B
TV_FLAG		equ	$5C0C

MENU1		equ	$5C0D ; 4 entries: size: 9+(4*2) = 9+16 = 25
	


SCREEN		equ	$4000
ATTR		equ	$5800
