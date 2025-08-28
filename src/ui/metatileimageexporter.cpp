#include "metatileimageexporter.h"
#include "ui_metatileimageexporter.h"
#include "filedialog.h"
#include "imageproviders.h"
#include "utility.h"
#include "project.h"
#include "metatile.h"

#include <QTimer>

MetatileImageExporter::MetatileImageExporter(QWidget *parent, Tileset *primaryTileset, Tileset *secondaryTileset, Settings *savedSettings) :
    QDialog(parent),
    ui(new Ui::MetatileImageExporter),
    m_primaryTileset(primaryTileset),
    m_secondaryTileset(secondaryTileset),
    m_savedSettings(savedSettings)
{
    ui->setupUi(this);
    m_transparencyButtons = {
        ui->radioButton_TransparencyNormal,
        ui->radioButton_TransparencyBlack,
        ui->radioButton_TransparencyFirst,
    };

    m_scene = new CheckeredBgScene(QSize(8,8), this);
    m_preview = m_scene->addPixmap(QPixmap());
    ui->graphicsView_Preview->setScene(m_scene);

    if (projectConfig.tripleLayerMetatilesEnabled) {
        // When triple-layer metatiles are enabled there is no unused layer,
        // so this setting becomes pointless. Disable it.
        ui->checkBox_Placeholders->setVisible(false);
    }

    uint16_t maxMetatileId = Block::getMaxMetatileId();
    ui->spinBox_MetatileStart->setMaximum(maxMetatileId);
    ui->spinBox_MetatileEnd->setMaximum(maxMetatileId);
    ui->spinBox_WidthMetatiles->setRange(1, maxMetatileId);
    ui->spinBox_WidthPixels->setRange(1 * Metatile::pixelWidth(), maxMetatileId * Metatile::pixelWidth());

    if (m_primaryTileset) {
        ui->comboBox_PrimaryTileset->setTextItem(m_primaryTileset->name);
    }
    if (m_secondaryTileset) {
        ui->comboBox_SecondaryTileset->setTextItem(m_secondaryTileset->name);
    }

    if (m_savedSettings) {
        populate(*m_savedSettings);
    } else {
        populate({});
    }
    
    connect(ui->listWidget_Layers, &ReorderableListWidget::itemChanged, this, &MetatileImageExporter::updatePreview);
    connect(ui->listWidget_Layers, &ReorderableListWidget::reordered, this, &MetatileImageExporter::updatePreview);

    connect(ui->pushButton_Save,  &QPushButton::pressed, [this] { if (saveImage()) close(); });
    connect(ui->pushButton_Close, &QPushButton::pressed, this, &MetatileImageExporter::close);
    connect(ui->pushButton_Reset, &QPushButton::pressed, this, &MetatileImageExporter::reset);

    connect(ui->spinBox_WidthMetatiles, &UIntSpinBox::valueChanged, this, &MetatileImageExporter::syncPixelWidth);
    connect(ui->spinBox_WidthMetatiles, &UIntSpinBox::valueChanged, this, &MetatileImageExporter::queuePreviewUpdate);
    connect(ui->spinBox_WidthMetatiles, &UIntSpinBox::editingFinished, this, &MetatileImageExporter::tryUpdatePreview);

    connect(ui->spinBox_WidthPixels, &UIntSpinBox::valueChanged, this, &MetatileImageExporter::syncMetatileWidth);
    connect(ui->spinBox_WidthPixels, &UIntSpinBox::valueChanged, this, &MetatileImageExporter::queuePreviewUpdate);
    connect(ui->spinBox_WidthPixels, &UIntSpinBox::editingFinished, this, &MetatileImageExporter::syncPixelWidth); // Round pixel width to multiple of 16
    connect(ui->spinBox_WidthPixels, &UIntSpinBox::editingFinished, this, &MetatileImageExporter::tryUpdatePreview);

    connect(ui->spinBox_MetatileStart, &UIntHexSpinBox::valueChanged, this, &MetatileImageExporter::validateMetatileEnd);
    connect(ui->spinBox_MetatileStart, &UIntHexSpinBox::valueChanged, this, &MetatileImageExporter::queuePreviewUpdate);
    connect(ui->spinBox_MetatileStart, &UIntHexSpinBox::editingFinished, this, &MetatileImageExporter::tryUpdatePreview);

    connect(ui->spinBox_MetatileEnd, &UIntHexSpinBox::valueChanged, this, &MetatileImageExporter::validateMetatileStart);
    connect(ui->spinBox_MetatileEnd, &UIntHexSpinBox::valueChanged, this, &MetatileImageExporter::queuePreviewUpdate);
    connect(ui->spinBox_MetatileEnd, &UIntHexSpinBox::editingFinished, this, &MetatileImageExporter::tryUpdatePreview);

    // If we used toggled instead of clicked we'd get two preview updates instead of one when the setting changes.
    connect(ui->radioButton_TransparencyNormal, &QRadioButton::clicked, this, &MetatileImageExporter::updatePreview);
    connect(ui->radioButton_TransparencyBlack,  &QRadioButton::clicked, this, &MetatileImageExporter::updatePreview);
    connect(ui->radioButton_TransparencyFirst,  &QRadioButton::clicked, this, &MetatileImageExporter::updatePreview);

    connect(ui->checkBox_Placeholders,     &QCheckBox::toggled, this, &MetatileImageExporter::updatePreview);
    connect(ui->checkBox_PrimaryTileset,   &QCheckBox::toggled, this, &MetatileImageExporter::tryEnforceMetatileRange);
    connect(ui->checkBox_PrimaryTileset,   &QCheckBox::toggled, this, &MetatileImageExporter::updatePreview);
    connect(ui->checkBox_SecondaryTileset, &QCheckBox::toggled, this, &MetatileImageExporter::tryEnforceMetatileRange);
    connect(ui->checkBox_SecondaryTileset, &QCheckBox::toggled, this, &MetatileImageExporter::updatePreview);

    ui->graphicsView_Preview->setFocus();
}

