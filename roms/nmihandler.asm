include "port_defs.asm"
include "command_defs.asm"

; ENTRY point when we are already runnning ROM.
NMIHANDLER_FROM_ROM:
	LD	A, $02
        JR	NMIHANDLER1
        
; ENTRY point when we are called from physical NMI
NMIHANDLER_FROM_NMIREQUEST:
	; From NMI, we already saved AF.
        ; We need to understand if we had interrupts enabled prior to entering NMI.
        ; This is because we need to manipulate IFFx later on, and by doing that we
        ; destroy IFF2 contents.
        LD	A, R
        LD	A, 0
        ; At this point, IFF2 is in parity flag
        JP	PO, _iff_not_enabled
        INC	A 	; Set bit 0, indicating we need to re-enable interrupts
_iff_not_enabled:
        
NMIHANDLER1:
	; At this point A contains:
	; bit 0: interrupts enabled, need to be restored.
        ; bit 1: "soft" call, do not use RETN
        
        ; This is NOT reentrant! We need to figure out a way to
        ; bring up the menu while other menu is running.
        
        OUT     (PORT_SCRATCH0), A
	; Upon entry we already have AF on the stack.
        XOR	A
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
        
        ; Fetch PC from stack, used for debugging
        
        POP	DE ; AF
        ;INC	SP
        ;INC	SP
        POP	HL ; PC
        PUSH	HL
        ;DEC	SP
        ;DEC	SP
        PUSH	DE
        OUT	(C), L          ; Ram: 0x2007
        OUT	(C), H          ; Ram: 0x2008
        
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
        
        ; Get flags from SP. THIS IS WRONG!
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
        
        ; For debugging purposes, save alternate registers
        EX	AF, AF'
        PUSH	AF
        POP	DE
        OUT	(C), E   ; F'   ; Ram: 0x401C
        OUT	(C), D   ; A'   ; Ram: 0x401D
        EX	AF, AF'
        
        EXX
        LD	A, C
        LD	C, PORT_RAM_DATA
        OUT	(C), A	; C'   ; Ram: 0x401E
        OUT	(C), B  ; B'   ; Ram: 0x401F
        OUT	(C), E	; E'   ; Ram: 0x4020
        OUT	(C), D  ; D'   ; Ram: 0x4021
        OUT	(C), L	; L'   ; Ram: 0x4022
        OUT	(C), H  ; H'   ; Ram: 0x4023
        LD	C, A ; Restore C, so we don't mess with it.
        EXX
        
        ; Enable interrupts so we can sync screen updates
        EI
        
;        DEBUGSTR "Serving NMI\n"
        CALL	NMIPROCESS_VIDEOONLY

NMIRESTORE:

	DI
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
	
        ; Before we leave, check if we ought to return with RETN or regular RET
        IN	A, (PORT_SCRATCH0)
        BIT	1, A
        JR	NZ, _return_with_ret
        BIT	0, A
        JR	NZ, _retn_with_ei


        ; Last one is C.
        XOR	A
        OUT	(PORT_RAM_ADDR_0), A        ; Ram: 0x2000
        IN	C, (C)
        ; SP is "good" here.
	POP 	AF
        DI	; Clear IFF1 AND IFF2
        RETN
_retn_with_ei;
        ; Last one is C.
        XOR	A
        OUT	(PORT_RAM_ADDR_0), A        ; Ram: 0x2000
        IN	C, (C)
        ; SP is "good" here.
	POP 	AF
        EI	; Set IFF1 AND IFF2
        RETN
        
_return_with_ret:
        BIT	0, A
        JR	NZ, _ret_with_ei

        XOR	A
        OUT	(PORT_RAM_ADDR_0), A        ; Ram: 0x2000
        IN	C, (C)
	; For RET, we did not save AF. We already have interrupts disabled.
        RET
