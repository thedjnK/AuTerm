/******************************************************************************
** Copyright (C) 2015-2022 Laird Connectivity
**
** Project: AuTerm
**
** Module: UwxMainWindow.h
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
#ifndef UWXMAINWINDOW_H
#define UWXMAINWINDOW_H

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
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QFileInfo>
#include <QHostInfo>
#include <QStringView>
//Need cmath for std::ceil function
#include <cmath>
#if TARGET_OS_MAC
#include "QStandardPaths"
#endif
#include "LrdScrollEdit.h"
#include "UwxPopup.h"
#include "LrdLogger.h"
#if SKIPAUTOMATIONFORM != 1
#include "UwxAutomation.h"
#endif
#if SKIPERRORCODEFORM != 1
#include "UwxErrorCode.h"
#endif
#if SKIPSCRIPTINGFORM != 1
#include "UwxScripting.h"
#endif
#include "UwxEscape.h"
#if SKIPUSBRECOVERY != 1
//Conditional compile for BL654 USB dongle autorun exit
#if defined(_WIN32) && defined(_MSC_VER)
//MSVC build on windows
#define FTD2XX_STATIC
#include "FTDI/ftd2xx.h"
#elif defined(__linux__)
//Linux
#include <unistd.h>
#include <libftdi1/ftdi.h>
#include <libusb-1.0/libusb.h>
#include <string.h>
#endif
#endif

/******************************************************************************/
// Defines
/******************************************************************************/
#ifndef QT_NO_SSL
    #define UseSSL //By default enable SSL if Qt supports it (requires OpenSSL runtime libraries). Comment this line out to build without SSL support or if you get errors when communicating with the server
#endif
#ifdef UseSSL
    #include <QSslSocket>
