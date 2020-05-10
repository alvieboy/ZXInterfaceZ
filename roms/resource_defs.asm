
RESOURCE_TYPE_INVALID equ $ff
RESOURCE_TYPE_INTEGER equ $00
RESOURCE_TYPE_STRING equ $01
RESOURCE_TYPE_BITMAP equ $02
RESOURCE_TYPE_DIRECTORYLIST equ $03
RESOURCE_TYPE_APLIST equ $04
RESOURCE_TYPE_OPSTATUS equ $05 ; Operation status

RESOURCE_ID_VERSION	equ $00
RESOURCE_ID_LOGO	equ $01
RESOURCE_ID_STATUS	equ $02
RESOURCE_ID_DIRECTORY	equ $03

RESOURCE_ID_OPERATION_STATUS	equ $10

	STATUS_INPROGRESS 	equ $FE
        STATUS_ERROR 		equ $FF
        STATUS_OK 		equ $00
