/*
 Taken from colorized-logs (https://raw.githubusercontent.com/kilobyte/colorized-logs/master/ansi2html.c)
 Modified to fit purpose

 License: MIT

 Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/


#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#define BOLD         0x010000
#define DIM          0x020000
#define ITALIC       0x040000
#define UNDERLINE    0x080000
#define BLINK        0x100000
#define INVERSE      0x200000
#define STRIKE       0x400000

typedef struct {
    bool in_span;
    int fg, bg, fl, frgb, brgb;
    const char *input;
    char *output;
    int maxlen;
} ansi2html_context_t;

static void ansi2html__printf(ansi2html_context_t *ctx, const char *fmt, ...)
{
    va_list ap;

    if (ctx->maxlen==0)
        return;

    va_start(ap, fmt);
    int printed = vsnprintf(ctx->output, ctx->maxlen, fmt, ap);
    va_end(ap);
    ctx->maxlen -= printed;
    ctx->output += printed;

}

static const char *cols[]={"BLK","RED","GRN","YEL","BLU","MAG","CYN","WHI",
                           "HIK","HIR","HIG","HIY","HIB","HIM","HIC","HIW"};

typedef unsigned char u8;

static bool white = false;
static bool contrast = false;

static int rgb_from_256(int i)
{
    if (i < 16)
    {   /* Standard colours. */
        if (white)
        {
            if (i == 3)
                return 0x806000;
            if (i == 3+8)
                return 0xcccc00;
            int c = (i&1 ? 0x010000 : 0x000000)
                  | (i&2 ? 0x000100 : 0x000000)
                  | (i&4 ? 0x000001 : 0x000000);
            return i<8 ? c*0x80 : c*0xff;
        }
        if (i == 3)
            return 0xaa5500;
        int c = (i&1 ? 0xaa0000 : 0x000000)
              | (i&2 ? 0x00aa00 : 0x000000)
              | (i&4 ? 0x0000aa : 0x000000);
        return i<8 ? c : c+0x555555;
    }
    else if (i < 232)
    {   /* 6x6x6 colour cube. */
        i-=16;
        int r = i / 36, g = i / 6 % 6, b = i % 6;
        return (r ? r * 0x280000 + 0x370000 : 0)
             | (g ? g * 0x002800 + 0x003700 : 0)
             | (b ? b * 0x000028 + 0x000037 : 0);
    }
    else/* Grayscale ramp. */
        return i*0xa0a0a-((232*10-8)*0x10101);
}


static inline int rgb_to_int(u8 r, u8 g, u8 b)
{
    return (int)r<<16|(int)g<<8|(int)b;
}


static int get_frgb(int fg)
{
    return fg==-1?white?0x000000:0xaaaaaa:rgb_from_256(fg);
}


static int get_brgb(int bg)
{
    return bg==-1?white?0xaaaaaa:0x000000:rgb_from_256(bg);
}


