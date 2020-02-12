#include "log.h"
#include "parseutil.h"

#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStack>

ParseUtil::ParseUtil()
{
}

void ParseUtil::set_root(QString dir) {
    this->root = dir;
}

void ParseUtil::error(QString message, QString expression) {
    QStringList lines = text.split(QRegularExpression("[\r\n]"));
    int lineNum = 0, colNum = 0;
    for (QString line : lines) {
        lineNum++;
        colNum = line.indexOf(expression) + 1;
        if (colNum) break;
    }
    logError(QString("%1:%2:%3: %4").arg(file).arg(lineNum).arg(colNum).arg(message));
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

QString ParseUtil::readTextFile(QString path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open '%1': ").arg(path) + file.errorString());
        return QString();
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString text = "";
    while (!in.atEnd()) {
        text += in.readLine() + "\n";
    }
    return text;
}

QList<QStringList>* ParseUtil::parseAsm(QString filename) {
    QList<QStringList> *parsed = new QList<QStringList>;

    text = readTextFile(root + "/" + filename);
    QStringList lines = text.split('\n');
    for (QString line : lines) {
        QString label;
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
    }
    return parsed;
}

int ParseUtil::evaluateDefine(QString define, QMap<QString, int>* knownDefines) {
    QList<Token> tokens = tokenizeExpression(define, knownDefines);
    QList<Token> postfixExpression = generatePostfix(tokens);
    return evaluatePostfix(postfixExpression);
}

QList<Token> ParseUtil::tokenizeExpression(QString expression, QMap<QString, int>* knownIdentifiers) {
    QList<Token> tokens;

    QStringList tokenTypes = (QStringList() << "hex" << "decimal" << "identifier" << "operator" << "leftparen" << "rightparen");
    QRegularExpression re("^(?<hex>0x[0-9a-fA-F]+)|(?<decimal>[0-9]+)|(?<identifier>[a-zA-Z_0-9]+)|(?<operator>[+\\-*\\/<>|^%]+)|(?<leftparen>\\()|(?<rightparen>\\))");

    expression = expression.trimmed();
    while (!expression.isEmpty()) {
        QRegularExpressionMatch match = re.match(expression);
        if (!match.hasMatch()) {
            logWarn(QString("Failed to tokenize expression: '%1'").arg(expression));
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
                        tokenType = "error";
                        QString message = QString("unknown token '%1' found in expression '%2'")
                                          .arg(token).arg(expression);
                        error(message, expression);
                    }
                }
                else if (tokenType == "operator") {
                    if (!Token::precedenceMap.contains(token)) {
                        QString message = QString("unsupported postfix operator: '%1'")
                                          .arg(token);
                        error(message, expression);
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
        if (token.type == TokenClass::Number) {
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
                logError("Mismatched parentheses detected in expression!");
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
            logError("Mismatched parentheses detected in expression!");
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
        if (token.type == TokenClass::Operator && stack.size() > 1) {
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
            }
            stack.push(Token(QString("%1").arg(result), "decimal"));
        } else if (token.type != TokenClass::Error) {
            stack.push(token);
        } // else ignore errored tokens, we have already warned the user.
    }

    return stack.pop().value.toInt(nullptr, 0);
}

QString ParseUtil::readCIncbin(QString filename, QString label) {
    QString path;

    if (label.isNull()) {
        return path;
    }

    text = readTextFile(root + "/" + filename);

    QRegExp *re = new QRegExp(QString(
        "\\b%1\\b"
        "\\s*\\[?\\s*\\]?\\s*=\\s*"
        "INCBIN_[US][0-9][0-9]?"
        "\\(\\s*\"([^\"]*)\"\\s*\\)").arg(label));

    int pos = re->indexIn(text);
    if (pos != -1) {
        path = re->cap(1);
    }

    return path;
}

QMap<QString, int> ParseUtil::readCDefines(QString filename, QStringList prefixes) {
    QMap<QString, int> allDefines;
    QMap<QString, int> filteredDefines;

    file = filename;

    if (file.isEmpty()) {
        return filteredDefines;
    }

    QString filepath = root + "/" + file;
    text = readTextFile(filepath);

    if (text.isNull()) {
        logError(QString("Failed to read C defines file: '%1'").arg(filepath));
        return filteredDefines;
    }

    text.replace(QRegularExpression("(//.*)|(\\/+\\*+[^*]*\\*+\\/+)"), "");
    text.replace(QRegularExpression("(\\\\\\s+)"), "");

    QRegularExpression re("#define\\s+(?<defineName>\\w+)[^\\S\\n]+(?<defineValue>.+)");
    QRegularExpressionMatchIterator iter = re.globalMatch(text);
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString name = match.captured("defineName");
        QString expression = match.captured("defineValue");
        if (expression == " ") continue;
        int value = evaluateDefine(expression, &allDefines);
        allDefines.insert(name, value);
        for (QString prefix : prefixes) {
            if (name.startsWith(prefix) || QRegularExpression(prefix).match(name).hasMatch()) {
                filteredDefines.insert(name, value);
            }
        }
    }
    return filteredDefines;
}

void ParseUtil::readCDefinesSorted(QString filename, QStringList prefixes, QStringList* definesToSet) {    
    QMap<QString, int> defines = readCDefines(filename, prefixes);

    // The defines should to be sorted by their underlying value, not alphabetically.
    // Reverse the map and read out the resulting keys in order.
    QMultiMap<int, QString> definesInverse;
    for (QString defineName : defines.keys()) {
        definesInverse.insert(defines[defineName], defineName);
    }
    *definesToSet = definesInverse.values();
}

QStringList ParseUtil::readCArray(QString filename, QString label) {
    QStringList list;

    if (label.isNull()) {
        return list;
    }

    file = filename;
    text = readTextFile(root + "/" + filename);

    QRegularExpression re(QString(R"(\b%1\b\s*(\[?[^\]]*\])?\s*=\s*\{([^\}]*)\})").arg(label));
    QRegularExpressionMatch match = re.match(text);

    if (match.hasMatch()) {
        QString body = match.captured(2);
        QStringList split = body.split(',');
        for (QString item : split) {
            item = item.trimmed();
            if (!item.contains(QRegularExpression("[^A-Za-z0-9_&()\\s]"))) list.append(item);
            // do not print error info here because this is called dozens of times
        }
    }

    return list;
}

QMap<QString, QString> ParseUtil::readNamedIndexCArray(QString filename, QString label) {
    text = readTextFile(root + "/" + filename);
    QMap<QString, QString> map;

    QRegularExpression re_text(QString(R"(\b%1\b\s*(\[?[^\]]*\])?\s*=\s*\{([^\}]*)\})").arg(label));
    QString body = re_text.match(text).captured(2).replace(QRegularExpression("\\s*"), "");
    
    QRegularExpression re("\\[(?<index>[A-Za-z1-9_]*)\\]=(?<value>&?[A-Za-z1-9_]*)");
    QRegularExpressionMatchIterator iter = re.globalMatch(body);

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString key = match.captured("index");
        QString value = match.captured("value");
        map.insert(key, value);
    }

    return map;
}

