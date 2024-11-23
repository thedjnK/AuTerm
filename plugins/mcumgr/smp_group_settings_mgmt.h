/******************************************************************************
** Copyright (C) 2023-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_settings_mgmt.h
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
#ifndef SMP_GROUP_SETTINGS_MGMT_H
#define SMP_GROUP_SETTINGS_MGMT_H

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
// Class definitions
/******************************************************************************/
class smp_group_settings_mgmt : public smp_group
{
    Q_OBJECT

public:
    smp_group_settings_mgmt(smp_processor *parent);
    void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data) override;
    void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error) override;
    void cancel() override;
    bool start_read(QString name, uint32_t max_length, QByteArray *value);
    bool start_write(QString name, QByteArray value);
    bool start_delete(QString name);
    bool start_commit(void);
    bool start_load(void);
    bool start_save(void);

protected:
    void cleanup() override;
    QString mode_to_string(uint8_t mode) override;
    QString command_to_string(uint8_t command) override;

private:
    static bool error_lookup(int32_t rc, QString *error);
    static bool error_define_lookup(int32_t rc, QString *error);
    bool parse_read_response(QCborStreamReader &reader, QByteArray *value);

    //
    QByteArray *return_value;
};

#endif // SMP_GROUP_SETTINGS_MGMT_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
