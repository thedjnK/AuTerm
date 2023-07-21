# AuTerm Qt project qmake file
# By default all components are built for Github AuTerm releases

# Uncomment to exclude building automation form
#DEFINES += "SKIPAUTOMATIONFORM"
# Uncomment to exclude building error code lookup form
#DEFINES += "SKIPERRORCODEFORM"
# Uncomment to exclude building scripting form
#DEFINES += "SKIPSCRIPTINGFORM"
# Uncomment to exclude building speed test functionality
#DEFINES += "SKIPSPEEDTEST"
# Uncomment to resolve IP address from hostname prior to making web request
#DEFINES += "RESOLVEIPSEPARATELY"

include(../AuTerm-includes.pri)

QT       += core gui widgets serialport

TARGET = AuTerm
TEMPLATE = app

SOURCES += main.cpp\
    AutPlugin.cpp \
    AutScrollEdit.cpp \
    UwxMainWindow.cpp \
    UwxPopup.cpp \
    LrdLogger.cpp \
    UwxEscape.cpp

HEADERS  += \
    AutScrollEdit.h \
    UwxMainWindow.h \
    UwxPopup.h \
    LrdLogger.h \
    UwxEscape.h \
    AutPlugin.h

FORMS    += \
    UwxPopup.ui \
    UwxMainWindow.ui

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

!contains(DEFINES, SKIPPLUGINS) {
    contains(CONFIG, static) {
        !contains(DEFINES, SKIPPLUGIN_MCUMGR) {
            exists(../plugins/mcumgr) {
                DEFINES += "STATICPLUGIN_MCUMGR"

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../plugins/release/ -llibmcumgr
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../plugins/debug/ -llibmcumgr
else:unix: LIBS += -L$$OUT_PWD/../plugins/ -llibmcumgr

INCLUDEPATH += $$PWD/../plugins
DEPENDPATH += $$PWD/../plugins

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../plugins/release/libmcumgr.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../plugins/debug/libmcumgr.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../plugins/release/libmcumgr.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../plugins/debug/libmcumgr.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../plugins/libmcumgr.a
            }
        }
    }
}
