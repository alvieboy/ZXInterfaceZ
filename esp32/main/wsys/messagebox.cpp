#include "messagebox.h"
#include "screen.h"

MessageBox::MessageBox(const char *title,uint8_t w, uint8_t h): Dialog(title, w, h)
{
    m_layout = new VLayout();
    m_text = new Label();
    m_close = new Button(NULL,"Close [ENTER]");

    setChild(m_layout);
    m_layout->addChild(m_text, LAYOUT_FLAG_VEXPAND);
    m_layout->addChild(m_close);

    m_close->clicked().connect(this, &MessageBox::clicked );
}

static void nocallback(void *win, int val)
{
    screen__removeWindow(static_cast<Window*>(win));
}

void MessageBox::show(const char *text)
{
    MessageBox *m = new MessageBox();
    m->setText(text);
    m->exec(nocallback, NULL);
}

void MessageBox::clicked()
{
    screen__removeWindow(this);
}
