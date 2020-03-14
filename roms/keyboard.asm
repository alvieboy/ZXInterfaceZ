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

					; then decrease the counters within the two key-state maps
					; as this could cause one to become free.
					; if the keyboard has not been pressed during the last five interrupts
					; then both sets will be free.


		LD	HL,KSTATE_0	; point to KSTATE_0

					;;;$02C6
K_ST_LOOP:	BIT	7,(HL)		; is it free ?  ($FF)
		JR	NZ,K_CH_SET	; forward to K_CH_SET if so

		INC	HL		; address 5-counter
		DEC	(HL)		; decrease counter
		DEC	HL		; step back
		JR	NZ,K_CH_SET	; forward to K_CH_SET if not at end of count

		LD	(HL),$FF	; else mark it free.

					;;;$02D1
K_CH_SET:	LD	A,L		; store low address byte.
		LD	HL,KSTATE_4	; point to KSTATE_4
					; (ld l, $04)
		CP	L		; have 2 been done ?
		JR	NZ,K_ST_LOOP	; back to K_ST_LOOP to consider this 2nd set

					; now the raw key (0-38) is converted to a main key (uppercase).

		CALL	K_TEST		; routine K_TEST to get main key in A
		RET	NC		; return if single shift

		LD	HL,KSTATE_0	; point to KSTATE_0
		CP	(HL)		; does it match ?
		JR	Z,K_REPEAT	; forward to K_REPEAT if so

					; if not consider the second key map.

		EX	DE,HL		; save KSTATE_0 in DE
		LD	HL,KSTATE_4	; point to KSTATE_4
		CP	(HL)		; does it match ?
		JR	Z,K_REPEAT	; forward to K_REPEAT if so

					; having excluded a repeating key we can now consider a new key.
					; the second set is always examined before the first.

		BIT	7,(HL)		; is it free ?
		JR	NZ,K_NEW	; forward to K_NEW if so.

		EX	DE,HL		; bring back KSTATE_0
		BIT	7,(HL)		; is it free ?
		RET	Z		; return if not.
					; as we have a key but nowhere to put it yet.

					; continue or jump to here if one of the buffers was free.

					;;;$02F1
K_NEW:		LD	E,A		; store key in E
		LD	(HL),A		; place in free location
		INC	HL		; advance to interrupt counter
		LD	(HL),$05	; and initialize to 5
		INC	HL		; advance to delay
		LD	A,(REPDEL)	; pick up system variable REPDEL
		LD	(HL),A		; and insert that for first repeat delay.
		INC	HL		; advance to last location of state map.
		LD	C,(IY+$07)	; pick up MODE  (3 bytes)
		LD	D,(IY+$01)	; pick up FLAGS (3 bytes)
		PUSH	HL		; save state map location
					; Note. could now have used.
 					; ld l,$41; ld c,(hl); ld l,$3B; ld d,(hl).
					; six and two threes of course.
		CALL	K_DECODE	; routine K_DECODE
		POP	HL		; restore map pointer
		LD	(HL),A		; put decoded key in last location of map.

					;;;$0308
K_END:		LD	(LASTK),A	; update LASTK system variable.
		SET	5,(IY+$01)	; update FLAGS - signal new key.
		RET			; done

;-------------------
; Repeat key routine
;-------------------
; A possible repeat has been identified. HL addresses the raw (main) key.
; The last location holds the decoded key (from the first context).

					;;;$0310
K_REPEAT:	INC	HL		; advance
		LD	(HL),$05	; maintain interrupt counter at 5
		INC	HL		; advance
		DEC	(HL)		; decrease REPDEL value.
		RET	NZ		; return if not yet zero.

		LD	A,(REPPER)	; REPPER
		LD	(HL),A		; but for subsequent repeats REPPER will be used.
		INC	HL		; advance
		LD	A,(HL)		; pick up the key decoded possibly in another context.
		JR	K_END		; back to K_END

;---------------
; Test key value
;---------------
; also called from S_INKEY
; begin by testing for a shift with no other.

					;;;$031E
