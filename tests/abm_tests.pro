QT += core gui qml quick quickcontrols2 network testlib
CONFIG += c++17 testcase
TARGET = abm_tests
TEMPLATE = app
INCLUDEPATH += ../src
HEADERS += ../src/telemetry.h ../src/backend.h ../src/theme.h ../src/translator.h ../src/integration.h ../src/previews.h ../src/serverpolicy.h ../src/playerprofiles.h ../src/presetstore.h
SOURCES += abm_tests.cpp ../src/telemetry.cpp ../src/backend.cpp ../src/theme.cpp ../src/translator.cpp ../src/integration.cpp ../src/previews.cpp ../src/serverpolicy.cpp ../src/playerprofiles.cpp ../src/presetstore.cpp
unix: LIBS += -larchive -lcrypto
win32 {
    INCLUDEPATH += $$(VCPKG_ROOT)/installed/x64-windows/include
    LIBS += -L$$(VCPKG_ROOT)/installed/x64-windows/lib -larchive -llibcrypto
}
RESOURCES += ../src/resources.qrc
HEADERS += ../src/mobilehost.h ../src/companionfirewall.h ../src/usbtunnel.h
SOURCES += ../src/mobilehost.cpp ../src/companionfirewall.cpp ../src/usbtunnel.cpp
HEADERS += ../src/appupdates.h
SOURCES += ../src/appupdates.cpp
HEADERS += ../src/localtls.h
SOURCES += ../src/localtls.cpp
