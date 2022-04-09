#pragma once

#include "widget.h"
#include <string>
#include "color.h"

class SliderBase: public Widget
{
public:
    SliderBase();
    virtual void drawImpl() override = 0;
protected:
    void drawSlider(float percentage, const char *text);
    virtual void inc(bool ismajor) = 0;
    virtual void dec(bool ismajor) = 0;
protected:
    virtual bool handleEvent(wsys_input_event_t) override;
    void drawSliderLine(screenptr_t &ptr, unsigned w, unsigned vlen);

    void focusIn() override
    {
        redraw();
    }
    void focusOut() override
    {
        redraw();
    }

    char m_accel_decrease;
    char m_accel_decrease_h;
    char m_accel_increase;
    char m_accel_increase_h;
};

template<typename T>
class Slider: public SliderBase
{
public:
    Slider() {};


    virtual void drawImpl() override {
        /*
         value = 0;
         min = -1
         max = 1;
         delta  = 1 + 1 = 2;

         v=0, percent=1.0 * (-1-0)/2 = 50
         v=-1, percent = 1.0 * (-1+1)/2 = 0
         v=1 , percent = 1.0 * (-1-1)/2 = 100


         percent = 1.0 * (m_min-value/delta)

         */
        float delta = (float)m_max - (float)m_min;
        if (delta==0.0)
            delta=1.0; // Just to avoid div-by-zero
        float percent = (((float)m_value - (float)m_min))/delta;
        if (percent<0.0)
            percent = 0.0;
        if (percent>1.0)
            percent=1.0;
        //I void Slider<T>::drawImpl() [with T = float]: Percent 0.000000 from 1.000000 min 0.000000 max 1.000000 delta 1.000000

        WSYS_LOGI("Percent %f from %f min %f max %f delta %f\n", percent, m_value, m_min, m_max, delta);
        parentDrawImpl();
        // Draw BG

        attrptr_t attrptr = m_attrptr;
        for (unsigned i=0;i<width();i++) {
            uint8_t attrval;
            if (!hasFocus()) {
                attrval = MAKECOLORA(BLACK, WHITE, BRIGHT);
            } else {
                attrval = 0x68;
                /*if (i<m_spacing)
                    attrval = 0x78;
                if (i >= width()-m_spacing)
                attrval = 0x78;
                */
            }
            *attrptr++ = attrval;
        }



        if (m_drawfunc) {
            char text[16];
            m_drawfunc(m_value, text, sizeof(text));
            drawSlider(percent, text);
        } else {
            drawSlider(percent, "");
        }
    }
    void setMax(T max) { m_max=max; };
    void setMin(T min) { m_min=min; };
    void setMinMax(T min, T max) { m_min=min; m_max=max; }
    void setAccelMinor(char decr, char incr, T value) { m_accel_decrease=decr; m_accel_increase=incr; m_minincdec=value; }
    void setAccelMajor(char decr, char incr, T value) { m_accel_decrease_h=decr; m_accel_increase_h=incr; m_maxincdec=value; }
    void setValue(T value) { m_value=value; redraw(); }
    T getValue() const { return m_value; }
    void setDrawFunc( std::function<void(T value,char *dest, size_t max)> &);
    void inc(bool ismajor) override {
        WSYS_LOGI("INC");
        if (ismajor)
            m_value+=m_maxincdec;
        else
            m_value+=m_minincdec;
        if (m_value>m_max)
            m_value=m_max;
        redraw();
    }
    void dec(bool ismajor) override {
        WSYS_LOGI("DEC");
        if (ismajor)
            m_value-=m_maxincdec;
        else
            m_value-=m_minincdec;
        if (m_value<m_min)
            m_value=m_min;
        redraw();
    }

private:
    T m_value;
    T m_min, m_max;
    T m_minincdec, m_maxincdec;
    std::function< void(T value,char*dest,size_t max) > m_drawfunc;
};


typedef Slider<float> FloatSlider;
