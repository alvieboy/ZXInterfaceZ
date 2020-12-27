#include "json.h"
#include "fileaccess.h"
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

cJSON *json__load_from_file(const char *filename)
{
    cJSON *root = NULL;
    char *buf = NULL;

    int f = __open(filename, O_RDONLY);
    if (f<0)
        return NULL;
    off_t size = lseek(f,0,SEEK_END);
    lseek(f,0,SEEK_SET);
    if (size<=0) {
        close(f);
        return NULL;
    }


    buf = malloc(size);
    if (buf!=NULL) {
        if (read(f, buf,size)!=size) {
            close(f);
        } else {
            root = cJSON_Parse(buf);
        }
    }
    if (buf) {
        free(buf);
    }
    return root;
}

const char *json__get_string(cJSON*node, const char*name)
{
    cJSON *n = cJSON_GetObjectItemCaseSensitive(node, name);
    if (cJSON_IsString(n)) {
        return n->valuestring;
    }
    return NULL;
}

char *json__get_string_alloc(cJSON*node, const char *name)
{
    const char *s = json__get_string(node,name);
    if (NULL==s)
        return NULL;
    return strdup(s);
}

int json__get_ip(cJSON*node, const char *name, ip4_addr_t *addr)
{
    const char *ips  = json__get_string(node,name);
    if (!ips)
        return -1;

    return ip4addr_aton(ips,addr);
}
