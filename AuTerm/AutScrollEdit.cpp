/******************************************************************************
** Copyright (C) 2015-2022 Laird Connectivity
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module: AutScrollEdit.cpp
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
#include "AutScrollEdit.h"
#include "AutEscape.h"
#include <QRegularExpression>
#include <QTimer>

/******************************************************************************/
// Constants
/******************************************************************************/
const QChar unicode_replacement_char = QChar(0xfffd);

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
QTextCharFormat pre;
AutScrollEdit::AutScrollEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    //Enable an event filter
    installEventFilter(this);
    mintPrevTextSize = 0;
    mchItems = 0; //Number of items is 0
    mchPosition = 0; //Current position is 0
    mbLineMode = true; //Line mode is on by default
    mbSerialOpen = false; //Serial port is not open by default
    mbLocalEcho = true; //Local echo mode on by default
    mstrDatIn.clear(); //Data in is an empty string
    mstrDatOut = ""; //Data out is empty string
    mintCurPos = 0; //Current cursor position is 0
    mbContextMenuOpen = false; //Context menu not currently open
    mstrItemArray = NULL;
    nItemArraySize = 0;
    mbSliderShown = false;
    dat_out_updated = false;
    dat_in_new_len = 0;
    had_dat_in_data = false;
    trim_threshold = 0;
    trim_size = 0;

    mstrDatIn.reserve(32768);

    pre = this->textCursor().charFormat();
    last_format = pre;

    AutEscape::do_setup();
}

void AutScrollEdit::vt100_colour_process(uint32_t code, vt100_format_code *format)
{
    const QColor *tmp_col;

    if ((code >= 30 && code <= 37) || (code >= 40 && code <= 47) || (code >= 90 && code <= 97) || (code >= 100 && code <= 107))
    {
        switch (code)
        {
            case 30:
            case 40:
            {
                tmp_col = &col_black;
                break;
            }
            case 31:
            case 41:
            {
                tmp_col = &col_red;
                break;
            }
            case 32:
            case 42:
            {
                tmp_col = &col_green;
                break;
            }
            case 33:
            case 43:
            {
                tmp_col = &col_yellow;
                break;
            }
            case 34:
            case 44:
            {
                tmp_col = &col_blue;
                break;
            }
            case 35:
            case 45:
            {
                tmp_col = &col_magenta;
                break;
            }
            case 36:
            case 46:
            {
                tmp_col = &col_cyan;
                break;
            }
            case 37:
            case 47:
            {
                tmp_col = &col_white;
                break;
            }
            case 90:
            case 100:
            {
                tmp_col = &col_dark_gray;
                break;
            }
            case 91:
            case 101:
            {
                tmp_col = &col_light_red;
                break;
            }
            case 92:
            case 102:
            {
                tmp_col = &col_light_green;
                break;
            }
            case 93:
            case 103:
            {
                tmp_col = &col_light_yellow;
                break;
            }
            case 94:
            case 104:
            {
                tmp_col = &col_light_blue;
                break;
            }
            case 95:
            case 105:
            {
                tmp_col = &col_light_magenta;
                break;
            }
            case 96:
            case 106:
            {
                tmp_col = &col_light_cyan;
                break;
            }
            case 97:
            case 107:
            default:
            {
                tmp_col = &col_light_gray;
                break;
            }
        };

        if ((code >= 30 && code <= 37) || (code >= 90 && code <= 97))
        {
            format->foreground_color = *tmp_col;
            format->foreground_color_set = true;
        }
        else
        {
            format->background_color = *tmp_col;
            format->background_color_set = true;
        }
    }
    else if ((code >= 1 && code <= 9) || (code >= 22 && code <= 29))
    {
        switch (code)
        {
            case 1:
            {
                format->weight = FORMAT_DUAL_DOUBLE;
                break;
            }
            case 2:
            {
                format->weight = FORMAT_DUAL_HALF;
                break;
            }
            case 3:
            {
                format->italic = FORMAT_ENABLE;
                break;
            }
            case 4:
            {
                format->underline = FORMAT_ENABLE;
                break;
            }
            case 9:
            {
                format->strikethrough = FORMAT_ENABLE;
                break;
            }
            case 22:
            {
                format->weight = FORMAT_DUAL_DISABLE;
                break;
            }
            case 23:
            {
                format->italic = FORMAT_DISABLE;
                break;
            }
            case 24:
            {
                format->underline = FORMAT_DISABLE;
                break;
            }
            case 29:
            {
                format->strikethrough = FORMAT_DISABLE;
                break;
            }
        };
    }
    else if (code == 0)
    {
        format->clear_formatting = true;
    }
    else if (code == 9999)
    {
        format->temp = FORMAT_ENABLE;
    }
    else if (code == 9998)
    {
        format->temp = FORMAT_DISABLE;
    }
}