#endif

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
const QString OldOldServerHost                  = "uwterminalx.no-ip.org"; //Very old hostname/IP of online xcompile server which is set to be removed
const QString OldServerHost                     = "uwterminalx.lairdtech.com"; //Old hostname/IP of online xcompile server which is set to be removed
const QString ServerHost                        = "uwterminalx.lairdconnect.com"; //Hostname/IP of online xcompile server
//Constants for various file download functions
const qint8 MODE_COMPILE                        = 1;
const qint8 MODE_COMPILE_LOAD                   = 2;
const qint8 MODE_COMPILE_LOAD_RUN               = 3;
const qint8 MODE_LOAD                           = 4;
const qint8 MODE_LOAD_RUN                       = 5;
const qint8 MODE_SERVER_COMPILE                 = 9;
const qint8 MODE_SERVER_COMPILE_LOAD            = 10;
const qint8 MODE_SERVER_COMPILE_LOAD_RUN        = 11;
const qint8 MODE_CHECK_ERROR_CODE_VERSIONS      = 14;
const qint8 MODE_CHECK_UWTERMINALX_VERSIONS     = 15;
const qint8 MODE_UPDATE_ERROR_CODE              = 16;
const qint8 MODE_CHECK_FIRMWARE_VERSIONS        = 17;
const qint8 MODE_CHECK_FIRMWARE_SUPPORT         = 18;
//Constants for version and functions
const QString UwVersion                         = "1.16"; //Version string
//Constants for timeouts and streaming
const qint16 FileReadBlock                      = 512;     //Number of bytes to read per block when streaming files
const qint16 StreamProgress                     = 10000;   //Number of bytes between streaming progress updates
const qint16 BatchTimeout                       = 4000;    //Time (in ms) to wait for getting a response from a batch command for
const qint16 PrePostXCompTimeout                = 15000;   //Time (in ms) to allow a pre/post XCompilation process to execute for
const qint16 ModuleTimeout                      = 4000;    //Time (in ms) that a download stage command/process times out (module)
const qint8 MaxOldDevNameSize                   = 8;       //Size (in characters) to allow for older module device names (characters past this point will be chopped off)
const qint8 MaxDevNameSize                      = 18;      //Size (in characters) to allow for a module device name (characters past this point will be chopped off)
const qint16 AutoBaudTimeout                    = 1200;    //Time (in ms) to wait before checking the next baud rate when automatically detecting the module's baud rate
const qint16 XCompATIRespTimeout                = 1200;    //Time (in ms) to wait to get a version response from an XCompiler before killing the process
//Constants for default config values
const QString DefaultLogFile                    = "AuTerm.log";
const bool DefaultLogMode                       = 0;
const bool DefaultLogEnable                     = 0;
const QString DefaultCompilerDir                = "compilers/";
const bool DefaultCompilerSubDirs               = 0;
const bool DefaultDelUWCAfterDownload           = 0;
const bool DefaultSysTrayIcon                   = 1;
const qint16 DefaultSerialSignalCheckInterval   = 50;
const bool DefaultPrePostXCompRun               = 0;
const bool DefaultPrePostXCompFail              = 0;
const bool DefaultPrePostXCompMode              = 0;
const QString DefaultPrePostXCompPath           = "";
const bool DefaultOnlineXComp                   = 1;
const qint16 DefaultTextUpdateInterval          = 80;
const bool DefaultSkipDownloadDisplay           = 1;
const bool DefaultSSLEnable                     = 1;
const bool DefaultShowFileSize                  = 1;
const bool DefaultConfirmClear                  = 1;
const bool DefaultShiftEnterLineSeparator       = 1;
const bool DefaultLicenceCheckMode              = 1;
const bool DefaultUpdateCheckEnable             = 1;
const bool DefaultFirmwareVersionCheckEnable    = 1;
const QDate DefaultUpdateCheckLast              = QDate(1970, 1, 1);
const bool DefaultAutoDTrimBuffer               = false; //(Unlisted option)
const quint32 DefaultAutoTrimDBufferThreshold   = 0;     //(Unlisted option)
const quint32 DefaultAutoTrimDBufferSize        = 0;     //(Unlisted option)
const quint16 DefaultScrollbackBufferSize       = 32;    //(Unlisted option)
const bool DefaultSaveSize                      = false;
//Constants for URLs
const QString URLLinuxNonRootSetup = "https://github.com/LairdCP/UwTerminalX/wiki/Granting-non-root-USB-device-access-(Linux)";
//Constants for the protocol
#ifndef UseSSL
    //HTTP
    const QString WebProtocol                   = "http";
