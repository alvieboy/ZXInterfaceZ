#include "systemevent.h"

void systemevent__send_event(const systemevent_t *event)
{
    systemevent__handleevent(event);
}
