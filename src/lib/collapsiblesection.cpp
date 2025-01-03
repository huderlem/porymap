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

#include "collapsiblesection.h"
CollapsibleSection::CollapsibleSection(const QString& title, const bool expanded, const int animationDuration, QWidget* parent)
    : QWidget(parent), animationDuration(animationDuration), expanded(expanded)
{
    toggleButton = new QToolButton(this);
    headerLine = new QFrame(this);
    toggleAnimation = new QParallelAnimationGroup(this);
    contentArea = new QScrollArea(this);
    mainLayout = new QGridLayout(this);

    toggleButton->setStyleSheet("QToolButton {border: none;}");
    toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggleButton->setText(title);
    toggleButton->setCheckable(true);
    updateToggleButton();

    headerLine->setFrameShape(QFrame::HLine);
    headerLine->setFrameShadow(QFrame::Sunken);
    headerLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    contentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    contentArea->setMaximumHeight(0);
    contentArea->setMinimumHeight(0);

    sectionAnimations.insert(new QPropertyAnimation(this, "minimumHeight"));
    sectionAnimations.insert(new QPropertyAnimation(this, "maximumHeight"));
    for (const auto &anim : sectionAnimations)
        toggleAnimation->addAnimation(anim);
    contentAnimation = new QPropertyAnimation(contentArea, "maximumHeight");
    toggleAnimation->addAnimation(contentAnimation);

    mainLayout->setVerticalSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    int row = 0;
    mainLayout->addWidget(toggleButton, row, 0, 1, 1, Qt::AlignLeft);
    mainLayout->addWidget(headerLine, row++, 2, 1, 1);
    mainLayout->addWidget(contentArea, row, 0, 1, 3);
    setLayout(mainLayout);

    connect(toggleButton, &QToolButton::toggled, this, &CollapsibleSection::toggle);
}

void CollapsibleSection::updateToggleButton()
{
    toggleButton->setChecked(this->expanded);
    toggleButton->setArrowType(this->expanded ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
}

void CollapsibleSection::toggle(bool expand)
{
    if (toggleAnimation->state() != QAbstractAnimation::Stopped)
        return;
    if (this->expanded == expand)
        return;
    this->expanded = expand;

    updateToggleButton();

    if (expand) {
        // Opening animation. Set the contents to their maximum size immediately,
        // and they will be revealed slowly by the section animation.
        int contentHeight = getContentHeight();
        contentArea->setMinimumHeight(contentHeight);
        contentArea->setMaximumHeight(contentHeight);
        toggleAnimation->setDirection(QAbstractAnimation::Forward);
    } else {
        // Closing animation. Keep the contents at their current size, allowing
        // them to be hidden slowly by the section animation, then change their size
        // once the animation is complete so they aren't visible just below the title.
        auto ctx = new QObject();
        connect(toggleAnimation, &QAbstractAnimation::finished, ctx, [this, ctx]() {
            // This is a single-shot connection. Qt6 has built-in support for this kind of thing.
            contentArea->setMinimumHeight(0);
            contentArea->setMaximumHeight(0);
            ctx->deleteLater();
        });
        toggleAnimation->setDirection(QAbstractAnimation::Backward);
    }
    toggleAnimation->start();
}

void CollapsibleSection::setContentLayout(QLayout* contentLayout)
{
    if (contentArea->layout() == contentLayout)
        return;
    delete contentArea->layout();
    contentArea->setLayout(contentLayout);
    collapsedHeight = sizeHint().height() - contentArea->maximumHeight();

    int contentHeight = this->expanded ? getContentHeight() : 0;
    contentArea->setMinimumHeight(contentHeight);
    contentArea->setMaximumHeight(contentHeight);

    updateAnimationTargets();
}

void CollapsibleSection::setTitle(const QString &title)
{
    toggleButton->setText(title);
}

int CollapsibleSection::getContentHeight() const
{
    return contentArea->layout() ? contentArea->layout()->sizeHint().height() : 0;
}

void CollapsibleSection::updateAnimationTargets()
{
    const int contentHeight = getContentHeight();
    for (auto anim : sectionAnimations) {
        anim->setDuration(animationDuration);
        anim->setStartValue(collapsedHeight);
        anim->setEndValue(collapsedHeight + contentHeight);
    }
    contentAnimation->setDuration(animationDuration);
    contentAnimation->setStartValue(0);
    contentAnimation->setEndValue(contentHeight);
}
