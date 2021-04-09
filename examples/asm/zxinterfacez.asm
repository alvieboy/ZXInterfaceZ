;
; ZX Interface Z API macros for SJASMPLUS
;
;

ZXZ_SYSCALL_GETCWD              EQU     192
ZXZ_SYSCALL_CHDIR               EQU     193
ZXZ_SYSCALL_OPEN                EQU     194
ZXZ_SYSCALL_CLOSE               EQU     195
ZXZ_SYSCALL_READ                EQU     196
ZXZ_SYSCALL_WRITE               EQU     197
ZXZ_SYSCALL_OPENDIR             EQU     198
ZXZ_SYSCALL_READDIR             EQU     199
ZXZ_SYSCALL_CLOSEDIR            EQU     200

ZXZ_SYSCALL_SOCKET              EQU     201
ZXZ_SYSCALL_CONNECT             EQU     202
ZXZ_SYSCALL_SENDTO              EQU     203
ZXZ_SYSCALL_RECVFROM            EQU     204
ZXZ_SYSCALL_GETHOSTBYNAME       EQU     205
ZXZ_SYSCALL_GETHOSTBYADDR       EQU     206
ZXZ_SYSCALL_STRERROR            EQU     207

; Socket types, to be used with ZXZ_SOCKET
ZXZ_SOCKET_TCP            EQU    6
ZXZ_SOCKET_UDP            EQU    17


;
;   ZXZ_GETCWD
;
;   Get the current working directory (as a null-terminated string). A maximum of 255 bytes can be used.
;
;   Arguments:
;       HL: Pointer to the memory location where to store the directory name.
;       B:  Maximum size of the buffer
;
;   Returns:
;       If there is not enough room to store the directory name the C flag will be set.
;       If any error occured, carry flag will be set upon exit. 
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A: zero
;
;       B:  Length of the working directory (not including the null-termination character)
;
;       Destroys/Modifies:
;           A, Flags
;
    MACRO ZXZ_GETCWD
        RST    08
        DB    ZXZ_SYSCALL_GETCWD
    ENDM

;
;   ZXZ_CHDIR
;
;   Change the current working directory
;
;   Arguments:
;       HL: Pointer to the new directory, null-terminated string
;
;   Returns:
;       If any error occured, carry flag will be set upon exit. 
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A: zero
;
;       Destroys/Modifies:
;           A, Flags
;
    MACRO ZXZ_CHDIR
        RST    08
        DB    ZXZ_SYSCALL_CHDIR
    ENDM


;
;   ZXZ_OPEN
;
;   Open a file
;
;   Arguments:
;       HL: Pointer to the file name (null-terminated)
;       B:  Open flags. See ZXZ_FILE_MODE_* EQUs
;
;   Returns:
;       If any error occured, carry flag will be set upon exit. 
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A: zero
;
;       B:  File handle to be used on subsequent file operations
;
;       Destroys/Modifies:
;           A, B, Flags
;
    MACRO ZXZ_OPEN
        RST    08
        DB    ZXZ_SYSCALL_OPEN
    ENDM

;
;   ZXZ_CLOSE
;
;   Close a File/Socket descriptor
;
;   Arguments:
;       B:  File/Socket descriptor
;   Returns:
;       If any error occured, carry flag will be set upon exit.
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A:  zero
;
;       Destroys/Modifies:
;           A, Flags
;

    MACRO ZXZ_CLOSE
        RST    08
        DB    ZXZ_SYSCALL_CLOSE
    ENDM

;
;   ZXZ_READ
;
;   Read data from a socket or file descriptor
;   Amount of data read will depend on the descriptor type and availability of data.
;   This call may block if the descriptor is not non-blocking.
;
;   Arguments:
;       B:  File/Socket descriptor
;       HL: Pointer to target memory area which will contain the data
;       DE: Number of bytes (maximum) to read
;   Returns:
;       If any error occured, carry flag will be set upon exit.
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A:  zero
;
;       DE will contain the number of bytes actually read.
;       HL will point to the location past the last byte received.
;
;       Destroys/Modifies:
;           A, Flags, HL, DE
;

    MACRO ZXZ_READ
        RST    08
        DB    ZXZ_SYSCALL_READ
    ENDM

