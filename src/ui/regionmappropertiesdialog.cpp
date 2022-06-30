#include "project.h"
#include "regionmappropertiesdialog.h"
#include "ui_regionmappropertiesdialog.h"

RegionMapPropertiesDialog::RegionMapPropertiesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegionMapPropertiesDialog)
{
    ui->setupUi(this);
    this->hideMessages();
}

RegionMapPropertiesDialog::~RegionMapPropertiesDialog()
{
    delete ui;
}

void RegionMapPropertiesDialog::hideMessages() {
    ui->message_alias->setVisible(false);
    ui->message_tilemapFormat->setVisible(false);
    ui->message_tilemapWidth->setVisible(false);
    ui->message_tilemapHeight->setVisible(false);
    ui->message_tilemapImagePath->setVisible(false);
    ui->message_tilemapBinPath->setVisible(false);
    ui->message_layoutFormat->setVisible(false);
    ui->message_layoutPath->setVisible(false);
    ui->message_layoutWidth->setVisible(false);
    ui->message_layoutHeight->setVisible(false);

    this->adjustSize();
}

QString RegionMapPropertiesDialog::browse(QString filter, QFileDialog::FileMode mode) {
    if (!this->project) return QString();
    QFileDialog browser;
    browser.setFileMode(mode);
    QString filepath = browser.getOpenFileName(this, "Select a File", this->project->root, filter);

    // remove the project root from the filepath
    return filepath.replace(this->project->root + "/", "");
}

// TODO: test for missing fields/invalid cases
void RegionMapPropertiesDialog::setProperties(poryjson::Json json) {
    poryjson::Json::object object = json.object_items();

    // Region Map Properties
    ui->config_alias->setText(object["alias"].string_value());

    // Tilemap properties
    poryjson::Json::object tilemap = object["tilemap"].object_items();
    ui->config_tilemapFormat->setCurrentText(tilemap["format"].string_value());
    ui->config_tilemapWidth->setValue(tilemap["width"].int_value());
    ui->config_tilemapHeight->setValue(tilemap["height"].int_value());

    ui->config_tilemapImagePath->setText(tilemap["tileset_path"].string_value());
    ui->config_tilemapBinPath->setText(tilemap["tilemap_path"].string_value());
    if (tilemap.contains("palette"))
        ui->config_tilemapPalettePath->setText(tilemap["palette"].string_value());

    // Layout props
    if (object["layout"].is_null()) {
        ui->group_layout->setChecked(false);
    } else {
        poryjson::Json::object layout = object["layout"].object_items();
        ui->config_layoutFormat->setCurrentText(layout["format"].string_value());
        ui->config_layoutPath->setText(layout["path"].string_value());
        ui->config_layoutWidth->setValue(layout["width"].int_value());
        ui->config_layoutHeight->setValue(layout["height"].int_value());
        ui->config_leftOffs->setValue(layout["offset_left"].int_value());
        ui->config_topOffs->setValue(layout["offset_top"].int_value());
    }
}

poryjson::Json RegionMapPropertiesDialog::saveToJson() {
    poryjson::Json::object config;

    // TODO: make sure next comment is not a lie
    // data should already be verified and valid at this point
    config["alias"] = ui->config_alias->text();

    poryjson::Json::object tilemapObject;
    tilemapObject["width"] = ui->config_tilemapWidth->value();
    tilemapObject["height"] = ui->config_tilemapHeight->value();
    tilemapObject["format"] = ui->config_tilemapFormat->currentText();
    tilemapObject["tileset_path"] = ui->config_tilemapImagePath->text();
    tilemapObject["tilemap_path"] = ui->config_tilemapBinPath->text();
    tilemapObject["palette"] = ui->config_tilemapPalettePath->text();
    config["tilemap"] = tilemapObject;

    if (ui->group_layout->isChecked()) {
        poryjson::Json::object layoutObject;
        layoutObject["format"] = ui->config_layoutFormat->currentText();
        layoutObject["path"] = ui->config_layoutPath->text();
        layoutObject["width"] = ui->config_layoutWidth->value();
        layoutObject["height"] = ui->config_layoutHeight->value();
        layoutObject["offset_left"] = ui->config_leftOffs->value();
        layoutObject["offset_top"] = ui->config_topOffs->value();
        config["layout"] = layoutObject;
    } else {
        config["layout"] = nullptr;
    }

    return poryjson::Json(config);
}

void RegionMapPropertiesDialog::on_browse_tilesetImagePath_clicked() {
    QString path = browse("Images (*.png *.bmp)", QFileDialog::ExistingFile);
    if (!path.isEmpty()) {
        ui->config_tilemapImagePath->setText(path);
    }
}

void RegionMapPropertiesDialog::on_browse_tilemapBinPath_clicked() {
    QString path = browse("Binary (*.bin *.tilemap *.4bpp *.8bpp)", QFileDialog::AnyFile);
    if (!path.isEmpty()) {
        ui->config_tilemapBinPath->setText(path);
    }
}

void RegionMapPropertiesDialog::on_browse_tilemapPalettePath_clicked() {
    QString path = browse("Text (*.pal)", QFileDialog::AnyFile);
    if (!path.isEmpty()) {
        ui->config_tilemapPalettePath->setText(path);
    }
}

