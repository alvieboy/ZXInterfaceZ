include "port_defs.asm"
include "command_defs.asm"

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
        
        ; Save YM mixer status and clear sound.
        LD	D, C
        LD	BC, $FFFD
        LD	A, $07
        OUT	(C), A 	; Select register 0x07 (Mixer)
        IN	A, (C)
        LD	C, D            ; Back to RAM_DATA port
        ; Save mixer to Ram
        OUT	(C), A          ; Ram: 0x2006
        LD	BC, $BFFD
        LD	A, $FF
        OUT	(C), A    	; Clear Mixer status
        LD	C, D		; Back to RAM_DATA port 
        
        ; Reserve some space for future expansion.
        
        LD	A, $10
        OUT	(PORT_RAM_ADDR_0), A         ; Set external RAM LSB to 0x2010

        
        ; Save whole screen + scratch area to External RAM. 
        ; 8192 bytes (0x2000)
        LD	B, $00
        LD	D, $20
	LD	HL, SCREEN
lone:
        OTIR                   	; Starts at 0x2010
        DEC	D
        JR	NZ, lone
        ; Screen saved.
        
        ; Now, SP, IX, IY. Note that SP has 4 extra bytes for AF and NMI return address.
        
        LD	(NMI_SCRATCH), SP
        LD	HL, (NMI_SCRATCH)     ; HL=SP
        OUT	(C), L        	; Ram: 0x4010
        OUT	(C), H        	; Ram: 0x4011
        
        LD	A, IXL
        OUT	(C), A        	; Ram: 0x4012
        LD	A, IXH
        OUT	(C), A        	; Ram: 0x4013

        LD	A, IYL
        OUT	(C), A        	; Ram: 0x4014
        LD	A, IYH
        OUT	(C), A        	; Ram: 0x4015
        
        ; We don't touch alternative registers, so leave them alone for now.
        
        ; Do save AF, however. It's easier to just do it here.
        ; HL still contains SP value
        LD	A, (HL)
        OUT	(C), A          ; Ram: 0x4016 [flags]
        INC	HL
        LD	A, (HL)
        OUT	(C), A          ; Ram: 0x4017 [A]

        ; Get R 
        LD	A, R
        OUT	(C), A	   	; Ram: 0x4018

        LD	SP, NMI_SPVAL
        PUSH	AF
        
        ; Get flags from SP
        LD	HL, NMI_SPVAL-2
        LD	A, (HL)
        OUT	(C), A	   	; Ram: 0x4019
        POP	AF
        
        ; Finally, save I
        LD	A, I
        OUT	(C), A	   	; Ram: 0x401A
        
        ; 
        
        ; Save border.
        IN	A, (PORT_ULA)
        OUT	(C), A	   	; Ram: 0x401B
        DEBUGSTR "Serving NMI\n"

        CALL	NMIPROCESS_VIDEOONLY

NMIRESTORE:
	; Restore SP first, since we need to manipulate RAM 
        ; in order to restore it
        LD	C, PORT_RAM_DATA	       ; Data port 
        LD	A, $10		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        LD	A, $40          ; MSB
        OUT	(PORT_RAM_ADDR_1), A
        XOR	A
        OUT	(PORT_RAM_ADDR_2), A   ; Address: 0x004010
        
        IN	L, (C)          ; Ram: 0x4010
        IN	H, (C)          ; Ram: 0x4011
        LD	(NMI_SCRATCH), HL
        LD	SP, (NMI_SCRATCH)
        ; SP ready now.
        ; Restore IX, IY now
        IN	A, (C)          ; Ram: 0x4012
        LD	IXL, A
        IN	A, (C)          ; Ram: 0x4013
        LD	IXH, A
        IN	A, (C)          ; Ram: 0x4014
        LD	IYL, A
        IN	A, (C)
        LD	IYH, A          ; Ram: 0x4015
        
        ; Move back to screen area.
        LD	A, $10		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        LD	A, $20          ; MSB
        OUT	(PORT_RAM_ADDR_1), A     ; 0x2010
        ; Restore screen and scratchpad memory
        LD	B, $00
        LD	D, $20
        LD	E, A
	LD	HL, SCREEN
