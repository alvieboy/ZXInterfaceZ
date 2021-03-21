#include "multiwidget.h"
#include "spectrum_kbd.h"
//#define MAX_CHILDS 4

void MultiWidget::draw(bool force)
{
    WSYS_LOGI( "MultiWidget::draw force=%d damage=0x%02x", force?1:0, damage());

    if (force || (damage() & ~DAMAGE_CHILD)) { // If any bits beside child, then draw
        drawImpl();
    }

    for (int i=0;i<getNumberOfChildren();i++) {
        WSYS_LOGI( "MultiWidget::draw child force=%d child_damage=0x%02x", force?1:0, m_childs[i]->damage());

        if ((force || (damage() & DAMAGE_CHILD)))  {
            if (force || m_childs[i]->damage()) {
                WSYS_LOGI("Forcing child redraw %s", CLASSNAME(*m_childs[i]));
                m_childs[i]->draw(force);
            }

            m_childs[i]->clear_damage();
        }
    }

    clear_damage();


}

bool MultiWidget::handleLocalEvent(wsys_input_event_t evt)
{
    WSYS_LOGI("MultiWidget handleLocalEvent");
    if (evt.type==WSYS_INPUT_EVENT_KBD) {
        unsigned char c = spectrum_kbd__to_ascii(evt.code.v);
        WSYS_LOGI("MultiWidget handleLocalEvent key %02x", c);
        switch (c) {
        case KEY_RIGHT:
            WSYS_LOGI("MultiWidget request focus next");
            focusNextPrev(true);
            break;
        case KEY_LEFT:
            WSYS_LOGI("MultiWidget request focus prev");
            focusNextPrev(false);
            break;
        default:
            break;
        }
    }
    if (evt.type==WSYS_INPUT_EVENT_JOYSTICK && evt.joy_on) {
        switch (evt.joy_action) {
        case JOY_RIGHT:
            WSYS_LOGI("MultiWidget request focus next");
            focusNextPrev(true);
            break;
        case JOY_LEFT:
            WSYS_LOGI("MultiWidget request focus prev");
            focusNextPrev(false);
            break;
        default:
            break;
        }
    }
    return false;
}

bool MultiWidget::handleEvent(wsys_input_event_t evt)
{
    return handleLocalEvent(evt);
}

void MultiWidget::setdamage(uint8_t mask)
{
    Widget::setdamage(mask);
    if (mask!=DAMAGE_CHILD) {
        for (int i=0;i<getNumberOfChildren();i++) {
            m_childs[i]->setdamage(mask);
        }
    }
}

void MultiWidget::addChild(Widget *w)
{
    Widget *last = lastChild();

    w->focusInsertAfter(last!=NULL ? last : this);

    m_childs.push_back(w);
    w->setParent(this);
    WSYS_LOGI("DUMP FOCUS TREE after add child %s to %s", CLASSNAME(*w), CLASSNAME(*this));
    dumpFocusTree(this);
    WSYS_LOGI("END DUMP FOCUS TREE");

#if 0
    if (!m_focusWidget) {
        if (w->canFocus()) {
            m_focusWidget = w;
            WSYS_LOGI("Setting focus to child %s", CLASSNAME(*w));
            if (hasFocus())
                w->setFocus(true);          // First child grabs focus
        }
    }
#endif
//    w->setVisible(isVisible());
    resizeEvent();
    setdamage(DAMAGE_CHILD);
}

MultiWidget::~MultiWidget()
{
    for (auto w: m_childs) {
        delete(w);
    }
    m_childs.clear();
}

void MultiWidget::focusIn()
{
    WSYS_LOGI("MultiWidget Focus IN");
#if 0

    if (!m_focusWidget)
        m_focusWidget = findNextFocusable(0);

    if (m_focusWidget) {
#ifdef __linux__
        WSYS_LOGI("Focusing %s", typeid(*m_focusWidget).name());

#endif
        m_focusWidget->setFocus(true);
    } else {
        WSYS_LOGI("No child to focus");
    }
#endif
}
void MultiWidget::focusOut()
{
    WSYS_LOGI("MultiWidget Focus OUT");
#if 0
    if (m_focusWidget) {
        m_focusWidget->setFocus(false);
    }
#endif
}

int MultiWidget::getChild(Widget *c)
{
    int i;
    for (i=0;i<getNumberOfChildren();i++) {
        if (m_childs[i]==c)
            return i;
    }
    return -1;
}
    /*
Widget *MultiWidget::findNextFocusable(int start)
{
    if (getNumberOfChildren()==0 || start>=getNumberOfChildren())
        return NULL;

    int i = start;
    do {
        i++;
        if (i==getNumberOfChildren())
            i=0;

        Widget *candidate = m_childs[i];
        if (candidate->canFocus()) {
            WSYS_LOGI("Returning focus candidate %s\n", CLASSNAME(*this));
            return candidate;
        }
    } while (i!=start);
    return NULL;
}

Widget *MultiWidget::findPreviousFocusable(int start)
{
    WSYS_LOGI("Find prev focusable this=%p", this);
    WSYS_LOGI("Find prev focusable start %d count %d this=%p", start, getNumberOfChildren(), this);
    if (getNumberOfChildren()==0 || start>=getNumberOfChildren())
        return NULL;
    int i = start;
    do {
        i--;
        if (i<0)
            i=getNumberOfChildren()-1;

        Widget *candidate = m_childs[i];
        if (candidate->canFocus()) {
            WSYS_LOGI("Returning focus candidate %s\n", CLASSNAME(*this));
            return candidate;
        }
    } while (i!=start);
    return NULL;
}

void MultiWidget::focusNext()
{
    if (getNumberOfChildren()==0)
        return;
    focus( &MultiWidget::findNextFocusable );
}

void MultiWidget::focusPrev()
{
    if (getNumberOfChildren()==0)
        return;
    focus( &MultiWidget::findPreviousFocusable );
}

void MultiWidget::focus( Widget *(MultiWidget::*find_fun)(int start) )
{
    WSYS_LOGI("MultiWidget:: Focus next");
    int start = 0;
    if (m_focusWidget) {
        start = getChild(m_focusWidget);
        if (start<0) {
            WSYS_LOGE("Cannot find child!");
            return;
        }
    }
    Widget *next = (this->*find_fun)(start);
    if (next) {
        if (m_focusWidget)
            m_focusWidget->setFocus(false);
        next->setFocus(true);
        m_focusWidget = next;
    }
}
     */
bool MultiWidget::canFocus() const
{
    /*
    for (auto i: m_childs) {
        if (i->canFocus())
            return true;
    } */
    return false;
}

void MultiWidget::setFocus(bool focus)
{
#if 0
    Widget::setFocus(focus);
    if (m_focusWidget)
        m_focusWidget->setFocus(focus);
#endif
}


void MultiWidget::setVisible(bool visible)
{
    Widget::setVisible(visible);
    /*
    for (auto i: m_childs)
    i->setVisible(visible);
    */
}

int MultiWidget::getNumberOfVisibleChildren() const
{
    int count = 0;
    for (auto i: m_childs)
        if (i->isVisible())
            count++;
    return count;
}

