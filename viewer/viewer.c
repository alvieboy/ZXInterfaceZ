#include "SDL.h"
#include "SDL_syswm.h"
#include <X11/extensions/Xrandr.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#ifdef __arm__
#define FULLSCREEN
#endif
#define MAX_FRAME_PAYLOAD 2048

struct frame {
    uint8_t seq;
    uint8_t frag;
    uint8_t payload[MAX_FRAME_PAYLOAD];
};

const unsigned int BORDER_LR = 32;
const unsigned int BORDER_TB = 39;
bool flashinvert;

typedef struct {
    uint8_t pixeldata[32*192];
    uint8_t attributes[32*24];
} __attribute__((packed)) scr_t;


#define RGB565(r,g,b) (((r & 0xf8)<<8) + ((g & 0xfc)<<3)+(b>>3))

static const uint32_t normal_colors32[] = {
    0x000000,
    0x0000D7,
    0xD70000,
    0xD700D7,
    0x00D700,
    0x00D7D7,
    0xD7D700,
    0xD7D7D7
};

static const uint32_t bright_colors32[] = {
    0x000000,
    0x0000FF,
    0xFF0000,
    0xFF00FF,
    0x00FF00,
    0x00FFFF,
    0xFFFF00,
    0xFFFFFF
};

static const uint16_t normal_colors16[] = {
    RGB565(0,0,0),
    RGB565(0,0,0xD7),
    RGB565(0xD7,0,0),
    RGB565(0xD7,0,0xD7),
    RGB565(0, 0xD7, 0),
    RGB565(0, 0xD7, 0xD7),
    RGB565(0xD7, 0xD7,0),
    RGB565(0xD7, 0xD7, 0xD7)
};

static const uint16_t bright_colors16[] = {
    RGB565(0,0,0),
    RGB565(0,0,0xFF),
    RGB565(0xFF,0,0),
    RGB565(0xFF,0,0xFF),
    RGB565(0, 0xFF, 0),
    RGB565(0, 0xFF, 0xFF),
    RGB565(0xFF, 0xFF,0),
    RGB565(0xFF, 0xFF, 0xFF)
};

void showmodes(int displayIndex)
{
    int display_count = 0, display_index = 0, mode_index = 0;
    SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

    int nmodes = SDL_GetNumDisplayModes(displayIndex);
    SDL_Log("Number of modes: %d", nmodes);


    if ((display_count = SDL_GetNumVideoDisplays()) < 1) {
        SDL_Log("SDL_GetNumVideoDisplays returned: %i", display_count);
        return;
    }
    for (mode_index=0; mode_index < nmodes; mode_index++) {

        if (SDL_GetDisplayMode(display_index, mode_index, &mode) != 0) {
            SDL_Log("SDL_GetDisplayMode failed: %s", SDL_GetError());
            return;
        }


        SDL_Log("SDL_GetDisplayMode(0, 0, &mode):\t\t%i bpp\t%i x %i",
                SDL_BITSPERPIXEL(mode.format), mode.w, mode.h);
    }
}


SDL_Window *win;
SDL_Surface *rootsurface;

void renderscr32(const scr_t *scr, bool flashonly);
void renderscr16(const scr_t *scr, bool flashonly);

void (*renderscr)(const scr_t *scr, bool flashonly);

void open_window( int w, int h)
{
#ifdef FULLSCREEN
    win = SDL_CreateWindow(
                          "ZX Spectrum",
                          0,
                          0,
                          w,                               // width, in pixels
                          h,                               // height, in pixels
                          SDL_WINDOW_FULLSCREEN
                         );
#else
    win = SDL_CreateWindow(
                          "ZX Spectrum",
                          0,
                          0,
                          w,                               // width, in pixels
                          h,                               // height, in pixels
                          0
                         );
#endif
    rootsurface = SDL_GetWindowSurface(win);

    SDL_PixelFormat*fmt = rootsurface->format;

    printf("Running with BPP %d\n", fmt->BitsPerPixel);
    switch (fmt->BitsPerPixel) {
    case 32:
        /* Fall-through */
    case 24:
        renderscr = &renderscr32;
        break;
    case 16:
        renderscr = &renderscr16;
        break;
    default:
        printf("Unsupported BPP %d\n", fmt->BitsPerPixel);
        exit(-1);
    }
	


}

