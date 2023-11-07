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
#include <QStringList>
#include "smp_group.h"

static QList<smp_error_lookup_list_t> lookup_functions;
static QStringList smp_error_defines = QStringList() <<
    //Error index starts from 0
    "EOK" <<
    "EUNKNOWN" <<
    "ENOMEM" <<
    "EINVAL" <<
    "ETIMEOUT" <<
    "ENOENT" <<
    "EBADSTATE" <<
    "EMSGSIZE" <<
    "ENOTSUP" <<
    "ECORRUPT" <<
    "EBUSY" <<
    "EACCESSDENIED" <<
    "UNSUPPORTED_TOO_OLD" <<
    "UNSUPPORTED_TOO_NEW";

static QStringList smp_error_values = QStringList() <<
    //Error index starts from 0
    "No error" <<
    "Unknown error" <<
    "Insufficient memory" <<
    "Error in input value" <<
    "Operation timed out" <<
    "No such file/entry" <<
    "Current state disallows command" <<
    "Response too large" <<
    "Command not supported" <<
    "Corrupt" <<
    "Command blocked by processing of other command" <<
    "Access to specific function, command or resource denied" <<
    "Requested SMP MCUmgr protocol version is not supported (too old)" <<
    "Requested SMP MCUmgr protocol version is not supported (too new)";

QString smp_error::error_lookup_string(smp_error_t *error)
{
    QString error_string;
    uint16_t i = 0;

    if (error->rc < 0)
    {
        //MCUmgr error codes are 0 or greater, this is not a valid error code
        return "Provided error code is not a valid MCUmgr error code value";
    }

    if (error->type == SMP_ERROR_RC)
    {
        if (error->rc < smp_error_values.length())
        {
            error_string = smp_error_values.at(error->rc);
        }
    }
    else if (error->type == SMP_ERROR_RET)
    {
        if (error->rc < 2)
        {
            //Index 0 is reserved for no error, index 1 is reserved for unknown error
            error_string = smp_error_values.at(error->rc);
        }
        else
        {
            while (i < lookup_functions.length())
            {
                if (lookup_functions[i].group == error->group)
                {
                    //TODO: error handling?
                    (void)lookup_functions[i].lookup->lookup_error(error->rc, &error_string);
                    break;
                }

                ++i;
            }
        }
    }

    return error_string;
}

QString smp_error::error_lookup_define(smp_error_t *error)
{
    QString error_define;
    uint16_t i = 0;

    if (error->type == SMP_ERROR_RC)
    {
        if (error->rc < smp_error_defines.length())
        {
            error_define = smp_error_defines.at(error->rc);
        }
    }
    else if (error->type == SMP_ERROR_RET)
    {
        if (error->rc < 2)
        {
            //Index 0 is reserved for no error, index 1 is reserved for unknown error
            error_define = smp_error_defines.at(error->rc);
        }
        else
        {
            while (i < lookup_functions.length())
            {
                if (lookup_functions[i].group == error->group)
                {
                    //TODO: error handling?
                    (void)lookup_functions[i].lookup->lookup_error_define(error->rc, &error_define);
                    break;
                }

                ++i;
            }
        }
    }

    return error_define;
}

void smp_error::register_error_lookup_function(uint16_t group, smp_group *group_object)
{
    smp_error_lookup_list_t error_list_entry;
    error_list_entry.group = group;
    error_list_entry.lookup = group_object;

    lookup_functions.append(error_list_entry);
}
