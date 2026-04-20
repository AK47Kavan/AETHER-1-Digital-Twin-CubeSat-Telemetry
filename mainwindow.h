#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLCDNumber>
#include <QVBoxLayout>
#include <QtSerialPort/QSerialPort>
#include <QJsonObject>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>



// Forward declarations for Qt 3D classes to keep the header clean
namespace Qt3DCore {
class QEntity;
class QTransform;
}
namespace Qt3DExtras {
class QPhongMaterial;
}

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // --- Core Telemetry Functions ---
    void readTelemetry();
    void logEvent(QString message, QString color = "#E0E0E0");
    void showSatelliteImage(QString path);

    // --- UI Button Controls ---
    void on_connectBtn_clicked();
    void on_wakeBtn_clicked();
    void on_saveLogBtn_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;

    // --- AI & Operational States ---
    QString currentMode;
    double predictedLife;

    // --- 3D Visualization Members ---
    Qt3DCore::QEntity *satEntity;
    Qt3DCore::QTransform *satTransform;
    Qt3DExtras::QPhongMaterial *satMaterial; // Added this to change color to RED on disaster
    QWidget *container3D;

    QLineSeries *tempSeries;
    QLineSeries *aqiSeries;
    QLineSeries *altSeries;
    QChart *tempChart;
    QChart *aqiChart;
    QChart *altChart;
    int tickCount = 0;
    float creditOpacity = 0.0; // Starts invisible
    float introOpacity = 1.0;  // For the whole overlay
    float akasOpacity = 0.0;   // For the text itself
    bool introFinished = false;

protected:
    void paintEvent(QPaintEvent *event) override;

};

#endif
