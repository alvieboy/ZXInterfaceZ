#include "memdata.h"
#include <stdio.h>

#define ASSERT(x) \
    do { \
    if (!(x)) { \
    fprintf(stderr, "ASSERTION FAILED: %s\n", #x); \
    return -1; \
    } \
    }while (0)

static int test000()
{

    struct memdata_request req[2];
    uint8_t numreq;

    numreq = 255;

    uint8_t v;

    v = memdata__analyse_request(0x0000, 255, req, &numreq);
    ASSERT(v==255);
    ASSERT(numreq==1);
    ASSERT(req[0].address == 0x0000);
    ASSERT(req[0].len == 255);
    ASSERT(req[0].type == ROM);

    v = memdata__analyse_request(0x3FFF, 1, req, &numreq);
    ASSERT(v==1);
    ASSERT(numreq==1);
    ASSERT(req[0].address == 0x3FFF);
    ASSERT(req[0].len == 1);
    ASSERT(req[0].type == ROM);

    v = memdata__analyse_request(0x3FFF, 2, req, &numreq);
    ASSERT(v==2);
    ASSERT(numreq==2);
    ASSERT(req[0].address == 0x3FFF);
    ASSERT(req[0].len == 1);
    ASSERT(req[0].type == ROM);
    ASSERT(req[1].address == 0x4000);
    ASSERT(req[1].len == 1);
    ASSERT(req[1].type == SAVED_RAM);

    v = memdata__analyse_request(0x5FFF, 1, req, &numreq);
    ASSERT(v==1);
    ASSERT(numreq==1);
    ASSERT(req[0].address == 0x5FFF);
    ASSERT(req[0].len == 1);
    ASSERT(req[0].type == SAVED_RAM);

    v = memdata__analyse_request(0x5FFE, 5, req, &numreq);
    ASSERT(v==5);
    ASSERT(numreq==2);
    ASSERT(req[0].address == 0x5FFE);
    ASSERT(req[0].len == 2);
    ASSERT(req[0].type == SAVED_RAM);
    ASSERT(req[1].address == 0x6000);
    ASSERT(req[1].len == 3);
    ASSERT(req[1].type == RAM);

    // No wrapping at end
    v = memdata__analyse_request(0xFFFE, 2, req, &numreq);
    ASSERT(v==2);
    ASSERT(numreq==1);
    ASSERT(req[0].address == 0xFFFE);
    ASSERT(req[0].len == 2);
    ASSERT(req[0].type == RAM);

    // Wrapping at end
    v = memdata__analyse_request(0xFFFE, 3, req, &numreq);
    ASSERT(v==2);
    ASSERT(numreq==1);
    ASSERT(req[0].address == 0xFFFE);
    ASSERT(req[0].len == 2);
    ASSERT(req[0].type == RAM);


    return 0;
};


int run_tests()
{
    int r = 0;
    r += test000();
    return r;
}
