#ifndef __MINMAX_H__
#define __MINMAX_H__

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MIN3
#define MIN3(a,b,c) (MIN( MIN(a,b), c))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#endif
