/******************************************************************************
** Copyright (C) 2015-2022 Laird Connectivity
** Copyright (C) 2023-2024 Jamie M.
**
** Project: AuTerm
**
** Module: AutMainWindow.h
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
#ifndef AUTMAINWINDOW_H
#define AUTMAINWINDOW_H

#define AUTERM_APPLICATION

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMenu>
#include <QClipboard>
#include <QMimeData>
#include <QFileDialog>
#include <QScrollBar>
#include <QProcess>
#include <QTimer>
#include <QRegularExpression>
#include <QStringList>
#include <QMessageBox>
#include <QFile>
#include <QColorDialog>
#include <QKeyEvent>
#include <QFontDialog>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTextDocumentFragment>
#include <QTime>
#include <QDate>
#include <QElapsedTimer>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QStringView>
#include <QListWidgetItem>
//Need cmath for std::ceil function
#include <cmath>
#include <QStandardPaths>
#include "AutScrollEdit.h"
#include "AutPopup.h"
#include "AutLogger.h"
#ifndef SKIPAUTOMATIONFORM
#include "AutAutomation.h"
#endif
#ifndef SKIPERRORCODEFORM
#include "AutErrorCode.h"
#endif
#ifndef SKIPSCRIPTINGFORM
#include "AutScripting.h"
#endif
#include "AutEscape.h"
#ifndef SKIPPLUGINS
#include <QPluginLoader>
#include "AutPlugin.h"
#endif
#ifndef SKIPONLINE
#include <QNetworkReply>
#ifndef QT_NO_SSL
#include <QSslSocket>
#endif
#endif
#ifndef SKIPSERIALDETECT
#ifdef _WIN32
#include "AutSerialDetect_windows.h"
#else
//Assume linux
#include "AutSerialDetect_linux.h"
#endif
#endif

/******************************************************************************/
// Defines
/******************************************************************************/
//Decides if generic data types will be 32 or 64-bit
#if _WIN64 || __aarch64__ || TARGET_OS_MAC || __x86_64__
    //64-bit OS
    #define OS32_64UINT quint64
#else
    //32-bit or other OS
    #define OS32_64UINT quint32
#endif

#define WINDOWS_NEWLINE_SIZE                      2
#define NEWLINE_SIZE                              1
#define NEWLINE_LINES                             1

/******************************************************************************/
// Constants
/******************************************************************************/
//Constants for version and functions
const QString UwVersion                         = "0.33a"; //Version string
//Constants for timeouts and streaming
const qint16 FileReadBlock                      = 512;     //Number of bytes to read per block when streaming files
const qint16 StreamProgress                     = 10000;   //Number of bytes between streaming progress updates
//Constants for default config values
const QString DefaultLogFileName                = "AuTerm.log";
const bool DefaultLogMode                       = 0;
const bool DefaultLogEnable                     = 0;
const bool DefaultSysTrayIcon                   = 1;
const qint16 DefaultSerialSignalCheckInterval   = 50;
const qint16 DefaultTextUpdateInterval          = 80;
const bool DefaultAutoDTrimBuffer               = false;
const quint32 DefaultAutoTrimDBufferThreshold   = 512;
const quint32 DefaultAutoTrimDBufferSize        = 256;
const quint16 DefaultScrollbackBufferSize       = 32;    //(Unlisted option)
const bool DefaultSaveSize                      = false;
const bool DefaultOnlineUpdateCheck             = true;
const bool DefaultReconnectAfterDisconnect      = false;
//Constants for URLs
const QString URLLinuxNonRootSetup = "https://github.com/LairdCP/AuTerm/wiki/Granting-non-root-USB-device-access-(Linux)";
const qint8 FilenameIndexScripting              = 0;
const qint8 FilenameIndexOthers                 = 1;
//Constants for balloon (notification area) icon options
const qint8 BalloonActionShow                   = 1;
const qint8 BalloonActionExit                   = 2;
//Constants for speed testing
const qint16 SpeedTestStatUpdateTime            = 500;  //Time (in ms) between status updates for speed test mode
const QString WINDOWS_NEWLINE                   = "\r\n";
const QChar NEWLINE                             = '\n';

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class AutMainWindow;
}

//Union used for checking received byte array contents whilst speed testing
union pointer_buf {
    uint8_t *p8;
    uint16_t *p16;
    uint32_t *p32;
    uint64_t *p64;
};

