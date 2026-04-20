#include "mainwindow.h"
#include <QApplication>
#include <QSplashScreen>
#include <QPainter>
#include <QTimer>
#include <QThread>
#include <QRandomGenerator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    QPixmap pix("C:\\Users\\kavan\\OneDrive\\Documents\\AETHER-1\\AETHER-1\\aether_logo.png");
    QSplashScreen splash(pix);
    splash.show();


    for(int i = 0; i < 100; ++i) {
        QThread::msleep(20);
        a.processEvents();
    }


    QPixmap canvas(800, 600);
    canvas.fill(QColor(10, 10, 10));


    QFont font("Consolas", 36, QFont::Bold);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 10);
    QString text = "BUILT BY AKAS";


    for(int i = 0; i <= 100; i += 2) {
        QPixmap temp = canvas;
        QPainter painter(&temp);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setFont(font);

        float alpha = i / 100.0;


        bool isGlitched = (QRandomGenerator::global()->bounded(100) < 10);
        if (isGlitched) {
            alpha *= 0.3;
        }

        QColor color("#00FFCC");
        color.setAlphaF(alpha);
        painter.setPen(color);


        int xOffset = isGlitched ? QRandomGenerator::global()->bounded(-5, 5) : 0;
        QRect textRect = temp.rect();
        textRect.moveLeft(xOffset);

        painter.drawText(textRect, Qt::AlignCenter, text);

        splash.setPixmap(temp);
        a.processEvents();
        QThread::msleep(20);
    }

    QThread::msleep(1000);

    MainWindow w;
    w.show();
    splash.finish(&w);

    return a.exec();
}
