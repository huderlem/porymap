#include "filedialog.h"

QString FileDialog::prevDirectory;

 QString FileDialog::getDirectoryFromInput(const QString &dir) {
    if (dir.isEmpty())
        return FileDialog::prevDirectory;
    return dir;
 }

void FileDialog::setDirectoryFromFile(const QString &fileName) {
    if (!fileName.isEmpty())
        FileDialog::prevDirectory = QFileInfo(fileName).absolutePath();
}

void FileDialog::restoreFocus(QWidget *parent) {
    if (parent) {
        parent->raise();
        parent->activateWindow();
    }
}

QString FileDialog::getOpenFileName(QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options) {
    const QString fileName = QFileDialog::getOpenFileName(parent, caption, getDirectoryFromInput(dir), filter, selectedFilter, options);
    setDirectoryFromFile(fileName);
    restoreFocus(parent);
    return fileName;
}

QStringList FileDialog::getOpenFileNames(QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options) {
    const QStringList fileNames = QFileDialog::getOpenFileNames(parent, caption, getDirectoryFromInput(dir), filter, selectedFilter, options);
    if (!fileNames.isEmpty())
        setDirectoryFromFile(fileNames.last());
    restoreFocus(parent);
    return fileNames;
}

QString FileDialog::getSaveFileName(QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options) {
    const QString fileName = QFileDialog::getSaveFileName(parent, caption, getDirectoryFromInput(dir), filter, selectedFilter, options);
    setDirectoryFromFile(fileName);
    restoreFocus(parent);
    return fileName;
}

QString FileDialog::getExistingDirectory(QWidget *parent, const QString &caption, const QString &dir, QFileDialog::Options options) {
    const QString existingDir = QFileDialog::getExistingDirectory(parent, caption, getDirectoryFromInput(dir), options);
    if (!existingDir.isEmpty())
        setDirectory(existingDir);
    restoreFocus(parent);
    return existingDir;
}
