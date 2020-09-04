INC16 MACRO reg16
        ADD	A, L
    	LD	L, A   ; L = A+L
    	ADC	A, H   ; A = A+L+H+carry
    	SUB	L      ; A = H+carry
    	LD	H, A   ; H = H+carry
ENDM
