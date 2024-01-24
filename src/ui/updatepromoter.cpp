#include "updatepromoter.h"
#include "ui_updatepromoter.h"
#include "log.h"
#include "config.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDesktopServices>
#include <QTimer>

UpdatePromoter::UpdatePromoter(QWidget *parent, NetworkAccessManager *manager)
    : QDialog(parent),
      ui(new Ui::UpdatePromoter),
      manager(manager)
{
    ui->setupUi(this);

    // Set up "Do not alert me" check box
    this->updatePreferences();
    ui->checkBox_StopAlerts->setVisible(false);
    connect(ui->checkBox_StopAlerts, &QCheckBox::stateChanged, [this](int state) {
        bool enable = (state != Qt::Checked);
        porymapConfig.setCheckForUpdates(enable);
        emit this->changedPreferences();
    });

    // Set up button box
    this->button_Retry = ui->buttonBox->button(QDialogButtonBox::Retry);
    this->button_Downloads = ui->buttonBox->addButton("Go to Downloads...", QDialogButtonBox::ActionRole);
    ui->buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &UpdatePromoter::dialogButtonClicked);

    this->resetDialog();
}

void UpdatePromoter::resetDialog() {
    this->button_Downloads->setEnabled(false);

    ui->text_Changelog->setVisible(false);
    ui->label_Warning->setVisible(false);
    ui->label_Status->setText("");

    this->changelog = QString();
    this->downloadUrl = QString();
    this->breakingChanges = false;
    this->foundReleases = false;
    this->visitedUrls.clear();
}

void UpdatePromoter::checkForUpdates() {
    // If the Retry button is disabled, making requests is disabled
    if (!this->button_Retry->isEnabled())
        return;

    this->resetDialog();
    this->button_Retry->setEnabled(false);
    ui->label_Status->setText("Checking for updates...");

    // We could use the URL ".../releases/latest" to retrieve less data, but this would run into problems if the
    // most recent item on the releases page is not actually a new release (like the static windows build).
    // By getting all releases we can also present a multi-version changelog of all changes since the host release.
    static const QUrl url("https://api.github.com/repos/huderlem/porymap/releases");
    this->get(url);
}

void UpdatePromoter::get(const QUrl &url) {
    this->visitedUrls.insert(url);
    auto reply = this->manager->get(url);
    connect(reply, &NetworkReplyData::finished, [this, reply] () {
        if (!reply->errorString().isEmpty()) {
            this->error(reply->errorString());
            this->disableRequestsUntil(reply->retryAfter());
        } else {
            this->processWebpage(QJsonDocument::fromJson(reply->body()), reply->nextUrl());
        }
        reply->deleteLater();
    });
}

// Read all the items on the releases page, ignoring entries without a version identifier tag.
// Objects in the releases page data are sorted newest to oldest. 
// Returns true when finished, returns false to request processing for the next page.
void UpdatePromoter::processWebpage(const QJsonDocument &data, const QUrl &nextUrl) {
    const QJsonArray releases = data.array();
    int i;
    for (i = 0; i < releases.size(); i++) {
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
        this->foundReleases = true;
        if (!this->isNewerVersion(major, minor, patch))
            break;

        const QString description = release.value("body").toString();
        if (description.isEmpty()) {
            // If the release was published very recently it won't have a description yet, in which case don't tell the user about it yet.
            continue; 
        }

        if (this->downloadUrl.isEmpty()) {
            // This is the first (newest) release we've found. Record its URL for download.
            const QUrl url = QUrl(release.value("html_url").toString());
            if (url.isEmpty()) {
                // If there's no URL, something has gone wrong and we should skip this release.
                continue;
            }
            this->downloadUrl = url;
            this->breakingChanges = (major > PORYMAP_VERSION_MAJOR);
        }

        // Record the changelog of this release so we can show all changes since the host release.
        this->changelog.append(QString("## %1\n%2\n\n").arg(tagName).arg(description));
    }

    // If we read the entire page then we didn't find a release as old as the host version.
    // Keep looking on the second page, there might still be new releases there.
    if (i == releases.size() && !nextUrl.isEmpty() && !this->visitedUrls.contains(nextUrl)) {
        this->get(nextUrl);
        return;
    }

    if (!this->foundReleases) {
        // We retrieved the webpage but didn't successfully parse any releases.
        this->error("Error parsing releases webpage");
        return;
    }

    // Populate dialog with result
    bool updateAvailable = !this->changelog.isEmpty();
    ui->label_Status->setText(updateAvailable ? "A new version of Porymap is available!"
                                              : "Your version of Porymap is up to date!");
    ui->label_Warning->setVisible(this->breakingChanges);
    ui->text_Changelog->setMarkdown(this->changelog);
    ui->text_Changelog->setVisible(updateAvailable);
    this->button_Downloads->setEnabled(!this->downloadUrl.isEmpty());
    this->button_Retry->setEnabled(true);

    // Alert the user if there's a new update available and the dialog wasn't already open.
    // Show the window, but also show the option to turn off automatic alerts in the future.
    if (updateAvailable && !this->isVisible()) {
        ui->checkBox_StopAlerts->setVisible(true);
        this->show();
    }
}

void UpdatePromoter::disableRequestsUntil(const QDateTime time) {
    this->button_Retry->setEnabled(false);

    auto timeUntil = QDateTime::currentDateTime().msecsTo(time);
    if (timeUntil < 0) timeUntil = 0;
    QTimer::singleShot(timeUntil, Qt::VeryCoarseTimer, [this]() {
        this->button_Retry->setEnabled(true);
    });
}

void UpdatePromoter::error(const QString &err) {
    const QString message = QString("Failed to check for version update: %1").arg(err);
    ui->label_Status->setText(message);
    if (!this->isVisible())
        logWarn(message);
}

bool UpdatePromoter::isNewerVersion(int major, int minor, int patch) {
    if (major != PORYMAP_VERSION_MAJOR)
        return major > PORYMAP_VERSION_MAJOR;
    if (minor != PORYMAP_VERSION_MINOR)
        return minor > PORYMAP_VERSION_MINOR;
    return patch > PORYMAP_VERSION_PATCH;
}

void UpdatePromoter::updatePreferences() {
    const QSignalBlocker blocker(ui->checkBox_StopAlerts);
    ui->checkBox_StopAlerts->setChecked(!porymapConfig.getCheckForUpdates());
}

void UpdatePromoter::dialogButtonClicked(QAbstractButton *button) {
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::RejectRole) {
        this->close();
    } else if (button == this->button_Retry) {
        this->checkForUpdates();
    } else if (button == this->button_Downloads) {
        QDesktopServices::openUrl(this->downloadUrl);
    }
}
