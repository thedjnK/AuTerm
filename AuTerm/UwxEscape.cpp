/******************************************************************************
** Copyright (C) 2016-2017 Laird
**
** Project: AuTerm
**
** Module: UwxEscape.h
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
#include "UwxEscape.h"
#include <QRegularExpression>

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/

//=============================================================================
//=============================================================================
void
UwxEscape::EscapeCharacters(
    QByteArray *baData
    )
{
    //Escapes character sequences
    qint32 Next = baData->indexOf("\\");
    while (Next != -1)
    {
        if (baData->length() <= Next+1)
        {
            //No more string length, ignore
            break;
        }
        else if (baData->at(Next+1) == '\\')
        {
            //This is a \\ so remove one of the slashes and ignore the conversion
            baData->remove(Next, 1);
        }
        else if (baData->at(Next+1) == 'r' || baData->at(Next+1) == 'R')
        {
            //This is a \r or \R
            baData->insert(Next, "\r");
            baData->remove(Next+1, 2);
        }
        else if (baData->at(Next+1) == 'n' || baData->at(Next+1) == 'N')
        {
            //This is a \n or \N
            baData->insert(Next, "\n");
            baData->remove(Next+1, 2);
        }
        else if (baData->at(Next+1) == 't' || baData->at(Next+1) == 'T')
        {
            //This is a \t or \T
            baData->insert(Next, "\t");
            baData->remove(Next+1, 2);
        }
        else if (baData->length() <= Next+2)
        {
            //No more string length, ignore
            break;
        }
        else if (((baData->at(Next+1) >= '0' && baData->at(Next+1) <= '9') || (baData->at(Next+1) >= 'a' && baData->at(Next+1) <= 'f') || (baData->at(Next+1) >= 'A' && baData->at(Next+1) <= 'F')) && ((baData->at(Next+2) >= '0' && baData->at(Next+2) <= '9') || (baData->at(Next+2) >= 'a' && baData->at(Next+2) <= 'f') || (baData->at(Next+2) >= 'A' && baData->at(Next+2) <= 'F')))
        {
            //Character to escape
            baData->insert(Next, (char)baData->mid(Next+1, 2).toInt(NULL, 16));
            baData->remove(Next+1, 3);
        }

        //Search for the next instance
        Next = baData->indexOf("\\", Next+1);
    }
}

//=============================================================================
//=============================================================================
void
UwxEscape::StripVT100Formatting(
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