/* Searches for VT100 format codes and extracts them, removing the original
 * formatting codes and adding the formatting codes to a list. Returns true if
 * full input buffer was process, returns false if it could not be fully
 * checked due to insufficient data, in which case `checked_pos` will be
 * updated with the length of the data (after removals) that has been checked
 */
bool AutScrollEdit::vt100_process(QString *buffer, QList<vt100_format_code> *formats, int32_t *checked_pos)
{
    int32_t start = 0;
    int32_t l = buffer->length();
    vt100_format_code tmp_format;
    int32_t prev_pos = -1;

    while (start < l)
    {
        if (buffer->at(start) == (QChar)0x1b)
        {
            int32_t pos = start + 1;

            if (pos >= l)
            {
                //Not enough bytes to check
                *checked_pos = start;
                return false;
            }

            if (buffer->at(pos) == '[')
            {
                bool found_digit = false;
                uint32_t format = 0;
                ++pos;
                memset(&tmp_format, 0, sizeof(tmp_format));

                while (pos < l && (buffer->at(pos) == ';' || (buffer->at(pos) >= '0' && buffer->at(pos) <= '9')))
                {
                    if (buffer->at(pos) == ';')
                    {
                        if (found_digit == true)
                        {
                            vt100_colour_process(format, &tmp_format);
                            format = 0;
                            found_digit = false;
                        }
                        else
                        {
                            buffer->remove(start, 1);
                            --l;
                            break;
                        }
                    }
                    else
                    {
                        found_digit = true;
                        format = (format * 10) + (uint32_t)(buffer->at(pos).toLatin1() - '0');
                    }

                    ++pos;
                }

                if ((pos - start) > 10)
                {
                    //Too long, probably garbage
                    buffer->remove(start, 1);
                    --l;
                    continue;
                }
                else if (pos >= buffer->length())
                {
                    //Not enough bytes to check
                    *checked_pos = start;
                    return false;
                }
                else if (buffer->at(pos) == 'm')
                {
                    if (found_digit == false)
                    {
                        //assume that no data means clear formatting?
                        format = 0;
                    }
                    else if (prev_pos == start)
                    {
                        //Append to existing one by merging the two
                        vt100_format_combine(&tmp_format, &formats->last());
                        formats->pop_back();
                    }

                    vt100_colour_process(format, &tmp_format);

                    tmp_format.start = start;
                    formats->append(tmp_format);
                    buffer->remove(start, (pos - start + 1));
                    l -= (pos - start + 1);
                    prev_pos = start;
                    continue;
                }
                else if (buffer->at(pos) == 'C')
                {
                    //Replace with spaces
                    if (found_digit == true)
                    {
                        int32_t changes = (pos - start + 1) - format;
                        buffer->replace(start, (pos - start + 1), QString(" ").repeated(format));
                        l -= changes;
                        start -= changes;
                        continue;
                    }
                    else
                    {
                        //todo?
                    }
                }
                else
                {
                    buffer->remove(start, 1);
                    --l;
                    continue;
                }

                start = pos;
            }
            else
            {
                buffer->remove(start, 1);
                --l;
                continue;
            }
        }

        ++start;
    }

    return true;
}

AutScrollEdit::~AutScrollEdit()
{
    //Destructor
    delete[] mstrItemArray;
    nItemArraySize = 0;
}

bool AutScrollEdit::setup_scrollback(quint16 nLines)
{
    //Sets up the scrollback array
    mstrItemArray = new QString[nLines+1];
    nItemArraySize = nLines;
    return (mstrItemArray != NULL);
}

