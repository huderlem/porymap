#include "asm.h"

Asm::Asm()
{
}

void Asm::strip_comment(QString *line) {
    bool in_string = false;
    for (int i = 0; i < line->length(); i++) {
        if (line->at(i) == '"') {
            in_string = !in_string;
        } else if (line->at(i) == '@') {
            if (!in_string) {
                line->truncate(i);
                break;
            }
        }
    }
}

QList<QStringList>* Asm::parse(QString text) {
    QList<QStringList> *parsed = new QList<QStringList>;
    QStringList lines = text.split('\n');
    for (QString line : lines) {
        QString label;
        //QString macro;
        //QStringList *params;
        strip_comment(&line);
        if (line.isEmpty()) {
        } else if (line.contains(':')) {
            label = line.left(line.indexOf(':'));
            QStringList *list = new QStringList;
            list->append(".label"); // This is not a real keyword. It's used only to make the output more regular.
            list->append(label);
            parsed->append(*list);
            // There should not be anything else on the line.
            // gas will raise a syntax error if there is.
        } else {
            line = line.trimmed();
            //parsed->append(line.split(QRegExp("\\s*,\\s*")));
            QString macro;
            QStringList params;
            int index = line.indexOf(QRegExp("\\s+"));
            macro = line.left(index);
            params = line.right(line.length() - index).trimmed().split(QRegExp("\\s*,\\s*"));
            params.prepend(macro);
            parsed->append(params);
        }
        //if (macro != NULL) {
        //    if (macros->contains(macro)) {
        //        void* function = macros->value(macro);
        //        if (function != NULL) {
        //            std::function function(params);
        //        }
        //    }
        //}
    }
    return parsed;
}