#endif
const qint8 FilenameIndexApplication            = 0;
const qint8 FilenameIndexScripting              = 1;
const qint8 FilenameIndexOthers                 = 2;
//Constants for right click menu options
const qint8 MenuActionXCompile                  = 1;
const qint8 MenuActionXCompileLoad              = 2;
const qint8 MenuActionXCompileLoadRun           = 3;
const qint8 MenuActionLoad                      = 4;
const qint8 MenuActionLoadRun                   = 5;
const qint8 MenuActionErrorHex                  = 6;
const qint8 MenuActionErrorInt                  = 7;
const qint8 MenuActionLoopback                  = 8;
const qint8 MenuActionLoad2                     = 9;
const qint8 MenuActionEraseFile                 = 10;
const qint8 MenuActionDir                       = 11;
const qint8 MenuActionRun                       = 12;
const qint8 MenuActionDebug                     = 13;
const qint8 MenuActionDataFile                  = 14;
const qint8 MenuActionEraseFile2                = 15;
const qint8 MenuActionClearFilesystem           = 16;
const qint8 MenuActionMultiDataFile             = 17;
const qint8 MenuActionStreamFile                = 18;
const qint8 MenuActionFont                      = 19;
const qint8 MenuActionTextColour                = 20;
const qint8 MenuActionBackground                = 21;
const qint8 MenuActionRestoreDefaults           = 22;
const qint8 MenuActionRun2                      = 23;
const qint8 MenuActionAutomation                = 24;
const qint8 MenuActionScripting                 = 25;
const qint8 MenuActionBatch                     = 26;
const qint8 MenuActionClearModule               = 27;
const qint8 MenuActionClearDisplay              = 28;
const qint8 MenuActionClearRxTx                 = 29;
const qint8 MenuActionCopy                      = 30;
const qint8 MenuActionCopyAll                   = 31;
const qint8 MenuActionPaste                     = 32;
const qint8 MenuActionSelectAll                 = 33;
//Constants for web selection combo box
const qint8 WebSelectionBL654Applications       = 0;
const qint8 WebSelectionBL653Applications       = 1;
const qint8 WebSelectionBL652Applications       = 2;
const qint8 WebSelectionPinnacle100Firmware     = 3;
const qint8 WebSelectionBluegrassDemo           = 4;
const qint8 WebSelectionRM1xxApplications       = 5;
const qint8 WebSelectionBT900Applications       = 6;
const qint8 WebSelectionBL600Applications       = 7;
const qint8 WebSelectionBL620Applications       = 8;
const qint8 WebSelectionWirelessModules         = 9;
const qint8 WebSelectionUwFlashX                = 10;
//Constants for balloon (notification area) icon options
const qint8 BalloonActionShow                   = 1;
const qint8 BalloonActionExit                   = 2;
//Constants for speed test menu
const qint8 SpeedMenuActionRecv                 = 1;
const qint8 SpeedMenuActionSend                 = 2;
const qint8 SpeedMenuActionSendRecv             = 3;
const qint8 SpeedMenuActionSendRecv5Delay       = 4;
const qint8 SpeedMenuActionSendRecv10Delay      = 5;
const qint8 SpeedMenuActionSendRecv15Delay      = 6;
const qint8 SpeedModeInactive                   = 0b00;
const qint8 SpeedModeRecv                       = 0b01;
const qint8 SpeedModeSend                       = 0b10;
const qint8 SpeedModeSendRecv                   = 0b11;
//Constants for speed testing
const qint16 SpeedTestChunkSize                 = 512;  //Maximum number of bytes to send per chunk when speed testing
const qint16 SpeedTestMinBufSize                = 128;  //Minimum buffer size when speed testing, when there are less than this number of bytes in the output buffer it will be topped up
const qint16 SpeedTestStatUpdateTime            = 500;  //Time (in ms) between status updates for speed test mode
//XCompile exit codes
const int XCOMP_OK                              = 0;
const int XCOMP_GENERIC_FAIL                    = 1;
const int XCOMP_FILENAME_EMPTY                  = 2;
const int XCOMP_FACTORY_FILE_OPEN_ERROR         = 3;
const int XCOMP_RAWLINE_MALLOC_FAIL             = 4;
const int XCOMP_OUTFILE_NAME_CREATION           = 5;
const int XCOMP_PREPARSE_ERROR                  = 6;
const int XCOMP_TOKENISELINE_ERROR              = 7;
const int XCOMP_GENERIC_ERROR                   = 8;
const int XCOMP_ARGS_PARSING_ERROR              = 9;
const int XCOMP_EMPTY_FILENAME                  = 10;
const int XCOMP_TOKPUBOPEN_FAIL                 = 12;
const int XCOMP_MAINSCRIPTFILE_OPEN_ERROR       = 11;
const int XCOMP_NOT_ENOUGH_ARGS                 = 13;
const int XCOMP_MISSING_FILE                    = 14;
const int XCOMP_XCMPEXEFILE_OPEN_ERROR          = 15;
const int XCOMP_UNEXPECTED_ENDOF_FILE           = 20;
const int XCOMP_NEWXCMPEXEFILE_OPEN_ERROR       = 30;
const int XCOMP_SBXFULLPATH_MALLOC_FAIL         = 100;
const int XCOMP_SBX_DATABASE_MISSING            = 101;
const int XCOMP_MISSING_FILE_LEGACY             = 500;
const int XCOMP_XCMPEXEFILE_OPEN_ERROR_LEGACY   = 2000;
#if (SKIPUSBRECOVERY != 1) && !defined(TARGET_OS_MAC)
//Pin definitions for BL654 USB dongle
const int FTDI_BL654_USB_RX_DIO                 = 1;
const int FTDI_BL654_USB_RTS_DIO                = 4;
const int FTDI_BL654_USB_VSP_DIO                = 32;
const int FTDI_BL654_USB_NRESET_DIO             = 64;
const int FTDI_BL654_USB_AUTORUN_DIO            = 128;
const int FTDI_BL654_USB_RESET_DELAY            = 400;
#endif
const QString WINDOWS_NEWLINE                   = "\r\n";
const QChar NEWLINE                             = '\n';

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class MainWindow;
}

