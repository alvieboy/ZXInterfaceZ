#include "menuwindowindexed.h"


class VideoModeWindow: public MenuWindowIndexed
{
public:
    VideoModeWindow();
    void set(uint8_t index);
};


void videomodemenu__show();


