#include "reg16.h"
#include <cstring>

Reg16::Reg16(const char *regname,Widget *parent): Widget(parent)
{
    unsigned rsize = strlen(regname);
    char *dest = m_regname;
    if (rsize>3)
        rsize=3;
    unsigned regsize = rsize;
    while (regsize<3) {
        *dest++=' ';
        regsize++;
    }
    while (rsize--) {
        *dest++=*regname++;
    }
    *dest='\0';
    m_textcolor = attr_t(BLACK, WHITE);
    m_valuecolor = attr_t(BLACK, WHITE);
    m_value = 0;
    redraw();
}



uint8_t Reg16::getMinimumHeight() const {
    return 1;
}

uint8_t Reg16::getMinimumWidth() const
{
    return 3+1+4;
}

void Reg16::drawImpl()
{
    char value[5];
    parentDrawImpl();
    screenptr_t ptr = m_screenptr;
    ptr = ptr.drawstring(m_regname);
    ptr = ptr.drawstring(":");
    sprintf(value, "%04x", m_value);
    ptr.drawstring(value);
}


void Reg16::setValue(uint16_t newvalue)
{
    if (m_value!=newvalue) {
        m_value = newvalue;
        redraw();
    }
}