//Structure used for holding information on online XCompiled file locations
typedef struct
{
    QString strFilename; //Filename
    qint16 iStartingLine; //Starting line that file was put on
    qint16 iEndingLine; //Ending line that file's last data was put on
    qint16 iOrigLines; //Number of lines in the source file
    qint8 iParent; //Parent include file index
} FileSStruct;

//Structures used for holding information when listing available XCompilers on the local PC
#ifdef _WIN32
typedef struct
{
    QString ATI3;
    QString LanguageHash;
} XCompInfoStruct;

typedef struct
{
    QString DeviceName;
    QList<XCompInfoStruct> Firmwares;
} XCompDevStruct;
#endif

//Enum used for specifying type of data
enum class BitByteTypes
{
    TypeBytes,
    TypeDataBits,
    TypeAllBits
};

//Enum used for specifying type of data
enum class UpdateCheckTypes
{
    TypeNone,
    TypeNormal,
    TypeWekely
};

/******************************************************************************/
// Class definitions
/******************************************************************************/
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(
        QWidget *parent = 0
        );
    ~MainWindow(
        );

public slots:
    void
    SerialRead(
        );
    void
    MenuSelected(
        QAction* qaAction
        );
    void
    balloontriggered(
        QAction* qaAction
        );
    void
    DevRespTimeout(
        );
#if defined(_WIN32) || defined(WIN32)
    //_WIN32 isn't defined when MOC is ran but WIN32 is.
    void
    process_finished(
        int intExitCode,
        QProcess::ExitStatus esExitStatus
        );
    void
    process_error(
        QProcess::ProcessError
        );
#endif
    void
    SerialStatusSlot(
        );
    void
    SerialError(
        QSerialPort::SerialPortError speErrorCode
        );
    void
    EnterPressed(
        );
    void
    KeyPressed(
        int nKey,
        QChar chrKeyValue
        );
    void
    SerialBytesWritten(
        qint64 intByteCount
        );
    void
    SerialPortClosing(
        );
    void
    MessagePass(
        QByteArray baDataString,
        bool bEscapeString,
        bool bFromScripting
        );
#if SKIPSPEEDTEST != 1
    void
    SpeedMenuSelected(
        QAction* qaAction
        );
    quint64
    BitsBytesConvert(
        quint64 iCount,
        BitByteTypes bbtFrom,
        BitByteTypes bbtTo
        );
#endif

private slots:
    void
    on_btn_Accept_clicked(
        );
    void
    on_selector_Tab_currentChanged(
        int intIndex
        );
    void
    on_btn_Connect_clicked(
        );
    void
    on_btn_TermClose_clicked(
        );
    void
    on_btn_Refresh_clicked(
        );
    void
    on_btn_TermClear_clicked(
        );
    void
    on_btn_Duplicate_clicked(
        );
    void
    on_text_TermEditData_customContextMenuRequested(
        const QPoint &pos
        );
    void
    on_check_Break_stateChanged(
        );
    void
    on_check_RTS_stateChanged(
        );
    void
    on_check_DTR_stateChanged(
        );
    void
    on_check_Line_stateChanged(
        );
    void
    closeEvent(
        QCloseEvent *closeEvent
        );
    void
    on_btn_Cancel_clicked(
        );
    void
    UpdateReceiveText(
        );
    void
    BatchTimeoutSlot(
        );
    void
    on_combo_COM_currentIndexChanged(
        int intIndex
        );
    void
    replyFinished(
        QNetworkReply* nrReply
        );
