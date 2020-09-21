#include "menu.h"

class IndexedMenu: public Menu
{
public:
    IndexedMenu(Widget *parent): Menu(parent) {}

    typedef void (*Function)(void *userptr, uint8_t sel);

    void setFunctionHandler(Function f, void *userdata);
protected:
    virtual void activateEntry(uint8_t entry);

    Function m_function;
    void *m_userdata;
};
