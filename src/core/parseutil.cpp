#include "log.h"
#include "parseutil.h"

#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStack>

const QRegularExpression ParseUtil::re_incScriptLabel("\\b(?<label>[\\w_][\\w\\d_]*):{1,2}");
const QRegularExpression ParseUtil::re_globalIncScriptLabel("\\b(?<label>[\\w_][\\w\\d_]*)::");
const QRegularExpression ParseUtil::re_poryScriptLabel("\\b(script)(\\((global|local)\\))?\\s*\\b(?<label>[\\w_][\\w\\d_]*)");
const QRegularExpression ParseUtil::re_globalPoryScriptLabel("\\b(script)(\\((global)\\))?\\s*\\b(?<label>[\\w_][\\w\\d_]*)");
const QRegularExpression ParseUtil::re_poryRawSection("\\b(raw)\\s*`(?<raw_script>[^`]*)");

void ParseUtil::set_root(const QString &dir) {
    this->root = dir;
}

void ParseUtil::error(const QString &message, const QString &expression) {
    QStringList lines = text.split(QRegularExpression("[\r\n]"));
    int lineNum = 0, colNum = 0;
    for (QString line : lines) {
        lineNum++;
        colNum = line.indexOf(expression) + 1;
        if (colNum) break;
    }
    logError(QString("%1:%2:%3: %4").arg(file).arg(lineNum).arg(colNum).arg(message));
}

QString ParseUtil::readTextFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open '%1': ").arg(path) + file.errorString());
        return QString();
    }
    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif // Qt6 defaults to UTF-8, but setCodec is renamed to setEncoding
    QString text = "";
    while (!in.atEnd()) {
        text += in.readLine() + '\n';
    }
    return text;
}

int ParseUtil::textFileLineCount(const QString &path) {
    const QString text = readTextFile(path);
    return text.split('\n').count() + 1;
}

QList<QStringList> ParseUtil::parseAsm(const QString &filename) {
    QList<QStringList> parsed;

    text = readTextFile(root + '/' + filename);
    const QStringList lines = removeLineComments(text, "@").split('\n');
    for (const auto &line : lines) {
        const QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) {
            continue;
        }

        if (line.contains(':')) {
            const QString label = line.left(line.indexOf(':'));
            const QStringList list{ ".label", label }; // .label is not a real keyword. It's used only to make the output more regular.
            parsed.append(list);
            // There should not be anything else on the line.
            // gas will raise a syntax error if there is.
        } else {
            int index = trimmedLine.indexOf(QRegularExpression("\\s+"));
            const QString macro = trimmedLine.left(index);
            QStringList params(trimmedLine.right(trimmedLine.length() - index).trimmed().split(QRegularExpression("\\s*,\\s*")));
            params.prepend(macro);
            parsed.append(params);
        }
    }
    return parsed;
}

int ParseUtil::evaluateDefine(const QString &define, const QMap<QString, int> &knownDefines) {
    QList<Token> tokens = tokenizeExpression(define, knownDefines);
    QList<Token> postfixExpression = generatePostfix(tokens);
    return evaluatePostfix(postfixExpression);
}

