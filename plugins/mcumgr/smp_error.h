/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_error.h
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
#ifndef SMP_ERROR_H
#define SMP_ERROR_H

#include <QObject>

enum smp_error_type {
    SMP_ERROR_NONE,
    SMP_ERROR_RC,
    SMP_ERROR_RET,
};

enum smp_rc_errors {
    //Error index starts from 0
    SMP_RC_ERROR_EOK = 0,
    SMP_RC_ERROR_EUNKNOWN,
    SMP_RC_ERROR_ENOMEM,
    SMP_RC_ERROR_EINVAL,
    SMP_RC_ERROR_ETIMEOUT,
    SMP_RC_ERROR_ENOENT,
    SMP_RC_ERROR_EBADSTATE,
    SMP_RC_ERROR_EMSGSIZE,
    SMP_RC_ERROR_ENOTSUP,
    SMP_RC_ERROR_ECORRUPT,
    SMP_RC_ERROR_EBUSY,
    SMP_RC_ERROR_EACCESSDENIED,
    SMP_RC_ERROR_UNSUPPORTED_TOO_OLD,
    SMP_RC_ERROR_UNSUPPORTED_TOO_NEW,
};

struct smp_error_t {
    smp_error_type type;
    int32_t rc;
    uint16_t group;
};

class smp_group;

typedef bool (*smp_error_lookup)(int32_t rc, QString *error);
typedef bool (*smp_error_define_lookup)(int32_t rc, QString *define);

struct smp_error_lookup_list_t {
    uint16_t group;
    smp_group *lookup;
};

//All SMP group error codes (for SMP version 2) must start at offset 2
const int32_t smp_version_2_error_code_start = 2;

class smp_error
{
public:
    static QString error_lookup_string(smp_error_t *error);
    static QString error_lookup_define(smp_error_t *define);
    static void register_error_lookup_function(uint16_t group, smp_group *group_object);
};

#endif // SMP_ERROR_H
