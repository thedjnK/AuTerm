# AuTerm Qt project qmake file

include(../AuTerm-includes.pri)

QT += core gui widgets serialport

!contains(DEFINES, SKIPONLINE) {
    QT += network
}

TARGET = AuTerm
TEMPLATE = app

SOURCES += main.cpp\
    AutEscape.cpp \
    AutLogger.cpp \
    AutMainWindow.cpp \
    AutPlugin.cpp \
    AutPopup.cpp \
    AutScrollEdit.cpp

HEADERS  += \
    AutEscape.h \
    AutLogger.h \
    AutMainWindow.h \
    AutPopup.h \
    AutScrollEdit.h \

FORMS    += \
    AutMainWindow.ui \
    AutPopup.ui

RESOURCES += \
    AuTermImages.qrc

# Automation form
!contains(DEFINES, SKIPAUTOMATIONFORM) {
    SOURCES += \
	AutAutomation.cpp
    HEADERS += \
	AutAutomation.h
    FORMS += \
	AutAutomation.ui
}

# Scripting form
!contains(DEFINES, SKIPSCRIPTINGFORM) {
    SOURCES += \
	AutCodeEditor.cpp \
	AutHighlighter.cpp \
	AutScripting.cpp

    HEADERS += \
	AutCodeEditor.h \
	AutHighlighter.h \
	AutScripting.h

    FORMS += \
    AutScripting.ui
}

# Error code form
!contains(DEFINES, SKIPERRORCODEFORM) {
    SOURCES += \
	AutErrorCode.cpp
    HEADERS += \
	AutErrorCode.h
    FORMS += \
	AutErrorCode.ui
}

# Serial detection object
!contains(DEFINES, SKIPSERIALDETECT) {
    SOURCES += \
	AutSerialDetect.cpp
    HEADERS += \
	AutSerialDetect.h

    win32: LIBS += -luser32
}

# Windows application version information
win32:RC_FILE = version.rc

# Windows application icon
win32:RC_ICONS = images/AuTerm32.ico

# Mac application icon
ICON = MacAuTermIcon.icns

# Common build location
CONFIG(release, debug|release) {
    DESTDIR = ../release
} else {
    DESTDIR = ../debug
}

# Plugins
!contains(DEFINES, SKIPPLUGINS) {
    HEADERS += AutPlugin.h

    contains(CONFIG, static) {
	QT += $$ADDITIONAL_MODULES

        !contains(DEFINES, SKIPPLUGIN_MCUMGR) {
            exists(../plugins/mcumgr) {
                DEFINES += "STATICPLUGIN_MCUMGR"

		win32: LIBS += -L$$DESTDIR -lplugin_mcumgr
		else: LIBS += -L$$DESTDIR -lplugin_mcumgr

		win32-g++: PRE_TARGETDEPS += $$DESTDIR/libplugin_mcumgr.a
		else:win32:!win32-g++: PRE_TARGETDEPS += $$DESTDIR/plugin_mcumgr.lib
		else: PRE_TARGETDEPS += $$DESTDIR/libplugin_mcumgr.a
            }
        }

	!contains(DEFINES, SKIPPLUGIN_LOGGER) {
	    exists(../plugins/logger) {
		DEFINES += "STATICPLUGIN_LOGGER"

		win32: LIBS += -L$$DESTDIR -lplugin_logger
		else: LIBS += -L$$DESTDIR -lplugin_logger

		win32-g++: PRE_TARGETDEPS += $$DESTDIR/libplugin_logger.a
		else:win32:!win32-g++: PRE_TARGETDEPS += $$DESTDIR/plugin_logger.lib
		else: PRE_TARGETDEPS += $$DESTDIR/libplugin_logger.a
	    }
	}
    }
}
