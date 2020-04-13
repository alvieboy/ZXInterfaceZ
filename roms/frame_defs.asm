ifndef FRAME_DEFS_INCLUDED
FRAME_DEFS_INCLUDED EQU 1

;
; FRAME structure definition
; 00  01   Width of frame
; 01  01   Max. visible lines
; 02  02   Screen pointer (start)
; 04  02   Attribute pointer (start)
; 06  02   Ptr to title

FRAME_OFF_WIDTH 			EQU	$00
FRAME_OFF_NUMBER_OF_LINES 		EQU	$01
FRAME_OFF_SCREENPTR			EQU	$02
FRAME_OFF_ATTRPTR			EQU	$04
FRAME_OFF_TITLEPTR			EQU	$06

FRAME_OFF_MAX				EQU	$08 ; Max size of frame

endif
