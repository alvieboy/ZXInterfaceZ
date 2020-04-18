#include "resource.h"

void version_resource__update(struct resource *res);
int version_resource__sendToFifo(struct resource *res);
uint8_t version_resource__type(struct resource *res);
uint8_t version_resource__len(struct resource *res);

