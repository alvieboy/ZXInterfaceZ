#include "SDL.h"
#include "SDL_syswm.h"
#include <X11/extensions/Xrandr.h>

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

    // Clean up and exit the program.
    SDL_Quit();
    return 0;
}
