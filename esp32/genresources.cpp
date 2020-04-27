#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <vector>
#include <string>
#include <inttypes.h>
extern "C" {
#include <pam.h>
#include <pbm.h>
}
static const char *inputlist = "resourcelist.txt";
static const char *outputfile = "resource.bin";

#ifdef __unix__
#define O_BINARY (0)
#endif


struct res {
    uint8_t id;
    std::string type;
    std::string name;
};

static std::vector<struct res> resourcelist;

int help()
{
    fprintf(stderr, "Usage: genresources [-i <resourcelist.txt>] [-o <resource.bin>]\n\n");
    return -1;
}

void chomp(char *line)
{
    char *e;
    while ((e=strrchr(line,'\n'))!=NULL) {
        *e='\0';
    }
    while ((e=strrchr(line,'\r'))!=NULL) {
        *e='\0';
    }
}

int append_resource(int outfile, struct res &r)
{
    int fin = open(r.name.c_str(), O_RDONLY);
    if (fin<0) {
        fprintf(stderr,"Cannot open %s: %s\n", r.name.c_str(), strerror(errno));
        return -1;
    }
    off_t size = lseek(fin, 0, SEEK_END);
    lseek(fin, 0, SEEK_SET);
    if ((size<0) || (size>65535)) {
        return -1;
    }
    uint8_t len[2];
    len[0] = size & 0xff;
    len[1] = (size>>8) & 0xff;

    if (write(outfile, len, sizeof(len))!=sizeof(len)) {
        perror("write len");
        return -1;
    }
    return 0;
}

int generate_pbm_bitmap(int outfile, const std::string &filename)
{
    struct pam in_pbm;
    uint16_t resource_size;

    tuple * tuplerow;
    int row;
    uint8_t sizes[2];

    pm_init("genresources", 0);

    FILE *in = fopen(filename.c_str(),"r");

    pnm_readpaminit(in, &in_pbm, sizeof(in_pbm.tuple_type));

    printf("%d %d %d\n",
           in_pbm.width,
           in_pbm.height,
           in_pbm.depth);

    if (in_pbm.depth!=1) {
        fprintf(stderr,"Only bitmap images supported\n");
        return -1;
    }

    if ((in_pbm.width %8) !=0 ) {
        fprintf(stderr,"Only bitmaps with 8*x pixels are supported\n");
        return -1;
    }

    resource_size = 2+((in_pbm.width>>3)*in_pbm.height);

    
    uint8_t id = 0x02;
    if (write(outfile, &id, sizeof(id))!=sizeof(id)) {
        perror("write");
        return -1;
    }


    sizes[0] = resource_size & 0xff;
    sizes[1] = (resource_size>>8) & 0xff;

    /* Write resource size */

    if (write(outfile, sizes, sizeof(sizes))!=sizeof(sizes)) {
        perror("write");
        return -1;
    }




    sizes[0] = in_pbm.width>>3;
    sizes[1] = in_pbm.height;

    if (write(outfile, sizes, sizeof(sizes))!=sizeof(sizes)) {
        perror("write");
        return -1;
    }

    tuplerow = pnm_allocpamrow(&in_pbm);

    for (row = 0; row < in_pbm.height; ++row) {
       int column;
       pnm_readpamrow(&in_pbm, tuplerow);

/*       {
           for (int i=0;i<256;i++) {
               printf("%02x ", tuplerow[i][0]);
           }
           printf("\n");
       }
  */
       uint8_t pixel_byte = 0;

       for (column = 0; column < in_pbm.width; ++column) {
           pixel_byte<<=1;

           pixel_byte |= !tuplerow[column][0];

           if ((column & 7)==7)  {
               if (write(outfile, &pixel_byte, sizeof(pixel_byte))!=sizeof(pixel_byte)) {
                   perror("write");
                   return -1;
               }
           }
       }
    }
    fclose(in);

    return 0;
}

int generate_resources()
{
    int outfile = open(outputfile, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, 0666);
    if (outfile<0) {
        fprintf(stderr,"Cannot open %s: %s\n", inputlist, strerror(errno));
        return -1;
    }

    /* write number of resources */
    uint16_t numresources = resourcelist.size();
    uint8_t rbuf[2];
    rbuf[0] = numresources & 0xff;
    rbuf[1] = (numresources>>8) & 0xff;
    if (write(outfile, rbuf, sizeof(rbuf))!=sizeof(rbuf)) {
        perror("write");
        return -1;
    }
    for (auto i: resourcelist) {
        uint8_t id = i.id;
        if (write(outfile, &id, sizeof(id))!=sizeof(id)) {
            perror("write");
            return -1;
        }
        // According to type:
        if (i.type == "pbm") {
            generate_pbm_bitmap(outfile, i.name);
        } else {
            if (append_resource(outfile, i)<0) {
                fprintf(stderr, "Cannot append resource\n");
                return -1;
            }
        }
    }


    return close(outfile);
}

int register_resource(char *line)
{
    char *f = line;
    char *endp;
    f = strpbrk(line," \t");
    if (NULL==f)
        return -1;
    *f = '\0';
    f++;
    while (*f && isspace(*f)) {
        f++;
    }


    // Load id
    unsigned long id = strtoul(line, &endp, 0);
    if (*endp!='\0') {
        return -1;
    }
    if (id>255) {
        return -1;
    }
    struct res r;
    r.id = id;
    r.type = "pbm";
    r.name = f;

    resourcelist.push_back( r );
    return 0;
}

int run()
{
    char line[128];
    FILE *in = fopen(inputlist,"r");

    if (in==NULL) {
        fprintf(stderr,"Cannot open %s: %s\n", inputlist, strerror(errno));
        return -1;
    }
    while (fgets(line, sizeof(line),in)) {
        chomp(line);
        char *lptr = line;
        while (*lptr && isspace(*lptr)) {
            lptr++;
        }
        if (*lptr=='#')
            continue;
        if (register_resource(lptr)<0) {
            printf("Cannot register resource '%s'\n", lptr);
            return -1;
        }
    }
    fclose(in);

    printf("%ld resources\n", resourcelist.size());

    return generate_resources();
}

int main(int argc, char **argv)
{
    int c;
    while ((c=getopt(argc, argv,"i:o:"))!=-1) {
        switch (c) {
        case 'i':
            inputlist = optarg;
            break;
        case 'o':
            outputfile = optarg;
            break;
        default:
            return help();
        }
    }
    return run();
}
