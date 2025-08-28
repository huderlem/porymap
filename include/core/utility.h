#pragma once
#ifndef UTILITY_H
#define UTILITY_H

#include <QString>
#include <QLineEdit>
#include <QColorSpace>

namespace Util {
    void numericalModeSort(QStringList &list);
    int roundUpToMultiple(int numToRound, int multiple);
    QString toDefineCase(QString input);
    QString toHexString(uint32_t value, int minLength = 0);
    QString toHtmlParagraph(const QString &text);
    QString stripPrefix(const QString &s, const QString &prefix);
    Qt::Orientations getOrientation(bool xflip, bool yflip);
    QString replaceExtension(const QString &path, const QString &newExtension);
    void setErrorStylesheet(QLineEdit *lineEdit, bool isError);
    QString toStylesheetString(const QFont &font);
    void show(QWidget *w);
    QColorSpace toColorSpace(int colorSpaceInt);
}

#endif // UTILITY_H
