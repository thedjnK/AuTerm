/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_fs_mgmt.cpp
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
#include "smp_group_fs_mgmt.h"

enum modes : uint8_t {
    MODE_IDLE = 0,
    MODE_UPLOAD,
    MODE_DOWNLOAD,
    MODE_STATUS,
    MODE_HASH_CHECKSUM,
    MODE_SUPPORTED_HASHES_CHECKSUMS,
    MODE_FILE_CLOSE
};

enum fs_mgmt_commands : uint8_t {
    COMMAND_UPLOAD_DOWNLOAD = 0,
    COMMAND_STATUS,
    COMMAND_HASH_CHECKSUM,
    COMMAND_SUPPORTED_HASHES_CHECKSUMS,
    COMMAND_FILE_CLOSE
};

enum fs_mgmt_errs : uint16_t {
    FS_MGMT_ERR_OK = 0,
    FS_MGMT_ERR_UNKNOWN,
    FS_MGMT_ERR_FILE_INVALID_NAME,
    FS_MGMT_ERR_FILE_NOT_FOUND,
    FS_MGMT_ERR_FILE_IS_DIRECTORY,
    FS_MGMT_ERR_FILE_OPEN_FAILED,
    FS_MGMT_ERR_FILE_SEEK_FAILED,
    FS_MGMT_ERR_FILE_READ_FAILED,
    FS_MGMT_ERR_FILE_TRUNCATE_FAILED,
    FS_MGMT_ERR_FILE_DELETE_FAILED,
    FS_MGMT_ERR_FILE_WRITE_FAILED,
    FS_MGMT_ERR_FILE_OFFSET_NOT_VALID,
    FS_MGMT_ERR_FILE_OFFSET_LARGER_THAN_FILE,
    FS_MGMT_ERR_CHECKSUM_HASH_NOT_FOUND
};

static QStringList smp_error_defines = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "FILE_INVALID_NAME" <<
    "FILE_NOT_FOUND" <<
    "FILE_IS_DIRECTORY" <<
    "FILE_OPEN_FAILED" <<
    "FILE_SEEK_FAILED" <<
    "FILE_READ_FAILED" <<
    "FILE_TRUNCATE_FAILED" <<
    "FILE_DELETE_FAILED" <<
    "FILE_WRITE_FAILED" <<
    "FILE_OFFSET_NOT_VALID" <<
    "FILE_OFFSET_LARGER_THAN_FILE" <<
    "CHECKSUM_HASH_NOT_FOUND";

static QStringList smp_error_values = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "The specified file name is not valid" <<
    "The specified file does not exist" <<
    "The specified file is a directory, not a file" <<
    "Error occurred whilst attempting to open a file" <<
    "Error occurred whilst attempting to seek to an offset in a file" <<
    "Error occurred whilst attempting to read data from a file" <<
    "Error occurred whilst trying to truncate file" <<
    "Error occurred whilst trying to delete file" <<
    "Error occurred whilst attempting to write data to a file" <<
    "Specified data offset is not valid" <<
    "The requested offset is larger than the size of the file on the device" <<
    "The requested checksum or hash type was not found or is not supported by this build";

smp_group_fs_mgmt::smp_group_fs_mgmt(smp_processor *parent) : smp_group(parent, "FS", SMP_GROUP_ID_FS, error_lookup, error_define_lookup)
{
    mode = MODE_IDLE;
}

