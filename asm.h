#ifndef ASM_H
#define ASM_H

#include <QString>
#include <QList>

class Asm
{
public:
    Asm();
    void strip_comment(QString*);
    QList<QStringList>* parse(QString);
};

#endif // ASM_H