;
;   ZXZ_WRITE
;
;   Write data to socket or file descriptor
;   This call may block if the descriptor is not non-blocking.
;
;   Arguments:
;       B:  File/Socket descriptor
;       HL: Pointer to source memory area which contains the data to be written
;       DE: Number of bytes to write
;   Returns:
;       If any error occured, carry flag will be set upon exit.
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A:  zero
;
;       DE will contain the number of bytes actually written.
;       HL will point to the location past the last byte written.
;
;       Destroys/Modifies:
;           A, Flags, HL, DE
;

    MACRO ZXZ_WRITE
        RST    08
        DB    ZXZ_SYSCALL_WRITE
    ENDM

;
;   ZXZ_OPENDIR
;
;   Open a directory
;
;   Arguments:
;       HL: Pointer to the directory name (null-terminated)
;
;   Returns:
;       If any error occured, carry flag will be set upon exit. 
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A: zero
;
;       B:  Directory handle to be used on subsequent directory operations
;
;       Destroys/Modifies:
;           A, B, Flags
;
    MACRO ZXZ_OPENDIR
        RST    08
        DB    ZXZ_SYSCALL_OPENDIR
    ENDM

;
;   ZXZ_READDIR
;
;   Read a directory
;
;   Arguments:
;       B:  Directory handle
;       HL: Pointer to a memory location (256 bytes) which will hold entry information:
;           (HL): Entry type. 0x01 (regular file) or 0x02 (directory). Other values reserved.
;           (HL+1 to HL+255):   Entry name, null-terminated.
;
;   Returns:
;       If any error occured, carry flag will be set upon exit. 
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A: zero
;
;       Destroys/Modifies:
;           A, Flags
;
    MACRO ZXZ_READDIR
        RST    08
        DB    ZXZ_SYSCALL_READDIR
    ENDM


;
;   ZXZ_CLOSEDIR
;
;   Close a directory descriptor
;
;   Arguments:
;       B:  Directory descriptor
;   Returns:
;       If any error occured, carry flag will be set upon exit.
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A:  zero
;
;       Destroys/Modifies:
;           A, Flags
;

    MACRO ZXZ_CLOSEDIR
        RST    08
        DB    ZXZ_SYSCALL_CLOSEDIR
    ENDM

;
;   ZXZ_SOCKET
;
;   Create a new socket for internet access
;
;   Arguments:
;       B:    Socket type (0x06 TCP, 0x11 UDP). Other values are reserved
;              and will return error. See ZXZ_SOCKET_* EQUs for easy to use socket types
;   Returns:
;       If any error occured, carry flag will be set upon exit.
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A:  New socket descriptor which should be used for all socket calls.
;
;       Destroys/Modifies:
;           A, Flags
;
    MACRO ZXZ_SOCKET
        RST    08
        DB    ZXZ_SYSCALL_SOCKET
    ENDM

;
;   ZXZ_GETHOSTBYNAME
;
;   Resolve (DNS) a host name into an IPv4 address
;
;   Arguments:
;       DE: Pointer to host name to resolve, which should be NULL-terminated
;       HL: Pointer to 4-byte location where IP address will be stored.
;
;   Returns:
;       If any error occured, carry flag will be set upon exit.
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A:  zero
;
;       Destroys/Modifies:
;           A, Flags, HL and DE also modified.
;

    MACRO ZXZ_GETHOSTBYNAME
        RST    08
        DB    ZXZ_SYSCALL_GETHOSTBYNAME
    ENDM

