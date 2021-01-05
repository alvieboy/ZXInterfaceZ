#ifndef __ESXDOS_H__
#define __ESXDOS_H__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESXDOS_OPEN_FILEMODE_READ 0x01
#define ESXDOS_OPEN_FILEMODE_WRITE 0x02
#define ESXDOS_OPEN_FILEMODE_CREAT_NOEXIST 0x04
#define ESXDOS_OPEN_FILEMODE_CREAT_TRUNC 0x08

int esxdos__diskinfo(uint8_t drive);
int esxdos__driveinfo(int8_t drive);
int esxdos__open(const char *filename, uint8_t mode);
int esxdos__read(uint8_t handle, uint16_t len);
int esxdos__close(uint8_t handle);

#ifdef __cplusplus
}
#endif

#endif
