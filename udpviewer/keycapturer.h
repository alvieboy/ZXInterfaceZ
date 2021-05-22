#ifndef __KEYCAPTURER_H__
#define __KEYCAPTURER_H__

#include <QObject>
#include <QMap>
#include <QKeyEvent>

class KeyCapturer: public QObject
{
    Q_OBJECT
public:
    KeyCapturer();
    bool eventFilter(QObject *obj, QEvent *event) override;
signals:
    void keyChanged(uint64_t keys);
protected:
    bool keyPressEvent(QKeyEvent *);
    bool keyReleaseEvent(QKeyEvent *);
    //QMap<unsigned,bool> m_keys;
    uint64_t m_keys;
};

#endif
