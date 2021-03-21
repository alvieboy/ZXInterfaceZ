#include "opstatus.h"
#include "interfacez_resources.h"
#include <string.h>

void opstatus__set_status(uint8_t val, const char *str)
{
    opstatus_resource__set_status(&opstatusresource, val, str);
}

void opstatus__set_error(const char *str)
{
    char errstr[31];
    errstr[0] = 'E';
    errstr[1] = ':';
    strlcpy(&errstr[2], str, 28);
    opstatus_resource__set_status(&opstatusresource, OPSTATUS_ERROR, errstr);
}
