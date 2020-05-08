#ifndef __SPECTCMD_H__
#define __SPECTCMD_H__

#define SPECTCMD_CMD_GETRESOURCE (0x00)
#define SPECTCMD_CMD_SETAP (0x01)
#define SPECTCMD_CMD_STARTSCAN (0x02)
#define SPECTCMD_CMD_SAVESNA (0x05)

void spectcmd__request(void);
void spectcmd__init(void);

#endif