;
;   ZXZ_CONNECT
;
;   Connect a previously created socket.
;
;   Arguments:
;       B:  Socket descriptor, obtained used ZXZ_SOCKET syscall
;       HL: Pointer to IPv4 address (as obtained for example with ZXZ_GETHOSTBYNAME)
;       DE: Port to connect to
;   Returns:
;       If any error occured, carry flag will be set upon exit.
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A:  zero
;
;       Destroys/Modifies:
;           A, Flags, HL
;

    MACRO ZXZ_CONNECT
        RST    08
        DB    ZXZ_SYSCALL_CONNECT
    ENDM

;
;   ZXZ_SENDTO
;
;   Send data packet to a specific host.
;   This call may block if the descriptor is not non-blocking.
;   Note that unlike most syscalls, the socket argument is in A, not B.
;   If DE is 0x0000, this function behaves like ZXZ_SEND and will use the
;   socket address if the socket is connected.
;
;   Arguments:
;       A:  File/Socket descriptor
;       HL: Pointer to source memory area which contains the data to be written
;       BC: Number of bytes to write
;       DE: Pointer to a structure containing:
;           - 4 bytes host IP address
;           - 2 bytes port (little-endian)
;   Returns:
;       If any error occured, carry flag will be set upon exit.
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A:  zero
;
;       BC will contain the number of bytes actually written.
;       HL will point to the location past the last byte written.
;
;       Destroys/Modifies:
;           A, Flags, HL, BC
;

    MACRO ZXZ_SENDTO
        RST    08
        DB    ZXZ_SYSCALL_SENDTO
    ENDM

;
;   ZXZ_SEND
;
;   Send data packet 
;   This call may block if the descriptor is not non-blocking.
;   Note that unlike most syscalls, the socket argument is in A, not B.
;   This function behaves like ZXZ_SENDTO with DE = 0x0000
;
;   Arguments:
;       A:  File/Socket descriptor
;       HL: Pointer to source memory area which contains the data to be written
;       BC: Number of bytes to write
;
;   Returns:
;       If any error occured, carry flag will be set upon exit.
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A:  zero
;
;       BC will contain the number of bytes actually written.
;       HL will point to the location past the last byte written.
;
;       Destroys/Modifies:
;           A, Flags, HL, BC
;

    
    MACRO ZXZ_SEND
        PUSH    DE
        LD      DE, 0
        RST     08
        DB      ZXZ_SYSCALL_SENDTO
        POP     DE
    ENDM

;
;   ZXZ_RECVFROM
;
;   Send data packet to a specific host.
;   This call may block if the descriptor is not non-blocking.
;   Note that unlike most syscalls, the socket argument is in A, not B.
;
;   Arguments:
;       A:  File/Socket descriptor
;       HL: Pointer to source memory area which contains the data to be written
;       BC: Number of bytes to write
;       DE: Pointer to a structure containing:
;           - 4 bytes host IP address
;           - 2 bytes port (little-endian)
;   Returns:
;       If any error occured, carry flag will be set upon exit.
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A:  zero
;
;       BC will contain the number of bytes actually written.
;       HL will point to the location past the last byte written.
;
;       Destroys/Modifies:
;           A, Flags, HL, BC
;

    MACRO ZXZ_RECVFROM
        RST    08
        DB    ZXZ_SYSCALL_RECVFROM
    ENDM

;
;   ZXZ_STRERROR
;
;   Convert an error code into a null-terminated string
;
;   Arguments:
;       A:  Error code
;       HL: Pointer to target memory area which will contain the null-terminated string
;       B:  Max size of target memory area
;
;   Returns:
;       If any error occured, carry flag will be set upon exit.
;
;       If carry is set:
;           A:  error code
;       If carry is not set:
;           A:  zero
;
;       B will contain the length of the error string, not including the null character
;       HL will point to after the NULL character
;
;       Destroys/Modifies:
;           A, Flags, DE
;

    MACRO ZXZ_STRERROR
        RST    08
        DB    ZXZ_SYSCALL_STRERROR
    ENDM

