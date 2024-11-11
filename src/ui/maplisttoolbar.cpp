#include "maplisttoolbar.h"
#include "ui_maplisttoolbar.h"
#include "editor.h"

#include <QToolTip>

/*
    TODO: The button states for each tool bar (just the two toggleable buttons, hide empty folders and allow editing)
          should be saved in the config. This will be cleaner/easier once the config is JSON, so holding off on that for now.
*/

MapListToolBar::MapListToolBar(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::MapListToolBar)
{
    ui->setupUi(this);

    ui->button_ToggleEmptyFolders->setChecked(!m_emptyFoldersVisible);
    ui->button_ToggleEdit->setChecked(m_editsAllowed);

    connect(ui->button_AddFolder, &QAbstractButton::clicked, this, &MapListToolBar::addFolderClicked); // TODO: Tool tip
    connect(ui->button_ExpandAll, &QAbstractButton::clicked, this, &MapListToolBar::expandList);
    connect(ui->button_CollapseAll, &QAbstractButton::clicked, this, &MapListToolBar::collapseList);
    connect(ui->button_ToggleEdit, &QAbstractButton::clicked, this, &MapListToolBar::toggleEditsAllowed);
    connect(ui->lineEdit_filterBox, &QLineEdit::textChanged, this, &MapListToolBar::applyFilter);
    connect(ui->button_ToggleEmptyFolders, &QAbstractButton::clicked, [this] {
        toggleEmptyFolders();

        // Display message to let user know what just happened (if there are no empty folders visible it's not obvious).
        const QString message = QString("%1 empty folders!").arg(m_emptyFoldersVisible ? "Showing" : "Hiding");
        QToolTip::showText(ui->button_ToggleEmptyFolders->mapToGlobal(QPoint(0, 0)), message);
    });
}

MapListToolBar::~MapListToolBar()
{
    delete ui;
}

void MapListToolBar::setList(MapTree *list) {
    m_list = list;

    // Sync list with current settings
    setEditsAllowed(m_editsAllowed);
    setEmptyFoldersVisible(m_emptyFoldersVisible);
}

void MapListToolBar::setEditsAllowedButtonVisible(bool visible) {
    ui->button_ToggleEdit->setVisible(visible);
}

void MapListToolBar::toggleEditsAllowed() {
    if (m_list) {
        m_list->clearSelection();
    }
    setEditsAllowed(!m_editsAllowed);
}

void MapListToolBar::setEditsAllowed(bool allowed) {
     m_editsAllowed = allowed;

     const QSignalBlocker b(ui->button_ToggleEdit);
     ui->button_ToggleEdit->setChecked(allowed);

    if (!m_list)
        return;

    if (allowed) {
        m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_list->setDragEnabled(true);
        m_list->setAcceptDrops(true);
        m_list->setDropIndicatorShown(true);
        m_list->setDragDropMode(QAbstractItemView::InternalMove);
        m_list->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    } else {
        m_list->setSelectionMode(QAbstractItemView::NoSelection);
        m_list->setDragEnabled(false);
        m_list->setAcceptDrops(false);
        m_list->setDropIndicatorShown(false);
        m_list->setDragDropMode(QAbstractItemView::NoDragDrop);
        m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
}

void MapListToolBar::toggleEmptyFolders() {
    setEmptyFoldersVisible(!m_emptyFoldersVisible);
}

void MapListToolBar::setEmptyFoldersVisible(bool visible) {
    m_emptyFoldersVisible = visible;

    if (m_list) {
        auto model = static_cast<FilterChildrenProxyModel*>(m_list->model());
        if (model) {
            model->setHideEmpty(!visible);
            model->setFilterRegularExpression(ui->lineEdit_filterBox->text());
        }
    }

    // Update tool tip to reflect what will happen if the button is pressed.
    const QString toolTip = QString("%1 empty folders in the list.").arg(visible ? "Hide" : "Show");
    ui->button_ToggleEmptyFolders->setToolTip(toolTip);

    const QSignalBlocker b(ui->button_ToggleEmptyFolders);
    ui->button_ToggleEmptyFolders->setChecked(!visible);
}

void MapListToolBar::expandList() {
    if (m_list)
        m_list->expandToDepth(0);
}

void MapListToolBar::collapseList() {
    if (m_list) {
        m_list->collapseAll();
    }
}

void MapListToolBar::applyFilter(const QString &filterText) {
    if (m_filterLocked)
        return;

    const QSignalBlocker b(ui->lineEdit_filterBox);
    ui->lineEdit_filterBox->setText(filterText);

    if (m_list) {
        auto model = static_cast<FilterChildrenProxyModel*>(m_list->model());
        if (model) model->setFilterRegularExpression(QRegularExpression(filterText, QRegularExpression::CaseInsensitiveOption));

        if (filterText.isEmpty()) {
            m_list->collapseAll();
            emit filterCleared(m_list);
        } else {
            m_list->expandToDepth(0);
        }
    }
}

void MapListToolBar::clearFilter() {
    applyFilter("");
}
