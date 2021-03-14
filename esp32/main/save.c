#include "save.h"
#include "fpga.h"
#include "byteorder.h"
#include "struct_assert.h"
#include <string.h>
#include "tap.h"
#include "resource.h"
#include "fileaccess.h"
#include "errno.h"
#include "tape.h"

static char savefilename[11];

// NOTE NOTE NOTE: we need a semaphore here!!!

static int tape_fd = -1;

typedef struct {
    uint8_t flag;
    struct spectrum_tape_header hdr;
} __attribute__((packed)) spectrum_save_tape_header_t;

ASSERT_STRUCT_SIZE(spectrum_save_tape_header_t, 18);

void save__notify_save_to_tap()
{
    uint8_t status = 0x01;
    fpga__load_resource_fifo(&status, 1, RESOURCE_DEFAULT_TIMEOUT);
}

void save__notify_no_save()
{
    uint8_t status = 0xff;
    fpga__load_resource_fifo(&status, 1, RESOURCE_DEFAULT_TIMEOUT);
}

void save__set_data_from_header(const uint8_t *data, unsigned size)
{
    char *dest = &savefilename[0];
    if (size==sizeof(spectrum_save_tape_header_t)) {
        const spectrum_save_tape_header_t *tape_hdr =
            (const spectrum_save_tape_header_t*)data;
        const char *c = &tape_hdr->hdr.filename[0];
        int len = 10;
        while (*c!=0x20 && len>0) {
            *dest++ = *c++;
        }
    }
    *dest='\0';
}

const char *save__get_requested_name()
{
    return savefilename;
}

void save__stop_tape()
{
    if (tape_fd>0) {
        close(tape_fd);
        tape_fd=-1;
    }
}

int save__start_save_tap(const char *filename,
                         bool append)
{
    char name[128];

    if (tape__is_tape_loaded()) {
        ESP_LOGE(TAG, "Tape already inserted");
        return -1;
    }

    if (tape_fd>=0) {
        close(tape_fd);
    }

    if (fullpath(filename, name, sizeof(name))==NULL) {
        ESP_LOGE(TAG, "Path too long: %s", name);
        return -1;
    }

    if (append) {
        tape_fd = __open(name, O_WRONLY | O_CREAT | O_APPEND, 0666);
    } else {
        tape_fd = __open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
    if (tape_fd<0) {
        ESP_LOGE(TAG, "Cannot open %s: %s", name, strerror(errno));

        return tape_fd;
    }

    if (tape__enter_tape_mode(TAPE_TAP_SAVE)<0) {
        ESP_LOGE(TAG, "Cannot start tape");
        if (tape_fd>=0)
            close(tape_fd);
        return -1;
    }

    return 0;
}

int save__append_from_buffer(const uint8_t *data, uint16_t datalen)
{
    le_uint16_t dlen = __le16(datalen);
    if (tape_fd<0)
        return -1;

    if (write(tape_fd, &dlen, 2)!=2) {
        ESP_LOGE(TAG, "Short file write");
        return -1;
    }

    tape__notify_save();

    return write(tape_fd, data, datalen)==datalen?0:-1;
}

int save__append_from_extram(uint32_t address, uint16_t datalen)
{
    le_uint16_t dlen = __le16(datalen +1);
    uint8_t checksum = 0;

    if (tape_fd<0)
        return -1;

    if (write(tape_fd, &dlen, 2)!=2) {
        ESP_LOGE(TAG, "Short file write");
        return -1;
    }

    int r =fpga__read_extram_block_into_file(address,
                                             tape_fd,
                                             datalen,
                                             &checksum);
    if (r<0)
        return -1;

    tape__notify_save();

    return write(tape_fd, &checksum, 1)==1 ? 0:-1;

}

void save__start_save_physical()
{
    tape__enter_tape_mode(TAPE_PHYSICAL_SAVE);
    save__notify_no_save();
}
