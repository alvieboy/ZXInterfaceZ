#include "inputdialog.h"


InputDialog::InputDialog(const char *title, uint8_t w, uint8_t h): Dialog(title,w,h)
{
    m_layout = new VLayout();
    m_label = new Label("");
    m_edit = new EditBox("");

    m_layout->addChild(m_label);//, LAYOUT_FLAG_VEXPAND);
    m_layout->addChild(m_edit);
    m_edit->setEditable(true);
    m_edit->enter().connect(this, &InputDialog::enterClicked);

    setChild(m_layout);
}

void InputDialog::enterClicked()
{
    setResult(0);
}
