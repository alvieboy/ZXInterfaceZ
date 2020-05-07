; Test address in HL
TESTADDRESS:
	LD	C, $15
        LD	A, L
        OUT	($11), A
        LD	A, H
        OUT	($13), A
p1:
        OUT	(C), B
        LD	A, L
        OUT	($11), A
        LD	A, H
        OUT	($13), A
        IN	A, (C)
        CP	B
        RET 	

EXTRAMTEST:
	DI
	LD	DE, SCREEN
        LD	HL, MSG1
        CALL	PRINTSTRING
        CALL 	MOVENEXTLINE
        
        LD	HL, 0
        LD	B, $5A
NEXTADDRESS:
        CALL	TESTADDRESS
        JR	NZ, ERROR
        LD	A, B
        ADD	A, $11
        LD	B, A
        INC	HL
        LD	A, L
        OR	H
        JR 	NZ, NEXTADDRESS
        
        LD	HL, MSG3
        CALL	PRINTSTRING
        ENDLESS
ERROR:
        PUSH 	BC
	PUSH	AF
        
	PUSH	HL
        LD	HL, MSG2
        CALL	PRINTSTRING
        POP	HL
        PUSH	HL
        LD	A, H
        CALL    PRINTHEX
        POP	HL
	;PUSH	HL
        LD	A, L
        CALL    PRINTHEX
        
        
        POP	AF
        
        INC 	DE ; Space
        
        CALL	PRINTHEX
        POP	BC
	LD	A, B
        INC 	DE ; Space
        CALL    PRINTHEX
        
        ENDLESS
MSG1:	DB "Performing RAM test", 0
MSG2:	DB "Error: ", 0
MSG3:   DB "OK.", 0
