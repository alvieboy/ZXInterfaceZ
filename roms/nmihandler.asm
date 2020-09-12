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
        CALL	Screen__redraw
        
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
        LD	IX, FileWindow_INST
        LD	DE, $1C0F ; width=28, height=8

        CALL	FileDialog__CTOR
        
	LD	HL, FileWindow_INST
        LD	IX, Screen_INST
        DEBUGSTR "Adding file window\n"
        
        CALL 	Screen__addWindowCentered
	
        
        LD	IX, FileWindow_INST
        LD	A, 1
        VCALL	Widget__setVisible

        POP	IX
	RET
;ASKFILENAME:
TBD:
LOADTAPE:
NMIENTRY4HANDLER:
NMIENTRY6HANDLER:
Wrap_close_mainwindow:
	DEBUGSTR "Called!!"
	RET

NMIMENU_CALLBACK_TABLE:
	DEFW LOADSNAPSHOT       ; Load snapshot
        DEFW TBD        ; Save snapshot
        DEFW LOADTAPE           ; Play tape
        DEFW NMIENTRY4HANDLER	; Poke
        DEFW Settings__show	; Settings
        DEFW NMIENTRY6HANDLER	; Reset
        DEFW Widget__destroyParent
