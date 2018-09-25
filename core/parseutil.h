#ifndef PARSEUTIL_H
#define PARSEUTIL_H

#include "core/heallocation.h"

#include <QString>
#include <QList>
#include <QMap>

enum TokenType {
    Number,
    Operator,
};

class Token {
public:
    Token(QString value = "", QString type = "") {
        this->value = value;
        this->type = TokenType::Operator;
        if (type == "decimal" || type == "hex") {
            this->type = TokenType::Number;
            this->operatorPrecedence = -1;
        } else if (type == "operator") {
            this->operatorPrecedence = precedenceMap[value];
        }
    }
    static QMap<QString, int> precedenceMap;
    QString value;
    TokenType type;
    int operatorPrecedence; // only relevant for operator tokens
};

class ParseUtil
{
public:
    ParseUtil();
    void strip_comment(QString*);
    QList<QStringList>* parseAsm(QString);
    int evaluateDefine(QString, QMap<QString, int>*);
    QList<HealLocation>* parseHealLocs(QString);
private:
    QList<Token> tokenizeExpression(QString expression, QMap<QString, int>* knownIdentifiers);
    QList<Token> generatePostfix(QList<Token> tokens);
    int evaluatePostfix(QList<Token> postfix);
};

#endif // PARSEUTIL_H
