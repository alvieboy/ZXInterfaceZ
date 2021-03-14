#include "messagebox.h"
#include "screen.h"

MessageBox::MessageBox(const char *title,uint8_t w, uint8_t h): Dialog(title, w, h)
{
    m_layout = create<VLayout>();
    m_buttonlayout = create<HLayout>();
    m_text = create<Label>("");
    setChild(m_layout);
    m_layout->addChild(m_text, LAYOUT_FLAG_VEXPAND);
    m_layout->addChild(m_buttonlayout);
    m_buttonlayout->setSpacing(1);
}


Button *MessageBox::addButton(const char *text, int retval)
{
    Button *b  = create<Button>(text);
    m_buttonlayout->addChild(b, LAYOUT_FLAG_HEXPAND);
    m_buttons.push_back(b);

    b->clicked().connect(
                         [this,b,retval]{ this->buttonClicked(b,retval); }
                        );

    return b;
}

void MessageBox::show(const char *text, const char *button_text)
{
    MessageBox *m = create<MessageBox>();
    m->setText(text);

    Button *b  = m->addButton(button_text, 0);
    if (b && button_text)
        b->setText(button_text);

    m->exec();
    //m->destroy();
}

void MessageBox::buttonClicked(Button *b, int retval)
{
    WSYS_LOGI("Button clicked, retval %d", retval);
    if (m_clicked.connected()) {
        m_clicked.emit(b, retval);
    } else {
        setResult(retval);
        //screen__removeWindow(this);
    }
}
