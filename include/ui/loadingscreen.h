#include "qgifimage.h"

#include <QSplashScreen>
#include <QTimer>
#include <QWidget>



namespace Ui {
class LoadingScreen;
}

class PorymapLoadingScreen : public QWidget {
    
    Q_OBJECT

public:
    explicit PorymapLoadingScreen(QWidget *parent = nullptr);
    ~PorymapLoadingScreen();

    void setPixmap(QPixmap pixmap);
    void showMessage(QString text);

    void start();
    void stop ();

private:
    void setupUi();

public slots:
    void updateFrame();

private:
    Ui::LoadingScreen *ui;

    QGifImage splashImage;
    int frame = 0;
    QTimer timer;
};

extern PorymapLoadingScreen *porysplash;
