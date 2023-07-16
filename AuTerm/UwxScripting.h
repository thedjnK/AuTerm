/******************************************************************************
** Copyright (C) 2016-2018 Laird
**
** Project: AuTerm
**
** Module: UwxScripting.h
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
#ifndef UWXSCRIPTING_H
#define UWXSCRIPTING_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QDialog>
#include <QFileDialog>
#include <QFile>
#include "LrdHighlighter.h"
#include <QSerialPort>
#include <QTimer>
#include <QFontDialog>
#include <QTextStream>
#include <QDate>
#include <QTime>
#include "UwxPopup.h"
#include <QStatusBar>
#include <QElapsedTimer>
#include <QMenu>
#include <QKeySequence>
#include <QShortcut>
#include "UwxEscape.h"

/******************************************************************************/
// Defines
/******************************************************************************/
//Decides if generic data types will be 32 or 64-bit
#if _WIN64 || __aarch64__ || TARGET_OS_MAC || __x86_64__
    //64-bit OS
    #define OS32_64INT qint64
#else
    //32-bit or other OS
    #define OS32_64INT qint32
#endif

/******************************************************************************/
// Constants
/******************************************************************************/
const QChar   ScriptingDataIn              = '<';  //Command that waits for data to be received
const QChar   ScriptingDataOut             = '>';  //Command that sends data out to module
const QChar   ScriptingWaitTime            = '~';  //Command that waits for a period of time (in ms)
const QString ScriptingComment             = "//"; //A null-function command that is used to explain/comment code
const qint8   ScriptingActionDataIn        = 1;    //Action ID when waiting to receive data
const qint8   ScriptingActionDataOut       = 2;    //Action ID when sending data out
const qint8   ScriptingActionWaitTime      = 3;    //Action ID when waiting for a period of time
const qint8   ScriptingActionOther         = 4;    //Action ID when doing no action (empty line/comment)
const qint8   MenuActionChangeFont         = 1;    //Menu action ID for changing font
const qint8   MenuActionExportStringPlayer = 2;    //Menu action ID for exporting to string player
const qint8   ScriptingReasonOK            = 0;    //Return code for no error
const qint8   ScriptingReasonPortClosed    = 1;    //Return code if serial port is not open
const qint8   ScriptingReasonTermBusy      = 2;    //Return code if terminal is busy

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class UwxScripting;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class UwxScripting : public QDialog
{
    Q_OBJECT

public:
    explicit UwxScripting(
        QWidget *parent = 0
        );
    ~UwxScripting(
        );
    void
    SetPopupHandle(
        PopupMessage *pmNewHandle
        );
    void
    SetAuTermVersion(
        const QString strVersion
        );
    void
    SerialPortStatus(
        bool PortStatus
        );
    void
    SerialPortError(
        QSerialPort::SerialPortError SPE
        );
    void
    SerialPortData(
        const QByteArray *Data
        );
    void
    SerialPortWritten(
        int iWritten
        );
    void
    closeEvent(
        QCloseEvent *ceClose
        );
    void
    SetButtonStatus(
        bool bStatus
        );
    void
    reject(
        );
    void
    SetEditorFocus(
        );
    void
    ScriptStartResult(
        bool bStatus,
        unsigned char ucReason
        );
    void
    LoadScriptFile(
        const QString *strFilename
        );
    void
    SetScriptLastDirectory(
        const QString *strDirectory
        );

private slots:
    void
    ChangeFont(
        );
    void
    on_btn_Load_clicked(
        );
    void
    on_btn_Save_clicked(
        );
    bool
    on_btn_Compile_clicked(
        );
    void
    on_btn_Run_clicked(
        );
    void
    on_btn_Stop_clicked(
        );
    void
    ExportToStringPlayer(
        );
    void
    AdvanceLine(
        );
    void
    on_btn_Help_clicked(
        );
    bool
    CheckRecvMatchBuffers(
        );
    void
    on_btn_Pause_toggled(
        bool
        );
    void
    UpdateStatusBar(
        );
    void
    on_btn_Options_clicked(
        );
    void
    MenuSelected(
        QAction *qaAction
        );
    void
    on_btn_Clear_clicked(
        );

private:
    Ui::UwxScripting *ui;
    PopupMessage *mFormAuto; //Holds handle of error message dialogue
    LrdHighlighter *mhlHighlighter; //Handle for text highlighter
    int mintCLine; //Current line number
    QTimer mtmrPauseTimer; //Timer used for wait commands
    QTextBlock mtbExecutionBlock; //Holds the current execution block from the editor
    bool mbIsRunning; //Set to true if the script is running
    QString mstrAuTermVersion; //String containing the AuTerm version
    bool mbWaitingForReceive; //Set to true if waiting in a receive data command for data to arrive
    QByteArray mbaMatchData; //Buffer containing data that is matched against data received from the module (awating a match)
    QByteArray mbaRecvData; //Buffer containing data received from the module (awating a match)
    int mbBytesWriteRemain; //Number of bytes remaining to be written from the buffer (when specific mode is enabled)
    QStatusBar *msbStatusBar; //Pointer to scripting status bar
    bool mbSerialStatus; //True if serial port is open in main window
    unsigned char ucLastAct; //Which action is currently being executed
    QTimer mtmrUpdateTimer; //Runs every second when waiting for data or waiting for a timer to elapse to update status bar text
    QElapsedTimer gtmrScriptTimer; //Times how long the script took to execute
    QElapsedTimer gtmrRecTimer; //Times how long a receive took
    QMenu *gpOptionsMenu; //Options menu
    QMenu *gpSOptionsMenu1; //Options export sub-menu
    QShortcut *qaKeyShortcuts[5]; //Shortcut object handles for various keyboard shortcuts
    OS32_64INT mnRepeats; //Number of script repeats completed (when specific mode is enabled)
    uint32_t mnScriptLines; //Number of lines in the script
    QString strLastScriptFile; //The last file/directory which was used in the file open dialogue

signals:
    void ScriptFinished(
        );
    void ScriptStartRequest(
        );
    void SendData(
        QByteArray baDataString,
        bool bEscapeString,
        bool bFromScripting
        );
    void UpdateScriptLastDirectory(
        const QString *strDirectory
        );
};

#endif // UWXSCRIPTING_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
