/******************************************************************************
** Copyright (C) 2015-2022 Laird Connectivity
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module: UwxMainWindow.cpp
**
** Notes:
**
** License: This program is free software: you can redistribute it and/or
**          modify it under the terms of the GNU General Public License as
**          published by the Free Software Foundation, version 3.
**
**          This program is distributed in the hope that it will be useful,
**          but WITHOUT ANY WARRANTY; without even the implied warranty of
**          MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**          GNU General Public License for more details.
**
**          You should have received a copy of the GNU General Public License
**          along with this program.  If not, see http://www.gnu.org/licenses/
**
*******************************************************************************/

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "UwxMainWindow.h"
#include "ui_UwxMainWindow.h"
#include "AutPlugin.h"
#include <QDebug>


#include <QPluginLoader>

//TODO: Needs to move to struct
QPluginLoader plugin_loader;

struct plugins {
    QString filename;
    QString name;
    QString version;
    QObject *object;
    AutPlugin *plugin;
};

QList<plugins> plugin_list;

/******************************************************************************/
// Conditional Compile Defines
/******************************************************************************/
#ifdef QT_DEBUG
    //Include debug output when compiled for debugging
    #include <QDebug>
#endif
#ifdef _WIN32
    //Windows
    #ifdef _WIN64
        //Windows 64-bit
        #define OS "Windows (x86_64)"
    #else
        //Windows 32-bit
        #define OS "Windows (x86)"
    #endif
#elif defined(__APPLE__)
    #include "TargetConditionals.h"
    #ifdef TARGET_OS_MAC
        //Mac OSX
        #define OS "Mac"
        QString gstrMacBundlePath;
    #endif
#else
    //Assume Linux
    #ifdef __aarch64__
        //ARM64
        #define OS "Linux (AArch64)"
    #elif __arm__
        //ARM
        #define OS "Linux (ARM)"
    #elif __x86_64__
        //x86_64
        #define OS "Linux (x86_64)"
    #elif __i386
        //x86
        #define OS "Linux (x86)"
    #else
        //Unknown
        #define OS "Linux (other)"
    #endif
#endif

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    int32_t i = 0;

    //Setup the GUI
    ui->setupUi(this);

#ifndef SKIPPLUGINS
    //Find and load plugins
#ifdef QT_STATIC
    //For static Qt builds, plugins must be compiled into the build
    QVector<QStaticPlugin> static_plugins = QPluginLoader::staticPlugins();
    struct plugins plugin;
    while (i < static_plugins.length())
    {
        if (static_plugins.at(i).metaData().contains("IID") == true && static_plugins.at(i).metaData().value("IID").toString() == AuTermPluginInterface_iid)
        {
//TODO: Add support for this
//            plugin.filename = plugin_names.at(i);
//            plugin.name = ;
//            plugin.version = ;

            plugin.object = static_plugins.at(i).instance();
            plugin.plugin = qobject_cast<AutPlugin *>(plugin.object);

            if (plugin.plugin)
            {
                plugin.plugin->setup(this);
                plugin_list.append(plugin);

//                connect(plugin.object, SIGNAL(show_message_box(QString)), gpmErrorForm, SLOT(show_message(QString)));
                connect(plugin.object, SIGNAL(plugin_set_status(bool,bool)), this, SLOT(plugin_set_status(bool,bool)));
            }
        }

        ++i;
    }
#else
    //For dynamic builds, external library plugins can be loaded
    QDir app_dir(QApplication::applicationDirPath());
    app_dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);

#ifdef _WIN32
    app_dir.setNameFilters(QStringList() << "plugin_*.dll");
#elif defined(__APPLE__)
#error "Plugins are not supported on mac"
#else
    app_dir.setNameFilters(QStringList() << "plugin_*.so");
#endif

    QStringList plugin_names = app_dir.entryList();
    struct plugins plugin;
    while (i < plugin_names.length())
    {
//TODO: move plugin loader to struct
        plugin_loader.setFileName(QString(QApplication::applicationDirPath()).append("/").append(plugin_names.at(i)));
        plugin.object = plugin_loader.instance();

        if (plugin_loader.isLoaded())
        {
            plugin.filename = plugin_names.at(i);
//TODO: Add support for this
//            plugin.name = ;
//            plugin.version = ;
            plugin.plugin = qobject_cast<AutPlugin *>(plugin.object);

            if (plugin.plugin)
            {
                plugin.plugin->setup(this);
                plugin_list.append(plugin);

//                connect(plugin.object, SIGNAL(show_message_box(QString)), gpmErrorForm, SLOT(show_message(QString)));
                connect(plugin.object, SIGNAL(plugin_set_status(bool,bool)), this, SLOT(plugin_set_status(bool,bool)));
            }
            else
            {
                plugin_loader.unload();
            }
        }

        ++i;
    }
#endif
#endif

#if SKIPSPEEDTEST == 1
    //Delete speed test elements to reduce RAM usage
    ui->tab_SpeedTest->setEnabled(false);
    ui->edit_SpeedBytesRec->deleteLater();
    ui->edit_SpeedBytesRec10s->deleteLater();
    ui->edit_SpeedBytesRecAvg->deleteLater();
    ui->edit_SpeedBytesSent->deleteLater();
    ui->edit_SpeedBytesSent10s->deleteLater();
    ui->edit_SpeedBytesSentAvg->deleteLater();
    ui->edit_SpeedPacketsBad->deleteLater();
    ui->edit_SpeedPacketsErrorRate->deleteLater();
    ui->edit_SpeedPacketsGood->deleteLater();
    ui->edit_SpeedPacketsRec->deleteLater();
    ui->edit_SpeedTestData->deleteLater();
    ui->text_SpeedEditData->deleteLater();
    ui->combo_SpeedDataDisplay->deleteLater();
    ui->combo_SpeedDataType->deleteLater();
    ui->check_SpeedDTR->deleteLater();
    ui->check_SpeedRTS->deleteLater();
    ui->check_SpeedShowErrors->deleteLater();
    ui->check_SpeedShowRX->deleteLater();
    ui->check_SpeedShowTX->deleteLater();
    ui->check_SpeedStringUnescape->deleteLater();
    ui->check_SpeedSyncReceive->deleteLater();
    ui->btn_SpeedClear->deleteLater();
    ui->btn_SpeedClose->deleteLater();
    ui->btn_SpeedCopy->deleteLater();
    ui->btn_SpeedStartStop->deleteLater();
    ui->tab_SpeedTest->deleteLater();
#endif

    //Output build information
#if SKIPAUTOMATIONFORM == 1 || SKIPERRORCODEFORM == 1 || SKIPSCRIPTINGFORM == 1 || SKIPSPEEDTEST == 1
    ui->text_Terms->appendPlainText("");
#endif
#if SKIPAUTOMATIONFORM == 1
    ui->text_Terms->appendPlainText("[Built without Automation support]");
#endif
#if SKIPERRORCODEFORM == 1
    ui->text_Terms->appendPlainText("[Built without Error code form]");
#endif
#if SKIPSCRIPTINGFORM == 1
    ui->text_Terms->appendPlainText("[Built without Scripting support]");
#endif
#if SKIPSPEEDTEST == 1
    ui->text_Terms->appendPlainText("[Built without Speed test support]");
#endif

#ifdef TARGET_OS_MAC
    //On mac, get the directory of the bundle (which will be <location>/Term.app/Contents/MacOS) and go up to the folder with the file in
    QDir BundleDir(QCoreApplication::applicationDirPath());
    BundleDir.cdUp();
    BundleDir.cdUp();
    BundleDir.cdUp();
    gstrMacBundlePath = BundleDir.path().append("/");
    if (!QDir().exists(QStandardPaths::writableLocation(QStandardPaths::DataLocation)))
    {
        //Create AuTerm directory in application support
        QDir().mkdir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    }

    //Fix mac's resize
    resize(740, 400);

    //Disable viewing files externally for mac
    ui->btn_EditExternal->setEnabled(false);

    //Increase duplicate button size for mac
    ui->btn_Duplicate->setMinimumWidth(130);
#endif

#if 0
#ifndef _WIN32
    #ifdef __APPLE__
        //Change size of text fonts for Mac
        QFont fntTmpFnt(ui->label_PreXCompInfo->font());
        fntTmpFnt.setPixelSize(12);
        ui->label_PreXCompInfo->setFont(fntTmpFnt);
        ui->label_OnlineXCompInfo->setFont(fntTmpFnt);
        ui->label_AuTermText->setFont(fntTmpFnt);
        ui->label_ErrorCodeText->setFont(fntTmpFnt);
        ui->label_AppFirmwareText1->setFont(fntTmpFnt);
        ui->label_AppFirmwareText2->setFont(fntTmpFnt);
    #else
        //Change size of text fonts for Linux
        QFont fntTmpFnt(ui->label_PreXCompInfo->font());
        fntTmpFnt.setPixelSize(11);
        ui->label_AuTermText->setFont(fntTmpFnt);
        ui->label_ErrorCodeText->setFont(fntTmpFnt);
    #endif
#endif
#endif

    //Define default variable values
    gbTermBusy = false;
    gbPluginRunning = false;
    gbStreamingFile = false;
    gintRXBytes = 0;
    gintTXBytes = 0;
    gintQueuedTXBytes = 0;
    gchTermBusyLines = 0;
    gchTermMode = 0;
    gchTermMode2 = 0;
    gbMainLogEnabled = false;
    gbLoopbackMode = false;
    gbSysTrayEnabled = false;
    gbIsUWCDownload = false;
    gbCTSStatus = 0;
    gbDCDStatus = 0;
    gbDSRStatus = 0;
    gbRIStatus = 0;
    gbStreamingBatch = false;
    gbaBatchReceive.clear();
    gbFileOpened = false;
    gbEditFileModified = false;
    giEditFileType = -1;
    gbErrorsLoaded = false;
    gbAutoBaud = false;
#if 0
    gnmManager = 0;
#endif
#if SKIPSPEEDTEST != 1
    gtmrSpeedTestDelayTimer = 0;
    gbSpeedTestRunning = false;
#endif
    gstrUpdateCheckString = 0;

#ifndef SKIPAUTOMATIONFORM
    guaAutomationForm = 0;
#endif
#ifndef SKIPERRORCODEFORM
    gecErrorCodeForm = 0;
#else
    ui->btn_Error->deleteLater();
#endif
#ifndef SKIPSCRIPTINGFORM
    gbScriptingRunning = false;
    gusScriptingForm = 0;
#endif
    gbAppStarted = false;

    //Clear display buffer byte array and reserve 128KB of RAM to reduce mallocs (should allow faster speed testing at 1M baud)
    gbaDisplayBuffer.clear();
    gbaDisplayBuffer.reserve(131072);

    //Also reserve 64KB of RAM to reduce mallocs when speed testing
    gbaSpeedReceivedData.reserve(65536);

    //Load settings from configuration files
    LoadSettings();

    //Create logging handle
    gpMainLog = new LrdLogger();

    //Move to 'Config' tab
    ui->selector_Tab->setCurrentIndex(ui->selector_Tab->indexOf(ui->tab_Config));

    //Set default values for combo boxes on 'Config' tab
    ui->combo_Baud->setCurrentIndex(8);
    ui->combo_Stop->setCurrentIndex(0);
    ui->combo_Data->setCurrentIndex(1);
    ui->combo_Handshake->setCurrentIndex(1);

    //Load images
    gimEmptyCircleImage = QImage(":/images/EmptyCircle.png");
    gimRedCircleImage = QImage(":/images/RedCircle.png");
    gimGreenCircleImage = QImage(":/images/GreenCircle.png");
#ifdef _WIN32
    //Load ICOs for windows
    gimUw16Image = QImage(":/images/AuTerm16.ico");
    gimUw32Image = QImage(":/images/AuTerm32.ico");
#else
    //Load PNGs for Linux/Mac
    gimUw16Image = QImage(":/images/AuTerm16.png");
    gimUw32Image = QImage(":/images/AuTerm32.png");
#endif

#ifndef UseSSL
    //Disable SSL checkbox for non-SSL builds
    ui->check_EnableSSL->setCheckable(false);
    ui->check_EnableSSL->setChecked(false);
    ui->check_EnableSSL->setEnabled(false);
#endif

    //Create pixmaps
    gpEmptyCirclePixmap = new QPixmap(QPixmap::fromImage(gimEmptyCircleImage));
    gpRedCirclePixmap = new QPixmap(QPixmap::fromImage(gimRedCircleImage));
    gpGreenCirclePixmap = new QPixmap(QPixmap::fromImage(gimGreenCircleImage));
    gpUw16Pixmap = new QPixmap(QPixmap::fromImage(gimUw16Image));
    gpUw32Pixmap = new QPixmap(QPixmap::fromImage(gimUw32Image));

    //Show images on help
    ui->label_AboutI1->setPixmap(*gpEmptyCirclePixmap);
    ui->label_AboutI2->setPixmap(*gpRedCirclePixmap);
    ui->label_AboutI3->setPixmap(*gpGreenCirclePixmap);

    //Default empty images
    ui->image_CTS->setPixmap(*gpEmptyCirclePixmap);
    ui->image_DCD->setPixmap(*gpEmptyCirclePixmap);
    ui->image_DSR->setPixmap(*gpEmptyCirclePixmap);
    ui->image_RI->setPixmap(*gpEmptyCirclePixmap);

    ui->image_CTSb->setPixmap(*gpEmptyCirclePixmap);
    ui->image_DCDb->setPixmap(*gpEmptyCirclePixmap);
    ui->image_DSRb->setPixmap(*gpEmptyCirclePixmap);
    ui->image_RIb->setPixmap(*gpEmptyCirclePixmap);

    //Enable custom context menu policy
    ui->text_TermEditData->setContextMenuPolicy(Qt::CustomContextMenu);

    //Connect quit signals
    connect(ui->btn_Quit, SIGNAL(clicked()), this, SLOT(close()));

    //Connect key-press signals
    connect(ui->text_TermEditData, SIGNAL(EnterPressed()), this, SLOT(EnterPressed()));
    connect(ui->text_TermEditData, SIGNAL(KeyPressed(int,QChar)), this, SLOT(KeyPressed(int,QChar)));

    //Connect file drag/drop signal
    connect(ui->text_TermEditData, SIGNAL(FileDropped(QString)), this, SLOT(DroppedFile(QString)));

    //Initialise popup message
    gpmErrorForm = new PopupMessage();

    //Populate the list of devices
    RefreshSerialDevices();

#if SKIPSPEEDTEST != 1
    //Setup speed test mode timers
    gtmrSpeedTestStats.setInterval(SpeedTestStatUpdateTime);
    gtmrSpeedTestStats.setSingleShot(false);
    connect(&gtmrSpeedTestStats, SIGNAL(timeout()), this, SLOT(UpdateSpeedTestValues()));
    gtmrSpeedTestStats10s.setInterval(10000);
    gtmrSpeedTestStats10s.setSingleShot(false);
    connect(&gtmrSpeedTestStats10s, SIGNAL(timeout()), this, SLOT(OutputSpeedTestStats()));
#endif
    //Display version
    ui->statusBar->showMessage(QString("AuTerm")
#ifdef UseSSL
    .append("-SSL")
#endif
    .append(" version ").append(UwVersion).append(" (").append(OS).append("), Built ").append(__DATE__).append(" Using QT ").append(QT_VERSION_STR)
#ifdef UseSSL
#ifdef TARGET_OS_MAC
    .append(", ").append(QString(QSslSocket::sslLibraryBuildVersionString()).replace(",", ":"))
#else
    .append(", ").append(QString(QSslSocket::sslLibraryBuildVersionString()).left(QSslSocket::sslLibraryBuildVersionString().indexOf(" ", 9)))
#endif
#endif
#ifdef QT_DEBUG
    .append(" [DEBUG BUILD]")
#endif
    );
    setWindowTitle(QString("AuTerm (v").append(UwVersion).append(")"));

    //Create menu items
    gpMenu = new QMenu(this);
    gpMenu->addAction("Lookup Selected Error-Code (Hex)")->setData(MenuActionErrorHex);
    gpMenu->addAction("Lookup Selected Error-Code (Int)")->setData(MenuActionErrorInt);
    gpMenu->addAction("Enable Loopback (Rx->Tx)")->setData(MenuActionLoopback);
    gpSMenu4 = gpMenu->addMenu("Customisation");
    gpSMenu4->addAction("Font")->setData(MenuActionFont);
    gpSMenu4->addAction("Text Colour")->setData(MenuActionTextColour);
    gpSMenu4->addAction("Background Colour")->setData(MenuActionBackground);
    gpSMenu4->addAction("Restore Defaults")->setData(MenuActionRestoreDefaults);
    gpMenu->addAction("Automation")->setData(MenuActionAutomation);
    gpMenu->addAction("Scripting")->setData(MenuActionScripting);
    gpMenu->addAction("Batch")->setData(MenuActionBatch);
    gpMenu->addAction("Clear Display")->setData(MenuActionClearDisplay);
    gpMenu->addAction("Clear RX/TX count")->setData(MenuActionClearRxTx);
    gpMenu->addSeparator();
    gpMenu->addAction("Copy")->setData(MenuActionCopy);
    gpMenu->addAction("Copy All")->setData(MenuActionCopyAll);
    gpMenu->addAction("Paste")->setData(MenuActionPaste);
    gpMenu->addAction("Select All")->setData(MenuActionSelectAll);

    //Create balloon menu items
    gpBalloonMenu = new QMenu(this);
    gpBalloonMenu->addAction("Show AuTerm")->setData(BalloonActionShow);
    gpBalloonMenu->addAction("Exit")->setData(BalloonActionExit);

#if SKIPSPEEDTEST != 1
    //Create speed test button items
    gpSpeedMenu = new QMenu(this);
    gpSpeedMenu->addAction("Receive-only test")->setData(SpeedMenuActionRecv);
    gpSpeedMenu->addAction("Send-only test")->setData(SpeedMenuActionSend);
    gpSpeedMenu->addAction("Send && receive test")->setData(SpeedMenuActionSendRecv);
    gpSpeedMenu->addAction("Send && receive test (delay 5 seconds)")->setData(SpeedMenuActionSendRecv5Delay);
    gpSpeedMenu->addAction("Send && receive test (delay 10 seconds)")->setData(SpeedMenuActionSendRecv10Delay);
    gpSpeedMenu->addAction("Send && receive test (delay 15 seconds)")->setData(SpeedMenuActionSendRecv15Delay);
#endif

    //Disable unimplemented actions
#ifdef SKIPAUTOMATIONFORM
    //Disable automation option
    gpMenu->actions().at(MenuActionAutomation)->setEnabled(false);
#endif
#ifdef SKIPSCRIPTINGFORM
    //Disable scripting option
    gpMenu->actions().at(MenuActionScripting)->setEnabled(false);
#endif

#if defined(TARGET_OS_MAC) || (defined(SKIPUSBRECOVERY) && SKIPUSBRECOVERY == 1)
    //Remove exit autorun button on mac
    ui->btn_ExitAutorun->deleteLater();
#endif

    //Connect the menu actions
    connect(gpMenu, SIGNAL(triggered(QAction*)), this, SLOT(MenuSelected(QAction*)), Qt::AutoConnection);
    connect(gpMenu, SIGNAL(aboutToHide()), this, SLOT(ContextMenuClosed()), Qt::AutoConnection);
    connect(gpBalloonMenu, SIGNAL(triggered(QAction*)), this, SLOT(balloontriggered(QAction*)), Qt::AutoConnection);
#if SKIPSPEEDTEST != 1
    connect(gpSpeedMenu, SIGNAL(triggered(QAction*)), this, SLOT(SpeedMenuSelected(QAction*)), Qt::AutoConnection);
#endif

    //Configure the module timeout timer
    gtmrDownloadTimeoutTimer.setSingleShot(true);
    gtmrDownloadTimeoutTimer.setInterval(ModuleTimeout);
    connect(&gtmrDownloadTimeoutTimer, SIGNAL(timeout()), this, SLOT(DevRespTimeout()));

    //Configure the signal timer
    gpSignalTimer = new QTimer(this);
    connect(gpSignalTimer, SIGNAL(timeout()), this, SLOT(SerialStatusSlot()));

    //Connect serial signals
    connect(&gspSerialPort, SIGNAL(readyRead()), this, SLOT(SerialRead()));
    connect(&gspSerialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(SerialError(QSerialPort::SerialPortError)));
    connect(&gspSerialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(SerialBytesWritten(qint64)));
    connect(&gspSerialPort, SIGNAL(aboutToClose()), this, SLOT(SerialPortClosing()));

    //Set update text display timer to be single shot only and connect to slot
    gtmrTextUpdateTimer.setSingleShot(true);
    gtmrTextUpdateTimer.setInterval(gpTermSettings->value("TextUpdateInterval", DefaultTextUpdateInterval).toInt());
    connect(&gtmrTextUpdateTimer, SIGNAL(timeout()), this, SLOT(UpdateReceiveText()));

#if SKIPSPEEDTEST != 1
    //Set update speed display timer to be single shot only and connect to slot
    gtmrSpeedUpdateTimer.setSingleShot(true);
    gtmrSpeedUpdateTimer.setInterval(gpTermSettings->value("TextUpdateInterval", DefaultTextUpdateInterval).toInt());
    connect(&gtmrSpeedUpdateTimer, SIGNAL(timeout()), this, SLOT(UpdateDisplayText()));
#endif

    //Setup timer for batch file timeout
    gtmrBatchTimeoutTimer.setSingleShot(true);
    connect(&gtmrBatchTimeoutTimer, SIGNAL(timeout()), this, SLOT(BatchTimeoutSlot()));

    //Set logging options
    ui->edit_LogFile->setText(gpTermSettings->value("LogFile", DefaultLogFile).toString());
    ui->check_LogEnable->setChecked(gpTermSettings->value("LogEnable", DefaultLogEnable).toBool());
    ui->check_LogAppend->setChecked(gpTermSettings->value("LogMode", DefaultLogMode).toBool());

#ifdef UseSSL
    //Set SSL status
    ui->check_EnableSSL->setChecked(gpTermSettings->value("SSLEnable", DefaultSSLEnable).toBool());
    if (ui->check_EnableSSL->isChecked() == true)
    {
        //HTTPS
        WebProtocol = "https";
    }
    else
    {
        //HTTP
        WebProtocol = "http";
    }
#endif

    //Set window size saving
    ui->check_EnableTerminalSizeSaving->setChecked(gpTermSettings->value("SaveSize", DefaultSaveSize).toBool());

    if (ui->check_EnableTerminalSizeSaving->isChecked() && gpTermSettings->contains("WindowWidth") && gpTermSettings->contains("WindowHeight"))
    {
        //Restore window size
        this->resize(gpTermSettings->value("WindowWidth", this->width()).toUInt(), gpTermSettings->value("WindowHeight", this->height()).toUInt());
    }
    else
    {
#ifdef _WIN32
        this->resize(580, 210);
#endif
    }

    //Check if default devices were created
    if (gpPredefinedDevice->value("DoneSetup").isNull())
    {
        //Create default device configurations... Zephyr
        uint8_t nCurrentDevice = 1;
        QString strPrefix = QString("Port").append(QString::number(nCurrentDevice));
        gpPredefinedDevice->setValue(QString(strPrefix).append("Name"), "Zephyr");
        gpPredefinedDevice->setValue(QString(strPrefix).append("Baud"), "115200");
        gpPredefinedDevice->setValue(QString(strPrefix).append("Parity"), "0");
        gpPredefinedDevice->setValue(QString(strPrefix).append("Stop"), "1");
        gpPredefinedDevice->setValue(QString(strPrefix).append("Data"), "8");
        gpPredefinedDevice->setValue(QString(strPrefix).append("Flow"), "0");
        ++nCurrentDevice;

        //Mark as completed
        gpPredefinedDevice->setValue(QString("DoneSetup"), "1");
    }

    //Add predefined devices
    i = 1;
    while (i < 255)
    {
        if (gpPredefinedDevice->value(QString("Port").append(QString::number(i)).append("Name")).isNull())
        {
            break;
        }
        ui->combo_PredefinedDevice->addItem(gpPredefinedDevice->value(QString("Port").append(QString::number(i)).append("Name")).toString());
        ++i;
    }

    //Load settings from first device
    if (ui->combo_PredefinedDevice->count() > 0)
    {
        on_combo_PredefinedDevice_currentIndexChanged(ui->combo_PredefinedDevice->currentIndex());
    }

    //Enable system tray if it's available and enabled
    if (gpTermSettings->value("SysTrayIcon", DefaultSysTrayIcon).toBool() == true && QSystemTrayIcon::isSystemTrayAvailable())
    {
        //System tray enabled and available on system, set it up with contect menu/icon and show it
        gpSysTray = new QSystemTrayIcon;
        gpSysTray->setContextMenu(gpBalloonMenu);
        gpSysTray->setToolTip(QString("AuTerm v").append(UwVersion));
        gpSysTray->setIcon(QIcon(*gpUw16Pixmap));
        gpSysTray->show();
        gbSysTrayEnabled = true;
    }

    //Load line separator setting
    ui->check_LineSeparator->setChecked(gpTermSettings->value("ShiftEnterLineSeparator", DefaultShiftEnterLineSeparator).toBool());

    //Notify scroll edit area of line separator value
    ui->text_TermEditData->mbLineSeparator = ui->check_LineSeparator->isChecked();

    //Load last directory path
    gstrLastFilename[FilenameIndexApplication] = gpTermSettings->value("LastFileDirectory", "").toString();
    gstrLastFilename[FilenameIndexScripting] = gpTermSettings->value("LastScriptFileDirectory", "").toString();
    gstrLastFilename[FilenameIndexOthers] = gpTermSettings->value("LastOtherFileDirectory", "").toString();

    //Refresh list of log files
    on_btn_LogRefresh_clicked();

    QFont fntTmpFnt2;

    if (gpTermSettings->contains("CustomFont") && gpTermSettings->contains("CustomPalette"))
    {
        //Load saved settings
        fntTmpFnt2 = QFont(gpTermSettings->value("CustomFont").value<QFont>());
        QPalette palTmp = gpTermSettings->value("CustomPalette").value<QPalette>();

        ui->text_TermEditData->setPalette(palTmp);
        ui->text_LogData->setPalette(palTmp);
#if SKIPSPEEDTEST != 1
        ui->text_SpeedEditData->setPalette(palTmp);
#endif
    }
    else
    {
        //Use monospaced font for terminal
        fntTmpFnt2 = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    }

    //Setup font
    ui->text_TermEditData->setFont(fntTmpFnt2);
    ui->text_LogData->setFont(fntTmpFnt2);
#if SKIPSPEEDTEST != 1
    ui->text_SpeedEditData->setFont(fntTmpFnt2);
#endif

    //Setup font spacing
    QFontMetrics tmTmpFM(fntTmpFnt2);
    ui->text_SpeedEditData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);
    ui->text_TermEditData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);
    ui->text_LogData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);

#ifdef RESOLVEIPSEPARATELY
    //Set resolved hostname to be empty
    gstrResolvedServer = "";
#endif

#ifdef UseSSL
    //Load SSL certificate
    QFile certFile(":/certificates/AuTerm_new.crt");
    if (certFile.open(QIODevice::ReadOnly))
    {
        //Load certificate data
        QSslConfiguration sslcConfig;
        sslcLairdSSLNew = new QSslCertificate(certFile.readAll());
        sslcConfig.addCaCertificate(*sslcLairdSSLNew);
        certFile.close();
    }
