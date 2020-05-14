; Place well-known values in registers so we can check the snapshot features
REGTEST:
	DI
	LD	A, $01
	EX 	AF, AF'
	EXX
        LD	BC, $0203
        LD	DE, $0405
        LD	HL, $0607
        EXX
        LD	SP, $FACA
        LD	BC, $090A
        LD	DE, $0D0E
        LD	HL, $0F10
        LD	IX, $1112
        LD	IY, $1314
        LD	A, $3F
        LD	I, A
        LD	A, $15
_end1:	JR	REGTEST

REGDEBUG:
	LD	(P_TEMP), SP
        LD	SP, NMI_SPVAL
        ; Push all into stack
        PUSH	AF
        PUSH	BC
        PUSH	DE
        PUSH	HL
        LD	HL, (P_TEMP)
        PUSH	HL ; SP
        PUSH	IX
        PUSH	IY
        EX 	AF, AF'
        PUSH	AF
        EXX	
        PUSH	BC
        PUSH	DE
        PUSH	HL
        EXX
        LD	A, I
        PUSH	AF	; I and EI/DI
        LD	A, R
        PUSH	AF	; R and EI/DI
        
        
        
        LD	HL, $4000
        LD	D, $BF
        LD	B, $FF

        CALL 	SETATTRS

        
        ; Print them
        LD	DE, SCREEN
        LD	IX, NMI_SPVAL-2
        
        LD	HL, RNAME_AF
        CALL	DUMPREG

        DEC	IX
        DEC	IX
        LD	HL, RNAME_BC
        CALL	DUMPREG
        

        ENDLESS
DUMPREG:
	PUSH	DE
	CALL	PRINTSTRING
        LD	H, (IX)
        LD	L, (IX+1)
        CALL    PRINTHEXHL
        POP	DE
        CALL	MOVEDOWN
        RET
        
RNAME_AF: 	DB "AF  :", 0
RNAME_BC: 	DB "BC  :", 0
;   ------------------------------------------------------------------------
;   0        1      byte   I                                      Check
;   1        8      word   HL',DE',BC',AF'                        Check
;   9        10     word   HL,DE,BC,IY,IX                         Check
;   19       1      byte   Interrupt (bit 2 contains IFF2, 1=EI/0=DI)  Check
;   20       1      byte   R                                      Check
;   21       4      words  AF,SP                                  Check
;   25       1      byte   IntMode (0=IM0/1=IM1/2=IM2)            Check
;   26       1      byte   BorderColor (0..7, not used by Spectrum 1.7)  Check
;   27       49152  bytes  RAM dump 16384..65535
;   ------------------------------------------------------------------------