void RegionMapPropertiesDialog::on_browse_layoutPath_clicked() {
    if (ui->config_layoutFormat->currentIndex() == 0) {
        QString path = browse("Text File (*.h *.c *.inc *.txt)", QFileDialog::AnyFile);
        if (!path.isEmpty()) {
            ui->config_layoutPath->setText(path);
        }
    } else {
        QString path = browse("Binary (*.bin)", QFileDialog::AnyFile);
        if (!path.isEmpty()) {
            ui->config_layoutPath->setText(path);
        }
    }
}

void RegionMapPropertiesDialog::accept() {
    bool valid = true;

    // verify alias is okay
    QString alias = ui->config_alias->text();
    if (alias.isEmpty()) {
        valid = false;
        ui->message_alias->setText("alias cannot be empty");
        ui->message_alias->setVisible(true);
    } else {
        QRegularExpression re("[A-Za-z0-9_\\- ]+");
        int temp = 0;
        QRegularExpressionValidator v(re, 0);
        
        if (v.validate(alias, temp) != QValidator::Acceptable) {
            ui->message_alias->setText("this alias is not an acceptable string");
            ui->message_alias->setVisible(true);
            valid = false;
        }
        else { // no issues detected
            ui->message_alias->setText("");
            ui->message_alias->setVisible(false);
        }
    }

    // tilemap properties
    // tilemap dimensions are specific: 64x32, 32x64, 16x16, 32x32, 64x64, 128x128
    // TODO: why is 30x20 allowed in firered?
    int tilemapWidth = ui->config_tilemapWidth->value();
    int tilemapHeight = ui->config_tilemapHeight->value();
    QMap<int, QList<int>> acceptableDimensions = { {16, {16}}, {30, {20}}, {32, {32, 64}}, {64, {32, 64}}, {128, {128}} };
    if (!acceptableDimensions.contains(tilemapWidth)) {
        // width not valid
        valid = false;
        ui->message_tilemapWidth->setText("tilemap width not valid, must be one of [16, 32, 64, 128]");
        ui->message_tilemapWidth->setVisible(true);
    } else {
        ui->message_tilemapWidth->setText("");
        ui->message_tilemapWidth->setVisible(false);
        // check width, height combo
        if (!acceptableDimensions.value(tilemapWidth).contains(tilemapHeight)) {
            valid = false;
            QString options = "";
            for (int i : acceptableDimensions.value(tilemapWidth))
                options += QString::number(i) + ", ";
            options.chop(2);
            ui->message_tilemapHeight->setText("tilemap height not valid, must be one of [" + options + "]");
            ui->message_tilemapHeight->setVisible(true);
        }
        else { // no issues detected
            ui->message_tilemapHeight->setText("");
            ui->message_tilemapHeight->setVisible(false);
        }
    }

    // layout dimensions
    // need to verify <= tilemap dimensions, only req here
    int width = ui->config_layoutWidth->value() + ui->config_leftOffs->value();
    int height = ui->config_layoutHeight->value() + ui->config_topOffs->value();
    if (width > tilemapWidth) {
        valid = false;
        ui->message_layoutWidth->setText("layout map width + left offset cannot be larger than tilemap width");
        ui->message_layoutWidth->setVisible(true);
    } else {
        ui->message_layoutWidth->setText("");
        ui->message_layoutWidth->setVisible(false);
    }
    if (height > tilemapHeight) {
        valid = false;
        ui->message_layoutHeight->setText("layout map height + top offset cannot be larger than tilemap height");
        ui->message_layoutHeight->setVisible(true);
    } else {
        ui->message_layoutHeight->setText("");
        ui->message_layoutHeight->setVisible(false);
    }

    // make sure paths arent empty
    if (ui->config_tilemapImagePath->text().isEmpty()) {
        valid = false;
        ui->message_tilemapImagePath->setText("this path cannot be empty");
        ui->message_tilemapImagePath->setVisible(true);
    } else {
        ui->message_tilemapImagePath->setText("");
        ui->message_tilemapImagePath->setVisible(false);
    }

    if (ui->config_tilemapBinPath->text().isEmpty()) {
        valid = false;
        ui->message_tilemapBinPath->setText("this path cannot be empty");
        ui->message_tilemapBinPath->setVisible(true);
    } else {
        ui->message_tilemapBinPath->setText("");
        ui->message_tilemapBinPath->setVisible(false);
    }

    // layout path only needs to exist if layout is enabled (obviously)
    if (ui->group_layout->isChecked()) {
        if (ui->config_layoutPath->text().isEmpty()) {
        valid = false;
        ui->message_layoutPath->setText("this path cannot be empty");
        ui->message_layoutPath->setVisible(true);
        } else {
            ui->message_layoutPath->setText("");
            ui->message_layoutPath->setVisible(false);
        }
    } else {
        ui->message_layoutPath->setText("");
        ui->message_layoutPath->setVisible(false);
    }

    // TODO: limitations for map width/height if implementing

    if (valid) {
        QDialog::accept();
    }
}
