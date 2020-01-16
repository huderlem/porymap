#ifndef HISTORY_H
#define HISTORY_H

#include <QList>
#include <QString>
#include <QMap>

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

class HistoryCommand {
    //
// public methods
public:
    //
    explicit HistoryCommand();
    explicit HistoryCommand(QString text);
    virtual ~HistoryCommand();

    void setMessage(QString text);
    QString getMessage() const;

    virtual void execute();

// private methods
private:
    //

// public attributes
public:
    //

// private attributes
private:
    //
    QString message;

    friend class CommandStack;
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

// inherit public QObject
// for signals to emit when undo and redo text changes for the view to update?
// or update it elsehow?
class CommandStack {
    //
// public methods
public:
    //
    explicit CommandStack();
    ~CommandStack();

    void commit(HistoryCommand *command);// push?
    void undo();
    void redo();

    QString undoText() const;
    QString redoText() const;

    void enable();
    void disable();
    bool isActive() const;

    HistoryCommand *current();
    bool canUndo() const;
    bool canRedo() const;
    void clear();

    int count() const;

// private methods
private:
    // clear all after this one
    void clearAfter(int index);

// public attributes
public:
    //
    void printStack();

// private attributes
private:
    //
    QList<HistoryCommand *> history;

    int pos;
    int size;
    bool active = false;

    friend class HistoryManager;
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

class HistoryManager {
    //
// public methods
public:
    //
    explicit HistoryManager();
    ~HistoryManager();

    CommandStack *activeStack();

    void setActiveStack(CommandStack *);

    void undo();
    void redo();

// private methods
private:
    //

// public attributes
public:
    //

// private attributes
private:
    //
    CommandStack *active_stack;
    //QList<CommandStack *> command_stacks;
    QMap<QString, CommandStack *> command_stacks;
    // ^ map name as key
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */





































template <typename T>
class History {
public:
    History() { }
    T back() {
        if (head > 0) {
            return history.at(--head);
        }
        return NULL;
    }

    T next() {
        if (head + 1 < history.length()) {
            return history.at(++head);
        }
        return NULL;
    }

    void push(T commit) {
        while (head + 1 < history.length()) {
            T item = history.last();
            history.removeLast();
            delete item;
        }
        if (saved > head) {
            saved = -1;
        }
        history.append(commit);
        head++;
    }

    T current() {
        if (head < 0 || history.length() == 0) {
            return NULL;
        }
        return history.at(head);
    }

    void save() {
        saved = head;
    }

    bool isSaved() {
        return saved == head;
    }

private:
    QList<T> history;
    int head = -1;
    int saved = -1;
};

#endif // HISTORY_H
