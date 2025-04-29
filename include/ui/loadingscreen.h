#include "qgifimage.h"

#include <QSplashScreen>
#include <QTimer>
#include <QWidget>

namespace Ui {
class LoadingScreen;
}

// Loading class
// LOAD() // or Offload() OFFLOAD() WorkerThread()
// function that wraps around QFuture and QtConcurrent, does not block gui thread
// can call any other function that is not directly painting the gui on another thread

// Execute function while playing loading screen and display text
// void executeLoadingScreen(QString text, void (*function)(void));

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
