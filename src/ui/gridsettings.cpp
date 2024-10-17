#include "ui_gridsettingsdialog.h"
#include "gridsettings.h"

// TODO: Save settings in config

const QMap<GridSettings::Style, QString> GridSettings::styleToName = {
    {Style::Solid, "Solid"},
    {Style::LargeDashes, "Large Dashes"},
    {Style::SmallDashes, "Small Dashes"},
    {Style::Crosshairs, "Crosshairs"},
    {Style::Dots, "Dots"},
};

QString GridSettings::getStyleName(GridSettings::Style style) {
    return styleToName.value(style);
}

GridSettings::Style GridSettings::getStyleFromName(const QString &name) {
    return styleToName.key(name, GridSettings::Style::Solid);
}

// We do some extra work here to A: try and center the dashes away from the intersections, and B: keep the dash pattern's total
// length equal to the length of a grid square. This keeps the patterns looking reasonable regardless of the grid size.
// Otherwise, the dashes can start to intersect in weird ways and create grid patterns that don't look like a rectangular grid.
QVector<qreal> GridSettings::getCenteredDashPattern(uint length, qreal dashLength, qreal gapLength) const {
    const qreal minEdgesLength = 0.6*2;
    if (length <= dashLength + minEdgesLength)
        return {dashLength};

    // Every dash after the first one needs to have room for a 'gapLength' segment.
    const int numDashes = 1 + ((length - minEdgesLength) - dashLength) / (dashLength + gapLength);

    // Total length of the pattern excluding the centering edges. There are always 1 fewer gap segments than dashes.
    const qreal mainLength = (dashLength * numDashes) + (gapLength * (numDashes-1));

    const qreal edgeLength = (length - mainLength) / 2;

    // Fill the pattern
    QVector<qreal> pattern = {0, edgeLength};
    for (int i = 0; i < numDashes-1; i++) {
        pattern.append(dashLength);
        pattern.append(gapLength);
    }
    pattern.append(dashLength);
    pattern.append(edgeLength);

    return pattern;
}

QVector<qreal> GridSettings::getDashPattern(uint length) const {
    switch (this->style) {

    // Equivalent to setting Qt::PenStyle::Solid with no dash pattern.
    case Style::Solid: return {1, 0};

    // Roughly equivalent to Qt::PenStyle::DashLine but with centering.
    case Style::LargeDashes: return getCenteredDashPattern(length, 3.0, 2.0);

    // Roughly equivalent to Qt::PenStyle::DotLine but with centering.
    case Style::SmallDashes: return getCenteredDashPattern(length, 1.0, 2.5);

    // Dashes only at intersections, in the shape of a crosshair.
    case Style::Crosshairs: {
        const qreal crosshairLength = 2.0;
        return {crosshairLength / 2, length - crosshairLength, crosshairLength / 2, 0};
    }

    // Dots only at intersections.
    case Style::Dots: {
        const qreal dotLength = 0.1;
        return {dotLength, length - dotLength};
    }

    // Invalid
    default: return {};
    }
}



GridSettingsDialog::GridSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GridSettingsDialog),
    m_settings(new GridSettings),
    m_originalSettings(*m_settings)
{
    m_ownedSettings = true;
    init();
}

GridSettingsDialog::GridSettingsDialog(GridSettings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GridSettingsDialog),
    m_settings(settings),
    m_originalSettings(*settings)
{
    m_ownedSettings = false;
    init();
}

