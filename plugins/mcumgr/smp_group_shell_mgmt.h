/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_shell_mgmt.h
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
#ifndef SMP_GROUP_SHELL_MGMT_H
#define SMP_GROUP_SHELL_MGMT_H

#include "smp_group.h"
#include "smp_error.h"
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>

class smp_group_shell_mgmt : public smp_group
{
    Q_OBJECT

public:
    smp_group_shell_mgmt(smp_processor *parent);
    void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data);
    void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error);
    void timeout(smp_message *message);
    void cancel();
    bool start_execute(QStringList *arguments, int32_t *ret);
    static bool error_lookup(int32_t rc, QString *error);
    static bool error_define_lookup(int32_t rc, QString *error);

private:
    bool parse_execute_response(QCborStreamReader &reader, int32_t *ret, QString *response);

    QString mode_to_string(uint8_t mode);
    QString command_to_string(uint8_t command);

    //
    uint8_t mode;
    int32_t *return_ret;
};

#endif // SMP_GROUP_SHELL_MGMT_H
