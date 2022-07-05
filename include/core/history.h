#pragma once
#ifndef HISTORY_H
#define HISTORY_H

#include <QList>

template <typename T>
class History {
public:
    History() { }

    ~History() {
        while (!history.isEmpty()) {
            delete history.takeLast();
        }
    }

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
