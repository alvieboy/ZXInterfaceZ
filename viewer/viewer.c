#include "SDL.h"
#include "SDL_syswm.h"
#include <X11/extensions/Xrandr.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#if 1 //def __arm__
#define FULLSCREEN
#endif
#define MAX_FRAME_PAYLOAD 2048

SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;

int tex_w;
int tex_h;
uint32_t tex_format;

struct frame {
    uint8_t seq:7;
    uint8_t val:1;
    uint8_t frag;
    uint8_t payload[MAX_FRAME_PAYLOAD];
};

unsigned int BORDER_LR = 32;
unsigned int BORDER_TB = 39;
bool flashinvert;

typedef struct {
    uint8_t pixeldata[32*192];
    uint8_t attributes[32*24];
} __attribute__((packed)) scr_t;


#define RGB565(r,g,b) (((r & 0xf8)<<8) + ((g & 0xfc)<<3)+(b>>3))

static const uint32_t normal_colors32[] = {
    0xFF000000,
    0xFF0000D7,
    0xFFD70000,
    0xFFD700D7,
    0xFF00D700,
    0xFF00D7D7,
    0xFFD7D700,
    0xFFD7D7D7
};

static const uint32_t bright_colors32[] = {
    0xFF000000,
    0xFF0000FF,
    0xFFFF0000,
    0xFFFF00FF,
    0xFF00FF00,
    0xFF00FFFF,
    0xFFFFFF00,
    0xFFFFFFFF
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
//SDL_Surface *rootsurface;

void renderscr32_2(const scr_t *scr, bool flashonly);
void renderscr32_4(const scr_t *scr, bool flashonly);
void renderscr16(const scr_t *scr, bool flashonly);

void (*renderscr)(const scr_t *scr, bool flashonly);


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

static uint8_t *image_pixels;


static inline void drawPixel32_4(int x, int y, uint32_t pixel)
{
    Uint32 *pixels = (Uint32 *)image_pixels;
    Uint32 *pptr = &pixels[ ( (y*4) * tex_w ) + (x*4) ];

    Uint32 *pptr2 = pptr;

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;

    pptr2 += (tex_w - 3);

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;


    pptr2 += (tex_w - 3);

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;

    pptr2 += (tex_w -3 );

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;
}

static inline void drawPixel32_2(int x, int y, uint32_t pixel)
{
    Uint8 *pixels = image_pixels;

    Uint32 *pptr = (Uint32*)&pixels[ ( (y*2) * (tex_w*sizeof(Uint32))) + (x*2*sizeof(Uint32)) ];

    Uint32 *pptr2 = pptr;

    *pptr2++=pixel;
    *pptr2=pixel;

    pptr2 += (tex_w - 1);

    *pptr2++=pixel;
    *pptr2=pixel;
}

void renderscr32_4(const scr_t *scr, bool flashonly)
{
    int x;
    int y;
    uint32_t fg, bg, t;
    int flash;

    //SDL_LockSurface( rootsurface );

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
                drawPixel32_4(BORDER_LR + (x*8 + ix),
                              BORDER_TB+y,
                              pixeldata8 & 0x80 ? fg: bg);
                pixeldata8<<=1;
            }
        }
    }
    //SDL_UnlockSurface( rootsurface );

};

void renderscr32_2(const scr_t *scr, bool flashonly)
{
    int x;
    int y;
    uint32_t fg, bg, t;
    int flash;

    //SDL_LockSurface( rootsurface );

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
                drawPixel32_2(BORDER_LR + (x*8 + ix),
                              BORDER_TB+y,
                              pixeldata8 & 0x80 ? fg: bg);
                pixeldata8<<=1;
            }
        }
    }
    //SDL_UnlockSurface( rootsurface );

};

static inline void drawPixel16_4(int x, int y, uint16_t pixel)
{
    Uint16 *pixels = (Uint16 *)image_pixels;
    Uint16 *pptr = &pixels[ ( (y*4) * tex_w ) + (x*4) ];

    Uint16 *pptr2 = pptr;

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;

    pptr2 += (tex_w - 3);

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;


    pptr2 += (tex_w - 3);

    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2++=pixel;
    *pptr2=pixel;

    pptr2 += (tex_w -3 );

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

    //SDL_LockSurface( rootsurface );

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
                drawPixel16_4(BORDER_LR + (x*8 + ix),
                              BORDER_TB+y,
                              pixeldata8 & 0x80 ? fg: bg);
                pixeldata8<<=1;
            }
        }
    }
    //SDL_UnlockSurface( rootsurface );

};


void open_window( int w, int h, int zoom)
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
   // rootsurface = SDL_GetWindowSurface(win);

    //    SDL_PixelFormat*fmt = rootsurface->format;

    sdlRenderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    sdlTexture = SDL_CreateTexture(sdlRenderer,
                                   SDL_PIXELFORMAT_RGB888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   w, h);
    tex_w = w;
    tex_h = h;
    tex_format = SDL_PIXELFORMAT_RGB888;


    printf("Running with BPP %d size %d %d pitch %d\n",
           SDL_BITSPERPIXEL(tex_format),
           tex_w,
           tex_h, 0);
    switch (SDL_BITSPERPIXEL(tex_format)) {
    case 32:
        /* Fall-through */
    case 24:
        if (zoom==4) {
            renderscr = &renderscr32_4;
        } else {
            renderscr = &renderscr32_2;
        }
        image_pixels = malloc(w*h*4);
        break;
    case 16:
        renderscr = &renderscr16;
        image_pixels = malloc(w*h*2);
        break;
    default:
        printf("Unsupported BPP %d\n", SDL_BITSPERPIXEL(tex_format));
        exit(-1);
    }




   


}

