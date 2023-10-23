/******************************************************************************
** Copyright (C) 2015-2017 Laird
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module: AutAutomation.cpp
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
#include "AutAutomation.h"
#include "ui_AutAutomation.h"

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
AutAutomation::AutAutomation(QWidget *parent) : QDialog(parent), ui(new Ui::AutAutomation)
{
    //On dialogue creation
    ui->setupUi(this);

    //Default values
    mchItemPosition = 0;
    mchItemHighest = 0;

    //Update the position label
    ArrayPositionUpdate();

    //Connect signals
    connect(ui->edit_Line1, SIGNAL(returnPressed()), this, SLOT(EnterPressed()));
    connect(ui->edit_Line2, SIGNAL(returnPressed()), this, SLOT(EnterPressed()));
    connect(ui->edit_Line3, SIGNAL(returnPressed()), this, SLOT(EnterPressed()));
    connect(ui->edit_Line4, SIGNAL(returnPressed()), this, SLOT(EnterPressed()));
    connect(ui->edit_Line5, SIGNAL(returnPressed()), this, SLOT(EnterPressed()));
    connect(ui->edit_Line6, SIGNAL(returnPressed()), this, SLOT(EnterPressed()));
    connect(ui->edit_Line7, SIGNAL(returnPressed()), this, SLOT(EnterPressed()));
    connect(ui->edit_Line8, SIGNAL(returnPressed()), this, SLOT(EnterPressed()));
    connect(ui->edit_Line9, SIGNAL(returnPressed()), this, SLOT(EnterPressed()));

    //Create status bar
    msbStatusBar = new QStatusBar;
    ui->StatusBarLayout->addWidget(msbStatusBar);
    msbStatusBar->showMessage("[Status]");

    //Set always on top state
    parent_window = parent;
    on_check_OnTop_stateChanged(0);
}

//=============================================================================
//=============================================================================
AutAutomation::~AutAutomation(
    )
{
    //On dialogue deletion
    disconnect(ui->edit_Line1, SIGNAL(returnPressed()));
    disconnect(ui->edit_Line2, SIGNAL(returnPressed()));
    disconnect(ui->edit_Line3, SIGNAL(returnPressed()));
    disconnect(ui->edit_Line4, SIGNAL(returnPressed()));
    disconnect(ui->edit_Line5, SIGNAL(returnPressed()));
    disconnect(ui->edit_Line6, SIGNAL(returnPressed()));
    disconnect(ui->edit_Line7, SIGNAL(returnPressed()));
    disconnect(ui->edit_Line8, SIGNAL(returnPressed()));
    disconnect(ui->edit_Line9, SIGNAL(returnPressed()));
    delete msbStatusBar;
    delete ui;
}

//=============================================================================
//=============================================================================
void
AutAutomation::SetPopupHandle(
    PopupMessage *pmNewHandle
    )
{
    //Sets the Popup Message handle
    mFormAuto = pmNewHandle;
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Load_clicked(
    )
{
    //Load button clicked
    QString strLoadFile;
    strLoadFile = QFileDialog::getOpenFileName(this, tr("Open Automation File"), "", tr("Text Files (*.txt);;All Files (*.*)"));
    if (strLoadFile != "")
    {
        //We have a file to load!
        LoadFile(strLoadFile);
    }
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Save_clicked(
    )
{
    //Save button clicked
    QString strSaveFile;
    strSaveFile = QFileDialog::getSaveFileName(this, tr("Save Automation File"), "", tr("Text Files (*.txt);;All Files (*.*)"));
    if (strSaveFile != "")
    {
        //We have a file to load!
        QFile file(strSaveFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            //Unable to open file
            QString strMessage = tr("Error during automation file save: Access to selected file is denied: ").append(strSaveFile);
            mFormAuto->SetMessage(&strMessage);
            mFormAuto->show();
            return;
        }

#ifdef SaveFilesWithUTF8Header
        //Output UTF-8 BOM header
        QByteArray baBOM;
        baBOM[0] = '\xEF'; //Changed from char to constant to stop warning with MSVC compiler
        baBOM[1] = '\xBB';
        baBOM[2] = '\xBF';
        file.write(baBOM);
#endif

        //Save the data
        unsigned char i = 0;
        ArrayHighest();
        while (i <= mchItemHighest)
        {
            file.write(mstrAutoItemArray[i].append("\n").toUtf8());
            ++i;
        }

        //Flush the file output and close the file handke
        file.flush();
        file.close();

        //
        msbStatusBar->showMessage(QString(strSaveFile).append(": ").append(QString::number(i)).append(" line").append((i == 1 ? " saved." : "s saved.")));
    }
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Top_clicked(
    )
{
    //Top button clicked
    mchItemPosition = 0;
    LoadTextData();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Up_clicked(
    )
{
    //Up button clicked
    if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        //Scroll by 10
        if (mchItemPosition >= nAutoItemsOnScreen)
        {
            mchItemPosition = mchItemPosition - nAutoItemsOnScreen;
            LoadTextData();
        }
        else if (mchItemPosition > 0)
        {
            mchItemPosition = 0;
            LoadTextData();
        }
    }
    else
    {
        //Scroll by 1
        if (mchItemPosition > 0)
        {
            --mchItemPosition;
            LoadTextData();
        }
    }
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Down_clicked(
    )
{
    //Down button clicked
    if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        //Scroll by 10, check for 20 as it needs to skip 10 items and display 10 items
        if (mchItemPosition < nAutoItemAllow - (nAutoItemsOnScreen*2))
        {
            mchItemPosition = mchItemPosition + nAutoItemsOnScreen;
            LoadTextData();
        }
        else
        {
            mchItemPosition = nAutoItemAllow - nAutoItemsOnScreen;
            LoadTextData();
        }
    }
    else
    {
        //Scroll by 1
        if (mchItemPosition < nAutoItemAllow - nAutoItemsOnScreen)
        {
            ++mchItemPosition;
            LoadTextData();
        }
    }
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Bottom_clicked(
    )
{
    //Bottom button clicked
    mchItemPosition = nAutoItemAllow - nAutoItemsOnScreen;
    LoadTextData();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Close_clicked(
    )
{
    //Close button clicked
    this->close();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Send1_clicked(
    )
{
    //Send button #1 pressed
    emit SendData(ui->edit_Line1->text().toUtf8(), ui->check_Unescape->isChecked(), false);
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Send2_clicked(
    )
{
    //Send button #2 pressed
    emit SendData(ui->edit_Line2->text().toUtf8(), ui->check_Unescape->isChecked(), false);
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Send3_clicked(
    )
{
    //Send button #3 pressed
    emit SendData(ui->edit_Line3->text().toUtf8(), ui->check_Unescape->isChecked(), false);
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Send4_clicked(
    )
{
    //Send button #4 pressed
    emit SendData(ui->edit_Line4->text().toUtf8(), ui->check_Unescape->isChecked(), false);
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Send5_clicked(
    )
{
    //Send button #5 pressed
    emit SendData(ui->edit_Line5->text().toUtf8(), ui->check_Unescape->isChecked(), false);
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Send6_clicked(
    )
{
    //Send button #6 pressed
    emit SendData(ui->edit_Line6->text().toUtf8(), ui->check_Unescape->isChecked(), false);
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Send7_clicked(
    )
{
    //Send button #7 pressed
    emit SendData(ui->edit_Line7->text().toUtf8(), ui->check_Unescape->isChecked(), false);
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Send8_clicked(
    )
{
    //Send button #8 pressed
    emit SendData(ui->edit_Line8->text().toUtf8(), ui->check_Unescape->isChecked(), false);
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Send9_clicked(
    )
{
    //Send button #9 pressed
    emit SendData(ui->edit_Line9->text().toUtf8(), ui->check_Unescape->isChecked(), false);
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Send10_clicked(
    )
{
    //Send button #10 pressed
    emit SendData(ui->edit_Line10->text().toUtf8(), ui->check_Unescape->isChecked(), false);
}

//=============================================================================
//=============================================================================
void
AutAutomation::LoadTextData(
    )
{
    //Loads the text boxes with the data in the array
    ui->edit_Line1->setText(mstrAutoItemArray[mchItemPosition]);
    ui->edit_Line2->setText(mstrAutoItemArray[(mchItemPosition+1)]);
    ui->edit_Line3->setText(mstrAutoItemArray[(mchItemPosition+2)]);
    ui->edit_Line4->setText(mstrAutoItemArray[(mchItemPosition+3)]);
    ui->edit_Line5->setText(mstrAutoItemArray[(mchItemPosition+4)]);
    ui->edit_Line6->setText(mstrAutoItemArray[(mchItemPosition+5)]);
    ui->edit_Line7->setText(mstrAutoItemArray[(mchItemPosition+6)]);
    ui->edit_Line8->setText(mstrAutoItemArray[(mchItemPosition+7)]);
    ui->edit_Line9->setText(mstrAutoItemArray[(mchItemPosition+8)]);
    ui->edit_Line10->setText(mstrAutoItemArray[(mchItemPosition+9)]);
    ArrayPositionUpdate();
}

//=============================================================================
//=============================================================================
void
AutAutomation::ConnectionChange(
    bool bEnabled
    )
{
    //Enable or disable interactivity of the window
    ui->btn_Send1->setEnabled(bEnabled);
    ui->btn_Send2->setEnabled(bEnabled);
    ui->btn_Send3->setEnabled(bEnabled);
    ui->btn_Send4->setEnabled(bEnabled);
    ui->btn_Send5->setEnabled(bEnabled);
    ui->btn_Send6->setEnabled(bEnabled);
    ui->btn_Send7->setEnabled(bEnabled);
    ui->btn_Send8->setEnabled(bEnabled);
    ui->btn_Send9->setEnabled(bEnabled);
    ui->btn_Send10->setEnabled(bEnabled);
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_edit_Line1_editingFinished(
    )
{
    //Update array with new text
    mstrAutoItemArray[mchItemPosition] = ui->edit_Line1->text();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_edit_Line2_editingFinished(
    )
{
    //Update array with new text
    mstrAutoItemArray[(mchItemPosition+1)] = ui->edit_Line2->text();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_edit_Line3_editingFinished(
    )
{
    //Update array with new text
    mstrAutoItemArray[(mchItemPosition+2)] = ui->edit_Line3->text();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_edit_Line4_editingFinished(
    )
{
    //Update array with new text
    mstrAutoItemArray[(mchItemPosition+3)] = ui->edit_Line4->text();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_edit_Line5_editingFinished(
    )
{
    //Update array with new text
    mstrAutoItemArray[(mchItemPosition+4)] = ui->edit_Line5->text();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_edit_Line6_editingFinished(
    )
{
    //Update array with new text
    mstrAutoItemArray[(mchItemPosition+5)] = ui->edit_Line6->text();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_edit_Line7_editingFinished(
    )
{
    //Update array with new text
    mstrAutoItemArray[(mchItemPosition+6)] = ui->edit_Line7->text();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_edit_Line8_editingFinished(
    )
{
    //Update array with new text
    mstrAutoItemArray[(mchItemPosition+7)] = ui->edit_Line8->text();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_edit_Line9_editingFinished(
    )
{
    //Update array with new text
    mstrAutoItemArray[(mchItemPosition+8)] = ui->edit_Line9->text();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_edit_Line10_editingFinished(
    )
{
    //Update array with new text
    mstrAutoItemArray[(mchItemPosition+9)] = ui->edit_Line10->text();
}

//=============================================================================
//=============================================================================
void
AutAutomation::ArrayHighest(
    )
{
    //Gets the highest position for a valid entry in the array
    unsigned char i = 0;
    mchItemHighest = 0;
    while (i < nAutoItemAllow)
    {
        if (mstrAutoItemArray[i] != "" )
        {
            mchItemHighest = i;
        }
        ++i;
    }
}

//=============================================================================
//=============================================================================
void
AutAutomation::ArrayPositionUpdate(
    )
{
    //Updates the label to show the current array position
    ui->label_Position->setText(QString('(').append(QString::number(mchItemPosition)).append('/').append(QString::number(nAutoItemAllow - nAutoItemsOnScreen)).append(')'));
}

//=============================================================================
//=============================================================================
void
AutAutomation::EnterPressed(
    )
{
    //Enter has been pressed with a line edit selected
    if (ui->edit_Line1->hasFocus())
    {
        emit SendData(ui->edit_Line1->text().toUtf8(), ui->check_Unescape->isChecked(), false);
    }
    else if (ui->edit_Line2->hasFocus())
    {
        emit SendData(ui->edit_Line2->text().toUtf8(), ui->check_Unescape->isChecked(), false);
    }
    else if (ui->edit_Line3->hasFocus())
    {
        emit SendData(ui->edit_Line3->text().toUtf8(), ui->check_Unescape->isChecked(), false);
    }
    else if (ui->edit_Line4->hasFocus())
    {
        emit SendData(ui->edit_Line4->text().toUtf8(), ui->check_Unescape->isChecked(), false);
    }
    else if (ui->edit_Line5->hasFocus())
    {
        emit SendData(ui->edit_Line5->text().toUtf8(), ui->check_Unescape->isChecked(), false);
    }
    else if (ui->edit_Line6->hasFocus())
    {
        emit SendData(ui->edit_Line6->text().toUtf8(), ui->check_Unescape->isChecked(), false);
    }
    else if (ui->edit_Line7->hasFocus())
    {
        emit SendData(ui->edit_Line7->text().toUtf8(), ui->check_Unescape->isChecked(), false);
    }
    else if (ui->edit_Line8->hasFocus())
    {
        emit SendData(ui->edit_Line8->text().toUtf8(), ui->check_Unescape->isChecked(), false);
    }
    else if (ui->edit_Line9->hasFocus())
    {
        emit SendData(ui->edit_Line9->text().toUtf8(), ui->check_Unescape->isChecked(), false);
    }
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_btn_Clear_clicked(
    )
{
    //Clears the array and all edit boxes and resets back to default
    unsigned char i = 0;
    while (i < nAutoItemAllow)
    {
        mstrAutoItemArray[i] = "";
        ++i;
    }
    mchItemPosition = 0;
    mchItemHighest = 0;
    ui->edit_Line1->setText("");
    ui->edit_Line2->setText("");
    ui->edit_Line3->setText("");
    ui->edit_Line4->setText("");
    ui->edit_Line5->setText("");
    ui->edit_Line6->setText("");
    ui->edit_Line7->setText("");
    ui->edit_Line8->setText("");
    ui->edit_Line9->setText("");
    ui->edit_Line10->setText("");

    //Update array position text
    ArrayPositionUpdate();
}

//=============================================================================
//=============================================================================
void
AutAutomation::on_check_OnTop_stateChanged(
    int
    )
{
    //Always on-top state changed
    bool bReShow = this->isVisible();

    this->setParent((ui->check_OnTop->isChecked() ? parent_window : nullptr));
    this->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

    if (bReShow == true)
    {
        //Repaint (show) window
        this->show();
    }
}

//=============================================================================
//=============================================================================
void
AutAutomation::SetFirstLineFocus(
    )
{
    //Sets the focus to the first line edit
    ui->edit_Line1->setFocus();
}

//=============================================================================
//=============================================================================
void
AutAutomation::LoadFile(
    QString strLoadFile
    )
{
    QFile file(strLoadFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        //Unable to open file
        QString strMessage = tr("Error during automation file open: Access to selected file is denied: ").append(strLoadFile);
        mFormAuto->SetMessage(&strMessage);
        mFormAuto->show();
        return;
    }

    //Clear out the array
    unsigned char i = 0;
    while (i < nAutoItemAllow)
    {
        mstrAutoItemArray[i] = "";
        ++i;
    }

    //Read the file data
    i = 0;
    while (!file.atEnd())
    {
        QByteArray baThisLine = file.readLine();
        mstrAutoItemArray[i] = baThisLine.replace("\r", "").replace("\n", "");
        ++i;
        if (i > nAutoItemAllow)
        {
            //Maximum lines reached, stop processing
            --i;
            break;
        }
    }

    //Close the file handle
    file.close();

    //Show number of lines loaded
    msbStatusBar->showMessage(QString(strLoadFile).append(": ").append(QString::number(i)).append(" line").append((i == 1 ? " loaded." : "s loaded.")));

    //Update the text boxes
    mchItemPosition = 0;
    LoadTextData();
}

//=============================================================================
//=============================================================================
void
AutAutomation::wheelEvent(
    QWheelEvent *event
    )
{
    if (event->angleDelta().y())
    {
        qint16 nLines = (event->angleDelta().y() / nAutoWheelScroll);

        if (ui->edit_Line1->hasFocus())
        {
            on_edit_Line1_editingFinished();
        }
        else if (ui->edit_Line2->hasFocus())
        {
            on_edit_Line2_editingFinished();
        }
        else if (ui->edit_Line3->hasFocus())
        {
            on_edit_Line3_editingFinished();
        }
        else if (ui->edit_Line4->hasFocus())
        {
            on_edit_Line4_editingFinished();
        }
        else if (ui->edit_Line5->hasFocus())
        {
            on_edit_Line5_editingFinished();
        }
        else if (ui->edit_Line6->hasFocus())
        {
            on_edit_Line6_editingFinished();
        }
        else if (ui->edit_Line7->hasFocus())
        {
            on_edit_Line7_editingFinished();
        }
        else if (ui->edit_Line8->hasFocus())
        {
            on_edit_Line8_editingFinished();
        }
        else if (ui->edit_Line9->hasFocus())
        {
            on_edit_Line9_editingFinished();
        }
        else if (ui->edit_Line10->hasFocus())
        {
            on_edit_Line10_editingFinished();
        }

        if (nLines > 0)
        {
            //Scroll up
            if (mchItemPosition > abs(nLines))
            {
                mchItemPosition = mchItemPosition - nLines;
                LoadTextData();
            }
            else if (mchItemPosition > 0)
            {
                mchItemPosition = 0;
                LoadTextData();
            }
        }
        else if (nLines < 0)
        {
            //Scroll down
            if (mchItemPosition < nAutoItemAllow - nAutoItemsOnScreen - abs(nLines))
            {
                mchItemPosition = mchItemPosition-nLines;
                LoadTextData();
            }
            else
            {
                mchItemPosition = nAutoItemAllow - nAutoItemsOnScreen;
                LoadTextData();
            }
        }
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
