/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_enum_mgmt.cpp
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

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "smp_group_enum_mgmt.h"
#include "smp_message.h"

/******************************************************************************/
// Defines
/******************************************************************************/
#define ENUM_GRP_MGMT_GROUP_DETAILS_DATA_LAYER 3

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum modes : uint8_t {
    MODE_IDLE = 0,
    MODE_COUNT,
    MODE_LIST,
    MODE_SINGLE,
    MODE_DETAILS,
};

enum enum_mgmt_commands : uint8_t {
    COMMAND_COUNT = 0,
    COMMAND_LIST,
    COMMAND_SINGLE,
    COMMAND_DETAILS
};

/******************************************************************************/
// Constants
/******************************************************************************/
static const QStringList smp_error_defines = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "TOO_MANY_GROUP_ENTRIES" <<
    "INSUFFICIENT_HEAP_FOR_ENTRIES";

static const QStringList smp_error_values = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "Too many group entries were provided" <<
    "Insufficient heap memory to store entry data";

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
smp_group_enum_mgmt::smp_group_enum_mgmt(smp_processor *parent) : smp_group(parent, "ENUM", SMP_GROUP_ID_ENUM, error_lookup, error_define_lookup)
{
    mode = MODE_IDLE;
}

bool smp_group_enum_mgmt::parse_count_response(QCborStreamReader &reader, uint16_t *count, bool *count_found)
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
                if (key == "count")
                {
                    //			    qDebug() << "found count";
                    *count = (uint16_t)reader.toUnsignedInteger();
                    *count_found = true;
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
                    log_error() << "Error decoding string";
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
                    parse_count_response(reader, count, count_found);
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

bool smp_group_enum_mgmt::parse_list_response(QCborStreamReader &reader, const QString *list_key, QList<uint16_t> *groups, bool *groups_found)
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
            if (*list_key != nullptr && *list_key == "groups")
            {
                //			    qDebug() << "found groups";
                groups->append((uint16_t)reader.toUnsignedInteger());
                *groups_found = true;
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
                log_error() << "Error decoding string";
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
                parse_list_response(reader, &key, groups, groups_found);
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

bool smp_group_enum_mgmt::parse_single_response(QCborStreamReader &reader, uint16_t *id, bool *end, bool *group_found)
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
        case QCborStreamReader::SimpleType:
        {
            if (key == "end")
            {
                //			    qDebug() << "found group";
                *end = reader.toBool();
            }

            reader.next();
            break;
        }

        case QCborStreamReader::UnsignedInteger:
        {
            if (key == "group")
            {
                //			    qDebug() << "found group";
                *id = (uint16_t)reader.toUnsignedInteger();
                *group_found = true;
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
                log_error() << "Error decoding string";
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
                parse_single_response(reader, id, end, group_found);
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

bool smp_group_enum_mgmt::parse_details_response(QCborStreamReader &reader, uint8_t layers, QList<enum_details_t> *groups, bool *groups_found, enum_fields_present_t *fields_present)
{
    QString key = "";
    bool keyset = true;
    enum_details_t temp_group;
    bool data_found = false;

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
            if (layers == ENUM_GRP_MGMT_GROUP_DETAILS_DATA_LAYER)
            {
                if (key == "group")
                {
                    //			    qDebug() << "found group";
                    temp_group.id = (uint16_t)reader.toUnsignedInteger();
                    data_found = true;
                    fields_present->id = true;
                }
                else if (key == "handlers")
                {
                    //			    qDebug() << "found handlers";
                    temp_group.handlers = (uint16_t)reader.toUnsignedInteger();
                    data_found = true;
                    fields_present->handlers = true;
                }
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
                log_error() << "Error decoding string";
            }
            else
            {
                if (key.isEmpty())
                {
                    key = data;
                    keyset = true;
                }
                else if (layers == ENUM_GRP_MGMT_GROUP_DETAILS_DATA_LAYER && key == "name")
                {
                    //			    qDebug() << "found name";
                    temp_group.name = data;
                    data_found = true;
                    fields_present->name = true;
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
                parse_details_response(reader, (layers + 1), groups, groups_found, fields_present);
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

    if (layers == ENUM_GRP_MGMT_GROUP_DETAILS_DATA_LAYER && data_found == true)
    {
        groups->append(temp_group);
        *groups_found = true;
    }

    return true;
}

void smp_group_enum_mgmt::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    Q_UNUSED(op);

//    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        log_error() << "Unexpected response, not busy";
        emit status(smp_user_data, STATUS_ERROR, "Unexpected response, shell mgmt not busy");
    }
    else if (group != SMP_GROUP_ID_ENUM)
    {
        log_error() << "Unexpected group " << group << ", not " << SMP_GROUP_ID_ENUM;
        emit status(smp_user_data, STATUS_ERROR, "Unexpected group, not img mgmt");
    }
    else
    {
        if (version != smp_version)
        {
            //The target device does not support the SMP version being used, adjust for duration of transfer and raise a warning to the parent
            smp_version = version;
            emit version_error(version);
        }

        if (mode == MODE_COUNT && command == COMMAND_COUNT)
        {
            //Response to count command groups
            QCborStreamReader cbor_reader(data);
            bool count_found = false;
            bool good = parse_count_response(cbor_reader, groups_count, &count_found);

            cleanup();

            if (good == true)
            {
                if (count_found == true)
                {
                    emit status(smp_user_data, STATUS_COMPLETE, nullptr);
                }
                else
                {
                    emit status(smp_user_data, STATUS_ERROR, "Missing count parameter");
                }
            }
            else
            {
                emit status(smp_user_data, STATUS_ERROR, "Did not decode response successfully");
            }
        }
        else if (mode == MODE_LIST && command == COMMAND_LIST)
        {
            //Response to list command groups
            QCborStreamReader cbor_reader(data);
            bool groups_found = false;
            bool good = parse_list_response(cbor_reader, nullptr, groups_list, &groups_found);

            cleanup();

            if (good == true)
            {
                if (groups_found == true)
                {
                    emit status(smp_user_data, STATUS_COMPLETE, nullptr);
                }
                else
                {
                    emit status(smp_user_data, STATUS_ERROR, "Missing groups parameter");
                }
            }
            else
            {
                emit status(smp_user_data, STATUS_ERROR, "Did not decode response successfully");
            }
        }
        else if (mode == MODE_SINGLE && command == COMMAND_SINGLE)
        {
            //Response to fetch command group ID
            QCborStreamReader cbor_reader(data);
            bool group_found = false;
            bool good = parse_single_response(cbor_reader, group_single_id, group_single_end, &group_found);

            cleanup();

            if (good == true)
            {
                if (group_found == true)
                {
                    emit status(smp_user_data, STATUS_COMPLETE, nullptr);
                }
                else
                {
                    emit status(smp_user_data, STATUS_ERROR, "Missing group parameter");
                }
            }
            else
            {
                emit status(smp_user_data, STATUS_ERROR, "Did not decode response successfully");
            }
        }
        else if (mode == MODE_DETAILS && command == COMMAND_DETAILS)
        {
            //Response to get command group details
            QCborStreamReader cbor_reader(data);
            bool groups_found = false;
            bool good = parse_details_response(cbor_reader, 0, groups_details, &groups_found, groups_details_fields_present);

            cleanup();

            if (good == true)
            {
                if (groups_found == true)
                {
                    emit status(smp_user_data, STATUS_COMPLETE, nullptr);
                }
                else
                {
                    emit status(smp_user_data, STATUS_ERROR, "Missing groups parameter");
                }
            }
            else
            {
                emit status(smp_user_data, STATUS_ERROR, "Did not decode response successfully");
            }
        }
        else
        {
            log_error() << "Unsupported command received";
            cleanup();
        }
    }
}

void smp_group_enum_mgmt::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    Q_UNUSED(version);
    Q_UNUSED(op);
    Q_UNUSED(group);
    Q_UNUSED(error);

    bool run_cleanup = true;
    log_error() << "error :(";

    if (command == COMMAND_COUNT && mode == MODE_COUNT)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_LIST && mode == MODE_LIST)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_SINGLE && mode == MODE_SINGLE)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_DETAILS && mode == MODE_DETAILS)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else
    {
        //Unexpected response operation for mode
        emit status(smp_user_data, STATUS_ERROR, QString("Unexpected error (Mode: %1, op: %2)").arg(mode_to_string(mode), command_to_string(command)));
    }

    if (run_cleanup == true)
    {
        cleanup();
    }
}

