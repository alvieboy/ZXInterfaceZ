# Project of ABI for Z-Interface

It's only proporsal for ABI. 

## ABI calls

There some options to make call to ABI.

We may use RST #8 error handler(like in esxDOS) and reimplement some parts of
API. We can use any place of ZX-Spectrum's rom that contains RET code(C9 byte).

For example, we use esxDOS-like aproach and our call will looks like:

```
	LD <REGS>,<PARAMS>
	RST #8
	DB <call number>
```

If we use any place of ROM - we can do something like this:

``` 
IZ_ENTRY  EQU #52

	LD IX, <CALL NUMBER>
	LD A, ....
	LD BC, ....
	CALL IZ_ENTRY
```

To detect IZ ABI we may make easiest test - get version call.

For example:

``` 
GET_VERSION             EQU #0
GET_API_VERSION     	EQU #0
GET_FEATURES	        EQU #1

	LD IX, GET_VERSION
	XOR A ; Same as LD A, GET_DOS_API_VERSION. Any ABI version should be bigger than 0 :)
	CALL IZ_ENTRY         ; A returns ABI version, if there isn't Interface Z - register A will contains zero still
	AND A
	JP Z, izNotFound
```

## DOS ABI

For making application there some minimal set of ABI calls:

### fopen(for example #10)

Arguments:

 * HL - path to StringZ that contains filename(relative/absolute path acceptable both)
 
 * B - file open mode - bitmask can be combined. 1 - Read, 2 - Write, 4 - Create

Returns:

If no error:

 * A - file number
 
 * Flag C is down 

If error happens

 * A - error code
 
 * Flag C setted 

Example:

```
FOPEN_ABI EQU #10
FILE_MODE_READ   EQU 1
FILE_MODE_WRITE  EQU 2
FILE_MODE_CREATE EQU 3

	LD IX, FOPEN_ABI
	LD B, FILE_MODE_READ
	LD HL, file_path
	CALL IZ_ENTRY
	JP C, file_open_error
	ld (fp), a
	.....

file_path db "test.scr"
fp 	  db 0
```

### fclose(#11)

Just closes opened file

Arguments:
 
 * A - file number

Returns:
 
 * If no error - flag C is down.
 
 * If there error - flag C is setted and A contains error code

### fread(#12)

Read some bytes to buffer from file

Arguments:
 
 * A - file pointer
 
 * BC - maximum count of bytes to read
 
 * HL - buffer 

Result:

If success:

 * Flag C is down

 * BC - actually read bytes count

 * Bytes in buffer pointed by HL

If error happens:

 * Flag C is setted 

 * A - error code

### fwrite(#13)

Same as fread but writes buffer to disk

### fseek(#14)

Set offset in opened file(or get offset - if you but zero into offset)

Offset modes:

 * 0 - from begin of file

 * 1 - from end of file

 * 2 - from current position 

Arguments:

 * A - file pointer

 * DEBC - offset

 * L - mode

Result:

 * Error info in flag C and register A(similar like in prev. calls)

 * DEBC - current file position from
 
### readdir(#15)

Read current directory listing.

Arguments:

 * A - limit

 * BC - offset(per file)

 * HL - Buffer

 * E - ordering(Do we really need it?)

Result:

 * Error info in flag C and register A(similar as before)

 * BC - returned files count

 * Filenames written to buffer(11 bytes per file without padding if we using 8+3. If we support LFN - maximum 64 bytes file name len, every file name is StingZ)

### Current Working Directory(#16)

Returns working directory.

Arguments:

 * HL - buffer

Return:

 * Error info in flag C and register A(similar as before)

 * Current absolute path(max. 255 bytes)

### Set Working Directory(#17)

Sets working directory

Arguments:

 * HL - pointer to path(can be absolute and relative)

Returns:

 * Error info in flag C and register A(similar as before)

### Set Working Drive(#18) - Do we need it?

Get or set working drive.

Arguments:

 * A - drive number(from 1). If value is zero - return current drive.

Returns:

 * Error info in flag C and register A(similar as before)

### Close all user defined file desciptors(#2F)

Close all opened by user files(can be used as )

## Network ABI

### Open TCP connection(#30) 

Arguments:

 * HL - StringZ to hostname

 * BC - port

Returns:

 * Error info in flag C and register A(similar as before)

 * If no error A will contains socket number

### Close TCP connection(#31)

Arguments:

 * A - socket number

Returns:

 * Error info in flag C and register A

### Get socket status(#32)

Arguments:

 * A - socket number

Returns:

 * A - socket flags:

   - Bit 0 - is connected

   - Bit 1 - is data available 

   - Bit 2 - is data sending

 * BC - data available to fetch(zero if socket closed)

### Socket read(#33)

Arguments:

  * A - socket number

  * BC - bytes count

  * HL - pointer to buffer

Returns:

 * Error info in flag C and register A

 * BC - count of bytes what was actually read

 ### Socket write(#34)

Arguments:

  * A - socket number

  * BC - bytes count

  * HL - pointer to buffer

Returns:

 * Error info in flag C and register A

 * BC - actally sent bytes count

### Bind UDP socket(#35)

Arguments:

 * BC - port

Returns:

 * Error info in flag C and register A(similar as before)

 * If no error A will contains socket number

### UDP sendto(#36)

Arguments:

 * A - socket number

 * BC - bytes count

 * HL - pointer to buffer

 * DE - pointer to ipv4(2 words)+port(1 word) struct

Returns:

 * Error info in flag C and register A

 * BC - actally sent bytes count

### ICMP Ping(#37)

Argumnets:

 * HL - pointer to hostname

 * BC - port

 * A - timeout

Returns:

 * Error info in flag C and register A

 * A - value if no error

### Get Host By Name(#38)

Arguments:
 
 * HL - pointer to host name stringZ

 * DE - pointer to ipaddrv4 buffer(2 words)

Returns:

 * Error info in flag C and register A

### Reset networking(#4F)

Close all active user defined connections(can be used as initialization routine).

Returns:
 
 * Flag C setted if WiFi not connected to access point