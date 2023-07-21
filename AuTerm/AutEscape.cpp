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

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/

//=============================================================================
//=============================================================================
void
AutEscape::escape_characters(
    QByteArray *data
    )
{
    //Escapes character sequences
    qint32 next = data->indexOf("\\");
    while (next != -1)
    {
        QChar cur_char;

        if (data->length() <= next)
        {
            //No more string length, ignore
            break;
        }

        cur_char = data->at(next + 1);

        if (cur_char == '\\')
        {
            //This is a \\ so remove one of the slashes and ignore the conversion
            data->remove(next, 1);
        }
        else if (cur_char == 'r' || cur_char == 'R')
        {
            //This is a \r or \R
            data->insert(next, "\r");
            data->remove((next + 1), 2);
        }
        else if (cur_char == 'n' || cur_char == 'N')
        {
            //This is a \n or \N
            data->insert(next, "\n");
            data->remove((next + 1), 2);
        }
        else if (cur_char == 't' || cur_char == 'T')
        {
            //This is a \t or \T
            data->insert(next, "\t");
            data->remove((next + 1), 2);
        }
        else if (data->length() <= (next + 2))
        {
            //No more string length, ignore
            break;
        }
        else
        {
            QChar next_char = data->at(next + 2);

            if (((cur_char >= '0' && cur_char <= '9') || (cur_char >= 'a' && cur_char <= 'f') || (cur_char >= 'A' && cur_char <= 'F')) && ((next_char >= '0' && next_char <= '9') || (next_char >= 'a' && next_char <= 'f') || (next_char >= 'A' && next_char <= 'F')))
            {
                //Character to escape
                data->insert(next, (char)data->mid((next + 1), 2).toInt(NULL, 16));
                data->remove((next + 1), 3);
            }
        }

        //Search for the next instance
        next = data->indexOf("\\", (next + 1));
    }
}

//=============================================================================
//=============================================================================
void
AutEscape::strip_vt100_formatting(
    QByteArray *data,
    int32_t offset
    )
{
//TODO: improve this
    QRegularExpression vt100_regex("\\x1b(\\[[0-9]{0,3}(;[0-9]{1,3})?[A-Za-z])");
    vt100_regex.setPatternOptions(QRegularExpression::MultilineOption);
    QRegularExpressionMatch regex_match = vt100_regex.match(*data, offset);

    while (regex_match.hasMatch())
    {
        data->remove(regex_match.capturedStart(0), regex_match.capturedLength(0));
        regex_match = vt100_regex.match(*data);
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
