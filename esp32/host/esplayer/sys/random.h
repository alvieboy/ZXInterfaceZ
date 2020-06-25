#ifndef _SYS_RANDOM_H
#define _SYS_RANDOM_H 1

#include <features.h>
#include <sys/types.h>

/* Flags for use with getrandom.  */
#define GRND_NONBLOCK 0x01
#define GRND_RANDOM 0x02

__BEGIN_DECLS

/* Write LENGTH bytes of randomness starting at BUFFER.  Return the
   number of bytes written, or -1 on error.  */
ssize_t getrandom (void *__buffer, size_t __length,
                   unsigned int __flags) __wur;

/* Write LENGTH bytes of randomness starting at BUFFER.  Return 0 on
   success or -1 on error.  */
int getentropy (void *__buffer, size_t __length) __wur;

__END_DECLS

#endif /* _SYS_RANDOM_H */
