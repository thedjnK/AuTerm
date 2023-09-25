/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_os_mgmt.cpp
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
#include "smp_group_os_mgmt.h"

enum modes : uint8_t {
    MODE_IDLE = 0,
    MODE_ECHO,
    MODE_TASK_STATS,
    MODE_MEMORY_POOL,
    MODE_DATE_TIME_GET,
    MODE_DATE_TIME_SET,
    MODE_RESET,
    MODE_MCUMGR_PARAMETERS,
    MODE_OS_APPLICATION_INFO
};

enum os_mgmt_commands : uint8_t {
    COMMAND_ECHO = 0,
    COMMAND_TASK_STATS = 2,
    COMMAND_MEMORY_POOL,
    COMMAND_DATE_TIME,
    COMMAND_RESET,
    COMMAND_MCUMGR_PARAMETERS,
    COMMAND_OS_APPLICATION_INFO,
};

static QStringList smp_error_defines = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "INVALID_FORMAT";

static QStringList smp_error_values = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "The provided format value is not valid";

smp_group_os_mgmt::smp_group_os_mgmt(smp_processor *parent) : smp_group(parent, "OS", SMP_GROUP_ID_OS, error_lookup, error_define_lookup)
{
    mode = MODE_IDLE;
}

bool smp_group_os_mgmt::parse_echo_response(QCborStreamReader &reader, QString *response)
{
    QString key = "";
    bool keyset = true;

    while (!reader.lastError() && reader.hasNext())
    {
        if (keyset == false && !key.isEmpty())
        {
            key.clear();
        }

        keyset = false;

        //	    qDebug() << "Key: " << key;
        //	    qDebug() << "Type: " << reader.type();
        switch (reader.type())
        {
            case QCborStreamReader::String:
            {
                QString data;
                auto r = reader.readString();
                while (r.status == QCborStreamReader::Ok)
                {
                    data.append(r.data);
                    r = reader.readString();
                }

                if (r.status == QCborStreamReader::Error)
                {
                    data.clear();
                    qDebug("Error decoding string");
                }
                else
                {
                    if (key.isEmpty())
                    {
                        key = data;
                        keyset = true;
                    }
                    else if (key == "r")
                    {
                        *response = data;
                    }
                }

                break;
            }

            case QCborStreamReader::Array:
            case QCborStreamReader::Map:
            {
                reader.enterContainer();
                while (reader.lastError() == QCborError::NoError && reader.hasNext())
                {
                    parse_echo_response(reader, response);
                }
                if (reader.lastError() == QCborError::NoError)
                {
                    reader.leaveContainer();
                }
                break;
            }

            default:
            {
                reader.next();
            }
        }
    }

    return true;
}

bool smp_group_os_mgmt::parse_task_stats_response(QCborStreamReader &reader, bool *in_tasks, task_list_t *current_task, QList<task_list_t> *task_array)
{
    QString key = "";
    bool keyset = true;

    while (!reader.lastError() && reader.hasNext())
    {
        if (keyset == false && !key.isEmpty())
        {
            key.clear();
        }

        keyset = false;

        //	    qDebug() << "Key: " << key;
        //	    qDebug() << "Type: " << reader.type();
        switch (reader.type())
        {
            case QCborStreamReader::UnsignedInteger:
            {
                uint32_t *var = nullptr;

                if (key == "prio")
                {
                    var = &current_task->priority;
                }
                else if (key == "tid")
                {
                    var = &current_task->id;
                }
                else if (key == "state")
                {
                    var = &current_task->state;
                }
                else if (key == "stkuse")
                {
                    var = &current_task->stack_usage;
                }
                else if (key == "stksiz")
                {
                    var = &current_task->stack_size;
                }
                else if (key == "cswcnt")
                {
                    var = &current_task->context_switches;
                }
                else if (key == "runtime")
                {
                    var = &current_task->runtime;
                }

                if (var != nullptr)
                {
                    *var = (uint32_t)reader.toUnsignedInteger();
                }

                reader.next();
                break;
            }
            case QCborStreamReader::String:
            {
                QString data;
                auto r = reader.readString();

                while (r.status == QCborStreamReader::Ok)
                {
                        data.append(r.data);
                        r = reader.readString();
                }

                if (r.status == QCborStreamReader::Error)
                {
                        data.clear();
                        qDebug("Error decoding string");
                }
                else
                {
                        if (key.isEmpty())
                        {
                            key = data;
                            keyset = true;
                        }

                        if (key == "tasks")
                        {
                            qDebug() << "in tasks";
                            *in_tasks = true;
                        }
                }

                break;
            }

            case QCborStreamReader::Array:
            case QCborStreamReader::Map:
            {
                reader.enterContainer();
                while (reader.lastError() == QCborError::NoError && reader.hasNext())
                {
                        if (*in_tasks == true)
                        {
                            current_task->name = key;
                            current_task->id = 0;
                            current_task->priority = 0;
                            current_task->state = 0;
                            current_task->context_switches = 0;
                            current_task->runtime = 0;
                            current_task->stack_size = 0;
                            current_task->stack_usage = 0;
                        }

                        parse_task_stats_response(reader, in_tasks, current_task, task_array);
                }
                if (reader.lastError() == QCborError::NoError)
                {
                        reader.leaveContainer();

                        if (*in_tasks == true && !current_task->name.isEmpty())
                        {
                            task_array->append(*current_task);
                            current_task->name.clear();
                        }
                }
                break;
            }

            default:
            {
                reader.next();
            }
        }
    }

    return true;
}

