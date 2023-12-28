#include "log.h"
#include "parseutil.h"

#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStack>

#include "lib/fex/lexer.h"
#include "lib/fex/parser.h"

const QRegularExpression ParseUtil::re_incScriptLabel("\\b(?<label>[\\w_][\\w\\d_]*):{1,2}");
const QRegularExpression ParseUtil::re_globalIncScriptLabel("\\b(?<label>[\\w_][\\w\\d_]*)::");
const QRegularExpression ParseUtil::re_poryScriptLabel("\\b(script)(\\((global|local)\\))?\\s*\\b(?<label>[\\w_][\\w\\d_]*)");
const QRegularExpression ParseUtil::re_globalPoryScriptLabel("\\b(script)(\\((global)\\))?\\s*\\b(?<label>[\\w_][\\w\\d_]*)");
const QRegularExpression ParseUtil::re_poryRawSection("\\b(raw)\\s*`(?<raw_script>[^`]*)");

using OrderedJson = poryjson::Json;

ParseUtil::ParseUtil() { }

void ParseUtil::set_root(const QString &dir) {
    this->root = dir;
}

void ParseUtil::recordError(const QString &message) {
    this->errorMap[this->curDefine].append(message);
}

void ParseUtil::recordErrors(const QStringList &errors) {
    if (errors.isEmpty()) return;
    this->errorMap[this->curDefine].append(errors);
}

void ParseUtil::logRecordedErrors() {
    QStringList errors = this->errorMap.value(this->curDefine);
    if (errors.isEmpty()) return;
    QString message = QString("Failed to parse '%1':").arg(this->curDefine);
    for (const auto error : errors)
        message.append(QString("\n%1").arg(error));
    logError(message);
}

QString ParseUtil::createErrorMessage(const QString &message, const QString &expression) {
    static const QRegularExpression newline("[\r\n]");
    QStringList lines = this->text.split(newline);
    int lineNum = 0, colNum = 0;
    for (QString line : lines) {
        lineNum++;
        colNum = line.indexOf(expression) + 1;
        if (colNum) break;
    }
    return QString("%1:%2:%3: %4").arg(this->file).arg(lineNum).arg(colNum).arg(message);
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

    this->text = readTextFile(this->root + '/' + filename);
    const QStringList lines = removeLineComments(this->text, "@").split('\n');
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
            static const QRegularExpression re_spaces("\\s+");
            int index = trimmedLine.indexOf(re_spaces);
            const QString macro = trimmedLine.left(index);
            static const QRegularExpression re_spacesCommaSpaces("\\s*,\\s*");
            QStringList params(trimmedLine.right(trimmedLine.length() - index).trimmed().split(re_spacesCommaSpaces));
            params.prepend(macro);
            parsed.append(params);
        }
    }
    return parsed;
}

// 'identifier' is the name of the #define to evaluate, e.g. 'FOO' in '#define FOO (BAR+1)'
// 'expression' is the text of the #define to evaluate, e.g. '(BAR+1)' in '#define FOO (BAR+1)'
// 'knownValues' is a pointer to a map of identifier->values for defines that have already been evaluated.
// 'unevaluatedExpressions' is a pointer to a map of identifier->expressions for defines that have not been evaluated. If this map contains any
//   identifiers found in 'expression' then this function will be called recursively to evaluate that define first.
// This function will maintain the passed maps appropriately as new #defines are evaluated.
int ParseUtil::evaluateDefine(const QString &identifier, const QString &expression, QMap<QString, int> *knownValues, QMap<QString, QString> *unevaluatedExpressions) {
    if (unevaluatedExpressions->contains(identifier))
        unevaluatedExpressions->remove(identifier);

    if (knownValues->contains(identifier))
        return knownValues->value(identifier);

    QList<Token> tokens = tokenizeExpression(expression, knownValues, unevaluatedExpressions);
    QList<Token> postfixExpression = generatePostfix(tokens);
    int value = evaluatePostfix(postfixExpression);

    knownValues->insert(identifier, value);
    return value;
}

