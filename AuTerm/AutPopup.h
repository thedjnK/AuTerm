/******************************************************************************
** Copyright (C) 2015-2017 Laird
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module: AutPopup.h
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
#ifndef UWXPOPUPMESSAGE_H
#define UWXPOPUPMESSAGE_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QDialog>

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class PopupMessage;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class PopupMessage : public QDialog
{
    Q_OBJECT

public:
    explicit PopupMessage(QWidget *parent = 0);
    ~PopupMessage();
    void SetMessage(QString *strMsg);

private slots:
    void on_btn_Close_clicked();

public slots:
    void show_message(QString str_message);

private:
    Ui::PopupMessage *ui;
};

#endif // UWXPOPUPMESSAGE_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