MetatileImageExporter::~MetatileImageExporter() {
    delete ui;
}

// Allow the window to open before displaying the preview.
// Metatile sheet image creation is generally quick, so this only
// really matters so that the graphics view can adjust the scale properly.
void MetatileImageExporter::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (!event->spontaneous()) {
        QTimer::singleShot(0, this, &MetatileImageExporter::updatePreview);
    }
}

void MetatileImageExporter::closeEvent(QCloseEvent *event) {
    if (m_savedSettings) {
        m_savedSettings->metatileStart = ui->spinBox_MetatileStart->value();
        m_savedSettings->metatileEnd = ui->spinBox_MetatileEnd->value();
        m_savedSettings->numMetatilesWide = ui->spinBox_WidthMetatiles->value();
        m_savedSettings->usePrimaryTileset = ui->checkBox_PrimaryTileset->isChecked();
        m_savedSettings->useSecondaryTileset = ui->checkBox_SecondaryTileset->isChecked();
        m_savedSettings->renderPlaceholders = ui->checkBox_Placeholders->isChecked();
        for (int i = 0; i < m_transparencyButtons.length(); i++) {
            if (m_transparencyButtons.at(i)->isChecked()) {
                m_savedSettings->transparencyMode = i;
                break;
            }
        }
        m_savedSettings->layerOrder.clear();
        for (int i = 0; i < ui->listWidget_Layers->count(); i++) {
            auto item = ui->listWidget_Layers->item(i);
            int layerNum = item->data(Qt::UserRole).toInt();
            m_savedSettings->layerOrder[layerNum] = (item->checkState() == Qt::Checked);
        }
    }
    QDialog::closeEvent(event);
}

void MetatileImageExporter::populate(const Settings &settings) {
    const QSignalBlocker b_MetatileStart(ui->spinBox_MetatileStart);
    ui->spinBox_MetatileStart->setValue(settings.metatileStart);

    const QSignalBlocker b_MetatileEnd(ui->spinBox_MetatileStart);
    ui->spinBox_MetatileEnd->setValue(settings.metatileEnd);

    const QSignalBlocker b_WidthMetatiles(ui->spinBox_MetatileStart);
    ui->spinBox_WidthMetatiles->setValue(settings.numMetatilesWide);

    const QSignalBlocker b_WidthPixels(ui->spinBox_MetatileStart);
    ui->spinBox_WidthPixels->setValue(settings.numMetatilesWide * Metatile::pixelWidth());

    const QSignalBlocker b_PrimaryTileset(ui->spinBox_MetatileStart);
    ui->checkBox_PrimaryTileset->setChecked(settings.usePrimaryTileset);

    const QSignalBlocker b_SecondaryTileset(ui->spinBox_MetatileStart);
    ui->checkBox_SecondaryTileset->setChecked(settings.useSecondaryTileset);

    const QSignalBlocker b_Placeholders(ui->spinBox_MetatileStart);
    ui->checkBox_Placeholders->setChecked(settings.renderPlaceholders);

    if (m_transparencyButtons.value(settings.transparencyMode)) {
        auto button = m_transparencyButtons[settings.transparencyMode];
        const QSignalBlocker b_Transparency(button);
        button->setChecked(true);
    }

    // Build layer list from settings
    const QSignalBlocker b_Layers(ui->listWidget_Layers);
    ui->listWidget_Layers->clear();
    for (auto it = settings.layerOrder.cbegin(); it != settings.layerOrder.cend(); it++) {
        int layerNum = it.key();
        bool enabled = it.value();

        auto item = new QListWidgetItem(Metatile::getLayerName(layerNum));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren);
        item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, layerNum); // Save the original index to identify the layer
        ui->listWidget_Layers->addItem(item);
    }
    // Don't give extra unnecessary space to the list
    ui->listWidget_Layers->setFixedHeight(ui->listWidget_Layers->sizeHintForRow(0) * ui->listWidget_Layers->count() + 4);

    tryEnforceMetatileRange();
}

