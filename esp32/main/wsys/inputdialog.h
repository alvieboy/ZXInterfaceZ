#include "dialog.h"
#include "label.h"
#include "button.h"
#include "hlayout.h"
#include "vlayout.h"
#include "editbox.h"

class InputDialog: public Dialog
{
public:
    InputDialog(const char *title, uint8_t w, uint8_t h);

    const char *getText() const { return m_edit->getText(); }

    void setLabel(const char *c) { m_label->setText(c);}
protected:
    void enterClicked();
private:
    Label *m_label;
    EditBox *m_edit;
    VLayout *m_layout;
};
