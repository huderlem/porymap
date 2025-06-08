#include "log.h"
#include "parseutil.h"
#include "loadingscreen.h"
#include "utility.h"

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

ParseUtil::ParseUtil() {
    resetCDefines();
}

QString ParseUtil::pathWithRoot(const QString &path) {
    if (this->root.isEmpty()) return path;
    if (path.startsWith(this->root)) return path;
    return QString("%1/%2").arg(this->root).arg(path);
}

void ParseUtil::recordError(const QString &message) {
    this->errorMap[this->curDefine].append(message);
}

void ParseUtil::recordErrors(const QStringList &errors) {
    if (errors.isEmpty()) return;
    this->errorMap[this->curDefine].append(errors);
}

void ParseUtil::logRecordedErrors() {
    const QStringList errors = this->errorMap.value(this->curDefine);
    if (errors.isEmpty()) return;
    QString message = QString("Failed to parse '%1':").arg(this->curDefine);
    for (const auto &error : errors)
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

void ParseUtil::updateSplashScreen(QString path) {
    if (!this->updatesSplashScreen)
        return;
    porysplash->showLoadingMessage(Util::stripPrefix(path, this->root));
}

QString ParseUtil::readTextFile(const QString &path, QString *error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
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

// Load the specified text file, either from the cache or by reading the file.
// Note that this doesn't insert any parsed files into the file cache, and we don't
// want it to (we read a lot of files only once, storing them all is a waste of memory).
QString ParseUtil::loadTextFile(const QString &path, QString *error) {
    updateSplashScreen(path);

    auto it = this->fileCache.constFind(path);
    if (it != this->fileCache.constEnd()) {
        // Load text file from cache
        return it.value();
    }
    return readTextFile(pathWithRoot(path), error);
}

bool ParseUtil::cacheFile(const QString &path, QString *error) {
    updateSplashScreen(path);

    // We use an internal '_error' variable because we use the error output to track success,
    // and we want to report success regardless of whether the caller provided 'error'.
    QString _error;
    this->fileCache.insert(path, readTextFile(pathWithRoot(path), &_error));
    if (error) *error = _error;
    return _error.isEmpty();
}

int ParseUtil::textFileLineCount(const QString &path) {
    const QString text = readTextFile(path);
    return text.split('\n').count() + 1;
}

QList<QStringList> ParseUtil::parseAsm(const QString &filename) {
    QList<QStringList> parsed;

    this->text = loadTextFile(filename);
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

// Try to evaluate the given #define/enum 'identifier' name using the information the parser has.
// If it recognizes the name as an identifier it's aware of (either from having parsed it or having been told about
// it using 'loadGlobalCDefines') it will evaluate it if necessary then return the resulting value and set 'ok' to true.
// Evaluated identifiers are cached, and will only be re-evaluated if the parser encounters a new expression for that identifier.
// If it doesn't recognize it, 'ok' will be set to false and it will return 0.
int ParseUtil::evaluateDefine(const QString &identifier, bool *ok) {
    if (ok) *ok = true;

    // Global defines take precedence
    if (this->globalDefineExpressions.contains(identifier)) {
        int value = evaluateExpression(this->globalDefineExpressions.take(identifier));
        this->globalDefineValues.insert(identifier, value);
        return value;
    }
    auto it = this->globalDefineValues.constFind(identifier);
    if (it != this->globalDefineValues.constEnd()) {
        return it.value();
    }

    // Check known expressions before checking known values.
    // If an identifier is redefined then we'll receive a new expression for it, and we want to make sure we re-evaluate it.
    if (this->knownDefineExpressions.contains(identifier)) {
        int value = evaluateExpression(this->knownDefineExpressions.take(identifier));
        this->knownDefineValues.insert(identifier, value);
        return value;
    }
    it = this->knownDefineValues.constFind(identifier);
    if (it != this->knownDefineValues.constEnd()) {
        return it.value();
    }

    if (ok) *ok = false;
    return 0;
}

int ParseUtil::evaluateExpression(const QString &expression) {
    QList<Token> tokens = tokenizeExpression(expression);
    QList<Token> postfixExpression = generatePostfix(tokens);
    return evaluatePostfix(postfixExpression);
}

QList<Token> ParseUtil::tokenizeExpression(QString expression) {
    QList<Token> tokens;

    static const QStringList tokenTypes = {"hex", "decimal", "identifier", "operator", "leftparen", "rightparen"};
    static const QRegularExpression re("^(?<hex>0[xX][0-9a-fA-F]+)|(?<decimal>\\d+)|(?<identifier>\\w+)|(?<operator>[+\\-*\\/<>|^%]+)|(?<leftparen>\\()|(?<rightparen>\\))");

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
                    bool ok;
                    int tokenValue = evaluateDefine(token, &ok);
                    if (ok) {
                        // Any errors encountered when this identifier was evaluated should be recorded for this expression as well.
                        recordErrors(this->errorMap.value(token));

                        // Replace token with evaluated expression
                        QString actualToken = QString::number(tokenValue);
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
        Token token = operatorStack.pop();
        if (token.value == "(" || token.value == ")") {
            recordError("Mismatched parentheses detected in expression!");
        } else {
            output.append(token);
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

    this->text = loadTextFile(filename);

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
    this->text = loadTextFile(filepath);

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

    this->text = loadTextFile(filename);

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

bool ParseUtil::defineNameMatchesFilter(const QString &name, const QSet<QString> &filterList) const {
    return filterList.contains(name);
}

bool ParseUtil::defineNameMatchesFilter(const QString &name, const QSet<QRegularExpression> &filterList) const {
    for (auto filter : filterList) {
        if (filter.match(name).hasMatch())
            return true;
    }
    return false;
}

ParseUtil::ParsedDefines ParseUtil::readCDefines(const QString &filename, const QSet<QString> &filterList, bool useRegex, QString *error) {
    ParsedDefines result;
    this->file = filename;

    if (this->file.isEmpty()) {
        return result;
    }

    this->text = loadTextFile(filename, error);
    if (this->text.isNull())
        return result;

    static const QRegularExpression re_extraChars("(//.*)|(\\/+\\*+[^*]*\\*+\\/+)");
    this->text.replace(re_extraChars, "");
    static const QRegularExpression re_extraSpaces("(\\\\\\s+)");
    this->text.replace(re_extraSpaces, "");

    if (this->text.isEmpty())
        return result;

    // If necessary, construct regular expressions from filter list
    QSet<QRegularExpression> filterList_Regex;
    if (useRegex) {
        for (auto filter : filterList) {
            filterList_Regex.insert(QRegularExpression(filter));
        }
    }

    // Create lambda function to match the define name to the filter, depending on the filter type
    auto matchesFilter = [this, &filterList, &filterList_Regex, useRegex](const QString &name) {
        if (useRegex)
            return defineNameMatchesFilter(name, filterList_Regex);
        return defineNameMatchesFilter(name, filterList);
    };

    // Capture either the name and value of a #define, or everything between the braces of 'enum { }'
    static const QRegularExpression re("#define\\s+(?<defineName>\\w+)[\\s\\n][^\\S\\n]*(?<defineValue>.+)?"
                                       "|\\benum\\b[^{]*{(?<enumBody>[^}]*)}");

    QRegularExpressionMatchIterator iter = re.globalMatch(this->text);
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        const QString enumBody = match.captured("enumBody");
        if (!enumBody.isNull()) {
            // Encountered an enum, extract the elements of the enum and give each an appropriate expression
            int baseNum = 0;
            QString baseExpression = "0";

            // Note: We lazily consider an enum's expression to be any characters after the assignment up until the first comma or EOL.
            // This would be a problem for e.g. NAME = MACRO(a, b), but we're currently unable to parse function-like macros anyway.
            // If this changes then the regex below needs to be updated.
            static const QRegularExpression re_enumElement("\\b(?<name>\\w+)\\b\\s*=?\\s*(?<expression>[^,]*)");
            QRegularExpressionMatchIterator elementIter = re_enumElement.globalMatch(enumBody);
            while (elementIter.hasNext()) {
                QRegularExpressionMatch elementMatch = elementIter.next();
                const QString name = elementMatch.captured("name");
                QString expression = elementMatch.captured("expression");
                if (expression.isEmpty()) {
                    // enum values may use tokens that we don't know how to evaluate yet.
                    // For now we define each element to be 1 + the previous element's expression.
                    expression = QString("((%1)+%2)").arg(baseExpression).arg(baseNum++);
                } else {
                    // This element was explicitly assigned an expression with '=', reset the bases for any subsequent elements.
                    baseExpression = expression;
                    baseNum = 1;
                }
                result.expressions.insert(name, expression);
                if (matchesFilter(name))
                    result.filteredNames.append(name);
            }
        } else {
            // Encountered a #define
            const QString name = match.captured("defineName");
            result.expressions.insert(name, match.captured("defineValue"));
            if (matchesFilter(name))
                result.filteredNames.append(name);
        }
    }
    // QHash::insert(const QHash<K, V> &other) was introduced in 5.15.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    this->knownDefineExpressions.insert(result.expressions);
#else
    for (auto it = result.expressions.constBegin(); it != result.expressions.constEnd(); it++) {
        this->knownDefineExpressions.insert(it.key(), it.value());
    }
#endif
    return result;
}

// Read all the define names and their expressions in the specified file, then evaluate the ones matching the search text (and any they depend on).
QHash<QString, int> ParseUtil::evaluateCDefines(const QString &filename, const QSet<QString> &filterList, bool useRegex, QString *error) {
    ParsedDefines defines = readCDefines(filename, filterList, useRegex, error);

    // Evaluate defines
    QHash<QString, int> filteredValues;
    this->errorMap.clear();
    while (!defines.filteredNames.isEmpty()) {
        this->curDefine = defines.filteredNames.takeFirst();
        filteredValues.insert(this->curDefine, evaluateDefine(this->curDefine));
        logRecordedErrors(); // Only log errors for defines that Porymap is looking for
    }

    return filteredValues;
}

// Find and evaluate a specific set of defines with known names.
QHash<QString, int> ParseUtil::readCDefinesByName(const QString &filename, const QSet<QString> &names, QString *error) {
    return evaluateCDefines(filename, names, false, error);
}

// Find and evaluate an unknown list of defines with a known name pattern.
QHash<QString, int> ParseUtil::readCDefinesByRegex(const QString &filename, const QSet<QString> &regexList, QString *error) {
    return evaluateCDefines(filename, regexList, true, error);
}

// Find an unknown list of defines with a known name pattern.
// Similar to readCDefinesByRegex, but for cases where we only need to show a list of define names.
// We can skip evaluating any expressions (and by extension skip reporting any errors from this process).
QStringList ParseUtil::readCDefineNames(const QString &filename, const QSet<QString> &regexList, QString *error) {
    return readCDefines(filename, regexList, true, error).filteredNames;
}

// Find any defines in the specified file and save their expressions.
// If any of these defines are encountered later by other define parsing functions then they'll be recognized and evaluated.
void ParseUtil::loadGlobalCDefinesFromFile(const QString &filename, QString *error) {
    loadGlobalCDefines(readCDefines(filename, {}, false, error).expressions);
}

void ParseUtil::loadGlobalCDefines(const QHash<QString,QString> &defines) {
    // QHash::insert(const QHash<K, V> &other) was introduced in 5.15.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    this->globalDefineExpressions.insert(defines);
#else
    for (auto it = defines.constBegin(); it != defines.constEnd(); it++) {
        this->globalDefineExpressions.insert(it.key(), it.value());
    }
#endif
}

void ParseUtil::loadGlobalCDefines(const QMap<QString,QString> &defines) {
    for (auto it = defines.constBegin(); it != defines.constEnd(); it++)
        this->globalDefineExpressions.insert(it.key(), it.value());
}

void ParseUtil::resetCDefines() {
    static const QHash<QString, int> defaultDefineValues = {
        {"FALSE", 0},
        {"TRUE", 1},
        {"SCHAR_MIN", SCHAR_MIN},
        {"SCHAR_MAX", SCHAR_MAX},
        {"CHAR_MIN", CHAR_MIN},
        {"CHAR_MAX", CHAR_MAX},
        {"UCHAR_MAX", UCHAR_MAX},
        {"SHRT_MIN", SHRT_MIN},
        {"SHRT_MAX", SHRT_MAX},
        {"USHRT_MAX", USHRT_MAX},
        {"INT_MIN", INT_MIN},
        {"INT_MAX", INT_MAX},
        {"UINT_MAX", UINT_MAX},
    };
    this->globalDefineValues = defaultDefineValues;
    this->globalDefineExpressions.clear();
    this->knownDefineValues.clear();
    this->knownDefineExpressions.clear();
}

QStringList ParseUtil::readCArray(const QString &filename, const QString &label) {
    QStringList list;

    if (label.isNull()) {
        return list;
    }

    this->file = filename;
    this->text = loadTextFile(filename);

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
    this->text = loadTextFile(filename);

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

QMap<QString, QString> ParseUtil::readNamedIndexCArray(const QString &filename, const QString &label, QString *error) {
    this->text = loadTextFile(filename, error);
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

int ParseUtil::gameStringToInt(const QString &gameString, bool * ok) {
    if (ok) *ok = true;
    if (QString::compare(gameString, "TRUE", Qt::CaseInsensitive) == 0)
        return 1;
    if (QString::compare(gameString, "FALSE", Qt::CaseInsensitive) == 0)
        return 0;
    return gameString.toInt(ok, 0);
}

bool ParseUtil::gameStringToBool(const QString &gameString, bool * ok) {
    return gameStringToInt(gameString, ok) != 0;
}

OrderedMap<QString, QHash<QString, QString>> ParseUtil::readCStructs(const QString &filename, const QString &label, const QHash<int, QString> &memberMap) {
    QString filePath = pathWithRoot(filename);
    auto cParser = fex::Parser();
    auto tokens = fex::Lexer().LexFile(filePath);
    auto topLevelObjects = cParser.ParseTopLevelObjects(tokens);
    OrderedMap<QString, QHash<QString, QString>> structs;
    for (auto it = topLevelObjects.begin(); it != topLevelObjects.end(); it++) {
        QString structLabel = QString::fromStdString(it->first);
        if (structLabel.isEmpty()) continue;
        if (!label.isEmpty() && label != structLabel) continue; // Speed up parsing if only looking for a particular symbol
        QHash<QString, QString> values;
        int i = 0;
        for (const fex::ArrayValue &v : it->second.values()) {
            if (v.type() == fex::ArrayValue::Type::kValuePair) {
                QString key = QString::fromStdString(v.pair().first);
                QString value = QString::fromStdString(v.pair().second->ToString());
                values.insert(key, value);
            } else {
                // For compatibility with structs that don't specify member names.
                if (memberMap.contains(i) && !values.contains(memberMap.value(i)))
                    values.insert(memberMap.value(i), QString::fromStdString(v.ToString()));
            }
            i++;
        }
        structs[structLabel] = values;
    }
    return structs;
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

bool ParseUtil::tryParseJsonFile(QJsonDocument *out, const QString &filepath, QString *error) {
    updateSplashScreen(filepath);

    QFile file(pathWithRoot(filepath));
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }

    const QByteArray data = file.readAll();
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError) {
        if (error) *error = parseError.errorString();
        return false;
    }

    *out = jsonDoc;
    return true;
}

bool ParseUtil::tryParseOrderedJsonFile(poryjson::Json::object *out, const QString &filepath, QString *error) {
    QString jsonTxt = loadTextFile(filepath, error);
    if (error && !error->isEmpty()) {
        return false;
    }
    *out = OrderedJson::parse(jsonTxt, error).object_items();
    if (error && !error->isEmpty()) {
        return false;
    }
    return true;
}

// QJsonValues are strictly typed, and so will not attempt any implicit conversions.
// The below functions are for attempting to convert a JSON value read from the user's
// project to a QString, int, or bool (whichever Porymap expects).
QString ParseUtil::jsonToQString(const QJsonValue &value, bool * ok) {
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

int ParseUtil::jsonToInt(const QJsonValue &value, bool * ok) {
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

bool ParseUtil::jsonToBool(const QJsonValue &value, bool * ok) {
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

int ParseUtil::getJsonLineNumber(const QString &filepath, const QString &searchText) {
    if (searchText.isEmpty())
        return 0;

    const QString text = readTextFile(filepath);
    int index = text.indexOf(searchText);
    if (index < 0)
        return 0;

    return text.left(index).count('\n') + 1;
}

int ParseUtil::getScriptLineNumber(const QString &filepath, const QString &scriptLabel) {
    if (scriptLabel.isEmpty())
        return 0;

    if (filepath.endsWith(".inc") || filepath.endsWith(".s"))
        return getRawScriptLineNumber(readTextFile(filepath), scriptLabel);
    else if (filepath.endsWith(".pory"))
        return getPoryScriptLineNumber(readTextFile(filepath), scriptLabel);

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

QStringList ParseUtil::getGlobalScriptLabels(const QString &filePath, QString *error) {
    if (filePath.endsWith(".inc") || filePath.endsWith(".s"))
        return getGlobalRawScriptLabels(readTextFile(filePath, error));
    else if (filePath.endsWith(".pory"))
        return getGlobalPoryScriptLabels(readTextFile(filePath, error));
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