QList<Token> ParseUtil::tokenizeExpression(QString expression, const QMap<QString, int> &knownIdentifiers) {
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
                    if (knownIdentifiers.contains(token)) {
                        QString actualToken = QString("%1").arg(knownIdentifiers.value(token));
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
    {"%", 3},
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
QList<Token> ParseUtil::generatePostfix(const QList<Token> &tokens) {
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
int ParseUtil::evaluatePostfix(const QList<Token> &postfix) {
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
            } else if (token.value == "%") {
                result = op1 % op2;
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
    return stack.size() ? stack.pop().value.toInt(nullptr, 0) : 0;
}

QString ParseUtil::readCIncbin(const QString &filename, const QString &label) {
    QString path;

    if (label.isNull()) {
        return path;
    }

    text = readTextFile(root + "/" + filename);

    QRegularExpression re(QString(
        "\\b%1\\b"
        "\\s*\\[?\\s*\\]?\\s*=\\s*"
        "INCBIN_[US][0-9][0-9]?"
        "\\(\\s*\"([^\"]*)\"\\s*\\)").arg(label));

    QRegularExpressionMatch match;
    qsizetype pos = text.indexOf(re, 0, &match);
    if (pos != -1) {
        path = match.captured(1);
    }

    return path;
}

QMap<QString, int> ParseUtil::readCDefines(const QString &filename,
                                           const QStringList &prefixes,
                                           QMap<QString, int> allDefines)
{
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
    allDefines.insert("FALSE", 0);
    allDefines.insert("TRUE", 1);

    QRegularExpression re("#define\\s+(?<defineName>\\w+)[^\\S\\n]+(?<defineValue>.+)");
    QRegularExpressionMatchIterator iter = re.globalMatch(text);
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString name = match.captured("defineName");
        QString expression = match.captured("defineValue");
        if (expression == " ") continue;
        int value = evaluateDefine(expression, allDefines);
        allDefines.insert(name, value);
        for (QString prefix : prefixes) {
            if (name.startsWith(prefix) || QRegularExpression(prefix).match(name).hasMatch()) {
                filteredDefines.insert(name, value);
            }
        }
    }
    return filteredDefines;
}

QStringList ParseUtil::readCDefinesSorted(const QString &filename,
                                          const QStringList &prefixes,
                                          const QMap<QString, int> &knownDefines)
{
    QMap<QString, int> defines = readCDefines(filename, prefixes, knownDefines);

    // The defines should to be sorted by their underlying value, not alphabetically.
    // Reverse the map and read out the resulting keys in order.
    QMultiMap<int, QString> definesInverse;
    for (QString defineName : defines.keys()) {
        definesInverse.insert(defines[defineName], defineName);
    }
    return definesInverse.values();
}

QStringList ParseUtil::readCArray(const QString &filename, const QString &label) {
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

QMap<QString, QString> ParseUtil::readNamedIndexCArray(const QString &filename, const QString &label) {
    text = readTextFile(root + "/" + filename);
    QMap<QString, QString> map;

    QRegularExpression re_text(QString(R"(\b%1\b\s*(\[?[^\]]*\])?\s*=\s*\{([^\}]*)\})").arg(label));
    QString body = re_text.match(text).captured(2).replace(QRegularExpression("\\s*"), "");

    QRegularExpression re("\\[(?<index>[A-Za-z0-9_]*)\\]=(?<value>&?[A-Za-z0-9_]*)");
    QRegularExpressionMatchIterator iter = re.globalMatch(body);

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString key = match.captured("index");
        QString value = match.captured("value");
        map.insert(key, value);
    }

    return map;
}

QList<QStringList> ParseUtil::getLabelMacros(const QList<QStringList> &list, const QString &label) {
    bool in_label = false;
    QList<QStringList> new_list;
    for (const auto &params : list) {
        const QString macro = params.value(0);
        if (macro == ".label") {
            if (params.value(1) == label) {
                in_label = true;
            } else if (in_label) {
                // If nothing has been read yet, assume the label
                // we're looking for is in a stack of labels.
                if (new_list.length() > 0) {
                    break;
                }
            }
        } else if (in_label) {
            new_list.append(params);
        }
    }
    return new_list;
}

// For if you don't care about filtering by macro,
// and just want all values associated with some label.
QStringList ParseUtil::getLabelValues(const QList<QStringList> &list, const QString &label) {
    const QList<QStringList> labelMacros = getLabelMacros(list, label);
    QStringList values;
    for (const auto &params : labelMacros) {
        const QString macro = params.value(0);
        if (macro == ".align" || macro == ".ifdef" || macro == ".ifndef") {
            continue;
        }
        for (int i = 1; i < params.length(); i++) {
            values.append(params.value(i));
        }
    }
    return values;
}

bool ParseUtil::tryParseJsonFile(QJsonDocument *out, const QString &filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Error: Could not open %1 for reading").arg(filepath));
        return false;
    }

    const QByteArray data = file.readAll();
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError) {
        logError(QString("Error: Failed to parse json file %1: %2").arg(filepath).arg(parseError.errorString()));
        return false;
    }

    *out = jsonDoc;
    return true;
}

bool ParseUtil::tryParseOrderedJsonFile(poryjson::Json::object *out, const QString &filepath) {
    QString err;
    QString jsonTxt = readTextFile(filepath);
    *out = OrderedJson::parse(jsonTxt, err).object_items();
    if (!err.isEmpty()) {
        logError(QString("Error: Failed to parse json file %1: %2").arg(filepath).arg(err));
        return false;
    }
    return true;
}

bool ParseUtil::ensureFieldsExist(const QJsonObject &obj, const QList<QString> &fields) {
    for (QString field : fields) {
        if (!obj.contains(field)) {
            logError(QString("JSON object is missing field '%1'.").arg(field));
            return false;
        }
    }
    return true;
}

