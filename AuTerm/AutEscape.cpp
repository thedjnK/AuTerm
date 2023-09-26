/******************************************************************************
** Copyright (C) 2016-2017 Laird
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module: AutEscape.h
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
#include "AutEscape.h"
#include <QRegularExpression>

static QRegularExpression vt100_colour_regex("\\x1b(\\[[0-9]{0,3}(;[0-9]{1,3})?[A-Za-z])");
static QRegularExpression vt100_non_colour_regex[8] = {
    QRegularExpression("\\x1b\\[(20|\\?[1,3-9])h"),
    QRegularExpression("\\x1b\\[(20|\\?[1-9])l"),
    QRegularExpression("\\x1b(=|>|N|O|H|#[3-6|8]|5n|0n|3n|6n|D|M|E|7|8|c)"),
    QRegularExpression("\\x1b\\[(\\(|\\))(A|B|[0-2])"),
    QRegularExpression("\\x1b\\[([0-9]+);([0-9]+)(r|H|f)"),
    QRegularExpression("\\x1b\\[([0-9]+)(A|B|C|D)"),
    QRegularExpression("\\x1b\\[(g|0g|3g|K|[0-2]K|J|[0-2]J|H|;H|f|;f|c|0c)"),
    QRegularExpression("\\x1b([0-9]+);([0-9]+)R")
};

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/

//=============================================================================
//=============================================================================
void AutEscape::do_setup(void)
{
    int8_t i = (sizeof(vt100_non_colour_regex) / sizeof(vt100_non_colour_regex[0])) - 1;
    vt100_colour_regex.setPatternOptions(QRegularExpression::MultilineOption);

    while (i >= 0)
    {
        vt100_non_colour_regex[i].setPatternOptions(QRegularExpression::MultilineOption);
        --i;
    }
}

//=============================================================================
//=============================================================================
void AutEscape::escape_characters(QByteArray *data)
{
    //Escapes character sequences
    qint32 next = data->indexOf("\\");

    while (next != -1)
    {
        QChar cur_char;

        if (data->length() <= (next + 1))
        {
            //No more string length, ignore
            break;
        }

        cur_char = QChar(data->at(next + 1)).toLower();

        if (cur_char == '\\')
        {
            //This is a \\ so remove one of the slashes and ignore the conversion
            data->remove(next, 1);
        }
        else if (cur_char == 'r')
        {
            //This is a \r or \R
            data->replace(next, 2, "\r");
        }
        else if (cur_char == 'n')
        {
            //This is a \n or \N
            data->replace(next, 2, "\n");
        }
        else if (cur_char == 't')
        {
            //This is a \t or \T
            data->replace(next, 2, "\t");
        }
        else if (data->length() <= (next + 2))
        {
            //No more string length, ignore
            break;
        }
        else
        {
            QChar next_char = QChar(data->at(next + 2)).toLower();

            if (((cur_char >= '0' && cur_char <= '9') || (cur_char >= 'a' && cur_char <= 'f')) && ((next_char >= '0' && next_char <= '9') || (next_char >= 'a' && next_char <= 'f')))
            {
                //Character to escape
                char replacement = (char)data->mid((next + 1), 2).toInt(NULL, 16);
                data->replace(next, 3, &replacement, sizeof(char));
            }
        }

        //Search for the next instance
        next = data->indexOf("\\", (next + 1));
    }
}

//=============================================================================
//=============================================================================
void AutEscape::strip_vt100_formatting(QByteArray *data, int32_t offset)
{
    QRegularExpressionMatch regex_match = vt100_colour_regex.match(*data, offset);

    while (regex_match.hasMatch())
    {
        data->remove(regex_match.capturedStart(0), regex_match.capturedLength(0));
        regex_match = vt100_colour_regex.match(*data);
    }

    strip_vt100_non_formatting(data, offset);
}

void AutEscape::strip_vt100_non_formatting(QByteArray *data, int32_t offset)
{
    uint8_t i = 0;
    uint8_t l = (sizeof(vt100_non_colour_regex) / sizeof(vt100_non_colour_regex[0]));

    while (i < l)
    {
        QRegularExpressionMatch regex_match = vt100_non_colour_regex[i].match(*data, offset);

        while (regex_match.hasMatch())
        {
            data->remove(regex_match.capturedStart(0), regex_match.capturedLength(0));
            regex_match = vt100_non_colour_regex[i].match(*data);
        }

        ++i;
    }
}

//=============================================================================
//=============================================================================
void AutEscape::replace_unprintable(QByteArray *data, bool include_1b)
{
    int32_t i = data->length() - 1;

    while (i >= 0)
    {
        uint8_t current = (uint8_t)data->at(i);

        if (current < 0x08 || (current >= 0x0b && current <= 0x0c) || (current >= 0x0e && current <= 0x0f))
        {
            data->replace(i, 1, QString("\\0").append(QString::number(current, 16)).toUtf8());
        }
        else if ((current >= 0x10 && current <= 0x1a) || (current >= 0x1c && current <= 0x1f && (include_1b == true || current != 0x1b)))
        {
            data->replace(i, 1, QString("\\").append(QString::number(current, 16)).toUtf8());
        }

        --i;
    }
}

//=============================================================================
//=============================================================================
void AutEscape::to_hex(QByteArray *data)
{
    int32_t i = data->length() - 1;

    while (i >= 0)
    {
        uint8_t current = (uint8_t)data->at(i);

        if (current <= 0x0f)
        {
            data->replace(i, 1, QString("0").append(QString::number(current, 16)).toUtf8());
        }
        else
        {
            data->replace(i, 1, QString::number(current, 16).toUtf8());
        }

        --i;
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
