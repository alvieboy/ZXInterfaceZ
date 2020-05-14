	DEVICE ZXSPECTRUM48
	ORG 23552

; Place well-known values in registers so we can check the snapshot features
	DB "Here"
REGTEST:
	DI
	LD	A, $01
	EX 	AF, AF'
	EXX
        LD	BC, $0203
        LD	DE, $0405
        LD	HL, $0607
        EXX
        LD	SP, $FACA
        LD	BC, $090A
        LD	DE, $0D0E
        LD	HL, $0F10
        LD	IX, $1112
        LD	IY, $1314
        LD	A, $AE
        LD	I, A
        LD	A, $15
    	JR	$
	DB "There"

	SAVESNA "regtest.sna", REGTEST
