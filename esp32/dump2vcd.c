#include <fcntl.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

// 0x00_6F_00_00_zz
double period = 10.41;

void printbin(uint32_t data, unsigned size)
{
    int mask = 1<<(size-1);
    printf("b");
    while (size--) {
        printf("%d", data&mask?1:0);
        data<<=1;
    }
}

int main(int argc, char **argv)
{
    uint8_t data[5];
    int fd = open(argv[1], O_RDONLY);
    if (fd<0) {
        perror("open");
        return -1;
    }
    printf("$date\n"
           "  Tue Dec  4 19:27:08 2012\n"
           "$end\n"
           "$version\n"
           "  GHDL v0\n"
           "$end\n"
           "$timescale\n"
           "  %lf ns\n"
           "$end\n", period);
    printf("$var wire 1 a MREQ $end\n"
           "$var wire 1 b IORQ $end\n"
           "$var wire 1 c RD $end\n"
           "$var wire 1 d WR $end\n"
           "$var wire 16 e A $end\n"
           "$var wire 8 f D $end\n"
           "$var wire 1 g CK $end\n"
           "$enddefinitions $end\n");
    // #0
    uint32_t time = 0;

    while (read(fd,data,sizeof(data))==5) {

        uint8_t d = data[4];
        uint16_t a = (((uint16_t)data[2])<<8) + data[3];
        uint8_t rep = ((uint16_t)data[0] << 4) + ( data[1]>>5);

        uint8_t ck   = !!(data[1] & 0x10);
        uint8_t wrn   = data[1] & 1;
        uint8_t rdn   = !!(data[1] & 2);
        uint8_t iorqn = !!(data[1] & 4);
        uint8_t mreqn = !!(data[1] & 8);

        time += 1 * ((uint32_t)rep+1);
        printf("#%d\n", time);
        printf("b%d a\n", mreqn);
        printf("b%d b\n", iorqn);
        printf("b%d c\n", rdn);
        printf("b%d d\n", wrn);
        printbin(a,16); printf(" e\n");
        printbin(d,8); printf(" f\n");
        printf("b%d g\n", ck);

        //  XMREQ_sync_s & XIORQ_sync_s & XRD_sync_s & XWR_sync_s & XA_sync_s & XD_sync_s;
        /*printf("%02x%02x%02x%02x%02x %03d IORQ=%d MEMRQ=%d RD=%d WR=%d A=%04x D=%02x\n",
               data[0],
               data[1],
               data[2],
               data[3],
               data[4],
               rep,
               iorqn, mreqn, rdn, wrn, a, d);
               */
    }

}
