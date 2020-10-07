#include "messagebox.h"
#include "screen.h"

MessageBox::MessageBox(const char *title,uint8_t w, uint8_t h): Dialog(title, w, h)
{
    m_layout = create<VLayout>();
    m_text = create<Label>("");
    m_close = create<Button>("Close [ENTER]");

    setChild(m_layout);
    m_layout->addChild(m_text, LAYOUT_FLAG_VEXPAND);
    m_layout->addChild(m_close);

    m_close->clicked().connect(this, &MessageBox::buttonClicked );
}

void MessageBox::show(const char *text, const char *button_text)
{
    MessageBox *m = create<MessageBox>();
    m->setText(text);
    if (button_text)
        m->setButtonText(button_text);
    m->exec();
    m->destroy();
}

void MessageBox::buttonClicked()
{
    if (m_clicked.connected()) {
        m_clicked.emit();
    } else {
        screen__removeWindow(this);
    }
}

void MessageBox::setButtonText(const char *text)
{
    m_close->setText(text);
}

