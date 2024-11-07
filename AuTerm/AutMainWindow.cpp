/******************************************************************************
** Copyright (C) 2015-2022 Laird Connectivity
** Copyright (C) 2023-2024 Jamie M.
**
** Project: AuTerm
**
** Module: AutMainWindow.cpp
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
#include "AutMainWindow.h"
#include "ui_AutMainWindow.h"
#include <QDebug>

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

#ifdef SKIPPLUGINS_TRANSPORT
#define transport_open gspSerialPort.open
#define transport_close gspSerialPort.close
#define transport_isOpen gspSerialPort.isOpen
#define transport_dataBits gspSerialPort.dataBits
#define transport_stopBits gspSerialPort.stopBits
#define transport_parity gspSerialPort.parity
#define transport_write gspSerialPort.write
#define transport_bytesAvailable gspSerialPort.bytesAvailable
#define transport_peek gspSerialPort.peek
#define transport_read gspSerialPort.read
#define transport_readAll gspSerialPort.readAll
#define transport_clear gspSerialPort.clear
#define transport_setBreakEnabled gspSerialPort.setBreakEnabled
#define transport_setRequestToSend gspSerialPort.setRequestToSend
#define transport_setDataTerminalReady gspSerialPort.setDataTerminalReady
#define transport_pinoutSignals gspSerialPort.pinoutSignals
//#define transport_error_to_error_string gspSerialPort.error_to_error_string
#define transport_name() "Serial port"
#define transport_supports_break() true
#define transport_supports_request_to_send() true
#define transport_supports_data_terminal_ready() true
#endif

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
AutMainWindow::AutMainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::AutMainWindow)
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
            if (static_plugins.at(i).metaData().value("MetaData").toObject().contains("Name") == true && static_plugins.at(i).metaData().value("MetaData").toObject().contains("Version") == true && static_plugins.at(i).metaData().value("MetaData").toObject().contains("Type") == true)
            {
                if (plugin_type_supported(plugin_type((QStaticPlugin *)&static_plugins.at(i))) == true)
                {
                    plugin.object = static_plugins.at(i).instance();
                    plugin.plugin = qobject_cast<AutPlugin *>(plugin.object);

                    if (plugin.plugin)
                    {
                        plugin.plugin->setup(this);
                        plugin_list.append(plugin);

                        ui->list_Plugin_Plugins->addItem(QString(static_plugins.at(i).metaData().value("MetaData").toObject().value("Name").toString()).append(", version ").append(static_plugins.at(i).metaData().value("MetaData").toObject().value("Version").toString()));
#ifndef SKIPPLUGINS_TRANSPORT
                        if (plugin.plugin->plugin_type() == AutPlugin::Transport)
                        {
                            QObject *plugin_object;
                            AutTransportPlugin *plugin_transport;
                            QWidget *plugin_tab = new QWidget();

                            plugin_transport = (AutTransportPlugin *)plugin.plugin;
                            plugin_tab->setObjectName("tab_transport_plugin_" % QString::number(plugin_list.count() - 1));
                            ui->tab_transport->addTab(plugin_tab, plugin_transport->transport_name());
//                            qDebug() << "found transport plugin " << static_plugins.at(i).metaData().value("MetaData").toObject().value("Name").toString();
                            plugin_object = plugin_transport->plugin_object();

                            connect(plugin_object, SIGNAL(readyRead()), this, SLOT(SerialRead()));
                            //connect(plugin_object, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(SerialError(QSerialPort::SerialPortError)));
                            connect(plugin_object, SIGNAL(bytesWritten(qint64)), this, SLOT(SerialBytesWritten(qint64)));
                            connect(plugin_object, SIGNAL(aboutToClose()), this, SLOT(SerialPortClosing()));

                            plugin_transport->transport_setup(plugin_tab);
                        }
#endif
                    }
                }
                else
                {
                    qDebug() << "Unsupported plugin type: " << static_plugins.at(i).metaData().value("MetaData").toObject().value("Type").toString();
                }
            }
            else
            {
                qDebug() << "Not an AuTerm plugin";
            }
        }

        ++i;
    }
#else
    //For dynamic builds, external library plugins can be loaded
    QDir lib_dir(QApplication::applicationDirPath());
    lib_dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);

#ifdef _WIN32
    lib_dir.setNameFilters(QStringList() << "plugin_*.dll");
#elif defined(__APPLE__)
    // On MacOs, lib_dir will be <location>/Term.app/Contents/MacOS
    lib_dir.cdUp();
    lib_dir.cdUp();
    lib_dir.cd("Frameworks");
    lib_dir.setNameFilters(QStringList() << "plugin_*.dylib");

#else
    lib_dir.setNameFilters(QStringList() << "plugin_*.so");
#endif

    QStringList plugin_names = lib_dir.entryList();
    struct plugins plugin;
    while (i < plugin_names.length())
    {
        plugin.plugin_loader = new QPluginLoader(lib_dir.path().append("/").append(plugin_names.at(i)));

        if (plugin.plugin_loader->metaData().contains("IID") == true && plugin.plugin_loader->metaData().value("IID").toString() == AuTermPluginInterface_iid)
        {
            if (plugin.plugin_loader->metaData().value("MetaData").toObject().contains("Name") == true && plugin.plugin_loader->metaData().value("MetaData").toObject().contains("Version") == true && plugin.plugin_loader->metaData().value("MetaData").toObject().contains("Type") == true)
            {
                if (plugin_type_supported(plugin_type(plugin.plugin_loader)) == true)
                {
                    plugin.object = plugin.plugin_loader->instance();

                    if (plugin.plugin_loader->isLoaded())
                    {
                        plugin.filename = plugin_names.at(i);
                        plugin.plugin = qobject_cast<AutPlugin *>(plugin.object);

                        if (plugin.plugin)
                        {
                            plugin.plugin->setup(this);
                            plugin_list.append(plugin);

                            ui->list_Plugin_Plugins->addItem(QString(plugin.plugin_loader->metaData().value("MetaData").toObject().value("Name").toString()).append(", version ").append(plugin.plugin_loader->metaData().value("MetaData").toObject().value("Version").toString()));
#ifndef SKIPPLUGINS_TRANSPORT
                            if (plugin.plugin->plugin_type() == AutPlugin::Transport)
                            {
                                QObject *plugin_object;
                                AutTransportPlugin *plugin_transport;
                                QWidget *plugin_tab = new QWidget();

                                plugin_transport = (AutTransportPlugin *)plugin.plugin;
                                plugin_tab->setObjectName("tab_transport_plugin_" % QString::number(plugin_list.count() - 1));
                                ui->tab_transport->addTab(plugin_tab, plugin_transport->transport_name());
//                                qDebug() << "found transport plugin " << plugin.plugin_loader->metaData().value("MetaData").toObject().value("Name").toString();
                                plugin_object = plugin_transport->plugin_object();

                                connect(plugin_object, SIGNAL(readyRead()), this, SLOT(SerialRead()));
                                //connect(plugin_object, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(SerialError(QSerialPort::SerialPortError)));
                                connect(plugin_object, SIGNAL(bytesWritten(qint64)), this, SLOT(SerialBytesWritten(qint64)));
                                connect(plugin_object, SIGNAL(aboutToClose()), this, SLOT(SerialPortClosing()));

                                plugin_transport->transport_setup(plugin_tab);
                            }
#endif
                        }
                        else
                        {
                            plugin.plugin_loader->unload();
                            delete plugin.plugin_loader;
                        }
                    }
                    else
                    {
                        qDebug() << plugin.plugin_loader->errorString();
                        delete plugin.plugin_loader;
                    }
                }
                else
                {
                    qDebug() << "Unsupported plugin type: " << plugin.plugin_loader->metaData().value("MetaData").toObject().value("Type").toString();
                    delete plugin.plugin_loader;
                }
            }
            else
            {
                qDebug() << "Not an AuTerm plugin: " << lib_dir.path().append("/").append(plugin_names.at(i));
                delete plugin.plugin_loader;
            }
        }

        ++i;
    }
#endif
#endif

#ifdef SKIPSPEEDTEST
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

#if defined(SKIPPLUGINS)
    ui->list_Plugin_Plugins->deleteLater();
    ui->btn_Plugin_Config->deleteLater();
    ui->btn_Plugin_Abort->deleteLater();
    ui->tab_Plugins->deleteLater();
#elif defined(QT_STATIC)
    ui->label_plugins_non_static->deleteLater();
#endif

#ifdef TARGET_OS_MAC
    //On mac, get the directory of the bundle (which will be <location>/Term.app/Contents/MacOS) and go up to the folder with the file in
    QDir BundleDir(QCoreApplication::applicationDirPath());
    BundleDir.cdUp();
    BundleDir.cdUp();
    BundleDir.cdUp();
    gstrMacBundlePath = BundleDir.path().append("/");
    if (!QDir().exists(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)))
    {
        //Create AuTerm directory in application support
        QDir().mkdir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    }

    //Fix mac's resize
    resize(740, 400);

    //Disable viewing files externally for mac
    ui->btn_EditExternal->setEnabled(false);

    //Increase duplicate button size for mac
    ui->btn_Duplicate->setMinimumWidth(130);
#endif

    //Define default variable values
    gbTermBusy = false;
    gbStreamingFile = false;
    gintRXBytes = 0;
    gintTXBytes = 0;
    gintQueuedTXBytes = 0;
    gchTermMode = 0;
    gbMainLogEnabled = false;
    gbLoopbackMode = false;
    gbSysTrayEnabled = false;
    gbCTSStatus = 0;
    gbDCDStatus = 0;
    gbDSRStatus = 0;
    gbRIStatus = 0;
    gbEditFileModified = false;
    giEditFileType = -1;
    gbErrorsLoaded = false;
#ifndef SKIPONLINE
    gnmManager = new QNetworkAccessManager(this);
    connect(gnmManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
#ifndef QT_NO_SSL
    connect(gnmManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));
#endif
#endif
#ifndef SKIPSPEEDTEST
    gtmrSpeedTestDelayTimer = 0;
    gbSpeedTestRunning = false;
#endif

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
#ifndef SKIPPLUGINS
    gbPluginRunning = false;
    gbPluginHideTerminalOutput = false;
    plugin_status_owner = nullptr;
#endif
    gbAppStarted = false;
    display_update_pending = false;
#ifndef SKIPPLUGINS_TRANSPORT
    plugin_active_transport = nullptr;
#endif

#ifndef SKIPSPEEDTEST
    //Also reserve 64KB of RAM to reduce mallocs when speed testing
    gbaSpeedReceivedData.reserve(65536);
#endif

    //Load settings from configuration files
    LoadSettings();

    //Create logging handle
    gpMainLog = new AutLogger();

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
    gimUw16Image = QImage(":/images/AuTerm16.png");
#if 0
    gimUw32Image = QImage(":/images/AuTerm32.png");
#endif

    //Create pixmaps
    gpEmptyCirclePixmap = new QPixmap(QPixmap::fromImage(gimEmptyCircleImage));
    gpRedCirclePixmap = new QPixmap(QPixmap::fromImage(gimRedCircleImage));
    gpGreenCirclePixmap = new QPixmap(QPixmap::fromImage(gimGreenCircleImage));
    gpUw16Pixmap = new QPixmap(QPixmap::fromImage(gimUw16Image));
#if 0
    gpUw32Pixmap = new QPixmap(QPixmap::fromImage(gimUw32Image));
#endif

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

    //Set window icon
    QMainWindow::setWindowIcon(QIcon(*gpUw16Pixmap));

    //Enable custom context menu policy
    ui->text_TermEditData->setContextMenuPolicy(Qt::CustomContextMenu);

    //Connect quit signals
    connect(ui->btn_Quit, SIGNAL(clicked()), this, SLOT(close()));

    //Connect key-press signals
    connect(ui->text_TermEditData, SIGNAL(enter_pressed()), this, SLOT(enter_pressed()));
    connect(ui->text_TermEditData, SIGNAL(key_pressed(int,QChar)), this, SLOT(key_pressed(int,QChar)));
    connect(ui->text_TermEditData, SIGNAL(vt100_send(QByteArray)), this, SLOT(vt100_send(QByteArray)));

    //Initialise popup message
    gpmErrorForm = new PopupMessage(this);

    //Populate the list of devices
    RefreshSerialDevices();

#ifndef SKIPSPEEDTEST
    //Setup speed test mode timers
    gtmrSpeedTestStats.setInterval(SpeedTestStatUpdateTime);
    gtmrSpeedTestStats.setSingleShot(false);
    connect(&gtmrSpeedTestStats, SIGNAL(timeout()), this, SLOT(UpdateSpeedTestValues()));
    gtmrSpeedTestStats10s.setInterval(10000);
    gtmrSpeedTestStats10s.setSingleShot(false);
    connect(&gtmrSpeedTestStats10s, SIGNAL(timeout()), this, SLOT(OutputSpeedTestStats()));
#endif
    //Display version
    ui->statusBar->showMessage(QString("AuTerm version ").append(UwVersion).append(" (").append(OS).append("), Built ").append(__DATE__).append(" Using QT ").append(QT_VERSION_STR)
#ifndef SKIPONLINE
#ifndef QT_NO_SSL
#ifdef TARGET_OS_MAC
    .append(", ").append(QString(QSslSocket::sslLibraryBuildVersionString()).replace(",", ":"))
#else
    .append(", ").append(QString(QSslSocket::sslLibraryBuildVersionString()).left(QSslSocket::sslLibraryBuildVersionString().indexOf(" ", 9)))
#endif
#endif
#endif
#ifdef QT_DEBUG
    .append(" [DEBUG BUILD]")
#endif
    );
    setWindowTitle(QString("AuTerm (v").append(UwVersion).append(")"));

    //Create menu items
    gpMenu = new QMenu(this);
    gpMenu->addAction("Lookup Selected Error-Code")->setData(MenuActionError);
    gpMenu->addAction("Enable Loopback (Rx->Tx)")->setData(MenuActionLoopback);
    gpMenu->addAction("Stream File Out")->setData(MenuActionStreamFile);
    gpSMenu4 = gpMenu->addMenu("Customisation");
    gpSMenu4->addAction("Font")->setData(MenuActionFont);
    gpSMenu4->addAction("Text Colour")->setData(MenuActionTextColour);
    gpSMenu4->addAction("Background Colour")->setData(MenuActionBackground);
    gpSMenu4->addAction("Restore Defaults")->setData(MenuActionRestoreDefaults);
    gpMenu->addAction("Automation")->setData(MenuActionAutomation);
#ifdef SKIPAUTOMATIONFORM
    //Disable unimplemented automation option
    gpMenu->actions().last()->setEnabled(false);
#endif
    gpMenu->addAction("Scripting")->setData(MenuActionScripting);
#ifdef SKIPSCRIPTINGFORM
    //Disable unimplemented scripting option
    gpMenu->actions().last()->setEnabled(false);
#endif
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

#ifndef SKIPSPEEDTEST
    //Create speed test button items
    gpSpeedMenu = new QMenu(this);
    gpSpeedMenu->addAction("Receive-only test")->setData(SpeedMenuActionRecv);
    gpSpeedMenu->addAction("Send-only test")->setData(SpeedMenuActionSend);
    gpSpeedMenu->addAction("Send && receive test")->setData(SpeedMenuActionSendRecv);
    gpSpeedMenu->addAction("Send && receive test (delay 5 seconds)")->setData(SpeedMenuActionSendRecv5Delay);
    gpSpeedMenu->addAction("Send && receive test (delay 10 seconds)")->setData(SpeedMenuActionSendRecv10Delay);
    gpSpeedMenu->addAction("Send && receive test (delay 15 seconds)")->setData(SpeedMenuActionSendRecv15Delay);
#endif

    //Connect the menu actions
    connect(gpMenu, SIGNAL(triggered(QAction*)), this, SLOT(MenuSelected(QAction*)), Qt::AutoConnection);
    connect(gpMenu, SIGNAL(aboutToHide()), this, SLOT(ContextMenuClosed()), Qt::AutoConnection);
    connect(gpBalloonMenu, SIGNAL(triggered(QAction*)), this, SLOT(balloontriggered(QAction*)), Qt::AutoConnection);
#ifndef SKIPSPEEDTEST
    connect(gpSpeedMenu, SIGNAL(triggered(QAction*)), this, SLOT(SpeedMenuSelected(QAction*)), Qt::AutoConnection);
#endif

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

#ifndef SKIPSPEEDTEST
    //Set update speed display timer to be single shot only and connect to slot
    gtmrSpeedUpdateTimer.setSingleShot(true);
    gtmrSpeedUpdateTimer.setInterval(gpTermSettings->value("TextUpdateInterval", DefaultTextUpdateInterval).toInt());
    connect(&gtmrSpeedUpdateTimer, SIGNAL(timeout()), this, SLOT(update_displayText()));
#endif

    //Set logging options
    ui->edit_LogFile->setText(gpTermSettings->value("LogFile", QString(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).append("/").append(DefaultLogFileName)).toString());
    ui->check_LogEnable->setChecked(gpTermSettings->value("LogEnable", DefaultLogEnable).toBool());
    ui->check_LogAppend->setChecked(gpTermSettings->value("LogMode", DefaultLogMode).toBool());

    //Set window size saving
    ui->check_EnableTerminalSizeSaving->setChecked(gpTermSettings->value("SaveSize", DefaultSaveSize).toBool());

    if (ui->check_EnableTerminalSizeSaving->isChecked() && gpTermSettings->contains("WindowWidth") && gpTermSettings->contains("WindowHeight"))
    {
        //Restore window size
        this->resize(gpTermSettings->value("WindowWidth", this->width()).toUInt(), gpTermSettings->value("WindowHeight", this->height()).toUInt());
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

    //Load last directory path
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
#ifndef SKIPSPEEDTEST
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
#ifndef SKIPSPEEDTEST
    ui->text_SpeedEditData->setFont(fntTmpFnt2);
#endif

    //Setup font spacing
    QFontMetrics tmTmpFM(fntTmpFnt2);
    ui->text_SpeedEditData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*8);
    ui->text_TermEditData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*8);
    ui->text_LogData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*8);

    //Setup the terminal scrollback buffer size
    ui->text_TermEditData->setup_scrollback(gpTermSettings->value("ScrollbackBufferSize", DefaultScrollbackBufferSize).toUInt());

    //Inform terminal what to do with VT100 control codes
    if (ui->radio_vt100_ignore->isChecked() == true)
    {
        on_radio_vt100_ignore_toggled(true);
    }
    else if (ui->radio_vt100_strip->isChecked() == true)
    {
        on_radio_vt100_strip_toggled(true);
    }
    else if (ui->radio_vt100_decode->isChecked() == true)
    {
        on_radio_vt100_decode_toggled(true);
    }

#ifndef SKIPSERIALDETECT
    serial_detect = nullptr;
    serial_detect_waiting = false;
    serial_close_dialog_open = false;

    ui->check_reconnect_after_disconnect->setChecked(gpTermSettings->value("ReconnectAfterDisconnect", DefaultReconnectAfterDisconnect).toBool());

    if (ui->check_reconnect_after_disconnect->isChecked())
    {
        serial_detect = new AutSerialDetect(this);
        connect(serial_detect, SIGNAL(port_reconnected(QString)), this, SLOT(serial_port_reconnected(QString)));
    }
#else
    ui->check_reconnect_after_disconnect->deleteLater();
#endif

    //Check command line
    QStringList slArgs = QCoreApplication::arguments();
    unsigned char chi = 1;
    bool bArgCom = false;
    bool bArgNoConnect = false;
#ifndef SKIPSCRIPTINGFORM
    bool bStartScript = false;
#endif
    while (chi < slArgs.length())
    {
        if (slArgs[chi].toUpper() == "DUPLICATE")
        {
            //Duplicate window so move to the top
            this->activateWindow();
            this->raise();
        }
        else if (slArgs[chi].left(5).toUpper() == "PORT=")
        {
            //Set serial port
            QString strPort = slArgs[chi].right(slArgs[chi].length()-5);
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
                guaAutomationForm = new AutAutomation(this);

                //Populate window handles for automation object
                guaAutomationForm->SetPopupHandle(gpmErrorForm);

                //Update automation form with connection status
                guaAutomationForm->ConnectionChange(transport_isOpen());

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
                guaAutomationForm = new AutAutomation(this);

                //Populate window handles for automation object
                guaAutomationForm->SetPopupHandle(gpmErrorForm);

                //Update automation form with connection status
                guaAutomationForm->ConnectionChange(transport_isOpen());

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
                gusScriptingForm = new AutScripting(this);

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

                if (transport_isOpen())
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

    //Setup UI elements
    ui->edit_LogFile->setEnabled(ui->check_LogEnable->isChecked());
    ui->check_LogAppend->setEnabled(ui->check_LogEnable->isChecked());

    if (bArgCom == true && bArgNoConnect == false)
    {
        //Enough information to connect!
        OpenDevice();
#ifndef SKIPSCRIPTINGFORM
        if (bStartScript == true)
        {
            //Start script execution request
            ScriptStartRequest();
        }
#endif
    }

    //Setup display buffer automatic trimming
    ui->check_trim->setChecked(gpTermSettings->value("AutoTrimDBuffer", DefaultAutoDTrimBuffer).toBool());
    ui->spin_trim_threshold->setValue(gpTermSettings->value("AutoTrimDBufferThreshold", DefaultAutoTrimDBufferThreshold).toULongLong());
    ui->spin_trim_size->setValue(gpTermSettings->value("AutoTrimDBufferSize", DefaultAutoTrimDBufferSize).toULongLong());
    on_check_trim_toggled(ui->check_trim->isChecked());
    update_display_trimming();

#ifdef __APPLE__
    //Show a warning to Mac users with the FTDI driver installed
    if ((QFile::exists("/System/Library/Extensions/FTDIUSBSerialDriver.kext") || QFile::exists("/Library/Extensions/FTDIUSBSerialDriver.kext")) && gpTermSettings->value("MacFTDIDriverWarningShown").isNull())
    {
        //FTDI driver detected and warning has not been shown, show warning
        gpTermSettings->setValue("MacFTDIDriverWarningShown", 1);
        QString strMessage = tr("Warning: The Mac FTDI VCP driver has been detected on your system. There is a known issue with this driver that can cause your system to crash if the serial port is closed and the buffer is not empty.\r\n\r\nIf you experience this issue, it is recommended that you remove the FTDI driver and use the apple VCP driver instead. Instructions to do this are available from the FTDI website (follow the uninstall section): http://www.ftdichip.com/Support/Documents/AppNotes/AN_134_FTDI_Drivers_Installation_Guide_for_MAC_OSX.pdf\r\n\r\nThis message will not be shown again.");
        gpmErrorForm->SetMessage(&strMessage);
        gpmErrorForm->show();
    }
#endif

#ifdef SKIPONLINE
    ui->check_enable_online_version_check->deleteLater();
    ui->label_version_update->deleteLater();
#else
    //Set status of online update checks
    ui->check_enable_online_version_check->setChecked(gpTermSettings->value("UpdateCheck", DefaultOnlineUpdateCheck).toBool());
#endif

    gbAppStarted = true;

#ifndef SKIPPLUGINS
    i = 0;

    while (i < plugin_list.length())
    {
        plugin_list[i].plugin->setup_finished();
        ++i;
    }
#endif

#ifndef SKIPONLINE
    if (ui->check_enable_online_version_check->isChecked())
    {
        bool run_check = true;

        if (gpTermSettings->contains("LastUpdateCheck"))
        {
            if (gpTermSettings->value("LastUpdateCheck", QDate::currentDate().toString()).toString() == QDate::currentDate().toString())
            {
                run_check = false;
            }
        }

        if (run_check == true)
        {
            gpTermSettings->setValue("LastUpdateCheck", QDate::currentDate().toString());
            ui->label_version_update->setText("checking...");
            AuTermUpdateCheck();
        }
        else
        {
            ui->label_version_update->setText("already checked today.");
        }
    }
#endif
}

AutMainWindow::~AutMainWindow()
{
    //Disconnect all signals
    disconnect(this, SLOT(close()));
    disconnect(this, SLOT(enter_pressed()));
    disconnect(this, SLOT(key_pressed(int,QChar)));
    disconnect(this, SLOT(vt100_send(QByteArray)));
    disconnect(this, SLOT(MenuSelected(QAction*)));
    disconnect(this, SLOT(balloontriggered(QAction*)));
    disconnect(this, SLOT(ContextMenuClosed()));
    disconnect(this, SLOT(SerialStatusSlot()));
    disconnect(this, SLOT(SerialRead()));
    disconnect(this, SLOT(SerialError(QSerialPort::SerialPortError)));
    disconnect(this, SLOT(SerialBytesWritten(qint64)));
    disconnect(this, SLOT(UpdateReceiveText()));
    disconnect(this, SLOT(SerialPortClosing()));
#ifndef SKIPONLINE
    disconnect(this, SLOT(replyFinished(QNetworkReply*)));
#ifndef QT_NO_SSL
    disconnect(this, SLOT(sslErrors(QNetworkReply*,const QList<QSslError>)));
#endif
#endif
    disconnect(this, SLOT(MessagePass(QByteArray,bool,bool)));
#ifndef SKIPSPEEDTEST
    disconnect(this, SLOT(update_displayText()));
    disconnect(this, SLOT(UpdateSpeedTestValues()));
    disconnect(this, SLOT(OutputSpeedTestStats()));
#ifndef SKIPPLUGINS
    //Plugins should disconnect these but just to be sure
    disconnect(this, SLOT(plugin_set_status(bool,bool,bool*)));
    disconnect(this, SLOT(plugin_add_open_close_button(QPushButton*)));
    disconnect(this, SLOT(plugin_to_hex(QByteArray*)));
    disconnect(this, SLOT(plugin_serial_open_close(uint8_t)));
    disconnect(this, SLOT(plugin_serial_transmit(QByteArray*)));
#endif

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

    if (transport_isOpen() == true || transport_isOpening() == true)
    {
        //Close serial connection before quitting
        transport_close();
        gpSignalTimer->stop();
#ifndef SKIPPLUGINS
        emit plugin_serial_closed();
        gbPluginHideTerminalOutput = false;
        gbPluginRunning = false;
#endif
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

#ifndef SKIPONLINE
    if (gnmManager != 0)
    {
        //Clear up network manager
        delete gnmManager;
    }
#endif

    display_buffers.clear();

#ifndef SKIPSPEEDTEST
    gbaSpeedReceivedData.squeeze();
#endif

#ifndef SKIPPLUGINS
    //Clear up plugins
    int32_t i = 0;
    while (i < plugin_list.length())
    {
        delete plugin_list.at(i).object;
#ifndef QT_STATIC
        plugin_list.at(i).plugin_loader->unload();
        delete plugin_list.at(i).plugin_loader;
#endif
        ++i;
    }
#endif

#ifndef SKIPSERIALDETECT
    if (serial_detect != nullptr)
    {
        if (serial_detect_waiting == true)
        {
            serial_detect->stop();
        }
        disconnect(this, SLOT(serial_port_reconnected(QString)));
        delete serial_detect;
    }
#endif

    //Delete variables
    delete gpMainLog;
    delete gpPredefinedDevice;
    delete gpTermSettings;
    delete gpErrorMessages;
    delete gpSignalTimer;
#ifndef SKIPSPEEDTEST
    delete gpSpeedMenu;
#endif
    delete gpBalloonMenu;
    delete gpSMenu4;
    delete gpMenu;
    delete gpEmptyCirclePixmap;
    delete gpRedCirclePixmap;
    delete gpGreenCirclePixmap;
    delete gpUw16Pixmap;
#if 0
    delete gpUw32Pixmap;
#endif
    delete gpmErrorForm;
    delete ui;
}

void AutMainWindow::closeEvent(QCloseEvent *)
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

void AutMainWindow::on_btn_Connect_clicked()
{
    //Connect to COM port button clicked.
    OpenDevice();
}

void AutMainWindow::on_btn_TermClose_clicked(bool from_plugin)
{
    if (transport_isOpen() == false && transport_isOpening() == false)
    {
        //Open connection
        OpenDevice(from_plugin);
    }
    else
    {
        //Close, but first clear up from download/streaming
        gbTermBusy = false;
        gchTermMode = 0;
        ui->btn_Cancel->setEnabled(false);
        if (gbStreamingFile == true)
        {
            //Clear up file stream
            gtmrStreamTimer.invalidate();
            gbStreamingFile = false;
            gpStreamFileHandle->close();
            delete gpStreamFileHandle;
        }
#ifndef SKIPSPEEDTEST
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
            gchSpeedTestMode = SPEED_MODE_INACTIVE;
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
        transport_clear();
        transport_close();

#ifndef SKIPPLUGINS
        emit plugin_serial_closed();
        gbPluginHideTerminalOutput = false;
        gbPluginRunning = false;
#endif
        gpSignalTimer->stop();

        //Disable active checkboxes
        ui->check_Break->setEnabled(false);
        ui->check_DTR->setEnabled(false);
        ui->check_Echo->setEnabled(false);
        ui->check_Line->setEnabled(false);
        ui->check_RTS->setEnabled(false);
#ifndef SKIPSPEEDTEST
        ui->check_SpeedDTR->setEnabled(false);
        ui->check_SpeedRTS->setEnabled(false);
#endif

        //Change status message
        ui->statusBar->showMessage("");

        //Change button text
        ui->btn_TermClose->setText("&Open Port");
#ifndef SKIPSPEEDTEST
        ui->btn_SpeedClose->setText("&Open Port");
#endif

#ifndef SKIPPLUGINS
        uint8_t i = 0;
        while (i < list_plugin_open_close_buttons.length())
        {
            list_plugin_open_close_buttons[i]->setText("&Open Port");
            ++i;
        }
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

#ifndef SKIPSPEEDTEST
        //Disable speed testing
        ui->btn_SpeedStartStop->setEnabled(false);
#endif
    }

    //Update images
    UpdateImages();
}

void AutMainWindow::on_btn_Refresh_clicked()
{
    //Refresh the list of serial ports
    RefreshSerialDevices();
}

void AutMainWindow::RefreshSerialDevices()
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

void AutMainWindow::on_btn_TermClear_clicked()
{
    //Clears the screen of the terminal tab
    ui->text_TermEditData->clear_dat_in();
}

void AutMainWindow::SerialRead()
{
    //Update the last received field
    if ((gtmrPortOpened.elapsed() / 1000) > gintLastSerialTimeUpdate)
    {
        ui->label_LastRx->setText(QDateTime::currentDateTime().toString("dd/MM @ hh:mm:ss"));
        gintLastSerialTimeUpdate = (gtmrPortOpened.elapsed() / 1000);
    }

    //Read the data into a buffer and copy it to edit for the display data
#ifndef SKIPSPEEDTEST
    if (gbSpeedTestRunning == true)
    {
        //Serial test is running, pass to speed test function
        SpeedTestReceive();
        return;
    }
#endif
    QByteArray baOrigData = transport_readAll();
//    qDebug() << "Received: " << baOrigData;


#ifndef SKIPPLUGINS
    if (gbPluginHideTerminalOutput == false || gbPluginRunning == false)
#endif
    {
        //Speed test is not running
//qDebug() << baOrigData;

#ifndef SKIPSCRIPTINGFORM
        if (gusScriptingForm != 0 && gbScriptingRunning == true)
        {
            gusScriptingForm->SerialPortData(&baOrigData);
        }
#endif

//        if (gbTermBusy == false)
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
            update_buffer(&baDispData, true);

            if (gbLoopbackMode == true)
            {
                //Loopback enabled, send this data back
                transport_write(baOrigData);
                gintQueuedTXBytes += baOrigData.length();
                gpMainLog->WriteRawLogData(baOrigData);
                update_buffer(&baDispData, false);
            }
        }

        //Update number of recieved bytes
        gintRXBytes = gintRXBytes + baOrigData.length();
        ui->label_TermRx->setText(QString::number(gintRXBytes));
    }

#ifndef SKIPPLUGINS
    if (gbPluginRunning == true)
    {
        //A plugin is running, siphon data to it
//TODO: limit to the running plugin only
        emit plugin_serial_receive(&baOrigData);
    }
#endif
}

void AutMainWindow::on_text_TermEditData_customContextMenuRequested(const QPoint &pos)
{
    //Creates the custom context menu
    gpMenu->popup(ui->text_TermEditData->viewport()->mapToGlobal(pos));
    ui->text_TermEditData->mbContextMenuOpen = true;
}

void AutMainWindow::MenuSelected(QAction* qaAction)
{
    //Runs when a menu item is selected
    int intItem = qaAction->data().toInt();

    if (intItem == MenuActionError)
    {
        //Shows a meaning for the error code selected
        bool bTmpBool;

        //QString
        unsigned int uiErrCode = ui->text_TermEditData->textCursor().selection().toPlainText().toUInt(&bTmpBool, 0);
        if (bTmpBool == true)
        {
            //Converted
            LookupErrorCode(uiErrCode);
        }
        else
        {
            update_buffer("\nSelected text does not have a numeric value, cannot look up error code.\n", false);
        }
    }
    else if (intItem == MenuActionLoopback && gbTermBusy == false && gbSpeedTestRunning == false)
    {
        //Enable/disable loopback mode
        SetLoopBackMode(!gbLoopbackMode);
    }
    else if (intItem == MenuActionStreamFile && gbTermBusy == false && gbSpeedTestRunning == false)
    {
        //Stream out a file
        if (transport_isOpen() == true && gbLoopbackMode == false && gbTermBusy == false)
        {
            //Not currently busy
            QString strFilename = QFileDialog::getOpenFileName(this, tr("Open File To Stream"), gstrLastFilename[FilenameIndexOthers], tr("Text Files (*.txt);;All Files (*.*)"));

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
                    gpmErrorForm->SetMessage(&strMessage);
                    gpmErrorForm->show();
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
                transport_write(baFileData);
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
#ifndef SKIPSPEEDTEST
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
#ifndef SKIPSPEEDTEST
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
#ifndef SKIPSPEEDTEST
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
        palTmp.setColor(QPalette::Active, QPalette::Text, QColorConstants::White);
        palTmp.setColor(QPalette::Active, QPalette::Base, QColorConstants::Black);
        ui->text_TermEditData->setPalette(palTmp);
        ui->text_LogData->setPalette(palTmp);

        //And font
        QFont fntTmpFnt = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        QFontMetrics tmTmpFM(fntTmpFnt);
        ui->text_TermEditData->setFont(fntTmpFnt);
        ui->text_TermEditData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);
        ui->text_LogData->setFont(fntTmpFnt);
        ui->text_LogData->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);
#ifndef SKIPSPEEDTEST
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
            guaAutomationForm = new AutAutomation(this);

            //Populate window handles for automation object
            guaAutomationForm->SetPopupHandle(gpmErrorForm);

            //Update automation form with connection status
            guaAutomationForm->ConnectionChange(transport_isOpen());

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
            gusScriptingForm = new AutScripting(this);

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

            if (transport_isOpen())
            {
                //Tell the form that the serial port is open
                gusScriptingForm->SerialPortStatus(true);
            }
        }
        gusScriptingForm->show();
        gusScriptingForm->SetEditorFocus();
    }
#endif
    else if (intItem == MenuActionClearDisplay)
    {
        //Clear display
        ui->text_TermEditData->clear_dat_in();
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
        ui->text_TermEditData->add_dat_out_text(QApplication::clipboard()->text());
    }
    else if (intItem == MenuActionSelectAll)
    {
        //Select all text
        ui->text_TermEditData->selectAll();
    }
}

void AutMainWindow::balloontriggered(QAction* qaAction)
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

void AutMainWindow::enter_pressed()
{
    //Enter pressed in line mode
    if (transport_isOpen())
    {
        if (gbTermBusy == false)
        {
            if (gbLoopbackMode == false)
            {
                QByteArray baTmpBA = ui->text_TermEditData->get_dat_out()->replace("\r", "\n").replace("\n", (ui->radio_LCR->isChecked() ? "\r" : ui->radio_LLF->isChecked() ? "\n" : ui->radio_LCRLF->isChecked() ? "\r\n" : "")).toUtf8();
                transport_write(baTmpBA);
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
                update_buffer("\n[Cannot send: Loopback mode is enabled.]\n", false);
            }

            if (ui->check_Echo->isChecked() == true)
            {
                //Local echo
                update_buffer(ui->text_TermEditData->get_dat_out()->toUtf8().append("\n"), false);
            }
            ui->text_TermEditData->clear_dat_out();
        }
    }
}

void AutMainWindow::UpdateImages()
{
    //Updates images to reflect status
    if (transport_isOpen() == true)
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

void AutMainWindow::key_pressed(int nKey, QChar chrKeyValue)
{
    //Key pressed, send it out
    if (transport_isOpen())
    {
        if (gbTermBusy == false && gbLoopbackMode == false)
        {
            if (ui->check_Echo->isChecked() == true)
            {
                //Echo mode on
                if (nKey == Qt::Key_Enter || nKey == Qt::Key_Return)
                {
                    update_buffer("\n", false);
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
                transport_write(baTmpBA);
                gintQueuedTXBytes += baTmpBA.size();
            }

            //Output back to screen buffer if echo mode is enabled
            if (ui->check_Echo->isChecked())
            {
                if (ui->check_ShowCLRF->isChecked() == true)
                {
                    //Escape \t, \r and \n in addition to normal escaping
                    update_buffer(baTmpBA.replace("\t", "\\t").replace("\r", "\\r").replace("\n", "\\n").replace('\0', "\\00").replace("\x01", "\\01").replace("\x02", "\\02").replace("\x03", "\\03").replace("\x04", "\\04").replace("\x05", "\\05").replace("\x06", "\\06").replace("\x07", "\\07").replace("\x08", "\\08").replace("\x0b", "\\0B").replace("\x0c", "\\0C").replace("\x0e", "\\0E").replace("\x0f", "\\0F").replace("\x10", "\\10").replace("\x11", "\\11").replace("\x12", "\\12").replace("\x13", "\\13").replace("\x14", "\\14").replace("\x15", "\\15").replace("\x16", "\\16").replace("\x17", "\\17").replace("\x18", "\\18").replace("\x19", "\\19").replace("\x1a", "\\1a").replace("\x1b", "\\1b").replace("\x1c", "\\1c").replace("\x1d", "\\1d").replace("\x1e", "\\1e").replace("\x1f", "\\1f"), false);
                }
                else
                {
                    //Normal escaping
                    update_buffer(baTmpBA.replace('\0', "\\00").replace("\x01", "\\01").replace("\x02", "\\02").replace("\x03", "\\03").replace("\x04", "\\04").replace("\x05", "\\05").replace("\x06", "\\06").replace("\x07", "\\07").replace("\x08", "\\08").replace("\x0b", "\\0B").replace("\x0c", "\\0C").replace("\x0e", "\\0E").replace("\x0f", "\\0F").replace("\x10", "\\10").replace("\x11", "\\11").replace("\x12", "\\12").replace("\x13", "\\13").replace("\x14", "\\14").replace("\x15", "\\15").replace("\x16", "\\16").replace("\x17", "\\17").replace("\x18", "\\18").replace("\x19", "\\19").replace("\x1a", "\\1a").replace("\x1b", "\\1b").replace("\x1c", "\\1c").replace("\x1d", "\\1d").replace("\x1e", "\\1e").replace("\x1f", "\\1f"), false);
                }

                //Output to log file
                gpMainLog->WriteLogData(QString(chrKeyValue).toUtf8());
            }
        }
        else if (gbLoopbackMode == true)
        {
            //Loopback is enabled
            update_buffer("[Cannot send: Loopback mode is enabled.]\n", false);
        }
    }
}

void AutMainWindow::vt100_send(QByteArray code)
{
    //Key pressed, send it out
    if (transport_isOpen())
    {
        if (gbTermBusy == false && gbLoopbackMode == false)
        {
            transport_write(code);
            gintQueuedTXBytes += code.size();
        }
        else if (gbLoopbackMode == true)
        {
            //Loopback is enabled
            update_buffer("[Cannot send: Loopback mode is enabled.]\n", false);
        }
    }
}

void AutMainWindow::DoLineEnd()
{
    //Outputs a line ending
    if (ui->radio_LLF->isChecked())
    {
        //LF (\n) *nix style
        transport_write("\n");
        gintQueuedTXBytes += 1;
    }
    else if (ui->radio_LCR->isChecked())
    {
        //CR (\r)
        transport_write("\r");
        gintQueuedTXBytes += 1;
    }
    else if (ui->radio_LCRLF->isChecked())
    {
        //CR LF (\r\n) windows style
        transport_write("\r\n");
        gintQueuedTXBytes += 2;
    }
    return;
}

void AutMainWindow::SerialStatus(bool bType)
{
    if (transport_isOpen() == true)
    {
        unsigned int intSignals = transport_pinoutSignals();

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
}

void AutMainWindow::SerialStatusSlot()
{
    //Slot function to update serial pinout status
    SerialStatus(0);
}

void AutMainWindow::OpenDevice(bool from_plugin)
{
    //Function to open serial port
    bool port_opened = false;

    if (transport_isOpen() == true || transport_isOpening() == true)
    {
        //Serial port is already open - cancel any pending operations
        if (gbTermBusy == true)
        {
            //Run cancel operation
            on_btn_Cancel_clicked();
        }

#ifndef SKIPSERIALDETECT
        if (serial_detect_waiting == true)
        {
            //Only triggered here when user connects to a different serial device than the one waiting for reconnect
            serial_close_dialog_open = false;
            serial_detect_waiting = false;
            serial_detect->stop();
            ui->btn_Cancel->setEnabled(false);
        }
#endif

        //Close serial port
        transport_clear();
        transport_close();

#ifndef SKIPPLUGINS
        emit plugin_serial_closed();
        gbPluginHideTerminalOutput = false;
        gbPluginRunning = false;
#endif
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

#ifndef SKIPPLUGINS_TRANSPORT
    if (ui->tab_transport->currentIndex() == 0)
#endif
    {
        //Use UART
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
                port_opened = true;

                //Update tooltip of system tray
                if (gbSysTrayEnabled == true)
                {
                    gpSysTray->setToolTip(QString("AuTerm v").append(UwVersion).append(" (").append(ui->combo_COM->currentText()).append(")"));
                }

                //Flow control
                if (ui->combo_Handshake->currentIndex() == 1)
                {
                    //Hardware handshaking
                    ui->check_RTS->setEnabled(false);
#ifndef SKIPSPEEDTEST
                    ui->check_SpeedRTS->setEnabled(false);
#endif
                }
                else
                {
                    //Not hardware handshaking - RTS
                    ui->check_RTS->setEnabled(true);
#ifndef SKIPSPEEDTEST
                    ui->check_SpeedRTS->setEnabled(true);
#endif
                    gspSerialPort.setRequestToSend(ui->check_RTS->isChecked());
                }
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
                gpmErrorForm->SetMessage(&strMessage);
                gpmErrorForm->show();
                ui->text_TermEditData->set_serial_open(false);
            }
        }
        else
        {
            //No serial port selected
            QString strMessage = tr("No serial port was selected, please select a serial port and try again.\r\nIf you see no serial ports listed, ensure your device is connected to your computer and you have the appropriate drivers installed.");
            gpmErrorForm->SetMessage(&strMessage);
            gpmErrorForm->show();
        }
    }
#ifndef SKIPPLUGINS_TRANSPORT
    else
    {
        qsizetype pos_underscore = ui->tab_transport->currentWidget()->objectName().lastIndexOf('_');
        uint plugin_index;

        if (pos_underscore == -1)
        {
            //TODO: error
            return;
        }

        plugin_index = ui->tab_transport->currentWidget()->objectName().mid((pos_underscore + 1)).toUInt();

        if (plugin_index >= plugin_list.length())
        {
            //TODO: error
            return;
        }

        //TODO: ensure plugin is transport

        plugin_active_transport = (AutTransportPlugin *)plugin_list.at(plugin_index).plugin;

        if (plugin_active_transport->open(QIODevice::ReadWrite))
        {
            //Successful
            port_opened = true;

            //Update tooltip of system tray
            if (gbSysTrayEnabled == true)
            {
//TODO
                gpSysTray->setToolTip(QString("AuTerm v").append(UwVersion).append(" (").append(ui->combo_COM->currentText()).append(")"));
            }

//TODO
            //Flow control
#if 0
            if (ui->combo_Handshake->currentIndex() == 1)
            {
                //Hardware handshaking
                ui->check_RTS->setEnabled(false);
#ifndef SKIPSPEEDTEST
                ui->check_SpeedRTS->setEnabled(false);
#endif
            }
            else
            {
                //Not hardware handshaking - RTS
                ui->check_RTS->setEnabled(true);
#ifndef SKIPSPEEDTEST
                ui->check_SpeedRTS->setEnabled(true);
#endif
                gspSerialPort.setRequestToSend(ui->check_RTS->isChecked());
            }
#endif
            ui->check_RTS->setEnabled(false);
#ifndef SKIPSPEEDTEST
            ui->check_SpeedRTS->setEnabled(false);
#endif
        }
        else
        {
            //Error whilst opening
            //todo: change to transport
            ui->statusBar->showMessage("Error: ");
            ui->statusBar->showMessage(ui->statusBar->currentMessage().append(gspSerialPort.errorString()));

            QString strMessage = tr("Error whilst attempting to open the serial device: ").append(gspSerialPort.errorString()).append("\n\nIf the serial port is open in another application, please close the other application")
#if !defined(_WIN32) && !defined( __APPLE__)
                                     .append(", please also ensure you have been granted permission to the serial device in /dev/")
#endif
                                     .append((ui->combo_Baud->currentText().toULong() > 115200 ? ", please also ensure that your serial device supports baud rates greater than 115200 (normal COM ports do not have support for these baud rates)" : ""))
                                     .append(" and try again.");
            gpmErrorForm->SetMessage(&strMessage);
            gpmErrorForm->show();
            ui->text_TermEditData->set_serial_open(false);

            plugin_active_transport = nullptr;
        }
    }
#endif

    if (port_opened == true)
    {
        ui->statusBar->showMessage(QString("[").append(ui->combo_COM->currentText()).append(":").append(ui->combo_Baud->currentText()).append(",").append((ui->combo_Parity->currentIndex() == 0 ? "N" : ui->combo_Parity->currentIndex() == 1 ? "O" : ui->combo_Parity->currentIndex() == 2 ? "E" : "")).append(",").append(ui->combo_Data->currentText()).append(",").append(ui->combo_Stop->currentText()).append(",").append((ui->combo_Handshake->currentIndex() == 0 ? "N" : ui->combo_Handshake->currentIndex() == 1 ? "H" : ui->combo_Handshake->currentIndex() == 2 ? "S" : "")).append("]{").append((ui->radio_LCR->isChecked() ? "\\r" : (ui->radio_LLF->isChecked() ? "\\n" : (ui->radio_LCRLF->isChecked() ? "\\r\\n" : "")))).append("}"));
        ui->label_TermConn->setText(ui->statusBar->currentMessage());
#ifndef SKIPSPEEDTEST
        ui->label_SpeedConn->setText(ui->statusBar->currentMessage());
#endif

        //Switch to Terminal tab if not on terminal or speed testing tab
        if (from_plugin == false && ui->selector_Tab->currentIndex() != ui->selector_Tab->indexOf(ui->tab_Term) && ui->selector_Tab->currentIndex() != ui->selector_Tab->indexOf(ui->tab_SpeedTest))
        {
            ui->selector_Tab->setCurrentIndex(ui->selector_Tab->indexOf(ui->tab_Term));
        }

        //Disable read-only mode
        ui->text_TermEditData->setReadOnly(false);

        //DTR
        if (transport_supports_data_terminal_ready() == true)
        {
            ui->check_DTR->setEnabled(true);
#ifndef SKIPSPEEDTEST
            ui->check_SpeedDTR->setEnabled(true);
#endif
            transport_setDataTerminalReady(ui->check_DTR->isChecked());
        }
        else
        {
            ui->check_DTR->setEnabled(false);
#ifndef SKIPSPEEDTEST
            ui->check_SpeedDTR->setEnabled(true);
#endif
        }

#if 0
        //Flow control
        if (ui->combo_Handshake->currentIndex() == 1)
        {
            //Hardware handshaking
            ui->check_RTS->setEnabled(false);
#ifndef SKIPSPEEDTEST
            ui->check_SpeedRTS->setEnabled(false);
#endif
        }
        else
        {
            //Not hardware handshaking - RTS
            ui->check_RTS->setEnabled(true);
#ifndef SKIPSPEEDTEST
            ui->check_SpeedRTS->setEnabled(true);
#endif
            gspSerialPort.setRequestToSend(ui->check_RTS->isChecked());
        }
#endif

        //Break
        if (transport_supports_break() == true)
        {
            ui->check_Break->setEnabled(true);
            transport_setBreakEnabled(ui->check_Break->isChecked());
        }
        else
        {
            ui->check_Break->setEnabled(false);
        }

        //Enable checkboxes
//        ui->check_Break->setEnabled(true);
//        ui->check_DTR->setEnabled(true);
//#ifndef SKIPSPEEDTEST
//        ui->check_SpeedDTR->setEnabled(true);
//#endif
        ui->check_Echo->setEnabled(true);
        ui->check_Line->setEnabled(true);

        //Update button text
        ui->btn_TermClose->setText("C&lose Port");
#ifndef SKIPSPEEDTEST
        ui->btn_SpeedClose->setText("C&lose Port");
#endif

#ifndef SKIPPLUGINS
        uint8_t i = 0;
        while (i < list_plugin_open_close_buttons.length())
        {
            list_plugin_open_close_buttons[i]->setText("C&lose Port");
            ++i;
        }
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
        ui->text_TermEditData->set_serial_open(true);

        //Set focus to input text edit
        ui->text_TermEditData->setFocus();

        //Disable log options
        ui->edit_LogFile->setEnabled(false);
        ui->check_LogEnable->setEnabled(false);
        ui->check_LogAppend->setEnabled(false);
        ui->btn_LogFileSelect->setEnabled(false);

#ifndef SKIPSPEEDTEST
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
            if (gpMainLog->OpenLogFile(QString((ui->edit_LogFile->text().left(1) == "/" || ui->edit_LogFile->text().left(1) == "\\") ? "" : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).append("/").append(ui->edit_LogFile->text())) == LOG_OK)
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
                gpmErrorForm->SetMessage(&strMessage);
                gpmErrorForm->show();
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
}

void AutMainWindow::on_check_Break_stateChanged()
{
    //Break status changed
    transport_setBreakEnabled(ui->check_Break->isChecked());
}

void AutMainWindow::on_check_RTS_stateChanged()
{
    //RTS status changed
    transport_setRequestToSend(ui->check_RTS->isChecked());
#ifndef SKIPSPEEDTEST
    if (ui->check_SpeedRTS->isChecked() != ui->check_RTS->isChecked())
    {
        //Update speed form checkbox
        ui->check_SpeedRTS->setChecked(ui->check_RTS->isChecked());
    }
#endif
}

void AutMainWindow::on_check_DTR_stateChanged()
{
    //DTR status changed
    transport_setDataTerminalReady(ui->check_DTR->isChecked());
#ifndef SKIPSPEEDTEST
    if (ui->check_SpeedDTR->isChecked() != ui->check_DTR->isChecked())
    {
        //Update speed form checkbox
        ui->check_SpeedDTR->setChecked(ui->check_DTR->isChecked());
    }
#endif
}

void AutMainWindow::on_check_Line_stateChanged()
{
    //Line mode status changed
    ui->text_TermEditData->set_line_mode(ui->check_Line->isChecked());
}

void AutMainWindow::SerialError(QSerialPort::SerialPortError speErrorCode)
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
#ifndef SKIPPLUGINS
        emit plugin_serial_opened();
#endif
        return;
    }
    else if (speErrorCode == QSerialPort::ResourceError || speErrorCode == QSerialPort::PermissionError)
    {
        //Resource error or permission error (device unplugged?)
        QString strMessage = tr("Fatal error with serial connection.\nPlease reconnect to the device to continue.");
        gpmErrorForm->SetMessage(&strMessage);
        gpmErrorForm->show();
        ui->text_TermEditData->set_serial_open(false);

        if (gspSerialPort.isOpen() == true)
        {
            //Close active connection
#ifndef SKIPSERIALDETECT
            if (ui->check_reconnect_after_disconnect->isChecked() && serial_detect != nullptr)
            {
                //Start watching for this serial port to reappear and automatically re-connect to it
                serial_detect->start(gspSerialPort.portName());
                serial_detect_waiting = true;
                serial_close_dialog_open = true;
                ui->btn_Cancel->setEnabled(true);
                ui->statusBar->showMessage(QString("Waiting for ").append(gspSerialPort.portName()).append(" to reappear..."));
            }
#endif
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
#ifndef SKIPSPEEDTEST
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
            gchSpeedTestMode = SPEED_MODE_INACTIVE;
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

#ifndef SKIPSERIALDETECT
        if (serial_detect_waiting == false)
        {
#endif
            //Disable cancel button
            ui->btn_Cancel->setEnabled(false);
#ifndef SKIPSERIALDETECT
        }
#endif

        //Disable active checkboxes
        ui->check_Break->setEnabled(false);
        ui->check_DTR->setEnabled(false);
        ui->check_Echo->setEnabled(false);
        ui->check_Line->setEnabled(false);
        ui->check_RTS->setEnabled(false);
#ifndef SKIPSPEEDTEST
        ui->check_SpeedDTR->setEnabled(false);
        ui->check_SpeedRTS->setEnabled(false);
#endif

        //Disable text entry
        ui->text_TermEditData->setReadOnly(true);

#ifndef SKIPSERIALDETECT
        if (serial_detect_waiting == false)
        {
#endif
            //Change status message
            ui->statusBar->showMessage("");
#ifndef SKIPSERIALDETECT
        }
#endif

        //Change button text
        ui->btn_TermClose->setText("&Open Port");
#ifndef SKIPSPEEDTEST
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

#ifndef SKIPPLUGINS
    emit plugin_serial_error(speErrorCode);

    if (port_closed == true)
    {
        emit plugin_serial_closed();
        gbPluginHideTerminalOutput = false;
        gbPluginRunning = false;
    }
#endif
}

void AutMainWindow::on_btn_Duplicate_clicked()
{
    //Duplicates instance of AuTerm
    QProcess DuplicateProcess;
    DuplicateProcess.startDetached(QCoreApplication::applicationFilePath(), QStringList() << "DUPLICATE" <<  QString("PORT=").append(ui->combo_COM->currentText()) << QString("BAUD=").append(ui->combo_Baud->currentText()) << tr("STOP=").append(ui->combo_Stop->currentText()) << tr("DATA=").append(ui->combo_Data->currentText()) << tr("PAR=").append(ui->combo_Parity->currentText()) << tr("FLOW=").append(QString::number(ui->combo_Handshake->currentIndex())) << tr("ENDCHR=").append((ui->radio_LCR->isChecked() == true ? "0" : ui->radio_LLF->isChecked() == true ? "1" : "2")) << tr("LOCALECHO=").append((ui->check_Echo->isChecked() == true ? "1" : "0")) << tr("LINEMODE=").append((ui->check_Line->isChecked() == true ? "1" : "0")) << "NOCONNECT");
}

void AutMainWindow::MessagePass(QByteArray baDataString, bool bEscapeString, bool bFromScripting)
{
    //Receive a command from the automation window
    if (transport_isOpen() == true && (gbTermBusy == false || bFromScripting == true) && gbLoopbackMode == false)
    {
        if (bEscapeString == true)
        {
            //Escape string sequences
            AutEscape::escape_characters(&baDataString);
        }

        //Output the data and send it to the log
        transport_write(baDataString);
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
//            baDataString.replace('\0', "\\00").replace("\x01", "\\01").replace("\x02", "\\02").replace("\x03", "\\03").replace("\x04", "\\04").replace("\x05", "\\05").replace("\x06", "\\06").replace("\x07", "\\07").replace("\x08", "\\08").replace("\x0b", "\\0B").replace("\x0c", "\\0C").replace("\x0e", "\\0E").replace("\x0f", "\\0F").replace("\x10", "\\10").replace("\x11", "\\11").replace("\x12", "\\12").replace("\x13", "\\13").replace("\x14", "\\14").replace("\x15", "\\15").replace("\x16", "\\16").replace("\x17", "\\17").replace("\x18", "\\18").replace("\x19", "\\19").replace("\x1a", "\\1a").replace("\x1b", "\\1b").replace("\x1c", "\\1c").replace("\x1d", "\\1d").replace("\x1e", "\\1e").replace("\x1f", "\\1f");

            //Output to display buffer
            update_buffer(&baDataString, false);
        }

        if (bEscapeString == false && bFromScripting == false)
        {
            //Not escaping sequences and not from scripting form so send line end
            DoLineEnd();
            gpMainLog->WriteLogData("\n");
        }
    }
    else if (transport_isOpen() == true && gbLoopbackMode == true)
    {
        //Loopback is enabled
        update_buffer("\n[Cannot send: Loopback mode is enabled.]\n", false);
    }
}

void AutMainWindow::LookupErrorCode(unsigned int intErrorCode)
{
    //Looks up an error code and outputs it in the edit (does NOT store it to the log)
#ifndef SKIPERRORCODEFORM
    if (gbErrorsLoaded == true)
    {
        //Error file has been loaded
        on_btn_Error_clicked();
        gecErrorCodeForm->display_error(QString::number(intErrorCode));
    }
    else
    {
        //Error file has not been loaded
        update_buffer("\nUnable to lookup error code: error code file not loaded.\n", false);
    }
#else
    update_buffer("\nError code form support not enabled.\n", false);
#endif
//    ui->text_TermEditData->moveCursor(QTextCursor::End);
}

void AutMainWindow::SerialBytesWritten(qint64 intByteCount)
{
    //Updates the display with the number of bytes written
#ifndef SKIPSPEEDTEST
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
            transport_write(baFileData);
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
                update_buffer(QString("Streamed ").append(QString::number(gintStreamBytesRead)).append(" bytes (").append(QString::number(gintStreamBytesRead*100/gintStreamBytesSize)).append("%).\n").toUtf8(), false);
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
    }

#ifndef SKIPPLUGINS
    emit plugin_serial_bytes_written(intByteCount);
#endif
}

void AutMainWindow::on_btn_Cancel_clicked()
{
    //Cancel current stream or file download
    if (gbTermBusy == true)
    {
        if (gbStreamingFile == true)
        {
            //Cancel stream
            FinishStream(true);
        }
    }

#ifndef SKIPSERIALDETECT
    if (serial_detect_waiting == true)
    {
        serial_detect->stop();
        serial_detect_waiting = false;
        serial_close_dialog_open = false;
        ui->statusBar->clearMessage();
    }
#endif

    //Disable button
    ui->btn_Cancel->setEnabled(false);
}

void AutMainWindow::FinishStream(bool bType)
{
    //Sending a file stream has finished
    if (bType == true)
    {
        //Stream cancelled
        update_buffer(QString("\nCancelled stream after ").append(QString::number(gintStreamBytesRead)).append(" bytes (").append(QString::number(1+(gtmrStreamTimer.nsecsElapsed()/1000000000LL))).append(" seconds) [~").append(QString::number((gintStreamBytesRead/(1+gtmrStreamTimer.nsecsElapsed()/1000000000LL)))).append(" bytes/second].\n").toUtf8(), false);
        ui->statusBar->showMessage("File streaming cancelled.");
    }
    else
    {
        //Stream finished
        update_buffer(QString("\nFinished streaming file, ").append(QString::number(gintStreamBytesRead)).append(" bytes sent in ").append(QString::number(1+(gtmrStreamTimer.nsecsElapsed()/1000000000LL))).append(" seconds [~").append(QString::number((gintStreamBytesRead/(1+gtmrStreamTimer.nsecsElapsed()/1000000000LL)))).append(" bytes/second].\n").toUtf8(), false);
        ui->statusBar->showMessage("File streaming complete!");
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

void AutMainWindow::UpdateReceiveText()
{
    //Updates the receive text buffer
    if (ui->selector_Tab->currentWidget() == ui->tab_Term)
    {
        ui->text_TermEditData->add_display_data(&display_buffers);
        display_update_pending = false;
        display_buffers.clear();
    }
    else
    {
        display_update_pending = true;
    }
}

void AutMainWindow::on_combo_COM_currentIndexChanged(int)
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

void AutMainWindow::dragEnterEvent(QDragEnterEvent *dragEvent)
{
    //A file is being dragged onto the window
    if (dragEvent->mimeData()->urls().count() == 1 && gbTermBusy == false && transport_isOpen() == true)
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

void AutMainWindow::dropEvent(QDropEvent *dropEvent)
{
    //A file has been dragged onto the window
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
}

void AutMainWindow::on_btn_Github_clicked()
{
    //Open webpage at the AuTerm github page)
    if (QDesktopServices::openUrl(QUrl("https://github.com/thedjnK/AuTerm")) == false)
    {
        //Failed to open URL
        QString strMessage = tr("An error occured whilst attempting to open a web browser, please ensure you have a web browser installed and configured. URL: https://github.com/thedjnK/AuTerm");
        gpmErrorForm->SetMessage(&strMessage);
        gpmErrorForm->show();

#ifndef SKIPSERIALDETECT
        serial_close_dialog_open = false;
#endif
    }
}

#ifndef SKIPONLINE
void AutMainWindow::replyFinished(QNetworkReply* nrReply)
{
    //Response received from online server
    if (nrReply->error() != QNetworkReply::NoError)
    {
        //An error occured
        ui->btn_Cancel->setEnabled(false);
        if (!gtmrTextUpdateTimer.isActive())
        {
            gtmrTextUpdateTimer.start();
        }

        //Display error message if operation wasn't cancelled
        if (nrReply->error() != QNetworkReply::OperationCanceledError)
        {
            //Output error message
            QString strMessage = QString("An error occured during an online request: ").append(nrReply->errorString());
            gpmErrorForm->SetMessage(&strMessage);
            gpmErrorForm->show();
            ui->statusBar->showMessage("Network request error");

#ifndef SKIPSERIALDETECT
            serial_close_dialog_open = false;
#endif

            if (gchTermMode == mode_check_for_update)
            {
                ui->label_version_update->setText("network error.");
            }
        }
        else
        {
            if (gchTermMode == mode_check_for_update)
            {
                ui->label_version_update->setText("request cancelled.");
            }

            ui->statusBar->showMessage("Network request cancelled.");
        }

        gchTermMode = 0;
        gbTermBusy = false;
    }
    else
    {
        if (gchTermMode == mode_check_for_update)
        {
            //AuTerm update response
            QByteArray baTmpBA = nrReply->readAll();

            //Back to non-busy mode
            gchTermMode = 0;
            gbTermBusy = false;
            ui->btn_Cancel->setEnabled(false);

            if (baTmpBA.length() > 8)
            {
                //Something not quite right with this response...
                ui->label_version_update->setText("unknown response.");
                QString string_response = QString("Unknown response from server: %1").arg(QString(baTmpBA));
                gpmErrorForm->SetMessage(&string_response);
                gpmErrorForm->show();
                ui->statusBar->showMessage("Unknown server response");

#ifndef SKIPSERIALDETECT
                serial_close_dialog_open = false;
#endif

            }
            else
            {
                QString newest_version = baTmpBA;

                if (is_newer(&newest_version, &UwVersion) == true)
                {
                    ui->label_version_update->setText(QString("<a href=\"https://github.com/thedjnK/AuTerm/releases\">update available: %1</a>").arg(newest_version));
                    ui->statusBar->showMessage(QString("AuTerm update to version %1 available").arg(newest_version));
                }
                else
                {
                    ui->label_version_update->setText("no updates.");
                    ui->statusBar->showMessage("No AuTerm update available");
                }
            }
        }
    }

    //Queue the network reply object to be deleted
    nrReply->deleteLater();
}

#ifndef QT_NO_SSL
void AutMainWindow::sslErrors(QNetworkReply* nrReply, const QList<QSslError> lstSSLErrors)
{
    //Error detected with SSL
    QString string_response = "SSL error(s) during network request: ";
    uint16_t i = 0;

    while (i < lstSSLErrors.length())
    {
        string_response.append(lstSSLErrors[i].errorString());
        ++i;
    }

    gpmErrorForm->SetMessage(&string_response);
    gpmErrorForm->show();

#ifndef SKIPSERIALDETECT
    serial_close_dialog_open = false;
#endif
}
#endif
#endif

QList<QString> AutMainWindow::SplitFilePath(QString strFilename)
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

void AutMainWindow::on_check_Echo_stateChanged(int)
{
    //Local echo checkbox state changed
    ui->text_TermEditData->mbLocalEcho = ui->check_Echo->isChecked();
}

void AutMainWindow::on_combo_PredefinedDevice_currentIndexChanged(int intIndex)
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

void AutMainWindow::on_btn_PredefinedAdd_clicked()
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

void AutMainWindow::on_btn_PredefinedDelete_clicked()
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

void AutMainWindow::on_btn_SaveDevice_clicked()
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

void AutMainWindow::ContextMenuClosed()
{
    //Right click context menu closed, send message to text edit object
    ui->text_TermEditData->mbContextMenuOpen = false;
    ui->text_TermEditData->update_display();
}

bool AutMainWindow::event(QEvent *evtEvent)
{
    if (evtEvent->type() == QEvent::WindowActivate && transport_isOpen() == true && ui->selector_Tab->currentIndex() == ui->selector_Tab->indexOf(ui->tab_Term))
    {
        //Focus on the terminal
        ui->text_TermEditData->setFocus();
    }
    return QMainWindow::event(evtEvent);
}

void AutMainWindow::SerialPortClosing()
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

#ifndef SKIPPLUGINS
    emit plugin_serial_about_to_close();
#endif
}

void AutMainWindow::on_btn_LogFileSelect_clicked()
{
    //Updates the log file
    QString strLogFilename = QFileDialog::getSaveFileName(this, "Select Log File", ui->edit_LogFile->text(), "Log Files (*.log);;All Files (*.*)");

    if (!strLogFilename.isEmpty())
    {
        //Update log file
        ui->edit_LogFile->setText(strLogFilename);
        on_edit_LogFile_editingFinished();
    }
}

void AutMainWindow::on_edit_LogFile_editingFinished()
{
    //Log filename has changed
    gpTermSettings->setValue("LogFile", ui->edit_LogFile->text());
}

void AutMainWindow::on_check_LogEnable_stateChanged(int state)
{
    //Logging enabled/disabled changed
    gpTermSettings->setValue("LogEnable", (ui->check_LogEnable->isChecked() == true ? 1 : 0));

    ui->edit_LogFile->setEnabled(state != 0);
    ui->check_LogAppend->setEnabled(state != 0);
}

void AutMainWindow::on_check_LogAppend_stateChanged(int)
{
    //Logging append/clearing changed
    gpTermSettings->setValue("LogMode", (ui->check_LogAppend->isChecked() == true ? 1 : 0));
}

void AutMainWindow::on_btn_Help_clicked()
{
    QString strMessage = "Command line options are:-\r\n\r\nPORT=n\r\n    Windows: COM[1..255] specifies a TTY device\r\n    GNU/Linux: /dev/tty[device] specifies a TTY device\r\n    Mac: /dev/[device] specifies a TTY device\r\n\r\nBAUD=n\r\n    [1200..5000000] (limited to 115200 for traditional UARTs)\r\n\r\nSTOP=n\r\n    [1..2]\r\n\r\nDATA=n\r\n    [7..8]\r\n\r\nPAR=n\r\n    [0=None; 1=Odd; 2=Even]\r\n\r\nFLOW=n\r\n    [0=None; 1=Cts/Rts; 2=Xon/Xoff]\r\n\r\nENDCHR=n\r\n    [line termination character :: 0=\\r, 1=\\n, 2=\\r\\n]\r\n\r\nNOCONNECT\r\n    Do not connect to device on startup\r\n\r\nLOCALECHO=n\r\n    [0=Disabled; 1=Enabled]\r\n\r\nLINEMODE=n\r\n    [0=Disabled; 1=Enabled]\r\n\r\nLOG\r\n    Write screen activity to new file '<appname>.log' (Cannot be used with LOG+, LOG+ will take priority)\r\n\r\nLOG+\r\n    Append screen activity to file '<appname>.log' (Cannot be used with LOG, LOG+ will take priority)\r\n\r\nLOG=filename\r\n    File to write the log data to this file (supply extension)\r\n\r\nSHOWCRLF\r\n    When displaying a TX or RX text on screen, show \\t,\\r,\\n as well\r\n\r\nAUTOMATION\r\n    Will initialise and open the automation form\r\n\r\nAUTOMATIONFILE=filename\r\n    Provided that the file exists, it will be loaded into the automation form.\r\n\r\nSCRIPTING\r\n    Will initialise and open the scripting form\r\n\r\nSCRIPTFILE=filename\r\n    Provided that the file exists, it will be opened in the scripting form (SCRIPTING must be provided before this argument)\r\n\r\nSCRIPTACTION=n\r\n    [1=Run script after serial port has been opened] (SCRIPTING and SCRIPTFILE must be provided before this argument)\r\n\r\nTITLE=title\r\n    Will append to the window title (and system tray icon tooltip) the provided text\r\n\r\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\r\n\r\nCharacter escape codes: These are supported in the Automation, Scripting and Speed Test features and allow non-printable ASCII characters to be used. The format of character escape codes is \\HH whereby H represents a hex character (0-9 and A-F), additionally \\r, \\n and \\t can be used to represent a carriage return, new line and tab character individually.\r\nThis function is enabled/disabled in the Automation and Speed Test features by checking the 'Un-escape strings' checkbox to enable it. It cannot be disabled for the Scripting functionality.\r\nFor example: \\00 can be used to represent a null character and \\4C can be used to represent an 'L' ASCII character.\r\n\r\nAdapted from UwTerminalX code, copyright © Laird Connectivity 2015-2022\r\nCopyright © Jamie M. 2023-2024\r\nFor updates and source code licensed under GPLv3, check https://github.com/thedjnK/AuTerm or the 'Update' tab.\r\n\r\nThis program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.\r\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\r\nYou should have received a copy of the GNU General Public License along with this program.  If not, see http://www.gnu.org/licenses/";
    gpmErrorForm->SetMessage(&strMessage);
    gpmErrorForm->show();

#ifndef SKIPSERIALDETECT
    serial_close_dialog_open = false;
#endif
}

void AutMainWindow::on_btn_LogRefresh_clicked()
{
    //Refreshes the log files available for viewing
    ui->combo_LogFile->clear();
    ui->combo_LogFile->addItem("- No file selected -");

    //Apply file filters
    QDir dirLogDir(QString(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).append("/"));
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

void AutMainWindow::on_btn_Licenses_clicked()
{
    //Show license text
    QFile license_file = QFile("://licenses");
    QString license_text;

    license_file.open(QFile::ReadOnly | QFile::Text);
    license_text = license_file.readAll();
    license_file.close();
    gpmErrorForm->SetMessage(&license_text);
    gpmErrorForm->show();

#ifndef SKIPSERIALDETECT
    serial_close_dialog_open = false;
#endif
}

void AutMainWindow::on_btn_EditViewFolder_clicked()
{
    //Open configuration folder
    QDesktopServices::openUrl(QUrl::fromLocalFile(gpTermSettings->fileName().left(gpTermSettings->fileName().lastIndexOf("/"))));
}

void AutMainWindow::on_combo_EditFile_currentIndexChanged(int)
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
        //Allow edits
        ui->text_EditData->setReadOnly(false);

        //Open the log file for reading
        QFile fileLogFile(ui->combo_EditFile->currentIndex() == 1 ? gpTermSettings->fileName() : gpPredefinedDevice->fileName());

        if (fileLogFile.open(QFile::ReadOnly | QFile::Text))
        {
            //Get the contents of the log file
            ui->text_EditData->setPlainText(fileLogFile.readAll());
            fileLogFile.close();
            gbEditFileModified = false;
            giEditFileType = ui->combo_EditFile->currentIndex();

            //Information about the file
            QFileInfo fiFileInfo(fileLogFile.fileName());
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

void AutMainWindow::on_btn_EditSave_clicked()
{
    if (ui->combo_EditFile->currentIndex() != 0)
    {
        //Open the file for writing
        QFile fileLogFile(ui->combo_EditFile->currentIndex() == 1 ? gpTermSettings->fileName() : gpPredefinedDevice->fileName());

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

void AutMainWindow::on_btn_EditLoad_clicked()
{
    if (ui->combo_EditFile->currentIndex() != 0)
    {
        //Reload file data
        on_combo_EditFile_currentIndexChanged(0);
    }
}

#ifndef __APPLE__
void AutMainWindow::on_btn_EditExternal_clicked()
{
    if (ui->combo_EditFile->currentIndex() == 1)
    {
        //Opens the AuTerm configuration file
        QDesktopServices::openUrl(QUrl::fromLocalFile(gpTermSettings->fileName()));
    }
    else if (ui->combo_EditFile->currentIndex() == 2)
    {
        //Opens the predefined devices configuration file
        QDesktopServices::openUrl(QUrl::fromLocalFile(gpPredefinedDevice->fileName()));
    }
}
#endif

void AutMainWindow::LoadSettings()
{
    gpTermSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "AuTerm", "settings"); //Handle to settings
    gpPredefinedDevice = new QSettings(QSettings::IniFormat, QSettings::UserScope, "AuTerm", "devices"); //Handle to predefined devices
    gpErrorMessages = new QSettings(":/error_codes.ini", QSettings::IniFormat); //Handle to error codes

    //Check if error code file is included
    if (!gpErrorMessages->allKeys().isEmpty())
    {
        //Error code file has been loaded
        gbErrorsLoaded = true;
    }

    //Check settings
    if (gpTermSettings->allKeys().isEmpty() || gpTermSettings->value("ConfigVersion").toString() != UwVersion)
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
            gpTermSettings->setValue("LogFile", QString(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).append("/").append(DefaultLogFileName)); //Default log file
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
        if (gpTermSettings->value("ConfigVersion").isNull() || gpTermSettings->value("ConfigVersion").toString() != UwVersion)
        {
            //Update configuration version
            gpTermSettings->setValue("ConfigVersion", UwVersion);
        }
    }
}

void AutMainWindow::UpdateSettings(int intMajor, int intMinor, QChar qcDelta)
{
    Q_UNUSED(intMajor);
    Q_UNUSED(intMinor);
    Q_UNUSED(qcDelta);
}

void AutMainWindow::on_btn_LogViewExternal_clicked()
{
    //View log in external editor
    if (ui->combo_LogFile->currentIndex() >= 1)
    {
        //Open file
        QDesktopServices::openUrl(QUrl::fromLocalFile(QString(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).append("/").append(ui->combo_LogFile->currentText())));
    }
    else
    {
        //Close
        ui->text_LogData->clear();
        ui->label_LogInfo->clear();
    }
}

void AutMainWindow::on_btn_LogViewFolder_clicked()
{
    //Open log folder
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).append("/")));
}

void AutMainWindow::on_text_EditData_textChanged()
{
    //Mark file as edited
    gbEditFileModified = true;
}

void AutMainWindow::on_combo_LogFile_currentIndexChanged(int)
{
    //List item changed - load log file
    ui->text_LogData->clear();
    ui->label_LogInfo->clear();

    if (ui->combo_LogFile->currentIndex() >= 1)
    {
        //Open the log file for reading
        QFile fileLogFile(QString(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).append("/").append(ui->combo_LogFile->currentText()));
        if (fileLogFile.open(QFile::ReadOnly | QFile::Text))
        {
            //Get the contents of the log file
            ui->text_LogData->setPlainText(fileLogFile.readAll().replace('\0', "\\00").replace("\x01", "\\01").replace("\x02", "\\02").replace("\x03", "\\03").replace("\x04", "\\04").replace("\x05", "\\05").replace("\x06", "\\06").replace("\x07", "\\07").replace("\x08", "\\08").replace("\x0b", "\\0B").replace("\x0c", "\\0C").replace("\x0e", "\\0E").replace("\x0f", "\\0F").replace("\x10", "\\10").replace("\x11", "\\11").replace("\x12", "\\12").replace("\x13", "\\13").replace("\x14", "\\14").replace("\x15", "\\15").replace("\x16", "\\16").replace("\x17", "\\17").replace("\x18", "\\18").replace("\x19", "\\19").replace("\x1a", "\\1a").replace("\x1b", "\\1b").replace("\x1c", "\\1c").replace("\x1d", "\\1d").replace("\x1e", "\\1e").replace("\x1f", "\\1f"));
            fileLogFile.close();

            //Information about the log file
            QFileInfo fiFileInfo(fileLogFile.fileName());
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

void AutMainWindow::on_btn_ReloadLog_clicked()
{
    //Reload log
    on_combo_LogFile_currentIndexChanged(ui->combo_LogFile->currentIndex());
}

#ifndef SKIPERRORCODEFORM
void AutMainWindow::on_btn_Error_clicked()
{
    //Open error form dialogue
    if (gecErrorCodeForm == 0)
    {
        //Initialise error code form
        gecErrorCodeForm = new AutErrorCode(nullptr);
        gecErrorCodeForm->SetErrorObject(gpErrorMessages);
    }
    gecErrorCodeForm->show();
}
#endif

#ifndef SKIPSCRIPTINGFORM
void AutMainWindow::ScriptStartRequest()
{
    //Request from scripting form to start running script
    unsigned char chReason = ScriptingReasonOK;

    if (!transport_isOpen())
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

void AutMainWindow::ScriptFinished()
{
    //Script execution has finished
    gbTermBusy = false;
    gbScriptingRunning = false;
    gchTermMode = 0;
}
#endif

#ifndef SKIPSPEEDTEST
void AutMainWindow::on_check_SpeedRTS_stateChanged(int)
{
    //RTS checkbox on speed test page state changed
    ui->check_RTS->setChecked(ui->check_SpeedRTS->isChecked());
}

void AutMainWindow::on_check_SpeedDTR_stateChanged(int)
{
    //DTR checkbox on speed test page state changed
    ui->check_DTR->setChecked(ui->check_SpeedDTR->isChecked());
}

void AutMainWindow::on_btn_SpeedClear_clicked()
{
    //Clear speed test display
    ui->text_SpeedEditData->clear();
}

void AutMainWindow::on_btn_SpeedStartStop_clicked()
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
                if (gchSpeedTestMode == SPEED_MODE_RECEIVE)
                {
                    //Cancel instantly
                    gchSpeedTestMode = SPEED_MODE_INACTIVE;
                }
                gtmrSpeedTestDelayTimer->stop();
            }
            disconnect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStartTimer()));
            disconnect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStopTimer()));
            delete gtmrSpeedTestDelayTimer;
            gtmrSpeedTestDelayTimer = 0;
        }

        if ((gchSpeedTestMode == SPEED_MODE_RECEIVE_TRANSMIT || gchSpeedTestMode == SPEED_MODE_RECEIVE) && (gintSpeedBytesReceived10s > 0 || ui->edit_SpeedBytesRec10s->text().toInt() > 0))
        {
            //Data has been received in the past 10 seconds: start a timer before stopping to catch the extra data packets
            gchSpeedTestMode = SPEED_MODE_RECEIVE;
            gtmrSpeedTestDelayTimer = new QTimer();
            gtmrSpeedTestDelayTimer->setSingleShot(true);
            connect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStopTimer()));
            gtmrSpeedTestDelayTimer->start(5000);

            //Show message that test will end soon
            ui->statusBar->showMessage("Waiting 5 seconds for packets to be received... Click cancel again to stop instantly.");
        }
        else if (gchSpeedTestMode == SPEED_MODE_RECEIVE_TRANSMIT || gchSpeedTestMode == SPEED_MODE_TRANSMIT)
        {
            //Delay for 5 seconds for buffer to clear
            gchSpeedTestMode = SPEED_MODE_INACTIVE;
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
            gchSpeedTestMode = SPEED_MODE_INACTIVE;
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

void AutMainWindow::on_btn_SpeedClose_clicked()
{
    //Close/open port on speed test page pressed
    on_btn_TermClose_clicked();
}

void AutMainWindow::SpeedMenuSelected(QAction *qaAction)
{
    //Speed test menu item selected
    qint8 chItem = qaAction->data().toInt();

    if (transport_isOpen() == true && gbTermBusy == false)
    {
        if (gbLoopbackMode == true)
        {
            QString strMessage = tr("Error: Cannot initiate speed testing as loopback mode is enabled.");
            gpmErrorForm->SetMessage(&strMessage);
            gpmErrorForm->show();
            return;
        }

        //Check size of string if sending data
        if (ui->combo_SpeedDataType->currentIndex() != 0 && !(ui->edit_SpeedTestData->text().length() > 3))
        {
            //Invalid string size
            QString strMessage = tr("Error: Test data string must be a minimum of 4 bytes for speed testing.");
            gpmErrorForm->SetMessage(&strMessage);
            gpmErrorForm->show();
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
                AutEscape::escape_characters(&gbaSpeedMatchData);
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
            gchSpeedTestMode = SPEED_MODE_RECEIVE;
        }
        else if (chItem == SpeedMenuActionSend)
        {
            //Send only test
            gchSpeedTestMode = SPEED_MODE_TRANSMIT;

            //Send data
            SendSpeedTestData(ui->edit_speed_test_chunk_append_size->value());
        }
        else if (chItem == SpeedMenuActionSendRecv || chItem == SpeedMenuActionSendRecv5Delay || chItem == SpeedMenuActionSendRecv10Delay || chItem == SpeedMenuActionSendRecv15Delay)
        {
            //Send and receive test
            gchSpeedTestMode = SPEED_MODE_RECEIVE_TRANSMIT;

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
                SendSpeedTestData(ui->edit_speed_test_chunk_append_size->value());
            }
        }

        if (!ui->check_SpeedSyncReceive->isChecked())
        {
            //Do not synchronise the receive delay when the first data packet is received
            gbSpeedTestReceived = true;
        }

        //Show message in status bar
        ui->statusBar->showMessage(QString((gchSpeedTestMode == SPEED_MODE_RECEIVE_TRANSMIT ? "Send & Receive" : (gchSpeedTestMode == SPEED_MODE_RECEIVE ? "Receive only" : (gchSpeedTestMode == SPEED_MODE_TRANSMIT ? "Send only" : "Unknown")))).append(" Speed testing started."));

        //Start timers
        gtmrSpeedTestStats.start();
        gtmrSpeedTestStats10s.start();
    }
}

void AutMainWindow::OutputSpeedTestStats()
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
    if ((gchSpeedTestMode & SPEED_MODE_RECEIVE) == SPEED_MODE_RECEIVE)
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

void AutMainWindow::on_combo_SpeedDataType_currentIndexChanged(int)
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

void AutMainWindow::on_btn_SpeedCopy_clicked()
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
    AutEscape::escape_characters(&baTmpBA);
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
        append((gchSpeedTestMode == SPEED_MODE_RECEIVE_TRANSMIT ? "Send/Receive" : (gchSpeedTestMode == SPEED_MODE_TRANSMIT ? "Send" : (gchSpeedTestMode == SPEED_MODE_RECEIVE ? "Receive" : "Inactive")))).
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

void AutMainWindow::SendSpeedTestData(int intMaxLength)
{
    //Send string out. It's OK to send less than the maximum length but not more, unless none fit
    int intSendTimes = 1;

    if (intMaxLength > gintSpeedTestMatchDataLength)
    {
        intSendTimes = (intMaxLength / gintSpeedTestMatchDataLength);
    }

    if (ui->check_SpeedShowTX->isChecked())
    {
        //Show TX data in terminal
        int print_times = intSendTimes;

        while (print_times > 0)
        {
            //Append to buffer
            gbaSpeedDisplayBuffer.append(gbaSpeedMatchData);
            --print_times;
        }

        if (!gtmrSpeedUpdateTimer.isActive())
        {
            gtmrSpeedUpdateTimer.start();
        }
    }

    while (intSendTimes > 0)
    {
        //Send out until finished
        transport_write(gbaSpeedMatchData);
        gintSpeedBufferCount += gintSpeedTestMatchDataLength;
        --intSendTimes;
        ++gintSpeedTestStatPacketsSent;
    }
}

void AutMainWindow::SpeedTestBytesWritten(qint64 intByteCount)
{
    //Serial port bytes have been written in speed test mode
    if ((gchSpeedTestMode & SPEED_MODE_TRANSMIT) == SPEED_MODE_TRANSMIT)
    {
        //Sending data in speed test
        gintSpeedBufferCount -= intByteCount;
        if (gintSpeedBufferCount <= ui->edit_speed_test_minimum_buffer_size->value())
        {
            //Buffer has space: send more data
            SendSpeedTestData(ui->edit_speed_test_chunk_append_size->value());
        }
    }

    //Add to bytes sent counters
    gintSpeedBytesSent += intByteCount;
    gintSpeedBytesSent10s += intByteCount;
}

void AutMainWindow::SpeedTestReceive()
{
    //Receieved data from serial port in speed test mode
    if ((gchSpeedTestMode & SPEED_MODE_RECEIVE) == SPEED_MODE_RECEIVE)
    {
        //Check data as in receieve mode
        uint64_t received_bytes = transport_bytesAvailable();
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
            gbaSpeedDisplayBuffer.append(transport_peek(received_bytes));
            if (!gtmrSpeedUpdateTimer.isActive())
            {
                gtmrSpeedUpdateTimer.start();
            }
        }

        if (ui->combo_SpeedDataType->currentIndex() != 0)
        {
            //Test data is OK
            int32_t remove_size = 0;
            gbaSpeedReceivedData.append(transport_read(received_bytes));
            while (remove_size < gbaSpeedReceivedData.length())
            {
                //Data to check
                int SizeToTest = gintSpeedTestMatchDataLength - gintSpeedTestReceiveIndex;
                if ((SizeToTest + remove_size) > gbaSpeedReceivedData.length())
                {
                    SizeToTest = gbaSpeedReceivedData.length() - remove_size;
                }

                //Optimised search check function. for testing only
                int32_t i = 0;
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
                        int32_t max_size = strFirst.length() > strSecond.length() ? strSecond.length() : strFirst.length();
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

void AutMainWindow::UpdateSpeedTestValues()
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

    if ((gchSpeedTestMode & SPEED_MODE_RECEIVE) == SPEED_MODE_RECEIVE)
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

void AutMainWindow::SpeedTestStartTimer()
{
    //Timer expired, begin sending speed test data
    disconnect(gtmrSpeedTestDelayTimer, SIGNAL(timeout()), this, SLOT(SpeedTestStartTimer()));
    delete gtmrSpeedTestDelayTimer;
    gtmrSpeedTestDelayTimer = 0;
    SendSpeedTestData(ui->edit_speed_test_chunk_append_size->value());
}

void AutMainWindow::SpeedTestStopTimer()
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
    gchSpeedTestMode = SPEED_MODE_INACTIVE;
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

void AutMainWindow::OutputSpeedTestAvgStats(qint64 lngElapsed)
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

    if ((gchSpeedTestMode & SPEED_MODE_RECEIVE) == SPEED_MODE_RECEIVE)
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

void AutMainWindow::update_displayText()
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

void AutMainWindow::on_combo_SpeedDataDisplay_currentIndexChanged(int)
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

quint64 AutMainWindow::BitsBytesConvert(quint64 iCount, BitByteTypes bbtFrom, BitByteTypes bbtTo)
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

void AutMainWindow::SetLoopBackMode(bool bNewMode)
{
    //Enables or disables loopback mode
    if (gbLoopbackMode != bNewMode)
    {
        //Change loopback mode
        gbLoopbackMode = bNewMode;
        if (gbLoopbackMode == true)
        {
            //Enabled
            update_buffer("\n[Loopback Enabled]\n", false);
            gpMenu->actions().at(MenuActionLoopback)->setText("Disable Loopback (Rx->Tx)");
        }
        else
        {
            //Disabled
            update_buffer("\n[Loopback Disabled]\n", false);
            gpMenu->actions().at(MenuActionLoopback)->setText("Enable Loopback (Rx->Tx)");
        }

        if (!gtmrTextUpdateTimer.isActive())
        {
            gtmrTextUpdateTimer.start();
        }
    }
}

#ifndef SKIPONLINE
void AutMainWindow::AuTermUpdateCheck()
{
    //Send request to check for AuTerm updates
    gbTermBusy = true;
    gchTermMode = mode_check_for_update;
    ui->btn_Cancel->setEnabled(true);
    gnmrReply = gnmManager->get(QNetworkRequest(QUrl("https://raw.githubusercontent.com/thedjnK/AuTerm/main/version.txt")));
    ui->statusBar->showMessage("Checking for AuTerm updates...");
}
#endif

void AutMainWindow::ScriptingFileSelected(const QString *strFilepath)
{
    QString strDirectory = SplitFilePath(*strFilepath).at(0);
    if (gpTermSettings->value("LastScriptFileDirectory").toString() != strDirectory)
    {
        //Update scripting directory
        gpTermSettings->setValue("LastScriptFileDirectory", strDirectory);
    }
}

void AutMainWindow::UpdateCustomisation(bool bDefault)
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

void AutMainWindow::on_check_EnableTerminalSizeSaving_stateChanged(int)
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

void AutMainWindow::resizeEvent(QResizeEvent *)
{
    if (ui->check_EnableTerminalSizeSaving->isChecked() == true && gbAppStarted == true)
    {
        gpTermSettings->setValue("WindowWidth", this->width());
        gpTermSettings->setValue("WindowHeight", this->height());
    }
}

void AutMainWindow::on_edit_Title_textEdited(const QString &)
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
        if (transport_isOpen())
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

#ifndef SKIPPLUGINS
void AutMainWindow::on_btn_Plugin_Abort_clicked()
{
    if (ui->list_Plugin_Plugins->currentRow() >= 0)
    {
        gpmErrorForm->show_message(plugin_list.at(ui->list_Plugin_Plugins->currentRow()).plugin->plugin_about());

#ifndef SKIPSERIALDETECT
        serial_close_dialog_open = false;
#endif
    }
}

void AutMainWindow::on_btn_Plugin_Config_clicked()
{
    if (ui->list_Plugin_Plugins->currentRow() >= 0 && plugin_list.at(ui->list_Plugin_Plugins->currentRow()).plugin->plugin_configuration() == false)
    {
        gpmErrorForm->show_message("This plugin does not have any configuration.");

#ifndef SKIPSERIALDETECT
        serial_close_dialog_open = false;
#endif
    }
}

void AutMainWindow::plugin_set_status(bool busy, bool hide_terminal_output, bool *accepted)
{
    *accepted = false;

    if (gbPluginRunning == true && busy == true)
    {
        qDebug() << "A plugin tried to run whilst another was busy";
        return;
    }

    if (busy == false)
    {
        if (this->sender() != plugin_status_owner)
        {
            qDebug() << "A plugin tried to declare it was no longer busy when another plugin had locked this";
            return;
        }

        gbPluginRunning = false;
        gbPluginHideTerminalOutput = false;
        plugin_status_owner = nullptr;
    }
    else
    {
        if (transport_isOpen() == false)
        {
            qDebug() << "A plugin tried to run whilst the UART was closed";
            return;
        }

        gbPluginRunning = busy;
        gbPluginHideTerminalOutput = hide_terminal_output;
        plugin_status_owner = this->sender();
    }

    *accepted = true;
}

void AutMainWindow::find_plugin(QString name, plugin_data *plugin)
{
    uint16_t i = 0;
    uint16_t l = plugin_list.length();

#ifdef QT_STATIC
    QVector<QStaticPlugin> static_plugins = QPluginLoader::staticPlugins();
#endif

    while (i < l)
    {
        if (name ==
#ifdef QT_STATIC
            static_plugins.at(i).metaData().value("MetaData").toObject().value("Name").toString()
#else
            plugin_list[i].plugin_loader->metaData().value("MetaData").toObject().value("Name").toString()
#endif
        )
        {
            plugin->object = plugin_list[i].object;
            plugin->found = true;
            return;
        }

        ++i;
    }

    plugin->found = false;
    return;
}

void AutMainWindow::plugin_serial_transmit(QByteArray *data)
{
//    qDebug() << "Transmitted";
    if (gbPluginRunning == true)
    {
        transport_write(*data);
        gintQueuedTXBytes += data->size();

//TODO: Add to log
        gpMainLog->WriteLogData(*data);

        if (gbPluginHideTerminalOutput == false && ui->check_Echo->isChecked())
        {
            update_buffer(data, false);
        }
    }
}

void AutMainWindow::plugin_add_open_close_button(QPushButton *button)
{
    list_plugin_open_close_buttons.append(button);
    connect(button, SIGNAL(clicked(bool)), this, SLOT(on_btn_TermClose_clicked()));
    button->setText((transport_isOpen() == true || transport_isOpening() == true) ? "C&lose Port" : "&Open Port");
}

void AutMainWindow::plugin_serial_open_close(uint8_t mode)
{
    switch (mode) {
        case 0:
        {
            //Open port
            if (transport_isOpen() == true || transport_isOpening() == true)
            {
                return;
            }
            break;
        }

        case 1:
        {
            //Close port
            if (transport_isOpen() == false && transport_isOpening() == false)
            {
                return;
            }
            break;
        }

        default:
        {
            //Open port if closed, close port if open
            break;
        }
    };

    on_btn_TermClose_clicked(true);
}

void AutMainWindow::plugin_serial_is_open(bool *open)
{
    *open = transport_isOpen();
}

void AutMainWindow::plugin_to_hex(QByteArray *data)
{
    AutEscape::to_hex(data);
}

void AutMainWindow::plugin_save_setting(QString name, QVariant data)
{
    name.prepend("plugin_");

    gpTermSettings->setValue(name, data);
}

void AutMainWindow::plugin_load_setting(QString name, QVariant *data, bool *found)
{
    name.prepend("plugin_");

    if (found != nullptr)
    {
        *found = gpTermSettings->contains(name);
    }

    if (data != nullptr)
    {
        *data = gpTermSettings->value(name);
    }
}
#endif

void AutMainWindow::plugin_get_image_pixmap(QString name, QPixmap **pixmap)
{
    if (name == "EmptyCircle")
    {
        *pixmap = gpEmptyCirclePixmap;
    }
    else if (name == "RedCircle")
    {
        *pixmap = gpRedCirclePixmap;
    }
    else if (name == "GreenCircle")
    {
        *pixmap = gpGreenCirclePixmap;
    }
    else if (name == "AuTerm16")
    {
        *pixmap = gpUw16Pixmap;
    }
    else
    {
        *pixmap = nullptr;
    }
}

void AutMainWindow::on_radio_vt100_ignore_toggled(bool checked)
{
    if (checked == true)
    {
        ui->text_TermEditData->set_vt100_mode(VT100_MODE_IGNORE);
    }
}

void AutMainWindow::on_radio_vt100_strip_toggled(bool checked)
{
    if (checked == true)
    {
        ui->text_TermEditData->set_vt100_mode(VT100_MODE_STRIP);
    }
}

void AutMainWindow::on_radio_vt100_decode_toggled(bool checked)
{
    if (checked == true)
    {
        ui->text_TermEditData->set_vt100_mode(VT100_MODE_DECODE);
    }
}

#ifndef SKIPPLUGINS
void AutMainWindow::on_list_Plugin_Plugins_itemDoubleClicked(QListWidgetItem *)
{
    on_btn_Plugin_Config_clicked();
}
#endif

#ifndef SKIPONLINE
bool AutMainWindow::is_newer(const QString *new_version, const QString *current_version)
{
    uint8_t i = 0;
    uint8_t l = new_version->length();
    bool match = true;

    if (current_version->length() < l)
    {
        l = current_version->length();
    }

    while (i < l)
    {
        if (new_version->at(i) > current_version->at(i))
        {
            return true;
        }
        else if (new_version->at(i) != current_version->at(i))
        {
            match = false;
            break;
        }

        ++i;
    }

    if (match == true && new_version->length() > current_version->length() && ((new_version->at(i) >= 'a' && new_version->at(i) <= 'z') || (new_version->at(i) >= '0' && new_version->at(i) <= '9')))
    {
        return true;
    }

    return false;
}

void AutMainWindow::on_check_enable_online_version_check_toggled(bool checked)
{
    if (gbAppStarted == true)
    {
        gpTermSettings->setValue("UpdateCheck", checked);
    }
}
#endif

void AutMainWindow::on_selector_Tab_currentChanged(int index)
{
    if (index == ui->selector_Tab->indexOf(ui->tab_Term))
    {
        if (display_update_pending == true)
        {
            UpdateReceiveText();
        }
    }
    else if (gtmrTextUpdateTimer.isActive())
    {
        gtmrTextUpdateTimer.stop();
        display_update_pending = true;
    }
}

void AutMainWindow::update_buffer(QByteArray data, bool apply_formatting)
{
    update_buffer(&data, apply_formatting);
}

void AutMainWindow::update_buffer(QByteArray *data, bool apply_formatting)
{
    if (display_buffers.length() > 0 && display_buffers.last().apply_formatting == apply_formatting)
    {
        display_buffers.last().data.append(*data);
    }
    else
    {
        display_buffer_struct temp;
        temp.data = *data;
        temp.apply_formatting = apply_formatting;
        display_buffers.append(temp);
    }

    if (!gtmrTextUpdateTimer.isActive())
    {
        gtmrTextUpdateTimer.start();
    }
}

void AutMainWindow::on_check_trim_toggled(bool checked)
{
    if (gbAppStarted == true)
    {
        gpTermSettings->setValue("AutoTrimDBuffer", checked);
        update_display_trimming();
    }

    ui->spin_trim_threshold->setEnabled(checked);
    ui->spin_trim_size->setEnabled(checked);
}

void AutMainWindow::on_spin_trim_threshold_editingFinished()
{
    if (gbAppStarted == true)
    {
        gpTermSettings->setValue("AutoTrimDBufferThreshold", ui->spin_trim_threshold->value());
        update_display_trimming();
    }
}

void AutMainWindow::on_spin_trim_size_editingFinished()
{
    if (gbAppStarted == true)
    {
        gpTermSettings->setValue("AutoTrimDBufferSize", ui->spin_trim_size->value());
        update_display_trimming();
    }

    //Do not allow threshold value to be lower than size value
    ui->spin_trim_threshold->setMinimum(ui->spin_trim_size->value() > DefaultAutoTrimDBufferThreshold ? ui->spin_trim_size->value() : DefaultAutoTrimDBufferThreshold);
}

void AutMainWindow::update_display_trimming()
{
    //Setup display buffer trimming
    if (ui->check_trim->isChecked() == true)
    {
        ui->text_TermEditData->set_trim_settings(ui->spin_trim_threshold->value(), ui->spin_trim_size->value());
    }
    else
    {
        ui->text_TermEditData->set_trim_settings(0, 0);
    }
}

#ifndef SKIPSERIALDETECT
void AutMainWindow::serial_port_reconnected(QString port)
{
    //If the last message shown was the serial close dialog, close it upon reconnect
    if (serial_close_dialog_open == true)
    {
        if (gpmErrorForm->isVisible())
        {
            gpmErrorForm->close();
        }
        serial_close_dialog_open = false;
    }
    serial_detect_waiting = false;
    ui->btn_Cancel->setEnabled(false);
    ui->combo_COM->setCurrentText(port);
    on_combo_COM_currentIndexChanged(0);
    OpenDevice();
}

void AutMainWindow::on_check_reconnect_after_disconnect_toggled(bool checked)
{
    if (gbAppStarted == true)
    {
        gpTermSettings->setValue("ReconnectAfterDisconnect", checked);

        if (checked == true && serial_detect == nullptr)
        {
            serial_detect = new AutSerialDetect(this);
            connect(serial_detect, SIGNAL(port_reconnected(QString)), this, SLOT(serial_port_reconnected(QString)));
        }
        else if (checked == false && serial_detect != nullptr)
        {
            //Clean up active state (if waiting) and disable cancel button, then free up memory
            if (serial_detect_waiting == true)
            {
                serial_detect->stop();
                serial_detect_waiting = false;
                serial_close_dialog_open = false;
                ui->btn_Cancel->setEnabled(false);
            }

            disconnect(this, SLOT(serial_port_reconnected(QString)));
            delete serial_detect;
            serial_detect = nullptr;
        }
    }
}
#endif

#ifndef SKIPPLUGINS_TRANSPORT
bool AutMainWindow::transport_open(QIODeviceBase::OpenMode mode)
{
    //TODO:
    if (ui->tab_transport->currentIndex() == 0)
    {
        return gspSerialPort.open(mode);
    }

    return plugin_active_transport->open(mode);
}

void AutMainWindow::transport_close()
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.close();
    }

    plugin_active_transport->close();
    plugin_active_transport = nullptr;
}

bool AutMainWindow::transport_isOpen() const
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.isOpen();
    }

    return plugin_active_transport->isOpen();
}

bool AutMainWindow::transport_isOpening() const
{
    if (plugin_active_transport == nullptr)
    {
        return false;
    }

    return plugin_active_transport->isOpening();
}

QSerialPort::DataBits AutMainWindow::transport_dataBits() const
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.dataBits();
    }

    return plugin_active_transport->dataBits();
}

AutTransportPlugin::StopBits AutMainWindow::transport_stopBits() const
{
    if (plugin_active_transport == nullptr)
    {
        return (AutTransportPlugin::StopBits)gspSerialPort.stopBits();
    }

    return plugin_active_transport->stopBits();
}

QSerialPort::Parity AutMainWindow::transport_parity() const
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.parity();
    }

    return plugin_active_transport->parity();
}

qint64 AutMainWindow::transport_write(const QByteArray &data)
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.write(data);
    }

    return plugin_active_transport->write(data);
}

qint64 AutMainWindow::transport_bytesAvailable() const
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.bytesAvailable();
    }

    return plugin_active_transport->bytesAvailable();
}

QByteArray AutMainWindow::transport_peek(qint64 maxlen)
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.peek(maxlen);
    }

    return plugin_active_transport->peek(maxlen);
}

QByteArray AutMainWindow::transport_read(qint64 maxlen)
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.read(maxlen);
    }

    return plugin_active_transport->read(maxlen);
}

QByteArray AutMainWindow::transport_readAll()
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.readAll();
    }

    return plugin_active_transport->readAll();
}

bool AutMainWindow::transport_clear(QSerialPort::Directions directions)
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.clear(directions);
    }

    return plugin_active_transport->clear(directions);
}

bool AutMainWindow::transport_setBreakEnabled(bool set)
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.setBreakEnabled(set);
    }

    return plugin_active_transport->setBreakEnabled(set);
}

bool AutMainWindow::transport_setRequestToSend(bool set)
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.setRequestToSend(set);
    }

    return plugin_active_transport->setRequestToSend(set);
}

bool AutMainWindow::transport_setDataTerminalReady(bool set)
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.setDataTerminalReady(set);
    }

    return plugin_active_transport->setDataTerminalReady(set);
}

QSerialPort::PinoutSignals AutMainWindow::transport_pinoutSignals()
{
    if (plugin_active_transport == nullptr)
    {
        return gspSerialPort.pinoutSignals();
    }

    return plugin_active_transport->pinoutSignals();
}

QString AutMainWindow::transport_error_to_error_string(int error)
{
    if (plugin_active_transport == nullptr)
    {
//        return gspSerialPort.er
    }

    return plugin_active_transport->to_error_string(error);
}

QString AutMainWindow::transport_name() const
{
    if (plugin_active_transport == nullptr)
    {
        return "UART";
    }

    return plugin_active_transport->transport_name();
}

bool AutMainWindow::transport_supports_break() const
{
    if (plugin_active_transport == nullptr)
    {
        return true;
    }

    return plugin_active_transport->supports_break();
}

bool AutMainWindow::transport_supports_request_to_send() const
{
    if (plugin_active_transport == nullptr)
    {
        return true;
    }

    return plugin_active_transport->supports_request_to_send();
}

bool AutMainWindow::transport_supports_data_terminal_ready() const
{
    if (plugin_active_transport == nullptr)
    {
        return true;
    }

    return plugin_active_transport->supports_data_terminal_ready();
}

void AutMainWindow::plugin_transport_error(int error)
{
    //TODO: unify with SerialError() and make a common clean up function
#ifndef SKIPSCRIPTINGFORM
    if (gbScriptingRunning == true)
    {
//        gusScriptingForm->SerialPortError(speErrorCode);
    }
#endif

    //Resource error or permission error (device unplugged?)
    QString strMessage = "Fatal error with transport connection: " % QString::number(error) % ".\nPlease reconnect to the device to continue.";
    gpmErrorForm->SetMessage(&strMessage);
    gpmErrorForm->show();
    ui->text_TermEditData->set_serial_open(false);

        if (gbStreamingFile == true)
        {
            //Clear up file stream
            gtmrStreamTimer.invalidate();
            gbStreamingFile = false;
            gpStreamFileHandle->close();
            delete gpStreamFileHandle;
        }
#ifndef SKIPSPEEDTEST
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
            gchSpeedTestMode = SPEED_MODE_INACTIVE;
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

#ifndef SKIPSERIALDETECT
        if (serial_detect_waiting == false)
        {
#endif
            //Disable cancel button
            ui->btn_Cancel->setEnabled(false);
#ifndef SKIPSERIALDETECT
        }
#endif

        //Disable active checkboxes
        ui->check_Break->setEnabled(false);
        ui->check_DTR->setEnabled(false);
        ui->check_Echo->setEnabled(false);
        ui->check_Line->setEnabled(false);
        ui->check_RTS->setEnabled(false);
#ifndef SKIPSPEEDTEST
        ui->check_SpeedDTR->setEnabled(false);
        ui->check_SpeedRTS->setEnabled(false);
#endif

        //Disable text entry
        ui->text_TermEditData->setReadOnly(true);

#ifndef SKIPSERIALDETECT
        if (serial_detect_waiting == false)
        {
#endif
            //Change status message
            ui->statusBar->showMessage("");
#ifndef SKIPSERIALDETECT
        }
#endif

        //Change button text
        ui->btn_TermClose->setText("&Open Port");
#ifndef SKIPSPEEDTEST
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

#ifndef SKIPPLUGINS
//    emit plugin_serial_error(speErrorCode);

//    if (port_closed == true)
    {
        emit plugin_serial_closed();
        gbPluginHideTerminalOutput = false;
        gbPluginRunning = false;
    }
#endif

    plugin_active_transport = nullptr;
}

void AutMainWindow::plugin_force_image_update()
{
    SerialStatus(true);

    if (gpSignalTimer->isActive() == false)
    {
        gpSignalTimer->start(gpTermSettings->value("SerialSignalCheckInterval", DefaultSerialSignalCheckInterval).toUInt());
    }
}

void AutMainWindow::plugin_show_message_box(QString message)
{
    gpmErrorForm->SetMessage(&message);
    gpmErrorForm->show();
}

#ifdef QT_STATIC
AutPlugin::PluginType AutMainWindow::plugin_type(QStaticPlugin *plugin)
#else
AutPlugin::PluginType AutMainWindow::plugin_type(QPluginLoader *plugin)
#endif
{
    QJsonObject plugin_metadata = plugin->metaData().value("MetaData").toObject();

    if (plugin_metadata.contains("Type") == true)
    {
        QString plugin_type = plugin_metadata.value("Type").toString();

        if (plugin_type == "feature")
        {
            return AutPlugin::Feature;
        }

        if (plugin_type == "transport")
        {
            return AutPlugin::Transport;
        }
    }

    return AutPlugin::Unknown;
}

bool AutMainWindow::plugin_type_supported(AutPlugin::PluginType type)
{
    switch (type)
    {
        case AutPlugin::Feature:
#ifndef SKIPPLUGINS_TRANSPORT
        case AutPlugin::Transport:
#endif
            return true;
        default:
            return false;
    };
}
#endif

/******************************************************************************/
// END OF FILE
/******************************************************************************/
