/******************************************************************************
** Copyright (C) 2026 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_transport_mgmt.cpp
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
#include "smp_group_transport_mgmt.h"

/******************************************************************************/
// Defines
/******************************************************************************/
#define TRANSPORT_GRP_MGMT_LIST_TRANSPORTS_DATA_LAYER 3
#define TRANSPORT_GRP_MGMT_DETAILS_DATA_LAYER 3
#define TRANSPORT_GRP_MGMT_CONFIG_DETAILS_DATA_LAYER 3

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum modes : uint8_t {
    MODE_IDLE = 0,
    MODE_CONNECT,
    MODE_DISCONNECT,
    MODE_STATUS,
    MODE_LIST_TRANSPORTS,
    MODE_TRANSPORT_MODES,
    MODE_TRANSPORT_CONFIG_DETAILS,
};

enum transport_mgmt_commands : uint8_t {
    COMMAND_CONNECT = 0,
    COMMAND_DISCONNECT,
    COMMAND_STATUS,
    COMMAND_LIST_TRANSPORTS = 6,
    COMMAND_TRANSPORT_MODES,
    COMMAND_TRANSPORT_CONFIG_DETAILS,
};

/******************************************************************************/
// Constants
/******************************************************************************/
static const QStringList smp_error_defines = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "TRANSPORT_MISSING_REQUIRED_FUNCTION" <<
    "TRANSPORT_MISSING_INFO_FUNCTIONS" <<
    "INVALID_TRANSPORT" <<
    "INVALID_MODE" <<
    "ALL_CONTEXTS_USED" <<
    "BOTH_TRANSPORT_AND_ALL_PARAMETERS" <<
    "NOT_BRIDGED" <<
    "SAME_BRIDGE_DEVICE_DISALLOWED" <<
    "TRANSPORT_INGOING_NOT_SUPPORTED" <<
    "TRANSPORT_OUTGOING_NOT_SUPPORTED" <<
    "TRANSPORT_INCOMING_TRANSPORT_ALREADY_BRIDGED" <<
    "TRANSPORT_OUTGOING_TRANSPORT_ALREADY_BRIDGED" <<
    "CONNECT_MISSING_PARAMETER" <<
    "CONNECT_INVALID_PARAMETER" <<
    "CONNECT_UNSUPPORTED_PARAMETER" <<
    "CONNECT_FAILED" <<
    "CONNECTION_ESTABLISHMENT_FATAL_ERROR";

static const QStringList smp_error_values = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "The transport is missing the required mandatory bridging functions" <<
    "The transport is missing the information bridging functions" <<
    "Invalid, unsupported or no transport ID provided" <<
    "Invalid, unsupported or no mode provided" <<
    "All transport bridging context are in use" <<
    "The transport or all parameters were both provided and only one should be supplied" <<
    "The transport is not bridged" <<
    "The transport does not support being used as both the input and output bridge device (either because of the nature of the transport or the specific build configuration)" <<
    "The transport does not support being used as the ingoing part of a bridge" <<
    "The transport does not support being used as the outgoing part of a bridge" <<
    "The incoming transport is already bridged to another transport" <<
    "The outgoing transport is already bridged to another transport" <<
    "The connection data provided is missing a parameter" <<
    "An item in the connection data provided is invalid" <<
    "An item in the connection data provided is valid, but not supported by this device (or due to device configuration)" <<
    "A bridge connection attempt has failed" <<
    "A bridge connection attempt was successful but a fatal error was encountered during/after the connection was established. This might indicate that the underlying transport has a problem and is unusable";

static const QStringList transport_config_type_text_names = QStringList() <<
    "Unsigned integer" <<
    "Signed integer" <<
    "Boolean" <<
    "Text string" <<
    "Byte string";

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
smp_group_transport_mgmt::smp_group_transport_mgmt(smp_processor *parent) : smp_group(parent, "Transport", SMP_GROUP_ID_TRANSPORT, error_lookup, error_define_lookup)
{
    mode = MODE_IDLE;
}

