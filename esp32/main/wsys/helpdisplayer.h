#pragma once

class HelpDisplayer
{
public:
    virtual void displayHelpText(const char *c) = 0;
};

#if 0
LOCAL int ICACHEFUN(getNumberOfPrintableChars)(const char *str, int *offset)
{
    int skip;
    int count = 0;
    const char *lastspace = NULL;

    unsigned font_width = 5;

    int maxwidth = width();

    /* Skip spaces first? */

    *offset = 0;

    DEBUG("Maxw %d\n", maxwidth);

    if (*str==0)
        return 0;

    do {
        const char *save = str;
        skip = 0;

        *offset += (str-save);

        if (maxwidth>=0) {
            if (maxwidth < font_width) {

                if (lastspace) {
                    //printf("lastspace\n");
                    lastspace++;
                    //printf("will restart at '%s'\n",lastspace);
                    count-=(str-lastspace);
                }
                return count;
            }
            if (isspace(*str))
                lastspace = str;
        }
        count++;
        str++;
        if (maxwidth>0) {
            maxwidth-=font_width;
        }
    } while (*str);
    *offset+=count;
    return count;
}


LOCAL int ICACHEFUN(textComputeLength)(const char *str, const textrendersettings_t *s, int *width, int *height)
{
    int i;
    int maxw = 0;
    int maxh = 0;
    int offset;
    DEBUG("Computing length");
    do {
        DEBUG("Str: '%s' start 0x%02x\n", str, str[0]);
        i = getNumberOfPrintableChars(str, s, &offset);
        DEBUG("Printable chars: %d, offset %d\n", i, offset);
        if (i<0) {
            return -1;
        }
        maxh++;
        str+=offset;
        DEBUG("Str now: '%s'\n", str);
        if (maxw<i)
            maxw=i;
    } while (*str);
    *width = (maxw * s->font->hdr.w);// + (maxw-1);  // Spacing between chars
    *height = (maxh * s->font->hdr.h) + (maxh-1); // Spacing between chars
    return 0;
}
#endif // 0
