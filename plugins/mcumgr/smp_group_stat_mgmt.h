/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_stat_mgmt.h
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
#ifndef SMP_GROUP_STAT_MGMT_H
#define SMP_GROUP_STAT_MGMT_H

#include "smp_group.h"
#include "smp_error.h"
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>

struct stat_value_t {
    QString name;
    uint32_t value;
};

class smp_group_stat_mgmt : public smp_group
{
    Q_OBJECT

public:
    smp_group_stat_mgmt(smp_processor *parent);
    void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data);
    void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error);
    void timeout(smp_message *message);
    void cancel();
    bool start_group_data(QString name, QList<stat_value_t> *stats);
    bool start_list_groups(QStringList *groups);
    static bool error_lookup(int32_t rc, QString *error);
    static bool error_define_lookup(int32_t rc, QString *error);

private:
    bool parse_group_data_response(QCborStreamReader &reader, QString *key_name, QList<stat_value_t> *stats);
    bool parse_list_groups_response(QCborStreamReader &reader, QString *key_name, QStringList *groups);

    QString mode_to_string(uint8_t mode);
    QString command_to_string(uint8_t command);

    //
    uint8_t mode;
    QList<stat_value_t> *stat_object;
    QStringList *group_object;
};

#endif // SMP_GROUP_STAT_MGMT_H
