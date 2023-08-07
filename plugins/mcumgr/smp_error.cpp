/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_error.cpp
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
#include "smp_error.h"

static QList<smp_error_lookup_list_t> lookup_functions;

const QString *smp_error::error_lookup_string(smp_error_t *error)
{
    QString *error_string = nullptr;
    uint16_t i = 0;

    if (error->type == SMP_ERROR_RC)
    {
        //TODO
    }
    else if (error->type == SMP_ERROR_RET)
    {
        while (i < lookup_functions.length())
        {
            if (lookup_functions[i].group == error->group)
            {
                lookup_functions[i].lookup(error->rc, error_string);
                break;
            }

            ++i;
        }
    }

    return error_string;
}

void smp_error::register_error_lookup_function(uint16_t group, smp_error_lookup lookup_function)
{
    smp_error_lookup_list_t error_list_entry;
    error_list_entry.group = group;
    error_list_entry.lookup = lookup_function;

    lookup_functions.append(error_list_entry);
}