#ifndef SKIPPLUGINS
//Struct used for holding plugin objects
struct plugins {
    QObject *object;
    AutPlugin *plugin;
#ifndef QT_STATIC
    QPluginLoader *plugin_loader;
    QString filename;
#endif
};
#endif

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
//Enum used for specifying type of data
enum class BitByteTypes
{
    TypeBytes,
    TypeDataBits,
    TypeAllBits
};

enum modes {
    mode_idle = 0,
    mode_check_for_update,
};

//Right-click menu options
enum menu_actions {
    MenuActionError                             = 0,
    MenuActionLoopback,
    MenuActionStreamFile,
    MenuActionFont,
    MenuActionTextColour,
    MenuActionBackground,
    MenuActionRestoreDefaults,
    MenuActionAutomation,
    MenuActionScripting,
    MenuActionClearDisplay,
    MenuActionClearRxTx,
    MenuActionCopy,
    MenuActionCopyAll,
    MenuActionPaste,
    MenuActionSelectAll
};

//Speed test menu
enum speed_menu_actions {
    SpeedMenuActionRecv                         = 0,
    SpeedMenuActionSend,
    SpeedMenuActionSendRecv,
    SpeedMenuActionSendRecv5Delay,
    SpeedMenuActionSendRecv10Delay,
    SpeedMenuActionSendRecv15Delay
};

enum speed_modes {
    SPEED_MODE_INACTIVE,
    SPEED_MODE_RECEIVE,
    SPEED_MODE_TRANSMIT,
    SPEED_MODE_RECEIVE_TRANSMIT,
};

/******************************************************************************/
// Class definitions
/******************************************************************************/
class AutMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AutMainWindow(QWidget *parent = 0);
    ~AutMainWindow();

public slots:
    void SerialRead();
    void MenuSelected(QAction* qaAction);
    void balloontriggered(QAction* qaAction);
    void SerialStatusSlot();
    void SerialError(QSerialPort::SerialPortError speErrorCode);
    void enter_pressed();
    void key_pressed(int nKey, QChar chrKeyValue);
    void vt100_send(QByteArray code);
    void SerialBytesWritten(qint64 intByteCount);
    void SerialPortClosing();
    void MessagePass(QByteArray baDataString, bool bEscapeString, bool bFromScripting);
#ifndef SKIPSPEEDTEST
    void SpeedMenuSelected(QAction* qaAction);
    quint64 BitsBytesConvert(quint64 iCount, BitByteTypes bbtFrom, BitByteTypes bbtTo);
#endif

private slots:
    void on_btn_Connect_clicked();
    void on_btn_TermClose_clicked(bool from_plugin = false);
    void on_btn_Refresh_clicked();
    void on_btn_TermClear_clicked();
    void on_btn_Duplicate_clicked();
    void on_text_TermEditData_customContextMenuRequested(const QPoint &pos);
    void on_check_Break_stateChanged();
    void on_check_RTS_stateChanged();
    void on_check_DTR_stateChanged();
    void on_check_Line_stateChanged();
    void closeEvent(QCloseEvent *closeEvent);
    void on_btn_Cancel_clicked();
    void UpdateReceiveText();
    void on_combo_COM_currentIndexChanged(int intIndex);
#ifndef SKIPONLINE
    void replyFinished(QNetworkReply* nrReply);
#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply *nrReply, QList<QSslError> lstSSLErrors);
#endif
#endif
    void on_btn_Github_clicked();
    QList<QString> SplitFilePath(QString strFilename);
    void on_check_Echo_stateChanged(int bChecked);
    void on_combo_PredefinedDevice_currentIndexChanged(int intIndex);
    void on_btn_PredefinedAdd_clicked();
    void on_btn_PredefinedDelete_clicked();
    void on_btn_SaveDevice_clicked();
    void ContextMenuClosed();
    bool event(QEvent *evtEvent);
    void on_btn_LogFileSelect_clicked();
    void on_edit_LogFile_editingFinished();
    void on_check_LogEnable_stateChanged(int state);
    void on_check_LogAppend_stateChanged(int intChecked);
    void on_btn_Help_clicked();
    void on_btn_LogRefresh_clicked();
    void on_btn_Licenses_clicked();
    void on_btn_EditViewFolder_clicked();
    void on_combo_EditFile_currentIndexChanged(int);
    void on_btn_EditSave_clicked();
    void on_btn_EditLoad_clicked();
#ifndef __APPLE__
    void on_btn_EditExternal_clicked();
#endif
    void on_btn_LogViewExternal_clicked();
    void on_btn_LogViewFolder_clicked();
    void on_text_EditData_textChanged();
    void on_combo_LogFile_currentIndexChanged(int);
    void on_btn_ReloadLog_clicked();
