#include "errorapi.h"
#include <errno.h>

int errorapi__from_errno()
{
    int err = errno;
    if (err<128) {
        return -err;
    }
    return -1;
}