static uint32_t parsecolor32( uint8_t col, int bright)
{
    return bright ? bright_colors32[col] : normal_colors32[col];
}

static uint16_t parsecolor16( uint8_t col, int bright)
{
    return bright ? bright_colors16[col] : normal_colors16[col];
}

static void parseattr32( uint8_t attr, uint32_t *fg, uint32_t *bg, int *flash)
{
    *fg = parsecolor32(attr & 0x7, attr & 0x40);
    *bg = parsecolor32((attr>>3) & 0x7, attr & 0x40);
    *flash = attr & 0x80;
}

static void parseattr16( uint8_t attr, uint16_t *fg, uint16_t *bg, int *flash)
{
    *fg = parsecolor16(attr & 0x7, attr & 0x40);
    *bg = parsecolor16((attr>>3) & 0x7, attr & 0x40);
    *flash = attr & 0x80;
}

static uint8_t getattr(const scr_t *scr, int x, int y)
{
    return scr->attributes[ x + (y*32) ];
}



void calc_sizes(int w, int h)
{
    // *0   1920 x 1080   ( 309mm x 173mm )  *60   120  48
    // Assumed square pixels

}


static inline void drawPixel32(int x, int y, uint32_t pixel)
{
    Uint32 *pixels = (Uint32 *)rootsurface->pixels;
    Uint32 *pptr = &pixels[ ( (y*4) * rootsurface->w ) + (x*4) ];

    Uint32 *pptr2 = pptr;

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;

    pptr2 += (rootsurface->w - 3);

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;


    pptr2 += (rootsurface->w - 3);

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;

    pptr2 += (rootsurface->w -3 );

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;
}

void renderscr32(const scr_t *scr, bool flashonly)
{
    int x;
    int y;
    uint32_t fg, bg, t;
    int flash;

    SDL_LockSurface( rootsurface );

    for (y=0;y<192;y++) {
        for (x=0; x<32; x++) {
            unsigned offset = x; // 5 bits

            offset |= ((y>>3) & 0x7)<<5;  // Y5, Y4, Y3
            offset |= (y & 7)<<8;         // Y2, Y1, Y0
            offset |= ((y>>6) &0x3 ) << 11;               // Y8, Y7

            uint8_t attr = getattr(scr, x, y>>3);
            parseattr32( attr, &fg, &bg, &flash);

            if (!flash && flashonly)
                continue;

            if (flash && flashinvert) {
                t = bg;
                bg = fg;
                fg = t;
            }
            // Get m_scr
            uint8_t pixeldata8 = scr->pixeldata[offset];
            //printf("%d %d Pixel %02x attr %02x fg %08x bg %08x\n", x, y, pixeldata8, attr, fg, bg);

            for (int ix=0;ix<8;ix++) {
                drawPixel32(BORDER_LR + (x*8 + ix),
                          BORDER_TB+y,
                          pixeldata8 & 0x80 ? fg: bg);
                pixeldata8<<=1;
            }
        }
    }
    SDL_UnlockSurface( rootsurface );

};

static inline void drawPixel16(int x, int y, uint16_t pixel)
{
    Uint16 *pixels = (Uint16 *)rootsurface->pixels;
    Uint16 *pptr = &pixels[ ( (y*4) * rootsurface->w ) + (x*4) ];

    Uint16 *pptr2 = pptr;

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;

    pptr2 += (rootsurface->w - 3);

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;


    pptr2 += (rootsurface->w - 3);

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;

    pptr2 += (rootsurface->w -3 );

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;
}

void renderscr16(const scr_t *scr, bool flashonly)
{
    int x;
    int y;
    uint16_t fg, bg, t;
    int flash;

    SDL_LockSurface( rootsurface );

    for (y=0;y<192;y++) {
        for (x=0; x<32; x++) {
            unsigned offset = x; // 5 bits

            offset |= ((y>>3) & 0x7)<<5;  // Y5, Y4, Y3
            offset |= (y & 7)<<8;         // Y2, Y1, Y0
            offset |= ((y>>6) &0x3 ) << 11;               // Y8, Y7

            uint8_t attr = getattr(scr, x, y>>3);
            parseattr16( attr, &fg, &bg, &flash);

            if (!flash && flashonly)
                continue;

            if (flash && flashinvert) {
                t = bg;
                bg = fg;
                fg = t;
            }
            // Get m_scr
            uint8_t pixeldata8 = scr->pixeldata[offset];
            //printf("%d %d Pixel %02x attr %02x fg %08x bg %08x\n", x, y, pixeldata8, attr, fg, bg);

            for (int ix=0;ix<8;ix++) {
                drawPixel16(BORDER_LR + (x*8 + ix),
                          BORDER_TB+y,
                          pixeldata8 & 0x80 ? fg: bg);
                pixeldata8<<=1;
            }
        }
    }
    SDL_UnlockSurface( rootsurface );

};

