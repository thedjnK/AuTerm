/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_stat_mgmt.cpp
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
#include "smp_group_stat_mgmt.h"

enum modes : uint8_t {
    MODE_IDLE = 0,
    MODE_GROUP_DATA,
    MODE_LIST_GROUPS,
};

enum stat_mgmt_commands : uint8_t {
    COMMAND_GROUP_DATA = 0,
    COMMAND_LIST_GROUPS,
};

static QStringList smp_error_defines = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "INVALID_GROUP" <<
    "INVALID_STAT_NAME" <<
    "INVALID_STAT_SIZE" <<
    "WALK_ABORTED";

static QStringList smp_error_values = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "The provided statistic group name was not found" <<
    "The provided statistic name was not found" <<
    "The size of the statistic cannot be handled" <<
    "Walk through of statistics was aborted";

smp_group_stat_mgmt::smp_group_stat_mgmt(smp_processor *parent) : smp_group(parent, "STAT", SMP_GROUP_ID_STATS, error_lookup, error_define_lookup)
{
    mode = MODE_IDLE;
}

bool smp_group_stat_mgmt::parse_group_data_response(QCborStreamReader &reader, QString *key_name, QList<stat_value_t> *stats)
{
    //    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    bool keyset = true;
    stat_value_t stat;

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
            {
                if (key_name != nullptr && !key_name->isEmpty())
                {
                    stat.name = key;
                    stat.value = reader.toUnsignedInteger();
                    stats->append(stat);
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
    /*                else if (key == "o")
                    {
                        response->append(data);
                    }*/
                }

                break;
            }
            case QCborStreamReader::Array:
            case QCborStreamReader::Map:
            {
                reader.enterContainer();

                while (reader.lastError() == QCborError::NoError && reader.hasNext())
                {
                    parse_group_data_response(reader, &key, stats);
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

bool smp_group_stat_mgmt::parse_list_groups_response(QCborStreamReader &reader, QString *key_name, QStringList *groups)
{
    //    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    bool keyset = true;

    while (!reader.lastError() && reader.hasNext())
    {
        if (keyset == false && !key.isEmpty())
        {
            key.clear();
        }

        keyset = false;

        switch (reader.type())
        {
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
                    if (key_name != nullptr && *key_name == "stat_list")
                    {
                        groups->append(data);
                    }
                    else if (key.isEmpty())
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
                    parse_list_groups_response(reader, &key, groups);
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

void smp_group_stat_mgmt::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    Q_UNUSED(op);

    //    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        qDebug() << "Unexpected response, not busy";
        emit status(smp_user_data, STATUS_ERROR, nullptr);
    }
    else if (group != SMP_GROUP_ID_STATS)
    {
        qDebug() << "Unexpected group " << group << ", not " << SMP_GROUP_ID_STATS;
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

        if (finished_mode == MODE_GROUP_DATA && command == COMMAND_GROUP_DATA)
        {
            //Response to execute
            QCborStreamReader cbor_reader(data);
            stat_object->clear();
            bool good = parse_group_data_response(cbor_reader, nullptr, stat_object);

            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_LIST_GROUPS && command == COMMAND_LIST_GROUPS)
        {
            //Response to execute
            QCborStreamReader cbor_reader(data);
            group_object->clear();
            bool good = parse_list_groups_response(cbor_reader, nullptr, group_object);

            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else
        {
            qDebug() << "Unsupported command received";
        }
    }
}

void smp_group_stat_mgmt::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    Q_UNUSED(version);
    Q_UNUSED(op);
    Q_UNUSED(group);
    Q_UNUSED(error);

    bool cleanup = true;
    qDebug() << "error :(";

    if (command == COMMAND_GROUP_DATA && mode == MODE_GROUP_DATA)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, nullptr);
    }
    else if (command == COMMAND_LIST_GROUPS && mode == MODE_LIST_GROUPS)
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

void smp_group_stat_mgmt::timeout(smp_message *message)
{
    qDebug() << "timeout :(";

    //TODO:
    emit status(smp_user_data, STATUS_TIMEOUT, QString("Timeout (Mode: %1)").arg(mode_to_string(mode)));

    mode = MODE_IDLE;
}

void smp_group_stat_mgmt::cancel()
{
    if (mode != MODE_IDLE)
    {
        mode = MODE_IDLE;

        emit status(smp_user_data, STATUS_CANCELLED, nullptr);
    }
}

bool smp_group_stat_mgmt::start_group_data(QString name, QList<stat_value_t> *stats)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_STATS, COMMAND_GROUP_DATA);
    tmp_message->writer()->append("name");
    tmp_message->writer()->append(name);
    tmp_message->end_message();

    mode = MODE_GROUP_DATA;
    stat_object = stats;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_stat_mgmt::start_list_groups(QStringList *groups)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_STATS, COMMAND_LIST_GROUPS);
    tmp_message->end_message();

    mode = MODE_LIST_GROUPS;
    group_object = groups;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

QString smp_group_stat_mgmt::mode_to_string(uint8_t mode)
{
    switch (mode)
    {
    case MODE_IDLE:
        return "Idle";
    case MODE_GROUP_DATA:
        return "Group data";
    case MODE_LIST_GROUPS:
        return "Listing groups";
    default:
        return "Invalid";
    }
}

QString smp_group_stat_mgmt::command_to_string(uint8_t command)
{
    switch (command)
    {
    case COMMAND_GROUP_DATA:
        return "Group data";
    case COMMAND_LIST_GROUPS:
        return "Listing groups";
    default:
        return "Invalid";
    }
}

bool smp_group_stat_mgmt::error_lookup(int32_t rc, QString *error)
{
    rc -= 2;

    if (rc < smp_error_values.length())
    {
        *error = smp_error_values.at(rc);
        return true;
    }

    return false;
}

bool smp_group_stat_mgmt::error_define_lookup(int32_t rc, QString *error)
{
    rc -= 2;

    if (rc < smp_error_defines.length())
    {
        *error = smp_error_defines.at(rc);
        return true;
    }

    return false;
}
