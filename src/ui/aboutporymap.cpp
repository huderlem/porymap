#include "aboutporymap.h"
#include "ui_aboutporymap.h"

AboutPorymap::AboutPorymap(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutPorymap)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    static const QString commitHash = PORYMAP_LATEST_COMMIT;
    this->ui->label_Version->setText(getVersionString());

    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

QString AboutPorymap::getVersionString() {
    static const QString commitHash = PORYMAP_LATEST_COMMIT;
    return QString("Version %1%2\nQt %3 (%4)\n%5")
                                        .arg(QCoreApplication::applicationVersion())
                                        .arg(commitHash.isEmpty() ? "" : QString(" (%1)").arg(commitHash))
                                        .arg(QStringLiteral(QT_VERSION_STR))
                                        .arg(QSysInfo::buildCpuArchitecture())
                                        .arg(QStringLiteral(__DATE__));
}

AboutPorymap::~AboutPorymap()
{
    delete ui;
}
