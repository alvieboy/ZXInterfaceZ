#include "os/queue.h"

typedef Queue xQueueHandle;

#define xQueueReceive queue__receive
#define xQueueCreate queue__create
#define xQueueSend queue__send
#define xQueueSendFromISR queue__send_from_isr
