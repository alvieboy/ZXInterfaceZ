#include "object.h"
#include "malloc.h"

/**
 * \defgroup object Objects
 * \brief Reference counted objects
 */

/**
 * \ingroup object
 * \brief Allocate a new object
 */
struct object *object__alloc(unsigned size, const struct object_ops *ops)
{
    struct object *obj = (struct object*)calloc(1,size);
    if (obj)
        object__init(obj, ops);
    return obj;

}

/**
 * \ingroup object
 * \brief Reallocate a new object
 */
struct object *object__realloc(struct object *obj, unsigned size)
{
    obj = (struct object*)realloc(obj, size);
    return obj;
}


