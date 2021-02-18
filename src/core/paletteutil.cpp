#include "paletteutil.h"
#include "log.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>

QList<QRgb> parsePal(QString filepath, bool *error);
QList<QRgb> parseJASC(QString filepath, bool *error);
QList<QRgb> parseAdvanceMapPal(QString filepath, bool *error);
QList<QRgb> parseAdobeColorTable(QString filepath, bool *error);
QList<QRgb> parseTileLayerPro(QString filepath, bool *error);
QList<QRgb> parseAdvancePaletteEditor(QString filepath, bool *error);
int clampColorValue(int value);

QList<QRgb> PaletteUtil::parse(QString filepath, bool *error) {
    QFileInfo info(filepath);
    QString extension = info.completeSuffix();
    if (extension.isNull()) {
        logError(QString("Failed to parse palette file '%1' because it has an unrecognized extension '%2'").arg(filepath).arg(extension));
        *error = true;
        return QList<QRgb>();
    }

    extension = extension.toLower();
    if (extension == "pal") {
        return parsePal(filepath, error);
    } else if (extension == "act") {
        return parseAdobeColorTable(filepath, error);
    } else if (extension == "tpl") {
        return parseTileLayerPro(filepath, error);
    } else if (extension == "gpl") {
        return parseAdvancePaletteEditor(filepath, error);
    } else {
        logError(QString("Unsupported palette file. Supported formats are: .pal"));
        *error = true;
    }

    return QList<QRgb>();
}

void PaletteUtil::writeJASC(QString filepath, QVector<QRgb> palette, int offset, int nColors) {
    if (!nColors) {
        logWarn(QString("Cannot save a palette with no colors."));
        return;
    }
    if (offset > palette.size() || offset + nColors > palette.size()) {
        logWarn("Palette offset out of range for color table.");
        return;
    }

    QString text = "JASC-PAL\r\n0100\r\n";
    text += QString::number(nColors) + "\r\n";

    for (int i = offset; i < offset + nColors; i++) {
        QRgb color = palette.at(i);
        text += QString::number(qRed(color)) + " "
              + QString::number(qGreen(color)) + " "
              + QString::number(qBlue(color)) + "\r\n";
    }

    QFile file(filepath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(text.toUtf8());
    } else {
        logWarn(QString("Could not write to file '%1': ").arg(filepath) + file.errorString());
    }
}

QList<QRgb> parsePal(QString filepath, bool *error) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = true;
        logError(QString("Could not open palette file '%1': ").arg(filepath) + file.errorString());
        return QList<QRgb>();
    }

    QTextStream in(&file);
    QString firstLine = in.readLine();
    if (firstLine == "JASC-PAL") {
        file.close();
        return parseJASC(filepath, error);
    } else {
        file.close();
        return parseAdvanceMapPal(filepath, error);
    }
}

QList<QRgb> parseJASC(QString filepath, bool *error) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = true;
        logError(QString("Could not open JASC palette file '%1': ").arg(filepath) + file.errorString());
        return QList<QRgb>();
    }

    QTextStream in(&file);
    if (in.readLine() != "JASC-PAL") {
        *error = true;
        logError(QString("JASC palette file '%1' had an unexpected format. First line must be 'JASC-PAL'.").arg(filepath));
        file.close();
        return QList<QRgb>();
    }

    if (in.readLine() != "0100") {
        *error = true;
        logError(QString("JASC palette file '%1' had an unexpected format. Second line must be '0100'.").arg(filepath));
        file.close();
        return QList<QRgb>();
    }

    QString numColorsStr = in.readLine();
    bool numOk;
    int numColors = numColorsStr.toInt(&numOk);
    if (!numOk) {
        *error = true;
        logError(QString("JASC palette file '%1' had an unexpected format. Third line must be the number of colors.").arg(filepath));
        return QList<QRgb>();
    }

    QList<QRgb> palette;
    QRegularExpression re("(?<red>\\d+)\\s(?<green>\\d+)\\s(?<blue>\\d+)");
    while (!in.atEnd() && numColors > 0) {
        numColors--;
        QString line = in.readLine();
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            QString redStr = match.captured("red");
            QString greenStr = match.captured("green");
            QString blueStr = match.captured("blue");
            bool redOk, greenOk, blueOk;
            int red = redStr.toInt(&redOk);
            int green = greenStr.toInt(&greenOk);
            int blue = blueStr.toInt(&blueOk);
            if (!redOk || !greenOk || !blueOk) {
                *error = true;
                logError(QString("JASC palette file '%1' had an unexpected format. Invalid color '%2'.").arg(filepath).arg(line));
                return QList<QRgb>();
            }

            palette.append(qRgb(clampColorValue(red),
                                clampColorValue(green),
                                clampColorValue(blue)));
        } else {
            *error = true;
            logError(QString("JASC palette file '%1' had an unexpected format. Invalid color '%2'.").arg(filepath).arg(line));
            file.close();
            return QList<QRgb>();
        }
    }

    file.close();
    return palette;
}

