/******************************************************************************
** Copyright (C) 2016-2017 Laird
** Copyright (C) 2023 Jamie M.
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
#ifndef UWXESCAPE_H
#define UWXESCAPE_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QByteArray>

/******************************************************************************/
// Class definitions
/******************************************************************************/
class UwxEscape
{
public:
    static
    void
    EscapeCharacters(
        QByteArray *baData
        );

    static
    void
    StripVT100Formatting(
        QByteArray *data,
        int32_t offset
        );
};

#endif // UWXESCAPE_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
