ZXZ_SYSCALL_GETCWD              EQU 	192
ZXZ_SYSCALL_CHDIR               EQU 	193
ZXZ_SYSCALL_OPEN                EQU 	194
ZXZ_SYSCALL_CLOSE               EQU 	195
ZXZ_SYSCALL_READ                EQU 	196
ZXZ_SYSCALL_WRITE               EQU 	197
ZXZ_SYSCALL_OPENDIR             EQU 	198
ZXZ_SYSCALL_READDIR             EQU 	199
ZXZ_SYSCALL_CLOSEDIR            EQU 	200
ZXZ_SYSCALL_SOCKET              EQU 	201
ZXZ_SYSCALL_CONNECT             EQU 	202
ZXZ_SYSCALL_SENDTO              EQU 	203
ZXZ_SYSCALL_RECVFROM            EQU 	204
ZXZ_SYSCALL_GETHOSTBYNAME       EQU 	205
;ZXZ_SYSCALL_GETHOSTBYADDR       EQU 	206
ZXZ_SYSCALL_STRERROR            EQU 	207


SYSCALLTABLE:
	DW	SYSCALL_GETCWD                ; C0
        DW	SYSCALL_CHDIR                 ; C1
        DW	SYSCALL_OPEN                  ; C2
        DW	SYSCALL_CLOSE                 ; C3
        DW	SYSCALL_READ                  ; C4
        DW	SYSCALL_WRITE                 ; C5
        DW	SYSCALL_OPENDIR               ; C6
        DW	SYSCALL_READDIR               ; C7
        DW	SYSCALL_CLOSEDIR              ; C8
        DW	SYSCALL_SOCKET                ; C9
        DW	SYSCALL_CONNECT               ; CA
        DW	SYSCALL_SENDTO                ; CB
        DW	SYSCALL_RECVFROM              ; CC
        DW	SYSCALL_GETHOSTBYNAME         ; CD
        DW	SYSCALL_NOTIMPLEMENTED        ; CE
        DW	SYSCALL_NOTIMPLEMENTED        ; CF
        
        DW	SYSCALL_STRERROR              ; D0
        DW	SYSCALL_NOTIMPLEMENTED        ; D1
        DW	SYSCALL_NOTIMPLEMENTED        ; D2
        DW	SYSCALL_NOTIMPLEMENTED        ; D3
        DW	SYSCALL_NOTIMPLEMENTED        ; D4
        DW	SYSCALL_NOTIMPLEMENTED        ; D5
        DW	SYSCALL_NOTIMPLEMENTED        ; D6
        DW	SYSCALL_NOTIMPLEMENTED        ; D7
        DW	SYSCALL_NOTIMPLEMENTED        ; D8
        DW	SYSCALL_NOTIMPLEMENTED        ; D9
        DW	SYSCALL_NOTIMPLEMENTED        ; DA
        DW	SYSCALL_NOTIMPLEMENTED        ; DB
        DW	SYSCALL_NOTIMPLEMENTED        ; DC
        DW	SYSCALL_NOTIMPLEMENTED        ; DD
        DW	SYSCALL_NOTIMPLEMENTED        ; DE
        DW	SYSCALL_NOTIMPLEMENTED        ; DF

	MACRO	CHECK_NEGATIVE
        LD	C,A
        RLC	C
        ENDM
;
; SOCKET syscall
;

; Inputs:
; 	B: Socket type (0x06 TCP, 0x11 UDP)
; Returns:
;	A: File handle. "C" is set if error

SYSCALL_SOCKET:
	PUSH	BC
        LD	A, CMD_SOCKET
        CALL	WRITECMDFIFO
        LD	A, B
        CALL 	WRITECMDFIFO
        CALL 	READRESFIFO
        CHECK_NEGATIVE
POPBCRET:
	POP	BC
        JP	RETTOMAINROM

;
; GETHOSTBYNAME syscall
;
; Inputs:
;	DE: Hostname to lookup
; 	HL: IP address
; Returns:
;	A: 0 on success. "C" is set if error
;	HL and DE are not preserved

SYSCALL_GETHOSTBYNAME:
	PUSH	BC
        LD	A, CMD_GETHOSTBYNAME
        CALL	WRITECMDFIFO
        EX	DE, HL
        CALL	WRITECMDSTRING ; In HL
        CALL	READRESFIFO
        CHECK_NEGATIVE
        JR	C, POPBCRET
        ; Read 4 bytes with IP address
        LD	B, 4
