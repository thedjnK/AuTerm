# AuTerm

## Preface

Note that AuTerm is currently in the process of being developed for an initial release, the code is not considered stable and only pre-release test versions are available for download.

Sneek preview:
![Sneek preview](/docs/images/peek.png?raw=true)

## About

AuTerm is a cross-platform terminal utility, designed for communicating with embedded devices and systems, created in Qt 5.

## Downloading

Only source code is provided at present, and some pre-release test binaries.

## Setup

### Windows:

Download and open the zip file, extract the files to a folder on your computer and double click 'AuTerm.exe' to run AuTerm.

If using the SSL version of AuTerm, then the Visual Studio 2015 runtime files are required which are available on the [Microsoft site](https://www.microsoft.com/en-gb/download/details.aspx?id=48145).

### Mac:

Mac versions must be built from source using Qt 5.x

### Linux (Including Raspberry Pi):

Download the tar file and extract it's contents to a location on your computer, this can be done using a graphical utility or from the command line using:

	tar xf AuTerm_<version>.tar.gz -C ~/

Where '\~/' is the location of where you want it extracted to, '\~/' will extract to the home directory of your local user account). To launch AuTerm, either double click on the executable and click the 'run' button (if asked), or execute it from a terminal as so:

	./AuTerm

Before running, you may need to install some additional libraries, please see https://github.com/LairdCP/AuTerm/wiki/Installing for further details. You may also be required to add a udev rule to grant non-root users access to USB devices, please see https://github.com/LairdCP/AuTerm/wiki/Granting-non-root-USB-device-access-(Linux) for details.

## Help and contributing

Users are welcome to open issues and submit pull requests to have features merged. PRs should target the `develop` branch.

## Speed/Throughput testing

There is a quick guide available giving an overview of the speed testing feature of AuTerm, https://github.com/LairdCP/AuTerm/wiki/Using-the-Speed-Test-feature

## Compiling

For details on compiling, please refer to [the wiki](https://github.com/LairdCP/AuTerm/wiki/Compiling).

## License

AuTerm is released under the [GPLv3 license](https://github.com/LairdCP/AuTerm/blob/master/LICENSE).