static void span(ansi2html_context_t *ctx)
{
    int tmp, _fg=ctx->fg, _bg=ctx->bg, _frgb=ctx->frgb, _brgb=ctx->brgb;
    char clbuf[32], *cl=clbuf;

    if (ctx->fg==-1 && ctx->bg==-1 && ctx->frgb==-1 && ctx->brgb==-1 && !ctx->fl)
        return;
    if (ctx->fl&INVERSE)
    {
        if (_fg==-1)
            _fg=white?0:7;
        if (_bg==-1)
            _bg=white?7:0;
        tmp=_fg; _fg=_bg; _bg=tmp;
        tmp=_frgb; _frgb=_brgb; _brgb=tmp;
    }
    if (ctx->fl&BLINK)
    {
        if (_frgb==-1)
            _frgb=get_frgb(_fg);
        _frgb=rgb_to_int((_frgb>>16&0xff)*3/4,
                         (_frgb>> 8&0xff)*3/4,
                         (_frgb    &0xff)*3/4)+0x606060;
        if (_brgb==-1)
            _brgb=get_brgb(_bg);
        _brgb=rgb_to_int((_brgb>>16&0xff)*3/4,
                         (_brgb>> 8&0xff)*3/4,
                         (_brgb    &0xff)*3/4)+0x606060;
    }
    if (ctx->fl&DIM)
        _fg=8;

    if (contrast && (_frgb==-1?get_frgb(_fg):_frgb)==(_brgb==-1?get_brgb(_bg):_brgb))
    {
        if (_frgb==-1)
            _frgb=get_frgb(_fg);
        _frgb^=0x808080;
    }

#if 0
    if (ctx->no_header)
        goto do_span;

    ansi2html__printf("<b"); // Redefining <b> helps Braille and speech readers.
    if (_fg!=-1)
    {
        if (ctx->fl&BOLD)
            _fg|=8;
        cl+=sprintf(cl, " %s", cols[_fg]);
    }
    else if (ctx->fl&BOLD)
        cl+=sprintf(cl, " BOLD");

    if (_bg!=-1)
        cl+=sprintf(cl, " B%s", cols[_bg]);

    if (fl&ITALIC)
        cl+=sprintf(cl, " ITA");
    if (fl&UNDERLINE)
        cl+=sprintf(cl, (fl&STRIKE)?" UNDSTR":" UND");
    else if (fl&STRIKE)
        cl+=sprintf(cl, " STR");

    if (cl>clbuf)
    {
        *cl=0;
        if (cl>=clbuf+5)
            ansi2html__printf(ctx," class=\"%s\"", clbuf+1);
        else /* implies no spaces */
            ansi2html__printf(ctx," class=%s", clbuf+1);
    }

    if (_frgb!=-1 || _brgb!=-1)
    {
        ansi2html__printf(ctx," style=\"");
        if (_frgb!=-1)
            ansi2html__printf(ctx,"color:#%06x;", _frgb);
        if (_brgb!=-1)
            ansi2html__printf(ctx,"background-color:#%06x", _brgb);
        ansi2html__printf(ctx,"\"");
    }

    ansi2html__printf(ctx,">");
    ctx->in_span=1;
    return;
#endif

do_span:
    ansi2html__printf(ctx,"<span style=\"");
    if (_frgb!=-1)
    {
        ansi2html__printf(ctx,"color:#%06x", _frgb);
        if (ctx->fl&BOLD)
            ansi2html__printf(ctx,";font-weight:bold");
    }
    else if (_fg!=-1)
    {
        if (ctx->fl&BOLD)
            ansi2html__printf(ctx,"color:#%c%c%c", _fg&1?'f':'5', _fg&2?'f':'5', _fg&4?'f':'5');
        else
            ansi2html__printf(ctx,"color:#%c%c%c", _fg&1?'a':'0', _fg&2?'a':'0', _fg&4?'a':'0');
    }
    else if (ctx->fl&BOLD)
        ansi2html__printf(ctx,"font-weight:bold");

    if (_brgb!=-1)
        ansi2html__printf(ctx,";background-color:#%06x", _brgb);
    else if (_bg!=-1)
        ansi2html__printf(ctx,";background-color:#%c%c%c", _bg&1?'a':'0', _bg&2?'a':'0', _bg&4?'a':'0');

    if (ctx->fl&ITALIC)
        ansi2html__printf(ctx,";font-style:italic");
    if (ctx->fl&(UNDERLINE|BLINK|STRIKE))
    {
        ansi2html__printf(ctx,";text-decoration:");
        if (ctx->fl&UNDERLINE)
            ansi2html__printf(ctx," underline");
        if (ctx->fl&BLINK)
            ansi2html__printf(ctx," blink");
        if (ctx->fl&STRIKE)
            ansi2html__printf(ctx," line-through");
    }

    ansi2html__printf(ctx,"\">");
    ctx->in_span=1;
}




static void unspan(ansi2html_context_t *ctx)
{
    if (ctx->in_span)
        ansi2html__printf(ctx,"</span>");
    ctx->in_span=0;
}


static void print_string(ansi2html_context_t *ctx, const char *restrict str)
{
    for (; *str; str++)
        switch (*str)
        {
        case '<':
            ansi2html__printf(ctx,"&lt;");
            break;
        case '>':
            ansi2html__printf(ctx,"&gt;");
            break;
        case '&':
            ansi2html__printf(ctx,"&amp;");
            break;
        default:
            ansi2html__printf(ctx, "%c", str);
        }
}


int ansi2html__inchar(ansi2html_context_t *ctx)
{
    char c = *(ctx->input);
    ctx->input++;
    return c == '\0' ? EOF: c;
}

