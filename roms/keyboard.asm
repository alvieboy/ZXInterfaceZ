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
                ; IY might be changed outside interrupt.
                LD	IY, IYBASE
                SET	5, (IY+(FLAGS-IYBASE))
                RET

