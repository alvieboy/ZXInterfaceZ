
KEYBTEST:
	DI
        XOR	A
        LD	(NMI_SCRATCH), A
KEYBTEST2:
	LD	DE, SCREEN
        LD	HL, MSG1
        CALL	PRINTSTRING
        CALL 	MOVENEXTLINE

	LD	A, (NMI_SCRATCH)
        CALL	PRINTHEX
        CALL	MOVENEXTLINE

        LD	BC, $FEFE
        IN	A, (C)
        AND	%00011111
        CALL	PRINTHEX
        CALL	MOVENEXTLINE

        LD	BC, $FDFE
        IN	A, (C)
        AND	%00011111
        CALL	PRINTHEX
        CALL	MOVENEXTLINE

        LD	BC, $FBFE
        IN	A, (C)
        AND	%00011111
        CALL	PRINTHEX
        CALL	MOVENEXTLINE

        LD	BC, $F7FE
        IN	A, (C)
        AND	%00011111
        CALL	PRINTHEX
        CALL	MOVENEXTLINE

        LD	BC, $EFFE
        IN	A, (C)
        AND	%00011111
        CALL	PRINTHEX
        CALL	MOVENEXTLINE

        LD	BC, $DFFE
        IN	A, (C)
        AND	%00011111
        CALL	PRINTHEX
        CALL	MOVENEXTLINE

        LD	BC, $BFFE
        IN	A, (C)
        AND	%00011111
        CALL	PRINTHEX
        CALL	MOVENEXTLINE

        LD	BC, $7FFE
        IN	A, (C)
        AND	%00011111
        CALL	PRINTHEX
        CALL	MOVENEXTLINE
        
        LD	A, (NMI_SCRATCH)
        INC	A
        LD	(NMI_SCRATCH), A
        
        JP 	KEYBTEST2
        
        ENDLESS
MSG1:	DB "Keyboard test", 0
MSG2:	DB "Error: ", 0
MSG3:   DB "OK.", 0
