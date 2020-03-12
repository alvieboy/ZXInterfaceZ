#include "netcomms.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include <string.h>

void netcomms__send_ok(int sock)
{
    send(sock, "OK\n", 3,  0);
}

void netcomms__send_error(int sock, const char *errorstr)
{
    send(sock, "ERROR:", 3,  MSG_MORE);
    if (errorstr!=NULL) {
        send(sock, errorstr, strlen(errorstr),  MSG_MORE);
    } else {
        send(sock, "Unspecified error", 17,  MSG_MORE);
    }
    send(sock, "\n", 1,  0);
}

void netcomms__send_progress(int sock, int total, int current)
{
    char tmp[64];
    char l = sprintf(tmp,"P:%d/%d\n", total, current);
    send(sock, tmp, l, 0);
}
