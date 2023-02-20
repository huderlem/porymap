#include "aboutporymap.h"
#include "ui_aboutporymap.h"
#include "log.h"

AboutPorymap::AboutPorymap(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AboutPorymap)
{
    ui->setupUi(this);

    this->ui->textBrowser->setSource(QUrl("qrc:/CHANGELOG.md"));
}

AboutPorymap::~AboutPorymap()
{
    delete ui;
}

// Returns the Porymap version number as a list of ints with the order {major, minor, patch}
QList<int> AboutPorymap::getVersionNumbers()
{
    // Get the version string "#.#.#"
    static const QRegularExpression regex("Version (\\d+)\\.(\\d+)\\.(\\d+)");
    QRegularExpressionMatch match = regex.match(ui->label_Version->text());
    if (!match.hasMatch()) {
        logError("Failed to locate Porymap version text");
        return QList<int>({0, 0, 0});
    }
    return QList<int>({match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt()});
}
