ORG ROM_PATCHED_SNALOAD
RESTOREREGS:
; For 128K machines we need to restore ROM number if needed. Can also set the RAM mapping
; correctly. 
	LD	BC, $1FFD
        LD	A, $04 ; Bit 2 set (48K ROM)
        OUT	(C), A
        LD	B, $7F
        LD      A, $10 ; Bit 4 set (48K ROM).
        OUT	(C), A
RESTOREREGS_NOROM:
; This is the register restore routine.
        LD	HL, (P_TEMP)	; Save P_TEMP
        EXX
        LD	DE, $FFFF 	; FILL register: AF'
        LD	(P_TEMP), DE
        LD	SP, P_TEMP      ; 
        LD	BC, $FFFF	; FILL register: BC' 
        LD	DE, $FFFF       ; FILL register: DE'
        LD	HL, $FFFF       ; FILL register: HL'
        POP	AF
        EX	AF, AF'
        EXX
        LD	A, $FF          ; FILL register: I
        LD	I, A
        LD	DE, $FFFF 	; FILL register : AF
        LD	(P_TEMP), DE
        LD	BC, $FFFF       ; FILL register : BC
        
        LD	SP, P_TEMP      ; TBD: check if SP points to actual word
        LD	A, $FF          ; FILL register: R
        LD	R, A
        LD	A, $FF		; FILL border
        OUT	($FE), A
        POP	AF
        LD	DE, $FFFF       ; FILL register : DE
        LD	IX, $FFFF       ; FILL register: IX
        LD	IY, $FFFF       ; FILL register: IY
        LD	(P_TEMP), HL    ; Restore P_TEMP
        LD	SP, $FFFF       ; FILL register : SP
	NOP
        NOP
        NOP     ; LD HL, xxxx	: 21xxxx ; NOTE: this will be changed into (LD HL,XXXX) for Z80 snapshots
        NOP	; PUSH	HL	: E5     ; NOTE: this will be changed into (PUSH HL) for Z80 snapshots
        LD	HL, $FFFF       ; FILL register : HL
        IM	0		; FILL IM X
        EI			; FILL EI or DI, depending. We should time this so that no interrupt can come before JP
RETINST:RETN
