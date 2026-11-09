QT += core gui qml quick quickcontrols2 network
CONFIG += c++17 release
TARGET = abm
TEMPLATE = app
HEADERS += src/backend.h src/telemetry.h src/theme.h src/translator.h src/protocol.h src/integration.h src/previews.h src/serverpolicy.h
SOURCES += src/main.cpp src/backend.cpp src/telemetry.cpp src/theme.cpp src/translator.cpp src/integration.cpp src/previews.cpp src/serverpolicy.cpp
unix: LIBS += -larchive
win32 {
    INCLUDEPATH += $$(VCPKG_ROOT)/installed/x64-windows/include
    LIBS += -L$$(VCPKG_ROOT)/installed/x64-windows/lib -larchive
}
RESOURCES += src/resources.qrc
