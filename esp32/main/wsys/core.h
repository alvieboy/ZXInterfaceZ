#ifndef __WSYS_CORE_H__
#define __WSYS_CORE_H__

#include <inttypes.h>
#include <stdlib.h>
#include <functional>
#include "systemevent.h"
#include "charmap.h"
#include "struct_assert.h"
#include "joystick.h"

#include "../wsys.h"
#include "log.h"
#include "defs.h"

#ifdef __linux__
#define WSYS_BOUND_CHECKS
#define WSYS_ENABLE_DEBUG
#endif


union u16_8_t
{
    struct {
        uint8_t l;
        uint8_t h;
    };
    uint16_t v;
    operator uint16_t () const { return v; }
    u16_8_t operator++(int) {  u16_8_t s = *this; v++; return s; }
    u16_8_t &operator+=(int delta) { v +=delta; return *this; }
};

typedef enum {
    WSYS_INPUT_EVENT_KBD,
    WSYS_INPUT_EVENT_JOYSTICK
} __attribute__((packed)) wsys_event_type_t;

typedef union {
    uint32_t raw;
    struct {
        wsys_event_type_t type;
        union {
            u16_8_t code;
            struct {
                joy_action_t joy_action;
                bool joy_on;
            };
        };
    };
} wsys_input_event_t;

#ifndef __linux__
ASSERT_STRUCT_SIZE(wsys_input_event_t, 4);
#endif

typedef enum {
    BLACK   = 0,    // 000
    BLUE,           // 001
    RED,            // 010
    MAGENTA,        // 011
    GREEN,          // 100
    CYAN,           // 101
    YELLOW,         // 110
    WHITE,          // 111
} color_t;

struct attr_t {
    union {
        struct {
            color_t m_fg:3;
            color_t m_bg:3;
            uint8_t m_bright:1;
            uint8_t m_flash:1;
        } __attribute__((packed));
        uint8_t raw:8;
    };

    attr_t(color_t fg, color_t bg=BLACK, bool bright=false, bool flash=false):
        m_fg(fg),
        m_bg(bg),
        m_bright(bright?1:0),
        m_flash(flash?1:0)
    {}
    attr_t() {}
    explicit attr_t(uint8_t raw);
    operator uint8_t () const {return raw; };
} __attribute__((packed));




#ifdef WSYS_ENABLE_DEBUG


#define WSYS_LOGI(x...) LOG_DEBUG(DEBUG_ZONE_WSYS, __PRETTY_FUNCTION__, x);

#else

#define WSYS_LOGI(x...)

#endif

#define WSYS_LOGW(x...) do \
     ESP_LOGW(__PRETTY_FUNCTION__, x); \
    while (0);

#define WSYS_LOGE(x...) do \
     ESP_LOGE(__PRETTY_FUNCTION__, x); \
    while (0);




struct framebuffer {
    union {
        struct {
            uint8_t screen[32*24*8];
            uint8_t attr[32*24];
            uint8_t seq;
            uint8_t unused[3];
        } ;
        uint32_t w32[1792];
    };
};

#ifdef WSYS_BOUND_CHECKS

#define CHECK_BOUNDS_SCREEN(x) do { \
    if (off.v>sizeof(spectrum_framebuffer.screen)) { \
    ESP_LOGE("WSYS", "OUT of bounds screen acccess %d (%d)", off.v, sizeof(spectrum_framebuffer.screen));\
    abort(); \
    } \
} while (0);
#define CHECK_BOUNDS_ATTR(x) do { \
    if (off>sizeof(spectrum_framebuffer.attr)) { \
    ESP_LOGE("WSYS", "OUT of bounds attr acccess %d (%d)", off, sizeof(spectrum_framebuffer.attr));\
    abort(); \
    }\
} while (0);

#else

#define CHECK_BOUNDS_SCREEN(x)
#define CHECK_BOUNDS_ATTR(x)

#endif


extern uint8_t current_charset;
static inline const uint8_t*getcurrentcharset(void) {
    return CHAR_SETS[current_charset];
}


extern struct framebuffer spectrum_framebuffer;

void push_charset(int index);
int pop_charset();

static inline uint16_t getxyattrstart(uint8_t x, uint8_t y)
{
    uint16_t off = (x+(y*32));
    return off;
}

static inline u16_8_t getxyscreenstart(uint8_t x, uint8_t y)
{
    u16_8_t off;
    off.l = (y<<5) & 0xE0;
    off.l += x;
    off.h = y & 0x18;
    return off;
}


struct attrptr_t {

