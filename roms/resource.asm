include "resource_defs.asm"

READRESFIFO:
	IN	A, (PORT_RESOURCE_FIFO_STATUS)
        OR	A
        JR	NZ, READRESFIFO
        ; Grab resoruce data
        IN	A, (PORT_RESOURCE_FIFO_DATA)
        RET


; Corrupts: C AF

WRITECMDFIFO:
	LD	C, A
WAITFIFO1:
        IN 	A, (PORT_CMD_FIFO_STATUS)
        ;OR	A
        BIT	0, A
        JR	NZ, WAITFIFO1
        ; Send resource ID
        LD	A, C
        OUT	(PORT_CMD_FIFO_DATA), A
	RET
;
; Inputs: 	A:  Resource ID
;         	HL: Allocation function. This function will be called
;		    after fetching the resource size in DE, should return
;		    in HL the memory area to load the resource.
; Returns:
;		A:  Resource type, or $FF if not found
;		DE: Resource size
;		Z:  Zero if resource is invalid.
;		HL points past last item received.

; Corrupts:	HL, BC, F

LOADRESOURCE_ALLOCFUN:

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

	PUSH	DE
        
        ; Call the alloc function. Need to set up return address first
        PUSH	HL
        LD	HL, WAITFIFO3
        EX	(SP), HL
        JP	(HL)
        ; Continues in WAITFIFO3


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

WRITECMDSTRING:
        PUSH	HL
        CALL	STRLEN
        POP	HL
        CALL	WRITECMDFIFO ; Send len.
        CP	$00
        RET	Z	     ; Empty string, skip
        ; Send string
        LD	B, A
_l3:
        LD	A,(HL)
        INC	HL
        CALL	WRITECMDFIFO
        DJNZ 	_l3
        RET
        
        ; A: size to read
        ; HL: target
        ; DE: count, which will be decremented.
READRESTOBUFFER:
        ; Copy chunk of data first
        LD	B, A
_readmore:
        CALL	READRESFIFO
        LD	(HL), A
        INC	HL
        DEC	DE
        DJNZ	_readmore
        RET
