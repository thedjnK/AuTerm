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
    MODE_DATE_TIME,
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

smp_group_os_mgmt::smp_group_os_mgmt(smp_processor *parent) : smp_group(parent, SMP_GROUP_ID_OS, error_lookup)
{
    mode = MODE_IDLE;
}

bool smp_group_os_mgmt::parse_echo_response(QCborStreamReader &reader, QString *response)
{
    QString key = "";

    while (!reader.lastError() && reader.hasNext())
    {
        bool keyset = false;
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

        if (keyset == false && !key.isEmpty())
        {
            key = "";
        }
    }

    return true;
}

void smp_group_os_mgmt::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        qDebug() << "Unexpected response, not busy";
        emit status(smp_user_data, STATUS_ERROR, nullptr);
    }
    else if (group != 0)
    {
        qDebug() << "Unexpected group, not 1";
        emit status(smp_user_data, STATUS_ERROR, nullptr);
    }
    else if (mode == MODE_ECHO && command == COMMAND_ECHO)
    {
        if (version != smp_version)
        {
            //The target device does not support the SMP version being used, adjust for duration of transfer and raise a warning to the parent
            smp_version = version;
            //TODO: raise warning
        }

        //Response to set image state
        QString response;
        QCborStreamReader cbor_reader(data);
        bool good = parse_echo_response(cbor_reader, &response);
        emit status(smp_user_data, STATUS_COMPLETE, response);
    }
    else
    {
        qDebug() << "Unsupported command received";
    }
}

void smp_group_os_mgmt::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    bool cleanup = true;
    qDebug() << "error :(";

    if (command == COMMAND_ECHO && mode == MODE_ECHO)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, nullptr);
    }
    else
    {
        //Unexpected response operation for mode
        emit status(smp_user_data, STATUS_ERROR, QString("Unexpected error (Mode: %1, op: %2)").arg(mode_to_string(mode), op_to_string(op)));
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
    tmp_message->start_message(SMP_OP_WRITE, smp_version, 0x00, 0x01, 0x00);
    tmp_message->writer()->append("d");
    tmp_message->writer()->append(data);
    tmp_message->end_message();

    mode = MODE_ECHO;

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
    case MODE_DATE_TIME:
        return "Date/time";
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

QString smp_group_os_mgmt::op_to_string(uint8_t op)
{
    switch (op)
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
    return false;
}
