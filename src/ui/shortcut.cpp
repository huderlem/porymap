#include "shortcut.h"

#include <QtEvents>
#include <QWhatsThis>


Shortcut::Shortcut(QWidget *parent) :
    QObject(parent),
    sc_member(nullptr),
    sc_ambiguousmember(nullptr),
    sc_context(Qt::WindowShortcut),
    sc_vec(QVector<QShortcut *>({new QShortcut(parent)}))
{  }

Shortcut::Shortcut(const QKeySequence &key, QWidget *parent,
                   const char *member, const char *ambiguousMember,
                   Qt::ShortcutContext shortcutContext) :
    QObject(parent),
    sc_member(member),
    sc_ambiguousmember(ambiguousMember),
    sc_context(shortcutContext),
    sc_vec(QVector<QShortcut *>())
{
    setKey(key);
}

Shortcut::Shortcut(const QList<QKeySequence> &keys, QWidget *parent,
                   const char *member, const char *ambiguousMember,
                   Qt::ShortcutContext shortcutContext) :
    QObject(parent),
    sc_member(member),
    sc_ambiguousmember(ambiguousMember),
    sc_context(shortcutContext),
    sc_vec(QVector<QShortcut *>())
{
    setKeys(keys);
}

Shortcut::~Shortcut()
{
    for (auto *sc : sc_vec)
        delete sc;
}

void Shortcut::addKey(const QKeySequence &key) {
    sc_vec.append(new QShortcut(key, parentWidget(), sc_member, sc_ambiguousmember, sc_context));
}

void Shortcut::setKey(const QKeySequence &key) {
    if (sc_vec.isEmpty()) {
        addKey(key);
    } else {
        while (sc_vec.count() != 1)
            delete sc_vec.takeLast();
        sc_vec.first()->setKey(key);
    }
}

QKeySequence Shortcut::key() const {
    return sc_vec.first()->key();
}

void Shortcut::addKeys(const QList<QKeySequence> &keys) {
    for (auto key : keys)
        addKey(key);
}

void Shortcut::setKeys(const QList<QKeySequence> &keys) {
    if (keys.isEmpty())
        return;

    while (sc_vec.count() < keys.count())
        addKey(QKeySequence());
    
    while (sc_vec.count() > keys.count())
        delete sc_vec.takeLast();

    for (int i = 0; i < keys.count(); ++i)
        sc_vec[i]->setKey(keys[i]);
}

QList<QKeySequence> Shortcut::keys() const {
    QList<QKeySequence> ks_list = QList<QKeySequence>();
    for (auto *sc : sc_vec)
        ks_list.append(sc->key());
    return ks_list;
}

void Shortcut::setEnabled(bool enable) {
    for (auto *sc : sc_vec)
        sc->setEnabled(enable);
}

bool Shortcut::isEnabled() const {
    return sc_vec.first()->isEnabled();
}

void Shortcut::setContext(Qt::ShortcutContext context) {
    sc_context = context;
    for (auto *sc : sc_vec)
        sc->setContext(context);
}

Qt::ShortcutContext Shortcut::context() const {
    return sc_context;
}

void Shortcut::setWhatsThis(const QString &text) {
    for (auto *sc : sc_vec)
        sc->setWhatsThis(text);
}

QString Shortcut::whatsThis() const {
    return sc_vec.first()->whatsThis();
}

void Shortcut::setAutoRepeat(bool on) {
    for (auto *sc : sc_vec)
        sc->setAutoRepeat(on);
}

bool Shortcut::autoRepeat() const {
    return sc_vec.first()->autoRepeat();
}

int Shortcut::id() const {
    return sc_vec.first()->id();
}

QList<int> Shortcut::ids() const {
    QList<int> id_list;
    for (auto *sc : sc_vec)
        id_list.append(sc->id());
    return id_list;
}

bool Shortcut::event(QEvent *e) {
    if (isEnabled() && e->type() == QEvent::Shortcut) {
        auto se = static_cast<QShortcutEvent *>(e);
        if (ids().contains(se->shortcutId()) && keys().contains(se->key())) {
            if (QWhatsThis::inWhatsThisMode()) {
                QWhatsThis::showText(QCursor::pos(), whatsThis());
            } else {
                if (se->isAmbiguous())
                    emit activatedAmbiguously();
                else
                    emit activated();
            }
            return true;
        }
    }
    return false;
}
