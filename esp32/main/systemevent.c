#include "systemevent.h"

void systemevent__send(const systemevent_t *event)
{
    systemevent__handleevent(event);
}
