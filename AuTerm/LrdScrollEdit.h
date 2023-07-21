/******************************************************************************
** Copyright (C) 2015-2022 Laird Connectivity
** Copyright (C) 2023 Jamie M.
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

enum vt100_mode {
    VT100_MODE_IGNORE = 0,
    VT100_MODE_STRIP,
    VT100_MODE_DECODE,
};

enum vt100_format_type {
    FORMAT_UNSET = 0,
    FORMAT_DISABLE,
    FORMAT_ENABLE,
};

struct vt100_format_code {
    uint32_t start;
    QColor background_color;
    bool background_color_set;
    QColor foreground_color;
    bool foreground_color_set;

    vt100_format_type bold;
    vt100_format_type dim;
    vt100_format_type italic;
    vt100_format_type underline;

    bool clear_formatting;
    uint8_t options;
};

//QColor col_default = QColor();
const QColor col_black = QColor(0, 0, 0);
const QColor col_red = QColor(255, 0, 0);
const QColor col_green = QColor(0, 255, 0);
const QColor col_yellow = QColor(255, 247, 0);
const QColor col_blue = QColor(0, 0, 255);
const QColor col_magenta = QColor(202, 31, 123);
const QColor col_cyan = QColor(0, 183, 235);
const QColor col_light_gray = QColor(211, 211, 211);
const QColor col_dark_gray = QColor(135, 135, 135);
const QColor col_light_red = QColor(255, 203, 203);
const QColor col_light_green = QColor(203, 255, 203);
const QColor col_light_yellow = QColor(255, 255, 224);
const QColor col_light_blue = QColor(203, 203, 255);
const QColor col_light_magenta = QColor(255, 128, 255);
const QColor col_light_cyan = QColor(224, 225, 225);
const QColor col_white = QColor(255, 255, 255);

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
        QByteArray *baDat,
        bool apply_formatting
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
    void
    set_vt100_mode(
        vt100_mode mode
        );

protected:
    bool
    eventFilter(
        QObject *target,
        QEvent *event
        );
    bool
    vt100_process(
        QString *buffer,
        int32_t start,
        QList<vt100_format_code> *format,
        int32_t *checked_pos
        );
    bool
    vt100_process_no_rem(
        QString *buffer,
        int32_t start,
        QList<vt100_format_code> *format,
        int32_t *checked_pos
        );
    void
    vt100_colour_process(
        uint32_t code,
        vt100_format_code *format
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
    uint32_t mintPrevTextSize; //Holds a count of the previous text size
    bool mbSliderShown; //True if the slider moving to the bottom position upon appearing has been ran
    bool dat_out_updated; //True if mstrDatOut has been updated and needs redrawing
    int32_t dat_in_prev_check_len; //Holds position of mstrDatIn where hex character escaping was last performed to
    int32_t dat_in_new_len; //Holds position of QString-version of mstrDatIn where the mstrDatIn ends
    QTextCharFormat last_format; //Last format applied to dat out data
    vt100_mode vt100_control_mode; //VT100 control code mode

public:
    bool mbLocalEcho; //True if local echo is enabled
    bool mbContextMenuOpen; //True when context menu is open
    bool mbLineSeparator; //True if line separator is enabled
};

#endif // LRDSCROLLEDIT_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