bool AutScrollEdit::eventFilter(QObject *target, QEvent *event)
{
    if (target == this->verticalScrollBar())
    {
        //Vertical scroll bar event
        if (event->type() == QEvent::MouseButtonRelease)
        {
            //Button was released, update display buffer via a timer, as it needs to run after this function returns
            QTimer::singleShot(1, this, [this] () {
                this->update_display();
            });
        }
        else if (event->type() == QEvent::UpdateLater && mbSliderShown == false)
        {
            //Slider has been shown, scroll down to the bottom of the text edit if cursor position is at the end
            if (this->textCursor().atEnd() == true)
            {
                this->verticalScrollBar()->setValue(this->verticalScrollBar()->maximum());
            }

            mbSliderShown = true;
        }
        else if (event->type() == QEvent::Hide)
        {
            //Slider has been hidden, clear flag
            mbSliderShown = false;
        }
    }
    else if (event->type() == QEvent::KeyPress)
    {
        //Key has been pressed...
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if ((keyEvent->modifiers() & Qt::ControlModifier) == Qt::ControlModifier)
        {
            //Check if this is a shortcut for cut
            if (QKeySequence(keyEvent->key() | Qt::ControlModifier) == QKeySequence::Cut)
            {
                //We can either disallow cut or treat it as a copy - we will treat it as copy
                QApplication::clipboard()->setText(this->textCursor().selection().toPlainText());
                return true;
            }
        }

        dat_out_updated = true;
        if (mbLineMode == true)
        {
            //Line mode
            if (keyEvent->key() == Qt::Key_Up && !(keyEvent->modifiers() & Qt::ShiftModifier))
            {
                //Up pressed without holding shift
                if (mchPosition > 0)
                {
                    mchPosition = mchPosition-1;
                }

                mstrDatOut = mstrItemArray[mchPosition];
                mintCurPos = mstrDatOut.length();

                this->update_display();

                return true;
            }
            else if (keyEvent->key() == Qt::Key_Down && !(keyEvent->modifiers() & Qt::ShiftModifier))
            {
                //Down pressed without holding shift
                if (mchPosition < mchItems)
                {
                    mchPosition = mchPosition+1;
                }

                mstrDatOut = mstrItemArray[mchPosition];
                mintCurPos = mstrDatOut.length();

                this->update_display();

                return true;
            }
            else if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && !(keyEvent->modifiers() & Qt::ControlModifier) && !(keyEvent->modifiers() & Qt::ShiftModifier))
            {
                //Enter pressed
                if (mbSerialOpen == true)
                {
                    if (mstrDatOut != "" && (mchItems == 0 || mstrDatOut != mstrItemArray[(mchItems - 1)]))
                    {
                        //Previous entry is not the same as this entry
                        if (mchItems > (nItemArraySize - 1))
                        {
                            //Shift out last array item
                            unsigned char i = 1;
                            while (i < nItemArraySize)
                            {
                                mstrItemArray[(i - 1)] = mstrItemArray[i];
                                ++i;
                            }

                            mchItems--;
                        }

                        mstrItemArray[mchItems] = mstrDatOut;
                        mchItems++;
                    }
//todo: only store if successfully accepted
                    mchPosition = mchItems;

                    //Send message to main window
                    emit enter_pressed();
                }
//                this->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
//                mintCurPos = 0;
                return true;
            }
            else if (keyEvent->key() == Qt::Key_Backspace)
            {
                if ((keyEvent->modifiers() & Qt::ControlModifier))
                {
                    //Delete word
                    if (mintCurPos > 0)
                    {
                            quint32 intSpacePos = mintCurPos - 1;
                            bool found_non_space = false;
                            while (intSpacePos > 0)
                            {
                                --intSpacePos;
                                if (mstrDatOut.at(intSpacePos) == ' ')
                                {
                                        //Found a space
                                        if (found_non_space == true)
                                        {
                                            ++intSpacePos;
                                                break;
                                        }
                                }
                                else
                                {
                                        found_non_space = true;
                                }
                        }

                        //Previous word found, remove up to the previous word
                        mstrDatOut.remove(intSpacePos, mintCurPos-intSpacePos);
                        mintCurPos -= mintCurPos-intSpacePos;
                    }
                }
                else if (mintCurPos > 0)
                {
                    //Delete character
                    mstrDatOut.remove((mintCurPos - 1), 1);
                    --mintCurPos;
                }

                this->update_display();
                this->update_cursor();

                return true;
            }
            else if (keyEvent->key() == Qt::Key_Left)
            {
                //Left key pressed
                if (keyEvent->modifiers() & Qt::ControlModifier)
                {
                    //Move to previous word
                    while (mintCurPos > 0)
                    {
                        --mintCurPos;
                        if (mstrDatOut.at(mintCurPos) == ' ' || mstrDatOut.at(mintCurPos) == '\r' || mstrDatOut.at(mintCurPos) == '\n')
                        {
                            //Found a space or newline character
                            break;
                        }
                    }
                    this->update_cursor();
                }
                else if (mintCurPos > 0)
                {
                    //Move left
                    --mintCurPos;
                    this->update_cursor();
                }
                return true;
            }
            else if (keyEvent->key() == Qt::Key_Right)
            {
                //Right key pressed
                if (keyEvent->modifiers() & Qt::ControlModifier)
                {
                    //Move to next word
                    while (mintCurPos < mstrDatOut.length())
                    {
                        ++mintCurPos;
                        if (mintCurPos < mstrDatOut.length() && (mstrDatOut.at(mintCurPos) == ' ' || mstrDatOut.at(mintCurPos) == '\r' || mstrDatOut.at(mintCurPos) == '\n'))
                        {
                            //Found a space or newline character
                            break;
                        }
                    }
                    this->update_cursor();
                }
                else if (mintCurPos < mstrDatOut.length())
                {
                    //Move right
                    ++mintCurPos;
                    this->update_cursor();
                }
                return true;
            }
            else if (keyEvent->key() == Qt::Key_Home)
            {
                //Home key pressed
                if (!(keyEvent->modifiers() & Qt::ControlModifier))
                {
                    //Move to beginning of line
                    mintCurPos = 0;
                    this->update_cursor();
                }
                return true;
            }
            else if (keyEvent->key() == Qt::Key_End)
            {
                //End key pressed
                if (!(keyEvent->modifiers() & Qt::ControlModifier))
                {
                    //Move to end of line
                    mintCurPos = mstrDatOut.length();
                    this->update_cursor();
                }
                return true;
            }
            else if (keyEvent->key() == Qt::Key_Delete)
            {
                //Delete key pressed
                if ((keyEvent->modifiers() & Qt::ControlModifier))
                {
                    //Delete word
                    qint32 intSpacePos = mintCurPos;
                    while (intSpacePos < mstrDatOut.length())
                    {
                        ++intSpacePos;
                        if (mstrDatOut.at(intSpacePos) == ' ')
                        {
                            //Found a space
                            break;
                        }
                    }

                    if (intSpacePos == mstrDatOut.length())
                    {
                        //Next word not found, remove text from current position until end
                        mstrDatOut.remove(mintCurPos, mstrDatOut.length()-mintCurPos);
                    }
                    else
                    {
                        //Next word found, remove up to next word
                        mstrDatOut.remove(mintCurPos, intSpacePos-mintCurPos);
                    }
                }
                else if (mstrDatOut.length() > 0)
                {
                    //Delete character
                    mstrDatOut.remove(mintCurPos, 1);
                }
                this->update_display();
                this->update_cursor();
                return true;
            }
            else if (keyEvent->key() != Qt::Key_Escape && keyEvent->key() != Qt::Key_Backtab && keyEvent->key() != Qt::Key_Backspace && keyEvent->key() != Qt::Key_Insert && keyEvent->key() != Qt::Key_Pause && keyEvent->key() != Qt::Key_Print && keyEvent->key() != Qt::Key_SysReq && keyEvent->key() != Qt::Key_Clear && keyEvent->key() != Qt::Key_Home && keyEvent->key() != Qt::Key_End && keyEvent->key() != Qt::Key_Shift && keyEvent->key() != Qt::Key_Control && keyEvent->key() != Qt::Key_Meta && keyEvent->key() != Qt::Key_Alt && keyEvent->key() != Qt::Key_AltGr && keyEvent->key() != Qt::Key_CapsLock && keyEvent->key() != Qt::Key_NumLock && keyEvent->key() != Qt::Key_ScrollLock && !(keyEvent->modifiers() & Qt::ControlModifier))
            {
                //Move cursor to correct position to prevent inserting at wrong location if e.g. text is selected
                this->update_cursor();

                //Add character
                mstrDatOut.insert(mintCurPos, keyEvent->text());
                mintCurPos += keyEvent->text().length();
            }
        }
        else
        {
            //Character mode
            if (mbSerialOpen == true)
            {
                if (!(keyEvent->modifiers() & Qt::ControlModifier))
                {
                    //Control key not held down
                    this->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
                    if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right)
                    {
                        //Send VT100 codes for arrow keys
                        switch (keyEvent->key())
                        {
                            case Qt::Key_Up:
                            {
                                emit vt100_send(QByteArray("\x1b[A"));
                                break;
                            }
                            case Qt::Key_Down:
                            {
                                emit vt100_send(QByteArray("\x1b[B"));
                                break;
                            }
                            case Qt::Key_Left:
                            {
                                emit vt100_send(QByteArray("\x1b[D"));
                                break;
                            }
                            case Qt::Key_Right:
                            {
                                emit vt100_send(QByteArray("\x1b[C"));
                                break;
                            }
                        };

                        this->update_display();
                    }
                    else if (keyEvent->key() != Qt::Key_Escape && keyEvent->key() != Qt::Key_Tab && keyEvent->key() != Qt::Key_Backtab && /*keyEvent->key() != Qt::Key_Backspace &&*/ keyEvent->key() != Qt::Key_Insert && keyEvent->key() != Qt::Key_Delete && keyEvent->key() != Qt::Key_Pause && keyEvent->key() != Qt::Key_Print && keyEvent->key() != Qt::Key_SysReq && keyEvent->key() != Qt::Key_Clear && keyEvent->key() != Qt::Key_Home && keyEvent->key() != Qt::Key_End && keyEvent->key() != Qt::Key_Shift && keyEvent->key() != Qt::Key_Control && keyEvent->key() != Qt::Key_Meta && keyEvent->key() != Qt::Key_Alt && keyEvent->key() != Qt::Key_AltGr && keyEvent->key() != Qt::Key_CapsLock && keyEvent->key() != Qt::Key_NumLock && keyEvent->key() != Qt::Key_ScrollLock)
                    {
                        //Not a special character
                        emit key_pressed(keyEvent->key(), *keyEvent->text().unicode());
                        this->update_display();
                    }
                    return true;
                }
            }
            else
            {
                return true;
            }
        }

        if (mbLocalEcho == false)
        {
            //Return true now if local echo is off
            return true;
        }
    }

    return QObject::eventFilter(target, event);
}