void update()
{
   // printf("Update frame\n");

    int i;
//    drawPixel32_4(0,0,0xffffffff);
    //drawPixel32_4(1,0,0xffff);
    SDL_UpdateTexture(sdlTexture, NULL, image_pixels, tex_w * sizeof (Uint32));
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(sdlRenderer);

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

#define SPECTRUM_FRAME_SIZE (32*(192+24))

uint8_t framedata[SPECTRUM_FRAME_SIZE];

void udp_process(const uint8_t *data, int len)
{
    if (len<2)
        return;

    unsigned payloadlen = len - 2;
    //qDebug()<<"Dgram";
    const struct frame *f = (const struct frame*)data;

    unsigned maxpayload = 1<<((f->frag) >> 4);

    uint8_t lastfrag = (((unsigned)SPECTRUM_FRAME_SIZE+((unsigned)maxpayload-1))/(unsigned)maxpayload)-1;
    uint8_t frag = f->frag & 0xf;
//    printf("UDP: seq %d frag %d lastfrag %d maxpayload %d\n", f->seq, frag, lastfrag,maxpayload);

    memcpy( &framedata[ frag * maxpayload ], f->payload, payloadlen);

    if (frag>=lastfrag) {
        renderscr( (scr_t*)framedata, false );
        update();
    }
}


#include <sys/select.h>

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
    //printf("UDP recv: %d\n", r);
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

void run(int w, int h, int zoom)
{
    open_window(w,h, zoom);

    //load("MANIC.SCR");
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

    request_data("192.168.1.194");
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

#define MINIMUM_WIDTH (256*2)
#define MINIMUM_HEIGHT (192*2)

int compare_modes(const SDL_DisplayMode *prev_mode, const SDL_DisplayMode *new_mode)
{
    if (prev_mode->format == SDL_PIXELFORMAT_UNKNOWN)
        return 0; // Best
    if ((new_mode->w > MINIMUM_WIDTH) &&
        (new_mode->h > MINIMUM_HEIGHT)) {
        SDL_Log("Valid mode %d %d\n", new_mode->w, new_mode->h);
        if (new_mode->w < prev_mode->w) {
            return 0;
        }
    }
    return -1;

}

int find_video_mode(int *w, int *h, int *zoom_out)
{
    int display_count = 0, display_index = 0, mode_index = 0;
    SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };
    SDL_DisplayMode best_mode = {
        .format = SDL_PIXELFORMAT_UNKNOWN,
        .w = 4096,
        .h = 4096
    };

    if ((display_count = SDL_GetNumVideoDisplays()) < 1) {
        SDL_Log("SDL_GetNumVideoDisplays returned: %i", display_count);
        return -1;
    }

    int num_modes = SDL_GetNumDisplayModes(0);
    SDL_Log("Number of modes: %d\n", num_modes);

    for (mode_index = 0; mode_index < num_modes; mode_index++) {

        if (SDL_GetDisplayMode(display_index, mode_index, &mode) != 0) {
            SDL_Log("SDL_GetDisplayMode failed: %s", SDL_GetError());
            return 1;
        }
        SDL_Log("SDL_GetDisplayMode(0, 0, &mode):\t\t%i bpp\t%i x %i",
                SDL_BITSPERPIXEL(mode.format), mode.w, mode.h);
        switch (mode.format) {
        case SDL_PIXELFORMAT_RGB888:
            // Check if better than previous mode
            SDL_Log("Valid mode");
            if (compare_modes(&best_mode, &mode)==0) {
                best_mode = mode;
            }
            break;
        default:
            SDL_Log("Skipping mode");
            break;
        }
    }

    if (best_mode.format == SDL_PIXELFORMAT_UNKNOWN) {
        SDL_Log("Cannot find a suitable video mode\n");
        return -1;
    }

    SDL_Log("Choosing mode %i bpp\t%i x %i",
            SDL_BITSPERPIXEL(best_mode.format), best_mode.w, best_mode.h);


    int zoom = 4;
    while (zoom>=2){
        if ((zoom*256) < best_mode.w) {
            if ((zoom*192) < best_mode.h) {
                SDL_Log("Can zoom %d\n", zoom);
                break;
            }
        }
        zoom>>=1;
    }
    if (zoom<2) {
        SDL_Log("Unsupported zoom level %d\n", zoom);
        return -1;
    }

    // Compute borders, offsets, etc. Set up proper render callback

    int usable_w = best_mode.w / zoom;
    int usable_h = best_mode.h / zoom;

    int remain_w = usable_w - 256;
    int remain_h = usable_h - 192;

    BORDER_LR = remain_w >> 1;
    BORDER_TB = remain_h >> 1;

    //    SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags);
    *zoom_out = zoom;
    *w = best_mode.w;
    *h = best_mode.h;

    return 0;
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

    int w=800, h=600;
    int zoom = 2;

    if (find_video_mode(&w, &h, &zoom)<0)
        return -1;

    run(w,h, zoom);

    // Clean up and exit the program.
    SDL_Quit();
    return 0;
}



