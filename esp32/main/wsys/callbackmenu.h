#include "menu.h"

class CallbackMenu: public Menu
{
public:
    CallbackMenu(Widget *parent): Menu(parent) {}

    typedef void (*Function)(void);

    void setCallbackTable(const Function f[]);

protected:
    virtual void activateEntry(uint8_t entry);

    const Function *m_functions;
    void *m_userdata;
};

