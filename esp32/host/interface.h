#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#undef FPGA_USE_SOCKET_PROTOCOL


#ifdef FPGA_USE_SOCKET_PROTOCOL

int fpga_set_comms_socket(int socket);

#else
typedef int (*spi_transceive_fun_t)(const uint8_t *tx, uint8_t *rx, unsigned size);

int interface__set_comms_fun(
                             spi_transceive_fun_t fun
                            );

void interface__gpio_trigger(int num);
void interface__rawpindata(uint64_t);

#endif



#ifdef __cplusplus
}
#endif

#endif
