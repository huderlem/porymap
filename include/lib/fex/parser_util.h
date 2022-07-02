#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include <QString>
#include <QStringList>

class ParserUtil
{
public:
    ParserUtil(QString root);
    QStringList ReadDefines(QString filename, QString prefix);
    QStringList ReadDefinesValueSort(QString filename, QString prefix);

private:
    QString root_;
};


#endif // PARSER_UTIL_H
