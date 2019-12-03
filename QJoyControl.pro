#-------------------------------------------------
#
# Project created by Erik Werner 2018-09-22T21:09:40
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = QJoyControl
TEMPLATE = app

VERSION = 0.2
QMAKE_TARGET_PRODUCT = "QJoyControl"
QMAKE_TARGET_COMPANY = "UntitledSoftware"

DEFINES += APP_VERSION=\"\\\"$${VERSION}\\\"\" \
           APP_PRODUCT=\"\\\"$${QMAKE_TARGET_PRODUCT}\\\"\" \
           APP_COMPANY=\"\\\"$${QMAKE_TARGET_COMPANY}\\\"\"


# With C++11 support
greaterThan(QT_MAJOR_VERSION, 4){
CONFIG += c++11
} else {
QMAKE_CXXFLAGS += -std=c++0x
}

INCLUDEPATH += hidapi/hidapi

SOURCES += \
    inputmappanel.cpp \
    inputmapwidget.cpp \
    main.cpp \
    mainwindow.cpp \
    joyconworker.cpp \
    statuswidget.cpp



HEADERS += \
    inputmappanel.h \
    inputmapwidget.h \
    mainwindow.h \
    joyconworker.h \
    luts.h \
    ir_sensor.h \
    statuswidget.h \
    eventhandler.h \
    hidapi/hidapi/hidapi.h

FORMS += \
    inputmappanel.ui \
    inputmapwidget.ui \
    mainwindow.ui \
    statuswidget.ui

macx {
    CONFIG += app_bundle
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
    ICON = img/Logo.icns

    # use custom plist to hide dock icon
    #QMAKE_INFO_PLIST = Info.plist

    # for homebrew hidapi installation
    #HIDAPI_PATH = /usr/local/Cellar/hidapi/0.9.0
    #INCLUDEPATH += $$HIDAPI_PATH/include/hidapi

    # use local hidapi installation
    SOURCES += hidapi/mac/hid.c
    HIDAPI_PATH = $$PWD/hidapi/mac
    INCLUDEPATH += $$HIDAPI_PATH

    #prevent app nap from sleeping threads
    INCLUDEPATH += $$PWD/mac
    HEADERS += mac/powertools.h
    OBJECTIVE_SOURCES += mac/powertools.mm
    LIBS += -framework Foundation

    # include hidapi dynamic library
    #LIBS += $$HIDAPI_PATH/lib/libhidapi.dylib

    # include power management libs
    LIBS += -framework CoreFoundation -framework IOkit

    # include system event services Mac
    LIBS += -framework ApplicationServices
    SOURCES += mac/eventhandler_macos.cpp
}

unix:!macx {
    SOURCES += linux/hid-libusb.c
    LIBS += -lusb-1.0
    # include system event services Linux
    #unix: !macx:  LIB +=
    SOURCES += eventhandler_linux.cpp
}

win32 {
    # include system event services Windows
    #win32: LIB +=
    SOURCES += eventhandler_win.cpp
    SOURCES += windows/hid.cpp
    LIBS += -lSetupAPI
}

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
