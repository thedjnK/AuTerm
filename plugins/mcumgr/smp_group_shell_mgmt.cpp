/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_shell_mgmt.cpp
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
#include "smp_group_shell_mgmt.h"

enum modes : uint8_t {
    MODE_IDLE = 0,
    MODE_EXECUTE,
};

enum shell_mgmt_commands : uint8_t {
    COMMAND_EXECUTE = 0,
};

static QStringList smp_error_defines = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "COMMAND_TOO_LONG" <<
    "EMPTY_COMMAND";

static QStringList smp_error_values = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "The provided command to execute is too long" <<
    "No command to execute was provided";

smp_group_shell_mgmt::smp_group_shell_mgmt(smp_processor *parent) : smp_group(parent, "SHELL", SMP_GROUP_ID_SHELL, error_lookup, error_define_lookup)
{
    mode = MODE_IDLE;
}

bool smp_group_shell_mgmt::parse_execute_response(QCborStreamReader &reader, int32_t *ret, QString *response)
{
    //    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    bool keyset = true;

    *ret = 0;

    while (!reader.lastError() && reader.hasNext())
    {
        if (keyset == false && !key.isEmpty())
        {
            key.clear();
        }

        keyset = false;

        switch (reader.type())
        {
            case QCborStreamReader::UnsignedInteger:
            case QCborStreamReader::NegativeInteger:
            {
                if (key == "ret")
                {
                    *ret = (int32_t)reader.toInteger();
                }

                reader.next();
                break;
            }
            case QCborStreamReader::ByteArray:
            {
                auto r = reader.readByteArray();
                while (r.status == QCborStreamReader::Ok)
                {
                    r = reader.readByteArray();
                }

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
                    else if (key == "o")
                    {
                        response->append(data);
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
                    parse_execute_response(reader, ret, response);
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
                continue;
            }
        };
    }

    return true;
}

void smp_group_shell_mgmt::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    Q_UNUSED(op);
    //    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        qDebug() << "Unexpected response, not busy";
        emit status(smp_user_data, STATUS_ERROR, nullptr);
    }
    else if (group != SMP_GROUP_ID_SHELL)
    {
        qDebug() << "Unexpected group " << group << ", not " << SMP_GROUP_ID_SHELL;
        emit status(smp_user_data, STATUS_ERROR, nullptr);
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

        if (finished_mode == MODE_EXECUTE && command == COMMAND_EXECUTE)
        {
            //Response to execute
            QString response;
            QCborStreamReader cbor_reader(data);
            bool good = parse_execute_response(cbor_reader, return_ret, &response);

            emit status(smp_user_data, STATUS_COMPLETE, response);
        }
        else
        {
            qDebug() << "Unsupported command received";
        }
    }
}

void smp_group_shell_mgmt::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    Q_UNUSED(version);
    Q_UNUSED(op);
    Q_UNUSED(group);
    Q_UNUSED(error);

    bool cleanup = true;
    qDebug() << "error :(";

    if (command == COMMAND_EXECUTE && mode == MODE_EXECUTE)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, nullptr);
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

void smp_group_shell_mgmt::timeout(smp_message *message)
{
    qDebug() << "timeout :(";

    //TODO:
    emit status(smp_user_data, STATUS_TIMEOUT, QString("Timeout (Mode: %1)").arg(mode_to_string(mode)));

    mode = MODE_IDLE;
}

void smp_group_shell_mgmt::cancel()
{
    if (mode != MODE_IDLE)
    {
        mode = MODE_IDLE;

        emit status(smp_user_data, STATUS_CANCELLED, nullptr);
    }
}

bool smp_group_shell_mgmt::start_execute(QStringList *arguments, int32_t *ret)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_SHELL, COMMAND_EXECUTE);
    tmp_message->writer()->append("argv");
    tmp_message->writer()->startArray();

    uint8_t i = 0;
    while (i < arguments->length())
    {
        tmp_message->writer()->append(arguments->at(i));
        ++i;
    }

    tmp_message->writer()->endArray();
    tmp_message->end_message();

    return_ret = ret;
    *return_ret = 0;
    mode = MODE_EXECUTE;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

QString smp_group_shell_mgmt::mode_to_string(uint8_t mode)
{
    switch (mode)
    {
    case MODE_IDLE:
        return "Idle";
    case MODE_EXECUTE:
        return "Executing";
    default:
        return "Invalid";
    }
}

QString smp_group_shell_mgmt::command_to_string(uint8_t command)
{
    switch (command)
    {
    case COMMAND_EXECUTE:
        return "Executing";
    default:
        return "Invalid";
    }
}

bool smp_group_shell_mgmt::error_lookup(int32_t rc, QString *error)
{
    rc -= 2;

    if (rc < smp_error_values.length())
    {
        *error = smp_error_values.at(rc);
        return true;
    }

    return false;
}

bool smp_group_shell_mgmt::error_define_lookup(int32_t rc, QString *error)
{
    rc -= 2;

    if (rc < smp_error_defines.length())
    {
        *error = smp_error_defines.at(rc);
        return true;
    }

    return false;
}
