#ifndef __PROGRESSBAR_H__
#define __PROGRESSBAR_H__

#include <QDialog>

class QStatusBar;
class QProgressBar;
class QPushButton;

class ProgressBar: public QDialog
{
public:
    typedef enum {
        MODE_SINGLE,
        MODE_DUAL
    } mode_t;
    ProgressBar(QWidget *parent, const QString &title, mode_t mode = MODE_SINGLE);

    void updateDescriptionText(const QString&);
    void close();
    void setStatusText(const QString &text);
    void setPrimaryPercentage(int);
    void setSecondaryPercentage(int);
    void completed();
    void error(const QString&);
signals:
    void onButtonClicked();

private:
    QProgressBar *m_p1, *m_p2;
    QStatusBar *m_status;
    QPushButton *m_button;
    bool m_finished;
};
#endif

