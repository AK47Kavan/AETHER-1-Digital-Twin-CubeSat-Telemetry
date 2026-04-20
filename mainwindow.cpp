#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QSerialPortInfo>
#include <QVBoxLayout>
#include <QDir>
#include <QFile>

// 3D Headers
#include <Qt3DRender/QPointLight>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DRender/QCamera>
#include <Qt3DExtras/QConeMesh>

#include <QVariantAnimation>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    introFinished = false;
    introOpacity = 1.0f;
    akasOpacity = 0.0f;

    QVariantAnimation *akasFade = new QVariantAnimation(this);
    akasFade->setDuration(1500);
    akasFade->setStartValue(0.0f);
    akasFade->setEndValue(1.0f);
    connect(akasFade, &QVariantAnimation::valueChanged, [this](const QVariant &v){
        akasOpacity = v.toFloat();
        update();
    });

    QVariantAnimation *screenFade = new QVariantAnimation(this);
    screenFade->setDuration(1000);
    screenFade->setStartValue(1.0f);
    screenFade->setEndValue(0.0f);
    connect(screenFade, &QVariantAnimation::valueChanged, [this](const QVariant &v){
        introOpacity = v.toFloat();
        update();
    });
    connect(screenFade, &QAbstractAnimation::finished, [this](){
        introFinished = true;
        update();
    });

    akasFade->start(QAbstractAnimation::DeleteWhenStopped);
    QTimer::singleShot(2500, this, [screenFade]() {
        screenFade->start(QAbstractAnimation::DeleteWhenStopped);
    });

    if (ui->satelliteViewLayout->layout() == nullptr) {
        new QVBoxLayout(ui->satelliteViewLayout);
        ui->satelliteViewLayout->layout()->setContentsMargins(0,0,0,0);
    }

    Qt3DExtras::Qt3DWindow *view = new Qt3DExtras::Qt3DWindow();
    view->defaultFrameGraph()->setClearColor(QColor(10, 10, 10));
    container3D = QWidget::createWindowContainer(view);
    container3D->setMinimumSize(QSize(300, 300));
    ui->satelliteViewLayout->layout()->addWidget(container3D);

    Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();
    satEntity = new Qt3DCore::QEntity(rootEntity);
    satTransform = new Qt3DCore::QTransform();
    Qt3DExtras::QCuboidMesh *bodyMesh = new Qt3DExtras::QCuboidMesh();
    bodyMesh->setXExtent(1.2f); bodyMesh->setYExtent(1.2f); bodyMesh->setZExtent(1.2f);

    satMaterial = new Qt3DExtras::QPhongMaterial();
    satMaterial->setDiffuse(QColor(180, 180, 180));
    satMaterial->setAmbient(QColor(60, 60, 60));
    satMaterial->setShininess(150.0f);

    satEntity->addComponent(bodyMesh);
    satEntity->addComponent(satMaterial);
    satEntity->addComponent(satTransform);

    Qt3DExtras::QPhongMaterial *solarMaterial = new Qt3DExtras::QPhongMaterial();
    solarMaterial->setDiffuse(QColor(0, 40, 120));
    Qt3DExtras::QCuboidMesh *wingMesh = new Qt3DExtras::QCuboidMesh();
    wingMesh->setXExtent(1.8f); wingMesh->setYExtent(0.05f); wingMesh->setZExtent(1.2f);

    auto createWing = [&](float side) {
        Qt3DCore::QEntity *wingFrame = new Qt3DCore::QEntity(satEntity);
        Qt3DCore::QTransform *wingTrans = new Qt3DCore::QTransform();
        wingTrans->setTranslation(QVector3D(side * 1.6f, 0, 0));
        wingFrame->addComponent(wingMesh);
        wingFrame->addComponent(solarMaterial);
        wingFrame->addComponent(wingTrans);

        Qt3DCore::QEntity *arm = new Qt3DCore::QEntity(satEntity);
        Qt3DExtras::QCuboidMesh *armMesh = new Qt3DExtras::QCuboidMesh();
        armMesh->setXExtent(0.8f); armMesh->setYExtent(0.05f); armMesh->setZExtent(0.2f);
        Qt3DCore::QTransform *armTrans = new Qt3DCore::QTransform();
        armTrans->setTranslation(QVector3D(side * 0.8f, 0, 0));
        arm->addComponent(armMesh);
        arm->addComponent(satMaterial);
        arm->addComponent(armTrans);
    };

    createWing(-1.0f); createWing(1.0f);

    Qt3DCore::QEntity *dishNeck = new Qt3DCore::QEntity(satEntity);
    Qt3DExtras::QCylinderMesh *neckMesh = new Qt3DExtras::QCylinderMesh();
    neckMesh->setRadius(0.05f); neckMesh->setLength(0.4f);
    Qt3DCore::QTransform *neckTrans = new Qt3DCore::QTransform();
    neckTrans->setTranslation(QVector3D(0, 0.8f, 0));
    dishNeck->addComponent(neckMesh);
    dishNeck->addComponent(neckTrans);

    Qt3DCore::QEntity *dishEntity = new Qt3DCore::QEntity(satEntity);
    Qt3DExtras::QConeMesh *dishMesh = new Qt3DExtras::QConeMesh();
    dishMesh->setBottomRadius(0.6f); dishMesh->setLength(0.3f);
    Qt3DCore::QTransform *dishTransform = new Qt3DCore::QTransform();
    dishTransform->setTranslation(QVector3D(0, 1.1f, 0));
    dishTransform->setRotationX(-90.0f);
    Qt3DExtras::QPhongMaterial *dishMaterial = new Qt3DExtras::QPhongMaterial();
    dishMaterial->setDiffuse(QColor(240, 240, 240));
    dishEntity->addComponent(dishMesh);
    dishEntity->addComponent(dishMaterial);
    dishEntity->addComponent(dishTransform);

    Qt3DRender::QCamera *camera = view->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(5.0f, 4.0f, 7.0f));
    camera->setViewCenter(QVector3D(0, 0, 0));
    view->setRootEntity(rootEntity);

    auto setupChart = [&](QString title, QLineSeries* &series, QChart* &chart, QWidget* layoutWidget) {
        series = new QLineSeries();
        chart = new QChart();
        chart->addSeries(series);
        chart->setTitle(title);
        chart->setAnimationOptions(QChart::SeriesAnimations);
        chart->setBackgroundVisible(false);
        chart->legend()->hide();
        chart->setTitleBrush(QBrush(QColor("#00FFCC")));
        chart->setTitleFont(QFont("Consolas", 10, QFont::Bold));
        QValueAxis *axisX = new QValueAxis;
        axisX->setRange(0, 100); axisX->setGridLineVisible(false); axisX->setLabelsVisible(false);
        QValueAxis *axisY = new QValueAxis;
        axisY->setRange(0, 100); axisY->setGridLineVisible(false); axisY->setLabelsVisible(false);
        chart->addAxis(axisX, Qt::AlignBottom); chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisX); series->attachAxis(axisY);
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setStyleSheet("background: transparent;");
        if(layoutWidget->layout() == nullptr) new QVBoxLayout(layoutWidget);
        layoutWidget->layout()->addWidget(chartView);
    };

    setupChart("TEMPERATURE (°C)", tempSeries, tempChart, ui->tempGraphLayout);
    setupChart("AIR QUALITY (IAQ)", aqiSeries, aqiChart, ui->aqiGraphLayout);
    setupChart("ALTITUDE (M)", altSeries, altChart, ui->altGraphLayout);

    serial = new QSerialPort(this);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readTelemetry);
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->portSelector->addItem(info.portName());
    }

    this->setStyleSheet(
        "QMainWindow { background-color: #0A0A0A; }"
        "QGroupBox { color: #00FFCC; border: 1px solid #333333; border-radius: 5px; font-weight: bold; margin-top: 10px; }"
        "QLabel { color: #00FFCC; font-family: 'Consolas'; font-size: 13px; }"
        "QPushButton { background-color: #222222; color: #00FFCC; border: 1px solid #00FFCC; border-radius: 4px; padding: 5px; }"
        "QPushButton:hover { background-color: #00FFCC; color: black; }"
        "QProgressBar { border: 1px solid #333333; border-radius: 5px; text-align: center; color: white; }"
        "QProgressBar::chunk { background-color: #00FFCC; }"
        "QPlainTextEdit { background-color: #000000; color: #00FF00; font-family: 'Consolas'; border: 1px solid #333333; }"
        );

    currentMode = "INIT";
    logEvent("SYSTEM: Mission Control Initialized.", "#00FFCC");
    QTimer::singleShot(1500, this, [this](){ logEvent("HW_SCAN: SX1262 LoRa Radio... ONLINE", "#00FFCC"); });
    QTimer::singleShot(2000, this, [this](){ logEvent("HW_SCAN: OV2640 Imager... READY", "#00FFCC"); });
    QTimer::singleShot(2500, this, [this](){ logEvent("HW_SCAN: NEO-6M GPS... LOCKING", "#FFFF00"); });
}

