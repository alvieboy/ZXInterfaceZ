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
 ;       DEBUGSTR "Serving NMI\n"

        CALL	NMIPROCESS_VIDEOONLY

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
        LD	(FLAGS),A
        
        CALL	ALLOC__INIT
        ; Create menu window


        LD	HL, Screen_INST ; Need to set up parent pointer in advance
        
        
        LD	IX, MainWindow_INST
        LD	DE, $1C09 ; width=28, height=8
        CALL	Menuwindow__CTOR
        
        LD	HL, MAINWINDOWTITLE
        CALL	Window__setTitle
        
        ; Set menu entries
        LD	A,  7
        LD	HL, MAINMENU_ENTRIES
        CALL	Menuwindow__setEntries
        
        LD	HL, NMIMENU_CALLBACK_TABLE
        CALL	Menuwindow__setCallbackTable
        
        PUSH	IX
        POP	HL
        
        ; Instantiate main screen @Screen_INST
        LD	IX, Screen_INST
        CALL	Screen__CTOR
        ; Add window to screen (window in HL), centered.
        CALL 	Screen__addWindowCentered

        LD	IX, MainWindow_INST
        LD	A, 1
        VCALL	Widget__setVisible
        
        LD	IX, Screen_INST
        ;CALL	Screen__redraw
        
        CALL	Screen__eventLoop
        
        JP	WAITFORNOKEY






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
        

UNSPECIFIEDMSG: DB "Unspecified error", 0
FILENAMETITLE:  DB "Save name", 0

MAINWINDOWTITLE:
	DB 	"ZX Interface Z", 0


NMIENTRY1: DB	"Load snapshot..", 0
NMIENTRY2: DB	"Save snapshot..", 0
NMIENTRY3: DB	"Play tape...", 0
NMIENTRY4: DB	"Poke...",0
NMIENTRY5: DB	"Settings...", 0
NMIENTRY6: DB	"Reset", 0
NMIENTRY7: DB	"Exit", 0

MAINMENU_ENTRIES:
	DB	0
        DW	NMIENTRY1
	DB	0
        DW	NMIENTRY2
	DB	0
        DW	NMIENTRY3
	DB	0
        DW	NMIENTRY4
	DB	0
        DW	NMIENTRY5
	DB	0
        DW	NMIENTRY6
	DB	0
        DW	NMIENTRY7


LOADSNAPSHOT:
	PUSH	IX
        LD	HL, REQUEST_SNAPSHOT
        LD	A, FILE_FILTER_SNAPSHOTS
        CALL	LOADFILE
        POP	IX
        RET

LOADTAPE:
	PUSH	IX
        LD	HL, REQUEST_TAPE
        LD	A, FILE_FILTER_TAPES
        CALL	LOADFILE
        POP	IX
        RET
        
REQUEST_TAPE:
        LD 	A, CMD_PLAY_TAPE
        CALL	WRITECMDFIFO
        ; String still in HL
        CALL	WRITECMDSTRING
        ; Close all widgets and return
        LD	IX, Screen_INST
        JP	Screen__closeAll
        
LOADFILE:
	PUSH	HL
        PUSH	AF
        
        LD	IX, FileWindow_INST
        LD	DE, $1C0F ; width=28, height=8
        LD	HL, Screen_INST

        CALL	FileDialog__CTOR
        
	LD	HL, FileWindow_INST
        LD	IX, Screen_INST
        CALL 	Screen__addWindowCentered
	
        
        LD	IX, FileWindow_INST
        LD	A, 1
        VCALL	Widget__setVisible
	POP	AF
	; A comes from parameter        
	CALL	FileDialog__setFilter

        LD	IX, FileWindow_INST
	CALL	Dialog__exec
        JR	NZ, _canceled
        
        LD	IX, FileWindow_INST
        CALL	FileDialog__getSelectionString
        PUSH	HL
        CALL	FileDialog__DTOR
        POP	HL
        
        RET 	; JP	(HL)
_canceled:
	POP	HL
        JP	FileDialog__DTOR

        
REQUEST_SNAPSHOT:
	; Request load snapshot
        LD 	A, CMD_LOAD_SNA
        CALL	WRITECMDFIFO
        ; String still in HL
        CALL	WRITECMDSTRING
        ;
        ; Wait for completion
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
        JP	Z, SNARAM
        
        LD	HL, NMICMD_RESPONSE
        INC	HL	
        ;JP	SHOWOPERRORMSG
        ENDLESS
_error1:
	JP	INTERNALERROR
	ENDLESS

REQUESTRESET:
	; Request RESET from FPGA
        DI
        LD   	A, CMD_RESET
        CALL 	WRITECMDFIFO
	ENDLESS
        

NMIPROCESS_VIDEOONLY:
	DI
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
        JR	screenloop
_leavenmi:
	LD	A, CMD_LEAVENMI
        CALL	WRITECMDFIFO
        RET
_processvideo:
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
        RET
        
;ASKFILENAME:
TBD:
NMIENTRY4HANDLER:
NMIENTRY6HANDLER:
Wrap_close_mainwindow:
	LD	IX, Screen_INST
        JP	Screen__closeAll

NMIMENU_CALLBACK_TABLE:
	DEFW LOADSNAPSHOT       ; Load snapshot
        DEFW TBD        	; Save snapshot
        DEFW LOADTAPE           ; Play tape
        DEFW NMIENTRY4HANDLER	; Poke
        DEFW Settings__show	; Settings
        DEFW REQUESTRESET	; Reset
        DEFW Wrap_close_mainwindow
