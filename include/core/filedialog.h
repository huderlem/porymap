#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include <QFileDialog>

/*
    Static QFileDialog functions will (unless otherwise specified) use native file dialogs.
    In general this is good (we want our file dialogs to be visually seamless) but unfortunately
    the native file dialogs ignore the parent widget, so in some cases they'll return focus to
    the main window rather than the window that opened the file dialog.

    To make working around this a little easier we use this class, which will use the native
    file dialog and manually return focus to the parent widget.

    It will also save the directory of the previous file selected in a file dialog, and if
    no 'dir' argument is specified it will open new dialogs at that directory.

*/

class FileDialog : public QFileDialog
{
public:
    FileDialog(QWidget *parent, Qt::WindowFlags flags) : QFileDialog(parent, flags) {};
    FileDialog(QWidget *parent = nullptr,
               const QString &caption = QString(),
               const QString &directory = QString(),
               const QString &filter = QString()) : QFileDialog(parent, caption, directory, filter) {};

    static void setDirectory(const QString &dir) { FileDialog::prevDirectory = dir; }
    static QString getDirectory() { return FileDialog::prevDirectory; }

    static QString getOpenFileName(QWidget *parent = nullptr,
                                   const QString &caption = QString(),
                                   const QString &dir = QString(),
                                   const QString &filter = QString(),
                                   QString *selectedFilter = nullptr,
                                   QFileDialog::Options options = Options());

    static QString getSaveFileName(QWidget *parent = nullptr,
                                   const QString &caption = QString(),
                                   const QString &dir = QString(),
                                   const QString &filter = QString(),
                                   QString *selectedFilter = nullptr,
                                   QFileDialog::Options options = Options());

private:
    static QString prevDirectory;
    static QString getDirectoryFromInput(const QString &dir);
    static void setDirectoryFromFile(const QString &fileName);
    static void restoreFocus(QWidget *parent);
};

#endif // FILEDIALOG_H