_l2:
        INIR                   	; Starts at 0x2010
        DEC	D
        JR	NZ, _l2
        
        ; Restore other regs

        LD	A, $06		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        LD	A, $20          ; MSB   
        OUT	(PORT_RAM_ADDR_1), A

	IN	E, (C)		; Mixer state in E
        ; Restore YM Mixer state
        LD	D, C		; Save RAM_DATA port
        LD	BC, $FFFD
        LD	A, $07
        OUT	(C), A
        LD	B, $BF
        OUT	(C), E		; Restore YM register 0x07

        LD	C, D		; Restore RAM_DATA port

	; Move pointer to 0x2001
        LD	A, $01		; LSB 
        OUT	(PORT_RAM_ADDR_0), A 
        ; BEDLH
       	IN	B,(C)           ; Ram: 0x2001
        IN	E,(C)           ; Ram: 0x2002
        IN	D,(C)           ; Ram: 0x2003
        IN	L,(C)           ; Ram: 0x2004
        IN	H,(C)           ; Ram: 0x2005
	; Last one is C.
        XOR	A
        OUT	(PORT_RAM_ADDR_0), A        ; Ram: 0x2000
        IN	C, (C)
        ; SP is "good" here.
	POP 	AF
        RETN
        

; Returns NZ if we have a key
CHECKKEY:
        LD	HL, FLAGS
	BIT	5,(HL)
	RET	Z
	LD	DE, (CURKEY)
	LD	A, D
	RES	5,(HL)
        DEC	A
        RET	Z
        LD	A, E
        RET
        
WAITFORENTER:
	CALL	KEYBOARD
        LD	HL, FLAGS
	BIT	5,(HL)
	JR	Z, WAITFORENTER
	LD	DE, (CURKEY)
	LD	A, D
	RES	5,(HL)
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
        



NMIPROCESS_VIDEOONLY:
 	LD	A, CMD_NMIREADY
        CALL	WRITECMDFIFO

        LD	C, PORT_RAM_ADDR_2
        LD	A, $02
        OUT	(C), A
        
        LD	A, 0
        LD	(FRAMES1), A

screenloop:

        ; Get sequence at 0x021B00
        LD	C, PORT_RAM_ADDR_1
        LD	A, $1B
        OUT	(C), A
        XOR	A
        LD	C, PORT_RAM_ADDR_0
        OUT	(C), A
        LD	C, PORT_RAM_DATA
	IN	A, (C)
        BIT	7, A
        JR 	NZ, _command

        ; Not a command. Compare with frame seq.
        LD	B, A
        LD	A, (FRAMES1)
        CP	B
        LD	A, B
        JR	NZ, _processvideo

        CALL	KEYBOARD
        LD	HL, FLAGS
        BIT	5, (HL)
        JR 	Z, _l1
        RES	5, (HL)
;        DEBUGHEXDE
        ; Send KBD update
        LD	A, CMD_KBDINPUT
        CALL	WRITECMDFIFO
        LD	DE, (CURKEY)
        LD	A, E
        CALL	WRITECMDFIFO
        LD	A, D        
        CALL	WRITECMDFIFO
        
_l1:    JR	screenloop
_command:
	DEBUGSTR "ROM: command "
        DEBUGHEXA
	CP	$FF
        JR	Z, _leavenmi
        CP	$FE
        JR	Z, _snapshot
        CP	$FD
        JR	Z, _z80sna
        CP	$FC
        JR	Z, _execandexit
        JR	screenloop
_leavenmi:
	LD	A, CMD_LEAVENMI
        CALL	WRITECMDFIFO

	; Wait for keys to be released.
        CALL	WAITFORNOKEY
        JP	WAITFORNOKEY ; TAILCALL

        ;RET
_execandexit:
	CALL	FREEAREA
        JR	_leavenmi
        
_z80sna:
	LD	A, CMD_LEAVENMI
        CALL	WRITECMDFIFO
	JP	RESTOREZ80VX
_snapshot:
	LD	A, CMD_LEAVENMI
        CALL	WRITECMDFIFO
        JP 	SNARAM
_processvideo:
	DEBUGSTR "Sequence "
        DEBUGHEXA
        LD	(FRAMES1), A

        LD	C, PORT_RAM_ADDR_0
        LD	A, $00
        OUT	(C), A
        LD	C, PORT_RAM_ADDR_1
        OUT	(C), A
        
        LD	HL, SCREEN

	;	6912 bytes. 216 blocks of 32 bytes, 108 blocks of 64 bytes, 54 blocks of 128 bytes.
        LD	A, 54
        LD	C, PORT_RAM_DATA
_loop1:
        REPT	128                 	; T16, T2048 total
          INI
        ENDM
        DEC	A     			; T4
        JP	NZ, _loop1              ; T10   (sum 111348)
       	JP 	screenloop
        
