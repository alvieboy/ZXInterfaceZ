#include "VCDWriter.h"

#include <fcntl.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <QString>

VCDWriter::VCDWriter()
{
    period = 10.41;
}

static void printbin(FILE *f, uint32_t data, unsigned size)
{
    int mask = 1<<(size-1);
    fprintf(f,"b");
    while (size--) {
        fprintf(f,"%d", data&mask?1:0);
        data<<=1;
    }
}

int VCDWriter::write(QByteArray *data, const QString &file)
{
    FILE *out = fopen(file.toLatin1().constData(),"w");
    if (!out) {
        return -1;
    }
    if (write(data,out)<0)
        return -1;

    return fclose(out);
}

int VCDWriter::write(QByteArray *b, FILE*f)//const QString &file)
{
    unsigned offset = 0;
    uint8_t *cdata = (uint8_t*)b->constData();
    unsigned len = b->length();

    fprintf(f,"$date\n"
            "  Tue Dec  4 19:27:08 2012\n"
            "$end\n"
            "$version\n"
            "  GHDL v0\n"
            "$end\n"
            "$timescale\n"
            "  %lf ns\n"
            "$end\n", period);
    fprintf(f,"$var wire 1 a MREQ $end\n"
           "$var wire 1 b IORQ $end\n"
           "$var wire 1 c RD $end\n"
           "$var wire 1 d WR $end\n"
           "$var wire 16 e A $end\n"
           "$var wire 8 f D $end\n"
           "$var wire 1 g CK $end\n"
           "$enddefinitions $end\n");

    // #0
    uint32_t time = 0;

    while (len>4) {
        const uint8_t *data = &cdata[offset];
        offset+=5;
        len-=5;
        printf("%02x%02x%02x%02x%02x\n",
               data[0],
               data[1],
               data[2],
               data[3],
               data[4]);


        uint8_t d = data[4];
        uint16_t a = (((uint16_t)data[2])<<8) + data[3];
        uint8_t rep = ((uint16_t)data[0] << 4) + ( data[1]>>5);

        uint8_t ck   = !!(data[1] & 0x10);
        uint8_t wrn   = data[1] & 1;
        uint8_t rdn   = !!(data[1] & 2);
        uint8_t iorqn = !!(data[1] & 4);
        uint8_t mreqn = !!(data[1] & 8);

        time += 1 * ((uint32_t)rep+1);
        fprintf(f,"#%d\n", time);
        fprintf(f,"b%d a\n", mreqn);
        fprintf(f,"b%d b\n", iorqn);
        fprintf(f,"b%d c\n", rdn);
        fprintf(f,"b%d d\n", wrn);
        printbin(f,a,16); fprintf(f," e\n");
        printbin(f,d,8); fprintf(f," f\n");
        fprintf(f,"b%d g\n", ck);

    }
    return 0;
}
