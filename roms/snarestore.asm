RESTOREREGS:
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
	LD	HL, $FFFF       ; FILL register : HL
        IM	0		; FILL IM X
        EI			; FILL EI or DI, depending. We should time this so that no interrupt can come before JP
RETINST:RETN