MainWindow::~MainWindow() {
    if(serial->isOpen()) serial->close();
    delete ui;
}

void MainWindow::on_connectBtn_clicked() {
    if (serial->isOpen()) {
        serial->close();
        ui->connectBtn->setText("Connect");
        logEvent("SYSTEM: Ground Link Disconnected.", "#FF9900");
    } else {
        serial->setPortName(ui->portSelector->currentText());
        serial->setBaudRate(QSerialPort::Baud115200);
        if (serial->open(QIODevice::ReadWrite)) {
            ui->connectBtn->setText("Disconnect");
            logEvent("SYSTEM: Ground Link Established.", "#00FFCC");
        } else {
            QMessageBox::critical(this, "Link Error", "Could not open port!");
        }
    }
}

void MainWindow::logEvent(QString message, QString color) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString formattedMsg = QString("<font color=\"gray\">[%1]</font> <font color=\"%2\">%3</font>")
                               .arg(timestamp, color, message);
    ui->telemetryLog->appendHtml(formattedMsg);
    ui->telemetryLog->moveCursor(QTextCursor::End);
}

void MainWindow::readTelemetry() {
    while (serial->canReadLine()) {
        QByteArray rawData = serial->readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(rawData);
        if (doc.isNull() || !doc.isObject()) continue;

        QJsonObject data = doc.object();

        int battery = data["batt"].toDouble();
        double gasValue = data["gas"].toDouble();
        int iaqValue = data["iaq"].toInt();
        QString prediction = data["pred"].toString();
        double conf = data["conf"].toDouble() * 100;
        double temperature = data["temp"].toDouble();
        double altitude = data["alt"].toDouble();
        QString mode = data["mode"].toString();
        QString cityName = data["city"].toString();

        double lat = data["lat"].toDouble();
        double lon = data["lon"].toDouble();
        int sats = data.contains("sats") ? data["sats"].toInt() : 0;

        QString locDisplay = cityName.isEmpty() ?
                                 QString("%1N %2E").arg(lat, 0, 'f', 2).arg(lon, 0, 'f', 2) : cityName;

        ui->gpsLabel->setText(QString("GPS: %1 SATS | %2").arg(sats).arg(locDisplay));

        if (sats > 8) ui->gpsLabel->setStyleSheet("color: #00FFCC;");
        else if (sats > 0) ui->gpsLabel->setStyleSheet("color: #FFFF00;");
        else ui->gpsLabel->setStyleSheet("color: #FF0000;");

        if (mode == "CAM_TRIGGER") {
            ui->statusLabel->setText("IMAGER: DOWNLINKING DATA...");
            ui->statusLabel->setStyleSheet("background: #00FFCC; color: black; font-weight: bold;");
        }

        tempSeries->append(tickCount, temperature);
        aqiSeries->append(tickCount, iaqValue);
        altSeries->append(tickCount, altitude);

        if (tickCount > 100) {
            tempChart->axisX()->setRange(tickCount - 100, tickCount);
            aqiChart->axisX()->setRange(tickCount - 100, tickCount);
            altChart->axisX()->setRange(tickCount - 100, tickCount);
        }

        tempChart->axisY()->setRange(temperature - 5, temperature + 5);
        if (iaqValue > 0) aqiChart->axisY()->setRange(0, iaqValue + 50);
        altChart->axisY()->setRange(altitude - 50, altitude + 50);

        tickCount++;

        ui->batteryBar->setValue(battery);
        if (battery < 20) ui->batteryBar->setStyleSheet("QProgressBar::chunk { background-color: #FF0000; }");
        else if (battery < 50) ui->batteryBar->setStyleSheet("QProgressBar::chunk { background-color: #FF9900; }");
        else ui->batteryBar->setStyleSheet("QProgressBar::chunk { background-color: #00FFCC; }");

        int totalMins = data["life_mins"].toInt();
        ui->durabilityLabel->display(totalMins / 60.0);

        QString sensorLog = QString("SENSOR: Gas %1 ohms | IAQ %2 | Temp %3C")
                                .arg(gasValue, 0, 'f', 0).arg(iaqValue).arg(temperature, 0, 'f', 1);
        logEvent(sensorLog, "#AAAAAA");

        if (prediction == "DISASTER") {
            ui->statusLabel->setText(tickCount % 2 == 0 ? ">>> CRITICAL ALERT <<<" : "!!! DISASTER DETECTED !!!");
            ui->statusLabel->setStyleSheet("background-color: #FF0000; color: white; font-weight: bold; padding: 5px; border-radius: 3px;");
            satMaterial->setDiffuse(QColor(255, 0, 0));
            static QString lastLoggedPred = "";
            if (lastLoggedPred != "DISASTER") {
                logEvent("AI_ALERT: Disaster detected at " + locDisplay, "#FF0000");
                lastLoggedPred = "DISASTER";
            }
        } else if (mode != "CAM_TRIGGER") {
            ui->statusLabel->setText("SYSTEM STATUS: NOMINAL");
            ui->statusLabel->setStyleSheet("color: #00FFCC; background-color: transparent;");
            satMaterial->setDiffuse(QColor(255, 215, 0));
        }

        if (mode != currentMode) {
            currentMode = mode;
            logEvent("STATE: Satellite switched to " + currentMode, "#FFFF00");
        }

        if(data.contains("pitch") && data.contains("roll")) {
            satTransform->setRotationX(data["pitch"].toDouble());
            satTransform->setRotationY(data["yaw"].toDouble());
            satTransform->setRotationZ(data["roll"].toDouble());
        }

        if (data.contains("img_data") && !data["img_data"].toString().isEmpty()) {
            QString encodedImg = data["img_data"].toString();
            QByteArray imageBytes = QByteArray::fromBase64(encodedImg.toUtf8());
            QString tempPath = QDir::tempPath() + "/sat_downlink.jpg";
            QFile file(tempPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(imageBytes);
                file.close();
                showSatelliteImage(tempPath);
            }
        }
    }
}