QList<Token> ParseUtil::tokenizeExpression(QString expression, QMap<QString, int> *knownValues, QMap<QString, QString> *unevaluatedExpressions) {
    QList<Token> tokens;

    static const QStringList tokenTypes = {"hex", "decimal", "identifier", "operator", "leftparen", "rightparen"};
    static const QRegularExpression re("^(?<hex>0x[0-9a-fA-F]+)|(?<decimal>[0-9]+)|(?<identifier>[a-zA-Z_0-9]+)|(?<operator>[+\\-*\\/<>|^%]+)|(?<leftparen>\\()|(?<rightparen>\\))");

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
                    if (unevaluatedExpressions->contains(token)) {
                        // This expression depends on a define we know of but haven't evaluated. Evaluate it now
                        evaluateDefine(token, unevaluatedExpressions->value(token), knownValues, unevaluatedExpressions);
                    }
                    if (knownValues->contains(token)) {
                        // Any errors encountered when this identifier was evaluated should be recorded for this expression as well.
                        recordErrors(this->errorMap.value(token));
                        QString actualToken = QString("%1").arg(knownValues->value(token));
                        expression = expression.replace(0, token.length(), actualToken);
                        token = actualToken;
                        tokenType = "decimal";
                    } else {
                        tokenType = "error";
                        QString message = QString("unknown token '%1' found in expression '%2'")
                                          .arg(token).arg(expression);
                        recordError(createErrorMessage(message, expression));
                    }
                }
                else if (tokenType == "operator") {
                    if (!Token::precedenceMap.contains(token)) {
                        QString message = QString("unsupported postfix operator: '%1'")
                                          .arg(token);
                        recordError(createErrorMessage(message, expression));
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
                recordError("Mismatched parentheses detected in expression!");
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
            recordError("Mismatched parentheses detected in expression!");
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

    this->text = readTextFile(this->root + "/" + filename);

    QRegularExpression re(QString(
        "\\b%1\\b"
        "\\s*\\[?\\s*\\]?\\s*=\\s*"
        "INCBIN_[US][0-9][0-9]?"
        "\\(\\s*\"([^\"]*)\"\\s*\\)").arg(label));

    QRegularExpressionMatch match;
    qsizetype pos = this->text.indexOf(re, 0, &match);
    if (pos != -1) {
        path = match.captured(1);
    }

    return path;
}

QMap<QString, QString> ParseUtil::readCIncbinMulti(const QString &filepath) {
    QMap<QString, QString> incbinMap;

    this->file = filepath;
    this->text = readTextFile(this->root + "/" + filepath);

    static const QRegularExpression regex("(?<label>[A-Za-z0-9_]+)\\s*\\[?\\s*\\]?\\s*=\\s*INCBIN_[US][0-9][0-9]?\\(\\s*\\\"(?<path>[^\\\\\"]*)\\\"\\s*\\)");

    QRegularExpressionMatchIterator iter = regex.globalMatch(this->text);
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString label = match.captured("label");
        QString labelText = match.captured("path");
        incbinMap[label] = labelText;
    }

    return incbinMap;
}

QStringList ParseUtil::readCIncbinArray(const QString &filename, const QString &label) {
    QStringList paths;

    if (label.isNull()) {
        return paths;
    }

    this->text = readTextFile(this->root + "/" + filename);

    bool found = false;
    QString arrayText;

    // Get the text starting after the label all the way to the definition's end
    static const QRegularExpression re_labelGroup(QString("(?<label>[A-Za-z0-9_]+)\\[([^;]*?)};"), QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator findLabelIter = re_labelGroup.globalMatch(this->text);
    while (findLabelIter.hasNext()) {
        QRegularExpressionMatch labelMatch = findLabelIter.next();
        if (labelMatch.captured("label") == label) {
            found = true;
            arrayText = labelMatch.captured(2);
            break;
        }
    }

    if (!found) {
        return paths;
    }

    // Extract incbin paths from the array
    static const QRegularExpression re_incbin("INCBIN_[US][0-9][0-9]?\\(\\s*\"([^\"]*)\"\\s*\\)");
    QRegularExpressionMatchIterator iter = re_incbin.globalMatch(arrayText);
    while (iter.hasNext()) {
        paths.append(iter.next().captured(1));
    }
    return paths;
}

QString ParseUtil::readCDefinesFile(const QString &filename)
{
    this->file = filename;

    if (this->file.isEmpty()) {
        return QString();
    }

    QString filepath = this->root + "/" + this->file;
    this->text = readTextFile(filepath);

    if (this->text.isNull()) {
        logError(QString("Failed to read C defines file: '%1'").arg(filepath));
        return QString();
    }

    static const QRegularExpression re_extraChars("(//.*)|(\\/+\\*+[^*]*\\*+\\/+)");
    this->text.replace(re_extraChars, "");
    static const QRegularExpression re_extraSpaces("(\\\\\\s+)");
    this->text.replace(re_extraSpaces, "");
    return this->text;
}

// Read all the define names and their expressions in the specified file, then evaluate the ones matching the search text (and any they depend on).
// If 'fullMatch' is true, 'searchText' is a list of exact define names to evaluate and return.
// If 'fullMatch' is false, 'searchText' is a list of prefixes or regexes for define names to evaluate and return.
QMap<QString, int> ParseUtil::readCDefines(const QString &filename, const QStringList &searchText, bool fullMatch)
{
    QMap<QString, int> filteredValues;

    this->text = this->readCDefinesFile(filename);
    if (this->text.isEmpty()) {
        return filteredValues;
    }

    // Extract all the define names and expressions
    QMap<QString, QString> allExpressions;
    QMap<QString, QString> filteredExpressions;
    static const QRegularExpression re("#define\\s+(?<defineName>\\w+)[^\\S\\n]+(?<defineValue>.+)");
    QRegularExpressionMatchIterator iter = re.globalMatch(this->text);
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        const QString name = match.captured("defineName");
        const QString expression = match.captured("defineValue");
        // If name matches the search text record it for evaluation.
        for (auto s : searchText) {
            if ((fullMatch && name == s) || (!fullMatch && (name.startsWith(s) || QRegularExpression(s).match(name).hasMatch()))) {
                filteredExpressions.insert(name, expression);
                break;
            }
        }
        allExpressions.insert(name, expression);
    }

    QMap<QString, int> allValues;
    allValues.insert("FALSE", 0);
    allValues.insert("TRUE", 1);

    // Evaluate defines
    this->errorMap.clear();
    while (!filteredExpressions.isEmpty()) {
        const QString name = filteredExpressions.firstKey();
        const QString expression = filteredExpressions.take(name);
        if (expression == " ") continue;
        this->curDefine = name;
        filteredValues.insert(name, evaluateDefine(name, expression, &allValues, &allExpressions));
        logRecordedErrors(); // Only log errors for defines that Porymap is looking for
    }
    return filteredValues;
}