void MetatileImageExporter::applySettings(const Settings &settings) {
    populate(settings);
    updatePreview();
}

void MetatileImageExporter::reset() {
    applySettings({});
}

QImage MetatileImageExporter::getImage() {
    tryUpdatePreview();
    return m_previewImage;
}

bool MetatileImageExporter::saveImage(QString filepath) {
    tryUpdatePreview();
    if (filepath.isEmpty()) {
        QString defaultFilepath = QString("%1/%2").arg(FileDialog::getDirectory()).arg(getDefaultFileName());
        filepath = FileDialog::getSaveFileName(this, windowTitle(), defaultFilepath, QStringLiteral("Image Files (*.png *.jpg *.bmp)"));
        if (filepath.isEmpty()) {
            return false;
        }
    }
    return m_previewImage.save(filepath);
}

QString MetatileImageExporter::getDefaultFileName() const {
    if (m_layerOrder.length() == 1) {
        // Exporting a metatile layer image is an expected use case for Porytiles, which expects certain file names.
        // We can make the process a little easier by setting the default file name to those expected file names.
        static const QStringList layerFilenames = { "bottom", "middle", "top" };
        return layerFilenames.at(m_layerOrder.constFirst()) + ".png";
    }

    QString defaultFilename;
    if (ui->checkBox_PrimaryTileset->isChecked() && m_primaryTileset) {
        defaultFilename.append(QString("%1_").arg(Tileset::stripPrefix(m_primaryTileset->name)));
    }
    if (ui->checkBox_SecondaryTileset->isChecked() && m_secondaryTileset) {
        defaultFilename.append(QString("%1_").arg(Tileset::stripPrefix(m_secondaryTileset->name)));
    }
    if (!m_layerOrder.isEmpty() && m_layerOrder != QList<int>({0,1,2})) {
        for (int i = m_layerOrder.length() - 1; i >= 0; i--) {
            defaultFilename.append(Metatile::getLayerName(m_layerOrder.at(i)));
        }
        defaultFilename.append("_");
    }
    defaultFilename.append("Metatile");

    uint16_t start = ui->spinBox_MetatileStart->value();
    uint16_t end = ui->spinBox_MetatileEnd->value();
    if (start != end) {
        defaultFilename.append("s");
    }
    if (!ui->checkBox_PrimaryTileset->isChecked() && !ui->checkBox_SecondaryTileset->isChecked()) {
        defaultFilename.append(QString("_%1").arg(Metatile::getMetatileIdString(start)));
        if (start != end) {
            defaultFilename.append(QString("-%1").arg(Metatile::getMetatileIdString(end)));
        }
    }
    return QString("%1.png").arg(defaultFilename);
}

void MetatileImageExporter::queuePreviewUpdate() {
    m_previewUpdateQueued = true;
}

// For updating only when a change has been recorded.
// Useful for something that might happen often, like an input widget losing focus.
void MetatileImageExporter::tryUpdatePreview() {
    if (m_previewImage.isNull() || m_previewUpdateQueued) {
        updatePreview();
    }
}

void MetatileImageExporter::updatePreview() {
    copyRenderSettings();

    m_layerOrder.clear();
    for (int i = 0; i < ui->listWidget_Layers->count(); i++) {
        auto item = ui->listWidget_Layers->item(i);
        if (item->checkState() == Qt::Checked) {
            int layerNum = item->data(Qt::UserRole).toInt();
            m_layerOrder.prepend(qBound(0, layerNum, 2));
        }
    }

    if (ui->checkBox_PrimaryTileset->isChecked() && ui->checkBox_SecondaryTileset->isChecked()) {
        // Special behavior to combine the two tilesets while skipping the unused region between tilesets.
        m_previewImage = getMetatileSheetImage(m_primaryTileset,
                                             m_secondaryTileset,
                                             ui->spinBox_WidthMetatiles->value(),
                                             m_layerOrder);
    } else {
        m_previewImage = getMetatileSheetImage(m_primaryTileset,
                                             m_secondaryTileset,
                                             ui->spinBox_MetatileStart->value(),
                                             ui->spinBox_MetatileEnd->value(),
                                             ui->spinBox_WidthMetatiles->value(),
                                             m_layerOrder);
    }

    m_previewImage.setColorSpace(Util::toColorSpace(porymapConfig.imageExportColorSpaceId));
    m_preview->setPixmap(QPixmap::fromImage(m_previewImage));
    m_scene->setSceneRect(m_scene->itemsBoundingRect());
    m_previewUpdateQueued = false;

    restoreRenderSettings();
}

