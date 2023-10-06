# AuTerm Qt project qmake file

include(../AuTerm-includes.pri)

QT       += core gui widgets serialport

TARGET = AuTerm
TEMPLATE = app

SOURCES += main.cpp\
    AutEscape.cpp \
    AutMainWindow.cpp \
    AutPlugin.cpp \
    AutScrollEdit.cpp \
    UwxPopup.cpp \
    LrdLogger.cpp

HEADERS  += \
    AutEscape.h \
    AutMainWindow.h \
    AutScrollEdit.h \
    UwxPopup.h \
    LrdLogger.h \

FORMS    += \
    AutMainWindow.ui \
    UwxPopup.ui

RESOURCES += \
    AuTermImages.qrc

# Automation form
!contains(DEFINES, SKIPAUTOMATIONFORM) {
    SOURCES += UwxAutomation.cpp
    HEADERS += UwxAutomation.h
    FORMS += UwxAutomation.ui
}

# Scripting form
!contains(DEFINES, SKIPSCRIPTINGFORM) {
    SOURCES += LrdCodeEditor.cpp \
    LrdHighlighter.cpp \
    UwxScripting.cpp

    HEADERS += LrdCodeEditor.h \
    LrdHighlighter.h \
    UwxScripting.h

    FORMS += UwxScripting.ui
}

# Error code form
!contains(DEFINES, SKIPERRORCODEFORM) {
    SOURCES += UwxErrorCode.cpp
    HEADERS += UwxErrorCode.h
    FORMS += UwxErrorCode.ui
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
		else:unix: LIBS += -L$$DESTDIR -lplugin_mcumgr

#INCLUDEPATH += $$PWD/../plugins
#DEPENDPATH += $$PWD/../plugins

		win32-g++: PRE_TARGETDEPS += $$DESTDIR/libplugin_mcumgr.a
		else:win32:!win32-g++: PRE_TARGETDEPS += $$DESTDIR/plugin_mcumgr.lib
		else:unix: PRE_TARGETDEPS += $$DESTDIR/libplugin_mcumgr.a
            }
        }

	!contains(DEFINES, SKIPPLUGIN_LOGGER) {
	    exists(../plugins/logger) {
		DEFINES += "STATICPLUGIN_LOGGER"

		win32: LIBS += -L$$DESTDIR -lplugin_logger
		else:unix: LIBS += -L$$DESTDIR -lplugin_logger

#INCLUDEPATH += $$PWD/../plugins
#DEPENDPATH += $$PWD/../plugins

		win32-g++: PRE_TARGETDEPS += $$DESTDIR/libplugin_logger.a
		else:win32:!win32-g++: PRE_TARGETDEPS += $$DESTDIR/plugin_logger.lib
		else:unix: PRE_TARGETDEPS += $$DESTDIR/libplugin_logger.a
	    }
	}
    }
}
