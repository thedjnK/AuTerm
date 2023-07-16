/******************************************************************************
** Copyright (C) 2015-2017 Laird
**
** Project: AuTerm
**
** Module: UwxPopup.cpp
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
#include "UwxPopup.h"
#include "ui_UwxPopup.h"

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
PopupMessage::PopupMessage(QWidget *parent):QDialog(parent), ui(new Ui::PopupMessage)
{
    //Setup window to be a dialog
    this->setWindowFlags((Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint));
    ui->setupUi(this);

    //Change terminal font to a monospaced font
    QFont fntTmpFnt = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->text_Message->setFont(fntTmpFnt);
}

//=============================================================================
//=============================================================================
PopupMessage::~PopupMessage(
    )
{
    delete ui;
}

//=============================================================================
//=============================================================================
void
PopupMessage::on_btn_Close_clicked(
    )
{
    //Close button clicked, close popup.
    this->close();
}

//=============================================================================
//=============================================================================
void PopupMessage::SetMessage(
    QString *strMsg
    )
{
    //Update popup message
    ui->text_Message->setPlainText(*strMsg);
}

void PopupMessage::show_message(QString str_message)
{
    ui->text_Message->setPlainText(str_message);
    this->show();
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
