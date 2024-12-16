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
    connect(ui->spinBox_FloorNumber, QOverload<int>::of(&QSpinBox::valueChanged), this, &MapHeaderForm::onFloorNumberChanged);
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

    QStringList locations = project->mapSectionIdNames;
    locations.sort();
    const QSignalBlocker b_Locations(ui->comboBox_Location);
    ui->comboBox_Location->clear();
    ui->comboBox_Location->addItems(locations);

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

// Unlike other combo boxes in the map header form, locations can be added or removed externally.
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

    if (!m_header) {
        clear();
        return;
    }

    // If the MapHeader is changed externally (for example, with the scripting API) update the UI accordingly
    connect(m_header, &MapHeader::songChanged, this, &MapHeaderForm::setSong);
    connect(m_header, &MapHeader::locationChanged, this, &MapHeaderForm::setLocation);
    connect(m_header, &MapHeader::requiresFlashChanged, this, &MapHeaderForm::setRequiresFlash);
    connect(m_header, &MapHeader::weatherChanged, this, &MapHeaderForm::setWeather);
    connect(m_header, &MapHeader::typeChanged, this, &MapHeaderForm::setType);
    connect(m_header, &MapHeader::battleSceneChanged, this, &MapHeaderForm::setBattleScene);
    connect(m_header, &MapHeader::showsLocationNameChanged, this, &MapHeaderForm::setShowsLocationName);
    connect(m_header, &MapHeader::allowsRunningChanged, this, &MapHeaderForm::setAllowsRunning);
    connect(m_header, &MapHeader::allowsBikingChanged, this, &MapHeaderForm::setAllowsBiking);
    connect(m_header, &MapHeader::allowsEscapingChanged, this, &MapHeaderForm::setAllowsEscaping);
    connect(m_header, &MapHeader::floorNumberChanged, this, &MapHeaderForm::setFloorNumber);

    // Immediately update the UI to reflect the assigned MapHeader
    setHeaderData(*m_header);
}

void MapHeaderForm::clear() {
    m_header = nullptr;
    setHeaderData(MapHeader());
}

void MapHeaderForm::setHeaderData(const MapHeader &header) {
    setSong(header.song());
    setLocation(header.location());
    setRequiresFlash(header.requiresFlash());
    setWeather(header.weather());
    setType(header.type());
    setBattleScene(header.battleScene());
    setShowsLocationName(header.showsLocationName());
    setAllowsRunning(header.allowsRunning());
    setAllowsBiking(header.allowsBiking());
    setAllowsEscaping(header.allowsEscaping());
    setFloorNumber(header.floorNumber());
}

MapHeader MapHeaderForm::headerData() const {
    if (m_header)
        return *m_header;

    // Build header from UI
    MapHeader header;
    header.setSong(song());
    header.setLocation(location());
    header.setRequiresFlash(requiresFlash());
    header.setWeather(weather());
    header.setType(type());
    header.setBattleScene(battleScene());
    header.setShowsLocationName(showsLocationName());
    header.setAllowsRunning(allowsRunning());
    header.setAllowsBiking(allowsBiking());
    header.setAllowsEscaping(allowsEscaping());
    header.setFloorNumber(floorNumber());
    return header;
}

// Set data in UI
void MapHeaderForm::setSong(const QString &song) {                 ui->comboBox_Song->setCurrentText(song); }
void MapHeaderForm::setLocation(const QString &location) {         ui->comboBox_Location->setCurrentText(location); }
void MapHeaderForm::setRequiresFlash(bool requiresFlash) {         ui->checkBox_RequiresFlash->setChecked(requiresFlash); }
void MapHeaderForm::setWeather(const QString &weather) {           ui->comboBox_Weather->setCurrentText(weather); }
void MapHeaderForm::setType(const QString &type) {                 ui->comboBox_Type->setCurrentText(type); }
void MapHeaderForm::setBattleScene(const QString &battleScene) {   ui->comboBox_BattleScene->setCurrentText(battleScene); }
void MapHeaderForm::setShowsLocationName(bool showsLocationName) { ui->checkBox_ShowLocationName->setChecked(showsLocationName); }
void MapHeaderForm::setAllowsRunning(bool allowsRunning) {         ui->checkBox_AllowRunning->setChecked(allowsRunning); }
void MapHeaderForm::setAllowsBiking(bool allowsBiking) {           ui->checkBox_AllowBiking->setChecked(allowsBiking); }
void MapHeaderForm::setAllowsEscaping(bool allowsEscaping) {       ui->checkBox_AllowEscaping->setChecked(allowsEscaping); }
void MapHeaderForm::setFloorNumber(int floorNumber) {              ui->spinBox_FloorNumber->setValue(floorNumber); }

// Read data from UI
QString MapHeaderForm::song() const {           return ui->comboBox_Song->currentText(); }
QString MapHeaderForm::location() const {       return ui->comboBox_Location->currentText(); }
bool MapHeaderForm::requiresFlash() const {     return ui->checkBox_RequiresFlash->isChecked(); }
QString MapHeaderForm::weather() const {        return ui->comboBox_Weather->currentText(); }
QString MapHeaderForm::type() const {           return ui->comboBox_Type->currentText(); }
QString MapHeaderForm::battleScene() const {    return ui->comboBox_BattleScene->currentText(); }
bool MapHeaderForm::showsLocationName() const { return ui->checkBox_ShowLocationName->isChecked(); }
bool MapHeaderForm::allowsRunning() const {     return ui->checkBox_AllowRunning->isChecked(); }
bool MapHeaderForm::allowsBiking() const {      return ui->checkBox_AllowBiking->isChecked(); }
bool MapHeaderForm::allowsEscaping() const {    return ui->checkBox_AllowEscaping->isChecked(); }
int MapHeaderForm::floorNumber() const {        return ui->spinBox_FloorNumber->value(); }

// Send changes in UI to tracked MapHeader (if there is one)
void MapHeaderForm::onSongUpdated(const QString &song) {               if (m_header) m_header->setSong(song); }
void MapHeaderForm::onLocationChanged(const QString &location) {       if (m_header) m_header->setLocation(location); }
void MapHeaderForm::onWeatherChanged(const QString &weather) {         if (m_header) m_header->setWeather(weather); }
void MapHeaderForm::onTypeChanged(const QString &type) {               if (m_header) m_header->setType(type); }
void MapHeaderForm::onBattleSceneChanged(const QString &battleScene) { if (m_header) m_header->setBattleScene(battleScene); }
void MapHeaderForm::onRequiresFlashChanged(int selected) {             if (m_header) m_header->setRequiresFlash(selected == Qt::Checked); }
void MapHeaderForm::onShowLocationNameChanged(int selected) {          if (m_header) m_header->setShowsLocationName(selected == Qt::Checked); }
void MapHeaderForm::onAllowRunningChanged(int selected) {              if (m_header) m_header->setAllowsRunning(selected == Qt::Checked); }
void MapHeaderForm::onAllowBikingChanged(int selected) {               if (m_header) m_header->setAllowsBiking(selected == Qt::Checked); }
void MapHeaderForm::onAllowEscapingChanged(int selected) {             if (m_header) m_header->setAllowsEscaping(selected == Qt::Checked); }
void MapHeaderForm::onFloorNumberChanged(int offset) {                 if (m_header) m_header->setFloorNumber(offset); }