    void fromxy(uint8_t x, uint8_t y) {
        off = getxyattrstart(x,y);
    }
    void nextline(int amt=1) { off+=(32*amt); }
    attrptr_t& operator++() { off++; return *this; }
    attrptr_t operator++(int delta __attribute__((unused))) { attrptr_t s = *this; off++; return s; }
    attrptr_t &operator+=(int delta) { off +=delta; return *this; }
    uint8_t & operator*() {
        CHECK_BOUNDS_ATTR(off);
        return spectrum_framebuffer.attr[off];
    }
    uint16_t getoff() const { return off; }
private:
    uint16_t off;
};

struct screenptr_t {
    explicit screenptr_t(int f) { off.v=f; }
    screenptr_t() {}
    void fromxy(uint8_t x, uint8_t y) {
        off.v = getxyscreenstart(x,y);
    }
    void nextpixelline(int amt=1);
    void nextcharline(int amt=1);
    screenptr_t drawchar(const uint8_t *data);
    screenptr_t drawascii(char);
    screenptr_t drawstring(const char *);
    screenptr_t drawstringn(const char *, int len);
    screenptr_t printf(const char *fmt,...) __attribute__ ((format (printf, 2, 3)));
    screenptr_t drawstringpad(const char *, int len);
    screenptr_t drawhline(int len);
    screenptr_t drawvalue(const uint8_t data);
    screenptr_t &operator++() { off++; return *this; }
    screenptr_t operator++(int delta  __attribute__((unused))) { screenptr_t s = *this; off++;return s; }
    screenptr_t &operator+=(int delta) { off+=delta; return *this; }
    uint8_t & operator*() {
        CHECK_BOUNDS_SCREEN(off);

        return spectrum_framebuffer.screen[off];

    }
    uint8_t & operator[](int delta) {
        CHECK_BOUNDS_SCREEN(off+delta);

        return spectrum_framebuffer.screen[off+delta];

    }
    uint16_t getoff() const { return off.v; }
private:
    u16_8_t off;
};

#ifdef __linux__

#define CLASSNAME(x) typeid(x).name()


#include <map>
class WSYSObject
{
public:
    static void *allocate_memory(size_t size, const char *file, int line);
    static void release_memory(void *data, const char *file, int line);
    static void report_alloc();
    void operator delete(void*) noexcept;
    void operator delete(void*,size_t) noexcept;
    void operator delete[](void*) noexcept;
    template<typename T, typename... Args>
    static T *create(Args... a) {
        T* object = new T(a...);

        struct alloc_info info;
        info.file = typeid(T).name();
        info.line = 0;
        info.size = sizeof(T);

        m_allocations[(void*)object] = info;

        return object;
    }
private:
    //    void *operator new(size_t);
    void *operator new(size_t size) { void *a = malloc(size); return a; }
    struct alloc_info {
        const char *file;
        int line;
        size_t size;
        void *trace[16];
        int trace_size;
    };
    static std::map<void*, alloc_info> m_allocations;
};

#define ALLOC(x) (WSYSObject::allocate_memory((x), __PRETTY_FUNCTION__, __LINE__))
#define FREE(x) (WSYSObject::release_memory((x), __PRETTY_FUNCTION__, __LINE__))

#else

#define ALLOC(x) (WSYSObject::allocate_memory((x)))
#define FREE(x) (WSYSObject::release_memory((x)))

#define CLASSNAME(x) ""

// ESP32 version.

class WSYSObject
{
public:
    template<typename T, typename... Args>
        static T *create(Args... a)
    {
        T* object = new T(a...);
        //ESP_LOGI("WSYSObject","%s Alloc %p -> %p len %d", __PRETTY_FUNCTION__, object, object+sizeof(T), sizeof(T));
        //HEAPCHECK();
        return object;
    }
    static void *allocate_memory(size_t size) { return malloc(size); }
    static void release_memory(void *data) { free(data); }
    static void report_alloc() {}

    void operator delete(void*) noexcept;
    void operator delete(void*,size_t) noexcept;
    void operator delete[](void*) noexcept;
private:
    void *operator new(size_t size) { void *a = malloc(size); return a; }
};
#endif


screenptr_t drawthumbchar(screenptr_t screenptr, unsigned &bit_offset, char c);
screenptr_t drawthumbstring(screenptr_t screenptr, const char *s, unsigned off=0);
screenptr_t drawthumbcharxor(screenptr_t screenptr, unsigned &bit_offset, char c);
screenptr_t drawthumbstringxor(screenptr_t screenptr, const char *s, unsigned off=0);



int wsys__subscribesystemeventfun(std::function<void(const systemevent_t&)> handler);

template<typename T>
    int wsys__subscribesystemevent(T *object, void (T::*function)(const systemevent_t&) ) {
        return wsys__subscribesystemeventfun(
                                             [=](const systemevent_t &event) {
                                                 (object->*function)(event);
                                             }
                                            );
    }

void wsys__unsubscribesystemevent(int index);
void wsys__propagatesystemevent(const systemevent_t &event);

#endif

