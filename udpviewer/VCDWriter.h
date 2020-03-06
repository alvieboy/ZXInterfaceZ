#ifndef __VCDWRITER_H__
#define __VCDWRITER_H__

#include <cstdio>

class QByteArray;
class QString;

class VCDWriter
{
public:
    VCDWriter();

    int write(QByteArray *data, FILE*f);
    int write(QByteArray *data, const QString &file);
private:
    double period = 10.41;

};

#endif
