#ifndef __LINEBUFFER_H__
#define __LINEBUFFER_H__

#include <QObject>
class QByteArray;

class LineBuffer: public QObject
{
    Q_OBJECT
public:
    LineBuffer(unsigned size=8192, QObject*parent=NULL);
    virtual ~LineBuffer();
    void reset();
    void append(const char *data, unsigned size);
    void append(const char *data);
    void append(const QByteArray &);
private:
    LineBuffer(const LineBuffer&);
    LineBuffer&operator=(const LineBuffer&);
signals:
    void lineReceived(QString); // Sub-optimal...
private:
    char *m_data;
    unsigned m_dataSize;
    unsigned m_maxDataSize;
};


#endif
