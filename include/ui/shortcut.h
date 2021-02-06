#ifndef SHORTCUT_H
#define SHORTCUT_H

#include <QObject>
#include <QWidget>
#include <QKeySequence>
#include <QShortcut>


// Alternative to QShortcut that adds support for multiple key sequences.
// Use this to allow the shortcut to be editable in ShortcutsEditor.
class Shortcut : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QShortcut)
    Q_PROPERTY(QKeySequence key READ key WRITE setKey)
    Q_PROPERTY(QString whatsThis READ whatsThis WRITE setWhatsThis)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
    Q_PROPERTY(bool autoRepeat READ autoRepeat WRITE setAutoRepeat)
    Q_PROPERTY(Qt::ShortcutContext context READ context WRITE setContext)

public:
    explicit Shortcut(QWidget *parent);
    Shortcut(const QKeySequence &key, QWidget *parent,
             const char *member = nullptr, const char *ambiguousMember = nullptr,
             Qt::ShortcutContext shortcutContext = Qt::WindowShortcut);
    Shortcut(const QList<QKeySequence> &keys, QWidget *parent,
             const char *member = nullptr, const char *ambiguousMember = nullptr,
             Qt::ShortcutContext shortcutContext = Qt::WindowShortcut);
    ~Shortcut();

    void addKey(const QKeySequence &key);
    void setKey(const QKeySequence &key);
    QKeySequence key() const;

    void addKeys(const QList<QKeySequence> &keys);
    void setKeys(const QList<QKeySequence> &keys);
    QList<QKeySequence> keys() const;

    void setEnabled(bool enable);
    bool isEnabled() const;

    void setContext(Qt::ShortcutContext context);
    Qt::ShortcutContext context() const;

    void setWhatsThis(const QString &text);
    QString whatsThis() const;

    void setAutoRepeat(bool on);
    bool autoRepeat() const;

    int id() const;
    QList<int> ids() const;

    inline QWidget *parentWidget() const
    { return static_cast<QWidget *>(QObject::parent()); }

signals:
    void activated();
    void activatedAmbiguously();

protected:
    bool event(QEvent *e) override;

private:
    const char *sc_member;
    const char *sc_ambiguousmember;
    Qt::ShortcutContext sc_context;
    QVector<QShortcut *> sc_vec;
};

#endif // SHORTCUT_H
