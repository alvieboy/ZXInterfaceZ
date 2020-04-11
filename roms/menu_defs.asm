;
; MENU structure definition
; off size meaning
; 00  01   Width of menu
; 01  01   Number of entries
; 02  01   Current selected entry
; 03  02   Screen pointer (start)
; 05  02   Attribute pointer (start)
; 07  02   Pointer to callback table (or NULL)
; 09  02   Ptr to menu title
; 0B  02   Ptr to entry #0
; 0D  02   Ptr to entry #1
; ..  ..   Ptr to entry #n

MENU_OFF_WIDTH 				EQU	0
MENU_OFF_MAX_VISIBLE_ENTRIES 		EQU	2
MENU_OFF_DATA_ENTRIES 			EQU	3
MENU_OFF_SELECTED_ENTRY			EQU	4
MENU_OFF_SCREENPTR			EQU	5
MENU_OFF_ATTRPTR			EQU	7
MENU_OFF_CALLBACKPTR			EQU	9
MENU_OFF_MENU_TITLE			EQU	$0B
MENU_OFF_FIRST_ENTRY			EQU	$0D
