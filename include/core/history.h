#pragma once
#ifndef HISTORY_H
#define HISTORY_H

#include <QList>

template <typename T>
class History {
public:
    History() { }

    ~History() {
        clear();
    }

    void clear() {
        while (!history.isEmpty()) {
            delete history.takeLast();
        }
        head = -1;
        saved = -1;
    }

    T back() {
        if (head > 0) {
            return history.at(--head);
        }
        head = -1;
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
            delete history.takeLast();
        }
        if (saved > head) {
            saved = -1;
        }
        history.append(commit);
        head++;
    }

    T current() const {
        if (head < 0 || history.length() == 0) {
            return NULL;
        }
        return history.at(head);
    }

    void save() {
        saved = head;
    }

    bool isSaved() const {
        return saved == head;
    }

    int length() const {
        return history.length();
    }

    bool isEmpty() const {
        return history.isEmpty();
    }

    int index() const {
        return head;
    }

    bool canUndo() const {
        return head >= 0;
    }

    bool canRedo() const {
        return (head + 1) < history.length();
    }

private:
    QList<T> history;
    int head = -1;
    int saved = -1;
};

#endif // HISTORY_H
