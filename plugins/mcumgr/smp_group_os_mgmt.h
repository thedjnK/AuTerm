/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_os_mgmt.h
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
#ifndef SMP_GROUP_OS_MGMT_H
#define SMP_GROUP_OS_MGMT_H

#include "smp_group.h"
#include "smp_error.h"
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>

struct task_list_t {
    QString name;
    uint32_t priority;
    uint32_t id;
    uint32_t state;
    uint32_t stack_size;
    uint32_t stack_usage;
    uint32_t context_switches;
    uint32_t runtime;
};

struct memory_pool_t {
    QString name;
    int32_t size;
    int32_t blocks;
    int32_t free;
    int32_t minimum;
};

class smp_group_os_mgmt : public smp_group
{
    Q_OBJECT

public:
    smp_group_os_mgmt(smp_processor *parent);
    void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data);
    void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error);
    void timeout(smp_message *message);
    void cancel();
    bool start_echo(QString data);
    bool start_task_stats(QList<task_list_t> *tasks);
    bool start_memory_pool(QList<memory_pool_t> *memory);
    bool start_reset(bool force);
    bool start_mcumgr_parameters();
    bool start_os_application_info(QString format);
    bool start_date_time_get(QDateTime *date_time);
    bool start_date_time_set(QDateTime date_time);
    bool start_bootloader_info(QString query, QVariant *response);
    static bool error_lookup(int32_t rc, QString *error);
    static bool error_define_lookup(int32_t rc, QString *error);

private:
    bool parse_echo_response(QCborStreamReader &reader, QString *response);
    bool parse_task_stats_response(QCborStreamReader &reader, bool *in_tasks, task_list_t *current_task, QList<task_list_t> *task_array);
    bool parse_memory_pool_response(QCborStreamReader &reader, memory_pool_t *current_memory, QList<memory_pool_t> *memory_array);
    bool parse_mcumgr_parameters_response(QCborStreamReader &reader, uint32_t *buffer_size, uint32_t *buffer_count);
    bool parse_os_application_info_response(QCborStreamReader &reader, QString *response);
    bool parse_date_time_response(QCborStreamReader &reader, QDateTime *date_time);
    bool parse_bootloader_info_response(QCborStreamReader &reader, QVariant *response);

    QString mode_to_string(uint8_t mode);
    QString command_to_string(uint8_t command);

    //
    uint8_t mode;
    QList<task_list_t> *task_list;
    QList<memory_pool_t> *memory_list;
    QString bootloader_query_value;
    QVariant *bootloader_info_response;
    QDateTime *rtc_get_date_time;
};

#endif // SMP_GROUP_OS_MGMT_H
