Dialog__result_OFFSET	EQU	Window__SIZE
Dialog__SIZE		EQU	Window__SIZE+1

Dialog__setResult:
	LD	(IX+Dialog__result_OFFSET), A
        RET

Dialog__exec:
        DEBUGSTR "Dialog exec "
        DEBUGHEXIX
        
	; Get screen
        PUSH	IX
        LD	L, (IX+Window__screen_OFFSET)
        LD	H, (IX+Window__screen_OFFSET+1)

        PUSH	HL
        POP	IX
        DEBUGSTR "Call eventLoop\n"
        CALL	Screen__eventLoop
        DEBUGSTR "Exited eventLoop\n"
        POP	IX
        ; TBD: Check if return is "yes" or "no"
        LD	A, (IX+Dialog__result_OFFSET)
        CP	0
        RET