bool smp_group_os_mgmt::parse_memory_pool_response(QCborStreamReader &reader, memory_pool_t *current_memory, QList<memory_pool_t> *memory_array)
{
    QString key = "";
    bool keyset = true;

    while (!reader.lastError() && reader.hasNext())
    {
        if (keyset == false && !key.isEmpty())
        {
            key.clear();
        }

        keyset = false;

        //	    qDebug() << "Key: " << key;
        //	    qDebug() << "Type: " << reader.type();
        switch (reader.type())
        {
            case QCborStreamReader::NegativeInteger:
            case QCborStreamReader::UnsignedInteger:
            {
                int64_t task_value = reader.toInteger();

                if (key == "blksiz")
                {
                    current_memory->size = task_value;
                }
                else if (key == "nblks")
                {
                    current_memory->blocks = task_value;
                }
                else if (key == "nfree")
                {
                    current_memory->free = task_value;
                }
                else if (key == "min")
                {
                    current_memory->minimum = task_value;
                }

                reader.next();
                break;
            }
            case QCborStreamReader::String:
            {
                QString data;
                auto r = reader.readString();

                while (r.status == QCborStreamReader::Ok)
                {
                    data.append(r.data);
                    r = reader.readString();
                }

                if (r.status == QCborStreamReader::Error)
                {
                    data.clear();
                    qDebug("Error decoding string");
                }
                else
                {
                    if (key.isEmpty())
                    {
                        key = data;
                        keyset = true;
                    }
                }

                break;
            }

            case QCborStreamReader::Array:
            case QCborStreamReader::Map:
            {
                reader.enterContainer();
                while (reader.lastError() == QCborError::NoError && reader.hasNext())
                {
                    current_memory->name = key;
                    current_memory->blocks = 0;
                    current_memory->free = 0;
                    current_memory->minimum = 0;
                    current_memory->size = 0;

                    parse_memory_pool_response(reader, current_memory, memory_array);
                }

                if (reader.lastError() == QCborError::NoError)
                {
                    reader.leaveContainer();

                    if (!current_memory->name.isEmpty())
                    {
                        memory_array->append(*current_memory);
                        current_memory->name.clear();
                    }
                }
                break;
            }

            default:
            {
                reader.next();
            }
        }
    }

    return true;
}

bool smp_group_os_mgmt::parse_mcumgr_parameters_response(QCborStreamReader &reader, uint32_t *buffer_size, uint32_t *buffer_count)
{
    QString key = "";
    bool keyset = true;

    while (!reader.lastError() && reader.hasNext())
    {
        if (keyset == false && !key.isEmpty())
        {
            key.clear();
        }

        keyset = false;

        //	    qDebug() << "Key: " << key;
        //	    qDebug() << "Type: " << reader.type();
        switch (reader.type())
        {
            case QCborStreamReader::UnsignedInteger:
            {
                uint32_t buffer_value = (uint32_t)reader.toUnsignedInteger();

                if (key == "buf_size")
                {
                    *buffer_size = buffer_value;
                }
                else if (key == "buf_count")
                {
                    *buffer_count = buffer_value;
                }

                reader.next();
                break;
            }
            case QCborStreamReader::String:
            {
                QString data;
                auto r = reader.readString();

                while (r.status == QCborStreamReader::Ok)
                {
                    data.append(r.data);
                    r = reader.readString();
                }

                if (r.status == QCborStreamReader::Error)
                {
                    data.clear();
                    qDebug("Error decoding string");
                }
                else
                {
                    if (key.isEmpty())
                    {
                        key = data;
                        keyset = true;
                    }
                }

                break;
            }

            case QCborStreamReader::Array:
            case QCborStreamReader::Map:
            {
                reader.enterContainer();
                while (reader.lastError() == QCborError::NoError && reader.hasNext())
                {
                    parse_mcumgr_parameters_response(reader, buffer_size, buffer_count);
                }
                if (reader.lastError() == QCborError::NoError)
                {
                    reader.leaveContainer();
                }
                break;
            }

            default:
            {
                reader.next();
            }
        }
    }

    return true;
}