#endif

    //Setup the terminal scrollback buffer size
    ui->text_TermEditData->SetupScrollback(gpTermSettings->value("ScrollbackBufferSize", DefaultScrollbackBufferSize).toUInt());

    //Check command line
    QStringList slArgs = QCoreApplication::arguments();
    unsigned char chi = 1;
    bool bArgCom = false;
    bool bArgNoConnect = false;
    bool bStartScript = false;
    while (chi < slArgs.length())
    {
        if (slArgs[chi].toUpper() == "DUPLICATE")
        {
            //Duplicate window so move to the top
            this->activateWindow();
            this->raise();
        }
        else if (slArgs[chi].left(4).toUpper() == "COM=")
        {
            //Set com port
            QString strPort = slArgs[chi].right(slArgs[chi].length()-4);
#ifdef _WIN32
            if (strPort.left(3) != "COM")
            {
                //Prepend COM for UwTerminal shortcut compatibility
                strPort.prepend("COM");
            }
#endif
            ui->combo_COM->setCurrentText(strPort);
            bArgCom = true;

            //Update serial port info
            on_combo_COM_currentIndexChanged(0);
        }
        else if (slArgs[chi].left(5).toUpper() == "BAUD=")
        {
            //Set baud rate
            ui->combo_Baud->setCurrentText(slArgs[chi].right(slArgs[chi].length()-5));
        }
        else if (slArgs[chi].left(5).toUpper() == "STOP=")
        {
            //Set stop bits
            if (slArgs[chi].right(1) == "1")
            {
                //One
                ui->combo_Stop->setCurrentIndex(0);
            }
            else if (slArgs[chi].right(1) == "2")
            {
                //Two
                ui->combo_Stop->setCurrentIndex(1);
            }
        }
        else if (slArgs[chi].left(5).toUpper() == "DATA=")
        {
            //Set data bits
            if (slArgs[chi].right(1) == "7")
            {
                //Seven
                ui->combo_Data->setCurrentIndex(0);
            }
            else if (slArgs[chi].right(1).toUpper() == "8")
            {
                //Eight
                ui->combo_Data->setCurrentIndex(1);
            }
        }
        else if (slArgs[chi].left(4).toUpper() == "PAR=")
        {
            //Set parity
            uint value = QStringView(slArgs[chi]).right(1).toUInt();
            if (value < 3)
            {
                ui->combo_Parity->setCurrentIndex(value);
            }
        }
        else if (slArgs[chi].left(5).toUpper() == "FLOW=")
        {
            //Set flow control
            uint value = QStringView(slArgs[chi]).right(1).toUInt();
            if (value < 3)
            {
                //Valid
                ui->combo_Handshake->setCurrentIndex(value);
            }
        }
        else if (slArgs[chi].left(7).toUpper() == "ENDCHR=")
        {
            //Sets the end of line character
            if (slArgs[chi].right(1) == "0")
            {
                //CR
                ui->radio_LCR->setChecked(true);
            }
            else if (slArgs[chi].right(1) == "1")
            {
                //LF
                ui->radio_LLF->setChecked(true);
            }
            else if (slArgs[chi].right(1) == "2")
            {
                //CRLF
                ui->radio_LCRLF->setChecked(true);
            }
        }
        else if (slArgs[chi].left(10).toUpper() == "LOCALECHO=")
        {
            //Enable or disable local echo
            if (slArgs[chi].right(1) == "0")
            {
                //Off
                ui->check_Echo->setChecked(false);
            }
            else if (slArgs[chi].right(1) == "1")
            {
                //On (default)
                ui->check_Echo->setChecked(true);
            }
        }
        else if (slArgs[chi].left(9).toUpper() == "LINEMODE=")
        {
            //Enable or disable line mode
            if (slArgs[chi].right(1) == "0")
            {
                //Off
                ui->check_Line->setChecked(false);
                on_check_Line_stateChanged();
            }
            else if (slArgs[chi].right(1) == "1")
            {
                //On (default)
                ui->check_Line->setChecked(true);
                on_check_Line_stateChanged();
            }
        }
        else if (slArgs[chi].toUpper() == "LOG")
        {
            //Enables logging
            ui->check_LogEnable->setChecked(true);
        }
        else if (slArgs[chi].toUpper() == "LOG+")
        {
            //Enables appending to the previous log file instead of erasing
            ui->check_LogAppend->setChecked(true);
        }
        else if (slArgs[chi].left(4).toUpper() == "LOG=")
        {
            //Specifies log filename
            ui->edit_LogFile->setText(slArgs[chi].mid(4, -1));
        }
        else if (slArgs[chi].toUpper() == "SHOWCRLF")
        {
            //Displays \t, \r, \n etc. as \t, \r, \n instead of [tab], [new line], [carriage return]
            ui->check_ShowCLRF->setChecked(true);
        }
        else if (slArgs[chi].toUpper() == "NOCONNECT")
        {
            //Connect to device at startup
            bArgNoConnect = true;
        }
#ifndef SKIPAUTOMATIONFORM
        else if (slArgs[chi].toUpper() == "AUTOMATION")
        {
            //Show automation window
            if (guaAutomationForm == 0)
            {
                //Initialise automation popup
                guaAutomationForm = new UwxAutomation(this);

                //Populate window handles for automation object
                guaAutomationForm->SetPopupHandle(gpmErrorForm);

                //Update automation form with connection status
                guaAutomationForm->ConnectionChange(gspSerialPort.isOpen());

                //Connect signals
                connect(guaAutomationForm, SIGNAL(SendData(QByteArray,bool,bool)), this, SLOT(MessagePass(QByteArray,bool,bool)));

                //Give focus to the first line
                guaAutomationForm->SetFirstLineFocus();

                //Show form
                guaAutomationForm->show();
            }
        }
        else if (slArgs[chi].left(15).toUpper() == "AUTOMATIONFILE=")
        {
            //Load automation file
            if (guaAutomationForm == 0)
            {
                //Initialise automation popup
                guaAutomationForm = new UwxAutomation(this);

                //Populate window handles for automation object
                guaAutomationForm->SetPopupHandle(gpmErrorForm);

                //Update automation form with connection status
                guaAutomationForm->ConnectionChange(gspSerialPort.isOpen());

                //Connect signals
                connect(guaAutomationForm, SIGNAL(SendData(QByteArray,bool,bool)), this, SLOT(MessagePass(QByteArray,bool,bool)));
            }

            //Load file
            guaAutomationForm->LoadFile(slArgs[chi].right(slArgs[chi].size()-15));
        }
#endif
#ifndef SKIPSCRIPTINGFORM
        else if (slArgs[chi].toUpper() == "SCRIPTING")
        {
            //Show scripting window
            if (gusScriptingForm == 0)
            {
                //Initialise scripting form
                gusScriptingForm = new UwxScripting(this);

                //Populate window handles for automation object
                gusScriptingForm->SetPopupHandle(gpmErrorForm);

                //Send the AuTerm version string
                gusScriptingForm->SetAuTermVersion(UwVersion);

                //Set last directory
                gusScriptingForm->SetScriptLastDirectory(&gstrLastFilename[FilenameIndexScripting]);

                //Connect the message passing signal and slot
                connect(gusScriptingForm, SIGNAL(SendData(QByteArray,bool,bool)), this, SLOT(MessagePass(QByteArray,bool,bool)));
                connect(gusScriptingForm, SIGNAL(ScriptStartRequest()), this, SLOT(ScriptStartRequest()));
                connect(gusScriptingForm, SIGNAL(ScriptFinished()), this, SLOT(ScriptFinished()));
                connect(gusScriptingForm, SIGNAL(UpdateScriptLastDirectory(const QString*)), this, SLOT(ScriptingFileSelected(const QString*)));

                if (gspSerialPort.isOpen())
                {
                    //Tell the form that the serial port is open
                    gusScriptingForm->SerialPortStatus(true);
                }

                //Show form
                gusScriptingForm->show();
                gusScriptingForm->SetEditorFocus();
            }
        }
        else if (slArgs[chi].left(11).toUpper() == "SCRIPTFILE=")
        {
            //Load a script file
            if (gusScriptingForm != 0)
            {
                QString strFilename = slArgs[chi].right(slArgs[chi].size()-11);
                gusScriptingForm->LoadScriptFile(&strFilename);
            }
        }
        else if (slArgs[chi].left(13).toUpper() == "SCRIPTACTION=")
        {
            //Action for scripting form
            if (slArgs[chi].right(slArgs[chi].size()-13) == "1" && gusScriptingForm != 0)
            {
                //Enable script execution
                bStartScript = true;
            }
        }
        else if (slArgs[chi].left(6).toUpper() == "TITLE=")
        {
            //Set window title
            QString strWindowTitle = slArgs[chi].right(slArgs[chi].size()-6);
            if (strWindowTitle.length() > ui->edit_Title->maxLength())
            {
                strWindowTitle.remove(ui->edit_Title->maxLength(), (strWindowTitle.length() - ui->edit_Title->maxLength()));
            }

            ui->edit_Title->setText(strWindowTitle);
            on_edit_Title_textEdited(NULL);
        }
#endif
        ++chi;
    }

    if (bArgCom == true && bArgNoConnect == false)
    {
        //Enough information to connect!
        OpenDevice();
        if (bStartScript == true)
        {
            //Start script execution request
            ScriptStartRequest();
        }
    }

    //(Unlisted option) Setup display buffer automatic trimming
    gbAutoTrimDBuffer = gpTermSettings->value("AutoTrimDBuffer", DefaultAutoDTrimBuffer).toBool();
    gintAutoTrimBufferDThreshold = gpTermSettings->value("AutoTrimDBufferThreshold", DefaultAutoTrimDBufferThreshold).toULongLong();
    gintAutoTrimBufferDSize = gpTermSettings->value("AutoTrimDBufferSize", DefaultAutoTrimDBufferSize).toULongLong();

#ifdef __APPLE__
    //Show a warning to Mac users with the FTDI driver installed
    if ((QFile::exists("/System/Library/Extensions/FTDIUSBSerialDriver.kext") || QFile::exists("/Library/Extensions/FTDIUSBSerialDriver.kext")) && gpTermSettings->value("MacFTDIDriverWarningShown").isNull())
    {
        //FTDI driver detected and warning has not been shown, show warning
        gpTermSettings->setValue("MacFTDIDriverWarningShown", 1);
        QString strMessage = tr("Warning: The Mac FTDI VCP driver has been detected on your system. There is a known issue with this driver that can cause your system to crash if the serial port is closed and the buffer is not empty.\r\n\r\nIf you experience this issue, it is recommended that you remove the FTDI driver and use the apple VCP driver instead. Instructions to do this are available from the FTDI website (follow the uninstall section): http://www.ftdichip.com/Support/Documents/AppNotes/AN_134_FTDI_Drivers_Installation_Guide_for_MAC_OSX.pdf\r\n\r\nThis message will not be shown again.");
        gpmErrorForm->show();
        gpmErrorForm->SetMessage(&strMessage);
    }
#endif

    gbAppStarted = true;
}

//=============================================================================
//=============================================================================
MainWindow::~MainWindow()
{
    //Disconnect all signals
    disconnect(this, SLOT(close()));
    disconnect(this, SLOT(EnterPressed()));
    disconnect(this, SLOT(KeyPressed(int,QChar)));
    disconnect(this, SLOT(DroppedFile(QString)));
    disconnect(this, SLOT(MenuSelected(QAction*)));
    disconnect(this, SLOT(balloontriggered(QAction*)));
    disconnect(this, SLOT(ContextMenuClosed()));
    disconnect(this, SLOT(DevRespTimeout()));
    disconnect(this, SLOT(SerialStatusSlot()));
    disconnect(this, SLOT(SerialRead()));
    disconnect(this, SLOT(SerialError(QSerialPort::SerialPortError)));
    disconnect(this, SLOT(SerialBytesWritten(qint64)));
    disconnect(this, SLOT(UpdateReceiveText()));
    disconnect(this, SLOT(SerialPortClosing()));
    disconnect(this, SLOT(BatchTimeoutSlot()));
//    disconnect(this, SLOT(replyFinished(QNetworkReply*)));
    disconnect(this, SLOT(MessagePass(QByteArray,bool,bool)));
#if SKIPSPEEDTEST != 1
    disconnect(this, SLOT(UpdateDisplayText()));
    disconnect(this, SLOT(UpdateSpeedTestValues()));
    disconnect(this, SLOT(OutputSpeedTestStats()));

    if (gtmrSpeedTestDelayTimer != 0)
    {
        if (gtmrSpeedTestDelayTimer->isActive())
        {
            //Stop timer
            gtmrSpeedTestDelayTimer->stop();
        }

        //Disconnect slots and delete timer
        disconnect(this, SLOT(SpeedTestStartTimer()));
        disconnect(this, SLOT(SpeedTestStopTimer()));
        delete gtmrSpeedTestDelayTimer;
    }
#endif

    if (gspSerialPort.isOpen() == true)
    {
        //Close serial connection before quitting
        gspSerialPort.close();
        gpSignalTimer->stop();
        for (int i = 0; i < plugin_list.length(); ++i)
        {
            plugin_list.at(i).plugin->serial_closed();
        }
        gbPluginHideTerminalOutput = false;
        gbPluginRunning = false;
    }

    if (gbMainLogEnabled == true)
    {
        //Close main log file before quitting
        gpMainLog->CloseLogFile();
    }

    //Close popups if open
    if (gpmErrorForm->isVisible())
    {
        //Close warning message
        gpmErrorForm->close();
    }
#ifndef SKIPAUTOMATIONFORM
    if (guaAutomationForm != 0)
    {
        if (guaAutomationForm->isVisible())
        {
            //Close automation form
            guaAutomationForm->close();
        }
        delete guaAutomationForm;
    }
#endif
#ifndef SKIPSCRIPTINGFORM
    if (gusScriptingForm != 0)
    {
        if (gusScriptingForm->isVisible())
        {
            //Close scripting form
            gusScriptingForm->close();
        }
        disconnect(this, SLOT(ScriptStartRequest()));
        disconnect(this, SLOT(ScriptFinished()));
        disconnect(this, SLOT(ScriptingFileSelected(const QString*)));
        delete gusScriptingForm;
    }
#endif
#ifndef SKIPERRORCODEFORM
    if (gecErrorCodeForm != 0)
    {
        if (gecErrorCodeForm->isVisible())
        {
            //Close error code form
            gecErrorCodeForm->close();
        }
        delete gecErrorCodeForm;
    }
#endif

    //Delete system tray object
    if (gbSysTrayEnabled == true)
    {
        gpSysTray->hide();
        delete gpSysTray;
    }

    //Clear up streaming data if opened
    if (gbTermBusy == true && gbStreamingFile == true)
    {
        gpStreamFileHandle->close();
        delete gpStreamFileHandle;
    }
    else if (gbTermBusy == true && gbStreamingBatch == true)
    {
        //Clear up batch
        gpStreamFileHandle->close();
        delete gpStreamFileHandle;
    }

#ifdef UseSSL
    if (sslcLairdSSLNew != NULL)
    {
        //Clear up (newer) SSL certificate
        delete sslcLairdSSLNew;
    }
#endif

#if 0
    if (gnmManager != 0)
    {
        //Clear up network manager
        delete gnmManager;
    }
#endif

    if (gstrUpdateCheckString != NULL)
    {
        //Clear up update checking string
        delete gstrUpdateCheckString;
    }

    //Release reserved memory buffers
    gbaDisplayBuffer.squeeze();
    gbaSpeedReceivedData.squeeze();

    //Clear up plugins
    int32_t i = 0;
    while (i < plugin_list.length())
    {
        disconnect(plugin_list.at(i).object, SIGNAL(show_message_box(QString)), gpmErrorForm, SLOT(show_message(QString)));
        disconnect(plugin_list.at(i).object, SIGNAL(plugin_set_status(bool,bool)), this, SLOT(plugin_set_status(bool,bool)));
        delete plugin_list.at(i).object;
        plugin_loader.unload();
        ++i;
    }

    //Delete variables
    delete gpMainLog;
    delete gpPredefinedDevice;
    delete gpTermSettings;
    delete gpErrorMessages;
    delete gpSignalTimer;
#if SKIPSPEEDTEST != 1
    delete gpSpeedMenu;
#endif
    delete gpBalloonMenu;
    delete gpSMenu4;
    delete gpSMenu3;
    delete gpSMenu2;
    delete gpSMenu1;
    delete gpMenu;
    delete gpEmptyCirclePixmap;
    delete gpRedCirclePixmap;
    delete gpGreenCirclePixmap;
    delete gpUw16Pixmap;
    delete gpUw32Pixmap;
    delete gpmErrorForm;
    delete ui;
}

//=============================================================================
//=============================================================================
void
MainWindow::closeEvent(
    QCloseEvent *
    )
{
    //Runs when the form is closed. Close child popups to exit the application
    if (gpmErrorForm->isVisible())
    {
        //Close warning message form
        gpmErrorForm->close();
    }
#ifndef SKIPAUTOMATIONFORM
    if (guaAutomationForm != 0 && guaAutomationForm->isVisible())
    {
        //Close automation form
        guaAutomationForm->close();
    }
#endif
#ifndef SKIPSCRIPTINGFORM
    if (gusScriptingForm != 0 && gusScriptingForm->isVisible())
    {
        //Close scripting form
        gusScriptingForm->close();
    }
#endif
#ifndef SKIPERRORCODEFORM
    if (gecErrorCodeForm != 0 && gecErrorCodeForm->isVisible())
    {
        //Close error code form
        gecErrorCodeForm->close();
    }
#endif

    //Close application
    QApplication::quit();
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Connect_clicked(
    )
{
    //Connect to COM port button clicked.
    OpenDevice();
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_TermClose_clicked(
    )
{
    if (ui->btn_TermClose->text() == "&Open Port" || ui->btn_TermClose->text() == "Open Port")
    {
        //Open connection
        OpenDevice();
    }
    else if (ui->btn_TermClose->text() == "C&lose Port" || ui->btn_TermClose->text() == "Close Port")
    {
        //Close, but first clear up from download/streaming
        gbTermBusy = false;
        gchTermMode = 0;
        gchTermMode2 = 0;
        ui->btn_Cancel->setEnabled(false);
        if (gbStreamingFile == true)
        {
            //Clear up file stream
            gtmrStreamTimer.invalidate();
            gbStreamingFile = false;
            gpStreamFileHandle->close();
            delete gpStreamFileHandle;
        }
        else if (gbStreamingBatch == true)
        {
            //Clear up batch
            gtmrStreamTimer.invalidate();
            gtmrBatchTimeoutTimer.stop();
            gbStreamingBatch = false;
            gpStreamFileHandle->close();
            delete gpStreamFileHandle;
            gbaBatchReceive.clear();
        }
#if SKIPSPEEDTEST != 1
        else if (gbSpeedTestRunning == true)
        {
            //Clear up speed testing
            if (gtmrSpeedTestDelayTimer != 0)
            {
                //Clean up timer
                disconnect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStartTimer()));
                disconnect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStopTimer()));
                delete gtmrSpeedTestDelayTimer;
                gtmrSpeedTestDelayTimer = 0;
            }

            ui->btn_SpeedStartStop->setEnabled(false);
            ui->check_SpeedSyncReceive->setEnabled(true);
            ui->combo_SpeedDataType->setEnabled(true);
            if (ui->combo_SpeedDataType->currentIndex() == 1)
            {
                //Enable string options
                ui->edit_SpeedTestData->setEnabled(true);
                ui->check_SpeedStringUnescape->setEnabled(true);
            }

            //Update values
            OutputSpeedTestAvgStats((gtmrSpeedTimer.nsecsElapsed() < 1000000000LL ? 1000000000LL : gtmrSpeedTimer.nsecsElapsed()/1000000000LL));

            //Set speed test as no longer running
            gchSpeedTestMode = SpeedModeInactive;
            gbSpeedTestRunning = false;

            if (gtmrSpeedTimer.isValid())
            {
                //Invalidate speed test timer
                gtmrSpeedTimer.invalidate();
            }
            if (gtmrSpeedTestStats.isActive())
            {
                //Stop stats update timer
                gtmrSpeedTestStats.stop();
            }
            if (gtmrSpeedTestStats10s.isActive())
            {
                //Stop 10 second stats update timer
                gtmrSpeedTestStats10s.stop();
            }

            //Clear buffers
            gbaSpeedMatchData.clear();
            gbaSpeedReceivedData.clear();

            //Show finished message in status bar
            ui->statusBar->showMessage("Speed testing failed due to serial port being closed.");
        }
#endif

        //Close the serial port
        if (gspSerialPort.isOpen() == true)
        {
            gspSerialPort.clear();
            gspSerialPort.close();
            for (int i = 0; i < plugin_list.length(); ++i)
            {
                plugin_list.at(i).plugin->serial_closed();
            }
            gbPluginHideTerminalOutput = false;
            gbPluginRunning = false;
        }
        gpSignalTimer->stop();

        //Disable active checkboxes
        ui->check_Break->setEnabled(false);
        ui->check_DTR->setEnabled(false);
        ui->check_Echo->setEnabled(false);
        ui->check_Line->setEnabled(false);
        ui->check_RTS->setEnabled(false);
#if SKIPSPEEDTEST != 1
        ui->check_SpeedDTR->setEnabled(false);
        ui->check_SpeedRTS->setEnabled(false);
#endif

        //Change status message
        ui->statusBar->showMessage("");

        //Change button text
        ui->btn_TermClose->setText("&Open Port");
#if SKIPSPEEDTEST != 1
        ui->btn_SpeedClose->setText("&Open Port");
#endif

#ifndef SKIPAUTOMATIONFORM
        //Notify automation form
        if (guaAutomationForm != 0)
        {
            guaAutomationForm->ConnectionChange(false);
        }
#endif

        //Disallow file drops
        setAcceptDrops(false);

        //Close log file if open
        if (gpMainLog->IsLogOpen() == true)
        {
            gpMainLog->CloseLogFile();
        }

        //Enable log options
        ui->edit_LogFile->setEnabled(true);
        ui->check_LogEnable->setEnabled(true);
        ui->check_LogAppend->setEnabled(true);
        ui->btn_LogFileSelect->setEnabled(true);

#if SKIPSPEEDTEST != 1
        //Disable speed testing
        ui->btn_SpeedStartStop->setEnabled(false);
#endif
    }

    //Update images
    UpdateImages();
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Refresh_clicked(
    )
{
    //Refresh the list of serial ports
    RefreshSerialDevices();
}

//=============================================================================
//=============================================================================
void
MainWindow::RefreshSerialDevices(
    )
{
    //Clears and refreshes the list of serial devices
    QString strPrev = "";
    QRegularExpression reTempRE("^(\\D*?)(\\d+)$");
    QList<int> lstEntries;
    lstEntries.clear();

    if (ui->combo_COM->count() > 0)
    {
        //Remember previous option
        strPrev = ui->combo_COM->currentText();
    }
    ui->combo_COM->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QRegularExpressionMatch remTempREM = reTempRE.match(info.portName());
        if (remTempREM.hasMatch() == true)
        {
            //Can sort this item
            int i = lstEntries.count()-1;
            while (i >= 0)
            {
                if (remTempREM.captured(2).toInt() > lstEntries[i])
                {
                    //Found correct order position, add here
                    ui->combo_COM->insertItem(i+1, info.portName());
                    lstEntries.insert(i+1, remTempREM.captured(2).toInt());
                    i = -1;
                }
                --i;
            }

            if (i == -1)
            {
                //Position not found, add to beginning
                ui->combo_COM->insertItem(0, info.portName());
                lstEntries.insert(0, remTempREM.captured(2).toInt());
            }
        }
        else
        {
            //Cannot sort this item
            ui->combo_COM->insertItem(ui->combo_COM->count(), info.portName());
        }
    }

    //Search for previous item if one was selected
    if (strPrev == "")
    {
        //Select first item
        ui->combo_COM->setCurrentIndex(0);
    }
    else
    {
        //Search for previous
        int i = 0;
        while (i < ui->combo_COM->count())
        {
            if (ui->combo_COM->itemText(i) == strPrev)
            {
                //Found previous item
                ui->combo_COM->setCurrentIndex(i);
                break;
            }
            ++i;
        }
    }

    //Update serial port info
    on_combo_COM_currentIndexChanged(0);
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_TermClear_clicked(
    )
{
    //Clears the screen of the terminal tab
    ui->text_TermEditData->ClearDatIn();
}

