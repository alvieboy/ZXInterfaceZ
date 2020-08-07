include "port_defs.asm"

NMIHANDLER:
	; Upon entry we already have AF on the stack.
        LD	A, 0
        OUT	(PORT_RAM_ADDR_0), A         ; Set external RAM LSB
        OUT	(PORT_RAM_ADDR_2), A         
        LD	A, $20
        OUT	(PORT_RAM_ADDR_1), A         ; and MSB addresses. So we start read/write at 0x002000
        
        ; Save regs.
        LD	A, C
        LD	C, PORT_RAM_DATA           ; Data port for external RAM
        
        OUT	(C), A           ; Ram: 0x2000
        OUT	(C), B           ; Ram: 0x2001
        OUT	(C), E           ; Ram: 0x2002
        OUT	(C), D           ; Ram: 0x2003
        OUT	(C), L           ; Ram: 0x2004
        OUT	(C), H           ; Ram: 0x2005
        
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
        LD	HL, (NMI_SCRATCH)     ; HL=SP
        OUT	(C), L        	; Ram: 0x4006
        OUT	(C), H        	; Ram: 0x4007
        
        LD	A, IXL
        OUT	(C), A        	; Ram: 0x4008
        LD	A, IXH
        OUT	(C), A        	; Ram: 0x4009

        LD	A, IYL
        OUT	(C), A        	; Ram: 0x400A
        LD	A, IYH
        OUT	(C), A        	; Ram: 0x400B
        
        ; We don't touch alternative registers, so leave them alone for now.
        
        ; Do save AF, however. It's easier to just do it here.
        ; HL still contains SP value
        LD	A, (HL)
        OUT	(C), A          ; Ram: 0x400C [flags]
        INC	HL
        LD	A, (HL)
        OUT	(C), A          ; Ram: 0x400D [A]

        ; Get R 
        LD	A, R
        OUT	(C), A	   	; Ram: 0x400E

        LD	SP, NMI_SPVAL
        PUSH	AF
        
        ; Get flags from SP
        LD	HL, NMI_SPVAL-2
        LD	A, (HL)
        OUT	(C), A	   	; Ram: 0x400F
        POP	AF
        
        ; Finally, save I
        LD	A, I
        OUT	(C), A	   	; Ram: 0x4010
        
        ; 
        
        ; Save border.
        IN	A, (PORT_ULA)
        OUT	(C), A	   	; Ram: 0x4011

        ; We are good to go.

        LD	IY, IYBASE


        CALL	NMIPROCESS

NMIRESTORE:
	; Restore SP first, since we need to manipulate RAM 
        ; in order to restore it
        LD	C, PORT_RAM_DATA	       ; Data port 
        LD	A, $06		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        LD	A, $40          ; MSB
        OUT	(PORT_RAM_ADDR_1), A
        XOR	A
        OUT	(PORT_RAM_ADDR_2), A   ; Address: 0x004006
        
        IN	L, (C)          ; Ram: 0x4006
        IN	H, (C)          ; Ram: 0x4007
        LD	(NMI_SCRATCH), HL
        LD	SP, (NMI_SCRATCH)
        ; SP ready now.
        ; Restore IX, IY now
        IN	A, (C)          ; Ram: 0x4008
        LD	IXL, A
        IN	A, (C)          ; Ram: 0x4009
        LD	IXH, A
        IN	A, (C)          ; Ram: 0x400A
        LD	IYL, A
        IN	A, (C)
        LD	IYH, A          ; Ram: 0x400B
        
        ; Move back to screen area.
        LD	A, $06		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        LD	A, $20          ; MSB
        OUT	(PORT_RAM_ADDR_1), A     ; 0x2006
        ; Restore screen and scratchpad memory
        LD	B, $00
        LD	D, $20
        LD	E, A
	LD	HL, SCREEN
_l2:
        INIR                   	; Starts at 0x2006
        DEC	D
        JR	NZ, _l2
        ; Restore other regs


        LD	A, $01		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        LD	A, $20          ; MSB   
        OUT	(PORT_RAM_ADDR_1), A
        ; BEDLH
       	IN	B,(C)           ; Ram: 0x2001
        IN	E,(C)           ; Ram: 0x2002
        IN	D,(C)           ; Ram: 0x2003
        IN	L,(C)           ; Ram: 0x2004
        IN	H,(C)           ; Ram: 0x2005
	; Last one is C.
        XOR	A
        OUT	(PORT_RAM_ADDR_0), A        ; Ram: 0x2000
        ;LD	A, $20
        ;OUT	(PORT_RAM_ADDR_1), A        ; Ram: 0x0000
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
        ; These start at 0x2012
        LD	C, PORT_RAM_DATA		; Data port 
        
        LD	A, $12		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        LD	A, $20          ; MSB
        OUT	(PORT_RAM_ADDR_1), A
        
        EXX	; We must not modify EX' registers!
	PUSH	BC
        PUSH	HL

        EX	AF, AF'
        PUSH	AF
        LD	A, C
        LD	C, PORT_RAM_DATA
        OUT	(C), A           ; Ram: 0x2012
        OUT	(C), B           ; Ram: 0x2013
        OUT	(C), E           ; Ram: 0x2014
        OUT	(C), D           ; Ram: 0x2015
        OUT	(C), L           ; Ram: 0x2016
        OUT	(C), H           ; Ram: 0x2017
        ; Fetch AF from stack
        LD	(NMI_SCRATCH), SP
        LD	HL, (NMI_SCRATCH)
        LD	A, (HL)
        OUT	(C), A           ; Ram: 0x2018 [flags]
        INC	HL
        LD	A, (HL)
        OUT	(C), A           ; Ram: 0x2019 [A]
        
        POP	AF
        
        EX	AF, AF'

        POP	HL
        POP	BC
        EXX
	
        ; Still missing IM.
        LD	A, $01           ; Ram: 0x201A [A]
        OUT	(C), A

	; In order to save SNA, finish memory save to external RAM
        ; 40960 bytes (0xA000)
        LD	B, $00
        LD	D, $A0
	LD	HL, $6000	; From 0x6000 to 0xFFFF
