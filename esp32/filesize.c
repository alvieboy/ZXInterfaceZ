#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

struct fheader
{
    uint32_t size;
    uint32_t crc;
};

int copy(int fin, int fout, int size, uint32_t *crc)
{
    char buf[8192];
    while (size) {
        int chunk = size>sizeof(buf)?sizeof(buf):size;
        read(fin,buf,chunk);
        write(fout,buf,chunk);
        size-=chunk;
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct stat st;
    int fout;
    struct fheader hdr;

    if (argc<2)
        return -1;

    int fin  =open(argv[1], O_RDONLY);
    if (fin<0) {
        fprintf(stderr,"Cannot open %s: %s\n", argv[1], strerror(errno));
        return -1;
    }

    if (fstat(fin, &st)<0) {
        fprintf(stderr,"Cannot stat %s: %s\n", argv[1], strerror(errno));
        close(fin);
        return -1;
    }

    fout = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fout<0) {
        fprintf(stderr,"Cannot open %s for writing: %s\n", argv[2], strerror(errno));
        return -1;
    }
    hdr.size = st.st_size;
    hdr.crc = 0xFFFFFFFF;

    write(fout, &hdr, sizeof(hdr));

    copy(fin,fout,st.st_size, &hdr.crc);

    close(fin);
    close(fout);
}
