ORG $0F38
	; 	Originally this calls WAIT_KEY. We replace the call alltogether
        PUSH	BC
        LD	A, CMD_KEYINJECT
        CALL	WRITECMDFIFO 		; Ask for key injection
        CALL 	READRESFIFO
        POP	BC
        CP	$FF                     ; Any key to inject?
        JR	Z, _waitkey
        ; We have a key. 
        RET_TO_ROM_AT $0F3B
_waitkey:
	; Call WAIT_KEY normally
        ; But first save return address so WAIT_KEY returns to where 
        ; originally it would ($0F3B)
        PUSH	HL
        LD	HL, $0F3B
        EX 	(SP), HL
        RET_TO_ROM_AT WAIT_KEY