void AutScrollEdit::set_line_mode(bool bNewLineMode)
{
    //Enables or disables line mode
    mbLineMode = bNewLineMode;
    this->update_display();
}

void AutScrollEdit::add_display_data(display_buffer_list *buffers)
{
    //Adds data to the display buffer
    uint32_t i = 0;
    uint32_t l = buffers->length();

    if (had_dat_in_data == false)
    {
        //Remove first newline
        uint32_t a = 0;
        uint32_t b = buffers->at(i).data.length();
        while (a < b && (buffers->at(i).data.at(a) == '\r' || buffers->at(i).data.at(a) == '\n'))
        {
            ++a;
        }

        if (a > 0)
        {
            //TODO: a better way to deal with this "hack"
            if (buffers->at(i).apply_formatting == false && vt100_control_mode == VT100_MODE_DECODE)
            {
                mstrDatIn += "\x1b[9999m";
            }

            mstrDatIn += buffers->at(i).data.mid(a).replace("\r\n", "\n").replace("\r", "\n");

            if (buffers->at(i).apply_formatting == false && vt100_control_mode == VT100_MODE_DECODE)
            {
                mstrDatIn += "\x1b[9998m";
            }

            ++i;
        }
    }

    while (i < l)
    {
        QByteArray temp_buffer = buffers->at(i).data;

        if (buffers->at(i).apply_formatting == false && vt100_control_mode == VT100_MODE_DECODE)
        {
            mstrDatIn += "\x1b[9999m";
        }

        mstrDatIn += temp_buffer.replace("\r\n", "\n").replace("\r", "\n");

        if (buffers->at(i).apply_formatting == false && vt100_control_mode == VT100_MODE_DECODE)
        {
            mstrDatIn += "\x1b[9998m";
        }

        ++i;
    }

    had_dat_in_data = true;
    this->update_display();
}