#ifdef UseSSL
    void
    sslErrors(
        QNetworkReply*,
        QList<QSslError>
        );
#endif
    void
    on_check_PreXCompRun_stateChanged(
        int intChecked
        );
    bool
    RunPrePostExecutable(
        QString strFilename
        );
    void
    on_btn_PreXCompSelect_clicked(
        );
    void
    on_radio_XCompPre_toggled(
        bool bChecked
        );
    void
    on_radio_XCompPost_toggled(
        bool bChecked
        );
    void
    on_check_PreXCompFail_stateChanged(
        int bChecked
        );
    void
    on_edit_PreXCompFilename_editingFinished(
        );
    void
    on_btn_GitHub_clicked(
        );
    void
    on_check_OnlineXComp_stateChanged(
        int bChecked
        );
    QList<QString>
    SplitFilePath(
        QString strFilename
        );
    void
    on_btn_ErrorCodeUpdate_clicked(
        );
    void
    on_btn_UwTerminalXUpdate_clicked(
        );
    void
    on_check_Echo_stateChanged(
        int bChecked
        );
    void
    on_btn_ErrorCodeDownload_clicked(
        );
    void
    on_combo_PredefinedDevice_currentIndexChanged(
        int intIndex
        );
    void
    on_btn_PredefinedAdd_clicked(
        );
    void
    on_btn_PredefinedDelete_clicked(
        );
    void
    DroppedFile(
        QString strFilename
        );
    void
    on_btn_SaveDevice_clicked(
        );
    void
    on_btn_ModuleFirmware_clicked(
        );
    void
    ContextMenuClosed(
        );
    bool
    event(
        QEvent *evtEvent
        );
    void
    on_btn_LogFileSelect_clicked(
        );
    void
    on_edit_LogFile_editingFinished(
        );
    void
    on_check_LogEnable_stateChanged(
        int intChecked
        );
    void
    on_check_LogAppend_stateChanged(
        int intChecked
        );
    void
    on_btn_Help_clicked(
        );
    void
    on_combo_LogDirectory_currentIndexChanged(
        int
        );
    void
    on_btn_LogRefresh_clicked(
        );
    void
    on_btn_Licenses_clicked(
        );
    void
    on_btn_OnlineXComp_Supported_clicked(
        );
    void
    on_check_SkipDL_stateChanged(
        int
        );
    bool
    LookupDNSName(
        bool bShowError
        );
    void
    on_btn_WebBrowse_clicked(
        );
    void
    on_btn_EditViewFolder_clicked(
        );
    void
    on_combo_EditFile_currentIndexChanged(
        int
        );
    void
    on_btn_EditSave_clicked(
        );
    void
    on_btn_EditLoad_clicked(
        );
#ifndef __APPLE__
    void
    on_btn_EditExternal_clicked(
        );
#endif
    void
    on_btn_LogViewExternal_clicked(
        );
    void
    on_btn_LogViewFolder_clicked(
        );
    void
    on_text_EditData_textChanged(
        );
    void
    on_combo_LogFile_currentIndexChanged(
        int
        );
    void
    on_btn_ReloadLog_clicked(
        );
#ifdef UseSSL
    void
    on_check_EnableSSL_stateChanged(
        int
        );
#endif
    void
    on_check_ShowFileSize_stateChanged(
        int
        );
    void
    on_btn_DetectBaud_clicked(
        );
    void
    DetectBaudTimeout(
        );
    void
    on_check_ConfirmClear_stateChanged(
        int
        );
    void
    on_check_LineSeparator_stateChanged(
        int
        );
