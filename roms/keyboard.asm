;------------------
; Keyboard scanning
;------------------
; from keyboard and S_INKEY
; returns 1 or 2 keys in DE, most significant shift first if any
; key values 0-39 else 255

					;;;$028E
KEY_SCAN:	LD	L,$2F		; initial key value
					; valid values are obtained by subtracting
					; eight five times.
		LD	DE,$FFFF	; a buffer to receive 2 keys.
		LD	BC,$FEFE	; the commencing port address
					; B holds 11111110 initially and is also
					; used to count the 8 half-rows
					;;;$0296
KEY_LINE:	IN	A,(C)		; read the port to A - bits will be reset
					; if a key is pressed else set.
		CPL			; complement - pressed key-bits are now set
		AND	$1F		; apply 00011111 mask to pick up the
					; relevant set bits.
		JR	Z,KEY_DONE	; forward to KEY_DONE if zero and therefore
					; no keys pressed in row at all.

		LD	H,A		; transfer row bits to H
		LD	A,L		; load the initial key value to A

					;;;$029F
KEY_3KEYS:	INC	D		; now test the key buffer
		RET	NZ		; if we have collected 2 keys already
					; then too many so quit.

					;;;$02A1
KEY_BITS:	SUB	$08		; subtract 8 from the key value
					; cycling through key values (top = $27)
					; e.g. 2F>  27>1F>17>0F>07
					;      2E>  26>1E>16>0E>06
		SRL	H		; shift key bits right into carry.
		JR	NC,KEY_BITS	; back to KEY_BITS if not pressed
					; but if pressed we have a value (0-39d)
		LD	D,E		; transfer a possible previous key to D
		LD	E,A		; transfer the new key to E
		JR	NZ,KEY_3KEYS	; back to KEY_3KEYS if there were more
					; set bits - H was not yet zero.

					;;;$02AB
KEY_DONE:	DEC	L		; cycles 2F>2E>2D>2C>2B>2A>29>28 for
					; each half-row.
		RLC	B		; form next port address e.g. FEFE > FDFE
		JR	C,KEY_LINE	; back to KEY_LINE if still more rows to do.

		LD	A,D		; now test if D is still FF ?
		INC	A		; if it is zero we have at most 1 key
					; range now $01-$28  (1-40d)
		RET	Z		; return if one key or no key.

		CP	$28		; is it capsshift (was $27) ?
		RET	Z		; return if so.

		CP	$19		; is it symbol shift (was $18) ?
		RET	Z		; return also

		LD	A,E		; now test E
		LD	E,D		; but first switch
		LD	D,A		; the two keys.
		CP	$18		; is it symbol shift ?
		RET			; return (with zero set if it was).
					; but with symbol shift now in D

;-------------------------------
; Scan keyboard and decode value
;-------------------------------
; from interrupt 50 times a second

					;;;$02BF
KEYBOARD:	CALL	KEY_SCAN	; routine KEY_SCAN
		RET	NZ		; return if invalid combinations

                LD	HL,(PREVKEY)    ; Load previous key
                                        ; Compare with new key
                OR	A		; Clear "C" flag 
		SBC	HL, DE
                LD	A, H
                OR	L
                RET	Z		; Same key
                LD	(PREVKEY), DE
					; Notify key pressed
                LD	(CURKEY), DE
                LD	IY, IYBASE
                SET	5, (IY+(FLAGS-IYBASE))
                RET

	; Input: DE
KEYTOASCII:
	LD	C, 0
	LD	A, D
        CP	$27 ; Caps shift
        JR 	NZ, _nocapsshift
        ; Caps key on.
        LD	L, LOW(CAPSSHIFT_KEYS)
        LD	H, HIGH(CAPSSHIFT_KEYS)
        JR	_l1
_nocapsshift:
        CP 	$18 ; Symbol shift
        JR	Z, _symbolshift
        ; Otherwise, normal
_regularkey:
        ; Regular key
        LD	L, LOW(NORMAL_KEYS)
        LD	H, HIGH(NORMAL_KEYS)
        ; Make sure we do not overflow table
_l1:    LD	A, E
	CP	$FF
        JR	Z, _invalidkey      
	CP	$27
        JR	NC, _invalidkey
        ADD_HL_A
        LD	A,(HL)
        RET
_invalidkey:
	LD	A, $FF
        RET
