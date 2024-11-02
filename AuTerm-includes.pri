# AuTerm global include qmake project settings
# By default all components are built for Github AuTerm releases

# Build for x86_64 and arm64 on mac
QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64

# Uncomment to exclude building automation form
#DEFINES += "SKIPAUTOMATIONFORM"

# Uncomment to exclude building error code lookup form
#DEFINES += "SKIPERRORCODEFORM"

# Uncomment to exclude building scripting form
#DEFINES += "SKIPSCRIPTINGFORM"

# Uncomment to exclude building speed test functionality
#DEFINES += "SKIPSPEEDTEST"

# Uncomment to exclude online functionlaity (update checking)
#DEFINES += "SKIPONLINE"

# Uncomment to skip building serial port detection object
#DEFINES += "SKIPSERIALDETECT"

# Serial port detection not currently supported on mac
macx: DEFINES += "SKIPSERIALDETECT"

# Uncomment to disable plugin support
#DEFINES += "SKIPPLUGINS"

# Uncomment to skip building MCUmgr plugin
#DEFINES += "SKIPPLUGIN_MCUMGR"

# Uncomment to skip building logger plugin
#DEFINES += "SKIPPLUGIN_LOGGER"

# Uncomment to disable transport plugin support (will be disabled if plugin support is disabled)
#DEFINES += "SKIPPLUGINS_TRANSPORT"

# Uncomment to build MCUmgr plugin transports (note: static builds need these extra modules in the base AuTerm build also)
!contains(DEFINES, SKIPPLUGINS) {
    !contains(DEFINES, SKIPPLUGIN_MCUMGR) {
	qtHaveModule(bluetooth) {
	    # Requires qtconnectivity
	    DEFINES += "PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH"
	}

	qtHaveModule(network) {
	    # Requires qtnetwork
	    DEFINES += "PLUGIN_MCUMGR_TRANSPORT_UDP"

	    qtHaveModule(mqtt) {
		# Requires qtnetwork and qtmqtt
		DEFINES += "PLUGIN_MCUMGR_TRANSPORT_LORAWAN"
	    }
	}
    }
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_BLUETOOTH) {
    ADDITIONAL_MODULES += "bluetooth"
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_UDP) | contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_LORAWAN) {
    ADDITIONAL_MODULES += "network"
}

contains(DEFINES, PLUGIN_MCUMGR_TRANSPORT_LORAWAN) {
    ADDITIONAL_MODULES += "mqtt"
}