// Find and evaluate an unknown list of defines with a known name prefix.
QMap<QString, int> ParseUtil::readCDefinesByPrefix(const QString &filename, QStringList prefixes) {
    prefixes.removeDuplicates();
    return this->readCDefines(filename, prefixes, false);
}

// Find and evaluate a specific set of defines with known names.
QMap<QString, int> ParseUtil::readCDefinesByName(const QString &filename, QStringList names) {
    names.removeDuplicates();
    return this->readCDefines(filename, names, true);
}

// Similar to readCDefines, but for cases where we only need to show a list of define names.
// We can skip reading/evaluating any expressions (and by extension skip reporting any errors from this process).
QStringList ParseUtil::readCDefineNames(const QString &filename, const QStringList &prefixes) {
    QStringList filteredNames;

    this->text = this->readCDefinesFile(filename);
    if (this->text.isEmpty()) {
        return filteredNames;
    }

    static const QRegularExpression re("#define\\s+(?<defineName>\\w+)[^\\S\\n]+");
    QRegularExpressionMatchIterator iter = re.globalMatch(this->text);
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString name = match.captured("defineName");
        for (QString prefix : prefixes) {
            if (name.startsWith(prefix) || QRegularExpression(prefix).match(name).hasMatch()) {
                filteredNames.append(name);
            }
        }
    }
    return filteredNames;
}

QStringList ParseUtil::readCArray(const QString &filename, const QString &label) {
    QStringList list;

    if (label.isNull()) {
        return list;
    }

    this->file = filename;
    this->text = readTextFile(this->root + "/" + filename);

    QRegularExpression re(QString(R"(\b%1\b\s*(\[?[^\]]*\])?\s*=\s*\{([^\}]*)\})").arg(label));
    QRegularExpressionMatch match = re.match(this->text);

    if (match.hasMatch()) {
        QString body = match.captured(2);
        QStringList split = body.split(',');
        for (QString item : split) {
            item = item.trimmed();
            static const QRegularExpression validChars("[^A-Za-z0-9_&()\\s]");
            if (!item.contains(validChars)) list.append(item);
            // do not print error info here because this is called dozens of times
        }
    }

    return list;
}

QMap<QString, QStringList> ParseUtil::readCArrayMulti(const QString &filename) {
    QMap<QString, QStringList> map;

    this->file = filename;
    this->text = readTextFile(this->root + "/" + filename);

    static const QRegularExpression regex(R"((?<label>\b[A-Za-z0-9_]+\b)\s*(\[[^\]]*\])?\s*=\s*\{(?<body>[^\}]*)\})");

    QRegularExpressionMatchIterator iter = regex.globalMatch(this->text);

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString label = match.captured("label");
        QString body = match.captured("body");

        QStringList list;
        QStringList split = body.split(',');
        for (QString item : split) {
            item = item.trimmed();
            static const QRegularExpression validChars("[^A-Za-z0-9_&()\\s]");
            if (!item.contains(validChars)) list.append(item);
            // do not print error info here because this is called dozens of times
        }
        map[label] = list;
    }

    return map;
}

