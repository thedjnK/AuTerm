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
}