QList<QStringList>* ParseUtil::getLabelMacros(QList<QStringList> *list, QString label) {
    bool in_label = false;
    QList<QStringList> *new_list = new QList<QStringList>;
    for (int i = 0; i < list->length(); i++) {
        QStringList params = list->value(i);
        QString macro = params.value(0);
        if (macro == ".label") {
            if (params.value(1) == label) {
                in_label = true;
            } else if (in_label) {
                // If nothing has been read yet, assume the label
                // we're looking for is in a stack of labels.
                if (new_list->length() > 0) {
                    break;
                }
            }
        } else if (in_label) {
            new_list->append(params);
        }
    }
    return new_list;
}

// For if you don't care about filtering by macro,
// and just want all values associated with some label.
QStringList* ParseUtil::getLabelValues(QList<QStringList> *list, QString label) {
    list = getLabelMacros(list, label);
    QStringList *values = new QStringList;
    for (int i = 0; i < list->length(); i++) {
        QStringList params = list->value(i);
        QString macro = params.value(0);
        if (macro == ".align" || macro == ".ifdef" || macro == ".ifndef") {
            continue;
        }
        for (int j = 1; j < params.length(); j++) {
            values->append(params.value(j));
        }
    }
    return values;
}

bool ParseUtil::tryParseJsonFile(QJsonDocument *out, QString filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Error: Could not open %1 for reading").arg(filepath));
        return false;
    }

    QByteArray data = file.readAll();
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError) {
        logError(QString("Error: Failed to parse json file %1: %2").arg(filepath).arg(parseError.errorString()));
        return false;
    }

    *out = jsonDoc;
    return true;
}

bool ParseUtil::ensureFieldsExist(QJsonObject obj, QList<QString> fields) {
    for (QString field : fields) {
        if (!obj.contains(field)) {
            logError(QString("JSON object is missing field '%1'.").arg(field));
            return false;
        }
    }
    return true;
}