QMap<QString, QString> ParseUtil::readNamedIndexCArray(const QString &filename, const QString &label) {
    this->text = readTextFile(this->root + "/" + filename);
    QMap<QString, QString> map;

    QRegularExpression re_text(QString(R"(\b%1\b\s*(\[?[^\]]*\])?\s*=\s*\{([^\}]*)\})").arg(label));
    QString arrayText = re_text.match(this->text).captured(2).replace(QRegularExpression("\\s*"), "");

    static const QRegularExpression re_findRow("\\[(?<index>[A-Za-z0-9_]*)\\][\\s=]+(?<value>&?[A-Za-z0-9_]*)");
    QRegularExpressionMatchIterator rowIter = re_findRow.globalMatch(arrayText);

    while (rowIter.hasNext()) {
        QRegularExpressionMatch match = rowIter.next();
        QString key = match.captured("index");
        QString value = match.captured("value");
        map.insert(key, value);
    }

    return map;
}

int ParseUtil::gameStringToInt(QString gameString, bool * ok) {
    if (ok) *ok = true;
    if (QString::compare(gameString, "TRUE", Qt::CaseInsensitive) == 0)
        return 1;
    if (QString::compare(gameString, "FALSE", Qt::CaseInsensitive) == 0)
        return 0;
    return gameString.toInt(ok, 0);
}

bool ParseUtil::gameStringToBool(QString gameString, bool * ok) {
    return gameStringToInt(gameString, ok) != 0;
}

QMap<QString, QHash<QString, QString>> ParseUtil::readCStructs(const QString &filename, const QString &label, const QHash<int, QString> memberMap) {
    QString filePath = this->root + "/" + filename;
    auto cParser = fex::Parser();
    auto tokens = fex::Lexer().LexFile(filePath.toStdString());
    auto structs = cParser.ParseTopLevelObjects(tokens);
    QMap<QString, QHash<QString, QString>> structMaps;
    for (auto it = structs.begin(); it != structs.end(); it++) {
        QString structLabel = QString::fromStdString(it->first);
        if (structLabel.isEmpty()) continue;
        if (!label.isEmpty() && label != structLabel) continue; // Speed up parsing if only looking for a particular symbol
        QHash<QString, QString> values;
        int i = 0;
        for (const fex::ArrayValue &v : it->second.values()) {
            if (v.type() == fex::ArrayValue::Type::kValuePair) {
                QString key = QString::fromStdString(v.pair().first);
                QString value = QString::fromStdString(v.pair().second->string_value());
                values.insert(key, value);
            } else {
                // For compatibility with structs that don't specify member names.
                if (memberMap.contains(i))
                    values.insert(memberMap.value(i), QString::fromStdString(v.string_value()));
            }
            i++;
        }
        structMaps.insert(structLabel, values);
    }
    return structMaps;
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

// QJsonValues are strictly typed, and so will not attempt any implicit conversions.
// The below functions are for attempting to convert a JSON value read from the user's
// project to a QString, int, or bool (whichever Porymap expects).
QString ParseUtil::jsonToQString(QJsonValue value, bool * ok) {
    if (ok) *ok = true;
    switch (value.type())
    {
    case QJsonValue::String: return value.toString();
    case QJsonValue::Double: return QString::number(value.toInt());
    case QJsonValue::Bool:   return QString::number(value.toBool());
    default:                 break;
    }
    if (ok) *ok = false;
    return QString();
}

int ParseUtil::jsonToInt(QJsonValue value, bool * ok) {
    if (ok) *ok = true;
    switch (value.type())
    {
    case QJsonValue::String: return ParseUtil::gameStringToInt(value.toString(), ok);
    case QJsonValue::Double: return value.toInt();
    case QJsonValue::Bool:   return value.toBool();
    default:                 break;
    }
    if (ok) *ok = false;
    return 0;
}

bool ParseUtil::jsonToBool(QJsonValue value, bool * ok) {
    if (ok) *ok = true;
    switch (value.type())
    {
    case QJsonValue::String: return ParseUtil::gameStringToBool(value.toString(), ok);
    case QJsonValue::Double: return value.toInt() != 0;
    case QJsonValue::Bool:   return value.toBool();
    default:                 break;
    }
    if (ok) *ok = false;
    return false;
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
