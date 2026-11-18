QT += core testlib
QT -= gui
CONFIG += console c++17 testcase
CONFIG -= app_bundle
TARGET = ModelTests
INCLUDEPATH += src
SOURCES += tests/model_tests.cpp src/uav.cpp src/sensor.cpp src/navigation.cpp src/controller.cpp src/autopilot.cpp
HEADERS += src/uav.h src/sensor.h src/navigation.h src/controller.h src/autopilot.h
