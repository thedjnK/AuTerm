# AuTerm

## Preface

Note that AuTerm is currently in the process of being developed for an initial release, the code is not considered stable and only pre-release test versions are available for download.

## About

AuTerm is a cross-platform terminal utility, designed for communicating with embedded devices and systems (with a primary focus on Zephyr devices, but supports any device be it embedded or not), created in Qt 6 (and supporting Qt 5, though Qt 6 is recommended).

## Features

* Serial port support
* UTF-8 text support
* Speed test feature (including statistics and validity)
* File streaming functionality
* Customisable interface
* Scripting functionality
* Log file output and log viewer
* Error code viewer
* VT100 control code support
* Plugin support for additional features and transports
* MCUmgr plugin
  - Image management group support
  - Filesystem management group support
  - OS management group support
  - Statistic management group support
  - Shell management group support
  - Settings management group support
  - Zephyr basic management group support
  - Enumeration management group support
  - Custom command support using JSON or CBOR
  - UART transport support
  - Bluetooth transport support (supporting write with and without response modes)
  - UDP transport support
  - LoRaWAN (TTS via MQTT) transport support
* Logger plugin
* NUS (Nordic UART Service) transport plugin

Functionality can be disabled in custom builds by uncommenting the SKIP lines in ``AuTerm-includes.pri``, which allows for lean and reduced size builds.

## Screenshots

Test release on Linux:

![Linux test release screenshot](/docs/images/linux_build.png?raw=true)

Test release on Windows:

![Windows test release screenshot](/docs/images/windows_build.png?raw=true)

## Downloading

Source code and pre-release test builds are provided.

## Setup

### Windows

**Note:** Only 64-bit x86_64 Windows builds are officially provided, 32-bit x86 builds can be built from source.

Download and open the 7zip file, extract the files to a folder on your computer and double click 'AuTerm.exe' to run AuTerm.

The Visual Studio 2022 runtime files are required which are available on the [Microsoft site](https://aka.ms/vs/17/release/vc_redist.x64.exe).

### Linux


#### x86

**Note:** Only 64-bit x86_64 Linux builds are officially provided, 32-bit x86 builds can be built from source.

Download the tar file and extract it's contents to a location on your computer, this can be done using a graphical utility or from the command line using:

	tar xf AuTerm_<version>.tar.gz -C ~/

Where '\~/' is the location of where you want it extracted to, '\~/' will extract to the home directory of your local user account). To launch AuTerm, either double click on the executable and click the 'run' button (if asked), or execute it from a terminal as so:

	./AuTerm

Before running, you may need to install some additional libraries, please see https://github.com/LairdCP/UwTerminalX/wiki/Installing for further details. You may also be required to add a udev rule to grant non-root users access to USB devices, please see https://github.com/LairdCP/UwTerminalX/wiki/Granting-non-root-USB-device-access-(Linux) for details.

#### ARM

ARM versions must be built from source using Qt 6.x. Pre-compiled release versions may be provided in the future if requested (please open an issue if this is something you would like).

### Mac

**Note:** No support for this build/os is provided, no hardware is available for testing, it is built on github CI only. Use at own risk.

Download the tar file and extract it's contents to a location on your computer, this can be done using a graphical utility or from the command line using:

	tar xf AuTerm_<version>.tar.gz -C ~/

Where '\~/' is the location of where you want it extracted to, '\~/' will extract to the home directory of your local user account). To launch AuTerm, either double click on the executable and click the 'run' button (if asked). It is possible that gatekeeper might prevent running of it until system security settings are changed.

## Help and contributing

Users are welcome to open issues and submit pull requests to have features merged. PRs on github should target the `main` branch, PRs on the internal git server should target the `develop` branch.

## Speed/Throughput testing

There is a quick guide available giving an overview of the speed testing feature of AuTerm, https://github.com/LairdCP/UwTerminalX/wiki/Using-the-Speed-Test-feature

## Compiling

For details on compiling, please refer to [the wiki](https://github.com/LairdCP/UwTerminalX/wiki/Compiling).

## License

AuTerm is released under the [GPLv3 license](https://github.com/thedjnK/AuTerm/blob/master/LICENSE).
