#include "mapheaderform.h"
#include "ui_mapheaderform.h"
#include "project.h"

MapHeaderForm::MapHeaderForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MapHeaderForm)
{
    ui->setupUi(this);

    // This value is an s8 by default, but we don't need to unnecessarily limit users.
    ui->spinBox_FloorNumber->setMinimum(INT_MIN);
    ui->spinBox_FloorNumber->setMaximum(INT_MAX);

    // When the UI is updated, sync those changes to the tracked MapHeader (if there is one)
    connect(ui->comboBox_Song,        &QComboBox::currentTextChanged, this, &MapHeaderForm::onSongUpdated);
    connect(ui->comboBox_Location,    &QComboBox::currentTextChanged, this, &MapHeaderForm::onLocationChanged);
    connect(ui->comboBox_Weather,     &QComboBox::currentTextChanged, this, &MapHeaderForm::onWeatherChanged);
    connect(ui->comboBox_Type,        &QComboBox::currentTextChanged, this, &MapHeaderForm::onTypeChanged);
    connect(ui->comboBox_BattleScene, &QComboBox::currentTextChanged, this, &MapHeaderForm::onBattleSceneChanged);
    connect(ui->checkBox_RequiresFlash,    &QCheckBox::stateChanged, this, &MapHeaderForm::onRequiresFlashChanged);
    connect(ui->checkBox_ShowLocationName, &QCheckBox::stateChanged, this, &MapHeaderForm::onShowLocationNameChanged);
    connect(ui->checkBox_AllowRunning,     &QCheckBox::stateChanged, this, &MapHeaderForm::onAllowRunningChanged);
    connect(ui->checkBox_AllowBiking,      &QCheckBox::stateChanged, this, &MapHeaderForm::onAllowBikingChanged);
    connect(ui->checkBox_AllowEscaping,    &QCheckBox::stateChanged, this, &MapHeaderForm::onAllowEscapingChanged);
    connect(ui->spinBox_FloorNumber, &QSpinBox::valueChanged, this, &MapHeaderForm::onFloorNumberChanged);
}

MapHeaderForm::~MapHeaderForm()
{
    delete ui;
}

void MapHeaderForm::init(const Project * project) {
    clear();

    if (!project)
        return;

    // Populate combo boxes

    const QSignalBlocker b_Song(ui->comboBox_Song);
    ui->comboBox_Song->clear();
    ui->comboBox_Song->addItems(project->songNames);

    const QSignalBlocker b_Weather(ui->comboBox_Weather);
    ui->comboBox_Weather->clear();
    ui->comboBox_Weather->addItems(project->weatherNames);

    const QSignalBlocker b_Type(ui->comboBox_Type);
    ui->comboBox_Type->clear();
    ui->comboBox_Type->addItems(project->mapTypes);

    const QSignalBlocker b_BattleScene(ui->comboBox_BattleScene);
    ui->comboBox_BattleScene->clear();
    ui->comboBox_BattleScene->addItems(project->mapBattleScenes);

    setLocations(project->mapSectionIdNames);

    // Hide config-specific settings

    bool hasFlags = projectConfig.mapAllowFlagsEnabled;
    ui->checkBox_AllowRunning->setVisible(hasFlags);
    ui->checkBox_AllowBiking->setVisible(hasFlags);
    ui->checkBox_AllowEscaping->setVisible(hasFlags);
    ui->label_AllowRunning->setVisible(hasFlags);
    ui->label_AllowBiking->setVisible(hasFlags);
    ui->label_AllowEscaping->setVisible(hasFlags);

    bool floorNumEnabled = projectConfig.floorNumberEnabled;
    ui->spinBox_FloorNumber->setVisible(floorNumEnabled);
    ui->label_FloorNumber->setVisible(floorNumEnabled);
}

// This combo box is treated specially because (unlike the other combo boxes)
// items that should be in this drop-down can be added or removed externally.
void MapHeaderForm::setLocations(QStringList locations) {
    locations.sort();

    const QSignalBlocker b(ui->comboBox_Location);
    const QString before = ui->comboBox_Location->currentText();
    ui->comboBox_Location->clear();
    ui->comboBox_Location->addItems(locations);
    ui->comboBox_Location->setCurrentText(before);
}

// Assign a MapHeader that the form will keep in sync with the UI.
void MapHeaderForm::setHeader(MapHeader *header) {
    if (m_header == header)
        return;

    if (m_header) {
        m_header->disconnect(this);
    }

    m_header = header;

    if (m_header) {
        // If the MapHeader is changed externally (for example, with the scripting API) update the UI accordingly
        connect(m_header, &MapHeader::songChanged, this, &MapHeaderForm::updateSong);
        connect(m_header, &MapHeader::locationChanged, this, &MapHeaderForm::updateLocation);
        connect(m_header, &MapHeader::requiresFlashChanged, this, &MapHeaderForm::updateRequiresFlash);
        connect(m_header, &MapHeader::weatherChanged, this, &MapHeaderForm::updateWeather);
        connect(m_header, &MapHeader::typeChanged, this, &MapHeaderForm::updateType);
        connect(m_header, &MapHeader::battleSceneChanged, this, &MapHeaderForm::updateBattleScene);
        connect(m_header, &MapHeader::showsLocationNameChanged, this, &MapHeaderForm::updateShowsLocationName);
        connect(m_header, &MapHeader::allowsRunningChanged, this, &MapHeaderForm::updateAllowsRunning);
        connect(m_header, &MapHeader::allowsBikingChanged, this, &MapHeaderForm::updateAllowsBiking);
        connect(m_header, &MapHeader::allowsEscapingChanged, this, &MapHeaderForm::updateAllowsEscaping);
        connect(m_header, &MapHeader::floorNumberChanged, this, &MapHeaderForm::updateFloorNumber);
    }

    // Immediately update the UI to reflect the assigned MapHeader
    updateUi();
}