_sone:
        OTIR                   	; Starts at 0x001B
        DEC	D
        JR	NZ, _sone
        
        ; All saved. Now, trigger SNA save
        LD	A, CMD_SAVE_SNA
        CALL	WRITECMDFIFO 
        ; Now, send in filename
        LD	HL, SNAFILENAME
        CALL	WRITECMDSTRING

_wait:
        LD	HL, NMICMD_RESPONSE
	LD	A, RESOURCE_ID_OPERATION_STATUS
        CALL	LOADRESOURCE
        JR	Z, _error1
        ; Get operation status
        LD	A, (NMICMD_RESPONSE)
        CP      STATUS_INPROGRESS
        JR	Z, _wait
        ;CP	STATUS_OK
        ;JR	Z, SHOWSUCCESS
	LD	HL, NMICMD_RESPONSE
        INC	HL	
	CALL	SHOWOPERRORMSG
        ; Clear
_leave:
        LD	A, $38
        CALL 	TEXTMESSAGE__CLEAR
        LD	HL, NMI_MENU
        CALL	MENU__DRAW
        RET
        ;RET
_showdefault:
	LD	HL, UNSPECIFIEDMSG
        CALL	TEXTMESSAGE__SHOW
        CALL	WAITFORENTER
        JR	_leave
_error1:
	


	ENDLESS
        JP 	INTERNALERROR
        RET

SHOWOPERRORMSG:
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
        JR	_l3
_showdefault:
	LD	HL, UNSPECIFIEDMSG
_l3:
	CALL    TEXTMESSAGE__SHOW
        JP	WAITFORENTER


; Returns NZ if we have a key
CHECKKEY:
	BIT	5,(IY+(FLAGS-IYBASE))	; test FLAGS  - has a new key been pressed ?
	RET	Z
	LD	DE, (CURKEY)
	LD	A, D
	RES	5,(IY+(FLAGS-IYBASE))	; update FLAGS  - reset the new key flag.
        DEC	A
        RET	Z
        LD	A, E
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

ASKFILENAME:
        ;CALL	RESTORESCREENAREA
	CALL 	SETUPASKFILENAME

_waitfilename:
	; Scanning loop
	CALL	KEYBOARD
        CALL	CHECKKEY
        JR	Z, _waitfilename
        
	LD	HL, FILENAMEENTRYWIDGET
        CALL	TEXTINPUT__HANDLEKEY
        CP	$FF
        JR	Z, _waitfilename
        
        ;CALL	RESTORESCREENAREA

        CP	0
        JP	Z, SNASAVE

        ; Redraw menu
       	LD	HL, NMI_MENU
        JP	MENU__DRAW

SETUPASKFILENAME:
	LD	IX, FILENAMEENTRYWIDGET
	LD	(IX+TEXTINPUT_OFF_MAX_LEN), 8
        LD	(IX+FRAME_OFF_WIDTH), 15
        LD      (IX+FRAME_OFF_TITLEPTR), LOW(FILENAMETITLE)
        LD      (IX+FRAME_OFF_TITLEPTR+1), HIGH(FILENAMETITLE)

	LD	HL, SNAFILENAME

	LD	(IX+TEXTINPUT_OFF_STRINGPTR), L
        LD	(IX+TEXTINPUT_OFF_STRINGPTR+1), H

        XOR	A
        LD	(HL), A ; Clear password

        LD	D, 7 ; line to display menu at.
	LD	HL, FILENAMEENTRYWIDGET
        CALL	TEXTINPUT__INIT
        LD	HL, FILENAMEENTRYWIDGET
	JP	TEXTINPUT__DRAW


	; Restore a screen area from previously saved data to external RAM
        ; DE:	pointer to screen address.
        ; TODO: this restores everything.
RESTORESCREENAREA:
	; Screen area starts at 0x2006
        ; Size: 0x1B00
        LD	A, $06
        OUT	(PORT_RAM_ADDR_0), A         ; Set external RAM LSB
        LD	A, $20
        OUT	(PORT_RAM_ADDR_1), A         ; and MSB addresses. So we start read/write at 0x0000
	LD	HL, SCREEN
	LD	B, 0
        LD	C, PORT_RAM_DATA
        LD	D, $1B
_r1:
        INIR                   	; Starts at 0x0006
        DEC	D
        JR	NZ, _r1
        RET
        

UNSPECIFIEDMSG: DB "Unspecified error", 0
FILENAMETITLE:  DB "Save name", 0