#if SKIPERRORCODEFORM != 1
    void
    on_btn_Error_clicked(
        );
#endif
#if SKIPSCRIPTINGFORM != 1
    void
    ScriptStartRequest(
        );
    void
    ScriptFinished(
        );
#endif
#if SKIPSPEEDTEST != 1
    void
    on_check_SpeedRTS_stateChanged(
        int
        );
    void
    on_check_SpeedDTR_stateChanged(
        int
        );
    void
    on_btn_SpeedClear_clicked(
        );
    void
    on_btn_SpeedClose_clicked(
        );
    void
    on_btn_SpeedStartStop_clicked(
        );
    void
    OutputSpeedTestStats(
        );
    void
    on_combo_SpeedDataType_currentIndexChanged(
        int
        );
    void
    on_btn_SpeedCopy_clicked(
        );
    void
    UpdateSpeedTestValues(
        );
    void
    SpeedTestStartTimer(
        );
    void
    SpeedTestStopTimer(
        );
    void
    on_combo_SpeedDataDisplay_currentIndexChanged(
        int
        );
    void
    UpdateDisplayText(
        );
#endif
    void
    on_check_CheckLicense_stateChanged(
        int
        );
    void
    on_check_EnableWeeklyUpdateCheck_stateChanged(
        int
        );
    QString
    XCompile_Error_Message(
        int intExitCode
        );
#ifdef _WIN32
    void
    on_btn_Local_XCompilers_clicked(
        );
#endif
#if !defined(SKIPUSBRECOVERY) && !defined(TARGET_OS_MAC)
    void
    on_btn_ExitAutorun_clicked(
        );
#endif
    void
    on_check_EnableModuleFirmwareCheck_stateChanged(
        int
        );
    void
    ScriptingFileSelected(
        const QString *strFilepath
        );
    void
    on_check_EnableTerminalSizeSaving_stateChanged(
        int
        );
    void
    resizeEvent(
        QResizeEvent *
        );
    void
    on_edit_Title_textEdited(
        const QString &
        );

private:
    Ui::MainWindow *ui;
    void
    RefreshSerialDevices(
        );
    unsigned char
    CompileApp(
        unsigned char chMode
        );
    void
    UpdateImages(
        );
    void
    DoLineEnd(
        );
    void
    SerialStatus(
        bool bType
        );
    void
    OpenDevice(
        );
    void
    LoadFile(
        bool bToUWC
        );
    void
    RunApplication(
        );
    void
    LookupErrorCode(
        unsigned int intErrorCode
        );
    QString
    LookupErrorCodeXCompile(
        unsigned int intErrorCode
        );
    void
    FinishStream(
        bool bType
        );
    void
    FinishBatch(
        bool bType
        );
    QString
    AtiToXCompName(
        QString strAtiResp
        );
    void
    LoadSettings(
        );
    void
    UpdateSettings(
        int intMajor,
        int intMinor,
        QChar qcDelta
        );
    QString
    CleanFilesize(
        QString strFilename
        );
    QString
    RemoveZeros(
        QString strData
        );
#if SKIPSPEEDTEST != 1
    void
    SendSpeedTestData(
        int intMaxLength
        );
    void
    SpeedTestBytesWritten(
        qint64 intByteCount
        );
    void
    SpeedTestReceive(
        );
    void
    OutputSpeedTestAvgStats(
        qint64 lngElapsed
        );
