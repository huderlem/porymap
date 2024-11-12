/*
    Elypson/qt-collapsible-section
    (c) 2016 Michael A. Voelkel - michael.alexander.voelkel@gmail.com

    This file is part of Elypson/qt-collapsible section.

    Elypson/qt-collapsible-section is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Elypson/qt-collapsible-section is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Elypson/qt-collapsible-section. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COLLAPSIBLESECTION_H
#define COLLAPSIBLESECTION_H

#include <QFrame>
#include <QGridLayout>
#include <QParallelAnimationGroup>
#include <QScrollArea>
#include <QToolButton>
#include <QWidget>

class CollapsibleSection : public QWidget
{
    Q_OBJECT
    
private:
    QGridLayout* mainLayout;
    QToolButton* toggleButton;
    QFrame* headerLine;
    QParallelAnimationGroup* toggleAnimation;
    QScrollArea* contentArea;
    int animationDuration;
    int collapsedHeight;
    bool isExpanded = false;
    
public slots:
    void toggle(bool collapsed);

public:
    // initialize section
    explicit CollapsibleSection(const QString& title = "", const int animationDuration = 0, QWidget* parent = 0);

    // set layout of content
    void setContentLayout(QLayout& contentLayout);
    
    // set title
    void setTitle(QString title);
    
    // update animations and their heights
    void updateHeights();
};

#endif // COLLAPSIBLESECTION_H
