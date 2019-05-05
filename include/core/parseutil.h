#ifndef PARSEUTIL_H
#define PARSEUTIL_H

#include "heallocation.h"
#include "log.h"

#include <QString>
#include <QList>
#include <QMap>

enum TokenType {
    Number,
    Operator,
    Error,
};

class DebugInfo {
public:
    DebugInfo(QString file, QStringList lines) {
        this->file = file;
        this->lines = lines;
    };
    QString file;
    int     line;
    bool    err;
    QStringList lines;
    void error(QString expression, QString token) {
        int lineNo = 0;
        for (QString line_ : lines) {
            lineNo++;
            if (line_.contains(expression)) {
                this->line = lineNo;
                break;
            }
        }
        logError(QString("%1:%2: unknown identifier found in expression: '%3'.").arg(file).arg(line).arg(token));
    }
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
        } else if (type == "error") {
            this->type = TokenType::Error;
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
    ParseUtil(QString, QString);
    void strip_comment(QString*);
    QList<QStringList>* parseAsm(QString);
    int evaluateDefine(QString, QMap<QString, int>*);
    DebugInfo *debug;
private:
    QList<Token> tokenizeExpression(QString expression, QMap<QString, int>* knownIdentifiers);
    QList<Token> generatePostfix(QList<Token> tokens);
    int evaluatePostfix(QList<Token> postfix);
};

#endif // PARSEUTIL_H
