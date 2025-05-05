#ifndef VALIDATOR_H
#define VALIDATOR_H

/*
    This file contains our subclasses of QValidator.

    - PrefixValidator is for input widgets that want to enforce a particular prefix.
      It differs from a QRegularExpressionValidator with a prefix in the regex because
      it will automatically enforce the prefix in fixup() if it isn't present.
      It's preferable to QLineEdit's input mask because it won't affect cursor behavior.

    - IdentifierValidator is for validating that input text can be used for the name of an identifier in the project.
      (i.e., starts with a letter or underscore, then may continue with letters, numbers, or underscores).
      Unless a prefix is specified this is a normal QRegularExpressionValidator, we only have a subclass because we use it so often.

    - UppercaseValidator is just a validator that uppercases input text.
*/

#include <QValidator>

class PrefixValidator : public QRegularExpressionValidator {
    Q_OBJECT

public:
    explicit PrefixValidator(const QString &prefix, QObject *parent = nullptr)
        : QRegularExpressionValidator(parent), m_prefix(prefix) {};
    explicit PrefixValidator(const QString &prefix, const QRegularExpression &re, QObject *parent = nullptr)
        : QRegularExpressionValidator(re, parent), m_prefix(prefix) {};
    ~PrefixValidator() {};

    virtual QValidator::State validate(QString &input, int &) const override;
    virtual void fixup(QString &input) const override;

    QString prefix() const { return m_prefix; }
    void setPrefix(const QString &prefix);

    bool isValid(const QString &input) const;

private:
    QString m_prefix;

    bool missingPrefix(const QString &input) const;
};

class IdentifierValidator : public PrefixValidator {
    Q_OBJECT

public:  
    explicit IdentifierValidator(QObject *parent = nullptr)
        : PrefixValidator("", re_identifier, parent) {};
    explicit IdentifierValidator(const QString &prefix, QObject *parent = nullptr)
        : PrefixValidator(prefix, re_identifier, parent) {};
    ~IdentifierValidator() {};

    void setAllowEmpty(bool allowEmpty);

private:
    static const QRegularExpression re_identifier;
    static const QRegularExpression re_identifierOrEmpty;
};

class UppercaseValidator : public QValidator {
    virtual QValidator::State validate(QString &input, int &) const override {
        input = input.toUpper();
        return QValidator::Acceptable;
    }
};

#endif // VALIDATOR_H
