TEMPLATE = subdirs

include(AuTerm-includes.pri)

SUBDIRS += \
    AuTerm

!contains(DEFINES, SKIPPLUGINS) {
    !contains(DEFINES, SKIPPLUGIN_MCUMGR) {
        SUBDIRS += \
            plugins/mcumgr

        AuTerm.depends += plugins/mcumgr
    }

    !contains(DEFINES, SKIPPLUGIN_LOGGER) {
        SUBDIRS += \
            plugins/logger

        AuTerm.depends += plugins/logger
    }

    !contains(DEFINES, SKIPPLUGINS_TRANSPORT) {
        !contains(DEFINES, SKIPPLUGIN_TRANSPORT_ECHO) {
            SUBDIRS += \
                plugins/echo_transport

            AuTerm.depends += plugins/echo_transport
        }

        !contains(DEFINES, SKIPPLUGIN_TRANSPORT_NUS) {
            SUBDIRS += \
                plugins/nus_transport

            AuTerm.depends += plugins/nus_transport
        }
    }
}
