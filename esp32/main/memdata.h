#ifndef __MEMDATA_H__
#define __MEMDATA_H__
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

struct memdata_request {
    uint16_t address;
    uint8_t len;
    enum {
        ROM,
        RAM,
        SAVED_RAM
    } type;
};

uint8_t memdata__analyse_request(uint16_t address, uint8_t len,
                                 struct memdata_request *out, uint8_t *num_requests);

const uint8_t *memdata__retrieve(uint8_t *size);
void memdata__store(const uint8_t *data, uint8_t size);

#ifdef __cplusplus
}
#endif


#endif
