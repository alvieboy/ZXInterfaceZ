ifndef MENU_DEFS_INCLUDED
MENU_DEFS_INCLUDED EQU 1

include "frame_defs.asm"

;
; MENU structure definition
;   FRAME: 
; 	-- begin of "frame" information
;	off size 
; 	00  01   Width of menu
; 	01  01   Max. visible entries
; 	02  02   Screen pointer (start)
; 	04  02   Attribute pointer (start)
; 	06  02   Ptr to menu title
; 	-- end of "frame" information
;
; off size 
; 08  01   Number of entries
; 09  01   Current selected entry
; 0A  01   Current offset
; 0B  02   Pointer to callback table (or NULL)
; 0D  02   Pointer to item draw function
; tbd offset
; tbd offset

; 0F  01   Flags for entry 0
; 11  02   Ptr to entry #0
; 12  01   Flags for entry #1
; 13  02   Ptr to entry #1
; ..  ..   Ptr to entry #n


MENU_OFF_DATA_ENTRIES 			EQU	FRAME_OFF_MAX
MENU_OFF_SELECTED_ENTRY			EQU	FRAME_OFF_MAX+1
MENU_OFF_DISPLAY_OFFSET			EQU	FRAME_OFF_MAX+2
MENU_OFF_CALLBACKPTR			EQU	FRAME_OFF_MAX+3
MENU_OFF_DRAWFUNC			EQU	FRAME_OFF_MAX+5
MENU_OFF_SCREENOFFSET			EQU	FRAME_OFF_MAX+7
MENU_OFF_ATTROFFSET			EQU	FRAME_OFF_MAX+9

MENU_OFF_FIRST_ENTRY			EQU	FRAME_OFF_MAX+11
MENU_OFF_MAX				EQU	FRAME_OFF_MAX+11

MENU_COLOR_NORMAL			EQU	%01111000    ; Normal color
MENU_COLOR_DISABLED			EQU	%00111000    ; Disabled color
MENU_COLOR_SELECTED			EQU 	%01101000    ; Cyan, for selected

endif
