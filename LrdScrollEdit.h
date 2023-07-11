/******************************************************************************
** Copyright (C) 2015-2022 Laird Connectivity
**
** Project: AuTerm
**
** Module: LrdScrollEdit.h
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
#ifndef LRDSCROLLEDIT_H
#define LRDSCROLLEDIT_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QPlainTextEdit>
#include <QApplication>
#include <QKeyEvent>
#include <QString>
#include <QScrollBar>
#include <QMimeData>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QClipboard>

/******************************************************************************/
// Class definitions
/******************************************************************************/
class LrdScrollEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit
    LrdScrollEdit(
        QWidget *parent = 0
        );
    ~LrdScrollEdit(
        );
    bool
    SetupScrollback(
        quint16 nLines
        );
    void
    SetLineMode(
        bool bNewLineMode
        );
    void
    insertFromMimeData(
        const QMimeData *mdSrc
        );
    void
    UpdateDisplay(
        );
    void
    AddDatInText(
        QByteArray *baDat
        );
    void
    AddDatOutText(
        const QString strDat
        );
    void
    ClearDatIn(
        );
    void
    ClearDatOut(
        );
    QString
    *GetDatOut(
        );
    void
    UpdateCursor(
        );
    void
    SetSerialOpen(
        bool SerialOpen
        );
    void TrimDatIn(
        qint32 intThreshold,
        quint32 intSize
        );

protected:
    bool
    eventFilter(
        QObject *target,
        QEvent *event
        );

signals:
    void
    EnterPressed(
        );
    void
    KeyPressed(
        int nKey,
        QChar chrKeyValue
        );
    void
    FileDropped(
        QString strFilename
        );

private:
    QString *mstrItemArray; //Item text
    quint16 nItemArraySize; //Array size
    unsigned char mchItems; //Number of items
    unsigned char mchPosition; //Current position
    bool mbLineMode; //True enables line mode
    bool mbSerialOpen; //True if serial port is open
    QByteArray mstrDatIn; //Incoming data (previous commands/received data)
    QString mstrDatOut; //Outgoing data (user typed keyboard data)
    int mintCurPos; //Current text cursor position
    unsigned int mintPrevTextSize; //Holds a count of the previous text size
    bool mbSliderShown; //True if the slider moving to the bottom position upon appearing has been ran

public:
    bool mbLocalEcho; //True if local echo is enabled
    bool mbContextMenuOpen; //True when context menu is open
    bool mbLineSeparator; //True if line separator is enabled
};

#endif // LRDSCROLLEDIT_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