bool smp_group_fs_mgmt::parse_upload_response(QCborStreamReader &reader, uint32_t *off, bool *off_found)
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
            case QCborStreamReader::UnsignedInteger:
            {
                if (key == "off")
                {
                    *off = (uint32_t)reader.toUnsignedInteger();
                    *off_found = true;
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
                    parse_upload_response(reader, off, off_found);
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

bool smp_group_fs_mgmt::parse_download_response(QCborStreamReader &reader, uint32_t *off, uint32_t *len, QByteArray *file_data)
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
            case QCborStreamReader::UnsignedInteger:
            {
                if (key == "off")
                {
                    *off = (uint32_t)reader.toUnsignedInteger();
                }
                else if (key == "len")
                {
                    *len = (uint32_t)reader.toUnsignedInteger();
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
                    qDebug("Error decoding byte array");
                }
                else
                {
                    if (key == "data")
                    {
                        *file_data = data;
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
                    parse_download_response(reader, off, len, file_data);
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

bool smp_group_fs_mgmt::parse_status_response(QCborStreamReader &reader, uint32_t *len)
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
            case QCborStreamReader::UnsignedInteger:
            {
                    if (key == "len")
                    {
                        *len = (uint32_t)reader.toUnsignedInteger();
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
                        parse_status_response(reader, len);
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

bool smp_group_fs_mgmt::parse_hash_checksum_response(QCborStreamReader &reader, QString *type, QByteArray *hash_checksum, uint32_t *file_size)
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
            case QCborStreamReader::UnsignedInteger:
            {
                    if (key == "output")
                    {
                        uint32_t tmp_hash_checksum = (uint32_t)reader.toUnsignedInteger();

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
                        //Flip endian for little endian systems to allow for inserting bytes in correct order
                        flip_endian((uint8_t *)&tmp_hash_checksum, sizeof(uint32_t));
#endif

                        hash_checksum->append((char *)&tmp_hash_checksum, sizeof(uint32_t));
                    }
                    else if (key == "len")
                    {
                        *file_size = (uint32_t)reader.toUnsignedInteger();
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
                        qDebug("Error decoding byte array");
                    }
                    else
                    {
                        if (key == "output")
                        {
                            *hash_checksum = data;
                        }
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
                        else if (key == "type")
                        {
                            *type = data;
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
                        parse_hash_checksum_response(reader, type, hash_checksum, file_size);
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

bool smp_group_fs_mgmt::parse_supported_hashes_checksums_response(QCborStreamReader &reader, bool in_data, QString *key_name, hash_checksum_t *current_item)
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
            case QCborStreamReader::UnsignedInteger:
            {
                if (key == "format")
                {
                    current_item->format = (uint8_t)reader.toUnsignedInteger();
                }
                else if (key == "size")
                {
                    current_item->size = (uint16_t)reader.toUnsignedInteger();
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

                        if (in_data == false && key == "types")
                        {
                            in_data = true;
                        }
                        else if (in_data == true && current_item->name.isEmpty())
                        {
                            current_item->name = data;
                        }
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
                    parse_supported_hashes_checksums_response(reader, in_data, &key, current_item);
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

    if (in_data == true && !current_item->name.isEmpty())
    {
        hash_checksum_object->append(*current_item);
        current_item->name.clear();
    }

    return true;
}

void smp_group_fs_mgmt::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    Q_UNUSED(op);

    //    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        qDebug() << "Unexpected response, not busy";
        emit status(smp_user_data, STATUS_ERROR, "Unexpected response, shell mgmt not busy");
    }
    else if (group != SMP_GROUP_ID_FS)
    {
        qDebug() << "Unexpected group " << group << ", not " << SMP_GROUP_ID_FS;
        emit status(smp_user_data, STATUS_ERROR, "Unexpected group, not fs mgmt");
    }
    else
    {
//        uint8_t finished_mode = mode;
//        mode = MODE_IDLE;

        if (version != smp_version)
        {
            //The target device does not support the SMP version being used, adjust for duration of transfer and raise a warning to the parent
            smp_version = version;
            emit version_error(version);
        }

        if (mode == MODE_UPLOAD && command == COMMAND_UPLOAD_DOWNLOAD)
        {
            //Response to upload
            bool off_found = false;
            QCborStreamReader cbor_reader(data);
            bool good = parse_upload_response(cbor_reader, &file_upload_area, &off_found);

            //todo
            if (off_found == true)
            {
                if (file_upload_area < local_file_size)
                {
                    //Upload next chunk
                    upload_chunk();
                    emit progress(smp_user_data, file_upload_area * 100 / local_file_size);
                }
                else
                {
                    //Upload complete
                    mode = MODE_IDLE;
                    local_file.close();
                    local_file_size = 0;
                    file_upload_area = 0;
                    //todo:
                    upload_tmr.invalidate();
                    device_file_name.clear();

                    emit progress(smp_user_data, 100);
                    emit status(smp_user_data, STATUS_COMPLETE, "Upload complete");
                }
            }
            else
            {
                mode = MODE_IDLE;
                local_file.close();
                local_file_size = 0;
                file_upload_area = 0;
                upload_tmr.invalidate();
                device_file_name.clear();

                emit status(smp_user_data, STATUS_ERROR, "Missing off parameter");
            }
        }
        else if (mode == MODE_DOWNLOAD && command == COMMAND_UPLOAD_DOWNLOAD)
        {
            //Response to download
            uint32_t off;
            uint32_t len;
            QByteArray file_data;
            QCborStreamReader cbor_reader(data);
            bool good = parse_download_response(cbor_reader, &off, &len, &file_data);

            if (len > 0)
            {
                local_file_size = len;
            }

            if (file_upload_area != off)
            {
                qDebug() << "Error: mismatch!";
            }

            if (file_data.isEmpty() == false)
            {
                local_file.write(file_data);
            }

            file_upload_area += file_data.length();

            if (file_upload_area < local_file_size)
            {
                //Download next chunk
                download_chunk();

                emit progress(smp_user_data, file_upload_area * 100 / local_file_size);
            }
            else
            {
                //Download complete
                mode = MODE_IDLE;
                local_file.close();
                local_file_size = 0;
                file_upload_area = 0;
                //todo:
                upload_tmr.invalidate();
                device_file_name.clear();

                emit progress(smp_user_data, 100);
                emit status(smp_user_data, STATUS_COMPLETE, "Download complete");
            }
        }
        else if (mode == MODE_STATUS && command == COMMAND_STATUS)
        {
            QCborStreamReader cbor_reader(data);
            bool good = parse_status_response(cbor_reader, file_size_object);
            mode = MODE_IDLE;

            qDebug() << "status done";
            qDebug() << "Len: " << *file_size_object;

            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (mode == MODE_HASH_CHECKSUM && command == COMMAND_HASH_CHECKSUM)
        {
            QString type;

            QCborStreamReader cbor_reader(data);
            bool good = parse_hash_checksum_response(cbor_reader, &type, hash_checksum_result_object, file_size_object);
            mode = MODE_IDLE;

            emit status(smp_user_data, STATUS_COMPLETE, type);
        }
        else if (mode == MODE_SUPPORTED_HASHES_CHECKSUMS && command == COMMAND_SUPPORTED_HASHES_CHECKSUMS)
        {
            hash_checksum_t temp_item;

            QCborStreamReader cbor_reader(data);
            hash_checksum_object->clear();
            bool good = parse_supported_hashes_checksums_response(cbor_reader, false, nullptr, &temp_item);
            mode = MODE_IDLE;

            qDebug() << "supported hash/checksum done";
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else if (mode == MODE_FILE_CLOSE && command == COMMAND_FILE_CLOSE)
        {
            mode = MODE_IDLE;
            qDebug() << "file close done";
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else
        {
            qDebug() << "Unsupported command received";
        }
    }
}

void smp_group_fs_mgmt::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    Q_UNUSED(version);
    Q_UNUSED(op);
    Q_UNUSED(group);
    Q_UNUSED(error);

    bool cleanup = true;
    qDebug() << "error :(";

    if (command == COMMAND_UPLOAD_DOWNLOAD && mode == MODE_UPLOAD)
    {
        //TODO
        if (error.type == SMP_ERROR_RET && error.group == SMP_GROUP_ID_FS && error.rc == FS_MGMT_ERR_FILE_OFFSET_NOT_VALID)
        {
            //TODO - Possible indication that another transport (or the device itself) has modified the underlying file
            qDebug() << "Possible MCUmgr FS upload transport clash";
        }

        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_UPLOAD_DOWNLOAD && mode == MODE_DOWNLOAD)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_STATUS && mode == MODE_STATUS)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_HASH_CHECKSUM && mode == MODE_HASH_CHECKSUM)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_SUPPORTED_HASHES_CHECKSUMS && mode == MODE_SUPPORTED_HASHES_CHECKSUMS)
    {
        //TODO
        emit status(smp_user_data, STATUS_ERROR, smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_FILE_CLOSE && mode == MODE_FILE_CLOSE)
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

void smp_group_fs_mgmt::timeout(smp_message *message)
{
    qDebug() << "timeout :(";

    //TODO:
    emit status(smp_user_data, STATUS_TIMEOUT, QString("Timeout (Mode: %1)").arg(mode_to_string(mode)));

    mode = MODE_IDLE;
}

void smp_group_fs_mgmt::cancel()
{
    if (mode != MODE_IDLE)
    {
        mode = MODE_IDLE;

        emit status(smp_user_data, STATUS_CANCELLED, nullptr);
    }
}

void smp_group_fs_mgmt::upload_chunk()
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_FS, COMMAND_UPLOAD_DOWNLOAD);

    if (local_file.pos() != file_upload_area)
    {
        local_file.seek(file_upload_area);
    }

    if (file_upload_area == 0)
    {
        tmp_message->writer()->append("len");
        tmp_message->writer()->append(local_file_size);
    }

    //TODO: Deal with size
    tmp_message->writer()->append("name");
    tmp_message->writer()->append(device_file_name);
    tmp_message->writer()->append("off");
    tmp_message->writer()->append(file_upload_area);
    tmp_message->writer()->append("data");
    tmp_message->writer()->append(local_file.read(64));
    tmp_message->end_message();

    //TODO: Not always true
    file_upload_area += 64;

    processor->send(tmp_message, smp_timeout, smp_retries, true);
}

void smp_group_fs_mgmt::download_chunk()
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_FS, COMMAND_UPLOAD_DOWNLOAD);

/*    if (local_file.pos() != file_upload_area)
    {
        local_file.seek(file_upload_area);
    }*/

    //TODO: Deal with size
    tmp_message->writer()->append("name");
    tmp_message->writer()->append(device_file_name);
    tmp_message->writer()->append("off");
    tmp_message->writer()->append(file_upload_area);
    tmp_message->end_message();

    processor->send(tmp_message, smp_timeout, smp_retries, true);
}

bool smp_group_fs_mgmt::start_upload(QString file_name, QString destination_name)
{
    local_file.setFileName(file_name);

    if (!local_file.open(QFile::ReadOnly))
    {
        emit status(smp_user_data, STATUS_ERROR, "File could not be opened in read mode");
        return false;
    }

    mode = MODE_UPLOAD;
    device_file_name = destination_name;
    local_file_size = local_file.size();
    file_upload_area = 0;
    upload_tmr.start();

    //	    qDebug() << "len: " << message.length();

    upload_chunk();

    return true;
}

bool smp_group_fs_mgmt::start_download(QString file_name, QString destination_name)
{
    local_file.setFileName(destination_name);

    if (!local_file.open(QFile::WriteOnly | QFile::Truncate))
    {
        emit status(smp_user_data, STATUS_ERROR, "File could not be opened in write mode");
        return false;
    }

    mode = MODE_DOWNLOAD;
    device_file_name = file_name;
    file_upload_area = 0;
    upload_tmr.start();

    //	    qDebug() << "len: " << message.length();

    download_chunk();

    return true;
}

//TODO
bool smp_group_fs_mgmt::start_status(QString file_name, uint32_t *file_size)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_FS, COMMAND_STATUS);
    tmp_message->writer()->append("name");
    tmp_message->writer()->append(file_name);
    tmp_message->end_message();

    mode = MODE_STATUS;
    file_size_object = file_size;
    *file_size = 0;

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_fs_mgmt::start_hash_checksum(QString file_name, QString hash_checksum, QByteArray *result, uint32_t *file_size)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_FS, COMMAND_HASH_CHECKSUM);
    tmp_message->writer()->append("name");
    tmp_message->writer()->append(file_name);
    tmp_message->writer()->append("type");
    tmp_message->writer()->append(hash_checksum);;
    tmp_message->end_message();

    mode = MODE_HASH_CHECKSUM;
    hash_checksum_result_object = result;
    result->clear();
    file_size_object = file_size;
    *file_size = 0;

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_fs_mgmt::start_supported_hashes_checksums(QList<hash_checksum_t> *hash_checksum_list)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_FS, COMMAND_SUPPORTED_HASHES_CHECKSUMS);
    tmp_message->end_message();

    mode = MODE_SUPPORTED_HASHES_CHECKSUMS;
    hash_checksum_object = hash_checksum_list;

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_fs_mgmt::start_file_close()
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_FS, COMMAND_FILE_CLOSE);
    tmp_message->end_message();

    mode = MODE_FILE_CLOSE;

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

QString smp_group_fs_mgmt::mode_to_string(uint8_t mode)
{
    switch (mode)
    {
    case MODE_IDLE:
        return "Idle";
    case MODE_UPLOAD:
        return "Uploading";
    case MODE_DOWNLOAD:
        return "Downloading";
    case MODE_STATUS:
        return "File status";
    case MODE_HASH_CHECKSUM:
        return "Hash/checksum";
    case MODE_SUPPORTED_HASHES_CHECKSUMS:
        return "Supported hashes/checksums";
    case MODE_FILE_CLOSE:
        return "Closing file";
    default:
        return "Invalid";
    }
}

QString smp_group_fs_mgmt::command_to_string(uint8_t command)
{
    switch (command)
    {
    case COMMAND_UPLOAD_DOWNLOAD:
        return "Uploading/downloading";
    case COMMAND_STATUS:
        return "File status";
    case COMMAND_HASH_CHECKSUM:
        return "Hash/checksum";
    case COMMAND_SUPPORTED_HASHES_CHECKSUMS:
        return "Supported hashes/checksums";
    case COMMAND_FILE_CLOSE:
        return "Closing file";
    default:
        return "Invalid";
    }
}

bool smp_group_fs_mgmt::error_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_values.length())
    {
        *error = smp_error_values.at(rc);
        return true;
    }

    return false;
}

bool smp_group_fs_mgmt::error_define_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_defines.length())
    {
        *error = smp_error_defines.at(rc);
        return true;
    }

    return false;
}

void smp_group_fs_mgmt::flip_endian(uint8_t *data, uint8_t size)
{
    uint8_t i = 0;

    while (i < (size / 2))
    {
        uint8_t temp = data[(size - 1) - i];

        data[(size - 1) - i] = data[i];
        data[i] = temp;

        ++i;
    }
}
