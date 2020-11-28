#include <inttypes.h>

typedef uint8_t STATUS;
#define OK (0)

STATUS uart_rx_one_char(unsigned char *);
STATUS uart_tx_one_char(uint8_t TxChar);