void AutScrollEdit::add_dat_in_text(QByteArray data)
{
    //Adds data to the DatOut buffer
    mstrDatIn += data.replace("\r\n", "\n").replace("\r", "\n");
    had_dat_in_data = true;
    this->update_display();
}

void AutScrollEdit::add_dat_out_text(const QString strDat)
{
    //Adds data to the DatOut buffer
    if (mbLineMode == true)
    {
        //Line mode
        mstrDatOut += strDat;
        mintCurPos += strDat.length();
        dat_out_updated = true;
        this->update_display();
    }
    else
    {
        //Character mode
        QChar qcTmpQC;
        foreach (qcTmpQC, strDat)
        {
            emit key_pressed(0, qcTmpQC);
        }
    }
}

void AutScrollEdit::clear_dat_in()
{
    //Clears the DatIn buffer
    mstrDatIn.clear();
    mintPrevTextSize = 0;
    dat_in_new_len = 0;
    last_format = pre;

    this->clear();
    dat_out_updated = true;
    had_dat_in_data = false;

    this->moveCursor(QTextCursor::End);
    this->update_display();
}

void AutScrollEdit::clear_dat_out()
{
    //Clears the DatOut buffer
    mstrDatOut.clear();
    mintCurPos = 0;
    dat_out_updated = true;
    this->update_display();
}

