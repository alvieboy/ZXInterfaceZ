/*
*  FPPA PDK14 Microcontroller ROM generator tool
*
*  Copyright 2020 Alvaro Lopes <alvieboy@alvie.com>
*
*  The FreeBSD license
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*  1. Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*  2. Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials
*     provided with the distribution.
*
*  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
*  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
*  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
*  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  ZPU PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
*  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
*  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
*  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
*  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
*  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
*  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include <unistd.h>
#include <getopt.h>
#include <cstdio>
#include <vector>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <cstring>
#include <cerrno>

static const char *template_in = NULL;
static const char *binary_in = NULL;
static const char *file_out = NULL;
static unsigned bin_words = 0;

#ifndef O_BINARY
#define O_BINARY 0
#endif

int usage()
{
    fprintf(stderr,"usage: bin2rom -t <templatefile.in> -i <input.bin> -o <outputfile.vhd>\n\n");
    return -1;
}

std::vector<uint8_t> romv;

int loadbin(const char *file)
{
    struct stat st;


    if (stat(file,&st)<0) {
        fprintf(stderr,"Cannot stat %s: %s\n", file, strerror(errno));
        return -1;
    }

    bin_words = st.st_size;

    int f = open(file, O_BINARY| O_RDONLY);

    if (f<0) {
        fprintf(stderr,"Cannot open %s: %s\n", file, strerror(errno));
        return -1;
    }
    romv.reserve(bin_words);

    while(bin_words--) {
        uint8_t v;
        if (read(f,&v,sizeof(v))!=sizeof(v))
            return -1;
        romv.push_back(v);
    }
    printf("Loaded %d words\n", romv.size());
    return 0;
}

void writeoutput(FILE *out)
{
    unsigned address = 0;
    bool first=true;
    for (auto v: romv) {
        if (!first)
            fprintf(out,", \n");
        first =false;
        fprintf(out,"      x\"%02x\"", v);
    }
}

int copyandupdate()
{
    char line[512];
    FILE *fin = fopen(template_in,"r");
    if (NULL==fin) {
        fprintf(stderr, "Cannot open %s: %s\n", template_in, strerror(errno));
        return -1;
    }

    FILE *fout = fopen(file_out,"w");
    if (NULL==fout)
        return -1;
    while (fgets(line,sizeof(line),fin)) {
        if (strstr(line,"-- ROM --")!=NULL) {
            writeoutput( fout );
        } else {
            fputs(line,fout);
        }
    }
    fclose(fout);
    fclose(fin);
    return 0;
}

int main(int argc, char **argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "i:t:o:")) != -1) {
        switch (opt) {
        case 'i':
            binary_in = optarg;
            break;
        case 't':
            template_in = optarg;
            break;
        case 'o':
            file_out = optarg;
            break;
        }
    }

    if (NULL==template_in || NULL==binary_in || NULL==file_out) {
        return usage();
    }

    if (loadbin(binary_in)<0)
        return -1;

    if (copyandupdate()<0) {
        fprintf(stderr,"Cannot create file\n");
        return -1;
    }
    return 0;
}


