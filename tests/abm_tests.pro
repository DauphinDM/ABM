QT += core gui qml quick quickcontrols2 network testlib
CONFIG += c++17 testcase
TARGET = abm_tests
TEMPLATE = app
INCLUDEPATH += ../src
HEADERS += ../src/telemetry.h ../src/backend.h ../src/theme.h ../src/translator.h ../src/integration.h ../src/previews.h
SOURCES += abm_tests.cpp ../src/telemetry.cpp ../src/backend.cpp ../src/theme.cpp ../src/translator.cpp ../src/integration.cpp ../src/previews.cpp
unix: LIBS += -larchive
win32 {
    INCLUDEPATH += $$(VCPKG_ROOT)/installed/x64-windows/include
    LIBS += -L$$(VCPKG_ROOT)/installed/x64-windows/lib -larchive
}
RESOURCES += ../src/resources.qrc
