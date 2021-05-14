#include <inttypes.h>
#include <stdio.h>

#include "rle.h"
#include "string.h"

int writes16(int16_t val, FILE*out)
{
    fwrite(&val, 2, 1, out);
    return 0;
}

int rle_compress(FILE *inFile, FILE *outFile)
{
    int currChar;                       /* current character */
    unsigned char charBuf[MAX_READ];    /* buffer of already read characters */
    unsigned int count;                /* number of characters in a run */

    /* prime the read loop */
    currChar = fgetc(inFile);
    count = 0;

    /* read input until there's nothing left */
    while (currChar != EOF)
    {
        charBuf[count] = (unsigned char)currChar;
        count++;
        if (count >= MIN_RUN)
        {
            int i;
            /* check for run  charBuf[count - 1] .. charBuf[count - MIN_RUN]*/
            for (i = 2; i <= MIN_RUN; i++)
            {
                if (currChar != charBuf[count - i])
                {
                    /* no run */
                    i = 0;
                    break;
                }
            }

            if (i != 0)
            {
                /* we have a run write out buffer before run*/
                int nextChar;

                if (count > MIN_RUN)
                {
                    /* block size - 1 followed by contents */
                    //fputc(count - MIN_RUN - 1, outFile);
                    writes16((count-MIN_RUN)-1,outFile);
                    //printf("Block real size %d\n", count-MIN_RUN);
                    fwrite(charBuf, sizeof(unsigned char), count - MIN_RUN,
                        outFile);
                }

                /* determine run length (MIN_RUN so far) */
                count = MIN_RUN;

                while ((nextChar = fgetc(inFile)) == currChar)
                {
                    count++;
                    if (MAX_RUN == count)
                    {
                        /* run is at max length */
                        break;
                    }
                }

                /* write out encoded run length and run symbol */
                //fputc((char)((int)(MIN_RUN - 1) - (int)(count)), outFile);
                writes16((int)(MIN_RUN - 1) - (int)(count),outFile);
                fputc(currChar, outFile);

                if ((nextChar != EOF) && (count != MAX_RUN))
                {
                    /* make run breaker start of next buffer */
                    charBuf[0] = nextChar;
                    count = 1;
                }
                else
                {
                    /* file or max run ends in a run */
                    count = 0;
                }
            }
        }

        if (MAX_READ == count)
        {
            int i;

            /* write out buffer */
            //fputc(MAX_COPY - 1, outFile);
            writes16(MAX_COPY-1, outFile);
            fwrite(charBuf, sizeof(unsigned char), MAX_COPY, outFile);

            /* start a new buffer */
            count = MAX_READ - MAX_COPY;

            /* copy excess to front of buffer */
            for (i = 0; i < count; i++)
            {
                charBuf[i] = charBuf[MAX_COPY + i];
            }
        }

        currChar = fgetc(inFile);
    }

    /* write out last buffer */
    if (0 != count)
    {
        if (count <= MAX_COPY)
        {
            /* write out entire copy buffer */
            writes16(count - 1, outFile);
            fwrite(charBuf, sizeof(unsigned char), count, outFile);
        }
        else
        {
            /* we read more than the maximum for a single copy buffer */
            writes16(MAX_COPY - 1, outFile);
            fwrite(charBuf, sizeof(unsigned char), MAX_COPY, outFile);

            /* write out remainder */
            count -= MAX_COPY;
            writes16(count - 1, outFile);
            fwrite(&charBuf[MAX_COPY], sizeof(unsigned char), count, outFile);
        }
    }

    return 0;
}


int main(int argc, char **argv)
{
    FILE *in;
    FILE *out;
    if (argc<3)
        return -1;

    if (strcmp(argv[1], "-")==0) {
        in = stdin;
    } else {
        in = fopen(argv[1],"r");
    }
    if (strcmp(argv[2], "-")==0) {
        out = stdout;
    } else {
        out = fopen(argv[2],"w");
    }
    rle_compress(in, out);
    if (in!=stdin)
        fclose(in);
    fflush(out);
    if (out!=stdout)
        fclose(out);
}
