#ifndef UPDATEPROMOTER_H
#define UPDATEPROMOTER_H

#include "network.h"

#include <QDialog>
#include <QPushButton>

namespace Ui {
class UpdatePromoter;
}

class UpdatePromoter : public QDialog
{
    Q_OBJECT

public:
    explicit UpdatePromoter(QWidget *parent, NetworkAccessManager *manager);
    ~UpdatePromoter() {};

    void checkForUpdates();
    void updatePreferences();

private:
    Ui::UpdatePromoter *ui;
    NetworkAccessManager *const manager;
    QPushButton * button_Downloads;
    QPushButton * button_Retry;

    QString changelog;
    QUrl downloadUrl;
    bool breakingChanges;
    bool foundReleases;

    QSet<QUrl> visitedUrls; // Prevent infinite redirection

    void resetDialog();
    void get(const QUrl &url);
    void processWebpage(const QJsonDocument &data, const QUrl &nextUrl);
    void error(const QString &err, const QDateTime time = QDateTime());
    bool isNewerVersion(int major, int minor, int patch);

private slots:
    void dialogButtonClicked(QAbstractButton *button);

signals:
    void changedPreferences();
};

#endif // UPDATEPROMOTER_H