K_TEST:		LD	B,D		; load most significant key to B
					; will be $FF if not shift.
		LD	D,$00		; and reset D to index into main table
		LD	A,E		; load least significant key from E
		CP	$27		; is it higher than 39d	i.e. FF
		RET	NC		; return with just a shift (in B now)

		CP	$18		; is it symbol shift ?
		JR	NZ,K_MAIN	; forward to K_MAIN if not

					; but we could have just symbol shift and no other

		BIT	7,B		; is other key $FF (ie not shift)
		RET	NZ		; return with solitary symbol shift

					;;;$032C
K_MAIN:		LD	HL,MAIN_KEYS	; address: MAIN_KEYS
		ADD	HL,DE		; add offset 0-38
		LD	A,(HL)		; pick up main key value
		SCF			; set carry flag
		RET			; return  (B has other key still)

;------------------
; Keyboard decoding
;------------------
; also called from S_INKEY

					;;;$0333
K_DECODE:	LD	A,E		; pick up the stored main key
		CP	$3A		; an arbitrary point between digits and letters
		JR	C,K_DIGIT	; forward to K_DIGIT with digits,space,enter

		DEC	C		; decrease MODE ( 0='KLC', 1='E', 2='G')
		JP	M,K_KLC_LET	; to K_KLC_LET if was zero

		JR	Z,K_E_LET	; to K_E_LET if was 1 for extended letters.

					; proceed with graphic codes.
					; Note. should selectively drop return address if code > 'U' ($55).
					; i.e. abort the KEYBOARD call.
					; e.g. cp 'V'; jr c addit; pop af; ;;addit etc. (5 bytes of instruction).
					; (S_INKEY never gets into graphics mode.)

					;; addit
		ADD	A,$4F		; add offset to augment 'A' to graphics A say.
		RET			; return.
					; Note. ( but [GRAPH] V gives RND, etc ).

					; the jump was to here with extended mode with uppercase A-Z.

					;;;$0341
K_E_LET:	LD	HL,E_UNSHIFT-$41; base address of E_UNSHIFT-$41
					; ( $01EB in standard ROM ) 
		INC	B		; test B is it empty i.e. not a shift
		JR	Z,K_LOOK_UP	; forward to K_LOOK_UP if neither shift

		LD	HL,EXT_SHIFT-$41; Address: $0205 EXT_SHIFT-$41 base

					;;;$034A
K_LOOK_UP:	LD	D,$00		; prepare to index
		ADD	HL,DE		; add the main key value
		LD	A,(HL)		; pick up other mode value
		RET			; return

					; the jump was here with mode = 0

					;;;$034F
K_KLC_LET:	LD	HL,SYM_CODES-$41; prepare base of SYM_CODES
		BIT	0,B		; shift=$27 sym-shift=$18
		JR	Z,K_LOOK_UP	; back to K_LOOK_UP with symbol-shift

		BIT	3,D		; test FLAGS is it 'K' mode (from OUT_CURS)
		JR	Z,K_TOKENS	; skip to K_TOKENS if so

		BIT	3,(IY+$30)	; test FLAGS2 - consider CAPS LOCK ?
		RET	NZ		; return if so with main code.

		INC	B		; is shift being pressed ?
					; result zero if not
		RET	NZ		; return if shift pressed.

		ADD	A,$20		; else convert the code to lower case.
		RET			; return.

					; the jump was here for tokens

					;;;$0364
K_TOKENS:	ADD	A,$A5		; add offset to main code so that 'A'
					; becomes 'NEW' etc.
		RET			; return

					; the jump was here with digits, space, enter and symbol shift (< $xx)

					;;;$0367
K_DIGIT:	CP	$30		; is it '0' or higher ?
		RET	C		; return with space, enter and symbol-shift

		DEC	C		; test MODE (was 0='KLC', 1='E', 2='G')
		JP	M,K_KLC_DGT	; jump to K_KLC_DGT if was 0.

		JR	NZ,K_GRA_DGT	; forward to K_GRA_DGT if mode was 2.

					; continue with extended digits 0-9.

		LD	HL,E_DIGITS-$30	; $0254 - base of E_DIGITS
		BIT	5,B		; test - shift=$27 sym-shift=$18
		JR	Z,K_LOOK_UP	; to K_LOOK_UP if sym-shift

		CP	$38		; is character '8' ?
		JR	NC,K_8_AND_9	; to K_8_AND_9 if greater than '7'

		SUB	$20		; reduce to ink range $10-$17
		INC	B		; shift ?
		RET	Z		; return if not.

		ADD	A,$08		; add 8 to give paper range $18 - $1F
		RET			; return

					; 89

					;;;$0382
