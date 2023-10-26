/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_custom.cpp
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
#include "smp_group_custom.h"

enum modes : uint8_t {
    MODE_IDLE = 0,
    MODE_COMMAND,
};


static QStringList smp_error_defines = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "INVALID_FORMAT" <<
    "QUERY_YIELDS_NO_ANSWER";

static QStringList smp_error_values = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "The provided format value is not valid" <<
    "Query was not recognized";

smp_group_custom::smp_group_custom(smp_processor *parent) : smp_group(parent, "Custom", SMP_GROUP_ID_CUSTOM, error_lookup, error_define_lookup)
{
    mode = MODE_IDLE;
}

bool smp_group_custom::parse_command_response(QCborStreamReader &reader, QString *response)
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
                    parse_command_response(reader, response);
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


void smp_group_custom::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    Q_UNUSED(op);
    // qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        log_error() << "Unexpected response, not busy";
        emit status(smp_user_data, STATUS_ERROR, "Unexpected response, shell mgmt not busy");
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

        if (finished_mode == MODE_COMMAND )
        {
            log_debug() << "decoding custom echo";
            //Response to set image state
            QString response;
            QCborStreamReader cbor_reader(data);
            bool good = parse_command_response(cbor_reader, &response);
            emit status(smp_user_data, STATUS_COMPLETE, response);
        }
      
        else
        {
            log_error() << "Unsupported command received";
        }
    }
}

void smp_group_custom::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    Q_UNUSED(version);
    Q_UNUSED(op);
    Q_UNUSED(group);
    Q_UNUSED(error);

    bool cleanup = true;
    log_error() << "error :(";

    if (mode == MODE_COMMAND)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else
    {
        //Unexpected response operation for mode
        emit status(smp_user_data, STATUS_ERROR, QString("Unexpected error (Mode: %1)").arg(mode_to_string(mode)));
    }

    if (cleanup == true)
    {
        mode = MODE_IDLE;
    }
}

void smp_group_custom::timeout(smp_message *message)
{
    log_error() << "timeout :( custom";

    //TODO:
    emit status(smp_user_data, STATUS_TIMEOUT, QString("Timeout (Mode: %1)").arg(mode_to_string(mode)));

    mode = MODE_IDLE;
}

void smp_group_custom::cancel()
{
    if (mode != MODE_IDLE)
    {
        mode = MODE_IDLE;

        emit status(smp_user_data, STATUS_CANCELLED, nullptr);
    }
}

bool smp_group_custom::start_command(int group_id, int command_id, QString data)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, group_id, command_id);
    tmp_message->writer()->append("d");
    tmp_message->writer()->append(data);
    tmp_message->end_message();

    mode = MODE_COMMAND;

    //	    qDebug() << "len: " << message.length();

    processor->send(SMP_GROUP_ID_CUSTOM, tmp_message, smp_timeout, smp_retries, true);

    return true;
}



QString smp_group_custom::mode_to_string(uint8_t mode)
{
    switch (mode)
    {
        case MODE_IDLE:
            return "Idle";
        case MODE_COMMAND:
            return "Command";
        default:
            return "Invalid";
    }
}



bool smp_group_custom::error_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_values.length())
    {
        *error = smp_error_values.at(rc);
        return true;
    }

    return false;
}

bool smp_group_custom::error_define_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_defines.length())
    {
        *error = smp_error_defines.at(rc);
        return true;
    }

    return false;
}