void MapHeaderForm::clear() {
    m_header = nullptr;
    updateUi();
}

void MapHeaderForm::updateUi() {
    updateSong();
    updateLocation();
    updateRequiresFlash();
    updateWeather();
    updateType();
    updateBattleScene();
    updateShowsLocationName();
    updateAllowsRunning();
    updateAllowsBiking();
    updateAllowsEscaping();
    updateFloorNumber();

}

MapHeader MapHeaderForm::headerData() const {
    if (m_header)
        return *m_header;

    // Build header from UI
    MapHeader header;
    header.setSong(ui->comboBox_Song->currentText());
    header.setLocation(ui->comboBox_Location->currentText());
    header.setRequiresFlash(ui->checkBox_RequiresFlash->isChecked());
    header.setWeather(ui->comboBox_Weather->currentText());
    header.setType(ui->comboBox_Type->currentText());
    header.setBattleScene(ui->comboBox_BattleScene->currentText());
    header.setShowsLocationName(ui->checkBox_ShowLocationName->isChecked());
    header.setAllowsRunning(ui->checkBox_AllowRunning->isChecked());
    header.setAllowsBiking(ui->checkBox_AllowBiking->isChecked());
    header.setAllowsEscaping(ui->checkBox_AllowEscaping->isChecked());
    header.setFloorNumber(ui->spinBox_FloorNumber->value());
    return header;
}

void MapHeaderForm::setLocationDisabled(bool disabled) {
    m_locationDisabled = disabled;
    ui->label_Location->setDisabled(m_locationDisabled);
    ui->comboBox_Location->setDisabled(m_locationDisabled);
}

void MapHeaderForm::updateSong() {
    const QSignalBlocker b(ui->comboBox_Song);
    ui->comboBox_Song->setCurrentText(m_header ? m_header->song() : QString());
}

void MapHeaderForm::updateLocation() {
    const QSignalBlocker b(ui->comboBox_Location);
    ui->comboBox_Location->setCurrentText(m_header ? m_header->location() : QString());
}

void MapHeaderForm::updateRequiresFlash() {
    const QSignalBlocker b(ui->checkBox_RequiresFlash);
    ui->checkBox_RequiresFlash->setChecked(m_header ? m_header->requiresFlash() : false);
}

void MapHeaderForm::updateWeather() {
    const QSignalBlocker b(ui->comboBox_Weather);
    ui->comboBox_Weather->setCurrentText(m_header ? m_header->weather() : QString());
}

void MapHeaderForm::updateType() {
    const QSignalBlocker b(ui->comboBox_Type);
    ui->comboBox_Type->setCurrentText(m_header ? m_header->type() : QString());
}

void MapHeaderForm::updateBattleScene() {
    const QSignalBlocker b(ui->comboBox_BattleScene);
    ui->comboBox_BattleScene->setCurrentText(m_header ? m_header->battleScene() : QString());
}

void MapHeaderForm::updateShowsLocationName() {
    const QSignalBlocker b(ui->checkBox_ShowLocationName);
    ui->checkBox_ShowLocationName->setChecked(m_header ? m_header->showsLocationName() : false);
}

void MapHeaderForm::updateAllowsRunning() {
    const QSignalBlocker b(ui->checkBox_AllowRunning);
    ui->checkBox_AllowRunning->setChecked(m_header ? m_header->allowsRunning() : false);
}

void MapHeaderForm::updateAllowsBiking() {
    const QSignalBlocker b(ui->checkBox_AllowBiking);
    ui->checkBox_AllowBiking->setChecked(m_header ? m_header->allowsBiking() : false);
}

void MapHeaderForm::updateAllowsEscaping() {
    const QSignalBlocker b(ui->checkBox_AllowEscaping);
    ui->checkBox_AllowEscaping->setChecked(m_header ? m_header->allowsEscaping() : false);
}

void MapHeaderForm::updateFloorNumber() {
    const QSignalBlocker b(ui->spinBox_FloorNumber);
    ui->spinBox_FloorNumber->setValue(m_header ? m_header->floorNumber() : 0);
}

void MapHeaderForm::onSongUpdated(const QString &song)
{
    if (m_header) m_header->setSong(song);
}

void MapHeaderForm::onLocationChanged(const QString &location)
{
    if (m_header) m_header->setLocation(location);
}

void MapHeaderForm::onWeatherChanged(const QString &weather)
{
    if (m_header) m_header->setWeather(weather);
}

void MapHeaderForm::onTypeChanged(const QString &type)
{
    if (m_header) m_header->setType(type);
}

void MapHeaderForm::onBattleSceneChanged(const QString &battleScene)
{
    if (m_header) m_header->setBattleScene(battleScene);
}

void MapHeaderForm::onRequiresFlashChanged(int selected)
{
    if (m_header) m_header->setRequiresFlash(selected == Qt::Checked);
}

void MapHeaderForm::onShowLocationNameChanged(int selected)
{
    if (m_header) m_header->setShowsLocationName(selected == Qt::Checked);
}

void MapHeaderForm::onAllowRunningChanged(int selected)
{
    if (m_header) m_header->setAllowsRunning(selected == Qt::Checked);
}

void MapHeaderForm::onAllowBikingChanged(int selected)
{
    if (m_header) m_header->setAllowsBiking(selected == Qt::Checked);
}

void MapHeaderForm::onAllowEscapingChanged(int selected)
{
    if (m_header) m_header->setAllowsEscaping(selected == Qt::Checked);
}

void MapHeaderForm::onFloorNumberChanged(int offset)
{
    if (m_header) m_header->setFloorNumber(offset);
}
