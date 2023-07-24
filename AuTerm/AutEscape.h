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
#ifndef AUTESCAPE_H
#define AUTESCAPE_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QByteArray>

/******************************************************************************/
// Class definitions
/******************************************************************************/
class AutEscape
{
public:
    static void escape_characters(QByteArray *baData);
    static void strip_vt100_formatting(QByteArray *data, int32_t offset);
    static void replace_unprintable(QByteArray *data, bool include_1b);
    static void to_hex(QByteArray *data);
};

#endif // AUTESCAPE_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
