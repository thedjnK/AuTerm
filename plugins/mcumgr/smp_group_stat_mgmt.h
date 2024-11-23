/******************************************************************************
** Copyright (C) 2023-2024 Jamie M.
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

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "smp_group.h"
#include "smp_error.h"
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
struct stat_value_t {
    QString name;
    uint32_t value;
};

/******************************************************************************/
// Class definitions
/******************************************************************************/
class smp_group_stat_mgmt : public smp_group
{
    Q_OBJECT

public:
    smp_group_stat_mgmt(smp_processor *parent);
    void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data) override;
    void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error) override;
    void cancel() override;
    bool start_group_data(QString name, QList<stat_value_t> *stats);
    bool start_list_groups(QStringList *groups);

protected:
    void cleanup() override;
    QString mode_to_string(uint8_t mode) override;
    QString command_to_string(uint8_t command) override;

private:
    static bool error_lookup(int32_t rc, QString *error);
    static bool error_define_lookup(int32_t rc, QString *error);
    bool parse_group_data_response(QCborStreamReader &reader, QString *key_name, QList<stat_value_t> *stats);
    bool parse_list_groups_response(QCborStreamReader &reader, QString *key_name, QStringList *groups);

    //
    QList<stat_value_t> *stat_object;
    QStringList *group_object;
};

#endif // SMP_GROUP_STAT_MGMT_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