_symbolshift:
	LD	L, LOW(SYM_KEYS)
        LD	H, HIGH(SYM_KEYS)
        JR	_l1

CAPSSHIFT_KEYS:	DEFB	$42		; B
		DEFB	$48		; H
		DEFB	$59		; Y
		DEFB	$36		; 6
		DEFB	$35		; 5
		DEFB	$54		; T
		DEFB	$47		; G
		DEFB	$56		; V
		DEFB	$4E		; N
		DEFB	$4A		; J
		DEFB	$55		; U
		DEFB	$37		; 7
		DEFB	$34		; 4
		DEFB	$52		; R
		DEFB	$46		; F
		DEFB	$43		; C
		DEFB	$4D		; M
		DEFB	$4B		; K
		DEFB	$49		; I
		DEFB	$38		; 8
		DEFB	$33		; 3
		DEFB	$45		; E
		DEFB	$44		; D
		DEFB	$58		; X
		DEFB	$FF		; SYMBOL SHIFT
		DEFB	$4C		; L
		DEFB	$4F		; O
		DEFB	$39		; 9
		DEFB	$32		; 2
		DEFB	$57		; W
		DEFB	$53		; S
		DEFB	$5A		; Z
		DEFB	$1B		; SPACE/ BREAK (ESC)
		DEFB	$FF		; ENTER
		DEFB	$50		; P
		DEFB	$08		; 0 - Backspace/DELETE
		DEFB	$31		; 1
		DEFB	$51		; Q
		DEFB	$41		; A

NORMAL_KEYS:	DEFB	$62		; B
		DEFB	$68		; H
		DEFB	$79		; Y
		DEFB	$36		; 6
		DEFB	$35		; 5
		DEFB	$74		; T
		DEFB	$67		; G
		DEFB	$76		; V
		DEFB	$6E		; N
		DEFB	$6A		; J
		DEFB	$75		; U
		DEFB	$37		; 7
		DEFB	$34		; 4
		DEFB	$72		; R
		DEFB	$66		; F
		DEFB	$63		; C
		DEFB	$6D		; M
		DEFB	$6B		; K
		DEFB	$69		; I
		DEFB	$38		; 8
		DEFB	$33		; 3
		DEFB	$65		; E
		DEFB	$64		; D
		DEFB	$78		; X
		DEFB	$FF		; SYMBOL SHIFT
		DEFB	$6C		; L
		DEFB	$6F		; O
		DEFB	$39		; 9
		DEFB	$32		; 2
		DEFB	$77		; W
		DEFB	$73		; S
		DEFB	$7A		; Z
		DEFB	$20		; SPACE
		DEFB	$0D		; ENTER
		DEFB	$70		; P
		DEFB	$30		; 0 
		DEFB	$31		; 1
		DEFB	$71		; Q
		DEFB	$61		; A

SYM_KEYS:	DEFB	'*'		; B
		DEFB	$FF		; H
		DEFB	'['		; Y EXTENDED
		DEFB	'&'		; 6
		DEFB	'%'		; 5
		DEFB	'>'		; T
		DEFB	'}'		; G EXTENDED
		DEFB	'/'		; V
		DEFB	','		; N
		DEFB	'-'		; J
		DEFB	']'		; U EXTENDED
		DEFB	$27		; 7
		DEFB	'$'		; 4
		DEFB	'<'		; R
		DEFB	'{'		; F EXTENDED
		DEFB	'?'		; C
		DEFB	'.'		; M
		DEFB	'+'		; K
		DEFB	$FF		; I
		DEFB	'('		; 8
		DEFB	'#'		; 3
		DEFB	$FF		; E
		DEFB	'\'		; D EXTENDED
		DEFB	$FF		; X  !!!!!WARNING!!!!!! - not using 163. Might crash
		DEFB	$FF		; SYMBOL SHIFT
		DEFB	'='		; L
		DEFB	';'		; O
		DEFB	')'		; 9
		DEFB	'@'		; 2
		DEFB	$FF		; W
		DEFB	'|'		; S EXTENDED
		DEFB	':'		; Z
		DEFB	$20		; SPACE
		DEFB	$0D		; ENTER
		DEFB	'"'		; P
		DEFB	$08		; 0 - Backspace/DELETE
		DEFB	'!'		; 1
		DEFB	$FF		; Q
		DEFB	'~'		; A EXTENDED


