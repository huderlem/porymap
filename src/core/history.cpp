#include "history.h"

#include <QDebug>



HistoryCommand::HistoryCommand() {
    this->message = "";
}

HistoryCommand::HistoryCommand(QString text) {
    setMessage(text);
}

HistoryCommand::~HistoryCommand() {}

void HistoryCommand::setMessage(QString text) {
    this->message = text;
}

QString HistoryCommand::getMessage() const {
    return this->message;
}

void HistoryCommand::execute() {}



CommandStack::CommandStack() {
    this->pos = -1;
    this->size = -1;
}

CommandStack::~CommandStack() {
    clear();
}

void CommandStack::undo() {
    // want to execute the command before it / restore history to previous state?
    // but no not really... shit hold up thought in the morning
    // but maybe not shit because I can just have different stacks for every kind
    // of edit and just restore a previous state which will be the same as undoing
    // but does that make any sense how would that really work. UGHHHH
    // maybe call it restore vs execute or save an old and new copy (before / after)
    if (canUndo()) history.value(--pos)->execute();

    printStack();
}

void CommandStack::redo() {
    //
    if (canRedo()) history.value(++pos)->execute();

    printStack();
}

void CommandStack::printStack() {
    QString logText = QString("Current Command History:\n");
    int cur = 0;
    for (HistoryCommand *historyItem : this->history) {
        QString marker;
        if (cur++ == pos) marker = " >> ";
        else marker = "    ";
        logText += QString("%1%2\n").arg(marker).arg(historyItem->getMessage());
    }
    qDebug().noquote() << logText;
}

void CommandStack::commit(HistoryCommand *command) {
    // if we are not at top of the stack, clear ahead of current position
    if (pos != size - 1) {
        clearAfter(pos);
    }

    // insert new command into the stack at the end
    history.append(command);

    // update class attributes
    size = history.size();
    pos = size - 1;

    // testing purposes
    printStack();
}

void CommandStack::clearAfter(int index) {
    int current = size;

    while (--current > index && !history.isEmpty()) {
        HistoryCommand *command = history.takeLast();
        if (command) delete command;
    }

    size = history.size();
}

void CommandStack::clear() {
    clearAfter(-1);
}

bool CommandStack::canUndo() const {
    return (pos < 1 || !history.value(pos)) ? false : true;
}

bool CommandStack::canRedo() const {
    return ((pos < size - 1) && history.value(pos + 1)) ? true : false;
}

QString CommandStack::undoText() const {
    return canUndo() ? history.value(pos)->getMessage() : QString();
}

QString CommandStack::redoText() const {
    return canRedo() ? history.value(pos + 1)->getMessage() : QString();
}


