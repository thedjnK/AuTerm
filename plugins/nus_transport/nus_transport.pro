include(../../AuTerm-includes.pri)

QT += gui widgets serialport $$ADDITIONAL_MODULES

TEMPLATE = lib

CONFIG += plugin
CONFIG += c++17

INCLUDEPATH    += ../../AuTerm
TARGET          = $$qtLibraryTarget(plugin_nus_transport)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    nus_bluetooth_setup.cpp \
    plugin_nus_transport.cpp

HEADERS += \
    ../../AuTerm/AutPlugin.h \
    nus_bluetooth_setup.h \
    plugin_nus_transport.h

FORMS += \
	nus_bluetooth_setup.ui

DISTFILES += plugin_nus_transport.json

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/plugin_nus_transport
}
!isEmpty(target.path): INSTALLS += target

CONFIG += install_ok  # Do not cargo-cult this!

# Logger plugin logging
!contains(DEFINES, SKIPPLUGIN_LOGGER) {
    exists(../logger/public) {
	INCLUDEPATH    += ../logger/public

	SOURCES += \
	    ../logger/public/debug_logger.cpp

	HEADERS += \
	    ../logger/public/debug_logger.h
    }
}

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