bool smp_group_transport_mgmt::parse_status_response(QCborStreamReader &reader, struct transport_status_t *response)
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
                log_error() << "Error decoding string";
            }
            else if (key.isEmpty())
            {
                key = data;
                keyset = true;
            }

            break;
        }

        case QCborStreamReader::UnsignedInteger:
        {
            uint32_t *var = nullptr;

            if (key == "supported")
            {
                var = &response->supported;
            }
            else if (key == "active")
            {
                var = &response->active;
            }

            if (var != nullptr)
            {
                *var = (uint32_t)reader.toUnsignedInteger();
            }

            reader.next();
            break;
        }

        case QCborStreamReader::SimpleType:
        {
            if (reader.toSimpleType() == QCborSimpleType::Null || reader.toSimpleType() == QCborSimpleType::Undefined)
            {
                //TODO
            }
            else if (key == "bridged")
            {
                response->bridged = (bool)(reader.toSimpleType() == QCborSimpleType::False ? false : true);
            }

            reader.next();
            break;
        }

        case QCborStreamReader::Array:
        case QCborStreamReader::Map:
        {
            reader.enterContainer();
            while (reader.lastError() == QCborError::NoError && reader.hasNext())
            {
                parse_status_response(reader, response);
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

bool smp_group_transport_mgmt::parse_list_transports_response(QCborStreamReader &reader, uint8_t layers, QList<struct transport_transports_t> *transports, bool *transports_found)
{
    QString key = "";
    bool keyset = true;
    struct transport_transports_t temp_transport;
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
                else if (layers == TRANSPORT_GRP_MGMT_LIST_TRANSPORTS_DATA_LAYER && key == "name")
                {
                    temp_transport.name = data;
                }
            }

            break;
        }

        case QCborStreamReader::UnsignedInteger:
        {
            if (layers == TRANSPORT_GRP_MGMT_LIST_TRANSPORTS_DATA_LAYER && key == "id")
            {
                temp_transport.id = reader.toUnsignedInteger();
                data_found = true;
            }
            reader.next();
            break;
        }

        case QCborStreamReader::Array:
        case QCborStreamReader::Map:
        {
            if (reader.type() == QCborStreamReader::Array && key == "transports")
            {
                *transports_found = true;
            }

            reader.enterContainer();
            while (reader.lastError() == QCborError::NoError && reader.hasNext())
            {
                parse_list_transports_response(reader, (layers + 1), transports, transports_found);
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

    if (layers == TRANSPORT_GRP_MGMT_LIST_TRANSPORTS_DATA_LAYER && data_found == true && *transports_found == true)
    {
        transports->append(temp_transport);
    }

    return true;
}

bool smp_group_transport_mgmt::parse_details_response(QCborStreamReader &reader, uint8_t layers, QList<struct transport_modes_t> *modes, bool *modes_found)
{
    QString key = "";
    bool keyset = true;
    struct transport_modes_t temp_mode = {0};
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
                else if (layers == TRANSPORT_GRP_MGMT_DETAILS_DATA_LAYER && key == "description")
                {
                    temp_mode.description = data;
                }
            }

            break;
        }

        case QCborStreamReader::UnsignedInteger:
        {
            if (layers == TRANSPORT_GRP_MGMT_DETAILS_DATA_LAYER && key == "id")
            {
                temp_mode.id = reader.toUnsignedInteger();
                data_found = true;
            }
            reader.next();
            break;
        }

        case QCborStreamReader::SimpleType:
        {
            if (reader.toSimpleType() == QCborSimpleType::Null || reader.toSimpleType() == QCborSimpleType::Undefined)
            {
                //TODO
            }
            else if (layers == TRANSPORT_GRP_MGMT_DETAILS_DATA_LAYER && key == "incoming")
            {
                temp_mode.incoming = (bool)(reader.toSimpleType() == QCborSimpleType::False ? false : true);
            }
            else if (layers == TRANSPORT_GRP_MGMT_DETAILS_DATA_LAYER && key == "outgoing")
            {
                temp_mode.outgoing = (bool)(reader.toSimpleType() == QCborSimpleType::False ? false : true);
            }

            reader.next();
            break;
        }

        case QCborStreamReader::Array:
        case QCborStreamReader::Map:
        {
            if (reader.type() == QCborStreamReader::Array && key == "modes")
            {
                *modes_found = true;
            }

            reader.enterContainer();
            while (reader.lastError() == QCborError::NoError && reader.hasNext())
            {
                parse_details_response(reader, (layers + 1), modes, modes_found);
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

    if (layers == TRANSPORT_GRP_MGMT_DETAILS_DATA_LAYER && data_found == true && *modes_found == true)
    {
        modes->append(temp_mode);
    }

    return true;
}

bool smp_group_transport_mgmt::parse_config_details_response(QCborStreamReader &reader, uint8_t layers, QList<struct transport_config_details_t> *configs, bool *configs_found)
{
    QString key = "";
    bool keyset = true;
    struct transport_config_details_t temp_config = {0};
    bool name_found = false;
    bool type_found = false;

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
                log_error() << "Error decoding string";
            }
            else
            {
                if (key.isEmpty())
                {
                    key = data;
                    keyset = true;
                }
                else if (layers == TRANSPORT_GRP_MGMT_CONFIG_DETAILS_DATA_LAYER && key == "name")
                {
                    temp_config.name = data;
                    name_found = true;
                }
            }

            break;
        }

        case QCborStreamReader::UnsignedInteger:
        {
            if (layers == TRANSPORT_GRP_MGMT_CONFIG_DETAILS_DATA_LAYER && key == "type")
            {
                temp_config.type = (enum transport_config_type_t)reader.toUnsignedInteger();
                type_found = true;
            }
            reader.next();
            break;
        }

        case QCborStreamReader::SimpleType:
        {
            if (reader.toSimpleType() == QCborSimpleType::Null || reader.toSimpleType() == QCborSimpleType::Undefined)
            {
                //TODO
            }
            else if (layers == TRANSPORT_GRP_MGMT_CONFIG_DETAILS_DATA_LAYER && key == "required")
            {
                temp_config.required = (bool)(reader.toSimpleType() == QCborSimpleType::False ? false : true);
            }

            reader.next();
            break;
        }

        case QCborStreamReader::Array:
        case QCborStreamReader::Map:
        {
            if (reader.type() == QCborStreamReader::Array && key == "configs")
            {
                *configs_found = true;
            }

            reader.enterContainer();
            while (reader.lastError() == QCborError::NoError && reader.hasNext())
            {
                parse_config_details_response(reader, (layers + 1), configs, configs_found);
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

    if (layers == TRANSPORT_GRP_MGMT_CONFIG_DETAILS_DATA_LAYER && name_found == true && type_found == true && *configs_found == true)
    {
        configs->append(temp_config);
    }

    return true;
}

void smp_group_transport_mgmt::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    Q_UNUSED(op);
    //    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        log_error() << "Unexpected response, not busy";
        emit status(smp_user_data, STATUS_ERROR, "Unexpected response, transport mgmt not busy");
    }
    else if (group != SMP_GROUP_ID_TRANSPORT)
    {
        log_error() << "Unexpected group " << group << ", not " << SMP_GROUP_ID_TRANSPORT;
        emit status(smp_user_data, STATUS_ERROR, "Unexpected group, not transport mgmt");
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

        if (finished_mode == MODE_CONNECT && command == COMMAND_CONNECT)
        {
            //No need to check response, it would have returned success to come through this callback
            cleanup();
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_DISCONNECT && command == COMMAND_DISCONNECT)
        {
            //No need to check response, it would have returned success to come through this callback
            cleanup();
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_STATUS && command == COMMAND_STATUS)
        {
            //Response to bridge status
            QCborStreamReader cbor_reader(data);
            bool good = parse_status_response(cbor_reader, status_response);

            cleanup();
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_LIST_TRANSPORTS && command == COMMAND_LIST_TRANSPORTS)
        {
            //Response to bridge status
            QCborStreamReader cbor_reader(data);
            bool transports_found = false;
            bool good = parse_list_transports_response(cbor_reader, 0, transports_response, &transports_found);

            cleanup();

            if (good == true)
            {
                if (transports_found == true)
                {
                    emit status(smp_user_data, STATUS_COMPLETE, nullptr);
                }
                else
                {
                    emit status(smp_user_data, STATUS_ERROR, "Missing transports parameter");
                }
            }
            else
            {
                emit status(smp_user_data, STATUS_ERROR, "Did not decode response successfully");
            }
        }
        else if (finished_mode == MODE_TRANSPORT_MODES && command == COMMAND_TRANSPORT_MODES)
        {
            //Response to bridge status
            QCborStreamReader cbor_reader(data);
            bool modes_found = false;
            bool good = parse_details_response(cbor_reader, 0, modes_response, &modes_found);

            cleanup();

            if (good == true)
            {
                if (modes_found == true)
                {
                    emit status(smp_user_data, STATUS_COMPLETE, nullptr);
                }
                else
                {
                    emit status(smp_user_data, STATUS_ERROR, "Missing modes parameter");
                }
            }
            else
            {
                emit status(smp_user_data, STATUS_ERROR, "Did not decode response successfully");
            }
        }
        else if (finished_mode == MODE_TRANSPORT_CONFIG_DETAILS && command == COMMAND_TRANSPORT_CONFIG_DETAILS)
        {
            //Response to bridge status
            QCborStreamReader cbor_reader(data);
            bool configs_found = false;
            bool good = parse_config_details_response(cbor_reader, 0, config_details_response, &configs_found);

            cleanup();

            if (good == true)
            {
                if (configs_found == true)
                {
                    emit status(smp_user_data, STATUS_COMPLETE, nullptr);
                }
                else
                {
                    emit status(smp_user_data, STATUS_ERROR, "Missing configs parameter");
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

void smp_group_transport_mgmt::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    Q_UNUSED(version);
    Q_UNUSED(op);
    Q_UNUSED(group);
    Q_UNUSED(error);

    bool run_cleanup = true;
    log_error() << "error :(";

    if (mode == MODE_CONNECT && command == COMMAND_CONNECT)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (mode == MODE_DISCONNECT && command == COMMAND_DISCONNECT)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (mode == MODE_STATUS && command == COMMAND_STATUS)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (mode == MODE_LIST_TRANSPORTS && command == COMMAND_LIST_TRANSPORTS)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (mode == MODE_TRANSPORT_MODES && command == COMMAND_TRANSPORT_MODES)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (mode == MODE_TRANSPORT_CONFIG_DETAILS && command == COMMAND_TRANSPORT_CONFIG_DETAILS)
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

void smp_group_transport_mgmt::cancel()
{
    if (mode != MODE_IDLE)
    {
        cleanup();
        emit status(smp_user_data, STATUS_CANCELLED, nullptr);
    }
}

bool smp_group_transport_mgmt::start_connect(uint8_t transport, transport_connect_params_t *items)
{
    QMapIterator<QString, QVariant> i(*items);
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_TRANSPORT, COMMAND_CONNECT, (1 + items->count()));

    tmp_message->writer()->append("transport");
    tmp_message->writer()->append(transport);

    while (i.hasNext())
    {
        i.next();
        tmp_message->writer()->append(i.key());
        switch (i.value().typeId())
        {
//TODO: ulonglong?
            case QMetaType::Bool:
            {
                tmp_message->writer()->append(i.value().toBool());
                break;
            }
            case QMetaType::UInt:
            {
                tmp_message->writer()->append(i.value().toUInt());
                break;
            }
            case QMetaType::Int:
            {
                tmp_message->writer()->append(i.value().toInt());
                break;
            }
            case QMetaType::QString:
            {
                tmp_message->writer()->append(i.value().toString());
                break;
            }
            case QMetaType::QByteArray:
            {
                tmp_message->writer()->append(i.value().toByteArray());
                break;
            }
            default:
            {
                log_error() << "Invalid configuration option type supplied: " << i.value().typeId();
            }
        };
    }

    tmp_message->end_message();

    mode = MODE_CONNECT;

    //	    qDebug() << "len: " << message.length();

    if (check_message_before_send(tmp_message) == false)
    {
        return false;
    }

    return handle_transport_error(processor->send(tmp_message, smp_timeout, smp_retries, true));
}

bool smp_group_transport_mgmt::start_disconnect(uint8_t transport)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_TRANSPORT, COMMAND_DISCONNECT, 1);
    //    tmp_message->writer()->append("transport");
    //    tmp_message->writer()->append(transport);
    tmp_message->writer()->append("all");
    tmp_message->writer()->append(true);
    tmp_message->end_message();

    mode = MODE_DISCONNECT;

    //	    qDebug() << "len: " << message.length();

    if (check_message_before_send(tmp_message) == false)
    {
        return false;
    }

    return handle_transport_error(processor->send(tmp_message, smp_timeout, smp_retries, true));
}

bool smp_group_transport_mgmt::start_status(struct transport_status_t *response)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_TRANSPORT, COMMAND_STATUS, 0);
    tmp_message->end_message();

    mode = MODE_STATUS;
    status_response = response;
    status_response->supported = 0;
    status_response->active = 0;
    status_response->bridged = false;

    //	    qDebug() << "len: " << message.length();

    if (check_message_before_send(tmp_message) == false)
    {
        return false;
    }

    return handle_transport_error(processor->send(tmp_message, smp_timeout, smp_retries, true));
}