#endif
    void
    StreamBatchContinue(
        QByteArray *baOrigData
        );
    void
    ClearFileDataList(
        );
    void
    SetLoopBackMode(
        bool bNewMode
        );
    void
    UwTerminalXUpdateCheck(
        bool bShowError
        );
    void
    ErrorCodeUpdateCheck(
        bool bShowError
        );
    void
    UpdateCustomisation(
        bool bDefault
    );

    //Private variables
    bool gbTermBusy; //True when compiling or loading a program or streaming a file (busy)
    bool gbStreamingFile; //True when a file is being streamed
    QSerialPort gspSerialPort; //Contains the handle for the serial port
    OS32_64UINT gintRXBytes; //Number of RX bytes
    OS32_64UINT gintTXBytes; //Number of TX bytes
    OS32_64UINT gintQueuedTXBytes; //Number of TX bytes that have been queued in buffer (not necesserially sent)
    unsigned char gchTermBusyLines; //Number of commands recieved (for sending applications)
    unsigned char gchTermMode; //What function is being ran when compiling
    unsigned char gchTermMode2; //Current sub-mode of download
    QString gstrTermFilename; //Holds the filename of the file to load
    QString gstrTermBusyData; //Holds the recieved data for checking
#ifdef _WIN32
    QProcess gprocCompileProcess; //Holds the data for the process to compile
#endif
    QImage gimEmptyCircleImage; //Holder for empty circle image
    QImage gimRedCircleImage; //Holder for red circle image
    QImage gimGreenCircleImage; //Holder for green circle image
    QImage gimUw16Image; //Holder for UwTerminal 16x16 icon
    QImage gimUw32Image; //Holder for UwTerminal 32x32 icon
    QPixmap *gpEmptyCirclePixmap; //Pixmap holder for empty circle image
    QPixmap *gpRedCirclePixmap; //Pixmap holder for red circle image
    QPixmap *gpGreenCirclePixmap; //Pixmap holder for green circle image
    QPixmap *gpUw32Pixmap; //Pixmap holder for UwTerminal 32x32 icon
    QPixmap *gpUw16Pixmap; //Pixmap holder for UwTerminal 16x16 icon
    QString gstrHexData; //Holds the hex data to be sent to the device
    QString gstrDownloadFilename; //Holds the inter-function download filename
    QTimer *gpSignalTimer; //Handle for a timer to update COM port signals
    QTimer gtmrDownloadTimeoutTimer; //Timer for module timeout indication
    LrdLogger *gpMainLog; //Handle to the main log file (if enabled/used)
    bool gbMainLogEnabled; //True if opened successfully (and enabled)
    QMenu *gpMenu; //Main menu
    QMenu *gpSMenu1; //Submenu 1
    QMenu *gpSMenu2; //Submenu 2
    QMenu *gpSMenu3; //Submenu 3
    QMenu *gpSMenu4; //Submenu 4
    QMenu *gpBalloonMenu; //Balloon menu
#if SKIPSPEEDTEST != 1
    QMenu *gpSpeedMenu; //Speed testing menu
#endif
    bool gbLoopbackMode; //True if loopback mode is enabled
    bool gbSysTrayEnabled; //True if system tray is enabled
    QSystemTrayIcon *gpSysTray; //Handle for system tray object
    bool gbIsUWCDownload; //Set to true when a UWC is being downloaded
    bool gbCTSStatus; //True when CTS is asserted
    bool gbDCDStatus; //True when DCD is asserted
    bool gbDSRStatus; //True when DSR is asserted
    bool gbRIStatus; //True when RI is asserted
    QFile *gpStreamFileHandle; //Handle for the file to stream data from
    OS32_64UINT gintStreamBytesSize; //The size of the file to stream in bytes
    OS32_64UINT gintStreamBytesRead; //The number of bytes read from the stream
    OS32_64UINT gintStreamBytesProgress; //The number of bytes when the next progress output should be made
    QByteArray gbaDisplayBuffer; //Buffer of data to display
    QElapsedTimer gtmrStreamTimer; //Counts how long a stream takes to send
    QTimer gtmrTextUpdateTimer; //Timer for slower updating of display buffer (but less display freezing)
    bool gbStreamingBatch; //True if batch file is being streamed
    QTimer gtmrBatchTimeoutTimer; //Timer for a batch command timeout
    QByteArray gbaBatchReceive; //Storage for batch data coming in
    QSettings *gpTermSettings; //Handle to settings
    QSettings *gpErrorMessages; //Handle to error codes
    QSettings *gpPredefinedDevice; //Handle to predefined devices
    QNetworkAccessManager *gnmManager; //Network access manager
    QNetworkReply *gnmrReply; //Network reply
    QString gstrDeviceID; //What the server compiler ID is
    bool gbFileOpened; //True when a file on the module has been opened
    QString gstrLastFilename[(FilenameIndexOthers+1)]; //Holds the filenames of the last selected files