void GridSettingsDialog::init() {
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    // Populate the styles combo box
    const QSignalBlocker b_Style(ui->comboBox_Style);
    ui->comboBox_Style->addItem(GridSettings::getStyleName(GridSettings::Style::Solid));
    ui->comboBox_Style->addItem(GridSettings::getStyleName(GridSettings::Style::LargeDashes));
    ui->comboBox_Style->addItem(GridSettings::getStyleName(GridSettings::Style::SmallDashes));
    ui->comboBox_Style->addItem(GridSettings::getStyleName(GridSettings::Style::Crosshairs));
    ui->comboBox_Style->addItem(GridSettings::getStyleName(GridSettings::Style::Dots));

    ui->button_LinkDimensions->setChecked(m_dimensionsLinked);
    ui->button_LinkOffsets->setChecked(m_offsetsLinked);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &GridSettingsDialog::dialogButtonClicked);
    connect(ui->button_LinkDimensions, &QAbstractButton::toggled, [this](bool on) { m_dimensionsLinked = on; });
    connect(ui->button_LinkOffsets, &QAbstractButton::toggled, [this](bool on) { m_offsetsLinked = on; });
    connect(ui->colorInput, &ColorInputWidget::colorChanged, this, &GridSettingsDialog::onColorChanged);

    updateInput();
}

GridSettingsDialog::~GridSettingsDialog() {
    delete ui;
    if (m_ownedSettings)
        delete m_settings;
}

void GridSettingsDialog::setSettings(const GridSettings &settings) {
    if (*m_settings == settings)
        return;
    *m_settings = settings;
    updateInput();
    emit changedGridSettings();
}

void GridSettingsDialog::updateInput() {
    setWidth(m_settings->width);
    setHeight(m_settings->height);
    setOffsetX(m_settings->offsetX);
    setOffsetY(m_settings->offsetY);

    const QSignalBlocker b_Color(ui->colorInput);
    ui->colorInput->setColor(m_settings->color.rgb());

    const QSignalBlocker b_Style(ui->comboBox_Style);
    ui->comboBox_Style->setCurrentText(GridSettings::getStyleName(m_settings->style));
}

void GridSettingsDialog::setWidth(int value) {
    const QSignalBlocker b(ui->spinBox_Width);
    ui->spinBox_Width->setValue(value);
    m_settings->width = value;
}

void GridSettingsDialog::setHeight(int value) {
    const QSignalBlocker b(ui->spinBox_Height);
    ui->spinBox_Height->setValue(value);
    m_settings->height = value;
}

void GridSettingsDialog::setOffsetX(int value) {
    const QSignalBlocker b(ui->spinBox_X);
    ui->spinBox_X->setValue(value);
    m_settings->offsetX = value;
}

void GridSettingsDialog::setOffsetY(int value) {
    const QSignalBlocker b(ui->spinBox_Y);
    ui->spinBox_Y->setValue(value);
    m_settings->offsetY = value;
}

void GridSettingsDialog::on_spinBox_Width_valueChanged(int value) {
    setWidth(value);
    if (m_dimensionsLinked)
        setHeight(value);

    emit changedGridSettings();
}

void GridSettingsDialog::on_spinBox_Height_valueChanged(int value) {
    setHeight(value);
    if (m_dimensionsLinked)
        setWidth(value);

    emit changedGridSettings();
}

void GridSettingsDialog::on_spinBox_X_valueChanged(int value) {
    setOffsetX(value);
    if (m_offsetsLinked)
        setOffsetY(value);

    emit changedGridSettings();
}

void GridSettingsDialog::on_spinBox_Y_valueChanged(int value) {
    setOffsetY(value);
    if (m_offsetsLinked)
        setOffsetX(value);

    emit changedGridSettings();
}

void GridSettingsDialog::on_comboBox_Style_currentTextChanged(const QString &text) {
    m_settings->style = GridSettings::getStyleFromName(text);
    emit changedGridSettings();
}

void GridSettingsDialog::onColorChanged(QRgb color) {
    m_settings->color = QColor::fromRgb(color);
    emit changedGridSettings();
}

void GridSettingsDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::AcceptRole) {
        // "OK"
        close();
    } else if (role == QDialogButtonBox::RejectRole) {
        // "Cancel"
        setSettings(m_originalSettings);
        close();
    } else if (role == QDialogButtonBox::ResetRole) {
        // "Restore Defaults"
        setSettings(m_defaultSettings);
    }
}