void MainWindow::on_wakeBtn_clicked() {
    if (serial && serial->isOpen()) {
        if (currentMode != "DEEP_SLEEP") {
            logEvent("UPLINK: System active. Command ignored.", "#AAAAAA");
            return;
        }
        serial->write("W");
        logEvent("UPLINK: EMERGENCY WAKE COMMAND SENT.", "#00AAFF");
    }
}

void MainWindow::on_saveLogBtn_clicked() {
    QString fileName = "AETHER_Log_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmm") + ".csv";
    QString filePath = QFileDialog::getSaveFileName(this, "Export Data", fileName, "CSV Files (*.csv)");
    if (filePath.isEmpty()) return;
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << ui->telemetryLog->toPlainText();
        file.close();
        logEvent("SYSTEM: Log saved.", "#00FFCC");
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QFont logoFont("Consolas", 24, QFont::Bold);
    logoFont.setLetterSpacing(QFont::AbsoluteSpacing, 5);
    painter.setFont(logoFont);
    painter.setPen(QColor("#00FFCC"));
    painter.drawText((this->width() - painter.fontMetrics().horizontalAdvance("AETHER - 1")) / 2, 45, "AETHER - 1");

    painter.setFont(QFont("Consolas", 9, QFont::Normal));
    painter.setPen(QColor("#666666"));
    painter.drawText(this->width() - painter.fontMetrics().horizontalAdvance("build by AKAS") - 15, this->height() - 15, "build by AKAS");

    QRect camRect = ui->cameraDisplay->geometry();
    painter.setPen(QPen(QColor(0, 255, 204, 150), 2));
    int len = 20;
    painter.drawLine(camRect.left(), camRect.top(), camRect.left() + len, camRect.top());
    painter.drawLine(camRect.left(), camRect.top(), camRect.left(), camRect.top() + len);
    painter.drawLine(camRect.right(), camRect.top(), camRect.right() - len, camRect.top());
    painter.drawLine(camRect.right(), camRect.top(), camRect.right(), camRect.top() + len);
    painter.drawLine(camRect.left(), camRect.bottom(), camRect.left() + len, camRect.bottom());
    painter.drawLine(camRect.left(), camRect.bottom(), camRect.left(), camRect.bottom() - len);
    painter.drawLine(camRect.right(), camRect.bottom(), camRect.right() - len, camRect.bottom());
    painter.drawLine(camRect.right(), camRect.bottom(), camRect.right(), camRect.bottom() - len);

    int scanY = camRect.top() + (tickCount * 10 % camRect.height());
    painter.setPen(QPen(QColor(0, 255, 204, 80), 1));
    painter.drawLine(camRect.left(), scanY, camRect.right(), scanY);

    if (currentMode == "CAM_TRIGGER" && tickCount % 2 == 0) {
        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(camRect.left() + 15, camRect.top() + 15, 10, 10);
        painter.setPen(Qt::red);
        painter.drawText(camRect.left() + 30, camRect.top() + 25, "LIVE FEED");
    }
}