bool smp_group_transport_mgmt::start_list_transports(QList<transport_transports_t> *response)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_TRANSPORT, COMMAND_LIST_TRANSPORTS, 0);
    tmp_message->end_message();

    mode = MODE_LIST_TRANSPORTS;
    transports_response = response;
    transports_response->clear();

    //	    qDebug() << "len: " << message.length();

    if (check_message_before_send(tmp_message) == false)
    {
        return false;
    }

    return handle_transport_error(processor->send(tmp_message, smp_timeout, smp_retries, true));
}

bool smp_group_transport_mgmt::start_details(uint8_t transport, QList<transport_modes_t> *response)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_TRANSPORT, COMMAND_TRANSPORT_MODES, 1);
    tmp_message->writer()->append("transport");
    tmp_message->writer()->append(transport);
    tmp_message->end_message();

    mode = MODE_TRANSPORT_MODES;
    modes_response = response;
    modes_response->clear();

    //	    qDebug() << "len: " << message.length();

    if (check_message_before_send(tmp_message) == false)
    {
        return false;
    }

    return handle_transport_error(processor->send(tmp_message, smp_timeout, smp_retries, true));
}

bool smp_group_transport_mgmt::start_config_details(uint8_t transport, uint8_t mode, QList<transport_config_details_t> *response)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_TRANSPORT, COMMAND_TRANSPORT_CONFIG_DETAILS, 2);
    tmp_message->writer()->append("transport");
    tmp_message->writer()->append(transport);
    tmp_message->writer()->append("mode");
    tmp_message->writer()->append(mode);
    tmp_message->end_message();

    this->mode = MODE_TRANSPORT_CONFIG_DETAILS;
    config_details_response = response;
    config_details_response->clear();

    //	    qDebug() << "len: " << message.length();

    if (check_message_before_send(tmp_message) == false)
    {
        return false;
    }

    return handle_transport_error(processor->send(tmp_message, smp_timeout, smp_retries, true));
}