void update()
{
    SDL_UpdateWindowSurface(win);
}

void close_window()
{
    SDL_DestroyWindow(win);
}

#include <unistd.h>
#include <fcntl.h>

void load(const char *file)
{
    int f = open(file,O_RDONLY);
    if (f<0)
        return;
    off_t size = lseek(f,0,SEEK_END);
    lseek(f,0,SEEK_SET);

    uint8_t *buf = (uint8_t*)malloc(size);
    read(f, buf, size);
    close(f);
    renderscr((scr_t*)buf, false);
}

uint8_t framedata[8192];

void udp_process(const uint8_t *data, int len)
{
    if (len<2)
        return;

    unsigned payloadlen = len - 2;
    //qDebug()<<"Dgram";
    const struct frame *f = (const struct frame*)data;

    switch (f->frag) {
    case 0:
        assert( payloadlen == MAX_FRAME_PAYLOAD );
        memcpy( &framedata[0], f->payload, MAX_FRAME_PAYLOAD);
        break;
    case 1:
        assert( payloadlen == MAX_FRAME_PAYLOAD );
        memcpy( &framedata[MAX_FRAME_PAYLOAD], f->payload, MAX_FRAME_PAYLOAD);
        break;
    case 2:
        assert( payloadlen == MAX_FRAME_PAYLOAD );
        memcpy( &framedata[MAX_FRAME_PAYLOAD*2], f->payload, MAX_FRAME_PAYLOAD);
        break;
    case 3:
        memcpy( &framedata[MAX_FRAME_PAYLOAD*3], f->payload, payloadlen);
        renderscr( (scr_t*)framedata, false );
        update();
        break;

    default:
        break;
    }
}


#include <sys/select.h>
#include "list.h"


int udp_socket = -1;
int tcp_socket = -1;


void tcp_data()
{
    char buf[128];
    int s = read(tcp_socket, buf, sizeof(buf));
    if (s<=0) {
        close(tcp_socket);
        tcp_socket = -1;
    } else {
        buf[s] = '\0';
        printf("Reply: %s\n", buf);
    }
}


void udp_data()
{
    uint8_t data[16368];
    int r = recv(udp_socket, data, sizeof(data), 0);
    printf("UDP recv: %d\n", r);
    if (r>0) {
        udp_process(data, r);
    }
}

void net_check()
{
    struct timeval tv;
    fd_set rfs;
    fd_set wfs;
    FD_ZERO(&rfs);
    FD_ZERO(&wfs);

    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    int max = -1;

    FD_SET(udp_socket, &rfs);
    max = udp_socket;

    if (tcp_socket>0) {
        FD_SET(tcp_socket, &rfs);
        if (tcp_socket>max)
            max=tcp_socket;
    }

    switch(select(max+1, &rfs, &wfs, 0, &tv)) {
    case 0:
        break;
    case -1:
        break;
    default:
        if (FD_ISSET(udp_socket,&rfs)) {
            udp_data();
        }
        if (FD_ISSET(tcp_socket,&rfs)) {
            tcp_data();
        }
        break;
    }
}


int request_data(const char *ip)
{
    int tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in s;

    s.sin_family = AF_INET;
    s.sin_port = htons(8003);
    s.sin_addr.s_addr = inet_addr(ip);

    if (connect(tcp_socket, (struct sockaddr*)&s, sizeof(s))<0) {
        printf("Cannot connect: %s\n", strerror(errno));
        return -1;
    }
    printf("Connected to %s\n", ip);

    write(tcp_socket, "stream 8010\n",12);
    shutdown(tcp_socket, SHUT_WR);

    //sendReceive(m_zxaddress, &MainWindow::progressReceived, true,  "stream 8010");

}

