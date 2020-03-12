#include "ProgressBar.h"
#include <QProgressBar>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QStatusBar>
#include <QPushButton>

ProgressBar::ProgressBar(QWidget *parent, const QString &title, mode_t mode): QDialog(parent)
{
    setWindowTitle(title);
    m_status = new QStatusBar(this);
    QVBoxLayout *l = new QVBoxLayout();
    setLayout(l);

    m_p1 = new QProgressBar(this);
    l->addWidget(m_p1);
    m_p1->setValue(0);
    m_p1->setTextVisible(true);
    if (mode == MODE_DUAL) {
        m_p2 = new QProgressBar(this);
        l->addWidget(m_p2);
        m_p2->setValue(0);
        m_p2->setTextVisible(true);
    } else {
        m_p2 = NULL;
    }

    l->addWidget(m_status);

    m_finished = false;

    m_button = new QPushButton("Cancel");
    connect(m_button, &QPushButton::clicked, this, &ProgressBar::onButtonClicked);
    l->addWidget(m_button);
}

void ProgressBar::setStatusText(const QString &text)
{
    m_status->showMessage(text);
}

void ProgressBar::setPrimaryPercentage(int v)
{
    m_p1->setValue(v);
}

void ProgressBar::setSecondaryPercentage(int v)
{
    if (m_p2)
        m_p2->setValue(v);

}

void ProgressBar::completed()
{
    m_button->setText("Close");
    m_finished=true;
}

void ProgressBar::error(const QString &s)
{
    setStatusText(s);
    m_button->setText("Close");
    m_finished=true;
}

void ProgressBar::onButtonClicked()
{
    if (m_finished) {
        destroy();
    } else {
    }
}