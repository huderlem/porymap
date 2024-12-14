#include "validator.h"

// Identifiers must only contain word characters, and cannot start with a digit.
const QRegularExpression IdentifierValidator::re_identifier = QRegularExpression("[A-Za-z_]+[\\w]*");


bool PrefixValidator::missingPrefix(const QString &input) const {
    return !m_prefix.isEmpty() && !input.startsWith(m_prefix);
}

QValidator::State PrefixValidator::validate(QString &input, int &pos) const {
    auto state = QRegularExpressionValidator::validate(input, pos);
    if (state == QValidator::Acceptable) {
        // This input could be valid. If there's a prefix we should require it now.
        if (missingPrefix(input))
            state = QValidator::Intermediate;
    }
    return state;
}

void PrefixValidator::fixup(QString &input) const {
    QRegularExpressionValidator::fixup(input);
    if (missingPrefix(input))
        input.prepend(m_prefix);
}

bool PrefixValidator::isValid(QString &input) const {
    int pos = 0;
    return validate(input, pos) == QValidator::Acceptable;
}