void smp_group_enum_mgmt::cancel()
{
    if (mode != MODE_IDLE)
    {
        cleanup();
        emit status(smp_user_data, STATUS_CANCELLED, nullptr);
    }
}

bool smp_group_enum_mgmt::start_enum_count(uint16_t *count)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_ENUM, COMMAND_COUNT, 0);
    tmp_message->end_message();

    mode = MODE_COUNT;
    groups_count = count;
    *groups_count = 0;

    //	    qDebug() << "len: " << message.length();

    if (check_message_before_send(tmp_message) == false)
    {
        return false;
    }

    return handle_transport_error(processor->send(tmp_message, smp_timeout, smp_retries, true));
}

bool smp_group_enum_mgmt::start_enum_list(QList<uint16_t> *groups)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_ENUM, COMMAND_LIST, 0);
    tmp_message->end_message();

    mode = MODE_LIST;
    groups_list = groups;
    groups_list->clear();

    //	    qDebug() << "len: " << message.length();

    if (check_message_before_send(tmp_message) == false)
    {
        return false;
    }

    return handle_transport_error(processor->send(tmp_message, smp_timeout, smp_retries, true));
}

bool smp_group_enum_mgmt::start_enum_single(uint16_t index, uint16_t *id, bool *end)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_ENUM, COMMAND_SINGLE, (index > 0 ? 1 : 0));

    if (index > 0)
    {
        tmp_message->writer()->append("index");
        tmp_message->writer()->append((uint32_t)index);
    }

    tmp_message->end_message();

    mode = MODE_SINGLE;
    group_single_id = id;
    group_single_end = end;
    *group_single_id = 0;
    *group_single_end = false;

    //	    qDebug() << "len: " << message.length();

    if (check_message_before_send(tmp_message) == false)
    {
        return false;
    }

    return handle_transport_error(processor->send(tmp_message, smp_timeout, smp_retries, true));
}

