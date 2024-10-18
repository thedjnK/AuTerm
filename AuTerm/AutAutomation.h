/******************************************************************************
** Copyright (C) 2015-2018 Laird
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module: UwxPopup.h
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
#ifndef AUTAUTOMATION_H
#define AUTAUTOMATION_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QDialog>
#include "UwxPopup.h"
#include <QFileDialog>
#include <QFile>
#include <QStatusBar>
#include <QWheelEvent>

/******************************************************************************/
// Defines
/******************************************************************************/
#define SaveFilesWithUTF8Header        //Define to save files with a UTF8 BOM header

/******************************************************************************/
// Constants
/******************************************************************************/
const quint16 nAutoItemAllow = 200;    //Number of items in the list to allow
const quint16 nAutoItemsOnScreen = 10; //Number of items in the list to allow
const quint16 nAutoWheelScroll = 120;  //Number of lines to scroll per scrolls

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class AutAutomation;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class AutAutomation : public QDialog
{
    Q_OBJECT

public:
    explicit AutAutomation(QWidget *parent = 0);
    ~AutAutomation();
    void SetPopupHandle(PopupMessage *pmNewHandle);
    void ConnectionChange(bool bEnabled);
    void SetFirstLineFocus();
    void LoadFile(QString strLoadFile);

private slots:
    void on_btn_Load_clicked();
    void on_btn_Save_clicked();
    void on_btn_Top_clicked();
    void on_btn_Up_clicked();
    void on_btn_Down_clicked();
    void on_btn_Bottom_clicked();
    void on_btn_Close_clicked();
    void on_btn_Send1_clicked();
    void on_btn_Send2_clicked();
    void on_btn_Send3_clicked();
    void on_btn_Send4_clicked();
    void on_btn_Send5_clicked();
    void on_btn_Send6_clicked();
    void on_btn_Send7_clicked();
    void on_btn_Send8_clicked();
    void on_btn_Send9_clicked();
    void on_btn_Send10_clicked();
    void LoadTextData();
    void on_edit_Line1_editingFinished();
    void on_edit_Line2_editingFinished();
    void on_edit_Line3_editingFinished();
    void on_edit_Line4_editingFinished();
    void on_edit_Line5_editingFinished();
    void on_edit_Line6_editingFinished();
    void on_edit_Line7_editingFinished();
    void on_edit_Line8_editingFinished();
    void on_edit_Line9_editingFinished();
    void on_edit_Line10_editingFinished();
    void ArrayHighest();
    void ArrayPositionUpdate();
    void EnterPressed();
    void on_btn_Clear_clicked();
    void on_check_OnTop_stateChanged(int);
    void wheelEvent(QWheelEvent *event);

signals:
    void SendData(QByteArray baDataString, bool bEscapeString, bool bFromScripting);

private:
    Ui::AutAutomation *ui;
    QWidget *parent_window;
    PopupMessage *mFormAuto; //Holds handle of error message dialogue
    QString mstrAutoItemArray[(nAutoItemAllow+1)]; //Holds the text items
    unsigned char mchItemPosition; //Current position of the array for the text boxes
    unsigned char mchItemHighest; //Highest number in the array with an entry that has data
    QStatusBar *msbStatusBar; //Pointer to automation status bar
};

#endif // AUTAUTOMATION_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
