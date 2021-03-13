#ifndef __WSYS_REG16_H__
#define __WSYS_REG16_H__

#include "widget.h"
#include <string>

class Reg16: public Widget
{
public:
    Reg16(const char *regname,Widget *parent=NULL);
    virtual void drawImpl();
    void setValue(uint16_t );
    void setColor(color_t color);
    virtual uint8_t getMinimumHeight() const override;
    virtual uint8_t getMinimumWidth() const override;
    uint16_t value() const { return m_value; }
private:
    char m_regname[4];
    uint16_t m_value;
    attr_t m_textcolor;
    attr_t m_valuecolor;
};


#endif