K_8_AND_9:	SUB	$36		; reduce to 02 and 03  bright codes
		INC	B		; test if shift pressed.
		RET	Z		; return if not.

		ADD	A,$FE		; subtract 2 setting carry
		RET			; to give 0 and 1 flash codes.

					; graphics mode with digits

					;;;$0389
K_GRA_DGT:	LD	HL,CTL_CODES-$30; $0230 base address of CTL_CODES

		CP	$39		; is key '9' ?
		JR	Z,K_LOOK_UP	; back to K_LOOK_UP - changed to $0F, GRAPHICS.

		CP	$30		; is key '0' ?
		JR	Z,K_LOOK_UP	; back to K_LOOK_UP - changed to $0C, delete.

					; for keys '0' - '7' we assign a mosaic character depending on shift.

		AND	$07		; convert character to number. 0 - 7.
		ADD	A,$80		; add offset - they start at $80
		INC	B		; destructively test for shift
		RET	Z		; and return if not pressed.

		XOR	$0F		; toggle bits becomes range $88-$8F
		RET			; return.

					; now digits in 'KLC' mode

					;;;$039D
K_KLC_DGT:	INC	B		; return with digit codes if neither
		RET	Z		; shift key pressed.

		BIT	5,B		; test for caps shift.
		LD	HL,CTL_CODES-$30; prepare base of table CTL_CODES.
		JR	NZ,K_LOOK_UP	; back to K_LOOK_UP if shift pressed.

					; must have been symbol shift

		SUB	$10		; for ascii most will now be correct
					; on a standard typewriter.
		CP	$22		; but '@' is not - see below.
		JR	Z,K_AT_CHAR	; forward to to K_AT_CHAR if so

		CP	$20		; '_' is the other one that fails
		RET	NZ		; return if not.

		LD	A,$5F		; substitute ascii '_'
		RET			; return.

					;;;$03B2
K_AT_CHAR:	LD	A,$40		; substitute ascii '@'
		RET			; return.


;-------------------------------------------------------------------------
; The Spectrum Input character keys. One or two are abbreviated.
; From $00 Flash 0 to $FF COPY. The routine above has decoded all these.

