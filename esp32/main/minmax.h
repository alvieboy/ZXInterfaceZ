#ifndef __MINMAX_H__
#define __MINMAX_H__

#ifndef MIN
/**
 * \ingroup misc
 * \brief Return minimum of a, b
 */
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MIN3
/**
 * \ingroup misc
 * \brief Return minimum of a, b, c
 */
#define MIN3(a,b,c) (MIN( MIN(a,b), c))
#endif

#ifndef MAX
/**
 * \ingroup misc
 * \brief Return maximum of a, b
 */
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#endif
