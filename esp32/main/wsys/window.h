#include "bin.h"

class Window: public Bin
{
public:
    Window(const char *title, uint8_t w, uint8_t h);
    ~Window();
    // Virtual methods
    virtual void resizeEvent() override;
    void setTitle(const char *title);

protected:
    void fillHeaderLine(attrptr_t attr);
    void setBackground(uint8_t);
    virtual void drawImpl() override;

    const char *m_title;
};