#ifndef SKIPERRORCODEFORM
    void on_btn_Error_clicked();
#endif
#ifndef SKIPSCRIPTINGFORM
    void ScriptStartRequest();
    void ScriptFinished();
#endif
#ifndef SKIPSPEEDTEST
    void on_check_SpeedRTS_stateChanged(int);
    void on_check_SpeedDTR_stateChanged(int);
    void on_btn_SpeedClear_clicked();
    void on_btn_SpeedClose_clicked();
    void on_btn_SpeedStartStop_clicked();
    void OutputSpeedTestStats();
    void on_combo_SpeedDataType_currentIndexChanged(int);
    void on_btn_SpeedCopy_clicked();
    void UpdateSpeedTestValues();
    void SpeedTestStartTimer();
    void SpeedTestStopTimer();
    void on_combo_SpeedDataDisplay_currentIndexChanged(int);
    void update_displayText();
#endif
    void ScriptingFileSelected(const QString *strFilepath);
    void on_check_EnableTerminalSizeSaving_stateChanged(int);
    void resizeEvent(QResizeEvent *);
    void on_edit_Title_textEdited(const QString &);
#ifndef SKIPPLUGINS
    void on_btn_Plugin_Abort_clicked();
    void on_btn_Plugin_Config_clicked();
#endif
    void on_radio_vt100_ignore_toggled(bool checked);
    void on_radio_vt100_strip_toggled(bool checked);
    void on_radio_vt100_decode_toggled(bool checked);
#ifndef SKIPONLINE
    void on_check_enable_online_version_check_toggled(bool checked);
#endif
    void on_selector_Tab_currentChanged(int index);
    void on_check_trim_toggled(bool checked);
    void on_spin_trim_threshold_editingFinished();
    void on_spin_trim_size_editingFinished();
#ifndef SKIPSERIALDETECT
    void serial_port_reconnected(QString port);
    void on_check_reconnect_after_disconnect_toggled(bool checked);
#endif

#ifndef SKIPPLUGINS
    void on_list_Plugin_Plugins_itemDoubleClicked(QListWidgetItem *);

signals:
    void plugin_serial_receive(QByteArray *data);
    void plugin_serial_error(QSerialPort::SerialPortError speErrorCode);
    void plugin_serial_bytes_written(qint64 intByteCount);
    void plugin_serial_about_to_close();
    void plugin_serial_opened();
    void plugin_serial_closed();

public slots:
    void plugin_set_status(bool busy, bool hide_terminal_output, bool *accepted);
    void find_plugin(QString name, plugin_data *plugin);
    void plugin_serial_transmit(QByteArray *data);
    void plugin_add_open_close_button(QPushButton *button);
    void plugin_serial_open_close(uint8_t mode);
    void plugin_serial_is_open(bool *open);
    void plugin_to_hex(QByteArray *data);
    void plugin_save_setting(QString name, QVariant data);
    void plugin_load_setting(QString name, QVariant *data, bool *found);
    void plugin_get_image_pixmap(QString name, QPixmap **pixmap);
    void plugin_force_image_update();
    void plugin_show_message_box(QString message);
    void plugin_transport_error(int error);
#endif

private:
    Ui::AutMainWindow *ui;
    void RefreshSerialDevices();
    void UpdateImages();
    void DoLineEnd();
    void SerialStatus(bool bType);
    void OpenDevice(bool from_plugin = false);
    void LookupErrorCode(unsigned int intErrorCode);
    void FinishStream(bool bType);
    void LoadSettings();
    void UpdateSettings(int intMajor, int intMinor, QChar qcDelta);
#ifndef SKIPSPEEDTEST
    void SendSpeedTestData(int intMaxLength);
    void SpeedTestBytesWritten(qint64 intByteCount);
    void SpeedTestReceive();
    void OutputSpeedTestAvgStats(qint64 lngElapsed);
#endif
    void SetLoopBackMode(bool bNewMode);
#ifndef SKIPONLINE
    void AuTermUpdateCheck();
    bool is_newer(const QString *new_version, const QString *current_version);
#endif
    void UpdateCustomisation(bool bDefault);
    void update_buffer(QByteArray data, bool apply_formatting);
    void update_buffer(QByteArray *data, bool apply_formatting);
    void update_display_trimming();