_ret_with_ei:
        XOR	A
        OUT	(PORT_RAM_ADDR_0), A        ; Ram: 0x2000
        IN	C, (C)
	; For RET, we did not save AF. 
        EI
        RET
        

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
	; Clear command immediatly.
        LD	B, A
        XOR	A
        LD	C, PORT_RAM_ADDR_0
        OUT	(C), A
        LD	C, PORT_RAM_DATA
	OUT	(C), A
        LD	A, B
        
	;DEBUGSTR "ROM: command "
        ;DEBUGHEXA
	CP	$FF
        JR	Z, _leavenmi
        CP	$FE
        JR	Z, _snapshot
        CP	$FD
        JR	Z, _z80sna
        CP	$FC
        JR	Z, _execandexit
        CP 	$FB
        JP	Z, _readmem
        CP 	$FA
        JP	Z, _readrom
        CP 	$F9
        JP	Z, _tapfiletophysical
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
	;DEBUGSTR "Sequence "
        ;DEBUGHEXA
        ; Wait for next interrupt
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

	; Sync to interrupt
        HALT

_loop1:
        REPT	128                 	; T16, T2048 total
          INI
        ENDM
        DEC	A     			; T4
        JP	NZ, _loop1              ; T10   (sum 111348)
       	JP 	screenloop
        
_readmem:
	; Read len, pointer. 
	IN	B, (C)
	IN	L, (C)
        IN	H, (C)
	LD	A, CMD_MEMDATA
        CALL	WRITECMDFIFO
        LD	A, B
        CALL	WRITECMDFIFO
_memread:
        LD	A, (HL)
        INC	HL
        CALL	WRITECMDFIFO
        DJNZ	_memread
        JP	screenloop

_readrom:
	; We need to copy the readrom routine to RAM
        ; For this, we use a free area which we already saved past the
        ; screen area. Also used for ROM calculation (ROMCRC_RAM)
        LD	A, C
        LD	DE, ROMCRC_RAM
        LD	HL, READROM_ROM
        LD	BC, READROM_ROM_SIZE
        LDIR

        LD	C, A
	; Read len, pointer. 
	IN	B, (C)
	IN	L, (C)
        IN	H, (C)

	LD	A, CMD_MEMDATA
        CALL	WRITECMDFIFO
        LD	A, B
        CALL	WRITECMDFIFO

        ; Call it
        CALL	ROMCRC_RAM

        JP	screenloop
_tapfiletophysical:
	; Convert a TAP/TZX file into normal tape.
        ; 
        ; For this to work, we read 
        ; 00000_010 - 00001_101
        ; 00001_110 - 00000_001 
        ;

	; Ultra-fast version
      	DI
        ; Save border so we can restore later
        IN	A, ($FE)
        AND	$07
        LD	B, A
        ;
        LD	C, PORT_VIRTUALAUDIO
        ; First, wait until tape is playing.
_waitfortape:
	IN	A, (C)	; Flags updated accordingly
        JR	Z, _waitfortape
_loop:
	IN	A, (C)	; Flags updated accordingly
        OUT	($FE), A
	JR 	NZ, _loop
        ; Duplication complete, restore border
        ; Do not disturb other bits except border, otherwise
        ; we can cause glitches int the audio
        ;
        AND	$F8 	; Remove border, keep other bits.
        OR	B
        OUT	($FE), A
        EI
        RET

_nextbit:
	LD	A,$7F		; test the space key and
	IN	A,($FE)		; 
        
        RRA			; if a space is pressed
	JR	NC, _break	; return to SA_LD_RET - - >
        RLA
        ; Rotate bit into carry
        LD	C, $0E
        RLA
        RLA
        LD	A, C
        JR	NC, _noinv
	XOR	$0F
_noinv:	
        OUT	($FE), A
        JR	_nextbit
_break:
	RET

READROM_ROM:
        LD	A, $02
        OUT	(PORT_NMIREASON), A 	; Temporarly disable ROMCS

_memread_rom_ram:
        LD	A, (HL)
        INC	HL
        
	LD	C, A
_WAITFIFO1:
        IN 	A, (PORT_CMD_FIFO_STATUS)
        OR	A
        JR	NZ, _WAITFIFO1
        ; Send resource ID
        LD	A, C
        OUT	(PORT_CMD_FIFO_DATA), A

        DJNZ	_memread_rom_ram
        
	LD	A, $00
        OUT	(PORT_NMIREASON), A
        RET ; Return to ROM
        
READROM_ROM_SIZE EQU $ - READROM_ROM

        