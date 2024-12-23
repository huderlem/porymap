#include "aboutporymap.h"
#include "ui_aboutporymap.h"

AboutPorymap::AboutPorymap(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutPorymap)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    static const QString commitHash = PORYMAP_LATEST_COMMIT;
    this->ui->label_Version->setText(QString("Version %1%2\nQt %3 (%4)\n%5")
                                        .arg(QCoreApplication::applicationVersion())
                                        .arg(commitHash.isEmpty() ? "" : QString(" (%1)").arg(commitHash))
                                        .arg(QStringLiteral(QT_VERSION_STR))
                                        .arg(QSysInfo::buildCpuArchitecture())
                                        .arg(QStringLiteral(__DATE__))
                                    );

    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

AboutPorymap::~AboutPorymap()
{
    delete ui;
}