QString smp_group_transport_mgmt::mode_to_string(uint8_t mode)
{
    switch (mode)
    {
        case MODE_IDLE:
            return "Idle";
        case MODE_CONNECT:
            return "Connect";
        case MODE_DISCONNECT:
            return "Disconnect";
        case MODE_STATUS:
            return "Status";
        case MODE_LIST_TRANSPORTS:
            return "List";
        case MODE_TRANSPORT_MODES:
            return "Get modes";
        case MODE_TRANSPORT_CONFIG_DETAILS:
            return "Config details";
        default:
            return "Invalid";
    }
}

QString smp_group_transport_mgmt::command_to_string(uint8_t command)
{
    switch (command)
    {
        case COMMAND_CONNECT:
            return "Connect";
        case COMMAND_DISCONNECT:
            return "Disconnect";
        case COMMAND_STATUS:
            return "Status";
        case COMMAND_LIST_TRANSPORTS:
            return "List";
        case COMMAND_TRANSPORT_MODES:
            return "Get modes";
        case COMMAND_TRANSPORT_CONFIG_DETAILS:
            return "Config details";
        default:
            return "Invalid";
    }
}

bool smp_group_transport_mgmt::error_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_values.length())
    {
        *error = smp_error_values.at(rc);
        return true;
    }

    return false;
}

bool smp_group_transport_mgmt::error_define_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_defines.length())
    {
        *error = smp_error_defines.at(rc);
        return true;
    }

    return false;
}

void smp_group_transport_mgmt::cleanup()
{
    mode = MODE_IDLE;
    processor->cancel();
}

QString smp_group_transport_mgmt::config_type_to_string(enum transport_config_type_t type, bool *valid)
{
    if (type > TRANSPORT_CONFIG_TYPE_COUNT)
    {
        if (valid != NULL)
        {
            *valid = false;
        }

        return "";
    }

    if (valid != NULL)
    {
        *valid = true;
    }

    return transport_config_type_text_names.at(type);
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
