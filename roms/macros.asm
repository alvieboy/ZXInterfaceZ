ADD_HL_A MACRO
        ADD	A, L
    	LD	L, A   ; L = A+L
    	ADC	A, H   ; A = A+L+H+carry
    	SUB	L      ; A = H+carry
    	LD	H, A   ; H = H+carry
ENDM

ADD_IX_A MACRO
        ADD	A, IXL
    	LD	IXL, A   ; L = A+L
    	ADC	A, IXH   ; A = A+L+H+carry
    	SUB	IXL      ; A = H+carry
    	LD	IXH, A   ; H = H+carry
ENDM

ADD_IY_A MACRO
        ADD	A, IYL
    	LD	IYL, A   ; L = A+L
    	ADC	A, IYH   ; A = A+L+H+carry
    	SUB	IYL      ; A = H+carry
    	LD	IYH, A   ; H = H+carry
ENDM

ENDLESS MACRO
	_endless: JR _endless
ENDM

SWAP_IX_HL MACRO
	PUSH	IX
        EX	(SP), HL
        POP	IX 
ENDM

SWAP_IX_DE MACRO
	PUSH	DE
        EX	(SP), IX
        POP	DE
ENDM

UNIMPLEMENTED MACRO
	ENDLESS
ENDM