QList<QRgb> parseAdvanceMapPal(QString filepath, bool *error) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = true;
        logError(QString("Could not open Advance Map 1.92 palette file '%1': ").arg(filepath) + file.errorString());
        return QList<QRgb>();
    }

    QByteArray in = file.readAll();
    file.close();

    if (in.length() % 4 != 0) {
        *error = true;
        logError(QString("Advance Map 1.92 palette file '%1' had an unexpected format. File's length must be a multiple of 4, but the length is %2.").arg(filepath).arg(in.length()));
        return QList<QRgb>();
    }

    QList<QRgb> palette;
    int i = 0;
    while (i < in.length()) {
        unsigned char red = static_cast<unsigned char>(in.at(i));
        unsigned char green = static_cast<unsigned char>(in.at(i + 1));
        unsigned char blue = static_cast<unsigned char>(in.at(i + 2));
        palette.append(qRgb(clampColorValue(red),
                            clampColorValue(green),
                            clampColorValue(blue)));
        i += 4;
    }

    return palette;
}

QList<QRgb> parseAdobeColorTable(QString filepath, bool *error) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = true;
        logError(QString("Could not open Adobe Color Table palette file '%1': ").arg(filepath) + file.errorString());
        return QList<QRgb>();
    }

    QByteArray in = file.readAll();
    file.close();

    if (in.length() != 0x300) {
        *error = true;
        logError(QString("Adobe Color Table palette file '%1' had an unexpected format. File's length must be exactly 768, but the length is %2.").arg(filepath).arg(in.length()));
        return QList<QRgb>();
    }

    QList<QRgb> palette;
    int i = 0;
    while (i < in.length()) {
        unsigned char red = static_cast<unsigned char>(in.at(i));
        unsigned char green = static_cast<unsigned char>(in.at(i + 1));
        unsigned char blue = static_cast<unsigned char>(in.at(i + 2));
        palette.append(qRgb(clampColorValue(red),
                            clampColorValue(green),
                            clampColorValue(blue)));
        i += 3;
    }

    return palette;
}

QList<QRgb> parseTileLayerPro(QString filepath, bool *error) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = true;
        logError(QString("Could not open Tile Layer Pro palette file '%1': ").arg(filepath) + file.errorString());
        return QList<QRgb>();
    }

    QByteArray in = file.readAll();
    file.close();

    if (in.length() < 4 || in.at(0) != 'T' || in.at(1) != 'L' || in.at(2) != 'P' || in.at(3) != 0) {
        *error = true;
        logError(QString("Tile Layer Pro palette file '%1' had an unexpected format. The TLP header is missing.").arg(filepath).arg(in.length()));
        return QList<QRgb>();
    }

    if (in.length() != 0x304) {
        *error = true;
        logError(QString("Tile Layer Pro palette file '%1' had an unexpected format. File's length must be exactly 772, but the length is %2.").arg(filepath).arg(in.length()));
        return QList<QRgb>();
    }

    QList<QRgb> palette;
    int i = 4;
    while (i < in.length()) {
        unsigned char red = static_cast<unsigned char>(in.at(i));
        unsigned char green = static_cast<unsigned char>(in.at(i + 1));
        unsigned char blue = static_cast<unsigned char>(in.at(i + 2));
        palette.append(qRgb(clampColorValue(red),
                            clampColorValue(green),
                            clampColorValue(blue)));
        i += 3;
    }

    return palette;
}

QList<QRgb> parseAdvancePaletteEditor(QString filepath, bool *error) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = true;
        logError(QString("Could not open GPL palette file '%1': ").arg(filepath) + file.errorString());
        return QList<QRgb>();
    }

    QTextStream in(&file);
    if (in.readLine() != "[APE Palette]") {
        *error = true;
        logError(QString("GPL palette file '%1' had an unexpected format. First line must be '[APE Palette]'.").arg(filepath));
        file.close();
        return QList<QRgb>();
    }

    QList<QRgb> palette;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        bool ok;
        unsigned int raw = line.toUInt(&ok);
        if (!ok) {
            *error = true;
            logError(QString("GPL palette file '%1' had an unexpected format. Invalid color '%2'.").arg(filepath).arg(line));
            file.close();
            return QList<QRgb>();
        }

        raw = ((raw & 0xFF)<< 8) | ((raw >> 8) & 0xFF);
        int red = (raw & 0x1F) * 8;
        int green = ((raw >> 5) & 0x1F) * 8;
        int blue = ((raw >> 10) & 0x1F) * 8;
        palette.append(qRgb(clampColorValue(red),
                            clampColorValue(green),
                            clampColorValue(blue)));
    }

    file.close();
    return palette;
}

int clampColorValue(int value) {
    if (value < 0) {
        value = 0;
    }
    if (value > 255) {
        value = 255;
    }
    return value;
}
