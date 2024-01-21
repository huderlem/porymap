#ifndef UPDATEPROMOTER_H
#define UPDATEPROMOTER_H

#include <QDialog>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class UpdatePromoter;
}

class UpdatePromoter : public QDialog
{
    Q_OBJECT

public:
    explicit UpdatePromoter(QWidget *parent, QNetworkAccessManager *manager);
    ~UpdatePromoter() {};

    void checkForUpdates();
    void requestDialog();
    void updatePreferences();

private:
    Ui::UpdatePromoter *ui;
    QNetworkAccessManager *const manager;
    QNetworkReply * reply = nullptr;
    QPushButton * button_Downloads;
    QString downloadLink;
    QString changelog;

    void resetDialog();
    void processWebpage(const QJsonDocument &data);
    void processError(const QString &err);
    bool isNewerVersion(int major, int minor, int patch);

private slots:
    void dialogButtonClicked(QAbstractButton *button);

signals:
    void changedPreferences();
};

#endif // UPDATEPROMOTER_H
