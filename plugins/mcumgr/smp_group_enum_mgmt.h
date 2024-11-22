/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_enum_mgmt.h
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
#ifndef SMP_GROUP_ENUM_MGMT_H
#define SMP_GROUP_ENUM_MGMT_H

#include "smp_group.h"
#include "smp_error.h"
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>

struct enum_details_t {
    uint16_t id;
    QString name;
    uint16_t handlers;
};

struct enum_fields_present_t {
    bool id;
    bool name;
    bool handlers;
};

class smp_group_enum_mgmt : public smp_group
{
    Q_OBJECT

public:
    smp_group_enum_mgmt(smp_processor *parent);
    void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data) override;
    void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error) override;
    void cancel() override;
    bool start_enum_count(uint16_t *count);
    bool start_enum_list(QList<uint16_t> *groups);
    bool start_enum_single(uint16_t index, uint16_t *id, bool *end);
    bool start_enum_details(QList<enum_details_t> *groups, enum_fields_present_t *fields_present);

protected:
    void cleanup() override;
    QString mode_to_string(uint8_t mode) override;
    QString command_to_string(uint8_t command) override;

private:
    static bool error_lookup(int32_t rc, QString *error);
    static bool error_define_lookup(int32_t rc, QString *error);
    bool parse_count_response(QCborStreamReader &reader, uint16_t *count, bool *count_found);
    bool parse_list_response(QCborStreamReader &reader, const QString *list_key, QList<uint16_t> *groups, bool *groups_found);
    bool parse_single_response(QCborStreamReader &reader, uint16_t *id, bool *end, bool *group_found);
    bool parse_details_response(QCborStreamReader &reader, uint8_t layers, QList<enum_details_t> *groups, bool *groups_found, enum_fields_present_t *fields_present);

    //
    uint16_t *groups_count;
    QList<uint16_t> *groups_list;
    uint16_t *group_single_id;
    bool *group_single_end;
    QList<enum_details_t> *groups_details;
    enum_fields_present_t *groups_details_fields_present;
};

#endif // SMP_GROUP_ENUM_MGMT_H
