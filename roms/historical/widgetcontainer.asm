; WidgetContainer class.

;
; C++ pseudo
;
;
;	#define WIDGETCONTAINER_MAX_CHILDS 4
;
;	class WidgetContainer: public Widget {
;	  public:
;	    WidgetContainer(Widget *parent);
;	    // Non-virtual methods
;	    void addChild(Widget *, point p);
;	    // Inherited virtual methods
;           virtual void realise() = 0; /* Pure virtual */
;           virtual void redraw() = 0;  /* Pure virtual */
;           virtual void moveTo(point);  /* virtual */
;	    virtual point mapToGlobal(point p);
;	    // Virtual methods
;           virtual void moveChild(Widget *, point p) = 0;
;	  protected:
;	    unsigned char numchilds;
;	    Widget *childs[MAX_CHILDS];
;	}

WidgetContainer__MAXCHILDS		EQU 	4
; Member offsets
WidgetContainer__numchilds__OFFSET	EQU	Widget__SIZE+0
WidgetContainer__childs__OFFSET		EQU	Widget__SIZE+1
WidgetContainer__SIZE			EQU	Widget__SIZE+1+(WidgetContainer__MAXCHILDS*2)

; Virtual function indexes
WidgetContainer__moveChild		EQU	Widget__NUM_VTABLE_ENTRIES + 0
WidgetContainer__NUM_VTABLE_ENTRIES	EQU	Widget__NUM_VTABLE_ENTRIES + 1

; Constructor
WidgetContainer__CTOR:
	CALL	Widget__CTOR
        ;LD	(IX), 	LOW(WidgetContainer__VTABLE)
        ;LD	(IX+1), HIGH(WidgetContainer__VTABLE)
        ; Set numchilds to zero
        LD	(IX+WidgetContainer__numchilds__OFFSET), 0
        RET

	
WidgetContainer__addChild:
	; HL: widget to add
        ; DE: point location
        PUSH 	BC        
        PUSH 	IX
        
        LD	A, (IX+WidgetContainer__numchilds__OFFSET)        
        LD 	BC, WidgetContainer__childs__OFFSET
        ADD	A, C
        LD	C, A ; BC holds offset now into IX
        ADD	IX, BC
        ; Store it into (IX)
        LD	(IX), L
        LD	(IX+1), H
        
        POP	IX      ; IX: this pointer, HL: child
        
        ; Swap "this" and "child" pointer.
        SWAP_IX_HL
        DEBUGSTR "Setting parent "
        CALL	Widget__setParent

        ; Swap "this" and "child" pointer.
	SWAP_IX_HL
        
        POP	BC

        ; Move item into position
        VCALL	WidgetContainer__moveChild
        
        
        RET
        
WidgetContainer__redraw:

        LD	A, (IX+WidgetContainer__numchilds__OFFSET)
        OR	A
        RET	Z
        
        PUSH	BC
        PUSH 	IX
        
        DEBUGSTR "WidgetContainer redraw childs"
        CALL DEBUGHEXIX
        LD	BC, WidgetContainer__childs__OFFSET
        ADD	IX, BC 
        LD	B, A
_drawone:
        LD	L, (IX)
        LD	H, (IX+1)
        SWAP_IX_HL
        DEBUGSTR "Child @"
        CALL DEBUGHEXIX
        PUSH	HL
        VCALL	Widget__draw
        POP	HL
        INC	IX
        INC	IX
        DJNZ	_drawone
        
        POP	IX
        POP	BC
        RET


WidgetContainer__foreach_vcall_DE:
        LD	A, (IX+WidgetContainer__numchilds__OFFSET)
        OR	A
        RET	Z
        
        PUSH	DE
        PUSH 	IX
        
        DEBUGSTR "WidgetContainer redraw childs"
        CALL DEBUGHEXIX

        LD	BC, WidgetContainer__childs__OFFSET
        ADD	IX, BC 
        LD	B, A
_callone:
        LD	L, (IX)
        LD	H, (IX+1)
        PUSH	BC
        LD	C, E
        LD	B, D
        SWAP_IX_HL
        DEBUGSTR "Child @"
        CALL DEBUGHEXIX
        PUSH	HL
        CALL	VCALL_internal ; BC has offset
        POP	HL
        INC	IX
        INC	IX
        
        POP	BC
        
        DJNZ	_callone
        
        POP	IX
        POP	DE
        RET

WidgetContainer__show:
        DEBUGSTR	"Show container "
        CALL 	DEBUGHEXIX
	CALL	Widget_V__show
        DEBUGSTR  "Show childs\n"
	LD	DE, Widget__setVisible*2
        CALL	WidgetContainer__foreach_vcall_DE
        ; Redraw
        JP	WidgetContainer__redraw
        