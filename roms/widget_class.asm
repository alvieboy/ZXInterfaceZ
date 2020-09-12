; Widget class.

;
; C++ pseudo
;
;	// A point is a 16-bit value/register
;	union {
;	  struct {
;           unsigned char x;
;           unsigned char y;
;	  }
;         unsigned short xy;
;	} point;
;
;	struct {
;	  point from;
;	  point to;
;	} twopoint;
;
;	class Widget {
;	  public:
;	    Widget();
;	    setParent(Widget*);
;	    // Virtual methods
;           virtual void draw();
;	    virtual void show();
;	  protected:
;           virtual void resize(point xy /* HL */, point width_height /* DE */);  /* virtual, protected */
;           virtual void drawImpl(); /* virtual, protected */
;	    virtual void handleEvent(); /* virtual, protected */
;	    Widget *parent;
;	    unsigned char flags;
;	    point location;
;	}
;

;
; Widget VTABLE
;	DEFW	DTOR		/* Widget::~Widget() */
;	DEFW	draw		/* Widget::draw() */
;	DEFW	Widget_V__setVisible			/* Widget::setVisible() */
;	DEFW	Widget_PV__resize		/* Widget::resize() */
;	DEFW	drawImpl	/* Widget::drawImpl() */
;	DEFW	handleEvent	/* Widget::handleEvent() */


; Member offsets
Widget__parent_OFFSET 		EQU	Object__SIZE+0
Widget__x_OFFSET		EQU	Object__SIZE+2
Widget__y_OFFSET		EQU	Object__SIZE+3
Widget__flags_OFFSET		EQU	Object__SIZE+4
Widget__screenptr_OFFSET	EQU	Object__SIZE+5
Widget__attrptr_OFFSET		EQU	Object__SIZE+7
Widget__width_OFFSET		EQU	Object__SIZE+9
Widget__height_OFFSET		EQU	Object__SIZE+10
Widget__SIZE			EQU	Object__SIZE+11


; Virtual method indexes

Widget__DTOR			EQU	0
Widget__draw			EQU	1
Widget__setVisible		EQU	2
Widget__resize			EQU     3
Widget__drawImpl		EQU	4
Widget__handleEvent		EQU	5
Widget__NUM_VTABLE_ENTRIES	EQU	6

; Defines
WIDGET_FLAGS_VISIBLE_BIT 	EQU 	0


; Constructor
Widget__CTOR:
	; There is no VTABLE. There is no parent class.
        LD	(IX+Widget__parent_OFFSET+0), 0
        LD	(IX+Widget__parent_OFFSET+1), 0
        LD	(IX+Widget__flags_OFFSET), 0
        RET
;
; C++ pseudo
;
; point mapToGlobal(point p /* HL */ )
; {
;   if (parent) {
;     p = parent->mapToGlobal(p);
;   }
;   p.x += x;
;   p.y += y;
;   return p;
; }

Widget_PV__resize:
        DEBUGSTR "Resizing "
	DEBUGHEXIX

	LD	A, E
        CP	$FF
        JR	Z, _nowidthheight
 	LD	(IX+Widget__width_OFFSET), D
        LD	(IX+Widget__height_OFFSET), E
_nowidthheight:
	LD	A, L
        CP	$FF
        RET	Z 	; No xy updated
 	LD	(IX+Widget__x_OFFSET), H
        LD	(IX+Widget__y_OFFSET), L
        DEBUGSTR "Recompute pointers "
        DEBUGSTR " XY="
        DEBUGHEXHL
        DEBUGSTR " WH="
        DEBUGHEXDE
        
        
        JP	Widget_P__recalculateScreenPointers
        RET

Widget__setParent:
	DEBUGSTR "Set parent to "
        DEBUGHEXHL
        DEBUGSTR " > we are "
        DEBUGHEXIX

        LD	(IX+Widget__parent_OFFSET), L
        LD 	(IX+Widget__parent_OFFSET+1), H
        RET

Widget_V__setVisible:
	CP	$00
        JR	Z, _hide
	SET 	WIDGET_FLAGS_VISIBLE_BIT, (IX+Widget__flags_OFFSET)
        VCALL	Widget__draw
        RET
_hide:
	RES 	WIDGET_FLAGS_VISIBLE_BIT, (IX+Widget__flags_OFFSET)
        ; TBD: we need to redraw other widgets
	RET

Widget_V__draw:
	VCALL	Widget__drawImpl
	RET

Widget_P__recalculateScreenPointers:
	PUSH	DE
        
	LD	C, (IX+Widget__x_OFFSET)
        LD	D, (IX+Widget__y_OFFSET)
        CALL	GETXYSCREENSTART
        LD	(IX+Widget__screenptr_OFFSET), L
        LD	(IX+Widget__screenptr_OFFSET+1), H
        
        CALL	GETXYATTRSTART
        LD	(IX+Widget__attrptr_OFFSET), L
        LD	(IX+Widget__attrptr_OFFSET+1), H
        POP	DE
        RET

Widget__getWidth:
	LD	A, (IX+Widget__width_OFFSET)
        RET

Widget__getHeight:
	LD	A, (IX+Widget__height_OFFSET)
        RET

Widget_PV__handleEvent:
	RET

Widget__showParent
	DEBUGSTR "THIS="
        DEBUGHEXIX
        DEBUGSTR " > parent"
        LD	L, (IX+Widget__parent_OFFSET)
        LD	H, (IX+Widget__parent_OFFSET+1)
        DEBUGHEXHL
        RET

Widget__closeParent:
	RET
        
Widget_V__DTOR:
        ; If we have a parent, "cast" to a group anc remove ourselves from it.
        LD	L, (IX+Widget__parent_OFFSET)
        LD	H, (IX+Widget__parent_OFFSET+1)
        LD	A, L
        OR	H
        RET	Z
        SWAP_IX_HL
        ; <CAST>
        VCALL	WidgetGroup__removeChild
        SWAP_IX_HL
	RET

Widget__destroyParent:
        ; If we have a parent, "cast" to a group anc remove ourselves from it.
        LD	L, (IX+Widget__parent_OFFSET)
        LD	H, (IX+Widget__parent_OFFSET+1)
        LD	A, L
        OR	H
        RET	Z
        SWAP_IX_HL
        ; <CAST>
        VCALL	Widget__DTOR
	RET

