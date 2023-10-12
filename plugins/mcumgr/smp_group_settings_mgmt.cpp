/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_settings_mgmt.cpp
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
#include "smp_group_settings_mgmt.h"

enum modes : uint8_t {
    MODE_IDLE = 0,
    MODE_READ,
    MODE_WRITE,
    MODE_DELETE,
    MODE_COMMIT,
    MODE_LOAD,
    MODE_SAVE,
};

enum settings_mgmt_commands : uint8_t {
    COMMAND_READ_WRITE = 0,
    COMMAND_DELETE,
    COMMAND_COMMIT,
    COMMAND_LOAD_SAVE,
};

static QStringList smp_error_defines = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "KEY_TOO_LONG" <<
    "KEY_NOT_FOUND" <<
    "READ_NOT_SUPPORTED" <<
    "ROOT_KEY_NOT_FOUND" <<
    "WRITE_NOT_SUPPORTED" <<
    "DELETE_NOT_SUPPORTED";

static QStringList smp_error_values = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "The provided key name is too long to be used" <<
    "The provided key name does not exist" <<
    "The provided key name does not support being read" <<
    "The provided root key name does not exist" <<
    "The provided key name does not support being written" <<
    "The provided key name does not support being deleted";

smp_group_settings_mgmt::smp_group_settings_mgmt(smp_processor *parent) : smp_group(parent, "SETTINGS", SMP_GROUP_ID_SETTINGS, error_lookup, error_define_lookup)
{
    mode = MODE_IDLE;
}

bool smp_group_settings_mgmt::parse_read_response(QCborStreamReader &reader, QByteArray *value)
{
    //    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    bool keyset = true;

    value->clear();

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
                if (key == "max_size")
                {
                    //TODO
                    //*ret = (int32_t)reader.toInteger();
                }

                reader.next();
                break;
            }
            case QCborStreamReader::ByteArray:
            {
                QByteArray data;
                auto r = reader.readByteArray();
                while (r.status == QCborStreamReader::Ok)
                {
                    data.append(r.data);
                    r = reader.readByteArray();
                }

                if (r.status == QCborStreamReader::Error)
                {
                    data.clear();
                    qDebug() << "Error decoding byte array";
                }
                else if (key == "val")
                {
                    *value = data;
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
                }

                break;
            }
            case QCborStreamReader::Array:
            case QCborStreamReader::Map:
            {
                reader.enterContainer();

                while (reader.lastError() == QCborError::NoError && reader.hasNext())
                {
                    parse_read_response(reader, value);
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

void smp_group_settings_mgmt::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    Q_UNUSED(op);
    //    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        qDebug() << "Unexpected response, not busy";
        emit status(smp_user_data, STATUS_ERROR, "Unexpected response, settings mgmt not busy");
    }
    else if (group != SMP_GROUP_ID_SETTINGS)
    {
        qDebug() << "Unexpected group " << group << ", not " << SMP_GROUP_ID_SETTINGS;
        emit status(smp_user_data, STATUS_ERROR, "Unexpected group, not settings mgmt");
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

        if (finished_mode == MODE_READ && command == COMMAND_READ_WRITE)
        {
            //Response to read
            QCborStreamReader cbor_reader(data);
            bool good = parse_read_response(cbor_reader, return_value);

            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_WRITE && command == COMMAND_READ_WRITE)
        {
            //Response to write
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_DELETE && command == COMMAND_DELETE)
        {
            //Response to delete
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_COMMIT && command == COMMAND_COMMIT)
        {
            //Response to commit
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_LOAD && command == COMMAND_LOAD_SAVE)
        {
            //Response to load
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (finished_mode == MODE_SAVE && command == COMMAND_LOAD_SAVE)
        {
            //Response to save
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else
        {
            qDebug() << "Unsupported command received";
        }
    }
}

void smp_group_settings_mgmt::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    Q_UNUSED(version);
    Q_UNUSED(op);
    Q_UNUSED(group);
    Q_UNUSED(error);

    bool cleanup = true;
    qDebug() << "error :(";

    if (command == COMMAND_READ_WRITE && mode == MODE_READ)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_READ_WRITE && mode == MODE_WRITE)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_DELETE && mode == MODE_DELETE)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_COMMIT && mode == MODE_COMMIT)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_LOAD_SAVE && mode == MODE_LOAD)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_LOAD_SAVE && mode == MODE_SAVE)
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

void smp_group_settings_mgmt::timeout(smp_message *message)
{
    qDebug() << "timeout :(";

    //TODO:
    emit status(smp_user_data, STATUS_TIMEOUT, QString("Timeout (Mode: %1)").arg(mode_to_string(mode)));

    mode = MODE_IDLE;
}

void smp_group_settings_mgmt::cancel()
{
    if (mode != MODE_IDLE)
    {
        mode = MODE_IDLE;

        emit status(smp_user_data, STATUS_CANCELLED, nullptr);
    }
}

bool smp_group_settings_mgmt::start_read(QString name, uint32_t max_length, QByteArray *value)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_SETTINGS, COMMAND_READ_WRITE);
    tmp_message->writer()->append("name");
    tmp_message->writer()->append(name);

    if (max_length > 0)
    {
        tmp_message->writer()->append("max_size");
        tmp_message->writer()->append(max_length);
    }

    tmp_message->end_message();

    return_value = value;
    mode = MODE_READ;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_settings_mgmt::start_write(QString name, QByteArray value)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_SETTINGS, COMMAND_READ_WRITE);
    tmp_message->writer()->append("name");
    tmp_message->writer()->append(name);
    tmp_message->writer()->append("val");
    tmp_message->writer()->append(value);
    tmp_message->end_message();

    mode = MODE_WRITE;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_settings_mgmt::start_delete(QString name)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_SETTINGS, COMMAND_DELETE);
    tmp_message->writer()->append("name");
    tmp_message->writer()->append(name);
    tmp_message->end_message();

    mode = MODE_DELETE;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_settings_mgmt::start_commit(void)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_SETTINGS, COMMAND_COMMIT);
    tmp_message->end_message();

    mode = MODE_COMMIT;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_settings_mgmt::start_load(void)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_SETTINGS, COMMAND_LOAD_SAVE);
    tmp_message->end_message();

    mode = MODE_LOAD;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_settings_mgmt::start_save(void)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_SETTINGS, COMMAND_LOAD_SAVE);
    tmp_message->end_message();

    mode = MODE_SAVE;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

QString smp_group_settings_mgmt::mode_to_string(uint8_t mode)
{
    switch (mode)
    {
    case MODE_IDLE:
        return "Idle";
    case MODE_READ:
        return "Reading";
    case MODE_WRITE:
        return "Writing";
    case MODE_DELETE:
        return "Deleting";
    case MODE_COMMIT:
        return "Committing";
    case MODE_LOAD:
        return "Loading";
    case MODE_SAVE:
        return "Saving";
    default:
        return "Invalid";
    }
}

QString smp_group_settings_mgmt::command_to_string(uint8_t command)
{
    switch (command)
    {
    case COMMAND_READ_WRITE:
        return "Reading/writing";
    case COMMAND_DELETE:
        return "Deleting";
    case COMMAND_COMMIT:
        return "Committing";
    case COMMAND_LOAD_SAVE:
        return "Loading/saving";
    default:
        return "Invalid";
    }
}

bool smp_group_settings_mgmt::error_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_values.length())
    {
        *error = smp_error_values.at(rc);
        return true;
    }

    return false;
}

bool smp_group_settings_mgmt::error_define_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_defines.length())
    {
        *error = smp_error_defines.at(rc);
        return true;
    }

    return false;
}
