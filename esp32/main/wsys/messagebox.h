#pragma once

#include "dialog.h"
#include "vlayout.h"
#include "hlayout.h"
#include "label.h"
#include "button.h"

class MessageBox: public Dialog {
public:
    MessageBox(const char *title="Message", uint8_t w=28, uint8_t h=8);

    static void show(const char *message, const char *button_text=NULL);
    const std::vector<Button*> buttons() const;
    void setText(const char *c) { m_text->setText(c); }
    Button* addButton(const char *text, int retval);

protected:
    void buttonClicked(Button *, int retval);
private:
    VLayout *m_layout;
    Label *m_text;
    HLayout *m_buttonlayout;
    std::vector<Button*> m_buttons;
    //std::vector<int> m_retvals;
    Signal<Button*, int> m_clicked;
};
