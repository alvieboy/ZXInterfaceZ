#ifndef __WSYS_SIGNAL_H__
#define __WSYS_SIGNAL_H__

#include <functional>

template<typename... Arg>
class Signal
{
public:
    Signal(): m_function(NULL) {}
    void emit(Arg... arg) { if (m_function) { m_function(arg...); } }
    template<typename T>
        void connect(T *object, void (T::*F)(Arg... arg)) {
            m_function = [=](Arg... arg) { (object->*F)(arg...); };
        }
    bool connected() const { return m_function!=NULL; }
protected:
    std::function<void(Arg...)> m_function;
};

#endif