bool smp_group_enum_mgmt::start_enum_details(QList<enum_details_t> *groups, enum_fields_present_t *fields_present)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_ENUM, COMMAND_DETAILS, 0);
    tmp_message->end_message();

    mode = MODE_DETAILS;
    groups_details = groups;
    groups_details_fields_present = fields_present;
    groups_details->clear();
    groups_details_fields_present->id = false;
    groups_details_fields_present->name = false;
    groups_details_fields_present->handlers = false;

    //	    qDebug() << "len: " << message.length();

    if (check_message_before_send(tmp_message) == false)
    {
        return false;
    }

    return handle_transport_error(processor->send(tmp_message, smp_timeout, smp_retries, true));
}

QString smp_group_enum_mgmt::mode_to_string(uint8_t mode)
{
    switch (mode)
    {
        case MODE_IDLE:
            return "Idle";
        case MODE_COUNT:
            return "Count command groups";
        case MODE_LIST:
            return "List command groups";
        case MODE_SINGLE:
            return "Fetch single command group ID";
        case MODE_DETAILS:
            return "Get command group details";
        default:
            return "Invalid";
    }
}

QString smp_group_enum_mgmt::command_to_string(uint8_t command)
{
    switch (command)
    {
        case COMMAND_COUNT:
            return "Count command groups";
        case COMMAND_LIST:
            return "List command groups";
        case COMMAND_SINGLE:
            return "Fetch single command group ID";
        case COMMAND_DETAILS:
            return "Get command group details";
        default:
            return "Invalid";
    }
}

bool smp_group_enum_mgmt::error_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_values.length())
    {
        *error = smp_error_values.at(rc);
        return true;
    }

    return false;
}

bool smp_group_enum_mgmt::error_define_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_defines.length())
    {
        *error = smp_error_defines.at(rc);
        return true;
    }

    return false;
}

void smp_group_enum_mgmt::cleanup()
{
    mode = MODE_IDLE;
    groups_count = nullptr;
    groups_list = nullptr;
    group_single_id = nullptr;
    group_single_end = nullptr;
    groups_details = nullptr;
    groups_details_fields_present = nullptr;
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