_nextoctet:
        CALL	READRESFIFO
        LD	(DE),A 
        INC 	DE
        DJNZ	_nextoctet
        XOR	A
        JR	POPBCRET
;
; CONNECT syscall
;
; Inputs:
;	B: 	file handle
;	HL:	IPv4 address
;	DE:	port 
; Returns:
;       A: 0 on success. "C" is set if error
;	HL is not preserved

SYSCALL_CONNECT:
	PUSH	BC
        LD	A, CMD_CONNECT
        CALL 	WRITECMDFIFO
        LD	A, B
        CALL 	WRITECMDFIFO
        LD	A, (HL)
        INC	HL
        CALL 	WRITECMDFIFO
        LD	A, (HL)
        INC	HL
        CALL 	WRITECMDFIFO
        LD	A, (HL)
        INC	HL
        CALL 	WRITECMDFIFO
        LD	A, (HL)
        CALL 	WRITECMDFIFO
        LD	A, E
        CALL 	WRITECMDFIFO
        LD	A, D
        CALL 	WRITECMDFIFO
        CALL	READRESFIFO
        CHECK_NEGATIVE
        JR	POPBCRET

SYSCALL_READ:
	PUSH	BC
        ;
        ; DE holds size to read. The syscall only reads
        ; up to 127 bytes, because we cannot buffer the whole
        ; read inside CPU and we need room to return error
        ; results
        
        PUSH	DE	; Save requested size

_loadmore:
	; Send command and file descriptor
        LD	A, CMD_READ
        CALL 	WRITECMDFIFO
        LD	A, B
        CALL 	WRITECMDFIFO

        ; A = DE>127 ? 127 : (D)E
        
        LD	A, D
        OR	A
        JR 	NZ, _saturate 	; D is not zero, jump
        LD	A, E
        CP	128
        JR	C, _no_saturate	; Less than 128, jump
_saturate:
	LD	A, 127
_no_saturate:

	; At this point, we have in A the amount of data we can read

	CALL	WRITECMDFIFO
        ; Read out result
        CALL	READRESFIFO
        CHECK_NEGATIVE
        JR	C, READERROR	; Error result
        
        ; If we read zero, then EOF
        OR	A
        JR	Z, READEOF
        CALL	READRESTOBUFFER
        
	; DE now holds the amount of data we still need to read.
        ; TODO: for now we do not read any more data
        ;LD	A, D
        ;;OR	E
        ;JR 	NZ, _loadmore

	; All done.
READEOF:
	; Calculate amount of bytes read
        EX	(SP),HL ; This was DE saved upon entry
        AND	A
        SBC	HL, DE  ; Subtract the actual number of bytes read
        EX	DE, HL	; Back in DE
        ; Clear carry
        XOR	A
        POP	HL 	; Get back pointer.
READERROR:
        POP	BC
        JP	RETTOMAINROM


;
; CLOSE syscall
;
; Inputs:
;	B: 	file handle
; Returns:
;       A: 0 on success. "C" is set if error

SYSCALL_CLOSE:
	PUSH	BC
        LD	A, CMD_CLOSE
        CALL	WRITECMDFIFO
        LD	A, B
        CALL 	WRITECMDFIFO
        CALL 	READRESFIFO
        CHECK_NEGATIVE
        JR	READERROR

;
; STRERROR syscall
;
; Inputs:
;	E: 	Error
;	HL:	Target memory address
;	B: 	Max size
; Returns:
;       A: 0 on success. "C" is set if error
;

SYSCALL_STRERROR:
	PUSH	BC
        LD	A, CMD_STRERROR
        CALL	WRITECMDFIFO
        LD	A, E
        CALL	WRITECMDFIFO
        LD	A, B
        CALL 	WRITECMDFIFO
        CALL 	READRESFIFO
        CHECK_NEGATIVE
        JR	C, READERROR
        CALL	READRESTOBUFFER
        POP	BC
        JP 	RETTOMAINROM
	

SYSCALL_GETCWD:
SYSCALL_CHDIR:
SYSCALL_OPEN:
SYSCALL_WRITE:
SYSCALL_OPENDIR:
SYSCALL_READDIR:
SYSCALL_CLOSEDIR:
SYSCALL_SENDTO:
SYSCALL_RECVFROM:
SYSCALL_GETHOSTBYADDR:
SYSCALL_NOTIMPLEMENTED:
	LD	A, -14 ; EFAULT
        CHECK_NEGATIVE
        JP	RETTOMAINROM
