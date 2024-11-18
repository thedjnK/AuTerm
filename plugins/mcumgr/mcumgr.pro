include(../../AuTerm-includes.pri)

QT += gui widgets serialport $$ADDITIONAL_MODULES

TEMPLATE = lib

CONFIG += plugin
CONFIG += c++17

INCLUDEPATH    += ../../AuTerm
TARGET          = $$qtLibraryTarget(plugin_mcumgr)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../AuTerm/AutScrollEdit.cpp \
    ../../AuTerm/AutEscape.cpp \
    crc16.cpp \
    debug_logger.cpp \
    error_lookup.cpp \
    plugin_mcumgr.cpp \
    smp_error.cpp \
    smp_group_enum_mgmt.cpp \
    smp_group_fs_mgmt.cpp \
    smp_group_os_mgmt.cpp \
    smp_group_settings_mgmt.cpp \
    smp_group_shell_mgmt.cpp \
    smp_group_stat_mgmt.cpp \
    smp_group_zephyr_mgmt.cpp \
    smp_json.cpp \
    smp_message.cpp \
    smp_processor.cpp \
    smp_uart_auterm.cpp \
    smp_group_img_mgmt.cpp

HEADERS += \
    ../../AuTerm/AutPlugin.h \
    ../../AuTerm/AutScrollEdit.h \
    ../../AuTerm/AutEscape.h \
    crc16.h \
    debug_logger.h \
    error_lookup.h \
    plugin_mcumgr.h \
    smp_error.h \
    smp_group_array.h \
    smp_group_enum_mgmt.h \
    smp_group_fs_mgmt.h \
    smp_group_os_mgmt.h \
    smp_group_settings_mgmt.h \
    smp_group_shell_mgmt.h \
    smp_group_stat_mgmt.h \
    smp_group_zephyr_mgmt.h \
    smp_json.h \
    smp_message.h \
    smp_processor.h \
    smp_transport.h \
    smp_uart_auterm.h \
    smp_group.h \
    smp_group_img_mgmt.h

DISTFILES += plugin_mcumgr.json

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/plugin_mcumgr
}
!isEmpty(target.path): INSTALLS += target

CONFIG += install_ok  # Do not cargo-cult this!

# Common build location
CONFIG(release, debug|release) {
    DESTDIR = ../../release
} else {
    DESTDIR = ../../debug


    # The following form is only used for creating the GUI in Qt Creator, it is
    # not used by any part of the code in a normal build, therefore only build
    # this in debug mode.
    SOURCES += \
        form.cpp

    HEADERS += \
        form.h

    FORMS += \
        form.ui
}

# Do not prefix with lib for non-static builds
!contains(CONFIG, static) {
    CONFIG += no_plugin_name_prefix
}

FORMS += \
    error_lookup.ui

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH) {
    SOURCES += \
	bluetooth_setup.cpp \
	smp_bluetooth.cpp

    HEADERS += \
	bluetooth_setup.h \
	smp_bluetooth.h

    FORMS += \
	bluetooth_setup.ui
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_UDP) {
    SOURCES += \
	udp_setup.cpp \
	smp_udp.cpp

    HEADERS += \
	udp_setup.h \
	smp_udp.h

    FORMS += \
	udp_setup.ui
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_LORAWAN) {
    SOURCES += \
	lorawan_setup.cpp \
	smp_lorawan.cpp

    HEADERS += \
	lorawan_setup.h \
	smp_lorawan.h

    FORMS += \
    lorawan_setup.ui
}