void run(int w, int h)
{
    open_window(w,h);
    load("MANIC.SCR");
    update();

    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in s;
    s.sin_family = AF_INET;
    s.sin_port = htons(8010);
    s.sin_addr.s_addr = INADDR_ANY;
    if (bind(udp_socket,(struct sockaddr*)&s, sizeof(s))<0) {
        printf("Cannot bind: %s\n", strerror(errno));
        exit -1;
    }

    request_data("192.168.91.5");
    while (1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
             //printf("event %d\n", event.type);
            // handle your event here

            switch (event.type) {
                // exit if the window is closed
            case SDL_QUIT:
                return;
            case SDL_WINDOWEVENT:

                switch (event.window.event) {
                case SDL_WINDOWEVENT_CLOSE:
                    return;
                }
            default:
                break;
            }
        }
        net_check();
        //if (!hasevent) {
        //    SDL_Delay(10);
        //} else {
        //    count=0;
        //}
    }
    close_window();
}

void start_network()
{
}


int main(int argc, char **argv)
{
    SDL_DisplayMode current;
    struct SDL_SysWMinfo wminfo;

    SDL_Init(SDL_INIT_VIDEO);

    int i;
    // Get current display mode of all displays.
    for(i = 0; i < SDL_GetNumVideoDisplays(); ++i){

        int should_be_zero = SDL_GetCurrentDisplayMode(i, &current);

        if(should_be_zero != 0)
            // In case of error...
            SDL_Log("Could not get display mode for video display #%d: %s", i, SDL_GetError());

        else
            // On success, print the current display mode.
            SDL_Log("Display #%d: current display mode is %dx%dpx @ %dhz.", i, current.w, current.h, current.refresh_rate);

        showmodes(i);

    }


    int	event_base, error_base;
    int major, minor;

    SDL_Window *window = SDL_CreateWindow("", 0, 0, 0, 0, SDL_WINDOW_HIDDEN);
    if (NULL==window) {
        SDL_Log("Cannot open window");
        return -1;
    }

    SDL_VERSION(&wminfo.version); /* initialize info structure with SDL version info */

    if (!SDL_GetWindowWMInfo(window,
                             &wminfo)) {
        SDL_Log("Cannot get window information");
        return -1;
    }

    Display *dpy = wminfo.info.x11.display;

    if (!XRRQueryExtension (dpy, &event_base, &error_base) ||
        !XRRQueryVersion (dpy, &major, &minor))
    {
	fprintf (stderr, "RandR extension missing\n");
	exit (1);
    }

    // Get information

    XRRScreenSize *sizes;
    XRRScreenConfiguration *sc;

    int screen = DefaultScreen (dpy);

    Window root = RootWindow (dpy, screen);

    sc = XRRGetScreenInfo (dpy, root);

    if (sc == NULL) 
	exit (1);

    Rotation current_rotation;
    int nsize;
    int nrate;
    SizeID current_size = XRRConfigCurrentConfiguration (sc, &current_rotation);
    int  current_rate = XRRConfigCurrentRate (sc);

    sizes = XRRConfigSizes(sc, &nsize);

    if (current_size>nsize) {
        abort();
    }

    printf("%d %d %d %d\n",
           sizes[current_size].width,
           sizes[current_size].height,
           sizes[current_size].mwidth,
           sizes[current_size].mheight
          );



    if (1)    {
        int j;
        printf(" SZ:    Pixels          Physical       Refresh\n");
	for (i = 0; i < nsize; i++) {
	    printf ("%c%-2d %5d x %-5d  (%4dmm x%4dmm )",
		    i == current_size ? '*' : ' ',
		    i, sizes[i].width, sizes[i].height,
		    sizes[i].mwidth, sizes[i].mheight);
	    short *rates = XRRConfigRates (sc, i, &nrate);
	    if (nrate) printf ("  ");
	    for (j = 0; j < nrate; j++)
		printf ("%c%-4d",
			i == current_size && rates[j] == current_rate ? '*' : ' ',
			rates[j]);
	    printf ("\n");
	}

    }
    SDL_DestroyWindow(window);

    run(1280, 1024);

    // Clean up and exit the program.
    SDL_Quit();
    return 0;
}