QString *AutScrollEdit::get_dat_out()
{
    //Returns the DatOut buffer
    return &mstrDatOut;
}

void AutScrollEdit::insertFromMimeData(const QMimeData *mdSrc)
{
    if (mdSrc->hasUrls() == true)
    {
        //A file has been dropped
        QList<QUrl> urls = mdSrc->urls();
        if (urls.isEmpty())
        {
            //No files
            return;
        }
        else if (urls.length() > 1)
        {
            //More than 1 file - ignore
            return;
        }

        if (urls.first().toLocalFile().isEmpty())
        {
            //Invalid filename
            return;
        }

        //Send back to main application
        emit file_dropped(urls.first().toLocalFile());
    }
    else if (mdSrc->hasText() == true)
    {
        //Text has been pasted
        if (mbLineMode == true)
        {
            //Line mode
            mstrDatOut.insert(mintCurPos, mdSrc->text());
            mintCurPos += mdSrc->text().length();
            dat_out_updated = true;
            this->update_display();
            this->update_cursor();
        }
        else
        {
            //Character mode
            QString strTmpStr = mdSrc->text();
            QChar qcTmpQC;
            foreach (qcTmpQC, strTmpStr)
            {
                emit key_pressed(0, qcTmpQC);
            }
        }
    }
}