int ParseUtil::getScriptLineNumber(const QString &filePath, const QString &scriptLabel) {
    if (scriptLabel.isEmpty())
        return 0;

    if (filePath.endsWith(".inc") || filePath.endsWith(".s"))
        return getRawScriptLineNumber(readTextFile(filePath), scriptLabel);
    else if (filePath.endsWith(".pory"))
        return getPoryScriptLineNumber(readTextFile(filePath), scriptLabel);

    return 0;
}

int ParseUtil::getRawScriptLineNumber(QString text, const QString &scriptLabel) {
    text = removeStringLiterals(text);
    text = removeLineComments(text, "@");

    QRegularExpressionMatchIterator it = re_incScriptLabel.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        if (match.captured("label") == scriptLabel)
            return text.left(match.capturedStart("label")).count('\n') + 1;
    }

    return 0;
}

int ParseUtil::getPoryScriptLineNumber(QString text, const QString &scriptLabel) {
    text = removeStringLiterals(text);
    text = removeLineComments(text, {"//", "#"});

    QRegularExpressionMatchIterator it = re_poryScriptLabel.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        if (match.captured("label") == scriptLabel)
            return text.left(match.capturedStart("label")).count('\n') + 1;
    }

    QRegularExpressionMatchIterator raw_it = re_poryRawSection.globalMatch(text);
    while (raw_it.hasNext()) {
        const QRegularExpressionMatch match = raw_it.next();
        const int relativelineNum = getRawScriptLineNumber(match.captured("raw_script"), scriptLabel);
        if (relativelineNum)
            return text.left(match.capturedStart("raw_script")).count('\n') + relativelineNum;
    }

    return 0;
}

QStringList ParseUtil::getGlobalScriptLabels(const QString &filePath) {
    if (filePath.endsWith(".inc") || filePath.endsWith(".s"))
        return getGlobalRawScriptLabels(readTextFile(filePath));
    else if (filePath.endsWith(".pory"))
        return getGlobalPoryScriptLabels(readTextFile(filePath));
    else
        return { };
}

QStringList ParseUtil::getGlobalRawScriptLabels(QString text) {
    removeStringLiterals(text);
    removeLineComments(text, "@");

    QStringList rawScriptLabels;

    QRegularExpressionMatchIterator it = re_globalIncScriptLabel.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        rawScriptLabels << match.captured("label");
    }

    return rawScriptLabels;
}

QStringList ParseUtil::getGlobalPoryScriptLabels(QString text) {
    removeStringLiterals(text);
    removeLineComments(text, {"//", "#"});

    QStringList poryScriptLabels;

    QRegularExpressionMatchIterator it = re_globalPoryScriptLabel.globalMatch(text);
    while (it.hasNext())
        poryScriptLabels << it.next().captured("label");

    QRegularExpressionMatchIterator raw_it = re_poryRawSection.globalMatch(text);
    while (raw_it.hasNext())
        poryScriptLabels << getGlobalRawScriptLabels(raw_it.next().captured("raw_script"));

    return poryScriptLabels;
}

QString ParseUtil::removeStringLiterals(QString text) {
    static const QRegularExpression re_string("\".*\"");
    return text.remove(re_string);
}

QString ParseUtil::removeLineComments(QString text, const QString &commentSymbol) {
    const QRegularExpression re_lineComment(commentSymbol + "+.*");
    return text.remove(re_lineComment);
}

QString ParseUtil::removeLineComments(QString text, const QStringList &commentSymbols) {
    for (const auto &commentSymbol : commentSymbols)
        text = removeLineComments(text, commentSymbol);
    return text;
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))

#include <QProcess>

QStringList ParseUtil::splitShellCommand(QStringView command) {
    return QProcess::splitCommand(command);
}

#else

// The source for QProcess::splitCommand() as of Qt 5.15.2
QStringList ParseUtil::splitShellCommand(QStringView command) {
    QStringList args;
    QString tmp;
    int quoteCount = 0;
    bool inQuote = false;

    // handle quoting. tokens can be surrounded by double quotes
    // "hello world". three consecutive double quotes represent
    // the quote character itself.
    for (int i = 0; i < command.size(); ++i) {
        if (command.at(i) == QLatin1Char('"')) {
            ++quoteCount;
            if (quoteCount == 3) {
                // third consecutive quote
                quoteCount = 0;
                tmp += command.at(i);
            }
            continue;
        }
        if (quoteCount) {
            if (quoteCount == 1)
                inQuote = !inQuote;
            quoteCount = 0;
        }
        if (!inQuote && command.at(i).isSpace()) {
            if (!tmp.isEmpty()) {
                args += tmp;
                tmp.clear();
            }
        } else {
            tmp += command.at(i);
        }
    }
    if (!tmp.isEmpty())
        args += tmp;

    return args;
}

#endif
