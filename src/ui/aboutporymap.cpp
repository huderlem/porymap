#include "aboutporymap.h"
#include "ui_aboutporymap.h"
#include "log.h"

AboutPorymap::AboutPorymap(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AboutPorymap)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    QString versionInfo = QString("Version %1 - %2").arg(QCoreApplication::applicationVersion()).arg(QStringLiteral(__DATE__));

    static const QString commitHash = PORYMAP_LATEST_COMMIT;
    if (!commitHash.isEmpty())
        versionInfo.append(QString("\nCommit %1").arg(commitHash));

    this->ui->label_Version->setText(versionInfo);
    this->ui->textBrowser->setSource(QUrl("qrc:/CHANGELOG.md"));
}

AboutPorymap::~AboutPorymap()
{
    delete ui;
}
