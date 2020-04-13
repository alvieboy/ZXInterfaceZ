READRESFIFO:
	IN	A, ($0B)
        OR	A
        JR	NZ, READRESFIFO
        ; Grab resoruce data
        IN	A, ($0D)
        RET


; Corrupts: C AF

WRITECMDFIFO:
	LD	C, A
WAITFIFO1:
        IN 	A, ($07)
        OR	A
        JR	NZ, WAITFIFO1
        ; Send resource ID
        LD	A, C
        OUT	($09), A
	RET

;
; Inputs: 	A:  Resource ID
;         	HL: Target memory area
; Returns:
;		A:  Resource type, or $FF if not found
;		DE: Resource size
;		Z:  Zero if resource is invalid.
;		HL points past last item received.

; Corrupts:	HL, BC, F

LOADRESOURCE:
	LD	B, A
        XOR	A ; A=0
        CALL 	WRITECMDFIFO	; Send "REQUEST_RESOURCE" command
	LD	A, B		; Resource ID
        CALL 	WRITECMDFIFO	
        CALL	READRESFIFO
        CP	$FF
        RET	Z	; If invalid, return immediatly
        
        LD	C, A ; Save resource type in C

        CALL	READRESFIFO        
        LD	E, A 	; Resource size LSB

        CALL	READRESFIFO
        LD	D, A 	; Resource size MSB
        
        PUSH	DE	; Save resource size.
	
WAITFIFO3:

        CALL	READRESFIFO
        
        LD	(HL), A
        INC	HL
        DEC	DE
       	LD	A, D
    	OR	E
        JR 	NZ,   WAITFIFO3

        LD	A, C ; Get resource type back in A
	POP 	DE   ; Get resource size back

        CP	$FF
        RET

