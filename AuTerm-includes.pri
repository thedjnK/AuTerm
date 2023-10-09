# AuTerm global include qmake project settings
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

#Uncomment to disable plugin support
#DEFINES += "SKIPPLUGINS"

#Uncomment to skip building MCUmgr plugin
#DEFINES += "SKIPPLUGIN_MCUMGR"

#Uncomment to skip building logger plugin
DEFINES += "SKIPPLUGIN_LOGGER"

#Uncomment to build MCUmgr plugin transports (note: UDP requires Qt network, Bluetooth requires Qt Connectivity - note: static builds need those in the base AuTerm build also)
DEFINES += "PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH"
DEFINES += "PLUGIN_MCUMGR_TRANSPORT_UDP"

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH) {
    ADDITIONAL_MODULES += "bluetooth"
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_UDP) {
    ADDITIONAL_MODULES += "network"
}
