include(../../AuTerm-includes.pri)

QT += gui widgets serialport

TEMPLATE = lib

CONFIG += plugin
CONFIG += c++17

INCLUDEPATH    += ../../AuTerm
TARGET          = $$qtLibraryTarget(plugin_logger)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    plugin_logger.cpp

HEADERS += \
    ../../AuTerm/AutPlugin.h \
    plugin_logger.h

FORMS += \
    plugin_logger.ui

DISTFILES += plugin_logger.json

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/plugin_logger
}
!isEmpty(target.path): INSTALLS += target

CONFIG += install_ok  # Do not cargo-cult this!

# Common build location
CONFIG(release, debug|release) {
    DESTDIR = ../../release
} else {
    DESTDIR = ../../debug
}

# Do not prefix with lib for non-static builds
!contains(CONFIG, static) {
    CONFIG += no_plugin_name_prefix
}
