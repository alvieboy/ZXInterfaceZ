;
; General structure of an object.
;
;  This example shows two C++ classes, and how they map into
;  Z80 memory layout. The "new" operator can either be a "malloc-like" function
;  or a static memory area, note however that the Ctor *must* be called nonetheless.
;
;  class Parent {
;    public:
;      Parent();
;      virtual void firstFunction();
;      virtual void secondFunction();
;    protected:
;      unsigned char  parent_var_1;
;      unsigned char  parent_var_2;
;      unsigned short parent_var_3;
;  };
;
;  class Derived {
;    public:
;      Derived() {
;	  derived_var1 = 0;	/* Set up field */
;      }
;      virtual void firstFunction();
;      /* Second function not overriden */
;      virtual void thirdFunction();
;    protected:
;      unsigned char derived_var1;
;      unsigned char derived_var2;
;  };
;
;  VTABLE for Parent (only used if class does not have pure virtual functions):
;  Parent__VTABLE:
;  	DEFW Parent__firstFunction
;  	DEFW Parent__secondFunction
;
;  VTABLE for Derived (only used if class does not have pure virtual functions):
;  Derived__VTABLE:
;  	DEFW Derived__firstFunction
;  	DEFW Parent__secondFunction
;  	DEFW Derived__thirdFunction
;
;  Construtor for Parent:
;
;  Parent__CTOR:
;	LD	(IX), Parent__VTABLE   ; Pseudo LD
;	RET
;    Virtual function EQU's:
;    	Parent__firstFunction 		EQU 	0
;    	Parent__secondFunction 		EQU 	1
;    	Parent__thirdFunction 		EQU 	2
;	Parent__NUM_VTABLE_ENTRIES	EQU 	3

;
;  Construtor for Derived:
;
;  Derived__CTOR:
;	CALL	Parent__CTOR
;	LD	(IX), Derived__VTABLE   ; Pseudo LD
;	LD 	(IX+6), 0		; derived_var1 = 0;	/* Set up field */
;	RET
;
;    Virtual function EQU's:
;    	Derived__firstFunction 		EQU 	Parent__NUM_VTABLE_ENTRIES+0
;    	Derived__secondFunction 	EQU 	Parent__NUM_VTABLE_ENTRIES+1
;	Derived__NUM_VTABLE_ENTRIES	EQU 	(Parent__NUM_VTABLE_ENTRIES+2)

;  Memory layout for a Derived class (pointed by IX):
;
;	Offset	Len	Field
;	0	2       Vtable                \
;	2	1	parent_var_1          |  This is Parent class. VTable does point to the
;	3	1	parent_var_2          |  leaf class.
;	4	2	parent_var_3          /
;	6	1	derived_var_1
;	7	1	derived_var_2

;
; Creating an object of "Derived" using a static memory area.
;
;  Assume there is a memory area called DERIVED_INST which can hold the object (i.e., the 8 bytes above)
;
;  	LD	IX, DERIVED_INST
;  	CALL	Derived__CTOR
;
;  If you had a MALLOC-like function, this is how you'd do it:
;
;	LD	A, Derived__SIZE
;	CALL	MALLOC  		; Allocate "A" bytes, return pointer in IX
;	CALL	Derived__CTOR		; Call the constuctor
;
;  This would be equivalent to C++:
;
;	Derived *instance = new Derived();
;    where "instance" would be stored in IX
;
;
;
;  Calling Parent__firstFunction() on either class pointer in IX:
;
;	VCALL	Parent__firstFunction
;
;		    In case this is a Derived class, what happens with VCALL is:
;			- The Parent__firstFunction (which is 0) entry is loaded from the vtable:
;				-- (IX) is loaded, which is pointer to Vtable (Derived__VTABLE). This has been set up by
;				   the Derived__CTOR.
;				-- The first (index 0) item is loaded from the Vtable, which points to function
;              			   Derived__firstFunction
;				-- The function Derived__firstFunction is called.


; Virtual method call
;
; IX: 	"this" pointer for class.
;	The 1st word at (IX) is the virtual function table.
; offset: Offset (number) of the virtual function 
VCALL	MACRO offset
	PUSH	IX
        LD	IX, _retloc
        EX	(SP), IX   	; Return address
	PUSH	IX
        PUSH	AF
	LD	A, (IX)
        LD	IXL, A
        LD	A, (IX+1)
        LD	IXH, A
        ; IX now points to start of vtable
        LD	A, (IX+(offset*2))
        LD	IXL,A
        LD	A, (IX+(offset*2)+1)
        LD	IXH, A
        POP	AF
        ; Function in IX
        EX	(SP), IX
        RET	; Call function!
_retloc:
ENDM 