//=============================================================================
//=============================================================================
void
MainWindow::SerialRead(
    )
{
//    qDebug() << "Received";
    //Update the last received field
    if ((gtmrPortOpened.elapsed() / 1000) > gintLastSerialTimeUpdate)
    {
        ui->label_LastRx->setText(QDateTime::currentDateTime().toString("dd/MM @ hh:mm:ss"));
        gintLastSerialTimeUpdate = (gtmrPortOpened.elapsed() / 1000);
    }

    //Read the data into a buffer and copy it to edit for the display data
#if SKIPSPEEDTEST != 1
    if (gbSpeedTestRunning == true)
    {
        //Serial test is running, pass to speed test function
        SpeedTestReceive();
        return;
    }
#endif
    QByteArray baOrigData = gspSerialPort.readAll();

    if (gbPluginHideTerminalOutput == false || gbPluginRunning == false)
    {
        //Speed test is not running
//qDebug() << baOrigData;

#ifndef SKIPSCRIPTINGFORM
        if (gusScriptingForm != 0 && gbScriptingRunning == true)
        {
            gusScriptingForm->SerialPortData(&baOrigData);
        }
#endif

        if (gbTermBusy == false)
        {
            //Update the display with the data
            QByteArray baDispData = baOrigData;

            //Check if this should be passed to the logger
            if (ui->check_LogEnable->isChecked())
            {
                //Add to log
                gpMainLog->WriteRawLogData(baOrigData);
            }

            if (ui->check_ShowCLRF->isChecked() == true)
            {
                //Escape \t, \r and \n
                baDispData.replace("\t", "\\t").replace("\r", "\\r").replace("\n", "\\n");
            }

            //Replace unprintable characters
//            baDispData.replace('\0', "\\00").replace("\x01", "\\01").replace("\x02", "\\02").replace("\x03", "\\03").replace("\x04", "\\04").replace("\x05", "\\05").replace("\x06", "\\06").replace("\x07", "\\07").replace("\x08", "\\08").replace("\x0b", "\\0B").replace("\x0c", "\\0C").replace("\x0e", "\\0E").replace("\x0f", "\\0F").replace("\x10", "\\10").replace("\x11", "\\11").replace("\x12", "\\12").replace("\x13", "\\13").replace("\x14", "\\14").replace("\x15", "\\15").replace("\x16", "\\16").replace("\x17", "\\17").replace("\x18", "\\18").replace("\x19", "\\19").replace("\x1a", "\\1a").replace("\x1b", "\\1b").replace("\x1c", "\\1c").replace("\x1d", "\\1d").replace("\x1e", "\\1e").replace("\x1f", "\\1f");

            //Update display buffer
            gbaDisplayBuffer.append(baDispData);
            if (!gtmrTextUpdateTimer.isActive())
            {
                gtmrTextUpdateTimer.start();
            }

            if (gbLoopbackMode == true)
            {
                //Loopback enabled, send this data back
                gspSerialPort.write(baOrigData);
                gintQueuedTXBytes += baOrigData.length();
                gpMainLog->WriteRawLogData(baOrigData);
                gbaDisplayBuffer.append(baDispData);
            }
        }

        //Update number of recieved bytes
        gintRXBytes = gintRXBytes + baOrigData.length();
        ui->label_TermRx->setText(QString::number(gintRXBytes));

        //Send next chunk of batch data if enabled
        StreamBatchContinue(&baOrigData);
    }

    if (gbPluginRunning == true)
    {
        //A plugin is running, siphon data to it
//TODO: limit to the running plugin only
        for (int i = 0; i < plugin_list.length(); ++i)
        {
            plugin_list.at(i).plugin->serial_receive(&baOrigData);
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::StreamBatchContinue(
    QByteArray *baOrigData
    )
{
    if (gbStreamingBatch == true)
    {
        //Batch stream in progress
        gbaBatchReceive += *baOrigData;
        if (gbaBatchReceive.indexOf("\n00\r") != -1)
        {
            //Success code, next statement
            if (gpStreamFileHandle->atEnd())
            {
                //Finished sending
                FinishBatch(false);
            }
            else
            {
                //Send more data
                QByteArray baFileData = gpStreamFileHandle->readLine().replace("\n", "").replace("\r", "");
                gspSerialPort.write(baFileData);
                gintQueuedTXBytes += baFileData.length();
                DoLineEnd();
                gpMainLog->WriteLogData(QString(baFileData).append("\n"));
                gtmrBatchTimeoutTimer.start(BatchTimeout);
                ++gintStreamBytesRead;

                //Update the display buffer
                gbaDisplayBuffer.append(baFileData);
                if (!gtmrTextUpdateTimer.isActive())
                {
                    gtmrTextUpdateTimer.start();
                }
            }
            gbaBatchReceive.clear();
        }
        else if (gbaBatchReceive.indexOf("\n01\t") != -1 && gbaBatchReceive.indexOf("\r", gbaBatchReceive.indexOf("\n01\t")+4) != -1)
        {
            //Failure code
            QRegularExpression reTempRE("\t([a-zA-Z0-9]{1,9})(\t|\r)");
            QRegularExpressionMatch remTempREM = reTempRE.match(gbaBatchReceive);
            if (remTempREM.hasMatch() == true)
            {
                //Got the error code
                gbaDisplayBuffer.append("\nError during batch command, error code: ").append(remTempREM.captured(1).toUtf8()).append("\n");

                //Lookup error code
                bool bTmpBool;
                unsigned int ErrCode = QString("0x").append(remTempREM.captured(1)).toUInt(&bTmpBool, 16);
                if (bTmpBool == true)
                {
                    //Converted
                    LookupErrorCode(ErrCode);
                }
            }
            else
            {
                //Unknown error code
                gbaDisplayBuffer.append("\nError during batch command, unknown error code.\n");
            }
            if (!gtmrTextUpdateTimer.isActive())
            {
                gtmrTextUpdateTimer.start();
            }

            //Show status message
            ui->statusBar->showMessage(QString("Failed sending batch file at line ").append(QString::number(gintStreamBytesRead)));

            //Clear up and cancel timer
            gtmrBatchTimeoutTimer.stop();
            gbTermBusy = false;
            gbStreamingBatch = false;
            gchTermMode = 0;
            gpStreamFileHandle->close();
            delete gpStreamFileHandle;
            gbaBatchReceive.clear();
            ui->btn_Cancel->setEnabled(false);
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_text_TermEditData_customContextMenuRequested(
    const QPoint &pos
    )
{
    //Creates the custom context menu
    gpMenu->popup(ui->text_TermEditData->viewport()->mapToGlobal(pos));
    ui->text_TermEditData->mbContextMenuOpen = true;
}

//=============================================================================
//=============================================================================
void
MainWindow::MenuSelected(
    QAction* qaAction
    )
{
    //Runs when a menu item is selected
    int intItem = qaAction->data().toInt();

    if (intItem == MenuActionErrorHex)
    {
        //Shows a meaning for the error code selected (number in hex)
        bool bTmpBool;
        unsigned int uiErrCode = QString("0x").append(ui->text_TermEditData->textCursor().selection().toPlainText()).toUInt(&bTmpBool, 16);
        if (bTmpBool == true)
        {
            //Converted
            LookupErrorCode(uiErrCode);
        }
    }
    else if (intItem == MenuActionErrorInt)
    {
        //Shows a meaning for the error code selected (number as int)
        LookupErrorCode(ui->text_TermEditData->textCursor().selection().toPlainText().toInt());
    }
    else if (intItem == MenuActionLoopback && gbTermBusy == false && gbSpeedTestRunning == false)
    {
        //Enable/disable loopback mode
        SetLoopBackMode(!gbLoopbackMode);
    }
    else if (intItem == MenuActionStreamFile && gbTermBusy == false && gbSpeedTestRunning == false)
    {
        //Stream out a file
        if (gspSerialPort.isOpen() == true && gbLoopbackMode == false && gbTermBusy == false)
        {
            //Not currently busy
#ifndef SKIPAUTOMATIONFORM
            if (guaAutomationForm != 0)
            {
                guaAutomationForm->TempAlwaysOnTop(0);
            }
#endif
            QString strFilename = QFileDialog::getOpenFileName(this, tr("Open File To Stream"), gstrLastFilename[FilenameIndexOthers], tr("Text Files (*.txt);;All Files (*.*)"));
#ifndef SKIPAUTOMATIONFORM
            if (guaAutomationForm != 0)
            {
                guaAutomationForm->TempAlwaysOnTop(1);
            }
#endif

            if (strFilename.length() > 1)
            {
                //Set last directory config
                gstrLastFilename[FilenameIndexOthers] = strFilename;
                gpTermSettings->setValue("LastOtherFileDirectory", SplitFilePath(strFilename).at(0));

                //File was selected - start streaming it out
                gpStreamFileHandle = new QFile(strFilename);

                if (!gpStreamFileHandle->open(QIODevice::ReadOnly))
                {
                    //Unable to open file
                    QString strMessage = tr("Error during file streaming: Access to selected file is denied: ").append(strFilename);
                    gpmErrorForm->show();
                    gpmErrorForm->SetMessage(&strMessage);
                    return;
                }

                //We're now busy
                gbTermBusy = true;
                gbStreamingFile = true;
                gchTermMode = 50;
                ui->btn_Cancel->setEnabled(true);

                //Save the size of the file
                gintStreamBytesSize = gpStreamFileHandle->size();
                gintStreamBytesRead = 0;
                gintStreamBytesProgress = StreamProgress;

                //Start a timer
                gtmrStreamTimer.start();

                //Reads out each block
                QByteArray baFileData = gpStreamFileHandle->read(FileReadBlock);
                gspSerialPort.write(baFileData);
                gintQueuedTXBytes += baFileData.size();
                gintStreamBytesRead = baFileData.length();
                gpMainLog->WriteLogData(QString(baFileData).append("\n"));
                if (gpStreamFileHandle->atEnd())
                {
                    //Finished sending
                    FinishStream(false);
                }
            }
        }
    }
    else if (intItem == MenuActionFont)
    {
        //Change font
        bool bTmpBool;
        QFont fntTmpFnt = QFontDialog::getFont(&bTmpBool, ui->text_TermEditData->font(), this);
        if (bTmpBool == true)
        {
            //Set font and re-adjust tab spacing
            QFontMetrics tmTmpFM(fntTmpFnt);
            ui->text_TermEditData->setFont(fntTmpFnt);
            ui->text_TermEditData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);
            ui->text_LogData->setFont(fntTmpFnt);
            ui->text_LogData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);
#if SKIPSPEEDTEST != 1
            ui->text_SpeedEditData->setFont(fntTmpFnt);
            ui->text_SpeedEditData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);
#endif

            //Update saved customisation settings
            UpdateCustomisation(false);
        }
    }
    else if (intItem == MenuActionTextColour)
    {
        //Change text colour
        QPalette palTmp = ui->text_TermEditData->palette();
        palTmp.setColor(QPalette::Active, QPalette::Text, QColorDialog::getColor(palTmp.text().color(), this, "Select text colour"));
        if (palTmp.color(QPalette::Active, QPalette::Text).isValid())
        {
            //Update text colour
            palTmp.setColor(QPalette::Inactive, QPalette::Text, palTmp.color(QPalette::Active, QPalette::Text));
            ui->text_TermEditData->setPalette(palTmp);
            ui->text_LogData->setPalette(palTmp);
#if SKIPSPEEDTEST != 1
            ui->text_SpeedEditData->setPalette(palTmp);
#endif

            //Update saved customisation settings
            UpdateCustomisation(false);
        }
    }
    else if (intItem == MenuActionBackground)
    {
        //Change background colour
        QPalette palTmp = ui->text_TermEditData->palette();
        palTmp.setColor(QPalette::Active, QPalette::Base, QColorDialog::getColor(palTmp.base().color(), this, "Select background colour"));
        if (palTmp.color(QPalette::Active, QPalette::Base).isValid())
        {
            //Update background colour
            palTmp.setColor(QPalette::Inactive, QPalette::Base, palTmp.color(QPalette::Active, QPalette::Base));
            ui->text_TermEditData->setPalette(palTmp);
            ui->text_LogData->setPalette(palTmp);
#if SKIPSPEEDTEST != 1
            ui->text_SpeedEditData->setPalette(palTmp);
#endif

            //Update saved customisation settings
            UpdateCustomisation(false);
        }
    }
    else if (intItem == MenuActionRestoreDefaults)
    {
        //Restore customisations to default: colour
        QPalette palTmp = ui->text_TermEditData->palette();

#ifdef _MSC_BUILD
        //MSVC isn't a good compiler
        palTmp.setColor(QPalette::Active, QPalette::Text, QColor("White"));
        palTmp.setColor(QPalette::Active, QPalette::Base, QColor("Black"));
#else
        palTmp.setColor(QPalette::Active, QPalette::Text, QColorConstants::White);
        palTmp.setColor(QPalette::Active, QPalette::Base, QColorConstants::Black);
#endif

        ui->text_TermEditData->setPalette(palTmp);
        ui->text_LogData->setPalette(palTmp);

        //And font
        QFont fntTmpFnt = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        QFontMetrics tmTmpFM(fntTmpFnt);
        ui->text_TermEditData->setFont(fntTmpFnt);
        ui->text_TermEditData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);
        ui->text_LogData->setFont(fntTmpFnt);
        ui->text_LogData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);
#if SKIPSPEEDTEST != 1
        ui->text_SpeedEditData->setFont(fntTmpFnt);
        ui->text_SpeedEditData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);
        ui->text_SpeedEditData->setPalette(palTmp);
#endif

        //Clear saved customisation settings
        UpdateCustomisation(true);
    }
#ifndef SKIPAUTOMATIONFORM
    else if (intItem == MenuActionAutomation)
    {
        //Show automation window
        if (guaAutomationForm == 0)
        {
            //Initialise automation popup
            guaAutomationForm = new UwxAutomation(this);

            //Populate window handles for automation object
            guaAutomationForm->SetPopupHandle(gpmErrorForm);

            //Update automation form with connection status
            guaAutomationForm->ConnectionChange(gspSerialPort.isOpen());

            //Connect signals
            connect(guaAutomationForm, SIGNAL(SendData(QByteArray,bool,bool)), this, SLOT(MessagePass(QByteArray,bool,bool)));

            //Give focus to the first line
            guaAutomationForm->SetFirstLineFocus();
        }
        guaAutomationForm->show();
    }
#endif
#ifndef SKIPSCRIPTINGFORM
    else if (intItem == MenuActionScripting)
    {
        //Show scripting window
        if (gusScriptingForm == 0)
        {
            //Initialise scripting form
            gusScriptingForm = new UwxScripting(this);

            //Populate window handles for automation object
            gusScriptingForm->SetPopupHandle(gpmErrorForm);

            //Send the AuTerm version string
            gusScriptingForm->SetAuTermVersion(UwVersion);

            //Set last directory
            gusScriptingForm->SetScriptLastDirectory(&gstrLastFilename[FilenameIndexScripting]);

            //Connect the message passing signal and slot
            connect(gusScriptingForm, SIGNAL(SendData(QByteArray,bool,bool)), this, SLOT(MessagePass(QByteArray,bool,bool)));
            connect(gusScriptingForm, SIGNAL(ScriptStartRequest()), this, SLOT(ScriptStartRequest()));
            connect(gusScriptingForm, SIGNAL(ScriptFinished()), this, SLOT(ScriptFinished()));
            connect(gusScriptingForm, SIGNAL(UpdateScriptLastDirectory(const QString*)), this, SLOT(ScriptingFileSelected(const QString*)));

            if (gspSerialPort.isOpen())
            {
                //Tell the form that the serial port is open
                gusScriptingForm->SerialPortStatus(true);
            }
        }
        gusScriptingForm->show();
        gusScriptingForm->SetEditorFocus();
    }
#endif
    else if (intItem == MenuActionBatch && gbTermBusy == false && gbSpeedTestRunning == false)
    {
        //Start a Batch file script
        if (gspSerialPort.isOpen() == true && gbLoopbackMode == false && gbTermBusy == false)
        {
            //Not currently busy
#ifndef SKIPAUTOMATIONFORM
            if (guaAutomationForm != 0)
            {
                guaAutomationForm->TempAlwaysOnTop(0);
            }
#endif
            QString strFilename = QFileDialog::getOpenFileName(this, tr("Open Batch File"), gstrLastFilename[FilenameIndexOthers], tr("Text Files (*.txt);;All Files (*.*)"));
#ifndef SKIPAUTOMATIONFORM
            if (guaAutomationForm != 0)
            {
                guaAutomationForm->TempAlwaysOnTop(1);
            }
#endif

            if (strFilename.length() > 1)
            {
                //Set last directory config
                gstrLastFilename[FilenameIndexOthers] = strFilename;
                gpTermSettings->setValue("LastOtherFileDirectory", SplitFilePath(strFilename).at(0));

                //File selected
                gpStreamFileHandle = new QFile(strFilename);

                if (!gpStreamFileHandle->open(QIODevice::ReadOnly))
                {
                    //Unable to open file
                    QString strMessage = tr("Error during batch streaming: Access to selected file is denied: ").append(strFilename);
                    gpmErrorForm->show();
                    gpmErrorForm->SetMessage(&strMessage);
                    return;
                }

                //We're now busy
                gbTermBusy = true;
                gbStreamingBatch = true;
                gchTermMode = 50;
                gbaBatchReceive.clear();
                ui->btn_Cancel->setEnabled(true);

                //Start a timer
                gtmrStreamTimer.start();

                //Reads out first block
                QByteArray baFileData = gpStreamFileHandle->readLine().replace("\n", "").replace("\r", "");
                gspSerialPort.write(baFileData);
                gintQueuedTXBytes += baFileData.size();
                DoLineEnd();
                gpMainLog->WriteLogData(QString(baFileData).append("\n"));
                gintStreamBytesRead = 1;

                //Update the display buffer
                gbaDisplayBuffer.append(baFileData);
                if (!gtmrTextUpdateTimer.isActive())
                {
                    gtmrTextUpdateTimer.start();
                }

                //Start a timeout timer
                gtmrBatchTimeoutTimer.start(BatchTimeout);
            }
        }
    }
    else if (intItem == MenuActionClearDisplay)
    {
        //Clear display
        ui->text_TermEditData->ClearDatIn();
    }
    else if (intItem == MenuActionClearRxTx)
    {
        //Clear counts
        gintRXBytes = 0;
        gintTXBytes = 0;
        ui->label_TermRx->setText(QString::number(gintRXBytes));
        ui->label_TermTx->setText(QString::number(gintTXBytes));
    }
    else if (intItem == MenuActionCopy)
    {
        //Copy selected data
        QApplication::clipboard()->setText(ui->text_TermEditData->textCursor().selection().toPlainText());
    }
    else if (intItem == MenuActionCopyAll)
    {
        //Copy all data
        QApplication::clipboard()->setText(ui->text_TermEditData->toPlainText());
    }
    else if (intItem == MenuActionPaste)
    {
        //Paste data from clipboard
        ui->text_TermEditData->AddDatOutText(QApplication::clipboard()->text());
    }
    else if (intItem == MenuActionSelectAll)
    {
        //Select all text
        ui->text_TermEditData->selectAll();
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::balloontriggered(
    QAction* qaAction
    )
{
    //Runs when a balloon menu item is selected
    int intItem = qaAction->data().toInt();
    if (intItem == BalloonActionShow)
    {
        //Make AuTerm the active window
#ifdef TARGET_OS_MAC
        //Bugfix for mac (icon vanishes when clicked)
        gpSysTray->setIcon(QIcon(*gpUw16Pixmap));
#endif
        this->raise();
        this->activateWindow();
    }
    else if (intItem == BalloonActionExit)
    {
        //Exit
        QApplication::quit();
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::EnterPressed(
    )
{
    //Enter pressed in line mode
    if (gspSerialPort.isOpen())
    {
        if (gbTermBusy == false)
        {
            if (gbLoopbackMode == false)
            {
                QByteArray baTmpBA = ui->text_TermEditData->GetDatOut()->replace("\r", "\n").replace("\n", (ui->radio_LCR->isChecked() ? "\r" : ui->radio_LLF->isChecked() ? "\n" : ui->radio_LCRLF->isChecked() ? "\r\n" : "")).toUtf8();
                gspSerialPort.write(baTmpBA);
                gintQueuedTXBytes += baTmpBA.size();

                //Add to log
                gpMainLog->WriteLogData(baTmpBA);

                //Deal with line ending
                if ((ui->radio_LCR->isChecked() && baTmpBA.right(1) != "\r") || (ui->radio_LLF->isChecked() && baTmpBA.right(1) != "\n") || (ui->radio_LCRLF->isChecked() && baTmpBA.right(2) != "\r\n"))
                {
                    //Only add line ending if not present
                    DoLineEnd();

                    //Add to log
                    gpMainLog->WriteLogData("\n");
                }
            }
            else if (gbLoopbackMode == true)
            {
                //Loopback is enabled
                gbaDisplayBuffer.append("\n[Cannot send: Loopback mode is enabled.]\n");
                if (!gtmrTextUpdateTimer.isActive())
                {
                    gtmrTextUpdateTimer.start();
                }
            }

            if (ui->check_Echo->isChecked() == true)
            {
                //Local echo
                QByteArray baTmpBA = ui->text_TermEditData->GetDatOut()->toUtf8();
                baTmpBA.append("\n");
                ui->text_TermEditData->AddDatInText(&baTmpBA);
            }
            ui->text_TermEditData->ClearDatOut();
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::UpdateImages(
    )
{
    //Updates images to reflect status
    if (gspSerialPort.isOpen() == true)
    {
        //Port open
        SerialStatus(true);
    }
    else
    {
        //Port closed
        ui->image_CTS->setPixmap(*gpEmptyCirclePixmap);
        ui->image_DCD->setPixmap(*gpEmptyCirclePixmap);
        ui->image_DSR->setPixmap(*gpEmptyCirclePixmap);
        ui->image_RI->setPixmap(*gpEmptyCirclePixmap);

        ui->image_CTSb->setPixmap(*gpEmptyCirclePixmap);
        ui->image_DCDb->setPixmap(*gpEmptyCirclePixmap);
        ui->image_DSRb->setPixmap(*gpEmptyCirclePixmap);
        ui->image_RIb->setPixmap(*gpEmptyCirclePixmap);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::KeyPressed(
    int nKey,
    QChar chrKeyValue
    )
{
    //Key pressed, send it out
    if (gspSerialPort.isOpen())
    {
        if (gbTermBusy == false && gbLoopbackMode == false)
        {
            if (ui->check_Echo->isChecked() == true)
            {
                //Echo mode on
                if (nKey == Qt::Key_Enter || nKey == Qt::Key_Return)
                {
                    gbaDisplayBuffer.append("\n");
                }

                if (!gtmrTextUpdateTimer.isActive())
                {
                    gtmrTextUpdateTimer.start();
                }
            }

            //Convert character to a byte array (in case it's UTF-8 and more than 1 byte)
            QByteArray baTmpBA = QString((QChar)chrKeyValue).toUtf8();

            //Character mode, send right on
            if (nKey == Qt::Key_Enter || nKey == Qt::Key_Return || chrKeyValue == '\r' || chrKeyValue == '\n')
            {
                //Return key or newline
                gpMainLog->WriteLogData("\n");
                DoLineEnd();
            }
            else
            {
                //Not return
                gspSerialPort.write(baTmpBA);
                gintQueuedTXBytes += baTmpBA.size();
            }

            //Output back to screen buffer if echo mode is enabled
            if (ui->check_Echo->isChecked())
            {
                if (ui->check_ShowCLRF->isChecked() == true)
                {
                    //Escape \t, \r and \n in addition to normal escaping
                    gbaDisplayBuffer.append(baTmpBA.replace("\t", "\\t").replace("\r", "\\r").replace("\n", "\\n").replace('\0', "\\00").replace("\x01", "\\01").replace("\x02", "\\02").replace("\x03", "\\03").replace("\x04", "\\04").replace("\x05", "\\05").replace("\x06", "\\06").replace("\x07", "\\07").replace("\x08", "\\08").replace("\x0b", "\\0B").replace("\x0c", "\\0C").replace("\x0e", "\\0E").replace("\x0f", "\\0F").replace("\x10", "\\10").replace("\x11", "\\11").replace("\x12", "\\12").replace("\x13", "\\13").replace("\x14", "\\14").replace("\x15", "\\15").replace("\x16", "\\16").replace("\x17", "\\17").replace("\x18", "\\18").replace("\x19", "\\19").replace("\x1a", "\\1a").replace("\x1b", "\\1b").replace("\x1c", "\\1c").replace("\x1d", "\\1d").replace("\x1e", "\\1e").replace("\x1f", "\\1f"));
                }
                else
                {
                    //Normal escaping
                    gbaDisplayBuffer.append(baTmpBA.replace('\0', "\\00").replace("\x01", "\\01").replace("\x02", "\\02").replace("\x03", "\\03").replace("\x04", "\\04").replace("\x05", "\\05").replace("\x06", "\\06").replace("\x07", "\\07").replace("\x08", "\\08").replace("\x0b", "\\0B").replace("\x0c", "\\0C").replace("\x0e", "\\0E").replace("\x0f", "\\0F").replace("\x10", "\\10").replace("\x11", "\\11").replace("\x12", "\\12").replace("\x13", "\\13").replace("\x14", "\\14").replace("\x15", "\\15").replace("\x16", "\\16").replace("\x17", "\\17").replace("\x18", "\\18").replace("\x19", "\\19").replace("\x1a", "\\1a").replace("\x1b", "\\1b").replace("\x1c", "\\1c").replace("\x1d", "\\1d").replace("\x1e", "\\1e").replace("\x1f", "\\1f"));
                }

                //Run display update timer
                if (!gtmrTextUpdateTimer.isActive())
                {
                    gtmrTextUpdateTimer.start();
                }

                //Output to log file
                gpMainLog->WriteLogData(QString(chrKeyValue).toUtf8());
            }
        }
        else if (gbLoopbackMode == true)
        {
            //Loopback is enabled
            gbaDisplayBuffer.append("[Cannot send: Loopback mode is enabled.]\n");

            if (!gtmrTextUpdateTimer.isActive())
            {
                gtmrTextUpdateTimer.start();
            }
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::DoLineEnd(
    )
{
    //Outputs a line ending
    if (ui->radio_LLF->isChecked())
    {
        //LF (\n) *nix style
        gspSerialPort.write("\n");
        gintQueuedTXBytes += 1;
    }
    else if (ui->radio_LCR->isChecked())
    {
        //CR (\r)
        gspSerialPort.write("\r");
        gintQueuedTXBytes += 1;
    }
    else if (ui->radio_LCRLF->isChecked())
    {
        //CR LF (\r\n) windows style
        gspSerialPort.write("\r\n");
        gintQueuedTXBytes += 2;
    }
    return;
}

//=============================================================================
//=============================================================================
void
MainWindow::DevRespTimeout(
    )
{
    //Runs to indiciate a device timeout when waiting to compile an application
    if (gbTermBusy == true)
    {
        //Update buffer
        gbaDisplayBuffer.append("\nTimeout occured whilst attempting to XCompile application or download to module.\n");
        if (!gtmrTextUpdateTimer.isActive())
        {
            gtmrTextUpdateTimer.start();
        }

        //Reset variables
        gbTermBusy = false;
        gchTermBusyLines = 0;
        gstrTermBusyData = tr("");
        ui->btn_Cancel->setEnabled(false);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::SerialStatus(
    bool bType
    )
{
    if (gspSerialPort.isOpen() == true)
    {
        unsigned int intSignals = gspSerialPort.pinoutSignals();
        if ((((intSignals & QSerialPort::ClearToSendSignal) == QSerialPort::ClearToSendSignal ? 1 : 0) != gbCTSStatus || bType == true))
        {
            //CTS changed
            gbCTSStatus = ((intSignals & QSerialPort::ClearToSendSignal) == QSerialPort::ClearToSendSignal ? 1 : 0);
            ui->image_CTS->setPixmap((gbCTSStatus == true ? *gpGreenCirclePixmap : *gpRedCirclePixmap));
            ui->image_CTSb->setPixmap((gbCTSStatus == true ? *gpGreenCirclePixmap : *gpRedCirclePixmap));
        }
        if ((((intSignals & QSerialPort::DataCarrierDetectSignal) == QSerialPort::DataCarrierDetectSignal ? 1 : 0) != gbDCDStatus || bType == true))
        {
            //DCD changed
            gbDCDStatus = ((intSignals & QSerialPort::DataCarrierDetectSignal) == QSerialPort::DataCarrierDetectSignal ? 1 : 0);
            ui->image_DCD->setPixmap((gbDCDStatus == true ? *gpGreenCirclePixmap : *gpRedCirclePixmap));
            ui->image_DCDb->setPixmap((gbDCDStatus == true ? *gpGreenCirclePixmap : *gpRedCirclePixmap));
        }
        if ((((intSignals & QSerialPort::DataSetReadySignal) == QSerialPort::DataSetReadySignal ? 1 : 0) != gbDSRStatus || bType == true))
        {
            //DSR changed
            gbDSRStatus = ((intSignals & QSerialPort::DataSetReadySignal) == QSerialPort::DataSetReadySignal ? 1 : 0);
            ui->image_DSR->setPixmap((gbDSRStatus == true ? *gpGreenCirclePixmap : *gpRedCirclePixmap));
            ui->image_DSRb->setPixmap((gbDSRStatus == true ? *gpGreenCirclePixmap : *gpRedCirclePixmap));
        }
        if ((((intSignals & QSerialPort::RingIndicatorSignal) == QSerialPort::RingIndicatorSignal ? 1 : 0) != gbRIStatus || bType == true))
        {
            //RI changed
            gbRIStatus = ((intSignals & QSerialPort::RingIndicatorSignal) == QSerialPort::RingIndicatorSignal ? 1 : 0);
            ui->image_RI->setPixmap((gbRIStatus == true ? *gpGreenCirclePixmap : *gpRedCirclePixmap));
            ui->image_RIb->setPixmap((gbRIStatus == true ? *gpGreenCirclePixmap : *gpRedCirclePixmap));
        }
    }
    else
    {
        //Port isn't open, display empty circles
        ui->image_CTS->setPixmap(*gpEmptyCirclePixmap);
        ui->image_DCD->setPixmap(*gpEmptyCirclePixmap);
        ui->image_DSR->setPixmap(*gpEmptyCirclePixmap);
        ui->image_RI->setPixmap(*gpEmptyCirclePixmap);

        ui->image_CTSb->setPixmap(*gpEmptyCirclePixmap);
        ui->image_DCDb->setPixmap(*gpEmptyCirclePixmap);
        ui->image_DSRb->setPixmap(*gpEmptyCirclePixmap);
        ui->image_RIb->setPixmap(*gpEmptyCirclePixmap);

        //Disable timer
        gpSignalTimer->stop();
    }
    return;
}

//=============================================================================
//=============================================================================
void
MainWindow::SerialStatusSlot(
    )
{
    //Slot function to update serial pinout status
    SerialStatus(0);
}

//=============================================================================
//=============================================================================
void
MainWindow::OpenDevice(
    )
{
    //Function to open serial port
    if (gspSerialPort.isOpen() == true)
    {
        //Serial port is already open - cancel any pending operations
        if (gbTermBusy == true && gbFileOpened == true)
        {
            //Run cancel operation
            on_btn_Cancel_clicked();
        }

        //Close serial port
        if (gspSerialPort.isOpen() == true)
        {
            gspSerialPort.clear();
            gspSerialPort.close();
            for (int i = 0; i < plugin_list.length(); ++i)
            {
                plugin_list.at(i).plugin->serial_closed();
            }
            gbPluginHideTerminalOutput = false;
            gbPluginRunning = false;
        }
        gpSignalTimer->stop();

        //Change status message
        ui->statusBar->showMessage("");

#ifndef SKIPAUTOMATIONFORM
        //Notify automation form
        if (guaAutomationForm != 0)
        {
            guaAutomationForm->ConnectionChange(false);
        }
#endif

        //Update images
        UpdateImages();

        //Close log file if open
        if (gpMainLog->IsLogOpen() == true)
        {
            gpMainLog->CloseLogFile();
        }

        gtmrPortOpened.invalidate();
    }

    if (ui->combo_COM->currentText().length() > 0)
    {
        //Port selected: setup serial port
        gspSerialPort.setPortName(ui->combo_COM->currentText());
        gspSerialPort.setBaudRate(ui->combo_Baud->currentText().toInt());
        gspSerialPort.setDataBits((QSerialPort::DataBits)ui->combo_Data->currentText().toInt());
        gspSerialPort.setStopBits((QSerialPort::StopBits)ui->combo_Stop->currentText().toInt());

        //Parity
        gspSerialPort.setParity((ui->combo_Parity->currentIndex() == 1 ? QSerialPort::OddParity : (ui->combo_Parity->currentIndex() == 2 ? QSerialPort::EvenParity : QSerialPort::NoParity)));

        //Flow control
        gspSerialPort.setFlowControl((ui->combo_Handshake->currentIndex() == 1 ? QSerialPort::HardwareControl : (ui->combo_Handshake->currentIndex() == 2 ? QSerialPort::SoftwareControl : QSerialPort::NoFlowControl)));

        if (gspSerialPort.open(QIODevice::ReadWrite))
        {
            //Successful
            ui->statusBar->showMessage(QString("[").append(ui->combo_COM->currentText()).append(":").append(ui->combo_Baud->currentText()).append(",").append((ui->combo_Parity->currentIndex() == 0 ? "N" : ui->combo_Parity->currentIndex() == 1 ? "O" : ui->combo_Parity->currentIndex() == 2 ? "E" : "")).append(",").append(ui->combo_Data->currentText()).append(",").append(ui->combo_Stop->currentText()).append(",").append((ui->combo_Handshake->currentIndex() == 0 ? "N" : ui->combo_Handshake->currentIndex() == 1 ? "H" : ui->combo_Handshake->currentIndex() == 2 ? "S" : "")).append("]{").append((ui->radio_LCR->isChecked() ? "\\r" : (ui->radio_LLF->isChecked() ? "\\n" : (ui->radio_LCRLF->isChecked() ? "\\r\\n" : "")))).append("}"));
            ui->label_TermConn->setText(ui->statusBar->currentMessage());
#if SKIPSPEEDTEST != 1
            ui->label_SpeedConn->setText(ui->statusBar->currentMessage());
#endif

            //Update tooltip of system tray
            if (gbSysTrayEnabled == true)
            {
                gpSysTray->setToolTip(QString("AuTerm v").append(UwVersion).append(" (").append(ui->combo_COM->currentText()).append(")"));
            }

            //Switch to Terminal tab if not on terminal or speed testing tab
            if (ui->selector_Tab->currentIndex() != ui->selector_Tab->indexOf(ui->tab_Term) && ui->selector_Tab->currentIndex() != ui->selector_Tab->indexOf(ui->tab_SpeedTest))
            {
                ui->selector_Tab->setCurrentIndex(ui->selector_Tab->indexOf(ui->tab_Term));
            }

            //Disable read-only mode
            ui->text_TermEditData->setReadOnly(false);

            //DTR
            gspSerialPort.setDataTerminalReady(ui->check_DTR->isChecked());

            //Flow control
            if (ui->combo_Handshake->currentIndex() == 1)
            {
                //Hardware handshaking
                ui->check_RTS->setEnabled(false);
#if SKIPSPEEDTEST != 1
                ui->check_SpeedRTS->setEnabled(false);
#endif
            }
            else
            {
                //Not hardware handshaking - RTS
                ui->check_RTS->setEnabled(true);
#if SKIPSPEEDTEST != 1
                ui->check_SpeedRTS->setEnabled(true);
#endif
                gspSerialPort.setRequestToSend(ui->check_RTS->isChecked());
            }

            //Break
            gspSerialPort.setBreakEnabled(ui->check_Break->isChecked());

            //Enable checkboxes
            ui->check_Break->setEnabled(true);
            ui->check_DTR->setEnabled(true);
#if SKIPSPEEDTEST != 1
            ui->check_SpeedDTR->setEnabled(true);
#endif
            ui->check_Echo->setEnabled(true);
            ui->check_Line->setEnabled(true);

            //Update button text
            ui->btn_TermClose->setText("C&lose Port");
#if SKIPSPEEDTEST != 1
            ui->btn_SpeedClose->setText("C&lose Port");
#endif

            //Signal checking
            SerialStatus(1);

            //Enable timer
            gpSignalTimer->start(gpTermSettings->value("SerialSignalCheckInterval", DefaultSerialSignalCheckInterval).toUInt());

#ifndef SKIPAUTOMATIONFORM
            //Notify automation form
            if (guaAutomationForm != 0)
            {
                guaAutomationForm->ConnectionChange(true);
            }
#endif

            //Notify scroll edit
            ui->text_TermEditData->SetSerialOpen(true);

            //Set focus to input text edit
            ui->text_TermEditData->setFocus();

            //Disable log options
            ui->edit_LogFile->setEnabled(false);
            ui->check_LogEnable->setEnabled(false);
            ui->check_LogAppend->setEnabled(false);
            ui->btn_LogFileSelect->setEnabled(false);

#if SKIPSPEEDTEST != 1
            //Enable speed testing
            ui->btn_SpeedStartStop->setEnabled(true);
#endif

            //Clear last received date/time
            ui->label_LastRx->setText("N/A");

            //Open log file
            if (ui->check_LogEnable->isChecked() == true)
            {
                //Logging is enabled
#ifdef TARGET_OS_MAC
                if (gpMainLog->OpenLogFile(QString((ui->edit_LogFile->text().left(1) == "/" || ui->edit_LogFile->text().left(1) == "\\") ? "" : QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/").append(ui->edit_LogFile->text())) == LOG_OK)
#else
                if (gpMainLog->OpenLogFile(ui->edit_LogFile->text()) == LOG_OK)
#endif
                {
                    //Log opened
                    if (ui->check_LogAppend->isChecked() == false)
                    {
                        //Clear the log file
                        gpMainLog->ClearLog();
                    }
                    gpMainLog->WriteLogData(tr("-").repeated(31));
                    gpMainLog->WriteLogData(tr("\n Log opened ").append(QDate::currentDate().toString("dd/MM/yyyy")).append(" @ ").append(QTime::currentTime().toString("hh:mm")).append(" \n"));
                    gpMainLog->WriteLogData(tr(" AuTerm ").append(UwVersion).append(" \n"));
                    gpMainLog->WriteLogData(QString(" Port: ").append(ui->combo_COM->currentText()).append("\n"));
                    gpMainLog->WriteLogData(tr("-").repeated(31).append("\n\n"));
                    gbMainLogEnabled = true;
                }
                else
                {
                    //Log not writeable
                    QString strMessage = tr("Error whilst opening log.\nPlease ensure you have access to the log file ").append(ui->edit_LogFile->text()).append(" and have enough free space on your hard drive.");
                    gpmErrorForm->show();
                    gpmErrorForm->SetMessage(&strMessage);
                }
            }

            //Allow file drops for uploads
            setAcceptDrops(true);

#ifndef SKIPSCRIPTINGFORM
            if (gusScriptingForm != 0)
            {
                gusScriptingForm->SerialPortStatus(true);
            }
#endif

            gtmrPortOpened.start();
            gintLastSerialTimeUpdate = 0;
        }
        else
        {
            //Error whilst opening
            ui->statusBar->showMessage("Error: ");
            ui->statusBar->showMessage(ui->statusBar->currentMessage().append(gspSerialPort.errorString()));

            QString strMessage = tr("Error whilst attempting to open the serial device: ").append(gspSerialPort.errorString()).append("\n\nIf the serial port is open in another application, please close the other application")
#if !defined(_WIN32) && !defined( __APPLE__)
            .append(", please also ensure you have been granted permission to the serial device in /dev/")
#endif
            .append((ui->combo_Baud->currentText().toULong() > 115200 ? ", please also ensure that your serial device supports baud rates greater than 115200 (normal COM ports do not have support for these baud rates)" : ""))
            .append(" and try again.");
            gpmErrorForm->show();
            gpmErrorForm->SetMessage(&strMessage);
            ui->text_TermEditData->SetSerialOpen(false);
        }
    }
    else
    {
        //No serial port selected
        QString strMessage = tr("No serial port was selected, please select a serial port and try again.\r\nIf you see no serial ports listed, ensure your device is connected to your computer and you have the appropriate drivers installed.");
        gpmErrorForm->show();
        gpmErrorForm->SetMessage(&strMessage);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::LoadFile(
    bool bToUWC
    )
{
    //Load
    QList<QString> lstFI = SplitFilePath(gstrTermFilename);
    QFile fileFileName((bToUWC == true ? QString(lstFI[0]).append(lstFI[1]).append(".uwc") : gstrTermFilename));
    if (!fileFileName.open(QIODevice::ReadOnly))
    {
        //Unable to open file
        QString strMessage = tr("Error during XCompile: Access to selected file is denied: ").append((bToUWC ? QString(lstFI[0]).append(lstFI[1]).append(".uwc") : gstrTermFilename));
        gpmErrorForm->show();
        gpmErrorForm->SetMessage(&strMessage);
        gbTermBusy = false;
        ui->btn_Cancel->setEnabled(false);
        return;
    }

    //Is this a UWC download?
    gbIsUWCDownload = bToUWC;

    //Create a data stream and hex string holder
    QDataStream in(&fileFileName);
    gstrHexData = "";
    while (!in.atEnd())
    {
        //One byte at a time, convert the data to hex
        quint8 ThisByte;
        in >> ThisByte;
        QString strThisHex;
        strThisHex.setNum(ThisByte, 16);
        if (strThisHex.length() == 1)
        {
            //Expand to 2 characters
            gstrHexData.append("0");
        }

        //Add the hex character to the string
        gstrHexData.append(strThisHex.toUpper());
    }

    //Close the file handle
    fileFileName.close();

    //Download filename is filename without a file extension
    gstrDownloadFilename = (lstFI[1].indexOf(".") == -1 ? lstFI[1] : lstFI[1].left(lstFI[1].indexOf(".")));
}

//=============================================================================
//=============================================================================
void
MainWindow::on_check_Break_stateChanged(
    )
{
    //Break status changed
    gspSerialPort.setBreakEnabled(ui->check_Break->isChecked());
}

//=============================================================================
//=============================================================================
void
MainWindow::on_check_RTS_stateChanged(
    )
{
    //RTS status changed
    gspSerialPort.setRequestToSend(ui->check_RTS->isChecked());
#if SKIPSPEEDTEST != 1
    if (ui->check_SpeedRTS->isChecked() != ui->check_RTS->isChecked())
    {
        //Update speed form checkbox
        ui->check_SpeedRTS->setChecked(ui->check_RTS->isChecked());
    }
#endif
}

//=============================================================================
//=============================================================================
void
MainWindow::on_check_DTR_stateChanged(
    )
{
    //DTR status changed
    gspSerialPort.setDataTerminalReady(ui->check_DTR->isChecked());
#if SKIPSPEEDTEST != 1
    if (ui->check_SpeedDTR->isChecked() != ui->check_DTR->isChecked())
    {
        //Update speed form checkbox
        ui->check_SpeedDTR->setChecked(ui->check_DTR->isChecked());
    }
#endif
}

//=============================================================================
//=============================================================================
void
MainWindow::on_check_Line_stateChanged(
    )
{
    //Line mode status changed
    ui->text_TermEditData->SetLineMode(ui->check_Line->isChecked());
}

//=============================================================================
//=============================================================================
void
MainWindow::SerialError(
    QSerialPort::SerialPortError speErrorCode
    )
{
    bool port_closed = false;

#ifndef SKIPSCRIPTINGFORM
    if (gbScriptingRunning == true)
    {
        gusScriptingForm->SerialPortError(speErrorCode);
    }
#endif

    if (speErrorCode == QSerialPort::NoError)
    {
        //No error. Why this is ever emitted is a mystery to me.
        for (int i = 0; i < plugin_list.length(); ++i)
        {
            plugin_list.at(i).plugin->serial_opened();
        }
        return;
    }
    else if (speErrorCode == QSerialPort::ResourceError || speErrorCode == QSerialPort::PermissionError)
    {
        //Resource error or permission error (device unplugged?)
        QString strMessage = tr("Fatal error with serial connection.\nPlease reconnect to the device to continue.");
        gpmErrorForm->show();
        gpmErrorForm->SetMessage(&strMessage);
        ui->text_TermEditData->SetSerialOpen(false);

        if (gspSerialPort.isOpen() == true)
        {
            //Close active connection
            gspSerialPort.close();
            port_closed = true;
        }

        if (gbStreamingFile == true)
        {
            //Clear up file stream
            gtmrStreamTimer.invalidate();
            gbStreamingFile = false;
            gpStreamFileHandle->close();
            delete gpStreamFileHandle;
        }
        else if (gbStreamingBatch == true)
        {
            //Clear up batch
            gtmrStreamTimer.invalidate();
            gtmrBatchTimeoutTimer.stop();
            gbStreamingBatch = false;
            gpStreamFileHandle->close();
            delete gpStreamFileHandle;
            gbaBatchReceive.clear();
        }
#if SKIPSPEEDTEST != 1
        else if (gbSpeedTestRunning == true)
        {
            //Clear up speed testing
            if (gtmrSpeedTestDelayTimer != 0)
            {
                //Clean up timer
                disconnect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStartTimer()));
                disconnect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStopTimer()));
                delete gtmrSpeedTestDelayTimer;
                gtmrSpeedTestDelayTimer = 0;
            }

            ui->btn_SpeedStartStop->setEnabled(false);
            ui->check_SpeedSyncReceive->setEnabled(true);
            ui->combo_SpeedDataType->setEnabled(true);
            if (ui->combo_SpeedDataType->currentIndex() == 1)
            {
                //Enable string options
                ui->edit_SpeedTestData->setEnabled(true);
                ui->check_SpeedStringUnescape->setEnabled(true);
            }

            //Update values
            OutputSpeedTestAvgStats((gtmrSpeedTimer.nsecsElapsed() < 1000000000LL ? 1000000000LL : gtmrSpeedTimer.nsecsElapsed()/1000000000LL));

            //Set speed test as no longer running
            gchSpeedTestMode = SpeedModeInactive;
            gbSpeedTestRunning = false;

            if (gtmrSpeedTimer.isValid())
            {
                //Invalidate speed test timer
                gtmrSpeedTimer.invalidate();
            }
            if (gtmrSpeedTestStats.isActive())
            {
                //Stop stats update timer
                gtmrSpeedTestStats.stop();
            }
            if (gtmrSpeedTestStats10s.isActive())
            {
                //Stop 10 second stats update timer
                gtmrSpeedTestStats10s.stop();
            }

            //Clear buffers
            gbaSpeedMatchData.clear();
            gbaSpeedReceivedData.clear();

            //Show finished message in status bar
            ui->statusBar->showMessage("Speed testing failed due to serial port error.");
        }
#endif

        //No longer busy
        gbTermBusy = false;
        gchTermMode = 0;
        gchTermMode2 = 0;

        //Disable cancel button
        ui->btn_Cancel->setEnabled(false);

        //Disable active checkboxes
        ui->check_Break->setEnabled(false);
        ui->check_DTR->setEnabled(false);
        ui->check_Echo->setEnabled(false);
        ui->check_Line->setEnabled(false);
        ui->check_RTS->setEnabled(false);
#if SKIPSPEEDTEST != 1
        ui->check_SpeedDTR->setEnabled(false);
        ui->check_SpeedRTS->setEnabled(false);
#endif

        //Disable text entry
        ui->text_TermEditData->setReadOnly(true);

        //Change status message
        ui->statusBar->showMessage("");

        //Change button text
        ui->btn_TermClose->setText("&Open Port");
#if SKIPSPEEDTEST != 1
        ui->btn_SpeedClose->setText("&Open Port");
#endif

        //Update images
        UpdateImages();

        //Close log file if open
        if (gpMainLog->IsLogOpen() == true)
        {
            gpMainLog->CloseLogFile();
        }

        //Enable log options
        ui->edit_LogFile->setEnabled(true);
        ui->check_LogEnable->setEnabled(true);
        ui->check_LogAppend->setEnabled(true);
        ui->btn_LogFileSelect->setEnabled(true);

#ifndef SKIPAUTOMATIONFORM
        //Notify automation form
        if (guaAutomationForm != 0)
        {
            guaAutomationForm->ConnectionChange(false);
        }
#endif

        //Show disconnection balloon
        if (gbSysTrayEnabled == true && !this->isActiveWindow() && !gpmErrorForm->isActiveWindow()
#ifndef SKIPAUTOMATIONFORM
           && (guaAutomationForm == 0 || (guaAutomationForm != 0 && !guaAutomationForm->isActiveWindow()))
#endif
           )
        {
            gpSysTray->showMessage(ui->combo_COM->currentText().append(" Removed"), QString("Connection to device ").append(ui->combo_COM->currentText()).append(" has been lost due to disconnection."), QSystemTrayIcon::Critical);
        }

        //Disallow file drops
        setAcceptDrops(false);
    }

    for (int i = 0; i < plugin_list.length(); ++i)
    {
        plugin_list.at(i).plugin->serial_error(speErrorCode);
    }

    if (port_closed == true)
        {
        for (int i = 0; i < plugin_list.length(); ++i)
        {
            plugin_list.at(i).plugin->serial_closed();
        }
        gbPluginHideTerminalOutput = false;
        gbPluginRunning = false;
        }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Duplicate_clicked(
    )
{
    //Duplicates instance of AuTerm
    QProcess DuplicateProcess;
    DuplicateProcess.startDetached(QCoreApplication::applicationFilePath(), QStringList() << "DUPLICATE" <<  QString("COM=").append(ui->combo_COM->currentText()) << QString("BAUD=").append(ui->combo_Baud->currentText()) << tr("STOP=").append(ui->combo_Stop->currentText()) << tr("DATA=").append(ui->combo_Data->currentText()) << tr("PAR=").append(ui->combo_Parity->currentText()) << tr("FLOW=").append(QString::number(ui->combo_Handshake->currentIndex())) << tr("ENDCHR=").append((ui->radio_LCR->isChecked() == true ? "0" : ui->radio_LLF->isChecked() == true ? "1" : "2")) << tr("LOCALECHO=").append((ui->check_Echo->isChecked() == true ? "1" : "0")) << tr("LINEMODE=").append((ui->check_Line->isChecked() == true ? "1" : "0")) << "NOCONNECT");
}

//=============================================================================
//=============================================================================
void
MainWindow::MessagePass(
    QByteArray baDataString,
    bool bEscapeString,
    bool bFromScripting
    )
{
    //Receive a command from the automation window
    if (gspSerialPort.isOpen() == true && (gbTermBusy == false || bFromScripting == true) && gbLoopbackMode == false)
    {
        if (bEscapeString == true)
        {
            //Escape string sequences
            UwxEscape::EscapeCharacters(&baDataString);
        }

        //Output the data and send it to the log
        gspSerialPort.write(baDataString);
        gintQueuedTXBytes += baDataString.size();
        gpMainLog->WriteRawLogData(baDataString);

        if (ui->check_Echo->isChecked() == true)
        {
            if (ui->check_ShowCLRF->isChecked() == true)
            {
                //Escape \t, \r and \n
                baDataString.replace("\t", "\\t").replace("\r", "\\r").replace("\n", "\\n");
            }

            //Replace unprintable characters
            baDataString.replace('\0', "\\00").replace("\x01", "\\01").replace("\x02", "\\02").replace("\x03", "\\03").replace("\x04", "\\04").replace("\x05", "\\05").replace("\x06", "\\06").replace("\x07", "\\07").replace("\x08", "\\08").replace("\x0b", "\\0B").replace("\x0c", "\\0C").replace("\x0e", "\\0E").replace("\x0f", "\\0F").replace("\x10", "\\10").replace("\x11", "\\11").replace("\x12", "\\12").replace("\x13", "\\13").replace("\x14", "\\14").replace("\x15", "\\15").replace("\x16", "\\16").replace("\x17", "\\17").replace("\x18", "\\18").replace("\x19", "\\19").replace("\x1a", "\\1a").replace("\x1b", "\\1b").replace("\x1c", "\\1c").replace("\x1d", "\\1d").replace("\x1e", "\\1e").replace("\x1f", "\\1f");

            //Output to display buffer
            gbaDisplayBuffer.append(baDataString);
            if (!gtmrTextUpdateTimer.isActive())
            {
                gtmrTextUpdateTimer.start();
            }
        }

        if (bEscapeString == false && bFromScripting == false)
        {
            //Not escaping sequences and not from scripting form so send line end
            DoLineEnd();
            gpMainLog->WriteLogData("\n");
        }
    }
    else if (gspSerialPort.isOpen() == true && gbLoopbackMode == true)
    {
        //Loopback is enabled
        gbaDisplayBuffer.append("\n[Cannot send: Loopback mode is enabled.]\n");
        if (!gtmrTextUpdateTimer.isActive())
        {
            gtmrTextUpdateTimer.start();
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::LookupErrorCode(
    unsigned int intErrorCode
    )
{
    //Looks up an error code and outputs it in the edit (does NOT store it to the log)
    if (gbErrorsLoaded == true)
    {
        //Error file has been loaded
        gbaDisplayBuffer.append(QString("\nError code 0x").append(QString::number(intErrorCode, 16)).append(": ").append(gpErrorMessages->value(QString::number(intErrorCode), "Undefined Error Code").toString()).append("\n").toUtf8());
    }
    else
    {
        //Error file has not been loaded
        gbaDisplayBuffer.append(QString("\nUnable to lookup error code: error file (codes.csv) not loaded. Check the Update tab to download the latest version.\n").toUtf8());
    }

    if (!gtmrTextUpdateTimer.isActive())
    {
        gtmrTextUpdateTimer.start();
    }
    ui->text_TermEditData->moveCursor(QTextCursor::End);
}

//=============================================================================
//=============================================================================
QString
MainWindow::LookupErrorCodeXCompile(
    unsigned int intErrorCode
    )
{
    //Looks up an error code and returns the error text as a string
    return gpErrorMessages->value(QString::number(intErrorCode), "Undefined Error Code").toString();
}

//=============================================================================
//=============================================================================
void
MainWindow::SerialBytesWritten(
    qint64 intByteCount
    )
{
    //Updates the display with the number of bytes written
#if SKIPSPEEDTEST != 1
    if (gbSpeedTestRunning == true)
    {
        //Speed test is running, pass to speed test function
        SpeedTestBytesWritten(intByteCount);
    }
    else
#endif
    {
        //Not running speed test
        gintTXBytes += intByteCount;
        ui->label_TermTx->setText(QString::number(gintTXBytes));

#ifndef SKIPSCRIPTINGFORM
        if (gusScriptingForm != 0 && gbScriptingRunning == true)
        {
            gusScriptingForm->SerialPortWritten(intByteCount);
        }
#endif

        if (gbStreamingFile == true)
        {
            //File stream in progress, read out a block
            QByteArray baFileData = gpStreamFileHandle->read(FileReadBlock);
            gspSerialPort.write(baFileData);
            gintQueuedTXBytes += baFileData.size();
            gintStreamBytesRead += baFileData.length();
            if (gpStreamFileHandle->atEnd())
            {
                //Finished sending
                FinishStream(false);
            }
            else if (gintStreamBytesRead > gintStreamBytesProgress)
            {
                //Progress output
                gbaDisplayBuffer.append(QString("Streamed ").append(QString::number(gintStreamBytesRead)).append(" bytes (").append(QString::number(gintStreamBytesRead*100/gintStreamBytesSize)).append("%).\n").toUtf8());
                if (!gtmrTextUpdateTimer.isActive())
                {
                    gtmrTextUpdateTimer.start();
                }
                gintStreamBytesProgress = gintStreamBytesProgress + StreamProgress;
            }

            //Update status bar
            if (gintStreamBytesRead == gintStreamBytesSize)
            {
                //Finished streaming file
                ui->statusBar->showMessage("File streaming complete!");
            }
            else
            {
                //Still streaming
                ui->statusBar->showMessage(QString("Streamed ").append(QString::number(gintStreamBytesRead).append(" bytes of ").append(QString::number(gintStreamBytesSize))).append(" (").append(QString::number(gintStreamBytesRead*100/gintStreamBytesSize)).append("%)"));
            }
        }
        else if (gbStreamingBatch == true)
        {
            //Batch file command
            ui->statusBar->showMessage(QString("Sending Batch line number ").append(QString::number(gintStreamBytesRead)));
        }
    }

    for (int i = 0; i < plugin_list.length(); ++i)
    {
        plugin_list.at(i).plugin->serial_bytes_written(intByteCount);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Cancel_clicked(
    )
{
    //Cancel current stream or file download
    if (gbTermBusy == true)
    {
        if (gbStreamingFile == true)
        {
            //Cancel stream
            FinishStream(true);
        }
        else if (gbStreamingBatch == true)
        {
            //Cancel batch streaming
            FinishBatch(true);
        }
    }

    //Disable button
    ui->btn_Cancel->setEnabled(false);
}

//=============================================================================
//=============================================================================
void
MainWindow::FinishStream(
    bool bType
    )
{
    //Sending a file stream has finished
    if (bType == true)
    {
        //Stream cancelled
        gbaDisplayBuffer.append(QString("\nCancelled stream after ").append(QString::number(gintStreamBytesRead)).append(" bytes (").append(QString::number(1+(gtmrStreamTimer.nsecsElapsed()/1000000000LL))).append(" seconds) [~").append(QString::number((gintStreamBytesRead/(1+gtmrStreamTimer.nsecsElapsed()/1000000000LL)))).append(" bytes/second].\n").toUtf8());
        ui->statusBar->showMessage("File streaming cancelled.");
    }
    else
    {
        //Stream finished
        gbaDisplayBuffer.append(QString("\nFinished streaming file, ").append(QString::number(gintStreamBytesRead)).append(" bytes sent in ").append(QString::number(1+(gtmrStreamTimer.nsecsElapsed()/1000000000LL))).append(" seconds [~").append(QString::number((gintStreamBytesRead/(1+gtmrStreamTimer.nsecsElapsed()/1000000000LL)))).append(" bytes/second].\n").toUtf8());
        ui->statusBar->showMessage("File streaming complete!");
    }

    //Initiate timer for buffer update
    if (!gtmrTextUpdateTimer.isActive())
    {
        gtmrTextUpdateTimer.start();
    }

    //Clear up
    gtmrStreamTimer.invalidate();
    gbTermBusy = false;
    gbStreamingFile = false;
    gchTermMode = 0;
    gpStreamFileHandle->close();
    delete gpStreamFileHandle;
    ui->btn_Cancel->setEnabled(false);
}

//=============================================================================
//=============================================================================
void
MainWindow::FinishBatch(
    bool bType
    )
{
    //Sending a file stream has finished
    if (bType == true)
    {
        //Stream cancelled
        gbaDisplayBuffer.append(QString("\nCancelled batch (").append(QString::number(1+(gtmrStreamTimer.nsecsElapsed()/1000000000LL))).append(" seconds)\n").toUtf8());
        ui->statusBar->showMessage("Batch file sending cancelled.");
    }
    else
    {
        //Stream finished
        gbaDisplayBuffer.append(QString("\nFinished sending batch file, ").append(QString::number(gintStreamBytesRead)).append(" lines sent in ").append(QString::number(1+(gtmrStreamTimer.nsecsElapsed()/1000000000LL))).append(" seconds\n").toUtf8());
        ui->statusBar->showMessage("Batch file sending complete!");
    }

    //Initiate timer for buffer update
    if (!gtmrTextUpdateTimer.isActive())
    {
        gtmrTextUpdateTimer.start();
    }

    //Clear up and cancel timer
    gtmrStreamTimer.invalidate();
    gtmrBatchTimeoutTimer.stop();
    gbTermBusy = false;
    gbStreamingBatch = false;
    gchTermMode = 0;
    gpStreamFileHandle->close();
    delete gpStreamFileHandle;
    gbaBatchReceive.clear();
    ui->btn_Cancel->setEnabled(false);
}

//=============================================================================
//=============================================================================
void
MainWindow::UpdateReceiveText(
    )
{
    //Updates the receive text buffer
    ui->text_TermEditData->AddDatInText(&gbaDisplayBuffer);
    gbaDisplayBuffer.clear();

    //(Unlisted option) Trim display buffer if required
    if (gbAutoTrimDBuffer == true)
    {
        //Trim display buffer (this may split UTF-8 characters up)
        ui->text_TermEditData->TrimDatIn(gintAutoTrimBufferDThreshold, gintAutoTrimBufferDSize);
#pragma warning("TODO: Document trim options/add to GUI")
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::BatchTimeoutSlot(
    )
{
    //A response to a batch command has timed out
    gbaDisplayBuffer.append("\nModule command timed out.\n");
    if (!gtmrTextUpdateTimer.isActive())
    {
        gtmrTextUpdateTimer.start();
    }
    gbTermBusy = false;
    gbStreamingBatch = false;
    gchTermMode = 0;
    gpStreamFileHandle->close();
    ui->btn_Cancel->setEnabled(false);
    delete gpStreamFileHandle;
}

//=============================================================================
//=============================================================================
void
MainWindow::on_combo_COM_currentIndexChanged(
    int
    )
{
    //Serial port selection has been changed, update text
    if (ui->combo_COM->currentText().length() > 0)
    {
        QSerialPortInfo spiSerialInfo(ui->combo_COM->currentText());
        if (!spiSerialInfo.isNull())
        {
            //Port exists
            QString strDisplayText(spiSerialInfo.description());
            if (spiSerialInfo.manufacturer().length() > 1)
            {
                //Add manufacturer
                strDisplayText.append(" (").append(spiSerialInfo.manufacturer()).append(")");
            }
            if (spiSerialInfo.serialNumber().length() > 1)
            {
                //Add serial
                strDisplayText.append(" [").append(spiSerialInfo.serialNumber()).append("]");
            }
            ui->label_SerialInfo->setText(strDisplayText);
        }
        else
        {
            //No such port
            ui->label_SerialInfo->setText("Invalid serial port selected");
        }
    }
    else
    {
        //Clear text as no port is selected
        ui->label_SerialInfo->clear();
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::dragEnterEvent(
    QDragEnterEvent *dragEvent
    )
{
    //A file is being dragged onto the window
    if (dragEvent->mimeData()->urls().count() == 1 && gbTermBusy == false && gspSerialPort.isOpen() == true)
    {
        //Nothing is running, serial handle is open and a single file is being dragged - accept action
        dragEvent->acceptProposedAction();
    }
    else
    {
        //Terminal is busy, serial port is closed or more than 1 file was dropped
        dragEvent->ignore();
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::dropEvent(
    QDropEvent *dropEvent
    )
{
    //A file has been dragged onto the window - send this file if possible
    QList<QUrl> lstURLs = dropEvent->mimeData()->urls();
    if (lstURLs.isEmpty())
    {
        //No files
        return;
    }
    else if (lstURLs.length() > 1)
    {
        //More than 1 file - ignore
        return;
    }

    QString strFileName = lstURLs.first().toLocalFile();
    if (strFileName.isEmpty())
    {
        //Invalid filename
        return;
    }

    //Pass to other function call
    DroppedFile(strFileName);
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Github_clicked(
    )
{
    //Open webpage at the AuTerm github page)
    if (QDesktopServices::openUrl(QUrl("https://github.com/thedjnK/AuTerm")) == false)
    {
        //Failed to open URL
        QString strMessage = tr("An error occured whilst attempting to open a web browser, please ensure you have a web browser installed and configured. URL: https://github.com/thedjnK/AuTerm");
        gpmErrorForm->show();
        gpmErrorForm->SetMessage(&strMessage);
    }
}

#if 0
//=============================================================================
//=============================================================================
void
MainWindow::replyFinished(
    QNetworkReply* nrReply
    )
{
    //Response received from online server
    if (nrReply->error() != QNetworkReply::NoError && nrReply->error() != QNetworkReply::ServiceUnavailableError)
    {
        //An error occured
        ui->btn_Cancel->setEnabled(false);
        ui->btn_ErrorCodeUpdate->setEnabled(true);
        ui->btn_ErrorCodeDownload->setEnabled(true);
        ui->btn_AuTermUpdate->setEnabled(true);
        ui->btn_ModuleFirmware->setEnabled(true);
        ui->btn_OnlineXComp_Supported->setEnabled(true);
        gtmrDownloadTimeoutTimer.stop();
        gstrHexData = "";
        if (!gtmrTextUpdateTimer.isActive())
        {
            gtmrTextUpdateTimer.start();
        }
        gchTermMode = 0;
        gchTermMode2 = 0;
        gbTermBusy = false;

        //Display error message if operation wasn't cancelled
        if (nrReply->error() != QNetworkReply::OperationCanceledError)
        {
            //Output error message
            QString strMessage = QString("An error occured during an online request related to XCompilation or updates: ").append(nrReply->errorString());
            gpmErrorForm->show();
            gpmErrorForm->SetMessage(&strMessage);
        }
    }
    else
    {
        if (gchTermMode == MODE_CHECK_AuTerm_VERSIONS)
        {
            //AuTerm update response
            QByteArray baTmpBA = nrReply->readAll();
            QJsonParseError jpeJsonError;
            QJsonDocument jdJsonData = QJsonDocument::fromJson(baTmpBA, &jpeJsonError);

            if (jpeJsonError.error == QJsonParseError::NoError)
            {
                //Decoded JSON
                QJsonObject joJsonObject = jdJsonData.object();

                //Server responded with error
                if (joJsonObject["Result"].toString() == "-1")
                {
                    //Outdated version
                    ui->label_AuTermUpdate->setText(QString("Update available: ").append(joJsonObject["Version"].toString()));
                    QPalette palBGColour = QPalette();
                    palBGColour.setColor(QPalette::Active, QPalette::WindowText, Qt::darkGreen);
                    palBGColour.setColor(QPalette::Inactive, QPalette::WindowText, Qt::darkGreen);
                    palBGColour.setColor(QPalette::Disabled, QPalette::WindowText, Qt::darkGreen);
                    ui->label_AuTermUpdate->setPalette(palBGColour);
                }
                else if (joJsonObject["Result"].toString() == "-2")
                {
                    //Server error
                    QString strMessage = QString("A server error was encountered whilst checking for an updated AuTerm version.");
                    gpmErrorForm->show();
                    gpmErrorForm->SetMessage(&strMessage);
                }
                else if (joJsonObject["Result"].toString() == "1")
                {
                    //Version is OK
                    ui->label_AuTermUpdate->setText("No updates available.");
                    QPalette palBGColour = QPalette();
                    ui->label_AuTermUpdate->setPalette(palBGColour);
                }
                else
                {
                    //Server responded with error
                    QString strMessage = QString("Server responded with error code ").append(joJsonObject["Result"].toString()).append("; ").append(joJsonObject["Error"].toString());
                    gpmErrorForm->show();
                    gpmErrorForm->SetMessage(&strMessage);
                }

                if (gupdUpdateCheck == UpdateCheckTypes::TypeWekely)
                {
                    //Performing weekly update check
                    if (joJsonObject["Result"].toString() == "-1")
                    {
                        //AuTerm version is outdated
                        gstrUpdateCheckString = new QString("Weekly update check:\r\n\r\n\tAuTerm: Outdated, ");
                        gstrUpdateCheckString->append(joJsonObject["Version"].toString()).append(" available");
                    }
                }
                else
                {
                    //No longer checking for updates
                    gupdUpdateCheck = UpdateCheckTypes::TypeNone;
                }
            }
            else
            {
                //Error whilst decoding JSON
                QString strMessage = QString("Unable to decode JSON data from server, debug data: ").append(jdJsonData.toJson());
                gpmErrorForm->show();
                gpmErrorForm->SetMessage(&strMessage);
            }

            //Back to non-busy mode
            gchTermMode = 0;
            gchTermMode2 = 0;
            gbTermBusy = false;
            ui->btn_Cancel->setEnabled(false);
            ui->btn_ErrorCodeUpdate->setEnabled(true);
            ui->btn_ErrorCodeDownload->setEnabled(true);
            ui->btn_AuTermUpdate->setEnabled(true);
            ui->btn_ModuleFirmware->setEnabled(true);
            ui->btn_OnlineXComp_Supported->setEnabled(true);
            ui->statusBar->showMessage("");

            if (gupdUpdateCheck == UpdateCheckTypes::TypeWekely)
            {
                //Weekly check for error code file updates
                ErrorCodeUpdateCheck(false);
            }
        }
    }

    //Queue the network reply object to be deleted
    nrReply->deleteLater();
}
#endif

//=============================================================================
//=============================================================================
#ifdef UseSSL
void
MainWindow::sslErrors(
    QNetworkReply* nrReply,
    QList<QSslError> lstSSLErrors
    )
{
    //Error detected with SSL
    if (sslcLairdSSLNew != NULL && nrReply->sslConfiguration().peerCertificate() == *sslcLairdSSLNew)
    {
        //Server certificate matches
        nrReply->ignoreSslErrors(lstSSLErrors);
    }
}
#endif

//=============================================================================
//=============================================================================
QList<QString>
MainWindow::SplitFilePath(
    QString strFilename
    )
{
    //Extracts various parts from a file path; [0] path, [1] filename, [2] file extension
    QFileInfo fiFile(strFilename);
    QString strFilenameOnly = fiFile.fileName();
    QString strFileExtension = "";
    if (strFilenameOnly.indexOf(".") != -1)
    {
        //Dot found, only keep characters up to the dot
        strFileExtension = strFilenameOnly.mid(strFilenameOnly.lastIndexOf(".")+1, -1);
        strFilenameOnly = strFilenameOnly.left(strFilenameOnly.lastIndexOf("."));
    }

    //Return an array with path, filename and extension
    QList<QString> lstReturnData;
    lstReturnData << fiFile.path().append("/") << strFilenameOnly << strFileExtension;
    return lstReturnData;
}

//=============================================================================
//=============================================================================
void
MainWindow::on_check_Echo_stateChanged(
    int
    )
{
    //Local echo checkbox state changed
    ui->text_TermEditData->mbLocalEcho = ui->check_Echo->isChecked();
}

//=============================================================================
//=============================================================================
void
MainWindow::on_combo_PredefinedDevice_currentIndexChanged(
    int intIndex
    )
{
    //Load settings for current device
    const QString strNewBaud = gpPredefinedDevice->value(QString("Port").append(QString::number(intIndex+1).append("Baud")), "115200").toString();
    qint8 intNewIndex = ui->combo_Baud->findText(strNewBaud, Qt::MatchExactly);
    if (intNewIndex == -1)
    {
        //No existing item with an exact match, find the closest speed
        quint32 intNewSpeed = strNewBaud.toULong();
        while (intNewIndex < ui->combo_Baud->count())
        {
            if (ui->combo_Baud->itemText(intNewIndex).toUInt() > intNewSpeed)
            {
                //Found a speed faster than the new speed, use this
                if (intNewIndex > 0)
                {
                    --intNewIndex;
                }
                break;
            }
            ++intNewIndex;
        }

        //Set current index to closest speed and then update speed with new speed
        ui->combo_Baud->setCurrentIndex(intNewIndex);
        ui->combo_Baud->setCurrentText(strNewBaud);
    }
    else
    {
        //Set the current index to the existing item
        ui->combo_Baud->setCurrentIndex(intNewIndex);
    }
    ui->combo_Parity->setCurrentIndex(gpPredefinedDevice->value(QString("Port").append(QString::number(intIndex+1).append("Parity")), "0").toInt());
    ui->combo_Stop->setCurrentText(gpPredefinedDevice->value(QString("Port").append(QString::number(intIndex+1).append("Stop")), "1").toString());
    ui->combo_Data->setCurrentText(gpPredefinedDevice->value(QString("Port").append(QString::number(intIndex+1).append("Data")), "8").toString());
    ui->combo_Handshake->setCurrentIndex(gpPredefinedDevice->value(QString("Port").append(QString::number(intIndex+1).append("Flow")), "1").toInt());
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_PredefinedAdd_clicked(
    )
{
    //Adds a new predefined device entry
    ui->combo_PredefinedDevice->addItem("New");
    ui->combo_PredefinedDevice->setCurrentIndex(ui->combo_PredefinedDevice->count()-1);
    gpPredefinedDevice->setValue(QString("Port").append(QString::number((ui->combo_PredefinedDevice->count()))).append("Name"), "New");
    gpPredefinedDevice->setValue(QString("Port").append(QString::number((ui->combo_PredefinedDevice->count()))).append("Baud"), ui->combo_Baud->currentText());
    gpPredefinedDevice->setValue(QString("Port").append(QString::number((ui->combo_PredefinedDevice->count()))).append("Parity"), ui->combo_Parity->currentIndex());
    gpPredefinedDevice->setValue(QString("Port").append(QString::number((ui->combo_PredefinedDevice->count()))).append("Stop"), ui->combo_Stop->currentText());
    gpPredefinedDevice->setValue(QString("Port").append(QString::number((ui->combo_PredefinedDevice->count()))).append("Data"), ui->combo_Data->currentText());
    gpPredefinedDevice->setValue(QString("Port").append(QString::number((ui->combo_PredefinedDevice->count()))).append("Flow"), ui->combo_Handshake->currentIndex());
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_PredefinedDelete_clicked(
    )
{
    //Remove current device configuration
    if (ui->combo_PredefinedDevice->count() > 0)
    {
        //Item exists, delete selected item
        unsigned int uiDeviceNumber = ui->combo_PredefinedDevice->currentIndex();
        unsigned int i = uiDeviceNumber+2;
        gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Name"));
        gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Baud"));
        gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Parity"));
        gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Stop"));
        gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Data"));
        gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Flow"));
        while (!gpPredefinedDevice->value(QString("Port").append(QString::number(i)).append("Name")).isNull())
        {
            //Shift element up
            gpPredefinedDevice->setValue(QString("Port").append(QString::number(i-1)).append("Name"), gpPredefinedDevice->value(QString("Port").append(QString::number(i)).append("Name")).toString());
            gpPredefinedDevice->setValue(QString("Port").append(QString::number(i-1)).append("Baud"), gpPredefinedDevice->value(QString("Port").append(QString::number(i)).append("Baud")).toInt());
            gpPredefinedDevice->setValue(QString("Port").append(QString::number(i-1)).append("Parity"), gpPredefinedDevice->value(QString("Port").append(QString::number(i)).append("Parity")).toInt());
            gpPredefinedDevice->setValue(QString("Port").append(QString::number(i-1)).append("Stop"), gpPredefinedDevice->value(QString("Port").append(QString::number(i)).append("Stop")).toInt());
            gpPredefinedDevice->setValue(QString("Port").append(QString::number(i-1)).append("Data"), gpPredefinedDevice->value(QString("Port").append(QString::number(i)).append("Data")).toInt());
            gpPredefinedDevice->setValue(QString("Port").append(QString::number(i-1)).append("Flow"), gpPredefinedDevice->value(QString("Port").append(QString::number(i)).append("Flow")).toInt());
            ++i;
        }
        if (!gpPredefinedDevice->value(QString("Port").append(QString::number(i-1)).append("Name")).isNull())
        {
            //Remove last element
            gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Name"));
            gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Baud"));
            gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Parity"));
            gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Stop"));
            gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Data"));
            gpPredefinedDevice->remove(QString("Port").append(QString::number(i-1)).append("Flow"));
        }
        ui->combo_PredefinedDevice->removeItem(uiDeviceNumber);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::DroppedFile(
    QString strFilename
    )
{
    //File dragged for download
    if (gbTermBusy == false)
    {
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_SaveDevice_clicked(
    )
{
    //Saves changes to a configuration
    ui->combo_PredefinedDevice->setItemText(ui->combo_PredefinedDevice->currentIndex(), ui->combo_PredefinedDevice->currentText());
    gpPredefinedDevice->setValue(QString("Port").append(QString::number(((ui->combo_PredefinedDevice->currentIndex()+1)))).append("Name"), ui->combo_PredefinedDevice->currentText());
    gpPredefinedDevice->setValue(QString("Port").append(QString::number(((ui->combo_PredefinedDevice->currentIndex()+1)))).append("Baud"), ui->combo_Baud->currentText());
    gpPredefinedDevice->setValue(QString("Port").append(QString::number(((ui->combo_PredefinedDevice->currentIndex()+1)))).append("Parity"), ui->combo_Parity->currentIndex());
    gpPredefinedDevice->setValue(QString("Port").append(QString::number(((ui->combo_PredefinedDevice->currentIndex()+1)))).append("Stop"), ui->combo_Stop->currentText());
    gpPredefinedDevice->setValue(QString("Port").append(QString::number(((ui->combo_PredefinedDevice->currentIndex()+1)))).append("Data"), ui->combo_Data->currentText());
    gpPredefinedDevice->setValue(QString("Port").append(QString::number(((ui->combo_PredefinedDevice->currentIndex()+1)))).append("Flow"), ui->combo_Handshake->currentIndex());
}

//=============================================================================
//=============================================================================
void
MainWindow::ContextMenuClosed(
    )
{
    //Right click context menu closed, send message to text edit object
    ui->text_TermEditData->mbContextMenuOpen = false;
    ui->text_TermEditData->UpdateDisplay();
}

//=============================================================================
//=============================================================================
bool
MainWindow::event(
    QEvent *evtEvent
    )
{
    if (evtEvent->type() == QEvent::WindowActivate && gspSerialPort.isOpen() == true && ui->selector_Tab->currentIndex() == ui->selector_Tab->indexOf(ui->tab_Term))
    {
        //Focus on the terminal
        ui->text_TermEditData->setFocus();
    }
    return QMainWindow::event(evtEvent);
}

//=============================================================================
//=============================================================================
void
MainWindow::SerialPortClosing(
    )
{
    //Called when the serial port is closing
    ui->image_CTS->setPixmap(*gpEmptyCirclePixmap);
    ui->image_DCD->setPixmap(*gpEmptyCirclePixmap);
    ui->image_DSR->setPixmap(*gpEmptyCirclePixmap);
    ui->image_RI->setPixmap(*gpEmptyCirclePixmap);

    ui->image_CTSb->setPixmap(*gpEmptyCirclePixmap);
    ui->image_DCDb->setPixmap(*gpEmptyCirclePixmap);
    ui->image_DSRb->setPixmap(*gpEmptyCirclePixmap);
    ui->image_RIb->setPixmap(*gpEmptyCirclePixmap);

#ifndef SKIPSCRIPTINGFORM
    if (gusScriptingForm != 0)
    {
        //Notify scripting form
        gusScriptingForm->SerialPortStatus(false);
    }
#endif

    //Update tooltip of system tray
    if (gbSysTrayEnabled == true)
    {
        gpSysTray->setToolTip(QString("AuTerm v").append(UwVersion));
    }

    for (int i = 0; i < plugin_list.length(); ++i)
    {
        plugin_list.at(i).plugin->serial_about_to_close();
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_LogFileSelect_clicked(
    )
{
    //Updates the log file
#ifndef SKIPAUTOMATIONFORM
    if (guaAutomationForm != 0)
    {
        guaAutomationForm->TempAlwaysOnTop(0);
    }
#endif
    QString strLogFilename = QFileDialog::getSaveFileName(this, "Select Log File", ui->edit_LogFile->text(), "Log Files (*.log);;All Files (*.*)");
#ifndef SKIPAUTOMATIONFORM
    if (guaAutomationForm != 0)
    {
        guaAutomationForm->TempAlwaysOnTop(1);
    }
#endif
    if (!strLogFilename.isEmpty())
    {
        //Update log file
        ui->edit_LogFile->setText(strLogFilename);
        on_edit_LogFile_editingFinished();
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_edit_LogFile_editingFinished(
    )
{
    //Log filename has changed
    gpTermSettings->setValue("LogFile", ui->edit_LogFile->text());
}

//=============================================================================
//=============================================================================
void
MainWindow::on_check_LogEnable_stateChanged(
    int
    )
{
    //Logging enabled/disabled changed
    gpTermSettings->setValue("LogEnable", (ui->check_LogEnable->isChecked() == true ? 1 : 0));
}

//=============================================================================
//=============================================================================
void
MainWindow::on_check_LogAppend_stateChanged(
    int
    )
{
    //Logging append/clearing changed
    gpTermSettings->setValue("LogMode", (ui->check_LogAppend->isChecked() == true ? 1 : 0));
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Help_clicked(
    )
{
#if 0
    //Opens the help PDF file
#ifdef __APPLE__
    if (QFile::exists(QString(gstrMacBundlePath).append("Help.pdf")))
#else
    if (QFile::exists("Help.pdf"))
#endif
    {
        //File present - open
#ifdef __APPLE__
        QDesktopServices::openUrl(QUrl::fromLocalFile(QString(gstrMacBundlePath).append("Help.pdf")));
#else
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo("Help.pdf").absoluteFilePath()));
#endif
    }
    else
    {
        //File not present, open on website instead
#if 0
        if (QDesktopServices::openUrl(QUrl(QString("http://").append(ServerHost).append("/AuTerm_help.pdf"))) == false)
        {
            //Failed to open
            QString strMessage = tr("Help file (Help.pdf) was not found and an error occured whilst attempting to open the online version.");
            gpmErrorForm->show();
            gpmErrorForm->SetMessage(&strMessage);
        }
#endif
    }
#endif

    QString strMessage = "You are welcome to check our website for the latest version.\r\n\r\nCommand line options are:-\r\n\r\nCOM=n\r\n    Windows: COM[1..255] specifies a comport number\r\n    GNU/Linux: /dev/tty[device] specifies a TTY device\r\n    Mac: /dev/[device] specifies a TTY device\r\n\r\nBAUD=n\r\n    [1200..5000000] (limited to 115200 for traditional UARTs)\r\nr\nSTOP=n\r\n    [1..2]\r\n\r\nDATA=n\r\n    [7..8]\r\n\r\nPAR=n\r\n    [0=None; 1=Odd; 2=Even]\r\n\r\nFLOW=n\r\n    [0=None; 1=Cts/Rts; 2=Xon/Xoff]\r\n\r\nENDCHR=n\r\n    [line termination character :: 0=\\r, 1=\\n, 2=\\r\\n]\r\n\r\nNOCONNECT\r\n    Do not connect to device on startup\r\n\r\nLOCALECHO=n\r\n    [0=Disabled; 1=Enabled]\r\n\r\nLINEMODE=n\r\n    [0=Disabled; 1=Enabled]\r\n\r\nLOG\r\n    Write screen activity to new file '<appname>.log' (Cannot be used with LOG+, LOG+ will take priority)\r\n\r\nLOG+\r\n    Append screen activity to file '<appname>.log' (Cannot be used with LOG, LOG+ will take priority)\r\n\r\nLOG=filename\r\n    File to write the log data to this file (supply extension)\r\n\r\nSHOWCRLF\r\n    When displaying a TX or RX text on screen, show \\t,\\r,\\n as well\r\n\r\nAUTOMATION\r\n    Will initialise and open the automation form\r\n\r\nAUTOMATIONFILE=filename\r\n    Provided that the file exists, it will be loaded into the automation form.\r\n\r\nSCRIPTING\r\n    Will initialise and open the scripting form\r\n\r\nSCRIPTFILE=filename\r\n    Provided that the file exists, it will be opened in the scripting form (SCRIPTING must be provided before this argument)\r\n\r\nSCRIPTACTION=n\r\n    [1=Run script after serial port has been opened] (SCRIPTING and SCRIPTFILE must be provided before this argument)\r\n\r\nTITLE=title\r\n    Will append to the window title (and system tray icon tooltip) the provided text\r\n\r\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\r\n\r\nCharacter escape codes: These are supported in the Automation, Scripting and Speed Test features and allow non-printable ASCII characters to be used. The format of character escape codes is \\HH whereby H represents a hex character (0-9 and A-F), additionally \\r, \\n and \\t can be used to represent a carriage return, new line and tab character individually.\r\nThis function is enabled/disabled in the Automation and Speed Test features by checking the 'Un-escape strings' checkbox to enable it. It cannot be disabled for the Scripting functionality.\r\nFor example: \00 can be used to represent a null character and \4C can be used to represent an 'L' ASCII character.\r\n\r\nAdapted from UwTerminalX code, copyright © Laird Connectivity 2015-2022\r\nCopyright © Jamie M. 2023\r\nFor updates and source code licensed under GPLv3, check https://github.com/thedjnK/AuTerm or the 'Update' tab.\r\n\r\nThis program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.\r\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\r\nYou should have received a copy of the GNU General Public License along with this program.  If not, see http://www.gnu.org/licenses/";
    gpmErrorForm->SetMessage(&strMessage);
    gpmErrorForm->show();
}

//=============================================================================
//=============================================================================
void
MainWindow::on_combo_LogDirectory_currentIndexChanged(
    int
    )
{
    //Refresh the list of log files
    on_btn_LogRefresh_clicked();
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_LogRefresh_clicked(
    )
{
    //Refreshes the log files available for viewing
    ui->combo_LogFile->clear();
    ui->combo_LogFile->addItem("- No file selected -");
    QString strDirPath;
    if (ui->combo_LogDirectory->currentIndex() == 1)
    {
        //Log file directory
#ifdef TARGET_OS_MAC
        QFileInfo a(QString((ui->edit_LogFile->text().left(1) == "/" || ui->edit_LogFile->text().left(1) == "\\") ? "" : QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/")).append(ui->edit_LogFile->text()));
#else
        QFileInfo a(ui->edit_LogFile->text());
#endif
        strDirPath = a.absolutePath();
    }
    else
    {
        //Application directory
#ifdef TARGET_OS_MAC
        strDirPath = QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/");
#else
        strDirPath = "./";
#endif
    }

    //Apply file filters
    QDir dirLogDir(strDirPath);
    QFileInfoList filFiles;
    filFiles = dirLogDir.entryInfoList(QStringList() << "*.log");
    if (filFiles.count() > 0)
    {
        //At least one file was found
        int i = 0;
        while (i < filFiles.count())
        {
            //List all files
            ui->combo_LogFile->addItem(filFiles.at(i).fileName());
            ++i;
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Licenses_clicked(
    )
{
    //Show license text
    QString strMessage = tr("AuTerm uses the Qt framework version 5, which is licensed under the GPLv3 (not including later versions).\nAuTerm uses and may be linked statically to various other libraries including Xau, XCB, expat, fontconfig, zlib, bz2, harfbuzz, freetype, udev, dbus, icu, unicode, OpenSSL. The licenses for these libraries are provided below:\n\n\n"
"Lib Xau:\n\nCopyright 1988, 1993, 1994, 1998  The Open Group\n\nPermission to use, copy, modify, distribute, and sell this software and its\ndocumentation for any purpose is hereby granted without fee, provided that\nthe above copyright notice appear in all copies and that both that\ncopyright notice and this permission notice appear in supporting\ndocumentation.\nThe above copyright notice and this permission notice shall be included in\nall copies or substantial portions of the Software.\nTHE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\nIMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\nFITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE\nOPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN\nAN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\nCONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\nExcept as contained in this notice, the name of The Open Group shall not be\nused in advertising or otherwise to promote the sale, use or other dealings\nin this Software without prior written authorization from The Open Group.\n\n\n"
"xcb:\n\nCopyright (C) 2001-2006 Bart Massey, Jamey Sharp, and Josh Triplett.\nAll Rights Reserved.\n\nPermission is hereby granted, free of charge, to any person\nobtaining a copy of this software and associated\ndocumentation files (the 'Software'), to deal in the\nSoftware without restriction, including without limitation\nthe rights to use, copy, modify, merge, publish, distribute,\nsublicense, and/or sell copies of the Software, and to\npermit persons to whom the Software is furnished to do so,\nsubject to the following conditions:\n\nThe above copyright notice and this permission notice shall\nbe included in all copies or substantial portions of the\nSoftware.\n\nTHE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY\nKIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE\nWARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR\nPURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS\nBE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER\nIN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\nOUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\nOTHER DEALINGS IN THE SOFTWARE.\n\nExcept as contained in this notice, the names of the authors\nor their institutions shall not be used in advertising or\notherwise to promote the sale, use or other dealings in this\nSoftware without prior written authorization from the\nauthors.\n\n\n"
"expat:\n\nCopyright (c) 1998, 1999, 2000 Thai Open Source Software Center Ltd\n   and Clark Cooper\nCopyright (c) 2001, 2002, 2003, 2004, 2005, 2006 Expat maintainers.\nPermission is hereby granted, free of charge, to any person obtaining\na copy of this software and associated documentation files (the\n'Software'), to deal in the Software without restriction, including\nwithout limitation the rights to use, copy, modify, merge, publish,\ndistribute, sublicense, and/or sell copies of the Software, and to\npermit persons to whom the Software is furnished to do so, subject to\nthe following conditions:\n\nThe above copyright notice and this permission notice shall be included\nin all copies or substantial portions of the Software.\n\nTHE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,\nEXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\nMERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\nIN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY\nCLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,\nTORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE\nSOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\n\n"
"fontconfig:\n\nCopyright © 2001,2003 Keith Packard\n\nPermission to use, copy, modify, distribute, and sell this software and its\ndocumentation for any purpose is hereby granted without fee, provided that\nthe above copyright notice appear in all copies and that both that\ncopyright notice and this permission notice appear in supporting\ndocumentation, and that the name of Keith Packard not be used in\nadvertising or publicity pertaining to distribution of the software without\nspecific, written prior permission.  Keith Packard makes no\nrepresentations about the suitability of this software for any purpose.  It\nis provided 'as is' without express or implied warranty.\n\nKEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,\nINCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO\nEVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR\nCONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,\nDATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER\nTORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR\nPERFORMANCE OF THIS SOFTWARE.\n\n\n"
"z:\n\n (C) 1995-2013 Jean-loup Gailly and Mark Adler\n\n  This software is provided 'as-is', without any express or implied\n  warranty.  In no event will the authors be held liable for any damages\n  arising from the use of this software.\n\n  Permission is granted to anyone to use this software for any purpose,\n  including commercial applications, and to alter it and redistribute it\n  freely, subject to the following restrictions:\n\n  1. The origin of this software must not be misrepresented; you must not\n     claim that you wrote the original software. If you use this software\n     in a product, an acknowledgment in the product documentation would be\n     appreciated but is not required.\n  2. Altered source versions must be plainly marked as such, and must not be\n     misrepresented as being the original software.\n  3. This notice may not be removed or altered from any source distribution.\n\n  Jean-loup Gailly        Mark Adler\n  jloup@gzip.org          madler@alumni.caltech.edu\n\n\n"
"bz2:\n\n\nThis program, 'bzip2', the associated library 'libbzip2', and all\ndocumentation, are copyright (C) 1996-2010 Julian R Seward.  All\nrights reserved.\n\nRedistribution and use in source and binary forms, with or without\nmodification, are permitted provided that the following conditions\nare met:\n\n1. Redistributions of source code must retain the above copyright\n   notice, this list of conditions and the following disclaimer.\n\n2. The origin of this software must not be misrepresented; you must\n   not claim that you wrote the original software.  If you use this\n   software in a product, an acknowledgment in the product\n   documentation would be appreciated but is not required.\n\n3. Altered source versions must be plainly marked as such, and must\n   not be misrepresented as being the original software.\n\n4. The name of the author may not be used to endorse or promote\n   products derived from this software without specific prior written\n   permission.\n\nTHIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS\nOR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\nWARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\nARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY\nDIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\nDAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE\nGOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\nINTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,\nWHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\nNEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\nSOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\nJulian Seward, jseward@bzip.org\nbzip2/libbzip2 version 1.0.6 of 6 September 2010\n\n\n"
"harfbuzz:\n\nHarfBuzz is licensed under the so-called 'Old MIT' license.  Details follow.\n\nCopyright © 2010,2011,2012  Google, Inc.\nCopyright © 2012  Mozilla Foundation\nCopyright © 2011  Codethink Limited\nCopyright © 2008,2010  Nokia Corporation and/or its subsidiary(-ies)\nCopyright © 2009  Keith Stribley\nCopyright © 2009  Martin Hosken and SIL International\nCopyright © 2007  Chris Wilson\nCopyright © 2006  Behdad Esfahbod\nCopyright © 2005  David Turner\nCopyright © 2004,2007,2008,2009,2010  Red Hat, Inc.\nCopyright © 1998-2004  David Turner and Werner Lemberg\n\nFor full copyright notices consult the individual files in the package.\n\nPermission is hereby granted, without written agreement and without\nlicense or royalty fees, to use, copy, modify, and distribute this\nsoftware and its documentation for any purpose, provided that the\nabove copyright notice and the").append(" following two paragraphs appear in\nall copies of this software.\n\nIN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR\nDIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES\nARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN\nIF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH\nDAMAGE.\n\nTHE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,\nBUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND\nFITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS\nON AN 'AS IS' BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO\nPROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.\n\n\n"
"freetype:\n\nThe  FreeType 2  font  engine is  copyrighted  work and  cannot be  used\nlegally  without a  software license.   In  order to  make this  project\nusable  to a vast  majority of  developers, we  distribute it  under two\nmutually exclusive open-source licenses.\n\nThis means  that *you* must choose  *one* of the  two licenses described\nbelow, then obey  all its terms and conditions when  using FreeType 2 in\nany of your projects or products.\n\n  - The FreeType License, found in  the file `FTL.TXT', which is similar\n    to the original BSD license *with* an advertising clause that forces\n    you  to  explicitly cite  the  FreeType  project  in your  product's\n    documentation.  All  details are in the license  file.  This license\n    is  suited  to products  which  don't  use  the GNU  General  Public\n    License.\n\n    Note that  this license  is  compatible  to the  GNU General  Public\n    License version 3, but not version 2.\n\n  - The GNU General Public License version 2, found in  `GPLv2.TXT' (any\n    later version can be used  also), for programs which already use the\n    GPL.  Note  that the  FTL is  incompatible  with  GPLv2 due  to  its\n    advertisement clause.\n\nThe contributed BDF and PCF drivers come with a license similar  to that\nof the X Window System.  It is compatible to the above two licenses (see\nfile src/bdf/README and src/pcf/README).\n\nThe gzip module uses the zlib license (see src/gzip/zlib.h) which too is\ncompatible to the above two licenses.\n\nThe MD5 checksum support (only used for debugging in development builds)\nis in the public domain.\n\n\n"
"udev:\n\nCopyright (C) 2003 Greg Kroah-Hartman <greg@kroah.com>\nCopyright (C) 2003-2010 Kay Sievers <kay@vrfy.org>\n\nThis program is free software: you can redistribute it and/or modify\nit under the terms of the GNU General Public License as published by\nthe Free Software Foundation, either version 2 of the License, or\n(at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n\n\n"
"dbus:\n\nD-Bus is licensed to you under your choice of the Academic Free\nLicense version 2.1, or the GNU General Public License version 2\n(or, at your option any later version).\n\n\n"
"icu:\n\nICU License - ICU 1.8.1 and later\nCOPYRIGHT AND PERMISSION NOTICE\nCopyright (c) 1995-2015 International Business Machines Corporation and others\nAll rights reserved.\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the 'Software'), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, provided that the above copyright notice(s) and this permission notice appear in all copies of the Software and that both the above copyright notice(s) and this permission notice appear in supporting documentation.\nTHE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\nExcept as contained in this notice, the name of a copyright holder shall not be used in advertising or otherwise to promote the sale, use or other dealings in this Software without prior written authorization of the copyright holder.\n\n\n"
"Unicode:\n\nCOPYRIGHT AND PERMISSION NOTICE\n\nCopyright © 1991-2015 Unicode, Inc. All rights reserved.\nDistributed under the Terms of Use in\nhttp://www.unicode.org/copyright.html.\n\nPermission is hereby granted, free of charge, to any person obtaining\na copy of the Unicode data files and any associated documentation\n(the 'Data Files') or Unicode software and any associated documentation\n(the 'Software') to deal in the Data Files or Software\nwithout restriction, including without limitation the rights to use,\ncopy, modify, merge, publish, distribute, and/or sell copies of\nthe Data Files or Software, and to permit persons to whom the Data Files\nor Software are furnished to do so, provided that\n(a) this copyright and permission notice appear with all copies\nof the Data Files or Software,\n(b) this copyright and permission notice appear in associated\ndocumentation, and\n(c) there is clear notice in each modified Data File or in the Software\nas well as in the documentation associated with the Data File(s) or\nSoftware that the data or software has been modified.\n\nTHE DATA FILES AND SOFTWARE ARE PROVIDED 'AS IS', WITHOUT WARRANTY OF\nANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE\nWARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\nNONINFRINGEMENT OF THIRD PARTY RIGHTS.\nIN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS\nNOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL\nDAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,\nDATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER\nTORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR\nPERFORMANCE OF THE DATA FILES OR SOFTWARE.\n\nExcept as contained in this notice, the name of a copyright holder\nshall not be used in advertising or otherwise to promote the sale,\nuse or other dealings in these Data Files or Software without prior\nwritten authorization of the copyright holder.\n\n\n"
"OpenSSL:\r\n\r\nCopyright (c) 1998-2016 The OpenSSL Project.  All rights reserved.\r\n\r\nRedistribution and use in source and binary forms, with or without\r\nmodification, are permitted provided that the following conditions\r\nare met:\r\n\r\n1. Redistributions of source code must retain the above copyright\r\n   notice, this list of conditions and the following disclaimer. \r\n\r\n2. Redistributions in binary form must reproduce the above copyright\r\n   notice, this list of conditions and the following disclaimer in\r\n   the documentation and/or other materials provided with the\r\n   distribution.\r\n\r\n3. All advertising materials mentioning features or use of this\r\n   software must display the following acknowledgment:\r\n   'This product includes software developed by the OpenSSL Project\r\n   for use in the OpenSSL Toolkit. (http://www.openssl.org/)'\r\n\r\n4. The names 'OpenSSL Toolkit' and 'OpenSSL Project' must not be used to\r\n   endorse or promote products derived from this software without\r\n   prior written permission. For written permission, please contact\r\n   openssl-core@openssl.org.\r\n\r\n5. Products derived from this software may not be called 'OpenSSL'\r\n   nor may 'OpenSSL' appear in their names without prior written\r\n   permission of the OpenSSL Project.\r\n\r\n6. Redistributions of any form whatsoever must retain the following\r\n   acknowledgment:\r\n   'This product includes software developed by the OpenSSL Project\r\n   for use in the OpenSSL Toolkit (http://www.openssl.org/)'\r\n\r\nTHIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY\r\nEXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\r\nIMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\r\nPURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR\r\nITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\r\nSPECIAL, EXEMPLARY, OR").append(" CONSEQUENTIAL DAMAGES (INCLUDING, BUT\r\nNOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\r\nLOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\r\nHOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,\r\nSTRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\r\nARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED\r\nOF THE POSSIBILITY OF SUCH DAMAGE.\r\n====================================================================\r\n\r\nThis product includes cryptographic software written by Eric Young\r\n(eay@cryptsoft.com).  This product includes software written by Tim\r\nHudson (tjh@cryptsoft.com).\r\n\r\n\r\n Original SSLeay License\r\n -----------------------\r\n\r\nCopyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)\r\nAll rights reserved.\r\n\r\nThis package is an SSL implementation written\r\nby Eric Young (eay@cryptsoft.com).\r\nThe implementation was written so as to conform with Netscapes SSL.\r\n\r\nThis library is free for commercial and non-commercial use as long as\r\nthe following conditions are aheared to.  The following conditions\r\napply to all code found in this distribution, be it the RC4, RSA,\r\nlhash, DES, etc., code; not just the SSL code.  The SSL documentation\r\nincluded with this distribution is covered by the same copyright terms\r\nexcept that the holder is Tim Hudson (tjh@cryptsoft.com).\r\n\r\nCopyright remains Eric Young's, and as such any Copyright notices in\r\nthe code are not to be removed.\r\nIf this package is used in a product, Eric Young should be given attribution\r\nas the author of the parts of the library used.\r\nThis can be in the form of a textual message at program startup or\r\nin documentation (online or textual) provided with the package.\r\n\r\nRedistribution and use in source and binary forms, with or without\r\nmodification, are permitted provided that the following conditions\r\nare met:\r\n1. Redistributions of source code must retain the copyright\r\n   notice, this list of conditions and the following disclaimer.\r\n2. Redistributions in binary form must reproduce the above copyright\r\n   notice, this list of conditions and the following disclaimer in the\r\n   documentation and/or other materials provided with the distribution.\r\n3. All advertising materials mentioning features or use of this software\r\n   must display the following acknowledgement:\r\n   'This product includes cryptographic software written by\r\n    Eric Young (eay@cryptsoft.com)'\r\n   The word 'cryptographic' can be left out if the rouines from the library\r\n   being used are not cryptographic related :-).\r\n4. If you include any Windows specific code (or a derivative thereof) from \r\n   the apps directory (application code) you must include an acknowledgement:\r\n   'This product includes software written by Tim Hudson (tjh@cryptsoft.com)'\r\n\r\nTHIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND\r\nANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\r\nIMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\r\nARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE\r\nFOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\r\nDAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\r\nOR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\r\nHOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\r\nLIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\r\nOUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\r\nSUCH DAMAGE.\r\n\r\nThe licence and distribution terms for any publically available version or\r\nderivative of this code cannot be changed.  i.e. this code cannot simply be\r\ncopied and put under another distribution licence\r\n[including the GNU Public Licence.]\r\n");
    gpmErrorForm->show();
    gpmErrorForm->SetMessage(&strMessage);
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_EditViewFolder_clicked(
    )
{
    //Open application folder
#ifdef TARGET_OS_MAC
    QString strFullDirname = QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/");
#else
    QString strFullDirname = "./";
#endif

    //Open folder
    QDesktopServices::openUrl(QUrl::fromLocalFile(strFullDirname));
}

//=============================================================================
//=============================================================================
void
MainWindow::on_combo_EditFile_currentIndexChanged(
    int
    )
{
    if (gbEditFileModified == true)
    {
        //Confirm if user wants to discard changes
        if (QMessageBox::question(this, "Discard changes?", "You have unsaved changes to this file, do you wish to discard them and load another file?", QMessageBox::Yes|QMessageBox::No) == QMessageBox::No)
        {
            ui->combo_EditFile->setCurrentIndex(giEditFileType);
            return;
        }
    }

    //Clear the edit box and status text
    ui->text_EditData->clear();
    ui->label_EditInfo->setText("");

    if (ui->combo_EditFile->currentIndex() != 0)
    {
        //Load file data
        QString strFullFilename;

        //Allow edits
        ui->text_EditData->setReadOnly(false);

#ifdef TARGET_OS_MAC
        strFullFilename = QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/");
#else
        strFullFilename = "./";
#endif

        //Create the full filename
        strFullFilename = strFullFilename.append("/").append(ui->combo_EditFile->currentIndex() == 1 ? "AuTerm.ini" : "Devices.ini");

        //Open the log file for reading
        QFile fileLogFile(strFullFilename);
        if (fileLogFile.open(QFile::ReadOnly | QFile::Text))
        {
            //Get the contents of the log file
            ui->text_EditData->setPlainText(fileLogFile.readAll());
            fileLogFile.close();
            gbEditFileModified = false;
            giEditFileType = ui->combo_EditFile->currentIndex();

            //Information about the file
            QFileInfo fiFileInfo(strFullFilename);
            char cPrefixes[4] = {'K', 'M', 'G', 'T'};
            float fltFilesize = fiFileInfo.size();
            unsigned char cPrefix = 0;
            while (fltFilesize > 1024)
            {
                //Go to next prefix
                fltFilesize = fltFilesize/1024;
                ++cPrefix;
            }

            //Create the filesize string
            QString strFilesize = QString::number(fltFilesize);
            if (strFilesize.indexOf(".") != -1 && strFilesize.length() > (strFilesize.indexOf(".") + 3))
            {
                //Reduce filesize length
                strFilesize = strFilesize.left((strFilesize.indexOf(".") + 3));
            }

            //Update the string to file information of the current file
            ui->label_EditInfo->setText(QString("Created: ").append(fiFileInfo.birthTime().toString("hh:mm dd/MM/yyyy")).append(", Modified: ").append(fiFileInfo.lastModified().toString("hh:mm dd/MM/yyyy")).append(", Size: ").append(strFilesize));

            //Check if a prefix needs adding
            if (cPrefix > 0)
            {
                //Add size prefix
                ui->label_EditInfo->setText(ui->label_EditInfo->text().append(cPrefixes[cPrefix-1]));
            }

            //Append the Byte unit
            ui->label_EditInfo->setText(ui->label_EditInfo->text().append("B"));
        }
        else
        {
            //Configuration file opening failed
            ui->label_EditInfo->setText("Failed to open file.");
        }
    }
    else
    {
        //Lock edit from input
        ui->text_EditData->setReadOnly(true);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_EditSave_clicked(
    )
{
    if (ui->combo_EditFile->currentIndex() != 0)
    {
        //Save file data
        QString strFullFilename;

#ifdef TARGET_OS_MAC
        strFullFilename = QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/");
#else
        strFullFilename = "./";
#endif

        //Create the full filename
        strFullFilename = strFullFilename.append("/").append(ui->combo_EditFile->currentIndex() == 1 ? "AuTerm.ini" : "Devices.ini");

        //Open the log file for reading
        QFile fileLogFile(strFullFilename);
        if (fileLogFile.open(QFile::WriteOnly | QFile::Text))
        {
            //Get the contents of the log file
            fileLogFile.write(ui->text_EditData->toPlainText().toUtf8());
            fileLogFile.close();
            gbEditFileModified = false;
        }
        else
        {
            //Log file opening failed
            ui->label_EditInfo->setText("Failed to open file for writing.");
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_EditLoad_clicked(
    )
{
    if (ui->combo_EditFile->currentIndex() != 0)
    {
        //Reload file data
        on_combo_EditFile_currentIndexChanged(0);
    }
}

#ifndef __APPLE__
//=============================================================================
//=============================================================================
void
MainWindow::on_btn_EditExternal_clicked(
    )
{
    if (ui->combo_EditFile->currentIndex() == 1)
    {
        //Opens the AuTerm configuration file
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo("AuTerm.ini").absoluteFilePath()));
    }
    else if (ui->combo_EditFile->currentIndex() == 2)
    {
        //Opens the predefined devices configuration file
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo("Devices.ini").absoluteFilePath()));
    }
}
#endif

//=============================================================================
//=============================================================================
void
MainWindow::LoadSettings(
    )
{
#ifdef TARGET_OS_MAC
    if (!QDir().exists(QStandardPaths::writableLocation(QStandardPaths::DataLocation)))
    {
        //Create AuTerm directory in application support
        QDir().mkdir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    }
    gpTermSettings = new QSettings(QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/AuTerm.ini"), QSettings::IniFormat); //Handle to settings
    gpErrorMessages = new QSettings(QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/codes.csv"), QSettings::IniFormat); //Handle to error codes
    gpPredefinedDevice = new QSettings(QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/Devices.ini"), QSettings::IniFormat); //Handle to predefined devices
#else
    //Open files in same directory
    gpTermSettings = new QSettings(QString("AuTerm.ini"), QSettings::IniFormat); //Handle to settings
    gpErrorMessages = new QSettings(QString("codes.csv"), QSettings::IniFormat); //Handle to error codes
    gpPredefinedDevice = new QSettings(QString("Devices.ini"), QSettings::IniFormat); //Handle to predefined devices
#endif

    //Check if error code file exists
#ifdef TARGET_OS_MAC
    if (QFile::exists(QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/codes.csv")))
#else
    if (QFile::exists("codes.csv"))
#endif
    {
        //Error code file has been loaded
        gbErrorsLoaded = true;
    }

    //Check settings
#ifdef TARGET_OS_MAC
    if (!QFile::exists(QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/AuTerm.ini")) || gpTermSettings->value("ConfigVersion").toString() != UwVersion)
#else
    if (!QFile::exists("AuTerm.ini") || gpTermSettings->value("ConfigVersion").toString() != UwVersion)
#endif
    {
        //Extract old configuration version
        if (!gpTermSettings->value("ConfigVersion").isNull())
        {
            QRegularExpression reTempRE("([0-9]+)\\.([0-9]+)([a-zA-Z]{0,1})");
            QRegularExpressionMatch remTempREM = reTempRE.match(gpTermSettings->value("ConfigVersion").toString());
            if (remTempREM.hasMatch() == true)
            {
                //Update configuration
                UpdateSettings(remTempREM.captured(1).toInt(), remTempREM.captured(2).toInt(), (remTempREM.captured(3).isEmpty() || remTempREM.captured(3).isNull() ? '0' : remTempREM.captured(3).at(0)));
            }
        }

        //No settings, or some config values not present defaults;
        if (gpTermSettings->value("LogFile").isNull())
        {
            gpTermSettings->setValue("LogFile", DefaultLogFile); //Default log file
        }
        if (gpTermSettings->value("LogMode").isNull())
        {
            gpTermSettings->setValue("LogMode", DefaultLogMode); //Clear log before opening, 0 = no, 1 = yes
        }
        if (gpTermSettings->value("LogEnable").isNull())
        {
            gpTermSettings->setValue("LogEnable", DefaultLogEnable); //0 = disabled, 1 = enable
        }
        if (gpTermSettings->value("SysTrayIcon").isNull())
        {
            gpTermSettings->setValue("SysTrayIcon", DefaultSysTrayIcon); //0 = no, 1 = yes (Shows a system tray icon and provides balloon messages)
        }
        if (gpTermSettings->value("SerialSignalCheckInterval").isNull())
        {
            gpTermSettings->setValue("SerialSignalCheckInterval", DefaultSerialSignalCheckInterval); //How often to check status of CTS, DSR, etc. signals in mS (lower = faster but more CPU usage)
        }
        if (gpTermSettings->value("TextUpdateInterval").isNull())
        {
            gpTermSettings->setValue("TextUpdateInterval", DefaultTextUpdateInterval); //Interval between screen updates in mS, lower = faster but can be problematic when receiving/sending large amounts of data (200 is good for this)
        }
        if (gpTermSettings->value("ShiftEnterLineSeparator").isNull())
        {
            gpTermSettings->setValue("ShiftEnterLineSeparator", DefaultShiftEnterLineSeparator); //Shift+enter input (1 = line separater, 0 = newline character)
        }
        if (gpTermSettings->value("AutoTrimDBuffer").isNull())
        {
            gpTermSettings->setValue("AutoTrimDBuffer", DefaultAutoDTrimBuffer); //(Unlisted option) Automatically trim display buffer if size exceeds threshold (1 = enable, 0 = disable)
        }
        if (gpTermSettings->value("AutoTrimDBufferThreshold").isNull())
        {
            gpTermSettings->setValue("AutoTrimDBufferThreshold", DefaultAutoTrimDBufferThreshold); //(Unlisted option) Threshold level for automatically trimming display buffer
        }
        if (gpTermSettings->value("AutoTrimDBufferSize").isNull())
        {
            gpTermSettings->setValue("AutoTrimDBufferSize", DefaultAutoTrimDBufferSize); //(Unlisted option) Amount of data to leave after trimming display buffer
        }
        if (gpTermSettings->value("ScrollbackBufferSize").isNull())
        {
            gpTermSettings->setValue("ScrollbackBufferSize", DefaultScrollbackBufferSize); //The number of lines in the terminal scrollback buffer
        }
#ifdef UseSSL
        if (gpTermSettings->value("SSLEnable").isNull())
        {
            gpTermSettings->setValue("SSLEnable", DefaultSSLEnable); //If SSL should be used for online functionality or not (1 = use SSL, 0 = use HTTP)
            if (DefaultSSLEnable == true)
            {
                //HTTPS
                WebProtocol = "https";
            }
            else
            {
                //HTTP
                WebProtocol = "http";
            }
        }
#endif

        if (gpTermSettings->value("ConfigVersion").isNull() || gpTermSettings->value("ConfigVersion").toString() != UwVersion)
        {
            //Update configuration version
            gpTermSettings->setValue("ConfigVersion", UwVersion);
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::UpdateSettings(
    int intMajor,
    int intMinor,
    QChar qcDelta
    )
{
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_LogViewExternal_clicked(
    )
{
    //View log in external editor
    if (ui->combo_LogFile->currentIndex() >= 1)
    {
        //Create the full filename
        QString strFullFilename;

        if (ui->combo_LogDirectory->currentIndex() == 1)
        {
            //Log file directory
#ifdef TARGET_OS_MAC
            QFileInfo fiFileInfo(QString((ui->edit_LogFile->text().left(1) == "/" || ui->edit_LogFile->text().left(1) == "\\") ? "" : QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/")).append(ui->edit_LogFile->text()));
#else
            QFileInfo fiFileInfo(ui->edit_LogFile->text());
#endif
            strFullFilename = fiFileInfo.absolutePath();
        }
        else
        {
            //Application directory
#ifdef TARGET_OS_MAC
            strFullFilename = QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/");
#else
            strFullFilename = "./";
#endif
        }
        strFullFilename = strFullFilename.append("/").append(ui->combo_LogFile->currentText());

        //Open file
        QDesktopServices::openUrl(QUrl::fromLocalFile(strFullFilename));
    }
    else
    {
        //Close
        ui->text_LogData->clear();
        ui->label_LogInfo->clear();
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_LogViewFolder_clicked(
    )
{
    //Open log folder
    QString strFullDirname;

    if (ui->combo_LogDirectory->currentIndex() == 1)
    {
        //Log file directory
#ifdef TARGET_OS_MAC
        QFileInfo fiFileInfo(QString((ui->edit_LogFile->text().left(1) == "/" || ui->edit_LogFile->text().left(1) == "\\") ? "" : QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/")).append(ui->edit_LogFile->text()));
#else
        QFileInfo fiFileInfo(ui->edit_LogFile->text());
#endif
        strFullDirname = fiFileInfo.absolutePath();
    }
    else
    {
        //Application directory
#ifdef TARGET_OS_MAC
        strFullDirname = QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/");
#else
        strFullDirname = "./";
#endif
    }

    //Open folder
    QDesktopServices::openUrl(QUrl::fromLocalFile(strFullDirname));
}

//=============================================================================
//=============================================================================
void
MainWindow::on_text_EditData_textChanged(
    )
{
    //Mark file as edited
    gbEditFileModified = true;
}

//=============================================================================
//=============================================================================
void
MainWindow::on_combo_LogFile_currentIndexChanged(
    int
    )
{
    //List item changed - load log file
    QString strFullFilename;

    if (ui->combo_LogDirectory->currentIndex() == 1)
    {
        //Log file directory
#ifdef TARGET_OS_MAC
        QFileInfo fiFileInfo(QString((ui->edit_LogFile->text().left(1) == "/" || ui->edit_LogFile->text().left(1) == "\\") ? "" : QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/")).append(ui->edit_LogFile->text()));
#else
        QFileInfo fiFileInfo(ui->edit_LogFile->text());
#endif
        strFullFilename = fiFileInfo.absolutePath();
    }
    else
    {
        //Application directory
#ifdef TARGET_OS_MAC
        strFullFilename = QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).append("/");
#else
        strFullFilename = "./";
#endif
    }

    ui->text_LogData->clear();
    ui->label_LogInfo->clear();
    if (ui->combo_LogFile->currentIndex() >= 1)
    {
        //Create the full filename
        strFullFilename = strFullFilename.append("/").append(ui->combo_LogFile->currentText());

        //Open the log file for reading
        QFile fileLogFile(strFullFilename);
        if (fileLogFile.open(QFile::ReadOnly | QFile::Text))
        {
            //Get the contents of the log file
            ui->text_LogData->setPlainText(fileLogFile.readAll().replace('\0', "\\00").replace("\x01", "\\01").replace("\x02", "\\02").replace("\x03", "\\03").replace("\x04", "\\04").replace("\x05", "\\05").replace("\x06", "\\06").replace("\x07", "\\07").replace("\x08", "\\08").replace("\x0b", "\\0B").replace("\x0c", "\\0C").replace("\x0e", "\\0E").replace("\x0f", "\\0F").replace("\x10", "\\10").replace("\x11", "\\11").replace("\x12", "\\12").replace("\x13", "\\13").replace("\x14", "\\14").replace("\x15", "\\15").replace("\x16", "\\16").replace("\x17", "\\17").replace("\x18", "\\18").replace("\x19", "\\19").replace("\x1a", "\\1a").replace("\x1b", "\\1b").replace("\x1c", "\\1c").replace("\x1d", "\\1d").replace("\x1e", "\\1e").replace("\x1f", "\\1f"));
            fileLogFile.close();

            //Information about the log file
            QFileInfo fiFileInfo(strFullFilename);
            char cPrefixes[4] = {'K', 'M', 'G', 'T'};
            float fltFilesize = fiFileInfo.size();
            unsigned char cPrefix = 0;
            while (fltFilesize > 1024)
            {
                //Go to next prefix
                fltFilesize = fltFilesize/1024;
                ++cPrefix;
            }

            //Create the filesize string
            QString strFilesize = QString::number(fltFilesize);
            if (strFilesize.indexOf(".") != -1 && strFilesize.length() > (strFilesize.indexOf(".") + 3))
            {
                //Reduce filesize length
                strFilesize = strFilesize.left((strFilesize.indexOf(".") + 3));
            }

            //Update the string to file information of the current log
            ui->label_LogInfo->setText(QString("Created: ").append(fiFileInfo.birthTime().toString("hh:mm dd/MM/yyyy")).append(", Modified: ").append(fiFileInfo.lastModified().toString("hh:mm dd/MM/yyyy")).append(", Size: ").append(strFilesize));

            //Check if a prefix needs adding
            if (cPrefix > 0)
            {
                //Add size prefix
                ui->label_LogInfo->setText(ui->label_LogInfo->text().append(cPrefixes[cPrefix-1]));
            }

            //Append the Byte unit
            ui->label_LogInfo->setText(ui->label_LogInfo->text().append("B"));
        }
        else
        {
            //Log file opening failed
            ui->label_LogInfo->setText("Failed to open log file.");
        }
    }
    else
    {
        //Close
        ui->text_LogData->clear();
        ui->label_LogInfo->clear();
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_ReloadLog_clicked(
    )
{
    //Reload log
    on_combo_LogFile_currentIndexChanged(ui->combo_LogFile->currentIndex());
}

//=============================================================================
//=============================================================================
#ifdef UseSSL
void
MainWindow::on_check_EnableSSL_stateChanged(
    int
    )
{
    //Update SSL preference
    gpTermSettings->setValue("SSLEnable", ui->check_EnableSSL->isChecked());
    if (ui->check_EnableSSL->isChecked() == true)
    {
        //HTTPS
        WebProtocol = "https";
    }
    else
    {
        //HTTP
        WebProtocol = "http";
    }
}
#endif

//=============================================================================
//=============================================================================
QString
MainWindow::CleanFilesize(
    QString strFilename
    )
{
    //Cleanly calculates the filesize of an application rounding up
    QFile fileFileName(strFilename);
    if (!fileFileName.exists())
    {
        //File does not exist
        return "Size not known";
    }
    else
    {
        //File exists
        float intSize = fileFileName.size();
        if (intSize > 1073741824)
        {
            //GB (If this occurs then something went very, very wrong)
            intSize = std::ceil(intSize/10737418.24)/100;
            return RemoveZeros(QString::number(intSize, 'f', 2)).append("GB");
        }
        else if (intSize > 1048576)
        {
            //MB (This should never occur)
            intSize = std::ceil(intSize/10485.76)/100;
            return RemoveZeros(QString::number(intSize, 'f', 2)).append("MB");
        }
        else if (intSize > 1024)
        {
            //KB
            intSize = std::ceil(intSize/10.24)/100;
            return RemoveZeros(QString::number(intSize, 'f', 2)).append("KB");
        }
        else
        {
            //Bytes
            intSize = std::ceil(intSize*100)/100;
            return RemoveZeros(QString::number(intSize, 'f', 2)).append("B");
        }
    }
}

//=============================================================================
//=============================================================================
QString
MainWindow::RemoveZeros(
    QString strData
    )
{
    //Removes trailing zeros and decimal point
    if (strData.right(2) == "00")
    {
        //Remove trailing zeros and decimal point
        return strData.left(strData.size()-3);
    }
    else if (strData.right(1) == "0")
    {
        //Remove trailing zero
        return strData.left(strData.size()-1);
    }
    else
    {
        //Nothing to remove
        return strData;
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_check_LineSeparator_stateChanged(
    int
    )
{
    //Update line separator setting
    gpTermSettings->setValue("ShiftEnterLineSeparator", (ui->check_LineSeparator->isChecked() == true ? 1 : 0));

    //Notify scroll edit
    ui->text_TermEditData->mbLineSeparator = ui->check_LineSeparator->isChecked();
}

//=============================================================================
//=============================================================================
#ifndef SKIPERRORCODEFORM
void
MainWindow::on_btn_Error_clicked(
    )
{
    //Open error form dialogue
    if (gecErrorCodeForm == 0)
    {
        //Initialise error code form
        gecErrorCodeForm = new UwxErrorCode(this);
        gecErrorCodeForm->SetErrorObject(gpErrorMessages);
    }
    gecErrorCodeForm->show();
}
#endif

#ifndef SKIPSCRIPTINGFORM
//=============================================================================
//=============================================================================
void
MainWindow::ScriptStartRequest(
    )
{
    //Request from scripting form to start running script
    unsigned char chReason = ScriptingReasonOK;

    if (!gspSerialPort.isOpen())
    {
        //Serial port is not open
        chReason = ScriptingReasonPortClosed;
    }
    else if (gbTermBusy == true || gbSpeedTestRunning == true)
    {
        //Terminal is busy
        chReason = ScriptingReasonTermBusy;
    }
    else
    {
        //Disable loopback mode if active
        SetLoopBackMode(false);

        //Terminal is free: allow script execution
        gbScriptingRunning = true;
        gbTermBusy = true;
        gchTermMode = 50;
    }

    //Return the result to the scripting form
    gusScriptingForm->ScriptStartResult(gbScriptingRunning, chReason);
}

//=============================================================================
//=============================================================================
void
MainWindow::ScriptFinished(
    )
{
    //Script execution has finished
    gbTermBusy = false;
    gbScriptingRunning = false;
    gchTermMode = 0;
}
#endif

#if SKIPSPEEDTEST != 1
//=============================================================================
//=============================================================================
void
MainWindow::on_check_SpeedRTS_stateChanged(
    int
    )
{
    //RTS checkbox on speed test page state changed
    ui->check_RTS->setChecked(ui->check_SpeedRTS->isChecked());
}

//=============================================================================
//=============================================================================
void
MainWindow::on_check_SpeedDTR_stateChanged(
    int
    )
{
    //DTR checkbox on speed test page state changed
    ui->check_DTR->setChecked(ui->check_SpeedDTR->isChecked());
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_SpeedClear_clicked(
    )
{
    //Clear speed test display
    ui->text_SpeedEditData->clear();
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_SpeedStartStop_clicked(
    )
{
    //Speed test start/stop button pressed
    if (ui->btn_SpeedStartStop->text().indexOf(tr("ancel")) == -1)
    {
        //Speed testing start button pressed, Show speed test menu
        gpSpeedMenu->popup(QCursor::pos());
    }
    else
    {
        //Speed testing stop button pressed
        if (gtmrSpeedTestDelayTimer != 0)
        {
            //Clean up delayed send data timer
            if (gtmrSpeedTestDelayTimer->isActive())
            {
                if (gchSpeedTestMode == SpeedModeRecv)
                {
                    //Cancel instantly
                    gchSpeedTestMode = SpeedModeInactive;
                }
                gtmrSpeedTestDelayTimer->stop();
            }
            disconnect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStartTimer()));
            disconnect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStopTimer()));
            delete gtmrSpeedTestDelayTimer;
            gtmrSpeedTestDelayTimer = 0;
        }

        if ((gchSpeedTestMode == SpeedModeSendRecv || gchSpeedTestMode == SpeedModeRecv) && (gintSpeedBytesReceived10s > 0 || ui->edit_SpeedBytesRec10s->text().toInt() > 0))
        {
            //Data has been received in the past 10 seconds: start a timer before stopping to catch the extra data packets
            gchSpeedTestMode = SpeedModeRecv;
            gtmrSpeedTestDelayTimer = new QTimer();
            gtmrSpeedTestDelayTimer->setSingleShot(true);
            connect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStopTimer()));
            gtmrSpeedTestDelayTimer->start(5000);

            //Show message that test will end soon
            ui->statusBar->showMessage("Waiting 5 seconds for packets to be received... Click cancel again to stop instantly.");
        }
        else if (gchSpeedTestMode == SpeedModeSendRecv || gchSpeedTestMode == SpeedModeSend)
        {
            //Delay for 5 seconds for buffer to clear
            gchSpeedTestMode = SpeedModeInactive;
            gtmrSpeedTestDelayTimer = new QTimer();
            gtmrSpeedTestDelayTimer->setSingleShot(true);
            connect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStopTimer()));
            gtmrSpeedTestDelayTimer->start(5000);

            //Show message that test will end soon
            ui->statusBar->showMessage("Waiting 5 seconds for buffers to empty... Click cancel again to stop instantly.");
        }
        else
        {
            //Change control status
            ui->btn_SpeedStartStop->setText(tr("&Start Test"));
            ui->check_SpeedSyncReceive->setEnabled(true);
            ui->combo_SpeedDataType->setEnabled(true);
            if (ui->combo_SpeedDataType->currentIndex() == 1)
            {
                //Enable string options
                ui->edit_SpeedTestData->setEnabled(true);
                ui->check_SpeedStringUnescape->setEnabled(true);
            }

            //Update values
            OutputSpeedTestAvgStats((gtmrSpeedTimer.nsecsElapsed() < 1000000000LL ? 1000000000LL : gtmrSpeedTimer.nsecsElapsed()/1000000000LL));

            //Set speed test as no longer running
            gchSpeedTestMode = SpeedModeInactive;
            gbSpeedTestRunning = false;

            if (gtmrSpeedTimer.isValid())
            {
                //Invalidate speed test timer
                gtmrSpeedTimer.invalidate();
            }
            if (gtmrSpeedTestStats.isActive())
            {
                //Stop stats update timer
                gtmrSpeedTestStats.stop();
            }
            if (gtmrSpeedTestStats10s.isActive())
            {
                //Stop 10 second stats update timer
                gtmrSpeedTestStats10s.stop();
            }

            //Clear buffers
            gbaSpeedMatchData.clear();
            gbaSpeedReceivedData.clear();

            //Show message that test has finished
            ui->statusBar->showMessage("Speed testing finished.");
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_SpeedClose_clicked(
    )
{
    //Close/open port on speed test page pressed
    on_btn_TermClose_clicked();
}

//=============================================================================
//=============================================================================
void
MainWindow::SpeedMenuSelected(
    QAction *qaAction
    )
{
    //Speed test menu item selected
    qint8 chItem = qaAction->data().toInt();

    if (gspSerialPort.isOpen() == true && gbTermBusy == false)
    {
        if (gbLoopbackMode == true)
        {
            QString strMessage = tr("Error: Cannot initiate speed testing as loopback mode is enabled.");
            gpmErrorForm->show();
            gpmErrorForm->SetMessage(&strMessage);
            return;
        }

        //Check size of string if sending data
        if (ui->combo_SpeedDataType->currentIndex() != 0 && !(ui->edit_SpeedTestData->text().length() > 3))
        {
            //Invalid string size
            QString strMessage = tr("Error: Test data string must be a minimum of 4 bytes for speed testing.");
            gpmErrorForm->show();
            gpmErrorForm->SetMessage(&strMessage);
            return;
        }

        //Enable testing
        gintSpeedTestDataBits = gspSerialPort.dataBits();
        gintSpeedTestStartStopParityBits = gspSerialPort.stopBits() + 1 + (gspSerialPort.parity() == QSerialPort::NoParity ? 0 : 1); //Odd/even parity is one bit and include start bit
        gintSpeedTestBytesBits = ui->combo_SpeedDataDisplay->currentIndex();
        gbSpeedTestRunning = true;
        ui->btn_SpeedStartStop->setText(tr("&Cancel"));
        ui->check_SpeedSyncReceive->setEnabled(false);
        ui->combo_SpeedDataType->setEnabled(false);
        ui->edit_SpeedTestData->setEnabled(false);
        ui->check_SpeedStringUnescape->setEnabled(false);

        if (gtmrSpeedTimer.isValid())
        {
            //Invalidate timer so it can be restarted
            gtmrSpeedTimer.invalidate();
        }
        gtmrSpeedTimer.start();

        //Reset all counters and variables
        gintSpeedBytesReceived = 0;
        gintSpeedBytesReceived10s = 0;
        gintSpeedBytesSent = 0;
        gintSpeedBytesSent10s = 0;
        gintSpeedBufferCount = 0;
        gintSpeedTestStatPacketsSent = 0;
        gintSpeedTestStatPacketsReceived = 0;
        gintSpeedTestReceiveIndex = 0;
        gintSpeedTestStatSuccess = 0;
        gintSpeedTestStatErrors = 0;
        gbSpeedTestReceived = false;
        gintDelayedSpeedTestReceive = 0;

        //Clear all text boxes
        ui->edit_SpeedPacketsBad->setText("0");
        ui->edit_SpeedPacketsRec->setText("0");
        ui->edit_SpeedPacketsRec10s->setText("0");
        ui->edit_SpeedPacketsSent->setText("0");
        ui->edit_SpeedPacketsSent10s->setText("0");
        ui->edit_SpeedPacketsErrorRate->setText("0");
        ui->edit_SpeedPacketsGood->setText("0");
        ui->edit_SpeedPacketsRecAvg->setText("0");
        ui->edit_SpeedPacketsSentAvg->setText("0");
        ui->edit_SpeedBytesRec->setText("0");
        ui->edit_SpeedBytesRec10s->setText("0");
        ui->edit_SpeedBytesRecAvg->setText("0");
        ui->edit_SpeedBytesSent->setText("0");
        ui->edit_SpeedBytesSent10s->setText("0");
        ui->edit_SpeedBytesSentAvg->setText("0");

        //Clear all labels
        ui->label_SpeedRx->setText("0");
        ui->label_SpeedTx->setText("0");
        ui->label_SpeedTime->setText("00:00:00:00");

        //Clear received buffer and data match buffer
        gbaSpeedMatchData.clear();
        gbaSpeedReceivedData.clear();

        //Check if this is a string match or throughput-only test
        if (ui->combo_SpeedDataType->currentIndex() != 0)
        {
            //Escape character codes if enabled
            if (ui->check_SpeedStringUnescape->isChecked())
            {
                //Escape
                gbaSpeedMatchData = ui->edit_SpeedTestData->text().toUtf8();
                UwxEscape::EscapeCharacters(&gbaSpeedMatchData);
            }
            else
            {
                //Normal
                gbaSpeedMatchData = ui->edit_SpeedTestData->text().toUtf8();
            }

            //Set length of match data
            gintSpeedTestMatchDataLength = gbaSpeedMatchData.length();
        }

        //By default, no send delay
        gintDelayedSpeedTestSend = 0;

        if (chItem == SpeedMenuActionRecv)
        {
            //Receive only test
            gchSpeedTestMode = SpeedModeRecv;
        }
        else if (chItem == SpeedMenuActionSend)
        {
            //Send only test
            gchSpeedTestMode = SpeedModeSend;

            //Send data
            SendSpeedTestData(SpeedTestChunkSize);
        }
        else if (chItem == SpeedMenuActionSendRecv || chItem == SpeedMenuActionSendRecv5Delay || chItem == SpeedMenuActionSendRecv10Delay || chItem == SpeedMenuActionSendRecv15Delay)
        {
            //Send and receive test
            gchSpeedTestMode = SpeedModeSendRecv;

            if (chItem == SpeedMenuActionSendRecv5Delay || chItem == SpeedMenuActionSendRecv10Delay || chItem == SpeedMenuActionSendRecv15Delay)
            {
                //Send after delay
                gintDelayedSpeedTestSend = (chItem == SpeedMenuActionSendRecv15Delay ? 15 : (chItem == SpeedMenuActionSendRecv10Delay ? 10 : 5));
                gtmrSpeedTestDelayTimer = new QTimer();
                gtmrSpeedTestDelayTimer->setSingleShot(true);
                connect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStartTimer()));
                gtmrSpeedTestDelayTimer->start(chItem == SpeedMenuActionSendRecv15Delay ? 15000 : (chItem == SpeedMenuActionSendRecv10Delay ? 10000 : 5000));
            }
            else
            {
                //Send immediately
                SendSpeedTestData(SpeedTestChunkSize);
            }
        }

        if (!ui->check_SpeedSyncReceive->isChecked())
        {
            //Do not synchronise the receive delay when the first data packet is received
            gbSpeedTestReceived = true;
        }

        //Show message in status bar
        ui->statusBar->showMessage(QString((gchSpeedTestMode == SpeedModeSendRecv ? "Send & Receive" : (gchSpeedTestMode == SpeedModeRecv ? "Receive only" : (gchSpeedTestMode == SpeedModeSend ? "Send only" : "Unknown")))).append(" Speed testing started."));

        //Start timers
        gtmrSpeedTestStats.start();
        gtmrSpeedTestStats10s.start();
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::OutputSpeedTestStats(
    )
{
    //Output speed test (10s) stats
    if (gintSpeedBytesSent > 0)
    {
        //Sending active
        if (ui->combo_SpeedDataDisplay->currentIndex() == 1)
        {
            //Data bits
            ui->edit_SpeedBytesSent10s->setText(QString::number(gintSpeedBytesSent10s*gintSpeedTestDataBits));
        }
        else if (ui->combo_SpeedDataDisplay->currentIndex() == 2)
        {
            //All bits
            ui->edit_SpeedBytesSent10s->setText(QString::number(gintSpeedBytesSent10s*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits)));
        }
        else
        {
            //Bytes
            ui->edit_SpeedBytesSent10s->setText(QString::number(gintSpeedBytesSent10s));
        }
        ui->edit_SpeedPacketsSent10s->setText(QString::number(gintSpeedBytesSent10s/gintSpeedTestMatchDataLength));
        gintSpeedBytesSent10s = 0;
    }
    if ((gchSpeedTestMode & SpeedModeRecv) == SpeedModeRecv)
    {
        //Receiving active
        if (ui->combo_SpeedDataDisplay->currentIndex() == 1)
        {
            //Data bits
            ui->edit_SpeedBytesRec10s->setText(QString::number(gintSpeedBytesReceived10s*gintSpeedTestDataBits));
        }
        else if (ui->combo_SpeedDataDisplay->currentIndex() == 2)
        {
            //All bits
            ui->edit_SpeedBytesRec10s->setText(QString::number(gintSpeedBytesReceived10s*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits)));
        }
        else
        {
            //Bytes
            ui->edit_SpeedBytesRec10s->setText(QString::number(gintSpeedBytesReceived10s));
        }
        if (ui->combo_SpeedDataType->currentIndex() != 0)
        {
            //Show stats about packets
            ui->edit_SpeedPacketsRec10s->setText(QString::number(gintSpeedBytesReceived10s/gintSpeedTestMatchDataLength));
        }
        gintSpeedBytesReceived10s = 0;
    }

    //Update average speed test statistics
    OutputSpeedTestAvgStats(gtmrSpeedTimer.nsecsElapsed()/1000000000LL);
}

//=============================================================================
//=============================================================================
void
MainWindow::on_combo_SpeedDataType_currentIndexChanged(
    int
    )
{
    //Speed test type changed
    uint8_t i = 0;
    if (ui->combo_SpeedDataType->currentIndex() == 0)
    {
        //Throughput only
        ui->edit_SpeedTestData->setEnabled(false);
        ui->edit_SpeedPacketsSent->setEnabled(false);
        ui->edit_SpeedPacketsSent10s->setEnabled(false);
        ui->edit_SpeedPacketsSentAvg->setEnabled(false);
        ui->edit_SpeedPacketsRec->setEnabled(false);
        ui->edit_SpeedPacketsRec10s->setEnabled(false);
        ui->edit_SpeedPacketsRecAvg->setEnabled(false);
        ui->edit_SpeedPacketsGood->setEnabled(false);
        ui->edit_SpeedPacketsBad->setEnabled(false);
        ui->edit_SpeedPacketsErrorRate->setEnabled(false);

        //Disable sending modes
        while (i < gpSpeedMenu->actions().length())
        {
            gpSpeedMenu->actions().at(i)->setEnabled(false);
            ++i;
        }
    }
    else if (ui->combo_SpeedDataType->currentIndex() == 1)
    {
        //String
        ui->edit_SpeedTestData->setEnabled(true);
        ui->edit_SpeedPacketsSent->setEnabled(true);
        ui->edit_SpeedPacketsSent10s->setEnabled(true);
        ui->edit_SpeedPacketsSentAvg->setEnabled(true);
        ui->edit_SpeedPacketsRec->setEnabled(true);
        ui->edit_SpeedPacketsRec10s->setEnabled(true);
        ui->edit_SpeedPacketsRecAvg->setEnabled(true);
        ui->edit_SpeedPacketsGood->setEnabled(true);
        ui->edit_SpeedPacketsBad->setEnabled(true);
        ui->edit_SpeedPacketsErrorRate->setEnabled(true);

        //Enable sending modes
        while (i < gpSpeedMenu->actions().length())
        {
            gpSpeedMenu->actions().at(i)->setEnabled(true);
            ++i;
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_SpeedCopy_clicked(
    )
{
    //Copies some data to the clipboard about the test
    QByteArray baTmpBA = ui->edit_SpeedTestData->text().toUtf8();
    QString strResultStr;

    if (ui->combo_SpeedDataDisplay->currentIndex() == 1)
    {
        //Data bits
        strResultStr = QString("\r\n    > Tx (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesSent->text().toUInt()/gintSpeedTestDataBits)).
        append("\r\n    > Tx (Data bits): ").
        append(ui->edit_SpeedBytesSent->text()).
        append("\r\n    > Tx (All bits): ").
        append(QString::number(ui->edit_SpeedBytesSent->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits)/gintSpeedTestDataBits)).
        append("\r\n    > Tx last 10s (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesSent10s->text().toUInt()/gintSpeedTestDataBits)).
        append("\r\n    > Tx last 10s (Data bits): ").
        append(ui->edit_SpeedBytesSent10s->text()).
        append("\r\n    > Tx last 10s (All bits): ").
        append(QString::number(ui->edit_SpeedBytesSent10s->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits)/gintSpeedTestDataBits)).
        append("\r\n    > Tx average (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesSentAvg->text().toUInt()/gintSpeedTestDataBits)).
        append("\r\n    > Tx average (Data bits): ").
        append(ui->edit_SpeedBytesSentAvg->text()).
        append("\r\n    > Tx average (All bits): ").
        append(QString::number(ui->edit_SpeedBytesSentAvg->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits)/gintSpeedTestDataBits)).
        append("\r\n    > Rx (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesRec->text().toUInt()/gintSpeedTestDataBits)).
        append("\r\n    > Rx (Data bits): ").
        append(ui->edit_SpeedBytesRec->text()).
        append("\r\n    > Rx (All bits): ").
        append(QString::number(ui->edit_SpeedBytesRec->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits)/gintSpeedTestDataBits)).
        append("\r\n    > Rx last 10s (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesRec10s->text().toUInt()/gintSpeedTestDataBits)).
        append("\r\n    > Rx last 10s (Data bits): ").
        append(ui->edit_SpeedBytesRec10s->text()).
        append("\r\n    > Rx last 10s (All bits): ").
        append(QString::number(ui->edit_SpeedBytesRec10s->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits)/gintSpeedTestDataBits)).
        append("\r\n    > Rx average (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesRecAvg->text().toUInt()/gintSpeedTestDataBits)).
        append("\r\n    > Rx average (Data bits): ").
        append(ui->edit_SpeedBytesRecAvg->text()).
        append("\r\n    > Rx average (All bits): ").
        append(QString::number(ui->edit_SpeedBytesRecAvg->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits)/gintSpeedTestDataBits));
    }
    else if (ui->combo_SpeedDataDisplay->currentIndex() == 2)
    {
        //All bits
        strResultStr = QString("\r\n    > Tx (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesSent->text().toUInt()/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Tx (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesSent->text().toUInt()*gintSpeedTestDataBits/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Tx (All bits): ").
        append(ui->edit_SpeedBytesSent->text()).
        append("\r\n    > Tx last 10s (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesSent10s->text().toUInt()/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Tx last 10s (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesSent10s->text().toUInt()*gintSpeedTestDataBits/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Tx last 10s (All bits): ").
        append(ui->edit_SpeedBytesSent10s->text()).
        append("\r\n    > Tx average (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesSentAvg->text().toUInt()/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Tx average (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesSentAvg->text().toUInt()*gintSpeedTestDataBits/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Tx average (All bits): ").
        append(ui->edit_SpeedBytesSentAvg->text()).
        append("\r\n    > Rx (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesRec->text().toUInt()/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Rx (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesRec->text().toUInt()*gintSpeedTestDataBits/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Rx (All bits): ").
        append(ui->edit_SpeedBytesRec->text()).
        append("\r\n    > Rx last 10s (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesRec10s->text().toUInt()/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Rx last 10s (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesRec10s->text().toUInt()*gintSpeedTestDataBits/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Rx last 10s (All bits): ").
        append(ui->edit_SpeedBytesRec10s->text()).
        append("\r\n    > Rx average (Bytes): ").
        append(QString::number(ui->edit_SpeedBytesRecAvg->text().toUInt()/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Rx average (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesRecAvg->text().toUInt()*gintSpeedTestDataBits/(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Rx average (All bits): ").
        append(ui->edit_SpeedBytesRecAvg->text());
    }
    else
    {
        //Bytes
        strResultStr = QString("\r\n    > Tx (Bytes): ").
        append(ui->edit_SpeedBytesSent->text()).
        append("\r\n    > Tx (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesSent->text().toUInt()*gintSpeedTestDataBits)).
        append("\r\n    > Tx (All bits): ").
        append(QString::number(ui->edit_SpeedBytesSent->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Tx last 10s (Bytes): ").
        append(ui->edit_SpeedBytesSent10s->text()).
        append("\r\n    > Tx last 10s (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesSent10s->text().toUInt()*gintSpeedTestDataBits)).
        append("\r\n    > Tx last 10s (All bits): ").
        append(QString::number(ui->edit_SpeedBytesSent10s->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Tx average (Bytes): ").
        append(ui->edit_SpeedBytesSentAvg->text()).
        append("\r\n    > Tx average (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesSentAvg->text().toUInt()*gintSpeedTestDataBits)).
        append("\r\n    > Tx average (All bits): ").
        append(QString::number(ui->edit_SpeedBytesSentAvg->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Rx (Bytes): ").
        append(ui->edit_SpeedBytesRec->text()).
        append("\r\n    > Rx (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesRec->text().toUInt()*gintSpeedTestDataBits)).
        append("\r\n    > Rx (All bits): ").
        append(QString::number(ui->edit_SpeedBytesRec->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Rx last 10s (Bytes): ").
        append(ui->edit_SpeedBytesRec10s->text()).
        append("\r\n    > Rx last 10s (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesRec10s->text().toUInt()*gintSpeedTestDataBits)).
        append("\r\n    > Rx last 10s (All bits): ").
        append(QString::number(ui->edit_SpeedBytesRec10s->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits))).
        append("\r\n    > Rx average (Bytes): ").
        append(ui->edit_SpeedBytesRecAvg->text()).
        append("\r\n    > Rx average (Data bits): ").
        append(QString::number(ui->edit_SpeedBytesRecAvg->text().toUInt()*gintSpeedTestDataBits)).
        append("\r\n    > Rx average (All bits): ").
        append(QString::number(ui->edit_SpeedBytesRecAvg->text().toUInt()*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits)));
    }
    UwxEscape::EscapeCharacters(&baTmpBA);
    QApplication::clipboard()->setText(QString("=================================\r\n  AuTerm ").
        append(UwVersion).append(" Speed Test\r\n       ").
        append(QDate::currentDate().toString("dd/MM/yyyy")).
        append(" @ ").append(QTime::currentTime().toString("hh:mm")).
        append("\r\n---------------------------------\r\nSettings:").
        append("\r\n    > Port: ").
        append(ui->label_SpeedConn->text()).
        append("\r\n    > Data Type: ").
        append(ui->combo_SpeedDataType->currentText()).
        append("\r\n    > Data String: ").
        append(ui->edit_SpeedTestData->text()).
        append("\r\n    > Data String Length: ").
        append(QString::number(ui->edit_SpeedTestData->text().length())).
        append("\r\n    > Data String Size (bytes): ").
        append(QString::number(ui->edit_SpeedTestData->text().toUtf8().size())).
        append("\r\n    > Unescaped Data String: ").
        append(baTmpBA.replace('\0', "\\00").replace("\x01", "\\01").replace("\x02", "\\02").replace("\x03", "\\03").replace("\x04", "\\04").replace("\x05", "\\05").replace("\x06", "\\06").replace("\x07", "\\07").replace("\x08", "\\08").replace("\x0b", "\\0B").replace("\x0c", "\\0C").replace("\x0e", "\\0E").replace("\x0f", "\\0F").replace("\x10", "\\10").replace("\x11", "\\11").replace("\x12", "\\12").replace("\x13", "\\13").replace("\x14", "\\14").replace("\x15", "\\15").replace("\x16", "\\16").replace("\x17", "\\17").replace("\x18", "\\18").replace("\x19", "\\19").replace("\x1a", "\\1a").replace("\x1b", "\\1b").replace("\x1c", "\\1c").replace("\x1d", "\\1d").replace("\x1e", "\\1e").replace("\x1f", "\\1f")).
        append("\r\n    > Unescaped Data String Length: ").
        append(QString::number(QString(baTmpBA.replace('\0', "0")).length())).
        append("\r\n    > Unescaped Data String Size (bytes): ").
        append(QString::number(baTmpBA.size())).
        append("\r\n    > Unescape: ").
        append((ui->check_SpeedStringUnescape->isChecked() ? "Yes" : "No")).
        append("\r\n    > Send Delay: ").
        append(QString::number(gintDelayedSpeedTestSend)).
        append("\r\n    > Synchronise receive timer: ").
        append((ui->check_SpeedSyncReceive->isChecked() ? "Yes" : "No")).
        append("\r\n    > Receive Delay: ").
        append(QString::number(gintDelayedSpeedTestReceive)).
        append("\r\n    > Test Type: ").
        append((gchSpeedTestMode == SpeedModeSendRecv ? "Send/Receive" : (gchSpeedTestMode == SpeedModeSend ? "Send" : (gchSpeedTestMode == SpeedModeRecv ? "Receive" : "Inactive")))).
        append("\r\n---------------------------------\r\nResults:\r\n    > Test time: ").
        append(ui->label_SpeedTime->text()).
        append(strResultStr).
        append("\r\n    > Tx (Packets): ").
        append(ui->edit_SpeedPacketsSent->text()).
        append("\r\n    > Tx last 10s (Packets): ").
        append(ui->edit_SpeedPacketsSent10s->text()).
        append("\r\n    > Tx average (Packets): ").
        append(ui->edit_SpeedPacketsSentAvg->text()).
        append("\r\n    > Rx (Packets): ").
        append(ui->edit_SpeedPacketsRec->text()).
        append("\r\n    > Rx last 10s (Packets): ").
        append(ui->edit_SpeedPacketsRec10s->text()).
        append("\r\n    > Rx average (Packets): ").
        append(ui->edit_SpeedPacketsRecAvg->text()).
        append("\r\n    > Rx Good (Packets): ").
        append(ui->edit_SpeedPacketsGood->text()).
        append("\r\n    > Rx Bad (Packets): ").
        append(ui->edit_SpeedPacketsBad->text()).
        append("\r\n    > Rx Error Rate % (Packets): ").
        append(ui->edit_SpeedPacketsErrorRate->text()).
        append("\r\n=================================\r\n"));
}

//=============================================================================
//=============================================================================
void
MainWindow::SendSpeedTestData(
    int intMaxLength
    )
{
    //Send string out. It's OK to send less than the maximum length but not more
    int intSendTimes = (intMaxLength / gintSpeedTestMatchDataLength);
    if (ui->check_SpeedShowTX->isChecked())
    {
        //Show TX data in terminal
        while (intSendTimes > 0)
        {
            //Append to buffer
            gbaSpeedDisplayBuffer.append(gbaSpeedMatchData);
            --intSendTimes;
        }
        if (!gtmrSpeedUpdateTimer.isActive())
        {
            gtmrSpeedUpdateTimer.start();
        }

        //Reset variable for actual sending
        intSendTimes = (intMaxLength / gintSpeedTestMatchDataLength);
    }

    while (intSendTimes > 0)
    {
        //Send out until finished
        gspSerialPort.write(gbaSpeedMatchData);
        gintSpeedBufferCount += gintSpeedTestMatchDataLength;
        --intSendTimes;
        ++gintSpeedTestStatPacketsSent;
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::SpeedTestBytesWritten(
    qint64 intByteCount
    )
{
    //Serial port bytes have been written in speed test mode
    if ((gchSpeedTestMode & SpeedModeSend) == SpeedModeSend)
    {
        //Sending data in speed test
        gintSpeedBufferCount -= intByteCount;
        if (gintSpeedBufferCount <= SpeedTestMinBufSize)
        {
            //Buffer has space: send more data
            SendSpeedTestData(SpeedTestChunkSize);
        }
    }

    //Add to bytes sent counters
    gintSpeedBytesSent += intByteCount;
    gintSpeedBytesSent10s += intByteCount;
}

//=============================================================================
//=============================================================================
void
MainWindow::SpeedTestReceive(
    )
{
    //Receieved data from serial port in speed test mode
    if ((gchSpeedTestMode & SpeedModeRecv) == SpeedModeRecv)
    {
        //Check data as in receieve mode
        uint64_t received_bytes = gspSerialPort.bytesAvailable();
        gintSpeedBytesReceived += received_bytes;
        gintSpeedBytesReceived10s += received_bytes;

        if (ui->check_SpeedSyncReceive->isChecked() && gbSpeedTestReceived == false)
        {
            //Data has now been received, update the delay timer
            gbSpeedTestReceived = true;
            gintDelayedSpeedTestReceive = gtmrSpeedTimer.nsecsElapsed()/1000000000LL;
        }

        if (ui->check_SpeedShowRX->isChecked() == true)
        {
            //Append RX data to buffer
            gbaSpeedDisplayBuffer.append(gspSerialPort.peek(received_bytes));
            if (!gtmrSpeedUpdateTimer.isActive())
            {
                gtmrSpeedUpdateTimer.start();
            }
        }

        if (ui->combo_SpeedDataType->currentIndex() != 0)
        {
            //Test data is OK
            uint32_t remove_size = 0;
            gbaSpeedReceivedData.append(gspSerialPort.read(received_bytes));
            while (remove_size < gbaSpeedReceivedData.length())
            {
                //Data to check
                int SizeToTest = gintSpeedTestMatchDataLength - gintSpeedTestReceiveIndex;
                if ((SizeToTest + remove_size) > gbaSpeedReceivedData.length())
                {
                    SizeToTest = gbaSpeedReceivedData.length() - remove_size;
                }

                //Optimised search check function. for testing only
                uint32_t i = 0;
                bool good = true;
                pointer_buf rec_buf;
                pointer_buf match_buf;
                rec_buf.p8 = ((uint8_t *)gbaSpeedReceivedData.data() + remove_size);
                match_buf.p8 = ((uint8_t *)gbaSpeedMatchData.data() + gintSpeedTestReceiveIndex);

                while (i < SizeToTest)
                {
                    uint32_t loop_check_size = 8;
                    switch (SizeToTest - i)
                    {
                    case 1:
                        if (*rec_buf.p8 != *match_buf.p8)
                        {
                                good = false;
                        }
                        loop_check_size = 1;
                        break;
                    case 2:
                        if (*rec_buf.p16 != *match_buf.p16)
                        {
                                good = false;
                        }
                        loop_check_size = 2;
                        break;
                    case 3:
                        if (*rec_buf.p16 != *match_buf.p16 || *(rec_buf.p8+2) != *(match_buf.p8+2))
                        {
                                good = false;
                        }
                        loop_check_size = 3;
                        break;
                    case 4:
                        if (*rec_buf.p32 != *match_buf.p32)
                        {
                                good = false;
                        }
                        loop_check_size = 4;
                        break;
                    case 5:
                        if (*rec_buf.p32 != *match_buf.p32 || *(rec_buf.p8 + 4) != *(match_buf.p8 + 4))
                        {
                                good = false;
                        }
                        loop_check_size = 5;
                        break;
                    case 6:
                        if (*rec_buf.p32 != *match_buf.p32 || *(rec_buf.p16 + 2) != *(match_buf.p16 + 2))
                        {
                                good = false;
                        }
                        loop_check_size = 6;
                        break;
                    case 7:
                        if (*rec_buf.p32 != *match_buf.p32 || *(rec_buf.p16 + 2) != *(match_buf.p16 + 2) || *(rec_buf.p8 + 6) != *(match_buf.p8 + 6))
                        {
                                good = false;
                        }
                        loop_check_size = 7;
                        break;
                    default:
                        if (*rec_buf.p64 != *match_buf.p64)
                        {
                                good = false;
                        }
                        break;
                    }

                    if (good == false)
                    {
                        break;
                    }

                    i += loop_check_size;
                    rec_buf.p8 += loop_check_size;
                    match_buf.p8 += loop_check_size;
                }

                if (good == true)
                {
                    //Good
                    remove_size += SizeToTest;
                    gintSpeedTestReceiveIndex += SizeToTest;
                    if (gintSpeedTestReceiveIndex >= gintSpeedTestMatchDataLength)
                    {
                        ++gintSpeedTestStatSuccess;
                        ++gintSpeedTestStatPacketsReceived;
                        gintSpeedTestReceiveIndex = 0;
                    }
                }
                else
                {
                    //Bad
                    ++gintSpeedTestStatErrors;

                    if (ui->check_SpeedShowErrors->isChecked())
                    {
                        //Show error - find mismatch position
                        uint16_t new_offset = (i > 5 ? i - 5 : 0);
                        QString strFirst(gbaSpeedReceivedData.mid(remove_size + new_offset));
                        QString strSecond(gbaSpeedMatchData.mid(gintSpeedTestReceiveIndex + new_offset));
                        uint32_t max_size = strFirst.length() > strSecond.length() ? strSecond.length() : strFirst.length();
                        quint16 iOffset = 0;
                        while (iOffset < max_size)
                        {
                                if (strFirst.at(iOffset) != strSecond.at(iOffset))
                                {
                                        //Found
                                        ++iOffset;
                                        break;
                                }
                                ++iOffset;
                        }

                        if (strFirst.length() > max_size)
                        {
                                strFirst.remove(max_size, strFirst.length() - max_size);
                        }

                        if (strSecond.length() > max_size)
                        {
                                strSecond.remove(max_size, strSecond.length() - max_size);
                        }

                        //Add to display
                        gbaSpeedDisplayBuffer.append(QString("\r\nError: Data mismatch.\r\n\tExpected: ").append(strSecond).append("\r\n\tGot     : ").append(strFirst).append("\r\n\tPosition: ").append(QString("-").repeated(iOffset-1).append("^")).append("\r\n\tOccurred: ").append(ui->label_SpeedTime->text()).append(" (").append(QDateTime::currentDateTime().toLocalTime().toString()).append(")\r\n").toUtf8());
                        if (!gtmrSpeedUpdateTimer.isActive())
                        {
                                gtmrSpeedUpdateTimer.start();
                        }
                    }

                    //Search for start character (ignoring first character)
                    int StartChar = gbaSpeedReceivedData.indexOf(gbaSpeedMatchData.at(0), remove_size + 1);
                    if (StartChar == -1)
                    {
                        //Not found, clear whole receive buffer
                        gbaSpeedReceivedData.clear();
                    }
                    else
                    {
                        //Found, remove until this character
                        gbaSpeedReceivedData.remove(0, StartChar);
                    }

                    ++gintSpeedTestStatPacketsReceived;
                    gintSpeedTestReceiveIndex = 0;
                    remove_size = 0;
                }
            }

            if (remove_size > 0)
            {
                gbaSpeedReceivedData.remove(0, remove_size);
            }
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::UpdateSpeedTestValues(
    )
{
    //Update speed test statistics
    qint64 lngElapsed = gtmrSpeedTimer.nsecsElapsed()/1000000LL;
    unsigned int intHours = (lngElapsed / 3600000LL);
    unsigned char chMinutes = (lngElapsed / 60000LL) % 60;
    unsigned char chSeconds = (lngElapsed % 60000) / 1000;
    unsigned int intMiliseconds = (lngElapsed % 600) / 10;
    ui->label_SpeedTime->setText(QString((intHours < 10 ? "0" : "")).append(QString::number(intHours)).append((chMinutes < 10 ? ":0" : ":")).append(QString::number(chMinutes)).append((chSeconds < 10 ? ":0" : ":")).append(QString::number(chSeconds)).append((intMiliseconds < 10 ? ".0" : ".")).append(QString::number(intMiliseconds)));

    if (gintSpeedBytesSent > 0)
    {
        //Update sent data
        ui->label_SpeedTx->setText(QString::number(gintSpeedBytesSent));
        if (ui->combo_SpeedDataDisplay->currentIndex() == 1)
        {
            //Data bits
            ui->edit_SpeedBytesSent->setText(QString::number(gintSpeedBytesSent*gintSpeedTestDataBits));
        }
        else if (ui->combo_SpeedDataDisplay->currentIndex() == 2)
        {
            //All bits
            ui->edit_SpeedBytesSent->setText(QString::number(gintSpeedBytesSent*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits)));
        }
        else
        {
            //Bytes
            ui->edit_SpeedBytesSent->setText(QString::number(gintSpeedBytesSent));
        }
        ui->edit_SpeedPacketsSent->setText(QString::number(gintSpeedTestStatPacketsSent));
    }

    if ((gchSpeedTestMode & SpeedModeRecv) == SpeedModeRecv)
    {
        //Receive mode active
        ui->label_SpeedRx->setText(QString::number(gintSpeedBytesReceived));
        if (ui->combo_SpeedDataDisplay->currentIndex() == 1)
        {
            //Data bits
            ui->edit_SpeedBytesRec->setText(QString::number(gintSpeedBytesReceived*gintSpeedTestDataBits));
        }
        else if (ui->combo_SpeedDataDisplay->currentIndex() == 2)
        {
            //All bits
            ui->edit_SpeedBytesRec->setText(QString::number(gintSpeedBytesReceived*(gintSpeedTestDataBits + gintSpeedTestStartStopParityBits)));
        }
        else
        {
            //Bytes
            ui->edit_SpeedBytesRec->setText(QString::number(gintSpeedBytesReceived));
        }
        ui->edit_SpeedPacketsRec->setText(QString::number(gintSpeedTestStatPacketsReceived));
        ui->edit_SpeedPacketsBad->setText(QString::number(gintSpeedTestStatErrors));
        ui->edit_SpeedPacketsGood->setText(QString::number(gintSpeedTestStatSuccess));
        if (gintSpeedTestStatErrors > 0)
        {
            //Calculate error rate (up to 2 decimal places and rounding up)
            ui->edit_SpeedPacketsErrorRate->setText(QString::number(std::ceil((float)gintSpeedTestStatErrors*10000.0/(float)(gintSpeedTestStatSuccess+gintSpeedTestStatErrors))/100.0));
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::SpeedTestStartTimer(
    )
{
    //Timer expired, begin sending speed test data
    disconnect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStartTimer()));
    delete gtmrSpeedTestDelayTimer;
    gtmrSpeedTestDelayTimer = 0;
    SendSpeedTestData(SpeedTestChunkSize);
}

//=============================================================================
//=============================================================================
void
MainWindow::SpeedTestStopTimer(
    )
{
    //Timer expired, stop receiving speed test data
    disconnect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStopTimer()));
    delete gtmrSpeedTestDelayTimer;
    gtmrSpeedTestDelayTimer = 0;
    ui->btn_SpeedStartStop->setText(tr("&Start Test"));
    ui->check_SpeedSyncReceive->setEnabled(true);
    ui->combo_SpeedDataType->setEnabled(true);
    if (ui->combo_SpeedDataType->currentIndex() == 1)
    {
        //Enable string options
        ui->edit_SpeedTestData->setEnabled(true);
        ui->check_SpeedStringUnescape->setEnabled(true);
    }

    //Update values
    OutputSpeedTestAvgStats(gtmrSpeedTimer.nsecsElapsed()/1000000000LL);

    //Set speed test as no longer running
    gchSpeedTestMode = SpeedModeInactive;
    gbSpeedTestRunning = false;

    if (gtmrSpeedTimer.isValid())
    {
        //Invalidate speed test timer
        gtmrSpeedTimer.invalidate();
    }
    if (gtmrSpeedTestStats.isActive())
    {
        //Stop stats update timer
        gtmrSpeedTestStats.stop();
    }
    if (gtmrSpeedTestStats10s.isActive())
    {
        //Stop 10 second stats update timer
        gtmrSpeedTestStats10s.stop();
    }

    //Clear buffers
    gbaSpeedMatchData.clear();
    gbaSpeedReceivedData.clear();

    //Show finished message in status bar
    ui->statusBar->showMessage("Speed testing finished.");
}

//=============================================================================
//=============================================================================
void
MainWindow::OutputSpeedTestAvgStats(
    qint64 lngElapsed
    )
{
    //Update average statistics
    if (gintSpeedBytesSent > 0)
    {
        //Sending has activity
        if (ui->combo_SpeedDataDisplay->currentIndex() == 1)
        {
            //Data bits
            ui->edit_SpeedBytesSentAvg->setText(QString::number((quint64)gintSpeedBytesSent*(quint64)gintSpeedTestDataBits/((quint64)lngElapsed-(quint64)gintDelayedSpeedTestSend)));
        }
        else if (ui->combo_SpeedDataDisplay->currentIndex() == 2)
        {
            //All bits
            ui->edit_SpeedBytesSentAvg->setText(QString::number((quint64)gintSpeedBytesSent*((quint64)gintSpeedTestDataBits + (quint64)gintSpeedTestStartStopParityBits)/((quint64)lngElapsed-(quint64)gintDelayedSpeedTestSend)));
        }
        else
        {
            //Bytes
            ui->edit_SpeedBytesSentAvg->setText(QString::number((quint64)gintSpeedBytesSent/((quint64)lngElapsed-(quint64)gintDelayedSpeedTestSend)));
        }
        ui->edit_SpeedPacketsSentAvg->setText(QString::number((quint64)gintSpeedBytesSent/(quint64)gintSpeedTestMatchDataLength/((quint64)lngElapsed-(quint64)gintDelayedSpeedTestSend)));
    }

    if ((gchSpeedTestMode & SpeedModeRecv) == SpeedModeRecv)
    {
        //Receiving active
        if (ui->combo_SpeedDataDisplay->currentIndex() == 1)
        {
            //Data bits
            ui->edit_SpeedBytesRecAvg->setText(QString::number((quint64)gintSpeedBytesReceived*(quint64)gintSpeedTestDataBits/((quint64)lngElapsed-(quint64)gintDelayedSpeedTestReceive)));
        }
        else if (ui->combo_SpeedDataDisplay->currentIndex() == 2)
        {
            //All bits
            ui->edit_SpeedBytesRecAvg->setText(QString::number((quint64)gintSpeedBytesReceived*((quint64)gintSpeedTestDataBits + (quint64)gintSpeedTestStartStopParityBits)/((quint64)lngElapsed-(quint64)gintDelayedSpeedTestReceive)));
        }
        else
        {
            //Bytes
            ui->edit_SpeedBytesRecAvg->setText(QString::number((quint64)gintSpeedBytesReceived/((quint64)lngElapsed-(quint64)gintDelayedSpeedTestReceive)));
        }
        if (ui->combo_SpeedDataType->currentIndex() != 0)
        {
            //Show stats about packets
            ui->edit_SpeedPacketsRecAvg->setText(QString::number((quint64)gintSpeedBytesReceived/(quint64)gintSpeedTestMatchDataLength/((quint64)lngElapsed-(quint64)gintDelayedSpeedTestReceive)));
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::UpdateDisplayText(
    )
{
    //Updates the speed display with data from the buffer
    unsigned int uiAnchor = 0;
    unsigned int uiPosition = 0;

    //Slider not held down, update
    unsigned int Pos;
    if (ui->text_SpeedEditData->verticalScrollBar()->sliderPosition() == ui->text_SpeedEditData->verticalScrollBar()->maximum())
    {
        //Scroll to bottom
        Pos = 65535;
    }
    else
    {
        //Stay here
        Pos = ui->text_SpeedEditData->verticalScrollBar()->sliderPosition();
    }

    //Append data
    ui->text_SpeedEditData->moveCursor(QTextCursor::End);
    ui->text_SpeedEditData->insertPlainText(gbaSpeedDisplayBuffer);

    //Go back to previous position
    if (Pos == 65535)
    {
        //Bottom
        ui->text_SpeedEditData->verticalScrollBar()->setValue(ui->text_SpeedEditData->verticalScrollBar()->maximum());
        if (uiAnchor == 0 && uiPosition == 0)
        {
            ui->text_SpeedEditData->moveCursor(QTextCursor::End);
        }
    }
    else
    {
        //Maintain
        ui->text_SpeedEditData->verticalScrollBar()->setValue(Pos);
    }

    //Clear the buffer
    gbaSpeedDisplayBuffer.clear();
}

//=============================================================================
//=============================================================================
void
MainWindow::on_combo_SpeedDataDisplay_currentIndexChanged(
    int
    )
{
    //Change speed test display to bits or bytes
    if (ui->edit_SpeedBytesSent->text().length() > 0 || ui->edit_SpeedBytesRec->text().length() > 0)
    {
        //Change type
        BitByteTypes bbtFrom = (gintSpeedTestBytesBits == 1 ? BitByteTypes::TypeDataBits : (gintSpeedTestBytesBits == 2 ? BitByteTypes::TypeAllBits : BitByteTypes::TypeBytes));
        BitByteTypes bbtTo = (ui->combo_SpeedDataDisplay->currentIndex() == 1 ? BitByteTypes::TypeDataBits : (ui->combo_SpeedDataDisplay->currentIndex() == 2 ? BitByteTypes::TypeAllBits : BitByteTypes::TypeBytes));

        //Convert the data
        ui->edit_SpeedBytesSent->setText(QString::number(BitsBytesConvert(ui->edit_SpeedBytesSent->text().toULongLong(), bbtFrom, bbtTo)));
        ui->edit_SpeedBytesRec->setText(QString::number(BitsBytesConvert(ui->edit_SpeedBytesRec->text().toULongLong(), bbtFrom, bbtTo)));
        ui->edit_SpeedBytesSent10s->setText(QString::number(BitsBytesConvert(ui->edit_SpeedBytesSent10s->text().toULongLong(), bbtFrom, bbtTo)));
        ui->edit_SpeedBytesRec10s->setText(QString::number(BitsBytesConvert(ui->edit_SpeedBytesRec10s->text().toULongLong(), bbtFrom, bbtTo)));
        ui->edit_SpeedBytesSentAvg->setText(QString::number(BitsBytesConvert(ui->edit_SpeedBytesSentAvg->text().toULongLong(), bbtFrom, bbtTo)));
        ui->edit_SpeedBytesRecAvg->setText(QString::number(BitsBytesConvert(ui->edit_SpeedBytesRecAvg->text().toULongLong(), bbtFrom, bbtTo)));
    }

    if (ui->combo_SpeedDataDisplay->currentIndex() == 0)
    {
        //Display in bytes
        ui->group_SpeedBytesBits->setTitle("Bytes");
    }
    else if (ui->combo_SpeedDataDisplay->currentIndex() == 1)
    {
        //Display in bits (data only)
        ui->group_SpeedBytesBits->setTitle("Data Bits");
    }
    else
    {
        //Display in bits (including start/stop bits)
        ui->group_SpeedBytesBits->setTitle("All Bits (Data, Start, Stop and Parity bits)");
    }
    gintSpeedTestBytesBits = ui->combo_SpeedDataDisplay->currentIndex();
}

//=============================================================================
//=============================================================================
quint64
MainWindow::BitsBytesConvert(
    quint64 iCount,
    BitByteTypes bbtFrom,
    BitByteTypes bbtTo
    )
{
    //Convert the value to all bits
    quint64 iTemp = iCount;
    if (bbtFrom == BitByteTypes::TypeBytes)
    {
        //Convert from bytes
        iTemp = iCount * (gintSpeedTestDataBits + gintSpeedTestStartStopParityBits);
    }
    else if (bbtFrom == BitByteTypes::TypeDataBits)
    {
        //Convert from data bits
        iTemp = iCount * (gintSpeedTestDataBits + gintSpeedTestStartStopParityBits) / gintSpeedTestDataBits;
    }

    //Convert the value to the required type
    if (bbtTo == BitByteTypes::TypeBytes)
    {
        //Convert to bytes
        iTemp = iTemp / (gintSpeedTestDataBits + gintSpeedTestStartStopParityBits);
    }
    else if (bbtTo == BitByteTypes::TypeDataBits)
    {
        //Convert to data bits
        iTemp = iTemp * gintSpeedTestDataBits / (gintSpeedTestDataBits + gintSpeedTestStartStopParityBits);
    }

    //Return the value
    return iTemp;
}
#endif

//=============================================================================
//=============================================================================
void
MainWindow::SetLoopBackMode(
    bool bNewMode
    )
{
    //Enables or disables loopback mode
    if (gbLoopbackMode != bNewMode)
    {
        //Change loopback mode
        gbLoopbackMode = bNewMode;
        if (gbLoopbackMode == true)
        {
            //Enabled
            gbaDisplayBuffer.append("\n[Loopback Enabled]\n");
            gpMenu->actions().at(MenuActionLoopback)->setText("Disable Loopback (Rx->Tx)");
        }
        else
        {
            //Disabled
            gbaDisplayBuffer.append("\n[Loopback Disabled]\n");
            gpMenu->actions().at(MenuActionLoopback)->setText("Enable Loopback (Rx->Tx)");
        }
        if (!gtmrTextUpdateTimer.isActive())
        {
            gtmrTextUpdateTimer.start();
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::AuTermUpdateCheck(
    bool bShowError
    )
{
    //Send request to check for AuTerm updates
#if 0
    if (LookupDNSName(bShowError) == true)
    {
        gbTermBusy = true;
        gchTermMode = MODE_CHECK_AuTerm_VERSIONS;
        gchTermMode2 = MODE_CHECK_AuTerm_VERSIONS;
        ui->btn_Cancel->setEnabled(true);
        ui->btn_ErrorCodeUpdate->setEnabled(false);
        ui->btn_ErrorCodeDownload->setEnabled(false);
        ui->btn_AuTermUpdate->setEnabled(false);
        ui->btn_ModuleFirmware->setEnabled(false);
        ui->btn_OnlineXComp_Supported->setEnabled(false);
        gnmrReply = gnmManager->get(QNetworkRequest(QUrl(QString(WebProtocol).append("://").append(WEB_HOST_NAME).append("/update_AuTerm.php?Ver=").append(UwVersion).append("&OS=").append(
#ifdef _WIN32
    //Windows
    #ifdef _WIN64
        //Windows 64-bit
        "W64"
        #define OS "Windows (x86_64)"
    #else
        //Windows 32-bit
        "W32"
    #endif
#elif __APPLE__
        //OSX
        "OSX"
#else
    //Assume Linux
    #ifdef __aarch64__
        //ARM64
        "LxARM64"
    #elif __arm__
        //ARM
        "LxARM"
    #elif __x86_64__
        //x86_64
        "Lx86_64"
    #elif __i386
        //x86
        "Lx86"
    #else
        //Unknown
        "LxOth"
    #endif
#endif
        ))));
        ui->statusBar->showMessage("Checking for AuTerm updates...");
    }
#endif
}

//=============================================================================
//=============================================================================
void
MainWindow::ScriptingFileSelected(
    const QString *strFilepath
    )
{
    QString strDirectory = SplitFilePath(*strFilepath).at(0);
    if (gpTermSettings->value("LastScriptFileDirectory").toString() != strDirectory)
    {
        //Update scripting directory
        gpTermSettings->setValue("LastScriptFileDirectory", strDirectory);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::UpdateCustomisation(
    bool bDefault
    )
{
    if (bDefault == true)
    {
        gpTermSettings->remove("CustomFont");
        gpTermSettings->remove("CustomPalette");
    }
    else
    {
        gpTermSettings->setValue("CustomFont", ui->text_TermEditData->font());
        gpTermSettings->setValue("CustomPalette", ui->text_TermEditData->palette());
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_check_EnableTerminalSizeSaving_stateChanged(
    int
    )
{
    if (gbAppStarted == true)
    {
        gpTermSettings->setValue("SaveSize", ui->check_EnableTerminalSizeSaving->isChecked());

        if (ui->check_EnableTerminalSizeSaving->isChecked() == true)
        {
            gpTermSettings->setValue("WindowWidth", this->width());
            gpTermSettings->setValue("WindowHeight", this->height());
        }
        else
        {
            gpTermSettings->remove("WindowWidth");
            gpTermSettings->remove("WindowHeight");
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::resizeEvent(
    QResizeEvent *
    )
{
    if (ui->check_EnableTerminalSizeSaving->isChecked() == true && gbAppStarted == true)
    {
        gpTermSettings->setValue("WindowWidth", this->width());
        gpTermSettings->setValue("WindowHeight", this->height());
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_edit_Title_textEdited(
    const QString &
    )
{
    QString strWindowTitle = QString("AuTerm (v").append(UwVersion).append(")");
    if (ui->edit_Title->text().length() > 0)
    {
        //Append custom text to window title
        strWindowTitle.append(" ").append(ui->edit_Title->text());
    }
    setWindowTitle(strWindowTitle);

    if (gpTermSettings->value("SysTrayIcon", DefaultSysTrayIcon).toBool() == true && QSystemTrayIcon::isSystemTrayAvailable())
    {
        //Also update system tray icon text
        if (gspSerialPort.isOpen())
        {
            strWindowTitle = QString("AuTerm v").append(UwVersion).append(" (").append(ui->combo_COM->currentText()).append(")");
        }
        else
        {
            strWindowTitle = QString("AuTerm v").append(UwVersion);
        }

        if (ui->edit_Title->text().length() > 0)
        {
            //Append custom text to window title
            strWindowTitle.append(": ").append(ui->edit_Title->text());
        }

        gpSysTray->setToolTip(strWindowTitle);
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/

void MainWindow::on_btn_Plugin_Abort_clicked()
{
//    qDebug() << plugin_loader.isLoaded();
//    qDebug() << button->filters();

//todo: support plugin list
    gpmErrorForm->show_message(plugin_list.at(0).plugin->plugin_about());
//    gpmErrorForm->show();

//    button->plugin_about();
}

void MainWindow::on_btn_Plugin_Config_clicked()
{
//todo: support plgin list
    if (plugin_list.at(0).plugin->plugin_configuration() == false)
    {
        gpmErrorForm->show_message("This plugin does not have any configuration.");
    }

//    emit serial_status(true);
}

void MainWindow::plugin_set_status(bool busy, bool hide_terminal_output)
{
    if (busy == false)
    {
        gbPluginRunning = false;
        gbPluginHideTerminalOutput = false;
    }
    else
    {
            gbPluginRunning = busy;
    gbPluginHideTerminalOutput = hide_terminal_output;
    }
//    qDebug() << "Now: " << busy;
}

void MainWindow::plugin_serial_transmit(QByteArray *data)
{
//    qDebug() << "Transmitted";
    if (gbPluginRunning == true)
    {
    gspSerialPort.write(*data);
    gintQueuedTXBytes += data->size();

           //Add to log
    gpMainLog->WriteLogData(*data);

    if (gbPluginHideTerminalOutput == false && ui->check_Echo->isChecked())
    {
            ui->text_TermEditData->AddDatInText(data);
    }
    }
}
