
NMIHANDLER:
	; Upon entry we already have AF on the stack.
        LD	A, 0
        OUT	($11), A         ; Set external RAM LSB
        OUT	($13), A         ; and MSB addresses. So we start read/write at 0x0000
        ; Save regs.
        LD	A, C
        LD	C, $15           ; Data port for external RAM
        
        OUT	(C), A           ; Ram: 0x0000
        OUT	(C), B           ; Ram: 0x0001
        OUT	(C), E           ; Ram: 0x0002
        OUT	(C), D           ; Ram: 0x0003
        OUT	(C), L           ; Ram: 0x0004
        OUT	(C), H           ; Ram: 0x0005
        
        ; Save whole screen + scratch area to External RAM. 
        ; 8192 bytes (0x2000)
        LD	B, $00
        LD	D, $20
	LD	HL, SCREEN
lone:
        OTIR                   	; Starts at 0x0006
        DEC	D
        JR	NZ, lone
        ; Screen saved.
        
        ; Now, SP, IX, IY. Note that SP has 4 extra bytes for AF and NMI return address.
        
        LD	(NMI_SCRATCH), SP
        LD	HL, (NMI_SCRATCH)
        OUT	(C), L        	; Ram: 0x2006
        OUT	(C), H        	; Ram: 0x2007
        
        LD	A, IXL
        OUT	(C), A        	; Ram: 0x2008
        LD	A, IXH
        OUT	(C), A        	; Ram: 0x2009

        LD	A, IYL
        OUT	(C), A        	; Ram: 0x200A
        LD	A, IYH
        OUT	(C), A        	; Ram: 0x200B
        
        ; We don't touch alternative registers, so leave them alone for now.
        
        ; Do save AF, however. It's easier to just do it here.
        ; HL still contains SP value
        LD	A, (HL)
        OUT	(C), A          ; Ram: 0x200C
        INC	HL
        LD	A, (HL)
        OUT	(C), A          ; Ram: 0x200D
        
        ; Create a new SP
        LD	HL, NMI_SPVAL
	LD 	(NMI_SCRATCH), HL
        LD	SP, (NMI_SCRATCH)
        ; We are good to go.
        
        LD	IY, IYBASE

        CALL	NMIPROCESS

NMIRESTORE:
	; Restore SP first, since we need to manipulate RAM 
        ; in order to restore it
        LD	C, $15		; Data port 
        LD	A, $06		; LSB 
        OUT	($11), A 
        LD	A, $20          ; MSB
        OUT	($13), A
        IN	L, (C)
        IN	H, (C)
        LD	(NMI_SCRATCH), HL
        LD	SP, (NMI_SCRATCH)
        ; SP ready now.
        ; Restore IX, IY now
        IN	A, (C)
        LD	IXL, A
        IN	A, (C)
        LD	IXH, A
        IN	A, (C)
        LD	IYL, A
        IN	A, (C)
        LD	IYH, A
        ; Move back to screen area.
        LD	A, $06		; LSB 
        OUT	($11), A 
        LD	A, $00          ; MSB
        OUT	($13), A
        ; Restore screen and scratchpad memory
        LD	B, $00
        LD	D, $20
        LD	E, A
	LD	HL, SCREEN
_l2:
        INIR                   	; Starts at 0x0006
        DEC	D
        JR	NZ, _l2
        ; Restore other regs


        LD	A, $01		; LSB 
        OUT	($11), A 
        LD	A, $00          ; MSB
        OUT	($13), A
        ; BEDLH
       	IN	B,(C)
        IN	E,(C)
        IN	D,(C)
        IN	L,(C)
        IN	H,(C)
	; Last one is C.
        XOR	A
        OUT	($11), A
        IN	C, (C)
        ; SP is "good" here.
	POP 	AF
        RETN

        
NMIPROCESS:
	LD	A,0
        LD	(IY+(FLAGS-IYBASE)),A
        LD	(NMIEXIT), A

	CALL	NMIMENU__SETUP
       	LD	HL, NMI_MENU
        LD	D, 6 ; line to display menu at.
        CALL	MENU__INIT
        CALL	MENU__DRAW
        
NMILOOP:
	; Scanning loop
	CALL	KEYBOARD
        CALL	NMIKEY_INPUT
        LD	A, (NMIEXIT)
        CP	0
        JR	Z, NMILOOP
        JP	WAITFORNOKEY
	;RET

