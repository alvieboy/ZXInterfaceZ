#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <inttypes.h>

typedef enum {
    READCMD,
    READDATA
} cmdstate_t;


typedef enum {
    COMMAND_CLOSE_OK = 0,
    COMMAND_CONTINUE = 1,
    COMMAND_CLOSE_SILENT = 2,
    COMMAND_CLOSE_ERROR = -1
} command_result_t;

typedef struct command
{
    int socket;
    struct sockaddr_in *source_addr;
    command_result_t (*rxdatafunc)(struct command *cmdt);
    uint8_t tx_prebuffer[4];
    uint8_t rx_buffer[1024];
    unsigned len;
    unsigned romsize;
    unsigned romoffset;
    const char *errstr;
    int reported_progress;
    cmdstate_t state;
} command_t;


struct commandhandler_t {
    const char *cmd;
    uint8_t cmdlen;
    command_result_t (*handler)(command_t*, int argc, char **argv);
};

#define CMD(x) x, __builtin_strlen(x)

#endif
