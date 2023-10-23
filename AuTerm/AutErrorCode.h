/******************************************************************************
** Copyright (C) 2016-2018 Laird
**
** Project: AuTerm
**
** Copyright (C) 2016-2017 Laird
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module: AutErrorCode.h
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
#ifndef AUTERRORCODE_H
#define AUTERRORCODE_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QDialog>
#include <QCompleter>
#include <QSettings>
#include <QFontDatabase>
#include <QStatusBar>
#include <QClipboard>

/******************************************************************************/
// Defines
/******************************************************************************/
#define ERROR_CODE_TREAT_NEGATIVE_AS_POSITIVE

/******************************************************************************/
// Constants
/******************************************************************************/
const qint8 ErrorCodeLookupTab = 0;
const qint8 ErrorCodeListTab   = 1;
const qint8 ErrorCodeSearchTab = 2;

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class AutErrorCode;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class AutErrorCode : public QDialog
{
    Q_OBJECT

public:
    explicit
    AutErrorCode(
        QWidget *parent = 0
        );
    ~AutErrorCode();
    void
    SetErrorObject(
        QSettings *pErrorMessages
        );

public slots:
    void display_error(QString error);

private slots:
    void
    on_combo_Code_currentTextChanged(
        QString strComboText
        );
    void
    on_list_Codes_currentRowChanged(
        int iListRow
        );
    void
    on_list_Search_currentRowChanged(
        int iSearchRow
        );
    void
    SetObjectStatus(
        bool bStatus
        );
    void
    on_selector_Tab_currentChanged(
        int iTabIndex
        );
    void
    on_btn_Copy_clicked(
        );
    void on_edit_Search_textChanged(const QString &arg1);
    void on_btn_order_clicked();

private:
    Ui::AutErrorCode *ui;
    QCompleter *mcmpErrors; //Handle for error completer object for combo box
    QSettings *mpErrorMessages; //Handle for error values object (owned by UwxMainWindow)
    QStatusBar *msbStatusBar; //Pointer to error code status bar
};

#endif // AUTERRORCODE_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
