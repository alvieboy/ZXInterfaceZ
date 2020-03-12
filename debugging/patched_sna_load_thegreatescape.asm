; z80dasm 1.1.5
; command line: z80dasm -g 0 -l patched_sna_load_thegreatescape.bin

	org	00000h

l0000h:
	di	
	ld de,0ffffh
	jp l00cdh
	nop	
l0008h:
	jp l0008h
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	retn
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
	nop	
; SNA header:
;
;:00000000  3f c2 d2 00 00 00 00 1a  0c 8a af 2b 54 5e 04 00  |?..........+T^..|
;:00000010  80 82 ff 00 40 00 00 eb  ff 01 00 00 00 00 00 00  |....@...........|
;:00000020  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
;
; I:3f
; HL': d2c2
; DE': 0000
; BC': 0000
; AF': 0c1a
; HL : af8a
; DE : 542b
; BC : 045e
; IY : 8000
; IX : ff82
; INT: 00
; R  : 40 
; AF : 0000
; SP : ffeb
; IntMode: 01
; Border: 00 

l0080h:
	di	
	ld hl,(05c02h)
	exx	
	ld de,00c1ah
	ld (05c02h),de
	ld sp,05c02h
	ld bc,l0000h
	ld de,l0000h
	ld hl,0d2c2h
	pop af	
	exx	
	ld a,03fh
	ld i,a
	ld de,l0000h
	ld (05c02h),de
	ld bc,0045eh
	ld sp,05c02h
	ld a,040h
	ld r,a
	ld a,000h
	out (0feh),a
	pop af	
	ld de,0542bh
	ld ix,0ff82h
	ld iy,08000h
	ld (05c02h),hl
	ld sp,0ffebh
	ld hl,0af8ah
	im 1
	di	
	retn
l00cdh:
	ld a,007h
	out (0feh),a
	ld a,03fh
	ld i,a
	ld h,d	
	ld l,e	
l00d7h:
	ld (hl),002h
	dec hl	
	cp h	
	jr nz,l00d7h
l00ddh:
	and a	
	sbc hl,de
	add hl,de	
	inc hl	
	jr nc,l00eah
	dec (hl)	
	jr z,l00eah
	dec (hl)	
	jr z,l00ddh
l00eah:
	dec hl	
	dec hl	
	ld sp,hl	
	ld hl,04000h
	ld d,0bfh
	ld b,0ffh
l00f4h:
	in a,(00bh)
	or a	
	jr nz,l00f4h
	in a,(00dh)
	ld (hl),a	
	inc hl	
	djnz l00f4h
	dec d	
	jr nz,l00f4h
	jp l0080h