bool smp_group_os_mgmt::parse_os_application_info_response(QCborStreamReader &reader, QString *response)
{
    QString key = "";
    bool keyset = true;

    while (!reader.lastError() && reader.hasNext())
    {
        if (keyset == false && !key.isEmpty())
        {
            key.clear();
        }

        keyset = false;

        //	    qDebug() << "Key: " << key;
        //	    qDebug() << "Type: " << reader.type();
        switch (reader.type())
        {
            case QCborStreamReader::String:
            {
                QString data;
                auto r = reader.readString();

                while (r.status == QCborStreamReader::Ok)
                {
                    data.append(r.data);
                    r = reader.readString();
                }

                if (r.status == QCborStreamReader::Error)
                {
                    data.clear();
                    qDebug("Error decoding string");
                }
                else
                {
                    if (key.isEmpty())
                    {
                        key = data;
                        keyset = true;
                    }
                    else if (key == "output")
                    {
                        *response = data;
                    }
                }

                break;
            }

            case QCborStreamReader::Array:
            case QCborStreamReader::Map:
            {
                reader.enterContainer();
                while (reader.lastError() == QCborError::NoError && reader.hasNext())
                {
                    parse_os_application_info_response(reader, response);
                }
                if (reader.lastError() == QCborError::NoError)
                {
                    reader.leaveContainer();
                }
                break;
            }

            default:
            {
                reader.next();
            }
        }
    }

    return true;
}

void smp_group_os_mgmt::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    Q_UNUSED(op);
//    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        qDebug() << "Unexpected response, not busy";
        emit status(smp_user_data, STATUS_ERROR, "Unexpected response, shell mgmt not busy");
    }
    else if (group != SMP_GROUP_ID_OS)
    {
        qDebug() << "Unexpected group " << group << ", not " << SMP_GROUP_ID_OS;
        emit status(smp_user_data, STATUS_ERROR, "Unexpected group, not os mgmt");
    }
    else
    {
        uint8_t finished_mode = mode;
        mode = MODE_IDLE;

        if (version != smp_version)
        {
            //The target device does not support the SMP version being used, adjust for duration of transfer and raise a warning to the parent
            smp_version = version;
            emit version_error(version);
        }

        if (finished_mode == MODE_ECHO && command == COMMAND_ECHO)
        {
            //Response to set image state
            QString response;
            QCborStreamReader cbor_reader(data);
            bool good = parse_echo_response(cbor_reader, &response);
            emit status(smp_user_data, STATUS_COMPLETE, response);
        }
        else if (finished_mode == MODE_TASK_STATS && command == COMMAND_TASK_STATS)
        {
            //Response to set image state
            QCborStreamReader cbor_reader(data);
            bool in_tasks = false;
            task_list_t current_task;
            bool good = parse_task_stats_response(cbor_reader, &in_tasks, &current_task, task_list);

            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_MEMORY_POOL && command == COMMAND_MEMORY_POOL)
        {
            //Response to set image state
            QCborStreamReader cbor_reader(data);
            memory_pool_t current_memory;
            bool good = parse_memory_pool_response(cbor_reader, &current_memory, memory_list);

            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_RESET && command == COMMAND_RESET)
        {
            //No need to check response, it would have returned success to come through this callback
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_MCUMGR_PARAMETERS && command == COMMAND_MCUMGR_PARAMETERS)
        {
            //Response to MCUmgr buffer parameters
            QCborStreamReader cbor_reader(data);
            uint32_t buffer_size;
            uint32_t buffer_count;
            bool good = parse_mcumgr_parameters_response(cbor_reader, &buffer_size, &buffer_count);

            qDebug() << "buffer size: " << buffer_size << ", buffer count: " << buffer_count;

            emit status(smp_user_data, STATUS_COMPLETE, QString("Buffer size: %1\nBuffer count: %2").arg(QString::number(buffer_size), QString::number(buffer_count)));
        }
        else if (finished_mode == MODE_OS_APPLICATION_INFO && command == COMMAND_OS_APPLICATION_INFO)
        {
            //Response to OS/application info
            QCborStreamReader cbor_reader(data);
            QString response;
            bool good = parse_os_application_info_response(cbor_reader, &response);

            qDebug() << response;

            emit status(smp_user_data, STATUS_COMPLETE, response);
        }
        else
        {
            qDebug() << "Unsupported command received";
        }
    }
}