void MetatileImageExporter::validateMetatileStart() {
    const QSignalBlocker b(ui->spinBox_MetatileStart);
    ui->spinBox_MetatileStart->setValue(qMin(ui->spinBox_MetatileStart->value(),
                                             ui->spinBox_MetatileEnd->value()));
}

void MetatileImageExporter::validateMetatileEnd() {
    const QSignalBlocker b(ui->spinBox_MetatileEnd);
    ui->spinBox_MetatileEnd->setValue(qMax(ui->spinBox_MetatileStart->value(),
                                           ui->spinBox_MetatileEnd->value()));
}

void MetatileImageExporter::updateMetatileRange() {
    uint16_t min;
    uint16_t max;
    if (ui->checkBox_PrimaryTileset->isChecked() && m_primaryTileset) {
        if (ui->checkBox_SecondaryTileset->isChecked() && m_secondaryTileset) {
            // Both tilesets enforced
            min = qMin(m_primaryTileset->firstMetatileId(), m_secondaryTileset->firstMetatileId());
            max = qMax(m_primaryTileset->lastMetatileId(), m_secondaryTileset->lastMetatileId());
        } else {
            // Primary enforced
            min = m_primaryTileset->firstMetatileId();
            max = m_primaryTileset->lastMetatileId();
        }
    } else if (ui->checkBox_SecondaryTileset->isChecked() && m_secondaryTileset) {
        // Secondary enforced
        min = m_secondaryTileset->firstMetatileId();
        max = m_secondaryTileset->lastMetatileId();
    } else {
        // No tilesets enforced
        return;
    }

    const QSignalBlocker b_MetatileStart(ui->spinBox_MetatileStart);
    const QSignalBlocker b_MetatileEnd(ui->spinBox_MetatileEnd);
    ui->spinBox_MetatileStart->setValue(min);
    ui->spinBox_MetatileEnd->setValue(max);
}

void MetatileImageExporter::tryEnforceMetatileRange() {
    // Users can either specify which tileset(s) to render, or specify a range of metatiles, but not both.
    if (ui->checkBox_PrimaryTileset->isChecked() || ui->checkBox_SecondaryTileset->isChecked()) {
        updateMetatileRange();
        ui->groupBox_MetatileRange->setDisabled(true);
    } else {
        ui->groupBox_MetatileRange->setDisabled(false);
    }
}

void MetatileImageExporter::syncPixelWidth() {
    const QSignalBlocker b(ui->spinBox_WidthPixels);
    ui->spinBox_WidthPixels->setValue(ui->spinBox_WidthMetatiles->value() * Metatile::pixelWidth());
}

void MetatileImageExporter::syncMetatileWidth() {
    const QSignalBlocker b(ui->spinBox_WidthMetatiles);
    ui->spinBox_WidthMetatiles->setValue(Util::roundUpToMultiple(ui->spinBox_WidthPixels->value(), Metatile::pixelWidth()) / Metatile::pixelWidth());
}

// These settings control some rendering behavior that make metatiles render accurately to their in-game appearance,
// which may be undesirable when exporting metatile images for editing.
// The settings are buried in getMetatileImage at the moment, to change them we'll temporarily overwrite them.
void MetatileImageExporter::copyRenderSettings() {
    m_savedConfig.transparencyColor = projectConfig.transparencyColor;
    m_savedConfig.unusedTileNormal = projectConfig.unusedTileNormal;
    m_savedConfig.unusedTileCovered = projectConfig.unusedTileCovered;
    m_savedConfig.unusedTileSplit = projectConfig.unusedTileSplit;

    if (ui->radioButton_TransparencyNormal->isChecked()) {
        projectConfig.transparencyColor = QColor(Qt::transparent);
    } else if (ui->radioButton_TransparencyBlack->isChecked()) {
        projectConfig.transparencyColor = QColor(Qt::black);
    } else {
        projectConfig.transparencyColor = QColor();
    }

    if (!ui->checkBox_Placeholders->isChecked()) {
        projectConfig.unusedTileNormal = 0;
        projectConfig.unusedTileCovered = 0;
        projectConfig.unusedTileSplit = 0;
    }
}

void MetatileImageExporter::restoreRenderSettings() {
    projectConfig.transparencyColor = m_savedConfig.transparencyColor;
    projectConfig.unusedTileNormal = m_savedConfig.unusedTileNormal;
    projectConfig.unusedTileCovered = m_savedConfig.unusedTileCovered;
    projectConfig.unusedTileSplit = m_savedConfig.unusedTileSplit;
}