;  | 00 Fl0| 01 Fl1| 02 Br0| 03 Br1| 04 In0| 05 In1| 06 CAP| 07 EDT|
;  | 08 LFT| 09 RIG| 0A DWN| 0B UP | 0C DEL| 0D ENT| 0E SYM| 0F GRA|
;  | 10 Ik0| 11 Ik1| 12 Ik2| 13 Ik3| 14 Ik4| 15 Ik5| 16 Ik6| 17 Ik7|
;  | 18 Pa0| 19 Pa1| 1A Pa2| 1B Pa3| 1C Pa4| 1D Pa5| 1E Pa6| 1F Pa7|
;  | 20 SP | 21  ! | 22  " | 23  # | 24  $ | 25  % | 26  & | 27  ' |
;  | 28  ( | 29  ) | 2A  * | 2B  + | 2C  , | 2D  - | 2E  . | 2F  / |
;  | 30  0 | 31  1 | 32  2 | 33  3 | 34  4 | 35  5 | 36  6 | 37  7 |
;  | 38  8 | 39  9 | 3A  : | 3B  ; | 3C  < | 3D  = | 3E  > | 3F  ? |
;  | 40  @ | 41  A | 42  B | 43  C | 44  D | 45  E | 46  F | 47  G |
;  | 48  H | 49  I | 4A  J | 4B  K | 4C  L | 4D  M | 4E  N | 4F  O |
;  | 50  P | 51  Q | 52  R | 53  S | 54  T | 55  U | 56  V | 57  W |
;  | 58  X | 59  Y | 5A  Z | 5B  [ | 5C  \ | 5D  ] | 5E  ^ | 5F  _ |
;  | 60 ukp| 61  a | 62  b | 63  c | 64  d | 65  e | 66  f | 67  g |
;  | 68  h | 69  i | 6A  j | 6B  k | 6C  l | 6D  m | 6E  n | 6F  o |
;  | 70  p | 71  q | 72  r | 73  s | 74  t | 75  u | 76  v | 77  w |
;  | 78  x | 79  y | 7A  z | 7B  { | 7C  | | 7D  } | 7E  ~ | 7F (c)|
;  | 80 128| 81 129| 82 130| 83 131| 84 132| 85 133| 86 134| 87 135|
;  | 88 136| 89 137| 8A 138| 8B 139| 8C 140| 8D 141| 8E 142| 8F 143|
;  | 90 [A]| 91 [B]| 92 [C]| 93 [D]| 94 [E]| 95 [F]| 96 [G]| 97 [H]|
;  | 98 [I]| 99 [J]| 9A [K]| 9B [L]| 9C [M]| 9D [N]| 9E [O]| 9F [P]|
;  | A0 [Q]| A1 [R]| A2 [S]| A3 [T]| A4 [U]| A5 RND| A6 IK$| A7 PI |
;  | A8 FN | A9 PNT| AA SC$| AB ATT| AC AT | AD TAB| AE VL$| AF COD|
;  | B0 VAL| B1 LEN| B2 SIN| B3 COS| B4 TAN| B5 ASN| B6 ACS| B7 ATN|
;  | B8 LN | B9 EXP| BA INT| BB SQR| BC SGN| BD ABS| BE PEK| BF IN |
;  | C0 USR| C1 ST$| C2 CH$| C3 NOT| C4 BIN| C5 OR | C6 AND| C7 <= |
;  | C8 >= | C9 <> | CA LIN| CB THN| CC TO | CD STP| CE DEF| CF CAT|
;  | D0 FMT| D1 MOV| D2 ERS| D3 OPN| D4 CLO| D5 MRG| D6 VFY| D7 BEP|
;  | D8 CIR| D9 INK| DA PAP| DB FLA| DC BRI| DD INV| DE OVR| DF OUT|
;  | E0 LPR| E1 LLI| E2 STP| E3 REA| E4 DAT| E5 RES| E6 NEW| E7 BDR|
;  | E8 CON| E9 DIM| EA REM| EB FOR| EC GTO| ED GSB| EE INP| EF LOA|
;  | F0 LIS| F1 LET| F2 PAU| F3 NXT| F4 POK| F5 PRI| F6 PLO| F7 RUN|
;  | F8 SAV| F9 RAN| FA IF | FB CLS| FC DRW| FD CLR| FE RET| FF CPY|

; Note that for simplicity, Sinclair have located all the control codes
; below the space character.
; ascii DEL, $7F, has been made a copyright symbol.
; Also $60, '`', not used in Basic but used in other languages, has been
; allocated the local currency symbol for the relevant country -
; ukp in most Spectrums.

;-------------------------------------------------------------------------

;-----------
; Key tables
;-----------
; These six look-up tables are used by the keyboard reading routine
; to decode the key values.

; The first table contains the maps for the 39 keys of the standard
; 40-key Spectrum keyboard. The remaining key [SHIFT $27] is read directly.
; The keys consist of the 26 upper-case alphabetic characters, the 10 digit
; keys and the space, ENTER and symbol shift key.
; Unshifted alphabetic keys have $20 added to the value.
; The keywords for the main alphabetic keys are obtained by adding $A5 to
; the values obtained from this table.

					;;;$0205
MAIN_KEYS:	DEFB	$42		; B
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
		DEFB	$0E		; SYMBOL SHIFT
		DEFB	$4C		; L
		DEFB	$4F		; O
		DEFB	$39		; 9
		DEFB	$32		; 2
		DEFB	$57		; W
		DEFB	$53		; S
		DEFB	$5A		; Z
		DEFB	$20		; SPACE
		DEFB	$0D		; ENTER
		DEFB	$50		; P
		DEFB	$30		; 0
		DEFB	$31		; 1
		DEFB	$51		; Q
		DEFB	$41		; A

					;;;$022C
					;  The 26 unshifted extended mode keys for the alphabetic characters.
					;  The green keywords on the original keyboard.
