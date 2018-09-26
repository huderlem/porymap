#include "parseutil.h"

#include <QDebug>
#include <QRegularExpression>
#include <QStack>

ParseUtil::ParseUtil()
{
}

void ParseUtil::strip_comment(QString *line) {
    bool in_string = false;
    for (int i = 0; i < line->length(); i++) {
        if (line->at(i) == '"') {
            in_string = !in_string;
        } else if (line->at(i) == '@') {
            if (!in_string) {
                line->truncate(i);
                break;
            }
        }
    }
}

QList<QStringList>* ParseUtil::parseAsm(QString text) {
    QList<QStringList> *parsed = new QList<QStringList>;
    QStringList lines = text.split('\n');
    for (QString line : lines) {
        QString label;
        //QString macro;
        //QStringList *params;
        strip_comment(&line);
        if (line.trimmed().isEmpty()) {
        } else if (line.contains(':')) {
            label = line.left(line.indexOf(':'));
            QStringList *list = new QStringList;
            list->append(".label"); // This is not a real keyword. It's used only to make the output more regular.
            list->append(label);
            parsed->append(*list);
            // There should not be anything else on the line.
            // gas will raise a syntax error if there is.
        } else {
            line = line.trimmed();
            //parsed->append(line.split(QRegExp("\\s*,\\s*")));
            QString macro;
            QStringList params;
            int index = line.indexOf(QRegExp("\\s+"));
            macro = line.left(index);
            params = line.right(line.length() - index).trimmed().split(QRegExp("\\s*,\\s*"));
            params.prepend(macro);
            parsed->append(params);
        }
        //if (macro != NULL) {
        //    if (macros->contains(macro)) {
        //        void* function = macros->value(macro);
        //        if (function != NULL) {
        //            std::function function(params);
        //        }
        //    }
        //}
    }
    return parsed;
}

int ParseUtil::evaluateDefine(QString define, QMap<QString, int>* knownDefines) {
    QList<Token> tokens = tokenizeExpression(define, knownDefines);
    QList<Token> postfixExpression = generatePostfix(tokens);
    return evaluatePostfix(postfixExpression);
}

// arg here is the text in the file src/data/heal_locations.h
// returns a list of HealLocations (mapname, x, y)
QList<HealLocation>* ParseUtil::parseHealLocs(QString text) {
    QList<HealLocation> *parsed = new QList<HealLocation>;
    QStringList lines = text.split('\n');

    int i = 1;
    for (auto line : lines){
        if (line.contains("MAP_GROUP")){
            QList<QString> li = line.replace(" ","").chopped(2).remove('{').split(',');
            HealLocation hloc = HealLocation(li[1].remove("MAP_NUM(").remove(")"), i, li[2].toUShort(), li[3].toUShort());
            parsed->append(hloc);
            i++;
        }
    }
    return parsed;
}

QList<Token> ParseUtil::tokenizeExpression(QString expression, QMap<QString, int>* knownIdentifiers) {
    QList<Token> tokens;

    QStringList tokenTypes = (QStringList() << "hex" << "decimal" << "identifier" << "operator" << "leftparen" << "rightparen");
    QRegularExpression re("^(?<hex>0x[0-9a-fA-F]+)|(?<decimal>[0-9]+)|(?<identifier>[a-zA-Z_0-9]+)|(?<operator>[+\\-*\\/<>|^%]+)|(?<leftparen>\\()|(?<rightparen>\\))");

    expression = expression.trimmed();
    while (!expression.isEmpty()) {
        QRegularExpressionMatch match = re.match(expression);
        if (!match.hasMatch()) {
            qDebug() << "Failed to tokenize expression: " << expression;
            break;
        }
        for (QString tokenType : tokenTypes) {
            QString token = match.captured(tokenType);
            if (!token.isEmpty()) {
                if (tokenType == "identifier") {
                    if (knownIdentifiers->contains(token)) {
                        QString actualToken = QString("%1").arg(knownIdentifiers->value(token));
                        expression = expression.replace(0, token.length(), actualToken);
                        token = actualToken;
                        tokenType = "decimal";
                    } else {
                        qDebug() << "Unknown identifier found in expression: " << token;
                    }
                }

                tokens.append(Token(token, tokenType));
                expression = expression.remove(0, token.length()).trimmed();
                break;
            }
        }
    }
    return tokens;
}

QMap<QString, int> Token::precedenceMap = QMap<QString, int>(
{
    {"*", 3},
    {"/", 3},
    {"+", 4},
    {"-", 4},
    {"<<", 5},
    {">>", 5},
    {"&", 8},
    {"^", 9},
    {"|", 10}
});

// Shunting-yard algorithm for generating postfix notation.
// https://en.wikipedia.org/wiki/Shunting-yard_algorithm
QList<Token> ParseUtil::generatePostfix(QList<Token> tokens) {
    QList<Token> output;
    QStack<Token> operatorStack;
    for (Token token : tokens) {
        if (token.type == TokenType::Number) {
            output.append(token);
        } else if (token.value == "(") {
            operatorStack.push(token);
        } else if (token.value == ")") {
            while (!operatorStack.empty() && operatorStack.top().value != "(") {
                output.append(operatorStack.pop());
            }
            if (!operatorStack.empty()) {
                // pop the left parenthesis token
                operatorStack.pop();
            } else {
                qDebug() << "Mismatched parentheses detected in expression!";
            }
        } else {
            // token is an operator
            while (!operatorStack.isEmpty()
                   && operatorStack.top().operatorPrecedence <= token.operatorPrecedence
                   && operatorStack.top().value != "(") {
                output.append(operatorStack.pop());
            }
            operatorStack.push(token);
        }
    }

    while (!operatorStack.isEmpty()) {
        if (operatorStack.top().value == "(" || operatorStack.top().value == ")") {
            qDebug() << "Mismatched parentheses detected in expression!";
        } else {
            output.append(operatorStack.pop());
        }
    }

    return output;
}

// Evaluate postfix expression.
// https://en.wikipedia.org/wiki/Reverse_Polish_notation#Postfix_evaluation_algorithm
int ParseUtil::evaluatePostfix(QList<Token> postfix) {
    QStack<Token> stack;
    for (Token token : postfix) {
        if (token.type == TokenType::Operator) {
            int op2 = stack.pop().value.toInt(nullptr, 0);
            int op1 = stack.pop().value.toInt(nullptr, 0);
            int result = 0;
            if (token.value == "*") {
                result = op1 * op2;
            } else if (token.value == "/") {
                result = op1 / op2;
            } else if (token.value == "+") {
                result = op1 + op2;
            } else if (token.value == "-") {
                result = op1 - op2;
            } else if (token.value == "<<") {
                result = op1 << op2;
            } else if (token.value == ">>") {
                result = op1 >> op2;
            } else if (token.value == "&") {
                result = op1 & op2;
            } else if (token.value == "^") {
                result = op1 ^ op2;
            } else if (token.value == "|") {
                result = op1 | op2;
            } else {
                qDebug() << "Unsupported postfix operator: " << token.value;
            }
            stack.push(Token(QString("%1").arg(result), "decimal"));
        } else {
            stack.push(token);
        }
    }

    return stack.pop().value.toInt(nullptr, 0);
}