#ifdef RESOLVEIPSEPARATELY
    QString gstrResolvedServer; //Holds the resolved hostname of the XCompile server
#endif
    bool gbEditFileModified; //True if the file in the editor pane has been modified, otherwise false
    int giEditFileType; //Type of file currently open in the editor
    bool gbErrorsLoaded; //True if error csv file has been loaded
    QTimer gtmrBaudTimer; //Timer for automatic baud rate detection timeout
    bool gbAutoBaud; //True if automatic baud rate detection is in progress
    QList<FileSStruct *> lstFileData; //Holds a list of filenames and line numbers for the file currently being XCompiled
#ifdef UseSSL
    QString WebProtocol; //Holds HTTP or HTTPS depending on options selected
    QSslCertificate *sslcLairdSSLNew = NULL; //Holds the (newer) Laird SSL certificate
#endif
    PopupMessage *gpmErrorForm; //Error message form
#if SKIPAUTOMATIONFORM != 1
    UwxAutomation *guaAutomationForm; //Automation form
#endif
#if SKIPERRORCODEFORM != 1
    UwxErrorCode *gecErrorCodeForm; //Error code lookup form
#endif
#if SKIPSCRIPTINGFORM != 1
    UwxScripting *gusScriptingForm; //Scripting form
    bool gbScriptingRunning; //True if a script is running
#endif
    bool gbSpeedTestRunning; //True if speed test is running
#if SKIPSPEEDTEST != 1
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
    quint32 gintSpeedBufferCount; //Number of bytes waiting to be sent to the device (waiting in the buffer) in speed test mode
    quint32 gintSpeedTestMatchDataLength; //Length of MatchData
    quint32 gintSpeedTestReceiveIndex; //Current index for RecData
    quint32 gintSpeedTestStatErrors; //Number of errors in packets recieved in speed test mode
    quint32 gintSpeedTestStatSuccess; //Number of successful packets received in speed test mode
    quint32 gintSpeedTestStatPacketsSent; //Numbers of packets sent in speed test mode
    quint32 gintSpeedTestStatPacketsReceived; //Number of packets received in speed test mode
    quint8 gintSpeedTestDataBits; //Number of data bits (per byte) for speed testing
    quint8 gintSpeedTestStartStopParityBits; //Number of bits for start/stop/parity (per byte) for speed testing
    quint8 gintSpeedTestBytesBits; //Holds the current speed test combo selection option
    quint8 gintDelayedSpeedTestSend; //Stores the delay before sending data in a speed test begins (in seconds)
    quint32 gintDelayedSpeedTestReceive; //Stores the delay before data started being received after a speed test begins (in seconds)
    bool gbSpeedTestReceived; //Set to true when data has been received in a speed test
#endif
    UpdateCheckTypes gupdUpdateCheck; //Type of update check being performed
    QString *gstrUpdateCheckString; //String message for displaying weekly update message
    bool gbAutoTrimDBuffer; //(Unlisted option) Set to true to automatically trim the display buffer when it reaches a threashold
    quint32 gintAutoTrimBufferDThreshold; //(Unlisted option) Number of bytes at which to trim the display buffer
    quint32 gintAutoTrimBufferDSize; //(Unlisted option) Number of bytes to trim the recieve buffer
    bool gbAppStarted; //True if application startup is complete

protected:
    void dragEnterEvent(
        QDragEnterEvent *dragEvent
        );
    void dropEvent(
        QDropEvent *dropEvent
        );
};

#endif // UWXMAINWINDOW_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
