ifndef MENU_DEFS_INCLUDED
MENU_DEFS_INCLUDED EQU 1

;
; MENU structure definition
; off size meaning
; 00  01   Width of menu
; 01  01   Max. visible entries
; 02  02   Screen pointer (start)
; 04  02   Attribute pointer (start)
; 06  02   Ptr to menu title
; -- end of "frame" information

; 08  01   Number of entries
; 09  01   Current selected entry
; 0A  01   Current offset

; 0B  02   Pointer to callback table (or NULL)

; 0D  01   Flags for entry 0
; 0E  02   Ptr to entry #0
; 10  01   Flags for entry #1
; 11  02   Ptr to entry #1
; ..  ..   Ptr to entry #n

MENU_OFF_WIDTH 				EQU	$00
MENU_OFF_MAX_VISIBLE_ENTRIES 		EQU	$01
MENU_OFF_SCREENPTR			EQU	$02
MENU_OFF_ATTRPTR			EQU	$04
MENU_OFF_MENU_TITLE			EQU	$06
MENU_OFF_DATA_ENTRIES 			EQU	$08
MENU_OFF_SELECTED_ENTRY			EQU	$09
MENU_OFF_DISPLAY_OFFSET			EQU	$0A
MENU_OFF_CALLBACKPTR			EQU	$0B
MENU_OFF_FIRST_ENTRY			EQU	$0D

MENU_COLOR_NORMAL			EQU	%01111000    ; Normal color
MENU_COLOR_DISABLED			EQU	%00111000    ; Disabled color
MENU_COLOR_SELECTED			EQU 	%01101000    ; Cyan, for selected

endif