E_UNSHIFT:	DEFB	$E3		; READ
		DEFB	$C4		; BIN
		DEFB	$E0		; LPRINT
		DEFB	$E4		; DATA
		DEFB	$B4		; TAN
		DEFB	$BC		; SGN
		DEFB	$BD		; ABS
		DEFB	$BB		; SQR
		DEFB	$AF		; CODE
		DEFB	$B0		; VAL
		DEFB	$B1		; LEN
		DEFB	$C0		; USR
		DEFB	$A7		; PI
		DEFB	$A6		; INKEY$
		DEFB	$BE		; PEEK
		DEFB	$AD		; TAB
		DEFB	$B2		; SIN
		DEFB	$BA		; INT
		DEFB	$E5		; RESTORE
		DEFB	$A5		; RND
		DEFB	$C2		; CHR$
		DEFB	$E1		; LLIST
		DEFB	$B3		; COS
		DEFB	$B9		; EXP
		DEFB	$C1		; STR$
		DEFB	$B8		; LN

					;;;$0246
					;  The 26 shifted extended mode keys for the alphabetic characters.
					;  The red keywords below keys on the original keyboard.
EXT_SHIFT:	DEFB	$7E		; ~
		DEFB	$DC		; BRIGHT
		DEFB	$DA		; PAPER
		DEFB	$5C		; \ ;
		DEFB	$B7		; ATN
		DEFB	$7B		; {
		DEFB	$7D		; }
		DEFB	$D8		; CIRCLE
		DEFB	$BF		; IN
		DEFB	$AE		; VAL$
		DEFB	$AA		; SCREEN$
		DEFB	$AB		; ATTR
		DEFB	$DD		; INVERSE
		DEFB	$DE		; OVER
		DEFB	$DF		; OUT
		DEFB	$7F		; (Copyright character)
		DEFB	$B5		; ASN
		DEFB	$D6		; VERIFY
		DEFB	$7C		; |
		DEFB	$D5		; MERGE
		DEFB	$5D		; ]
		DEFB	$DB		; FLASH
		DEFB	$B6		; ACS
		DEFB	$D9		; INK
		DEFB	$5B		; [
		DEFB	$D7		; BEEP

					;;;$0260
					;  The ten control codes assigned to the top line of digits when the shift 
					;  key is pressed.
CTL_CODES:	DEFB	$0C		; DELETE
		DEFB	$07		; EDIT
		DEFB	$06		; CAPS LOCK
		DEFB	$04		; TRUE VIDEO
		DEFB	$05		; INVERSE VIDEO
		DEFB	$08		; CURSOR LEFT
		DEFB	$0A		; CURSOR DOWN
		DEFB	$0B		; CURSOR UP
		DEFB	$09		; CURSOR RIGHT
		DEFB	$0F		; GRAPHICS

					;;;$026A
					;  The 26 red symbols assigned to the alphabetic characters of the keyboard.
					;  The ten single-character digit symbols are converted without the aid of
					;  a table using subtraction and minor manipulation. 
SYM_CODES:	DEFB	$E2		; STOP
		DEFB	$2A		; *
		DEFB	$3F		; ?
		DEFB	$CD		; STEP
		DEFB	$C8		; >=
		DEFB	$CC		; TO
		DEFB	$CB		; THEN
		DEFB	$5E		; ^
		DEFB	$AC		; AT
		DEFB	$2D		; -
		DEFB	$2B		; +
		DEFB	$3D		; =
		DEFB	$2E		; .
		DEFB	$2C		; ,
		DEFB	$3B		; ;
		DEFB	$22		; "
		DEFB	$C7		; <=
		DEFB	$3C		; <
		DEFB	$C3		; NOT
		DEFB	$3E		; >
		DEFB	$C5		; OR
		DEFB	$2F		; /
		DEFB	$C9		; <>
		DEFB	$60		; pound
		DEFB	$C6		; AND
		DEFB	$3A		; :

					;;;$0284
					;  The ten keywords assigned to the digits in extended mode.
					;  The remaining red keywords below the keys.
E_DIGITS:	DEFB	$D0		; FORMAT
		DEFB	$CE		; DEF FN
		DEFB	$A8		; FN
		DEFB	$CA		; LINE
		DEFB	$D3		; OPEN#
		DEFB	$D4		; CLOSE#
		DEFB	$D1		; MOVE
		DEFB	$D2		; ERASE
		DEFB	$A9		; POINT
		DEFB	$CF		; CAT
