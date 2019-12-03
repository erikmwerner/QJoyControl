#include "mainwindow.h"
#include "powertools.h"
#include <QApplication>

#ifndef QT_NO_SYSTEMTRAYICON

#include <QMessageBox>

int main(int argc, char *argv[])
{
    int id1 = qRegisterMetaType<QList<double> >();
    int id2 = qRegisterMetaType<QList<int> >();
    int id3 = qRegisterMetaType<QList<unsigned char> >();
    int id4 = qRegisterMetaType<ir_image_config>();

    Q_INIT_RESOURCE(resources);
    QApplication a(argc, argv);
    disableAppNap();
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, QObject::tr("Systray"),
                              QObject::tr("Unable to detect a system tray "
                                          "on this system."));
        return 1;
    }
    QApplication::setQuitOnLastWindowClosed(false);
    MainWindow w;

    w.show();

    int code = a.exec();
    enableAppNap();
    return code;
}

#else

#include <QLabel>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString text("QSystemTrayIcon is not supported on this platform");

    QLabel *label = new QLabel(text);
    label->setWordWrap(true);

    label->show();
    qDebug() << text;

    app.exec();
}

#endif