static int ansi2html__convert(ansi2html_context_t *ctx)
{
    ansi2html__printf(ctx, "<pre>");

    ctx->fg=ctx->bg=-1;
    ctx->fl=0;
    ctx->in_span=false;
    ctx->frgb=ctx->brgb=-1;
    unsigned int ntok, tok[16];

    int ch=ansi2html__inchar(ctx);

normal:

    switch (ch)
    {
    case EOF:
        unspan(ctx);
        ansi2html__printf(ctx, "</pre>\n");
        return 0;

    case 0:  case 1:  case 2:  case 3:  case 4:  case 5:  case 6:
                               case 11:                   case 14: case 15:
    case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23:
    case 24: case 25: case 26:          case 28: case 29: case 30: case 31:
        ansi2html__printf(ctx, "&#x24%02X;", ch);
        ch=ansi2html__inchar(ctx);
        goto normal;
    case 7:
        ansi2html__printf(ctx, "&#x266A;");     /* bell */
        ch=ansi2html__inchar(ctx);
        goto normal;
    case 8:
        ansi2html__printf(ctx, "&#x232B;");     /* backspace */
        ch=ansi2html__inchar(ctx);
        goto normal;
    case 12:                    /* form feed */
    formfeed:
        ch=ansi2html__inchar(ctx);
        unspan(ctx);
        ansi2html__printf(ctx, "\n<hr>\n");
        goto normal;
    case 13:
        ch=ansi2html__inchar(ctx);
        unspan(ctx);
        if (ch!=10)
            ansi2html__printf(ctx, "&crarr;\n");
        goto normal;
    case 27:                    /* ESC */
        ch=ansi2html__inchar(ctx);
        goto esc;
    case '<':
        ansi2html__printf(ctx, "&lt;");
        ch=ansi2html__inchar(ctx);
        goto normal;
    case '>':
        ansi2html__printf(ctx, "&gt;");
        ch=ansi2html__inchar(ctx);
        goto normal;
    case '&':
        ansi2html__printf(ctx, "&amp;");
        ch=ansi2html__inchar(ctx);
        goto normal;
    case 127:
        ansi2html__printf(ctx, "&#x2326;");     /* delete */
        ch=ansi2html__inchar(ctx);
        goto normal;
    default:
        ansi2html__printf(ctx, "%c",ch);
        ch=ansi2html__inchar(ctx);
        goto normal;
    }
/****************************************************************************/
esc:
    switch (ch)
    {
    case '[':
        break;
    case ']':
        ch=ansi2html__inchar(ctx);
        if (ch<'0'||ch>'9') /* not an OSC, don't try to parse */
            goto normal;
        for (;;ch=ansi2html__inchar(ctx))
            switch (ch)
            {
            case 27:
                ch=ansi2html__inchar(ctx); /* want ESC \ but we accept ESC anything */
                // fallthru
            case 7:
                ch=ansi2html__inchar(ctx); /* BELL is the alternate terminator */
                // fallthru
            case EOF:
                goto normal;
            }
    case '%':
        ch=ansi2html__inchar(ctx);
        // fallthru
    default:
        ch=ansi2html__inchar(ctx);
        goto normal;
    }
    /* [ */
    ch=ansi2html__inchar(ctx);
    ntok=0;
    tok[0]=0;
/****************************************************************************/
csi:
    switch (ch)
    {
    case '?':
        ch=ansi2html__inchar(ctx);
        goto csiopt;
    case ';':
        if (++ntok>=sizeof(tok)/sizeof(tok[0]))
            goto normal;        /* too many tokens, something is fishy */
        tok[ntok]=0;
        ch=ansi2html__inchar(ctx);
        goto csi;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        tok[ntok]=tok[ntok]*10+ch-'0';
        ch=ansi2html__inchar(ctx);
        goto csi;
    case 'm':
        for (unsigned int i=0;i<=ntok;i++)
            switch (tok[i])
            {
            case 0:
                ctx->fg=ctx->bg=-1;
                ctx->fl=0;
                ctx->frgb=ctx->brgb=-1;
                break;
            case 1:
                ctx->fl|=BOLD;
                ctx->fl&=~DIM;
                break;
            case 2:
                ctx->fl|=DIM;
                ctx->fl&=~BOLD;
                break;
            case 3:
                ctx->fl|=ITALIC;
                break;
            case 4:
                ctx->fl|=UNDERLINE;
                break;
            case 5:
                ctx->fl|=BLINK;
                break;
            case 7:
                ctx->fl|=INVERSE;
                break;
            case 9:
                ctx->fl|=STRIKE;
                break;
            case 21:
            case 22:
                ctx->fl&=~(BOLD|DIM);
                break;
            case 23:
                ctx->fl&=~ITALIC;
                break;
            case 24:
                ctx->fl&=~UNDERLINE;
                break;
            case 25:
                ctx->fl&=~BLINK;
                break;
            case 27:
                ctx->fl&=~INVERSE;
                break;
            case 29:
                ctx->fl&=~STRIKE;
                break;
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                ctx->fg=tok[i]-30;
                ctx->frgb=-1;
                break;
            case 38:
                i++;
                if (i>ntok)
                    break;
                if (tok[i]==5 && i<ntok)
                {   /* 256 colours */
                    i++;
                    ctx->frgb=rgb_from_256(tok[i]);
                }
                else if (tok[i]==2 && i+3<=ntok)
                {   /* 24 bit */
                    ctx->frgb=rgb_to_int(tok[i+1], tok[i+2], tok[i+3]);
                    i+=3;
                }
                /* Subcommands 3 (CMY) and 4 (CMYK) are so insane
                 * there's no point in supporting them.
                 */
                break;
            case 39:
                ctx->fg=-1;
                ctx->frgb=-1;
                break;
            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                ctx->bg=tok[i]-40;
                ctx->brgb=-1;
                break;
            case 48:
                i++;
                if (i>ntok)
                    break;
                if (tok[i]==5 && i<ntok)
                {   /* 256 colours */
                    i++;
                    ctx->brgb=rgb_from_256(tok[i]);
                }
                else if (tok[i]==2 && i+3<=ntok)
                {   /* 24 bit */
                    ctx->brgb=rgb_to_int(tok[i+1], tok[i+2], tok[i+3]);
                    i+=3;
                }
                break;
            case 49:
                ctx->bg=-1;
                ctx->brgb=-1;
                break;
            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
                ctx->frgb=rgb_from_256(tok[i]-82);
                break;
            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                ctx->brgb=rgb_from_256(tok[i]-92);
                break;
            }
        unspan(ctx);
        span(ctx);
        ch=ansi2html__inchar(ctx);
        goto normal;
    case 'C':
        ntok=tok[0];
        if (ntok<=0)
            ntok=1;
        else if (ntok>512) /* sanity */
            ntok=512;
        for (unsigned int i=0;i<ntok;++i)
            ansi2html__printf(ctx, " ");
        ch=ansi2html__inchar(ctx);
        goto normal;
    case 'J': /* screen clear */
        goto formfeed;
    default:
        ch=ansi2html__inchar(ctx);           /* invalid/unimplemented code, ignore */
    case EOF:
        goto normal;
    }
/****************************************************************************/
csiopt:
    if (ch==';'||(ch>='0'&&ch<='9'))
    {
        ch=ansi2html__inchar(ctx);
        goto csiopt;
    }
    ch=ansi2html__inchar(ctx);
    goto normal;
}

void ansi_get_stylesheet(char *dest)
{
    dest = stpcpy(dest,
                  //"<style type=\"text/css\">\n"
                  "color: #000000;\n"
                  "pre {\n"
                  "\tfont-weight: normal;\n"
                  "\tcolor: red;\n"
                  "}\n"
                  "b {font-weight: normal}\n"
                  "b.BOLD {color: #bbb}\n"
                  "b.ITA {font-style: italic}\n"
                  "b.UND {text-decoration: underline}\n"
                  "b.STR {text-decoration: line-through}\n"
                  "b.UNDSTR {text-decoration: underline line-through}\n"
                 );

    for (int i=0; i<16; i++) {
        dest += sprintf(dest,"b.%s {color: #%06x}\n", cols[i], rgb_from_256(i));
    }

    for (int i=0; i<8; i++) {
        dest += sprintf(dest,"b.B%s {background-color: #%06x}\n", cols[i], rgb_from_256(i));
    }

    //dest = stpcpy(dest, "</style>\n");
}

int ansi_convert_line_to_html(const char *input, char *output, unsigned maxlen)
{
    ansi2html_context_t ctx;
    ctx.input = input;
    ctx.output = output;
    ctx.maxlen = maxlen;
    return ansi2html__convert(&ctx);
}
