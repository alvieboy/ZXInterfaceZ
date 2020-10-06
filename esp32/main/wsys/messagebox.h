#include "dialog.h"
#include "vlayout.h"
#include "label.h"
#include "button.h"
class MessageBox: public Dialog {
public:
    MessageBox(const char *title="Message", uint8_t w=28, uint8_t h=8);

    static void show(const char *message, const char *button_text=NULL);
    void setText(const char *text) { m_text->setText(text); }
    void setButtonText(const char *text);
protected:
    void buttonClicked();
private:
    VLayout *m_layout;
    Label *m_text;
    Button *m_close;
    Signal<> m_clicked;
};