#ifndef SKIPPLUGINS_TRANSPORT
    bool transport_open(QIODeviceBase::OpenMode mode);
    void transport_close();
    bool transport_isOpen() const;
    bool transport_isOpening() const;
    QSerialPort::DataBits transport_dataBits() const;
    AutTransportPlugin::StopBits transport_stopBits() const;
    QSerialPort::Parity transport_parity() const;
    qint64 transport_write(const QByteArray &data);
    qint64 transport_bytesAvailable() const;
    QByteArray transport_peek(qint64 maxlen);
    QByteArray transport_read(qint64 maxlen);
    QByteArray transport_readAll();
    bool transport_clear(QSerialPort::Directions directions = QSerialPort::AllDirections);
    bool transport_setBreakEnabled(bool set = true);
    bool transport_setRequestToSend(bool set);
    bool transport_setDataTerminalReady(bool set);
    QSerialPort::PinoutSignals transport_pinoutSignals();
    QString transport_error_to_error_string(int error);
    QString transport_name() const;
    bool transport_supports_break() const;
    bool transport_supports_request_to_send() const;
    bool transport_supports_data_terminal_ready() const;
#ifdef QT_STATIC
    AutPlugin::PluginType plugin_type(QStaticPlugin *plugin);
#else
    AutPlugin::PluginType plugin_type(QPluginLoader *plugin);
#endif
    bool plugin_type_supported(AutPlugin::PluginType type);
#endif

    //Private variables
    bool gbTermBusy; //True when compiling or loading a program or streaming a file (busy)
    bool gbStreamingFile; //True when a file is being streamed
    QSerialPort gspSerialPort; //Contains the handle for the serial port
    OS32_64UINT gintRXBytes; //Number of RX bytes
    OS32_64UINT gintTXBytes; //Number of TX bytes
    OS32_64UINT gintQueuedTXBytes; //Number of TX bytes that have been queued in buffer (not necesserially sent)
    unsigned char gchTermMode; //What function is being ran when compiling
    QImage gimEmptyCircleImage; //Holder for empty circle image
    QImage gimRedCircleImage; //Holder for red circle image
    QImage gimGreenCircleImage; //Holder for green circle image
    QImage gimUw16Image; //Holder for UwTerminal 16x16 icon
//    QImage gimUw32Image; //Holder for UwTerminal 32x32 icon
    QPixmap *gpEmptyCirclePixmap; //Pixmap holder for empty circle image
    QPixmap *gpRedCirclePixmap; //Pixmap holder for red circle image
    QPixmap *gpGreenCirclePixmap; //Pixmap holder for green circle image
//    QPixmap *gpUw32Pixmap; //Pixmap holder for UwTerminal 32x32 icon
    QPixmap *gpUw16Pixmap; //Pixmap holder for UwTerminal 16x16 icon
    QTimer *gpSignalTimer; //Handle for a timer to update COM port signals
    AutLogger *gpMainLog; //Handle to the main log file (if enabled/used)
    bool gbMainLogEnabled; //True if opened successfully (and enabled)
    QMenu *gpMenu; //Main menu
    QMenu *gpSMenu4; //Submenu 4
    QMenu *gpBalloonMenu; //Balloon menu
#ifndef SKIPSPEEDTEST
    QMenu *gpSpeedMenu; //Speed testing menu
#endif
    bool gbLoopbackMode; //True if loopback mode is enabled
    bool gbSysTrayEnabled; //True if system tray is enabled
    QSystemTrayIcon *gpSysTray; //Handle for system tray object
    bool gbCTSStatus; //True when CTS is asserted
    bool gbDCDStatus; //True when DCD is asserted
    bool gbDSRStatus; //True when DSR is asserted
    bool gbRIStatus; //True when RI is asserted
    QFile *gpStreamFileHandle; //Handle for the file to stream data from
    OS32_64UINT gintStreamBytesSize; //The size of the file to stream in bytes
    OS32_64UINT gintStreamBytesRead; //The number of bytes read from the stream
    OS32_64UINT gintStreamBytesProgress; //The number of bytes when the next progress output should be made
    display_buffer_list display_buffers; //List of pending data awaiting terminal display
    QElapsedTimer gtmrStreamTimer; //Counts how long a stream takes to send
    QTimer gtmrTextUpdateTimer; //Timer for slower updating of display buffer (but less display freezing)
    QSettings *gpTermSettings; //Handle to settings
    QSettings *gpErrorMessages; //Handle to error codes
    QSettings *gpPredefinedDevice; //Handle to predefined devices
#ifndef SKIPONLINE
    QNetworkAccessManager *gnmManager; //Network access manager
    QNetworkReply *gnmrReply; //Network reply
