#ifndef __COLOR_H__
#define __COLOR_H__


#define BLUE   (1<<0)
#define RED    (1<<1)
#define GREEN  (1<<2)
#define BRIGHT (1<<6)
#define FLASH (1<<7)

#define BLACK 0

#define WHITE (RED|GREEN|BLUE)



#define MAKECOLOR(fg, bg) (((bg)<<3)|(fg))
#define MAKECOLORA(fg, bg, a) (((bg)<<3)|(fg)|(a))



#endif
