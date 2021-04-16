#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include "spectfd.h"
#include "errorapi.h"
#include <string.h>
#include "log.h"

#define TAG "NETWORKAPI"

int networkapi__socket(uint8_t type)
{
    switch (type) {
    case IPPROTO_TCP: /* Fall-through */
    case IPPROTO_UDP: /* Fall-through */
        break;
    default:
        return -EINVAL;
        break;
    }

    spectfd_t sfd = spectfd__prealloc();

    if (sfd<0)
        return sfd;

    int socketfd = socket(AF_INET, type==IPPROTO_TCP?SOCK_STREAM:SOCK_DGRAM, type);
    if (socketfd<0) {
        spectfd__close(sfd);
        return errorapi__from_errno();
    }

    spectfd__update_systemfd(sfd, socketfd);

    return sfd;
}

int networkapi__gethostbyname(const char *name, uint32_t *target)
{
    struct addrinfo hints, *addr_list, *cur;
    memset( &hints, 0, sizeof( hints ) );

    hints.ai_family = AF_INET;

    if( getaddrinfo( name, NULL, &hints, &addr_list ) != 0 ) {
        return -EDESTADDRREQ;
    }

    for( cur = addr_list; cur != NULL; cur = cur->ai_next ) {

        if (cur->ai_family == AF_INET) {
            struct sockaddr_in *sock =(struct sockaddr_in*)cur->ai_addr;
            
            *target = sock->sin_addr.s_addr;

            freeaddrinfo( addr_list );

            return 0;
        } 
    }
    // no match found
    freeaddrinfo( addr_list );
    return -EDESTADDRREQ;
}

int networkapi__connect(spectfd_t fd, uint32_t host, uint16_t port)
{
    struct sockaddr_in s;


    int socketfd = spectfd__spect_to_system(fd);

    if (socketfd<0) {
        ESP_LOGE(TAG, "Invalid Spectrum FD %d", fd);
        return -EINVAL;
    }

    memset(&s,0,sizeof(s));

    s.sin_family = AF_INET;
    s.sin_addr.s_addr = host;
    s.sin_port = htons(port);

    char *h = inet_ntoa(s.sin_addr);

    int result;
    do {
        result = connect(socketfd, (struct sockaddr*)&s, sizeof(struct sockaddr_in));
        if (result>=0)
            break;
#ifdef __linux__
        if (errno!=EINTR)
            break;
#else
        break;
#endif
    } while (1);

    ESP_LOGI(TAG, "Connecting to 0x%08x (%s) port %d: %d", host, h, port, result);
    if (result<0)
        return errorapi__from_errno();

    return result;
}

int networkapi__sendto(spectfd_t fd, uint32_t host, uint16_t port, const uint8_t *data, uint16_t datalen)
{
    struct sockaddr_in s;

    int socketfd = spectfd__spect_to_system(fd);

    if (socketfd<0)
        return -EINVAL;

    int result;
    // If host is all zeros, use "send" not sendto.
    if (host==0) {
        result = send(socketfd, data, datalen,
                      0);
    } else {
        memset(&s,0,sizeof(s));

        s.sin_family = AF_INET;
        s.sin_addr.s_addr = host;
        s.sin_port = htons(port);

        result = sendto(socketfd, data, datalen,
                        0, // Flags
                        (struct sockaddr*)&s, sizeof(struct sockaddr_in));
    }

    if (result<0)
        return errorapi__from_errno();

    return result;
}