void smp_group_os_mgmt::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    Q_UNUSED(version);
    Q_UNUSED(op);
    Q_UNUSED(group);
    Q_UNUSED(error);

    bool cleanup = true;
    qDebug() << "error :(";

    if (command == COMMAND_ECHO && mode == MODE_ECHO)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_TASK_STATS && mode == MODE_TASK_STATS)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_MEMORY_POOL && mode == MODE_MEMORY_POOL)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_RESET && mode == MODE_RESET)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_MCUMGR_PARAMETERS && mode == MODE_MCUMGR_PARAMETERS)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_OS_APPLICATION_INFO && mode == MODE_OS_APPLICATION_INFO)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else
    {
        //Unexpected response operation for mode
        emit status(smp_user_data, STATUS_ERROR, QString("Unexpected error (Mode: %1, op: %2)").arg(mode_to_string(mode), command_to_string(command)));
    }

    if (cleanup == true)
    {
        mode = MODE_IDLE;
    }
}

void smp_group_os_mgmt::timeout(smp_message *message)
{
    qDebug() << "timeout :(";

    //TODO:
    emit status(smp_user_data, STATUS_TIMEOUT, QString("Timeout (Mode: %1)").arg(mode_to_string(mode)));

    mode = MODE_IDLE;
}

void smp_group_os_mgmt::cancel()
{
    if (mode != MODE_IDLE)
    {
        mode = MODE_IDLE;

        emit status(smp_user_data, STATUS_CANCELLED, nullptr);
    }
}

bool smp_group_os_mgmt::start_echo(QString data)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_OS, COMMAND_ECHO);
    tmp_message->writer()->append("d");
    tmp_message->writer()->append(data);
    tmp_message->end_message();

    mode = MODE_ECHO;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_os_mgmt::start_task_stats(QList<task_list_t> *tasks)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_OS, COMMAND_TASK_STATS);
    tmp_message->end_message();

    task_list = tasks;
    tasks->clear();
    mode = MODE_TASK_STATS;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_os_mgmt::start_memory_pool(QList<memory_pool_t> *memory)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_OS, COMMAND_MEMORY_POOL);
    tmp_message->end_message();

    memory_list = memory;
    memory->clear();
    mode = MODE_MEMORY_POOL;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_os_mgmt::start_reset(bool force)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_OS, COMMAND_RESET);
    if (force == true)
    {
        tmp_message->writer()->append("force");
        tmp_message->writer()->append(true);
    }
    tmp_message->end_message();

    mode = MODE_RESET;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_os_mgmt::start_mcumgr_parameters()
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_OS, COMMAND_MCUMGR_PARAMETERS);
    tmp_message->end_message();

    mode = MODE_MCUMGR_PARAMETERS;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_os_mgmt::start_os_application_info(QString format)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_OS, COMMAND_OS_APPLICATION_INFO);
    if (format.isEmpty() == false)
    {
        tmp_message->writer()->append("format");
        tmp_message->writer()->append(format);
    }
    tmp_message->end_message();

    mode = MODE_OS_APPLICATION_INFO;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

QString smp_group_os_mgmt::mode_to_string(uint8_t mode)
{
    switch (mode)
    {
        case MODE_IDLE:
            return "Idle";
        case MODE_ECHO:
            return "Echo";
        case MODE_TASK_STATS:
            return "Task Statistics";
        case MODE_MEMORY_POOL:
            return "Memory pool";
        case MODE_DATE_TIME_GET:
            return "Date/time get";
        case MODE_DATE_TIME_SET:
            return "Date/time set";
        case MODE_RESET:
            return "Reset";
        case MODE_MCUMGR_PARAMETERS:
            return "MCUmgr parameters";
        case MODE_OS_APPLICATION_INFO:
            return "OS/Application info";
        default:
            return "Invalid";
    }
}

QString smp_group_os_mgmt::command_to_string(uint8_t command)
{
    switch (command)
    {
        case COMMAND_ECHO:
            return "Echo";
        case COMMAND_TASK_STATS:
            return "Task Statistics";
        case COMMAND_MEMORY_POOL:
            return "Memory pool";
        case COMMAND_DATE_TIME:
            return "Date/time";
        case COMMAND_RESET:
            return "Reset";
        case COMMAND_MCUMGR_PARAMETERS:
            return "MCUmgr parameters";
        case COMMAND_OS_APPLICATION_INFO:
            return "OS/Application info";
        default:
            return "Invalid";
    }
}

bool smp_group_os_mgmt::error_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_values.length())
    {
        *error = smp_error_values.at(rc);
        return true;
    }

    return false;
}

bool smp_group_os_mgmt::error_define_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_defines.length())
    {
        *error = smp_error_defines.at(rc);
        return true;
    }

    return false;
}
