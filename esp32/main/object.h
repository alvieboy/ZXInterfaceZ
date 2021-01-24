#ifndef __OBJECT_H__
#define __OBJECT_H__

struct object;

struct object_ops {
    void (*release)(struct object *);
};

struct object {
    int refcnt;
    const struct object_ops *ops;
};

#endif

static inline void object__release(struct object *obj)
{
    if (obj->ops->release) {
        obj->ops->release(obj);
    }
}

static inline void object__put(struct object *obj)
{
    if (__sync_sub_and_fetch(&obj->refcnt,1)==0)
        object__release(obj);
}

static inline struct object *object__get(struct object *obj)
{
    __sync_fetch_and_add(&obj->refcnt,1);
    return obj;
}

static inline void object__init(struct object *obj, const struct object_ops *ops)
{
    obj->refcnt = 1;
    obj->ops = ops;
}

struct object *object__alloc(unsigned size, const struct object_ops *ops);
struct object *object__realloc(struct object*, unsigned size);

#define OBJECT_NEW(type, ops)  (type*)(object__alloc(sizeof(type), ops))
