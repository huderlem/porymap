#include "utility.h"

#include <QCollator>
#include <QRegularExpression>
#include <QFileInfo>

// Sometimes we want to sort names alphabetically to make them easier to find in large combo box lists.
// QStringList::sort (as of writing) can only sort numbers in lexical order, which has an undesirable
// effect (e.g. 'ROUTE_1, ROUTE_10, ROUTE_2,...' instead of 'ROUTE_1, ROUTE_2,... ROUTE_10').
// We can use QCollator to sort these lists with better handling for numbers.
void Util::numericalModeSort(QStringList &list) {
    static QCollator collator;
    collator.setNumericMode(true);
    std::sort(list.begin(), list.end(), collator);
}

int Util::roundUpToMultiple(int numToRound, int multiple) {
    if (multiple <= 0)
        return numToRound;

    int remainder = abs(numToRound) % multiple;
    if (remainder == 0)
        return numToRound;

    if (numToRound < 0)
        return -(abs(numToRound) - remainder);
    else
        return numToRound + multiple - remainder;
}

// Ex: input 'GraniteCave_B1F' returns 'GRANITE_CAVE_B1F'.
QString Util::toDefineCase(QString input) {
    static const QRegularExpression re_CaseChange("([a-z])([A-Z])");
    input.replace(re_CaseChange, "\\1_\\2");

    // Remove sequential underscores
    static const QRegularExpression re_Underscores("_+");
    input.replace(re_Underscores, "_");

    return input.toUpper();
}

QString Util::toHexString(uint32_t value, int minLength) {
    return "0x" + QString("%1").arg(value, minLength, 16, QChar('0')).toUpper();
}

QString Util::toHtmlParagraph(const QString &text) {
    return QString("<html><head/><body><p>%1</p></body></html>").arg(text);
}

QString Util::toStylesheetString(const QFont &font) {
    QString s = QString("font-family: \"%1\";").arg(font.family());

    // Font size
    if (font.pixelSize() >= 0) {
        s.append(QString(" font-size: %1px;").arg(font.pixelSize()));
    } else if (font.pointSize() >= 0) {
        s.append(QString(" font-size: %1pt;").arg(font.pointSize()));
    }

    // Font weight
    if (font.weight() >= 0) {
        s.append(QString(" font-weight: %1;").arg(font.weight()));
    }

    // Text decoration
    if (font.underline() || font.strikeOut()) {
        s.append(QString(" text-decoration:%1%2;")
                    .arg(font.underline() ? " underline" : "")
                    .arg(font.strikeOut() ? " line-through" : ""));
    } else {
        s.append(" text-decoration: none;");
    }

    // Font style
    if (font.style() == QFont::StyleItalic) {
        s.append(" font-style: italic;");
    } else if (font.style() == QFont::StyleOblique) {
        s.append(" font-style: oblique;");
    } else {
        s.append(" font-style: normal;");
    }

    return s;
}

QString Util::stripPrefix(const QString &s, const QString &prefix) {
    if (!s.startsWith(prefix)) {
        return s;
    }
    return QString(s).remove(0, prefix.length());
}

Qt::Orientations Util::getOrientation(bool xflip, bool yflip) {
    Qt::Orientations flags;
    if (xflip) flags |= Qt::Orientation::Horizontal;
    if (yflip) flags |= Qt::Orientation::Vertical;
    return flags;
}

QString Util::replaceExtension(const QString &path, const QString &newExtension) {
    if (path.isEmpty()) return path;
    int oldExtensionLen = QFileInfo(path).completeSuffix().length();
    QString basePath = path.left(path.size() - oldExtensionLen);
    if (!basePath.endsWith(".")) {
        basePath.append(".");
    }
    return basePath + newExtension;
}

void Util::setErrorStylesheet(QLineEdit *lineEdit, bool isError) {
    static const QString stylesheet = QStringLiteral("QLineEdit { background-color: rgba(255, 0, 0, 25%) }");
    lineEdit->setStyleSheet(isError ? stylesheet : "");
}

void Util::show(QWidget *w) {
    if (!w) return;

    if (!w->isVisible()) {
        w->show();
    } else if (w->isMinimized()) {
        w->showNormal();
    } else {
        w->raise();
        w->activateWindow();
    }
}

// Safe conversion from an int representing a QColorSpace::NamedColorSpace to a QColorSpace.
// This lets us use 0 to mean "no color space".
QColorSpace Util::toColorSpace(int colorSpaceInt) {
    QColorSpace colorSpace;

    int min = static_cast<int>(QColorSpace::SRgb);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))
    // Qt 6.8.0 introduced additional color spaces
    int max = static_cast<int>(QColorSpace::Bt2100Hlg);
#else
    int max = static_cast<int>(QColorSpace::ProPhotoRgb);
#endif

    if (colorSpaceInt >= min && colorSpaceInt <= max) {
        return QColorSpace(static_cast<QColorSpace::NamedColorSpace>(colorSpaceInt));
    } else {
        return QColorSpace();
    }
}