NMIKEY_INPUT:	
	BIT	5,(IY+(FLAGS-IYBASE))	; test FLAGS  - has a new key been pressed ?
	RET	Z		; return if not.
        ;CALL	DEBUGHEXA
	LD	DE, (CURKEY)
	LD	A, D
	RES	5,(IY+(FLAGS-IYBASE))	; update FLAGS  - reset the new key flag.
        DEC	A
        RET 	Z 	; Modifier key applied, skip
        LD	A, E
	JP	NMIMENU__HANDLEKEY

SNASAVE:
	; For SNA we need to also populate alternative registers.
        ; These start at 0x200E
        LD	C, $15		; Data port 
        
        LD	A, $0E		; LSB 
        OUT	($11), A 
        LD	A, $20          ; MSB
        OUT	($13), A
        
        EXX	; We must not modify EX' registers!
	PUSH	BC
        PUSH	HL
        PUSH	AF        
        LD	A, C
        LD	C, $15
        OUT	(C), A           ; Ram: 0x200E
        OUT	(C), B           ; Ram: 0x200F
        OUT	(C), E           ; Ram: 0x2010
        OUT	(C), D           ; Ram: 0x2011
        OUT	(C), L           ; Ram: 0x2012
        OUT	(C), H           ; Ram: 0x2013
        ; Fetch AF from stack
        LD	(NMI_SCRATCH), SP
        LD	HL, (NMI_SCRATCH)
        LD	A, (HL)
        OUT	(C), A           ; Ram: 0x2014
        INC	HL
        LD	A, (HL)
        OUT	(C), A           ; Ram: 0x2015
        
        POP	AF
        POP	HL
        POP	BC
        EXX

	; In order to save SNA, finish memory save to external RAM
        ; 40960 bytes (0xA000)
        LD	B, $00
        LD	D, $A0
	LD	HL, $6000	; From 0x6000 to 0xFFFF
_sone:
        OTIR                   	; Starts at 0x0006
        DEC	D
        JR	NZ, _sone
        
        ; Now, send in filename
        LD	HL, SNAFILENAME
        LD	A,0
        LD 	(HL),A

        ; All saved. Now, trigger SNA save
        LD	A, CMD_SAVE_SNA
        CALL	WRITECMDFIFO 

        PUSH	HL
        CALL	STRLEN
        POP	HL
        
        CALL	WRITECMDFIFO 

_wait:
        LD	HL, NMICMD_RESPONSE
	LD	A, RESOURCE_ID_OPERATION_STATUS
        CALL	LOADRESOURCE
        JR	Z, _error1
        ; Get operation status
        LD	A, (NMICMD_RESPONSE)
        CP      STATUS_INPROGRESS
        JR	Z, _wait
        CP	STATUS_OK
        RET 	Z
	LD	HL, NMICMD_RESPONSE
        INC	HL	
        ; Load error string len
        LD	A, (HL)
        CP	0
        JR	Z, _showdefault
        PUSH	HL
        INC	HL
        ADD	A, L
        LD	L, A ; Should not overflow.
        XOR	A
        LD	(HL), A
        POP	HL
        INC	HL
	CALL    TEXTMESSAGE__SHOW
        CALL	WAITFORENTER
        ; Clear
        LD	A, $38
        CALL 	TEXTMESSAGE__CLEAR
        LD	HL, NMI_MENU
        CALL	MENU__DRAW
        RET
        ;RET
_showdefault:
	LD	HL, UNSPECIFIEDMSG
        JP	TEXTMESSAGE__SHOW
_error1:
	


	ENDLESS
        JP 	INTERNALERROR
        RET
        
WAITFORENTER:
	CALL	KEYBOARD
	BIT	5,(IY+(FLAGS-IYBASE))	; test FLAGS  - has a new key been pressed ?
	JR	Z, WAITFORENTER
	LD	DE, (CURKEY)
	LD	A, D
	RES	5,(IY+(FLAGS-IYBASE))	; update FLAGS  - reset the new key flag.
        DEC	A
        JR	Z, WAITFORENTER ; Modifier key applied, skip
        LD	A, E
        CP	$21
        RET	Z
	JR 	WAITFORENTER

WAITFORNOKEY:
        LD	BC, $FEFE
_w1:
        IN	A, (C)
        OR	%11100000
        XOR	$FF
        JR	NZ, WAITFORNOKEY
        RLC	B		; form next port address e.g. FEFE > FDFE
	JR	C,_w1
        RET


UNSPECIFIEDMSG: DB "Unspecified error", 0
