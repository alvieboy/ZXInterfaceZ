#include "LineBuffer.h"
#include <QDebug>
#include <QByteArray>

LineBuffer::LineBuffer(unsigned size, QObject*parent): QObject(parent)
{
    m_data = new char[size];
    m_maxDataSize = size;
    reset();
}

void LineBuffer::reset()
{
    m_dataSize = 0;
}

LineBuffer::~LineBuffer(){
    delete[] m_data;
}

void LineBuffer::append(const char *c, unsigned size)
{
    if (m_dataSize+size < m_maxDataSize) {
        char *end;
        memcpy(&m_data[m_dataSize], c, size);
        m_dataSize+=size;
        m_data[m_dataSize] = '\0';
        while ((end=strchr(m_data,'\n'))) {
            // Cut.
            *end='\0';
            //processSerialLine(serialData);
            QString str = QString(m_data);
            emit lineReceived(str);
            // Move data to beginning
            end++;
            unsigned leftover = m_dataSize - (end-m_data);
            if (leftover) {
                memmove(m_data, end, leftover);
            }
            m_dataSize = leftover;

            m_data[m_dataSize]='\0';
        }

    }
}

void LineBuffer::append(const char *c)
{
    append(c,strlen(c));
}

void LineBuffer::append(const QByteArray &b)
{
    append(b.constData(), b.length());
}