#endif
    QString gstrLastFilename[(FilenameIndexOthers+1)]; //Holds the filenames of the last selected files
    bool gbEditFileModified; //True if the file in the editor pane has been modified, otherwise false
    int giEditFileType; //Type of file currently open in the editor
    bool gbErrorsLoaded; //True if error csv file has been loaded
    PopupMessage *gpmErrorForm; //Error message form
#ifndef SKIPAUTOMATIONFORM
    AutAutomation *guaAutomationForm; //Automation form
#endif
#ifndef SKIPERRORCODEFORM
    AutErrorCode *gecErrorCodeForm; //Error code lookup form
#endif
#ifndef SKIPSCRIPTINGFORM
    AutScripting *gusScriptingForm; //Scripting form
    bool gbScriptingRunning; //True if a script is running
#endif
    bool gbSpeedTestRunning; //True if speed test is running
#ifndef SKIPSPEEDTEST
    unsigned char gchSpeedTestMode; //What mode the speed test is (inactive, receive, send or send & receive)
    QElapsedTimer gtmrSpeedTimer; //Used for timing how long a speed test has been running
    QByteArray gbaSpeedDisplayBuffer; //Buffer of data to display for speed test mode
    QByteArray gbaSpeedMatchData; //Expected data to match in speed test mode
    QByteArray gbaSpeedReceivedData; //Received data from device in speed test mode
    QTimer gtmrSpeedTestStats; //Timer that runs every 250ms to update stats for speed test
    QTimer gtmrSpeedTestStats10s; //Timer that runs every 10 seconds to output 10s stats for speed test
    QTimer gtmrSpeedUpdateTimer; //Timer for slower updating of speed test buffer (but less display freezing)
    QTimer *gtmrSpeedTestDelayTimer; //Timer used for delay before sending data in speed test mode
    quint64 gintSpeedBytesReceived; //Number of bytes received from device in speed test mode
    quint64 gintSpeedBytesReceived10s; //Number of bytes received from device in the past 10 seconds in speed test mode
    quint64 gintSpeedBytesSent; //Number of bytes sent to the device in speed test mode
    quint64 gintSpeedBytesSent10s; //Number of bytes sent to the device in the past 10 seconds in speed test mode
    qint32 gintSpeedBufferCount; //Number of bytes waiting to be sent to the device (waiting in the buffer) in speed test mode
    qint32 gintSpeedTestMatchDataLength; //Length of MatchData
    qint32 gintSpeedTestReceiveIndex; //Current index for RecData
    qint32 gintSpeedTestStatErrors; //Number of errors in packets recieved in speed test mode
    qint32 gintSpeedTestStatSuccess; //Number of successful packets received in speed test mode
    qint32 gintSpeedTestStatPacketsSent; //Numbers of packets sent in speed test mode
    qint32 gintSpeedTestStatPacketsReceived; //Number of packets received in speed test mode
    quint8 gintSpeedTestDataBits; //Number of data bits (per byte) for speed testing
    quint8 gintSpeedTestStartStopParityBits; //Number of bits for start/stop/parity (per byte) for speed testing
    quint8 gintSpeedTestBytesBits; //Holds the current speed test combo selection option
    quint8 gintDelayedSpeedTestSend; //Stores the delay before sending data in a speed test begins (in seconds)
    quint32 gintDelayedSpeedTestReceive; //Stores the delay before data started being received after a speed test begins (in seconds)
    bool gbSpeedTestReceived; //Set to true when data has been received in a speed test
#endif
    bool gbAppStarted; //True if application startup is complete
    QElapsedTimer gtmrPortOpened; //Used for updating last received timestamp
    qint64 gintLastSerialTimeUpdate; //Used for recording when next last received timestamp should appear
#ifndef SKIPPLUGINS
    bool gbPluginRunning; //True if a plugin is running
    bool gbPluginHideTerminalOutput; //True if terminal output should not be updated whilst plugin is running
    QObject *plugin_status_owner; //Owner of the last plugin set operation
    QList<QPushButton *> list_plugin_open_close_buttons;
    QList<plugins> plugin_list;
#endif
    bool display_update_pending; //True if a display update is pending (for the tab to be switched to the terminal view)
#ifndef SKIPSERIALDETECT
    AutSerialDetect *serial_detect;
    bool serial_detect_waiting;
    bool serial_close_dialog_open;
#endif
#ifndef SKIPPLUGINS_TRANSPORT
    AutTransportPlugin *plugin_active_transport;
#endif

protected:
    void dragEnterEvent(QDragEnterEvent *dragEvent);
    void dropEvent(QDropEvent *dropEvent);
};

#endif // AUTMAINWINDOW_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
