# AuTerm

## Preface

Note that AuTerm is currently in the process of being developed for an initial release, the code is not considered stable and only pre-release test versions are available for download.

Test release on Linux:

![Linux test release screenshot](/docs/images/linux_build.png?raw=true)

Test release on Windows:

![Windows test release screenshot](/docs/images/windows_build.png?raw=true)

## About

AuTerm is a cross-platform terminal utility, designed for communicating with embedded devices and systems (with a primary focus on Zephyr devices, but supports any device be it embedded or not), created in Qt 5 (and supporting Qt 6).

## Downloading

Source code and pre-release test builds are provided.

## Setup

### Windows:

**Note:** Only 64-bit x86_64 Windows builds are officially provided, 32-bit x86 builds can be built from source.

Download and open the 7zip file, extract the files to a folder on your computer and double click 'AuTerm.exe' to run AuTerm.

The Visual Studio 2022 runtime files are required which are available on the [Microsoft site](https://aka.ms/vs/17/release/vc_redist.x64.exe).

### Linux:

**Note:** Only 64-bit x86_64 Linux builds are officially provided, 32-bit x86 and ARM builds can be built from source.

Download the tar file and extract it's contents to a location on your computer, this can be done using a graphical utility or from the command line using:

	tar xf AuTerm_<version>.tar.gz -C ~/

Where '\~/' is the location of where you want it extracted to, '\~/' will extract to the home directory of your local user account). To launch AuTerm, either double click on the executable and click the 'run' button (if asked), or execute it from a terminal as so:

	./AuTerm

Before running, you may need to install some additional libraries, please see https://github.com/LairdCP/UwTerminalX/wiki/Installing for further details. You may also be required to add a udev rule to grant non-root users access to USB devices, please see https://github.com/LairdCP/UwTerminalX/wiki/Granting-non-root-USB-device-access-(Linux) for details.

### Mac:

Mac versions must be built from source using Qt 5.x or 6.x.

## Help and contributing

Users are welcome to open issues and submit pull requests to have features merged. PRs should target the `develop` branch.

## Speed/Throughput testing

There is a quick guide available giving an overview of the speed testing feature of AuTerm, https://github.com/LairdCP/UwTerminalX/wiki/Using-the-Speed-Test-feature

## Compiling

For details on compiling, please refer to [the wiki](https://github.com/LairdCP/UwTerminalX/wiki/Compiling).

## License

AuTerm is released under the [GPLv3 license](https://github.com/thedjnK/AuTerm/blob/master/LICENSE).
