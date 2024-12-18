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


    PORYMAP NOTE: Modified to support having the section expanded by default, to stop the contents
                  squashing during the collapse animation, and to add some guard rails against crashes.
*/

#ifndef COLLAPSIBLESECTION_H
#define COLLAPSIBLESECTION_H

#include <QFrame>
#include <QGridLayout>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QSet>
#include <QScrollArea>
#include <QToolButton>
#include <QWidget>

class CollapsibleSection : public QWidget
{
    Q_OBJECT

public:
    explicit CollapsibleSection(const QString& title = "",  const bool expanded = false, const int animationDuration = 0, QWidget* parent = 0);

    void setContentLayout(QLayout* contentLayout);
    void setTitle(const QString &title);
    bool isExpanded() const { return this->expanded; }

public slots:
    void toggle(bool collapsed);
    
private:
    QGridLayout* mainLayout;
    QToolButton* toggleButton;
    QFrame* headerLine;
    QParallelAnimationGroup* toggleAnimation;
    QSet<QPropertyAnimation*> sectionAnimations;
    QPropertyAnimation* contentAnimation;
    QScrollArea* contentArea;
    int animationDuration;
    int collapsedHeight;
    bool expanded;

    void updateToggleButton();
    void updateAnimationTargets();
    int getContentHeight() const;

};

#endif // COLLAPSIBLESECTION_H
