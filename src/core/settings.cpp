#include "settings.h"
#include "project.h"

#include <QRegularExpression>

void Settings::add_setting(QString line) {
    //
    if (line.isEmpty()) return;
    QStringList key_val = line.remove(" ").split(":");
    setValue(key_val[0], QVariant(key_val[1]));


    qDebug() << "key: " << key_val[0] << "val: " << QVariant(key_val[1]);
}

// call in MainWindow::onCloseEvent
void Settings::save() {
    //
    qDebug() << "Settings::save()";

    QString yaml_text = "";

    for (QString key : all_keys_) {
        //
        yaml_text += key + ": " + settings_[key].toString() + '\n';
        qDebug() << key;
    }
    QFile file(path_ + ".test");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(yaml_text.toUtf8());
    }
}

void Settings::load(QString path) {
    qDebug() << "Settings::load(" << path << ")";
    //
    path_ = path;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) save();// this should create a new file

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        add_setting(line.replace(QRegularExpression("#.*"), ""));// remove comments
    }
}

// done?
void Settings::clear() {
    qDebug() << "Settings::clear()";
    //
    all_keys_.clear();
    settings_.clear();
    //save();
}

// done?
void Settings::setValue(QString key, QVariant value) {
    qDebug() << "Settings::setValue(" << key << ", " << value << ")";
    //
    if (!(all_keys_.contains(key))) all_keys_.append(key);
    settings_[key] = value;
}

void Settings::remove(QString key) {
    //
    all_keys_.removeAll(key);
    settings_.remove(key);
}

bool Settings::contains(QString key) {
    //
    return all_keys_.contains(key);
}

QStringList Settings::allKeys() {
    //
    return all_keys_;
}

QVariant Settings::value(QString key) {
    //
    return settings_[key];
}










































