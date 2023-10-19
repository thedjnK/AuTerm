/******************************************************************************
** Copyright (C) 2015-2022 Laird Connectivity
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module: AutScrollEdit.h
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
#ifndef AUTSCROLLEDIT_H
#define AUTSCROLLEDIT_H

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

enum vt100_dual_format_type {
    FORMAT_DUAL_UNSET = 0,
    FORMAT_DUAL_DISABLE,
    FORMAT_DUAL_HALF,
    FORMAT_DUAL_DOUBLE,
};

struct vt100_format_code {
    int32_t start;
    QColor background_color;
    bool background_color_set;
    QColor foreground_color;
    bool foreground_color_set;

    vt100_dual_format_type weight;
    vt100_format_type italic;
    vt100_format_type underline;
    vt100_format_type strikethrough;

    bool clear_formatting;
    uint8_t options;

    vt100_format_type temp;
};

struct display_buffer_struct {
    QByteArray data;
    bool apply_formatting;
};

typedef QList<display_buffer_struct> display_buffer_list;

//QColor col_default = QColor();
const QColor col_black = QColor(0, 0, 0);
const QColor col_red = QColor(255, 0, 0);
const QColor col_green = QColor(0, 255, 0);
const QColor col_yellow = QColor(255, 247, 0);
const QColor col_blue = QColor(0, 0, 255);
const QColor col_magenta = QColor(202, 31, 123);
const QColor col_cyan = QColor(0, 183, 235);
const QColor col_white = QColor(255, 255, 255);
const QColor col_light_gray = QColor(211, 211, 211);
const QColor col_dark_gray = QColor(135, 135, 135);
const QColor col_light_red = QColor(255, 203, 203);
const QColor col_light_green = QColor(203, 255, 203);
const QColor col_light_yellow = QColor(255, 255, 224);
const QColor col_light_blue = QColor(203, 203, 255);
const QColor col_light_magenta = QColor(255, 128, 255);
const QColor col_light_cyan = QColor(224, 225, 225);

/******************************************************************************/
// Class definitions
/******************************************************************************/
class AutScrollEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit AutScrollEdit(QWidget *parent = 0);
    ~AutScrollEdit();
    bool setup_scrollback(quint16 nLines);
    void set_line_mode(bool bNewLineMode);
    void insertFromMimeData(const QMimeData *mdSrc);
    void update_display();
    void add_display_data(display_buffer_list *buffers);
    void add_dat_in_text(QByteArray data);
    void add_dat_out_text(const QString strDat);
    void clear_dat_in();
    void clear_dat_out();
    QString *get_dat_out();
    void update_cursor();
    void set_serial_open(bool SerialOpen);
    void set_trim_settings(uint32_t threshold, uint32_t size);
    void set_vt100_mode(vt100_mode mode);

protected:
    bool eventFilter(QObject *target, QEvent *event);
    bool vt100_process(QString *buffer, QList<vt100_format_code> *format, int32_t *checked_pos);
    void vt100_colour_process(uint32_t code, vt100_format_code *format);
    void vt100_format_apply(QTextCursor *cursor, vt100_format_code *format);
    void vt100_format_combine(vt100_format_code *original, vt100_format_code *merge);

signals:
    void enter_pressed();
    void key_pressed(int nKey, QChar chrKeyValue);
    void vt100_send(QByteArray code);
    void file_dropped(QString strFilename);
    void scrollbar_drag_released();

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
    int32_t dat_in_new_len; //Holds position of QString-version of mstrDatIn where the mstrDatIn ends
    QTextCharFormat last_format; //Last format applied to dat out data
    vt100_mode vt100_control_mode; //VT100 control code mode
    bool had_dat_in_data; //True if there is current data displayed from the dat in buffer
    QTextCharFormat pre_dat_in_format_backup; //Backup of text format prior to dat in text being added
    uint32_t trim_threshold;
    uint32_t trim_size;

public:
    bool mbLocalEcho; //True if local echo is enabled
    bool mbContextMenuOpen; //True when context menu is open
};

#endif // AUTSCROLLEDIT_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
