;  Functions in this file
;
;  PRINTHEX	: Print HEX byte
;  PRINTNIBBLE  : Print HEX nibble
;  PRINTCHAR	: Print char (ascii)
;  PRINTSTRING	: Print NULL-terminated string in HL
;  PRINTSTRINGN	: Print fixed-size string in HL

; PRINTHEX:	Print HEX byte in A.
; Inputs
;	A:	Byte to print
; 	DE:	Target screen address
; Outputs:
;	DE:	Updated screen address
; Clobbers:
;	AF HL

PRINTHEX:
	PUSH 	BC
	LD 	B, A
        SRL     A
        SRL     A
        SRL     A
        SRL     A
        CALL	PRINTNIBBLE
        LD	A, B
        AND	$F
        CALL 	PRINTNIBBLE
        POP	BC
        RET

; PRINTSTRING:	Print NULL-terminated string in HL
; Inputs
;	HL:	Pointer to string
; 	DE:	Target screen address
; Outputs:
;	DE:	Updated screen address
; Clobbers:
;	AF HL

PRINTSTRING:
	LD 	A,(HL)
        OR	A
        RET 	Z
        PUSH	HL	
        CALL	PRINTCHAR
        POP	HL
        INC	HL
        JR 	PRINTSTRING

; PRINTSTRINGN:	Print fixed-size string in HL
; Inputs
;	HL:	Pointer to string
;	A:	String length
; 	DE:	Target screen address
; Outputs:
;	DE:	Updated screen address
; Clobbers:
;	AF HL BC

PRINTSTRINGN:
	LD	B, A
PS1:
	LD 	A,(HL)
        PUSH	HL
        PUSH	BC
        CALL	PRINTCHAR
        POP	BC
        POP	HL
        INC	HL
        DJNZ 	PS1
        RET
        
; PRINTNIBBLE:	Print HEX nibble in A.
; Inputs
;	A:	Nibble to print
; 	DE:	Target screen address
; Outputs:
;	DE:	Updated screen address
; Clobbers:
;	AF HL
PRINTNIBBLE:
        CP	10
        JR	NC, NDEC
	ADD 	A, '0'
        JR	PRINTCHAR
NDEC:   ADD	A, 'A'-10
        JR 	PRINTCHAR
        

; PRINTCHAR:	Print char (ascii) in A.
; Inputs
;	A:	Char to print
; 	DE:	Target screen address
; Outputs:
;	DE:	Updated screen address
; Clobbers:
;	AF HL
;
PRINTCHAR:
	SUB	32
        PUSH 	BC
        PUSH 	DE
        LD	BC, CHAR_SET
        LD	H,$00		; set high byte to 0
	LD	L,A		; character to A
        ADD	HL,HL		; multiply
	ADD	HL,HL		; by
        ADD	HL,HL		; eight
        ADD	HL, BC
        LD	B, 8
PCLOOP1:
	LD	A, (HL)
        LD	(DE), A
        INC	D
        INC 	HL
        DJNZ 	PCLOOP1
        POP	DE
        INC 	DE
        POP	BC
        RET