void AutScrollEdit::update_display()
{
    //Updates the receive text buffer, faster
    if (this->verticalScrollBar()->isSliderDown() != true && mbContextMenuOpen == false)
    {
        //Variables for text selection storage
        unsigned int uiAnchor = 0;
        unsigned int uiPosition = 0;
        QTextCursor tcTmpCur;
        bool bShiftStart = false;
        bool bShiftEnd = false;
        unsigned int uiCurrentSize = 0;
        uint32_t removed_size = 0;
        int32_t cannot_parse_bytes = 0;

        if (this->textCursor().anchor() != this->textCursor().position())
        {
            //Text is selected
            uiAnchor = this->textCursor().anchor();
            uiPosition = this->textCursor().position();
            tcTmpCur = this->textCursor();
            if (uiAnchor >= mintPrevTextSize)
            {
                //Start of selected text is in the output buffer
                bShiftStart = true;
                uiCurrentSize = mintPrevTextSize;
            }
            if (uiPosition >= mintPrevTextSize)
            {
                //End of selected text is in the output buffer
                bShiftEnd = true;
                uiCurrentSize = mintPrevTextSize;
            }
        }

        //Slider not held down, update
        unsigned int Pos;

        if (this->verticalScrollBar()->sliderPosition() == this->verticalScrollBar()->maximum())
        {
            //Scroll to bottom
            Pos = 65535;
        }
        else
        {
            //Stay here
            Pos = this->verticalScrollBar()->sliderPosition();
        }

        this->setUpdatesEnabled(false);

        if (vt100_control_mode == VT100_MODE_STRIP)
        {
            AutEscape::strip_vt100_formatting(&mstrDatIn, 0);
        }
        else if (vt100_control_mode == VT100_MODE_DECODE)
        {
            AutEscape::strip_vt100_non_formatting(&mstrDatIn, 0);
        }

//TODO: deal with partial VT100 escape codes

        //Replace unprintable characters with escape codes
        int32_t i = mstrDatIn.length() - 1;
        while (i >= 0)
        {
            uint8_t current = (uint8_t)mstrDatIn.at(i);

            if (current < 0x08 || (current >= 0x0b && current <= 0x0c) || (current >= 0x0e && current <= 0x0f))
            {
                mstrDatIn.replace(i, 1, QString("\\0").append(QString::number(current, 16)).toUtf8());
            }
            else if ((current >= 0x10 && current <= 0x1a) || (current >= 0x1c && current <= 0x1f) || (current == 0x1b && vt100_control_mode != VT100_MODE_DECODE))
            {
                mstrDatIn.replace(i, 1, QString("\\").append(QString::number(current, 16)).toUtf8());
            }

            --i;
        }

        if (mstrDatIn.length() > 0)
        {
            QString append_data = mstrDatIn;
            QList<vt100_format_code> format;
            int32_t end = 0;

            //Check if we have unicode replacement characters, if so, wait for next chunk
            i = append_data.length() - 1;
            while (i >= 0 && append_data[i] == unicode_replacement_char && end < 3)
            {
                ++end;
                --i;
            }

            if (end > 0)
            {
                cannot_parse_bytes += end;
                append_data.remove((append_data.length() - end), end);
            }

            if (vt100_control_mode == VT100_MODE_DECODE && vt100_process(&append_data, &format, &end) == false)
            {
                cannot_parse_bytes += append_data.length() - end;
                append_data.remove(end, (append_data.length() - end));
            }

            if (append_data.length() > 0)
            {
                dat_in_new_len = append_data.length();
                int32_t i = 0;

                tcTmpCur = this->textCursor();
                int32_t next_entry = -1;

                if (format.length() > 0 && format[0].start == 0)
                {
                    next_entry = 0;
                }

                bool first = true;

                int32_t aa = 0;

                while (i < dat_in_new_len)
                {
                    int32_t next = dat_in_new_len;

                    while (aa < format.length())
                    {
                        if (i < format[aa].start)
                        {
                            next = format[aa].start;
                            break;
                        }

                        ++aa;
                    }

                    tcTmpCur.setPosition(mintPrevTextSize + i);

                    if (first == true)
                    {
                        if (vt100_control_mode == VT100_MODE_DECODE)
                        {
                            tcTmpCur.setCharFormat(last_format);
                        }
                        else
                        {
                            tcTmpCur.setCharFormat(pre);
                        }
                        first = false;
                    }

                    if (next_entry != -1 && next_entry < format.length())
                    {
                        vt100_format_apply(&tcTmpCur, &format[next_entry]);
                    }

                    this->setTextCursor(tcTmpCur);
                    tcTmpCur.insertText(append_data.mid(i, (next - i)));
                    i = next;
                    next_entry = aa;
                }

                //Reset style if final character was for style
                if (next_entry < format.length())
                {
                    tcTmpCur.setPosition(format[next_entry].start);
                    vt100_format_apply(&tcTmpCur, &format[next_entry]);
                    this->setTextCursor(tcTmpCur);
                }
                else
                {
                    last_format = tcTmpCur.charFormat();
                }

                dat_in_new_len += mintPrevTextSize;
            }
        }

        if (trim_size > 0 && (uint32_t)dat_in_new_len >= trim_threshold)
        {
            //Trim buffer down to requested size
//TODO: this can be improved by doing it before the append above, i.e. if old length + new lengh > threshold, remove from one or both buffers
            removed_size = (uint32_t)dat_in_new_len - trim_size;

            tcTmpCur = this->textCursor();
            tcTmpCur.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1);
            tcTmpCur.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, removed_size);
            tcTmpCur.removeSelectedText();

            dat_in_new_len -= removed_size;
        }

        if (/*mbLocalEcho == true &&*/ mbLineMode == true && dat_out_updated == true)
        {
            /*QTextCursor*/ tcTmpCur = this->textCursor();
            tcTmpCur.setPosition(dat_in_new_len);
            tcTmpCur.movePosition(QTextCursor::End, QTextCursor::KeepAnchor, 1);

            tcTmpCur.setCharFormat(pre);
            this->setTextCursor(tcTmpCur);
            tcTmpCur.insertText(mstrDatOut);

            dat_out_updated = false;
        }

        this->setUpdatesEnabled(true);

        //Update previous text size variables
        mintPrevTextSize = dat_in_new_len;
        mstrDatIn.remove(0, (mstrDatIn.length() - cannot_parse_bytes));

        //Update the cursor position
        this->update_cursor();

        if (uiAnchor != 0 || uiPosition != 0)
        {
            //Reselect previous text
            if (bShiftStart == true)
            {
                //Adjust start position
                uiAnchor -= uiCurrentSize;
                uiAnchor += mintPrevTextSize;
            }
            else if (removed_size > 0)
            {
                //Data was trimmed from the buffer
                if (uiAnchor < removed_size)
                {
                    //The text that was selected has been trimmed
                    uiAnchor = 0;
                }
                else
                {
                    //The text that was selected has not been trimmed, offset to account for new position
                    uiAnchor -= removed_size;
                }
            }

            if (bShiftEnd == true)
            {
                //Adjust end position
                uiPosition -= uiCurrentSize;
                uiPosition += mintPrevTextSize;
            }
            else if (removed_size > 0)
            {
                //Data was trimmed from the buffer
                if (uiPosition < removed_size)
                {
                    //The text that was selected has been trimmed
                    uiPosition = 0;
                }
                else
                {
                    //The text that was selected has not been trimmed, offset to account for new position
                    uiPosition -= removed_size;
                }
            }

            tcTmpCur.setPosition(uiAnchor);
            tcTmpCur.setPosition(uiPosition, QTextCursor::KeepAnchor);
            this->setTextCursor(tcTmpCur);
        }

        //Go back to previous position
        if (Pos == 65535)
        {
            //Bottom
            this->verticalScrollBar()->setValue(this->verticalScrollBar()->maximum());
            if (uiAnchor == 0 && uiPosition == 0)
            {
                this->moveCursor(QTextCursor::End);
            }
        }
        else
        {
            //Maintain
            this->verticalScrollBar()->setValue(Pos);
        }
    }
}

