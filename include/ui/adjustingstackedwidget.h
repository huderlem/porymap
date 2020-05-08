#ifndef ADJUSTINGSTACKEDWIDGET_H
#define ADJUSTINGSTACKEDWIDGET_H

#include <QStackedWidget>



class AdjustingStackedWidget : public QStackedWidget
{
    Q_OBJECT

public:
    AdjustingStackedWidget(QWidget *parent = nullptr) : QStackedWidget(parent) {}

    // override this to allow the stacked widget's current page to dictate size
    virtual void setCurrentIndex(int index) {
        QStackedWidget::setCurrentIndex(index);
        for (int i = 0; i < this->count(); ++i) {
            this->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        }
        this->widget(index)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);        
    }
};

#endif // ADJUSTINGSTACKEDWIDGET_H
