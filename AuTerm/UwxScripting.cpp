/******************************************************************************
** Copyright (C) 2016-2018 Laird
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module: UwxScripting.cpp
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
#include "UwxScripting.h"
#include "ui_UwxScripting.h"

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
UwxScripting::UwxScripting(QWidget *parent) : QDialog(parent), ui(new Ui::UwxScripting)
{
    //On form creation
    ui->setupUi(this);

    //Set the textbox font
    QFont fntTmpFnt = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fntTmpFnt.setFixedPitch(true);
    fntTmpFnt.setPointSize(9);
    QFontMetrics tmTmpFM(fntTmpFnt);
    ui->edit_Script->setFont(fntTmpFnt);
    ui->edit_Script->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);

    //Create highlighter object
    mhlHighlighter = new LrdHighlighter(ui->edit_Script->document());

    //Setup pause timer
    mtmrPauseTimer.setSingleShot(true);
    connect(&mtmrPauseTimer, SIGNAL(timeout()), this, SLOT(AdvanceLine()));

    //Setup status bar update timer
    mtmrUpdateTimer.setSingleShot(false);
    connect(&mtmrUpdateTimer, SIGNAL(timeout()), this, SLOT(UpdateStatusBar()));

    //Script is not currently running or waiting for a data match
    mbIsRunning = false;
    mbWaitingForReceive = false;

    //Set the icons and tooltip of the buttons
    ui->btn_Load->setText("");
    ui->btn_Load->setIcon(this->style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->btn_Save->setText("");
    ui->btn_Save->setIcon(this->style()->standardIcon(QStyle::SP_DialogSaveButton));
    ui->btn_Compile->setText("");
    ui->btn_Compile->setIcon(this->style()->standardIcon(QStyle::SP_MessageBoxWarning));
    ui->btn_Run->setText("");
    ui->btn_Run->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
    ui->btn_Pause->setText("");
    ui->btn_Pause->setIcon(this->style()->standardIcon(QStyle::SP_MediaPause));
    ui->btn_Stop->setText("");
    ui->btn_Stop->setIcon(this->style()->standardIcon(QStyle::SP_MessageBoxCritical));
    ui->btn_Clear->setText("");
    ui->btn_Clear->setIcon(this->style()->standardIcon(QStyle::SP_DialogDiscardButton));
    ui->btn_Help->setText("");
    ui->btn_Help->setIcon(this->style()->standardIcon(QStyle::SP_MessageBoxQuestion));
    ui->btn_Options->setMinimumHeight(ui->btn_Compile->height()-2);
    ui->btn_Options->setMaximumHeight(ui->btn_Compile->height()-2);

    //Serial port is not yet open
    mbSerialStatus = false;

    //Create status bar
    msbStatusBar = new QStatusBar;
    ui->StatusBarLayout->addWidget(msbStatusBar);
    msbStatusBar->showMessage("[Status]");

    //Remove question mark button from window title
    this->setWindowFlags((Qt::Window | Qt::WindowCloseButtonHint));

    //Create option menu items
    gpOptionsMenu = new QMenu(this);
    gpOptionsMenu->addAction("Change Font")->setData(MenuActionChangeFont);
    //gpSOptionsMenu1 = gpOptionsMenu->addMenu("Export to");

    //Connect signals
    connect(gpOptionsMenu, SIGNAL(triggered(QAction*)), this, SLOT(MenuSelected(QAction*)));

    //Create shotcut objects
    qaKeyShortcuts[0] = new QShortcut(QKeySequence::Save, this);
    qaKeyShortcuts[1] = new QShortcut(QKeySequence::Open, this);
    qaKeyShortcuts[2] = new QShortcut(QKeySequence::HelpContents, this);
    qaKeyShortcuts[3] = new QShortcut(QKeySequence::New, this);
    qaKeyShortcuts[4] = new QShortcut(QKeySequence::Refresh, this);

    //Connect shortcuts
    connect(qaKeyShortcuts[0], SIGNAL(activated()), this, SLOT(on_btn_Save_clicked()));
    connect(qaKeyShortcuts[1], SIGNAL(activated()), this, SLOT(on_btn_Load_clicked()));
    connect(qaKeyShortcuts[2], SIGNAL(activated()), this, SLOT(on_btn_Help_clicked()));
    connect(qaKeyShortcuts[3], SIGNAL(activated()), this, SLOT(on_btn_Clear_clicked()));
    connect(qaKeyShortcuts[4], SIGNAL(activated()), this, SLOT(on_btn_Run_clicked()));
}

//=============================================================================
//=============================================================================
UwxScripting::~UwxScripting(
    )
{
    //On dialogue deletion
    disconnect(&mtmrPauseTimer, SIGNAL(timeout()), this, SLOT(AdvanceLine()));
    disconnect(gpOptionsMenu, SIGNAL(triggered(QAction*)), this, SLOT(MenuSelected(QAction*)));
    disconnect(qaKeyShortcuts[0], SIGNAL(activated()), this, SLOT(on_btn_Save_clicked()));
    disconnect(qaKeyShortcuts[1], SIGNAL(activated()), this, SLOT(on_btn_Load_clicked()));
    disconnect(qaKeyShortcuts[2], SIGNAL(activated()), this, SLOT(on_btn_Help_clicked()));
    disconnect(qaKeyShortcuts[3], SIGNAL(activated()), this, SLOT(on_btn_Clear_clicked()));
    disconnect(qaKeyShortcuts[4], SIGNAL(activated()), this, SLOT(on_btn_Run_clicked()));

    //Clean up
    delete qaKeyShortcuts[4];
    delete qaKeyShortcuts[3];
    delete qaKeyShortcuts[2];
    delete qaKeyShortcuts[1];
    delete qaKeyShortcuts[0];
    delete mhlHighlighter;
    delete msbStatusBar;
//    delete gpSOptionsMenu1;
    delete gpOptionsMenu;
    delete ui;
}

//=============================================================================
//=============================================================================
void
UwxScripting::SetPopupHandle(
    PopupMessage *pmNewHandle
    )
{
    //Sets the Popup Message handle
    mFormAuto = pmNewHandle;
}

//=============================================================================
//=============================================================================
void
UwxScripting::ChangeFont(
    )
{
    //Change font
    bool bTmpBool;
    QFont fntTmpFnt = QFontDialog::getFont(&bTmpBool, ui->edit_Script->font(), this);
    if (bTmpBool == true)
    {
        //Set font and re-adjust tab spacing
        QFontMetrics tmTmpFM(fntTmpFnt);
        ui->edit_Script->setFont(fntTmpFnt);
        ui->edit_Script->setTabStopDistance(tmTmpFM.horizontalAdvance(" ")*6);
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::on_btn_Load_clicked(
    )
{
    //Load buffer from file
    QString strLoadFile = QFileDialog::getOpenFileName(this, tr("Open File"), strLastScriptFile, "Text files (*.txt)");

    if (!strLoadFile.isNull() && !strLoadFile.isEmpty())
    {
        //File was selected
        LoadScriptFile(&strLoadFile);
        emit UpdateScriptLastDirectory(&strLoadFile);
        strLastScriptFile = strLoadFile;
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::on_btn_Save_clicked(
    )
{
    //Save buffer contents
    QString strSaveFile = QFileDialog::getSaveFileName(this, tr("Save File"), "", "Text files (*.txt)");

    if (strSaveFile.length() > 1)
    {
        //File was selected
        QFile fileSave(strSaveFile);
        if (fileSave.open(QFile::WriteOnly | QFile::Text))
        {
            //File opened for writing
            if (fileSave.size() > ui->edit_Script->toPlainText().length())
            {
                //Truncate file
                fileSave.resize(ui->edit_Script->toPlainText().length());
            }

            //Output the data to the file
            QTextStream tsDataStream(&fileSave);
            tsDataStream << ui->edit_Script->toPlainText();
            fileSave.close();
        }
        else
        {
            //Failed to open file
            QString strMessage = tr("Error during scripting file save: Access to selected file is denied: ").append(strSaveFile);
            mFormAuto->SetMessage(&strMessage);
            mFormAuto->show();
        }
    }
}

//=============================================================================
//=============================================================================
bool
UwxScripting::on_btn_Compile_clicked(
    )
{
    //Compile script
    ui->edit_Script->ClearBadLines();
    mintCLine = -1;
    ui->edit_Script->SetExecutionLine(mintCLine);
    unsigned int i = 0;
    bool bFailed = false;
    if (ui->edit_Script->document()->lineCount() > 0)
    {
        //At least one line of data
        QTextBlock tbCurrentBlock = ui->edit_Script->document()->firstBlock();
        while (tbCurrentBlock.isValid())
        {
            int iLineLength = tbCurrentBlock.text().length();
            if (iLineLength == 1)
            {
                //Fail
                ui->edit_Script->AddBadLine(i);
                bFailed = true;
            }
            else if (iLineLength > 1)
            {
                //Check the command
                if (tbCurrentBlock.text().at(0) == ScriptingWaitTime)
                {
                    //Check if the time value is valid or not
                    bool bConverted = false;
                    int intConv = tbCurrentBlock.text().right(tbCurrentBlock.text().length()-1).toInt(&bConverted);
                    if (bConverted == false)
                    {
                        //Invalid number
                        ui->edit_Script->AddBadLine(i);
                        bFailed = true;
                    }
                    else if (intConv <= 0)
                    {
                        //Time value is 0 or negative
                        ui->edit_Script->AddBadLine(i);
                        bFailed = true;
                    }
                }
                else if (!(tbCurrentBlock.text().at(0) == ScriptingDataOut) && !(tbCurrentBlock.text().at(0) == ScriptingDataIn) && !(tbCurrentBlock.text().at(0) == ScriptingWaitTime) && !(tbCurrentBlock.text().left(2) == ScriptingComment) && QString(tbCurrentBlock.text()).replace("\t", "").replace(" ", "").length() > 0)
                {
                    //Text present that isn't space/tab or a valid command
                    ui->edit_Script->AddBadLine(i);
                    bFailed = true;
                }
            }

            //Progrss to next block
            tbCurrentBlock = tbCurrentBlock.next();
            ++i;
        }
    }

    //Show status bar message
    msbStatusBar->showMessage((bFailed == false ? "Script compile successful: no errors." : "Script compile failed due to syntax errors."));

    //Repaint with line colours
    ui->edit_Script->repaint();

    //Return fail code
    return bFailed;
}

//=============================================================================
//=============================================================================
void
UwxScripting::on_btn_Run_clicked(
    )
{
    //Run script
    if (mbSerialStatus == true && on_btn_Compile_clicked() == false)
    {
        //Compile successful, check if main form is busy
        mnRepeats = 0;
        mnScriptLines = ui->edit_Script->blockCount();
        msbStatusBar->showMessage("Script execution request pending...");
        emit ScriptStartRequest();
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::SerialPortStatus(
    bool bPortStatus
    )
{
    //Serial port has been opened or closed
    mbSerialStatus = bPortStatus;
    if (bPortStatus == false)
    {
        if (mbIsRunning == true)
        {
            //Stop running script and display error
            on_btn_Stop_clicked();
            msbStatusBar->showMessage("Script stopped due to serial port disconnecting.");
        }

        //Disable run button
        ui->btn_Run->setEnabled(false);
    }
    else
    {
        //Enable run button
        ui->btn_Run->setEnabled(true);
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::SerialPortError(
    QSerialPort::SerialPortError SPE
    )
{
    //Serial port has an error
    if (SPE != QSerialPort::NoError)
    {
        if (mbIsRunning == true)
        {
            //Stop running script and display error
            on_btn_Stop_clicked();
            msbStatusBar->showMessage("Script stopped due to serial port error.");
        }
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::SerialPortData(
    const QByteArray *Data
    )
{
    //Serial port data received
    if (mbIsRunning == true)
    {
        //Append data
        mbaRecvData.append(*Data);
        if (mbWaitingForReceive == true)
        {
            //Check if there is a match for this line
            AdvanceLine();
        }
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::on_btn_Stop_clicked(
    )
{
    //Stop execution
    if (mbIsRunning == true)
    {
        //Stop execution and timer
        mbIsRunning = false;
        if (mtmrPauseTimer.isActive())
        {
            mtmrPauseTimer.stop();
        }

        //Stop update timer update if running
        if (mtmrUpdateTimer.isActive())
        {
            mtmrUpdateTimer.stop();
        }

        //Clear buffers
        mbaRecvData.clear();
        mbaMatchData.clear();
        mbWaitingForReceive = false;

        //Disable read only mode of editor
        SetButtonStatus(true);

        if (gtmrRecTimer.isValid())
        {
            //Invalidate receive timer
            gtmrRecTimer.invalidate();
        }

        //Update status bar
        QString strRepeatText;
        if (mnRepeats > 0)
        {
            //Create text for repeat information
            strRepeatText = QString(" with ").append(QString::number(mnRepeats)).append(" repeat").append(mnRepeats == 1 ? "" : "s").append(", ~");
            double fElapsed = (double)gtmrScriptTimer.elapsed()/1000.0;
            if (fElapsed == 0.0)
            {
                //Avoid division by zero
                fElapsed = 1.0;
            }
            strRepeatText.append(QString::number(fElapsed/(double)mnRepeats, 'f', 1)).append(" seconds/loop");
        }

        //Show script stopped message
        msbStatusBar->showMessage(QString("Script stopped after ~").append(QString::number(((double)gtmrScriptTimer.elapsed()/1000.0), 'f', 1)).append(" seconds").append(strRepeatText));
        gtmrScriptTimer.invalidate();
        ui->edit_Script->SetExecutionLineStatus(true);

        //Return window title
        setWindowTitle("Scripting");

        //Notify main form that script is no longer executing
        emit ScriptFinished();
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::AdvanceLine(
    )
{
    //Executes the next script line
    if (mtmrUpdateTimer.isActive() && ucLastAct != ScriptingActionDataIn)
    {
        //Stop pause update timer
        mtmrUpdateTimer.stop();
    }

    while (mtbExecutionBlock.isValid())
    {
        //Execution block exists
        ui->edit_Script->SetExecutionLine(mintCLine);

        if (mbIsRunning == false)
        {
            //Cancelled
            return;
        }
        else if (ui->btn_Pause->isChecked())
        {
            //Execution is paused
            return;
        }

        if (mtbExecutionBlock.text().length() > 0)
        {
            //Some data is present
            if (mtbExecutionBlock.text().at(0) == ScriptingDataOut)
            {
                //Clear receive buffer and send data out
                mbaRecvData.clear();
                QByteArray baTmpDat(mtbExecutionBlock.text().right(mtbExecutionBlock.text().length()-1).toUtf8());
                AutEscape::escape_characters(&baTmpDat);

                //Set the number of bytes remaining to be written to the length of the string (only used if the WaitForWrite checkout is enabled)
                mbBytesWriteRemain = QString(baTmpDat).length();

                //Pass the data back to the main form
                emit SendData(baTmpDat, false, true);

                ucLastAct = ScriptingActionDataOut;

                if (ui->check_WaitForWrite->isChecked() == true)
                {
                    //Wait for data to leave the buffer
                    UpdateStatusBar();
                    return;
                }
            }
            else if (mtbExecutionBlock.text().at(0) == ScriptingDataIn)
            {
                //Receive
                mbaMatchData = mtbExecutionBlock.text().right(mtbExecutionBlock.text().length()-1).toUtf8();
                AutEscape::escape_characters(&mbaMatchData);
                ucLastAct = ScriptingActionDataIn;

                if (!gtmrRecTimer.isValid())
                {
                    //Start timer
                    gtmrRecTimer.start();
                    mtmrUpdateTimer.start(500);
                }

                if (CheckRecvMatchBuffers() == false)
                {
                    //Waiting on a match
                    if (mbaRecvData.length() > ui->spin_MaxRecBufSize->value())
                    {
                        //Buffer is too big, clear and fail the script
                        QString strMsg = QString("Script failed (expected data not found after ").append(ui->spin_MaxRecBufSize->text()).append(" bytes (").append(QString::number(mbaRecvData.length())).append(" bytes in buffer) after ");
                        ui->edit_Script->SetExecutionLineStatus(true);
                        on_btn_Stop_clicked();
                        strMsg.append(msbStatusBar->currentMessage().right(msbStatusBar->currentMessage().length()-21));
                        msbStatusBar->showMessage(strMsg);
                        return;
                    }
                    mbWaitingForReceive = true;
                    UpdateStatusBar();
                    return;
                }
            }
            else if (mtbExecutionBlock.text().at(0) == ScriptingWaitTime)
            {
                //Wait for a specified period of time
                mtmrPauseTimer.start(mtbExecutionBlock.text().right(mtbExecutionBlock.text().length()-1).toUInt());
                mtbExecutionBlock = mtbExecutionBlock.next();
                ++mintCLine;
                ucLastAct = ScriptingActionWaitTime;
                UpdateStatusBar();
                mtmrUpdateTimer.start(1000);
                return;
            }
            else
            {
                //Comment line
                ucLastAct = ScriptingActionOther;
            }
        }
        else
        {
            //Blank line
            ucLastAct = ScriptingActionOther;
        }
        UpdateStatusBar();
        mtbExecutionBlock = mtbExecutionBlock.next();
        ++mintCLine;

        if (gtmrRecTimer.isValid())
        {
            //Stop timer and invalidate it
            gtmrRecTimer.invalidate();
            mtmrUpdateTimer.stop();
        }
    }

    //Means execution has finished
    if (ui->spin_Repeats->value() != 0)
    {
        //Script repeating is enabled, check if we need to repeat the script
        if (ui->spin_Repeats->value() == -1 || mnRepeats < ui->spin_Repeats->value())
        {
            //Repeat script and increment loop count
            mintCLine = 0;
            mtbExecutionBlock = ui->edit_Script->document()->firstBlock();
            ++mnRepeats;

            //Clear data buffers
            mbaRecvData.clear();
            mbaMatchData.clear();
            mbWaitingForReceive = false;

            //Advance to the next (first) line
            AdvanceLine();
            return;
        }
    }
    mintCLine = -1;
    ui->edit_Script->SetExecutionLine(mintCLine);
    mbIsRunning = false;

    //Disable read only mode of editor
    SetButtonStatus(true);

    //Update status bar
    QString strRepeatText;
    if (mnRepeats > 0)
    {
        //Create text for repeat information
        strRepeatText = QString(" with ").append(QString::number(mnRepeats)).append(" repeat").append(mnRepeats == 1 ? "" : "s").append(", ~");
        double fElapsed = (double)gtmrScriptTimer.elapsed()/1000.0;
        if (fElapsed == 0.0)
        {
            //Avoid division by zero
            fElapsed = 1.0;
        }
        strRepeatText.append(QString::number(fElapsed/(double)mnRepeats, 'f', 1)).append(" seconds/loop");
    }
    msbStatusBar->showMessage(QString("Script complete after ~").append(QString::number(((double)gtmrScriptTimer.elapsed()/1000.0), 'f', 1)).append(" seconds").append(strRepeatText));
    ui->progress_Complete->setValue(ui->progress_Complete->maximum());
    gtmrScriptTimer.invalidate();

    //Return window title
    setWindowTitle("Scripting");

    //Notify main form that script is no longer executing
    emit ScriptFinished();
}

//=============================================================================
//=============================================================================
void
UwxScripting::on_btn_Help_clicked(
    )
{
    //Display help
    QString strMessage = "AuTerm Scripting: This is a simple scripting language for sending/receiving data and waiting for specific periods of time. Each line must begin directly with a valid command and the parameter for that command. Commands are:\r\n    >  Send data out\r\n    <  Wait to receive data\r\n    ~  Wait for a period (in ms)\r\n    // A null-operation comment (used for describing the code)\r\n\r\nValid commands will be highlighed in red and comments in green. Use the check button (yellow warning icon) to check for syntax errors in a script before running it.\r\n\r\nTo the left side of the editor are individual line colours which will change to indicate the following:\r\n    Red:   Syntax error with line (compile failed)\r\n    Green: Currently executing line\r\n    Black: Script execution failed on this line";
    mFormAuto->SetMessage(&strMessage);
    mFormAuto->show();
}

//=============================================================================
//=============================================================================
void
UwxScripting::SetAuTermVersion(
    const QString strVersion
    )
{
    //Update version string
    mstrAuTermVersion = strVersion;
}

//=============================================================================
//=============================================================================
bool
UwxScripting::CheckRecvMatchBuffers(
    )
{
    //Check if the receive buffer contains the match data
    if (mbaRecvData.length() >= mbaMatchData.length())
    {
        //Length of receive buffer is greater than or equal to the match string, perform search
        int intPos = mbaRecvData.indexOf(mbaMatchData);

        if (intPos != -1 && intPos < ui->spin_MaxRecBufSize->value())
        {
            //Position OK: shift array and progress to next line
            mbaRecvData = mbaRecvData.right(mbaRecvData.length() - intPos - mbaMatchData.length());
            mbWaitingForReceive = false;
            return true;
        }
    }

    //Buffer doesn't contain requested data
    return false;
}

//=============================================================================
//=============================================================================
void
UwxScripting::SerialPortWritten(
    int iWritten
    )
{
    //Serial bytes have been written
    if (mbIsRunning == true && ui->check_WaitForWrite->isChecked() == true)
    {
        //Remove from the expected number of bytes
        mbBytesWriteRemain = mbBytesWriteRemain - iWritten;

        if (mbBytesWriteRemain <= 0)
        {
            //Bytes have been fully written, advance to next line
            mtbExecutionBlock = mtbExecutionBlock.next();
            ++mintCLine;

            //Run the next line
            AdvanceLine();
        }
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::on_btn_Pause_toggled(
    bool
    )
{
    //Pause status has been changed
    if (mbIsRunning == true && !ui->btn_Pause->isChecked())
    {
        if (ucLastAct == ScriptingActionDataIn)
        {
            //Waiting for data to be received
            AdvanceLine();
        }
        else if (ucLastAct == ScriptingActionDataOut)
        {
            //Waiting for data to be written
            if (ui->check_WaitForWrite->isChecked() == true)
            {
                if (mbBytesWriteRemain <= 0)
                {
                    //Bytes have been fully written, advance to next line
                    mtbExecutionBlock = mtbExecutionBlock.next();
                    ++mintCLine;

                    //Run the next line
                    AdvanceLine();
                }
            }
            else
            {
                //Recheck
                AdvanceLine();
            }
        }
        else if (ucLastAct == ScriptingActionWaitTime)
        {
            //Waiting for timer to end
            if (!mtmrPauseTimer.isActive())
            {
                //Recheck
                AdvanceLine();
            }
        }
        else
        {
            //Recheck
            AdvanceLine();
        }
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::UpdateStatusBar(
    )
{
    //Updates status bar with current action
    ui->progress_Complete->setValue((mintCLine-1)*100/mnScriptLines);
    QString strPercent = QString::number(ui->progress_Complete->value()).append("%");
    setWindowTitle(QString("Scripting (Running... ").append(strPercent).append(")"));
    if (ucLastAct == ScriptingActionDataIn)
    {
        //Receiving data
        double dblRecTimeSec = gtmrRecTimer.nsecsElapsed()/1000000000.0;
        if ((int)dblRecTimeSec >= ui->spin_MaxRecTime->value())
        {
            //Time has expired
            on_btn_Stop_clicked();
            msbStatusBar->showMessage(QString("Script failed (expected data not found after ").append(ui->spin_MaxRecTime->text()).append(" seconds).").append((mnRepeats > 0 ? QString(" on repeat #").append(QString::number(mnRepeats)) : "")));
            ui->edit_Script->SetExecutionLineStatus(true);
            setWindowTitle("Scripting");
        }
        else
        {
            //Time left
            msbStatusBar->showMessage(QString("#").append(QString::number(mintCLine+1)).append(": Waiting to receive data (").append(QString::number(mbaRecvData.length())).append(" bytes received in ").append(QString::number(dblRecTimeSec, 'f', 1)).append(" seconds)").append((mnRepeats > 0 ? QString(" with ").append(QString::number(mnRepeats)).append(" repeat").append(mnRepeats == 1 ? "" : "s") : "").append("... (").append(strPercent).append(")")));
        }
    }
    else if (ucLastAct == ScriptingActionDataOut)
    {
        //Data output
        if (ui->check_WaitForWrite->isChecked() == true)
        {
            //Sending data and waiting for it to be flushed
            msbStatusBar->showMessage(QString("#").append(QString::number(mintCLine+1)).append(": Waiting to data to be flushed (").append(QString::number(mbBytesWriteRemain)).append(" bytes remaining)").append((mnRepeats > 0 ? QString(" with ").append(QString::number(mnRepeats)).append(" repeat").append(mnRepeats == 1 ? "" : "s") : "").append("... (").append(strPercent).append(")")));
        }
        else
        {
            //Sending data
            msbStatusBar->showMessage(QString("#").append(QString::number(mintCLine+1)).append(": Outputting data... (").append(strPercent).append(")"));
        }
    }
    else if (ucLastAct == ScriptingActionWaitTime)
    {
        //Wait period
        msbStatusBar->showMessage(QString("#").append(QString::number(mintCLine)).append(": Wait period ").append(QString::number(mtmrPauseTimer.interval())).append("ms (").append(QString::number(mtmrPauseTimer.remainingTime())).append("ms left)").append((mnRepeats > 0 ? QString(" with ").append(QString::number(mnRepeats)).append(" repeat").append(mnRepeats == 1 ? "" : "s") : "").append("... (").append(strPercent).append(")")));
    }
    else
    {
        //Comment or blank line
        msbStatusBar->showMessage(QString("#").append(QString::number(mintCLine+1)).append(": Comment/Unknown line (").append(strPercent).append(")"));
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::closeEvent(
    QCloseEvent *ceClose
    )
{
    if (mbIsRunning == true)
    {
        //Script is running - ignore close event
        ceClose->ignore();
    }
    else
    {
        //Accept close event
        ceClose->accept();
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::on_btn_Options_clicked(
    )
{
    //Show options menu
    gpOptionsMenu->popup(QCursor::pos());
}

//=============================================================================
//=============================================================================
void
UwxScripting::MenuSelected(
    QAction *qaAction
    )
{
    //Menu item selected
    int intItem = qaAction->data().toInt();
    if (intItem == MenuActionChangeFont)
    {
        //Change font
        ChangeFont();
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::on_btn_Clear_clicked(
    )
{
    //Clears the editor
    if (mbIsRunning == false)
    {
        //Check user really wants to lose all data
        QMessageBox mbClearEditor;
        mbClearEditor.setText("Are you sure you want to clear the editor contents? Unsaved changes will be lost.");
        mbClearEditor.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        mbClearEditor.setDefaultButton(QMessageBox::No);

        if (mbClearEditor.exec() == QMessageBox::Yes)
        {
            //Clear text
            ui->edit_Script->clear();
            ui->edit_Script->ClearBadLines();
            mintCLine = -1;
            ui->edit_Script->SetExecutionLine(mintCLine);
        }
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::SetButtonStatus(
    bool bStatus
    )
{
    //Change status of inputs
    ui->edit_Script->setReadOnly(!bStatus);
    ui->check_WaitForWrite->setEnabled(bStatus);
    ui->spin_MaxRecBufSize->setEnabled(bStatus);
    ui->spin_MaxRecTime->setEnabled(bStatus);
    ui->btn_Run->setEnabled(bStatus);
    ui->btn_Stop->setEnabled(!bStatus);
    ui->btn_Clear->setEnabled(bStatus);
    ui->btn_Compile->setEnabled(bStatus);
    ui->btn_Load->setEnabled(bStatus);
    ui->btn_Save->setEnabled(bStatus);
    ui->btn_Options->setEnabled(bStatus);
}

//=============================================================================
//=============================================================================
void
UwxScripting::reject(
    )
{
    //Do not allow the dialog to be closed by pressing escape if a script is running
    if (mbIsRunning == false)
    {
        //Allow close
        QDialog::reject();
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::SetEditorFocus(
    )
{
    //Set input focus to the editor
    ui->edit_Script->setFocus();
}

//=============================================================================
//=============================================================================
void
UwxScripting::ScriptStartResult(
    bool bStatus,
    unsigned char ucReason
    )
{
    //Result from main window if script execution can begin
    if (bStatus == true)
    {
        //OK to start script execution
        mintCLine = 0;
        mtbExecutionBlock = ui->edit_Script->document()->firstBlock();
        mbIsRunning = true;

        //Clear data buffers
        mbaRecvData.clear();
        mbaMatchData.clear();
        mbWaitingForReceive = false;

        //Set editor to be read only
        SetButtonStatus(false);

        //Start timer
        gtmrScriptTimer.start();

        //Show start message
        msbStatusBar->showMessage(QString("Beginning script execution... ").append(QString::number(ui->edit_Script->document()->blockCount())).append(" lines."));

        //Advance to the next (first) line
        AdvanceLine();
    }
    else
    {
        //Terminal is busy or script cannot run now
        msbStatusBar->showMessage(QString("Failed to start script").append((ucReason == ScriptingReasonPortClosed ? ": Serial port is not open." : (ucReason == ScriptingReasonTermBusy ? ": Terminal currently busy." : QString(", error code: ").append(QString::number(ucReason))))));
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::LoadScriptFile(
    const QString *strFilename
    )
{
    //Load file contents
    ui->edit_Script->clear();
    QFile fileLoad(*strFilename);
    if (fileLoad.open(QFile::ReadOnly | QFile::Text))
    {
        //File opened, read contents
        ui->edit_Script->setPlainText(fileLoad.readAll());
        fileLoad.close();
    }
    else
    {
        //Failed to open file
        QString strMessage = QString("Error during scripting file load: Access to selected file is denied: ").append(*strFilename);
        mFormAuto->SetMessage(&strMessage);
        mFormAuto->show();
    }
}

//=============================================================================
//=============================================================================
void
UwxScripting::SetScriptLastDirectory(
    const QString *strDirectory
    )
{
    //Sets the last opened directory
    strLastScriptFile = *strDirectory;
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