void AutScrollEdit::update_cursor()
{
    //Updates the text control's cursor position
    if (mbLocalEcho == true && mbLineMode == true)
    {
        //Local echo mode and line mode are enabled so move the cursor
        QTextCursor tcTmpCur = this->textCursor();
        tcTmpCur.setPosition(mintPrevTextSize + mintCurPos);
tcTmpCur.setCharFormat(pre);
        this->setTextCursor(tcTmpCur);
    }
}

void AutScrollEdit::set_serial_open(bool SerialOpen)
{
    //Updates the serial open variable
    mbSerialOpen = SerialOpen;
}

void AutScrollEdit::set_trim_settings(uint32_t threshold, uint32_t size)
{
    trim_threshold = threshold;
    trim_size = size;

    if (trim_size > 0 && mintPrevTextSize >= trim_threshold)
    {
        //Buffer needs to be trimmed
        this->update_display();
    }
}

void AutScrollEdit::set_vt100_mode(vt100_mode mode)
{
    vt100_control_mode = mode;
}

void AutScrollEdit::vt100_format_apply(QTextCursor *cursor, vt100_format_code *format)
{
    bool changed = false;
    QTextCharFormat new_format;

    if (format->clear_formatting == true)
    {
        new_format = pre;
        changed = true;
    }
    else if (format->temp == FORMAT_ENABLE)
    {
        pre_dat_in_format_backup = cursor->charFormat();
        new_format = pre;
        changed = true;
    }
    else if (format->temp == FORMAT_DISABLE)
    {
        new_format = pre_dat_in_format_backup;
        changed = true;
    }
    else
    {
        new_format = cursor->charFormat();
    }

    if (format->foreground_color_set == true)
    {
        new_format.setForeground(QBrush(format->foreground_color));
        changed = true;
    }

    if (format->background_color_set == true)
    {
        new_format.setBackground(QBrush(format->background_color));
        changed = true;
    }

    if (format->weight != FORMAT_DUAL_UNSET)
    {
        new_format.setFontWeight((format->weight == FORMAT_DUAL_DOUBLE ? QFont::Bold : (format->weight == FORMAT_DUAL_HALF ? QFont::ExtraLight : QFont::Normal)));
        changed = true;
    }

    if (format->italic != FORMAT_UNSET)
    {
        new_format.setFontItalic((format->italic == FORMAT_ENABLE ? true : false));
        changed = true;
    }

    if (format->underline != FORMAT_UNSET)
    {
        new_format.setFontUnderline((format->underline == FORMAT_ENABLE ? true : false));
        changed = true;
    }

    if (format->strikethrough != FORMAT_UNSET)
    {
        new_format.setFontStrikeOut((format->strikethrough == FORMAT_ENABLE ? true : false));
        changed = true;
    }

    if (changed == true)
    {
        cursor->setCharFormat(new_format);
    }
}

void AutScrollEdit::vt100_format_combine(vt100_format_code *original, vt100_format_code *merge)
{
    if (merge->background_color_set == true)
    {
        original->background_color = merge->background_color;
        original->background_color_set = true;
    }

    if (merge->foreground_color_set == true)
    {
        original->foreground_color = merge->foreground_color;
        original->foreground_color_set = true;
    }

    if (merge->weight != FORMAT_DUAL_UNSET)
    {
        original->weight = merge->weight;
    }

    if (merge->italic != FORMAT_UNSET)
    {
        original->italic = merge->italic;
    }

    if (merge->underline != FORMAT_UNSET)
    {
        original->underline = merge->underline;
    }

    if (merge->strikethrough != FORMAT_UNSET)
    {
        original->strikethrough = merge->strikethrough;
    }

    if (merge->clear_formatting == true)
    {
        original->clear_formatting = merge->clear_formatting;
    }

    if (merge->temp != FORMAT_UNSET)
    {
        original->temp = merge->temp;
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
