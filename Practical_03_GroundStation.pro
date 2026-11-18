QT += core gui widgets charts
CONFIG += c++17
QMAKE_UIC_FLAGS += --no-autoconnection
TARGET = GroundStation
TEMPLATE = app
INCLUDEPATH += src
SOURCES += src/main.cpp src/mainwindow.cpp src/trajectoryview.cpp src/uav.cpp src/sensor.cpp src/navigation.cpp src/controller.cpp src/autopilot.cpp
HEADERS += src/mainwindow.h src/trajectoryview.h src/uav.h src/sensor.h src/navigation.h src/controller.h src/autopilot.h
FORMS += src/mainwindow.ui
RESOURCES += resources.qrc
QMAKE_MACOSX_DEPLOYMENT_TARGET = 13.0

QMAKE_TARGET_BUNDLE_NAME = Наземная станция