void MainWindow::showSatelliteImage(QString path) {
    QPixmap fullImg(path);

    if (fullImg.isNull()) {
        logEvent("CRITICAL: Image file corrupted or missing.", "#FF0000");
        ui->cameraDisplay->setText(">>> LINK ERROR: NO DATA <<<");
        return;
    }

    logEvent("DOWNLINK: Synchronizing LoRa Tiles...", "#00AAFF");

    // Initialize buffer
    QPixmap buffer(fullImg.size());
    buffer.fill(Qt::black);

    // We use a static or member-like pointer so the lambda can increment it
    int *currentChunk = new int(0);
    int totalChunks = 10;

    QTimer *downloadTimer = new QTimer(this);
    connect(downloadTimer, &QTimer::timeout, [this, fullImg, buffer, currentChunk, totalChunks, downloadTimer]() mutable {
        (*currentChunk)++; // Increment the actual value

        int h = (fullImg.height() * (*currentChunk)) / totalChunks;

        QPixmap temp = buffer;
        QPainter p(&temp);
        p.drawPixmap(0, 0, fullImg.width(), h, fullImg.copy(0, 0, fullImg.width(), h));
        p.end();

        ui->cameraDisplay->setPixmap(temp.scaled(ui->cameraDisplay->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

        if (*currentChunk >= totalChunks) {
            downloadTimer->stop();
            delete currentChunk; // Clean up memory
            downloadTimer->deleteLater();
            logEvent("SYSTEM: Image Payload Verified.", "#00FFCC");
        }
    });
    downloadTimer->start(150);
}
