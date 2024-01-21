#include "updatepromoter.h"
#include "ui_updatepromoter.h"
#include "log.h"
#include "config.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDesktopServices>

UpdatePromoter::UpdatePromoter(QWidget *parent, QNetworkAccessManager *manager)
    : QDialog(parent),
      ui(new Ui::UpdatePromoter),
      manager(manager)
{
    ui->setupUi(this);

    // Set up "Do not alert me" check box
    this->updatePreferences();
    connect(ui->checkBox_StopAlerts, &QCheckBox::stateChanged, [this](int state) {
        bool enable = (state != Qt::Checked);
        porymapConfig.setCheckForUpdates(enable);
        emit this->changedPreferences();
    });

    // Set up button box
    this->button_Downloads = ui->buttonBox->addButton("Go to Downloads...", QDialogButtonBox::ActionRole);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &UpdatePromoter::dialogButtonClicked);

    this->resetDialog();
}

void UpdatePromoter::resetDialog() {
    this->button_Downloads->setEnabled(false);
    ui->text_Changelog->setVisible(false);
    ui->label_Warning->setVisible(false);
    ui->label_Status->setText("Checking for updates...");

    this->changelog = QString();
    this->downloadLink = QString();
}

void UpdatePromoter::checkForUpdates() {
   // Ignore request if one is still active.
    if (this->reply && !this->reply->isFinished())
        return;
    this->resetDialog();
    ui->buttonBox->button(QDialogButtonBox::Retry)->setEnabled(false);

    // We could get ".../releases/latest" to retrieve less data, but this would run into problems if the
    // most recent item on the releases page is not actually a new release (like the static windows build).
    // By getting all releases we can also present a multi-version changelog of all changes since the host release.
    static const QNetworkRequest request(QUrl("https://api.github.com/repos/huderlem/porymap/releases"));
    this->reply = this->manager->get(request);

    connect(this->reply, &QNetworkReply::finished, [this] {
        ui->buttonBox->button(QDialogButtonBox::Retry)->setEnabled(true);
        auto error = this->reply->error();
        if (error == QNetworkReply::NoError) {
            this->processWebpage(QJsonDocument::fromJson(this->reply->readAll()));
        } else {
            this->processError(this->reply->errorString());
        }
    });
}

// Read all the items on the releases page, ignoring entries without a version identifier tag.
// Objects in the releases page data are sorted newest to oldest. 
void UpdatePromoter::processWebpage(const QJsonDocument &data) {
    bool updateAvailable = false;
    bool breakingChanges = false;
    bool foundRelease = false;

    const QJsonArray releases = data.array();
    for (int i = 0; i < releases.size(); i++) {
        auto release = releases.at(i).toObject();

        // Convert tag string to version numbers
        const QString tagName = release.value("tag_name").toString();
        const QStringList tag = tagName.split(".");
        if (tag.length() != 3) continue;
        bool ok;
        int major = tag.at(0).toInt(&ok);
        if (!ok) continue;
        int minor = tag.at(1).toInt(&ok);
        if (!ok) continue;
        int patch = tag.at(2).toInt(&ok);
        if (!ok) continue;

        // We've found a valid release tag. If the version number is not newer than the host version then we can stop looking at releases.
        foundRelease = true;
        logInfo(QString("Found release %1.%2.%3").arg(major).arg(minor).arg(patch)); // TODO: Remove
        if (major <= PORYMAP_VERSION_MAJOR && minor <= PORYMAP_VERSION_MINOR && patch <= PORYMAP_VERSION_PATCH)
            break;

        const QString description = release.value("body").toString();
        const QString url = release.value("html_url").toString();
        if (description.isEmpty() || url.isEmpty()) {
            // If the release was published very recently it won't have a description yet, in which case don't tell the user about it yet.
            // If there's no URL, something has gone wrong and we should skip this release.
            continue; 
        }

        if (this->downloadLink.isEmpty()) {
            // This is the first (newest) release we've found. Record its URL for download.
            this->downloadLink = url;
            breakingChanges = (major > PORYMAP_VERSION_MAJOR);
        }

        // Record the changelog of this release so we can show all changes since the host release.
        this->changelog.append(QString("## %1\n%2\n\n").arg(tagName).arg(description));
        updateAvailable = true;
    }

    if (!foundRelease) {
        // We retrieved the webpage but didn't successfully parse any releases.
        this->processError("Error parsing releases webpage");
        return;
    }

    // If there's a new update available the dialog will always be opened.
    // Otherwise the dialog is only open if the user requested it.
    if (updateAvailable) {
        this->button_Downloads->setEnabled(!this->downloadLink.isEmpty());
        ui->text_Changelog->setMarkdown(this->changelog);
        ui->text_Changelog->setVisible(true);
        ui->label_Warning->setVisible(breakingChanges);
        ui->label_Status->setText("A new version of Porymap is available!");
        this->show();
    } else {
        // The rest of the UI remains in the state set by resetDialog
        ui->label_Status->setText("Your version of Porymap is up to date!");
    }   
}

void UpdatePromoter::processError(const QString &err) {
    const QString message = QString("Failed to check for version update: %1").arg(err);
    if (this->isVisible()) {
        ui->label_Status->setText(message);
    } else {
        logWarn(message);
    }
}

// The dialog can either be shown programmatically when an update is available
// or if the user manually selects "Check for Updates" in the menu.
// When the dialog is shown programmatically there is a check box to disable automatic alerts.
// If the user requested the dialog (and it wasn't already open) this check box should be hidden.
void UpdatePromoter::requestDialog() {
    if (!this->isVisible()){
        ui->checkBox_StopAlerts->setVisible(false);
        this->show();
    } else if (this->isMinimized()) {
        this->showNormal();
    } else {
        this->raise();
        this->activateWindow();
    }
}

void UpdatePromoter::updatePreferences() {
    const QSignalBlocker blocker(ui->checkBox_StopAlerts);
    ui->checkBox_StopAlerts->setChecked(porymapConfig.getCheckForUpdates());
}

void UpdatePromoter::dialogButtonClicked(QAbstractButton *button) {
    auto buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::RejectRole) {
        this->close();
    } else if (buttonRole == QDialogButtonBox::AcceptRole) {
        // "Retry" button
        this->checkForUpdates();
    } else if (button == this->button_Downloads && !this->downloadLink.isEmpty()) {
        QDesktopServices::openUrl(QUrl(this->downloadLink));
    }
}
