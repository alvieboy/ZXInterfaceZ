MOVE_HL_PAST_NULL:
	LD	A,(HL)
        OR	A
        INC	HL
        JR	NZ, MOVE_HL_PAST_NULL
        RET
