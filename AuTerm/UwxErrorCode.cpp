/******************************************************************************
** Copyright (C) 2016-2017 Laird
**
** Project: AuTerm
**
** Module: UwxErrorCode.cpp
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
#include "UwxErrorCode.h"
#include "ui_UwxErrorCode.h"

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
UwxErrorCode::UwxErrorCode(QWidget *parent) : QDialog(parent), ui(new Ui::UwxErrorCode)
{
    //Setup UI
    ui->setupUi(this);

    //Set complete object to null
    mcmpErrors = 0;

    //Setup fonts
    QFont fntTmpFnt = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->list_Search->setFont(fntTmpFnt);
    ui->list_Codes->setFont(fntTmpFnt);

    //Create status bar
    msbStatusBar = new QStatusBar;
    ui->StatusBarLayout->addWidget(msbStatusBar);
    msbStatusBar->showMessage("");

    //Remove question mark button from window title
    this->setWindowFlags((Qt::Window | Qt::WindowCloseButtonHint));
}

//=============================================================================
//=============================================================================
UwxErrorCode::~UwxErrorCode()
{
    //Clear up
    if (mcmpErrors != 0)
    {
        //Delete completer object
        delete mcmpErrors;
        mcmpErrors = 0;
    }
    delete msbStatusBar;
    delete ui;
}

//=============================================================================
//=============================================================================
void
UwxErrorCode::on_combo_Code_currentTextChanged(
    const QString &strComboText
    )
{
    //Error code entered into combo box
    unsigned int uiErrCode = QString("0x").append(strComboText).toUInt(NULL, 16);
    QString strErrorCode = QString::number(uiErrCode);
    if (mpErrorMessages->value(strErrorCode).isValid())
    {
        //Valid error
        ui->edit_Result->setText(mpErrorMessages->value(strErrorCode).toString());
    }
    else
    {
        //Invalid error
        ui->edit_Result->setText("Invalid.");
    }
    ui->edit_Result->setCursorPosition(0);
}

//=============================================================================
//=============================================================================
void
UwxErrorCode::on_list_Codes_currentRowChanged(
    int iListRow
    )
{
    //Selection in full error list object changed
    if (iListRow >= 0)
    {
        //Item selected
        ui->edit_Result->setText(ui->list_Codes->item(iListRow)->text());
    }
    else
    {
        //No item selected
        ui->edit_Result->setText("Invalid.");
    }
    ui->edit_Result->setCursorPosition(0);
}

//=============================================================================
//=============================================================================
void
UwxErrorCode::on_edit_Search_returnPressed(
    )
{
    //Search for error
    QString SearchStr = ui->edit_Search->text().toLower();
    ui->list_Search->clear();
    foreach (const QString &strKey, mpErrorMessages->childKeys())
    {
        if (mpErrorMessages->value(strKey).toString().toLower().indexOf(SearchStr) != -1)
        {
            //Potential match. One short at a time, convert the data to hex
            quint16 ThisByte = strKey.toUInt();
            QString strThisHex;
            strThisHex.setNum(ThisByte, 16);
            strThisHex = strThisHex.toUpper();
            while (strThisHex.length() < 4)
            {
                //Expand to 4 characters
                strThisHex.prepend('0');
            }

            //Add error to list
            ui->list_Search->addItem(QString(strThisHex).append(": ").append(mpErrorMessages->value(strKey).toString()));
        }
    }

    //Sort items in ascending order
    ui->list_Search->sortItems(Qt::AscendingOrder);
}

//=============================================================================
//=============================================================================
void
UwxErrorCode::on_btn_Search_clicked(
    )
{
    //Search button pressed
    on_edit_Search_returnPressed();
}

//=============================================================================
//=============================================================================
void
UwxErrorCode::SetErrorObject(
    QSettings *pErrorMessages
    )
{
    //Update the error code objects
    if (mcmpErrors != 0)
    {
        //Remove old object
        ui->combo_Code->setCompleter(0);
        delete mcmpErrors;
        mcmpErrors = 0;
    }

    if (pErrorMessages != 0 && !pErrorMessages->value("Version").isNull())
    {
        //Mark errors as loaded
        mpErrorMessages = pErrorMessages;

        //Show error code file version
        msbStatusBar->showMessage(QString("Error code file version ").append(mpErrorMessages->value("Version", "Unknown").toString()));

        //String list to store error code values in
        QStringList slErrorCodes;

        //Iterate through all values
        foreach (const QString &strKey, mpErrorMessages->childKeys())
        {
            //Check if this key is the version string
            if (strKey[0] != 'V')
            {
                //One short at a time, convert the data to hex
                quint16 ThisByte = strKey.toUInt();
                QString strThisHex;
                strThisHex.setNum(ThisByte, 16);
                strThisHex = strThisHex.toUpper();
                while (strThisHex.length() < 4)
                {
                    //Expand to 4 characters
                    strThisHex.prepend('0');
                }

                //Append error code to list of error codes
                slErrorCodes << strThisHex;

                //Add error code to listview
                ui->list_Codes->addItem(QString(strThisHex).append(": ").append(mpErrorMessages->value(strKey).toString()));
            }
        }

        //Sort all items in ascending order
        ui->list_Codes->sortItems(Qt::AscendingOrder);

        //Create a new completer and apply it to the combo box
        mcmpErrors = new QCompleter(slErrorCodes, this);
        mcmpErrors->setCaseSensitivity(Qt::CaseInsensitive);
        ui->combo_Code->setCompleter(mcmpErrors);

        //Enable all options
        SetObjectStatus(true);
    }
    else
    {
        //Error code file not loaded
        msbStatusBar->showMessage(QString("Error code file not present, download from the update tab."));
        ui->list_Codes->clear();
        ui->list_Search->clear();
        ui->edit_Result->clear();
        SetObjectStatus(false);
    }
}

//=============================================================================
//=============================================================================
void
UwxErrorCode::on_list_Search_currentRowChanged(
    int iSearchRow
    )
{
    //Search list object row selection changed
    if (iSearchRow >= 0)
    {
        //Item selected
        ui->edit_Result->setText(ui->list_Search->item(iSearchRow)->text());
    }
    else
    {
        //No item selected
        ui->edit_Result->setText("Invalid.");
    }
    ui->edit_Result->setCursorPosition(0);
}

//=============================================================================
//=============================================================================
void
UwxErrorCode::SetObjectStatus(
    bool bStatus
    )
{
    //Enable or disable the form items
    ui->btn_Search->setEnabled(bStatus);
    ui->combo_Code->setEnabled(bStatus);
    ui->edit_Search->setEnabled(bStatus);
    ui->list_Codes->setEnabled(bStatus);
    ui->list_Search->setEnabled(bStatus);
    ui->selector_Tab->setEnabled(bStatus);
    ui->btn_Copy->setEnabled(bStatus);
}

//=============================================================================
//=============================================================================
void
UwxErrorCode::on_selector_Tab_currentChanged(
    int iTabIndex
    )
{
    //Tab has been changed
    if (iTabIndex == ErrorCodeLookupTab)
    {
        //Get error code from 4-character value
        on_combo_Code_currentTextChanged(ui->combo_Code->currentText());
    }
    else if (iTabIndex == ErrorCodeListTab)
    {
        //Get error code from full list
        on_list_Codes_currentRowChanged(ui->list_Codes->currentRow());
    }
    else if (iTabIndex == ErrorCodeSearchTab)
    {
        //Get error code from search
        on_list_Search_currentRowChanged(ui->list_Search->currentRow());
    }
}

//=============================================================================
//=============================================================================
void
UwxErrorCode::on_btn_Copy_clicked(
    )
{
    //Copy button clicked
    if (!ui->edit_Result->text().isEmpty())
    {
        //Copy to clipboard
        QApplication::clipboard()->setText(ui->edit_Result->text());
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
