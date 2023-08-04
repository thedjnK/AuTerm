/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_img_mgmt.cpp
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
#include "smp_group_img_mgmt.h"
#include <QFile>
#include <QCryptographicHash>

enum modes {
    MODE_IDLE = 0,
    MODE_UPLOAD_FIRMWARE,
    MODE_LIST_IMAGES,
    MODE_SET_IMAGE
};

enum img_mgmt_commands {
    COMMAND_STATE = 0,
    COMMAND_UPLOAD,
    COMMAND_ERASE = 5
};

const uint16_t group_id = 1;

//0x08 = new version, 0x00 = old
#define setup_smp_message(message, stream, write, group, id) \
message.append((char)((smp_version == 1 ? 0x08 : 0x00) | (write == true ? 0x02 : 0x00)));  /* Read | Write (0x00 | 0x02) */ \
    message.append((char)0x00);  /* Flags */ \
    message.append((char)0x00);  /* Length A */ \
    message.append((char)0x05);  /* Length B */ \
    message.append((char)(group >> 8));  /* Group A */ \
    message.append((char)group);  /* Group B */ \
    message.append((char)0x01);  /* Sequence */ \
    message.append((char)id);   /* Message ID */ \
    stream.startMap()

#define finish_smp_message(message, stream, data) \
    stream.endMap(); \
    message[2] = (uint8_t)(smp_data.length() >> 8); \
    message[3] = (uint8_t)smp_data.length(); \
    message.append(data)

smp_group_img_mgmt::smp_group_img_mgmt(smp_processor *parent) : smp_group(parent, group_id)
{
    busy = false;
    mode = 0;
}

bool smp_group_img_mgmt::extract_hash(QByteArray *file_data)
{
    bool found = false;

    int32_t pos = file_data->length() - 4;
    int16_t length;
    while (pos >= 0)
    {
        if (file_data->mid(pos, 2) == image_tlv_magic)
        {
            length = file_data->at(pos + 2);
            length |= ((uint16_t)file_data->at(pos + 3)) << 8;

            if ((pos + length) == file_data->length())
            {
                found = true;
                break;
            }
        }

        --pos;
    }

    if (found == true)
    {
        int32_t new_pos = pos + 4;
        while (new_pos < file_data->length())
        {
            uint8_t type = file_data->at(new_pos);
            int16_t local_length = file_data->at(new_pos + 2);
            local_length |= ((uint16_t)file_data->at(new_pos + 3)) << 8;

            //		    qDebug() << "Type " << type << ", length " << local_length;

            if (type == 0x10 && local_length == 32)
            {
                //We have the hash we wanted
                upload_hash = file_data->mid((new_pos + 4), local_length);
                //todo: check if another hash is present?
                return true;
            }

            new_pos += local_length + 4;
        }
    }

    return false;
}

bool smp_group_img_mgmt::handleStream_upload(QCborStreamReader &reader, int32_t *new_rc, int64_t *new_off)
{
    //    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    int32_t rc = -1;
    int64_t off = -1;

    while (!reader.lastError() && reader.hasNext())
    {
        bool keyset = false;
        //	    qDebug() << "Key: " << key;
        //	    qDebug() << "Type: " << reader.type();
        switch (reader.type())
        {
        case QCborStreamReader::UnsignedInteger:
        case QCborStreamReader::NegativeInteger:
        case QCborStreamReader::SimpleType:
        case QCborStreamReader::Float16:
        case QCborStreamReader::Float:
        case QCborStreamReader::Double:
        {
            //	handleFixedWidth(reader);
            if (key == "rc")
            {
                //			    qDebug() << "found rc";
                rc = reader.toInteger();
            }
            else if (key == "off")
            {
                //			    qDebug() << "found off";
                off = reader.toInteger();
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
        }
        break;
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
            reader.enterContainer();
            while (reader.lastError() == QCborError::NoError && reader.hasNext())
            {
                handleStream_upload(reader, new_rc, new_off);
            }
            if (reader.lastError() == QCborError::NoError)
            {
                reader.leaveContainer();
            }
            break;
        }

        if (keyset == false && !key.isEmpty())
        {
            key = "";
        }
    }

    if (new_rc != NULL && rc != -1)
    {
        *new_rc = rc;
    }

    if (new_off != NULL && off != -1)
    {
        *new_off = off;
    }

    return true;
}

//static QList<image_state_t> blaharray;
static image_state_t thisblah;
static slot_state_t thisblah2;

bool smp_group_img_mgmt::handleStream_state(QCborStreamReader &reader, int32_t *new_rc, QString array_name)
{
    QString array_name_dupe = array_name;
    qDebug() << reader.lastError() << reader.hasNext();

    QString key = "";
    int32_t rc = -1;
    int64_t off = -1;

    thisblah.image = 0;
    thisblah.image_set = false;
    thisblah.slot_list.clear();
    thisblah.item = nullptr;
    thisblah2.slot = 0;
    thisblah2.version.clear();
    thisblah2.hash.clear();
    thisblah2.bootable = false;
    thisblah2.pending = false;
    thisblah2.confirmed = false;
    thisblah2.active = false;
    thisblah2.permanent = false;
    thisblah2.splitstatus = false;
    thisblah2.item = nullptr;
    uint8_t items = 0;

    while (!reader.lastError() && reader.hasNext())
    {
        bool keyset = false;
        //	    qDebug() << "Key: " << key;
        //	    qDebug() << "Type: " << reader.type();
        switch (reader.type())
        {
        case QCborStreamReader::SimpleType:
        {
            bool *index = NULL;
            if (key == "bootable")
            {
                index = &thisblah2.bootable;
            }
            else if (key == "pending")
            {
                index = &thisblah2.pending;
            }
            else if (key == "confirmed")
            {
                index = &thisblah2.confirmed;
            }
            else if (key == "active")
            {
                index = &thisblah2.active;
            }
            else if (key == "permanent")
            {
                index = &thisblah2.permanent;
            }
            else if (key == "splitStatus")
            {
                index = &thisblah2.splitstatus;
            }

            if (index != NULL)
            {
                *index = reader.toBool();
            }

            reader.next();
            break;
        }

        case QCborStreamReader::UnsignedInteger:
        case QCborStreamReader::NegativeInteger:
        case QCborStreamReader::Float16:
        case QCborStreamReader::Float:
        case QCborStreamReader::Double:
        {
            //	handleFixedWidth(reader);
            if (key == "rc")
            {
                //			    qDebug() << "found rc";
                rc = reader.toInteger();
            }
            else if (key == "image")
            {
                thisblah.image = reader.toUnsignedInteger();
                thisblah.image_set = true;
            }
            else if (key == "slot")
            {
                thisblah2.slot = reader.toUnsignedInteger();
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

            if (key == "hash")
            {
                thisblah2.hash = data;
                emit plugin_to_hex(&thisblah2.hash);
                items |= 0x01;
            }
        }
        break;
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
                else if (key == "version")
                {
                    thisblah2.version = data.toUtf8();
                    items |= 0x02;
                }
            }
            break;
        }
        case QCborStreamReader::Array:
        case QCborStreamReader::Map:

            if (reader.type() == QCborStreamReader::Array)
            {
                array_name_dupe = key;
            }
            reader.enterContainer();
            while (reader.lastError() == QCborError::NoError && reader.hasNext())
            {
                qDebug() << "container/map";
                handleStream_state(reader, new_rc, array_name_dupe);
                //			    if (key == "images")
                //			    {
                //				    host_images->append(thisblah);
                //			    }
            }
            if (reader.lastError() == QCborError::NoError)
            {
                qDebug() << "leave";
                reader.leaveContainer();

                if (array_name == "images")
                {
                    image_state_t *image_state_ptr = nullptr;

                    if (host_images->length() > 0)
                    {
                        uint8_t i = 0;
                        while (i < host_images->length())
                        {
                            if (host_images->at(i).image_set == thisblah.image_set && (thisblah.image_set == false || host_images->at(i).image == thisblah.image))
                            {
                                image_state_ptr = &((*host_images)[i]);
                                break;
                            }

                            ++i;
                        }
                    }

                    if (image_state_ptr == nullptr)
                    {
                        if (thisblah.image_set == true)
                        {
                            thisblah.item = new QStandardItem(QString("Image ").append(QString::number(thisblah.image)));
                        }
                        else
                        {
                            thisblah.item = new QStandardItem("Images");
                        }
                        host_images->append(thisblah);
                        image_state_ptr = &host_images->last();
#if 0
                        model_image_state.appendRow(thisblah.item);
#endif
                    }

                    thisblah2.item = new QStandardItem(QString("Slot ").append(QString::number(thisblah2.slot)));
                    image_state_ptr->slot_list.append(thisblah2);
                    image_state_ptr->item->appendRow(thisblah2.item);
                }
            }
            break;
        }

        if (keyset == false && !key.isEmpty())
        {
            key = "";
        }
    }

    if (new_rc != NULL && rc != -1)
    {
        *new_rc = rc;
    }

    return true;
}

void smp_group_img_mgmt::file_upload(QByteArray *message)
{
    int32_t rc = -1;
    int64_t off = -1;
    bool good = true;

    if (message != nullptr)
    {
        QCborStreamReader cbor_reader(*message);
        good = handleStream_upload(cbor_reader, &rc, &off);

    //    qDebug() << "rc = " << rc << ", off = " << off;

        if (off != -1 && rc != 9)
        {
            file_upload_area = off;
        }
        //    qDebug() << "good is " << good;

        if (file_upload_area != 0)
        {
            emit progress(smp_user_data, file_upload_area * 100 / file_upload_data.length());
            //progress_IMG_Complete->setValue(file_upload_area * 100 / file_upload_data.length());
        }
    }

    if (good == true)
    {
        //Upload next chunk
        if (file_upload_area >= file_upload_data.length())
        {
            float blah = file_upload_data.length();
            uint8_t prefix = 0;
            while (blah >= 1024)
            {
                blah /= 1024;
                ++prefix;
            }
            QString bob;
            if (prefix == 0)
            {
                bob = "B";
            }
            else if (prefix == 1)
            {
                bob = "KiB";
            }
            else if (prefix == 2)
            {
                bob = "MiB";
            }
            else if (prefix == 3)
            {
                bob = "GiB";
            }

            blah = file_upload_data.length() / (float)(upload_tmr.elapsed() / 1000);
            prefix = 0;
            while (blah >= 1024)
            {
                blah /= 1024;
                ++prefix;
            }

            if (prefix == 0)
            {
                bob = "B";
            }
            else if (prefix == 1)
            {
                bob = "KiB";
            }
            else if (prefix == 2)
            {
                bob = "MiB";
            }
            else if (prefix == 3)
            {
                bob = "GiB";
            }
//            edit_IMG_Log->appendPlainText(QString("~").append(QString::number(blah)).append(bob).append("ps throughput"));

            file_upload_data.clear();

                mode = MODE_IDLE;
                upload_tmr.invalidate();
                upload_hash.clear();
                file_upload_area = 0;
//                emit plugin_set_status(false, false);
//                lbl_IMG_Status->setText("Finished.");
                emit progress(smp_user_data, 100);
                emit status(smp_user_data, STATUS_COMPLETE, QString("~").append(QString::number(blah)).append(bob).append("ps throughput"));


            return;
        }

        smp_message *tmp_message = new smp_message();

        QByteArray message;
        QByteArray smp_data;
        QCborStreamWriter smp_stream(&smp_data);

        setup_smp_message(message, smp_stream, true, 0x01, 0x01);

#if 0
//0x08 = new version, 0x00 = old
#define setup_smp_message(message, stream, write, group, id) \
message.append((char)((smp_version == 1 ? 0x08 : 0x00) | (write == true ? 0x02 : 0x00)));  /* Read | Write (0x00 | 0x02) */ \
    message.append((char)0x00);  /* Flags */ \
    message.append((char)0x00);  /* Length A */ \
    message.append((char)0x05);  /* Length B */ \
    message.append((char)(group >> 8));  /* Group A */ \
    message.append((char)group);  /* Group B */ \
    message.append((char)0x01);  /* Sequence */ \
    message.append((char)id);   /* Message ID */ \
    stream.startMap()

#define finish_smp_message(message, stream, data) \
    stream.endMap(); \
    message[2] = (uint8_t)(smp_data.length() >> 8); \
    message[3] = (uint8_t)smp_data.length(); \
    message.append(data)
#endif

        if (file_upload_area == 0)
        {
            //Initial packet, extra data is needed: generate upload hash
            QByteArray session_hash = QCryptographicHash::hash(file_upload_data, QCryptographicHash::Sha256);

            smp_stream.append("image");
            smp_stream.append(upload_image);
            smp_stream.append("len");
            smp_stream.append(file_upload_data.length());
            smp_stream.append("sha");
            smp_stream.append(session_hash);
        }

        smp_stream.append("off");
        smp_stream.append(file_upload_area);
        smp_stream.append("data");
        smp_stream.append(file_upload_data.mid(file_upload_area, smp_mtu));

        //	    qDebug() << "off: " << file_upload_area << ", left: " << file_upload_data.length();

        finish_smp_message(message, smp_stream, smp_data);

        //	    qDebug() << "len: " << smp_data.length();

        tmp_message->append(message);
        processor->send(tmp_message, 4000, 3);
    }
    else
    {
        mode = MODE_IDLE;
    }
}

void smp_group_img_mgmt::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        qDebug() << "Unexpected response, not busy";
    }
    else if (group != 1)
    {
        qDebug() << "Unexpected group, not 1";
    }
    else if (mode == MODE_UPLOAD_FIRMWARE && command == COMMAND_UPLOAD)
    {
#if 0
        if (command == 0x00)
        {
            //Response to set image state
            int32_t rc = -1;
            QCborStreamReader cbor_reader(*data);
            bool good = handleStream_state(cbor_reader, &rc, "");
            //		    qDebug() << "Got " << good << ", " << rc;

            //		    edit_IMG_Log->appendPlainText(QString("Finished #2 in ").append(QString::number(upload_tmr.elapsed())).append("ms"));
            lbl_IMG_Status->setText(QString("Finished #2 in ").append(QString::number(upload_tmr.elapsed())).append("ms"));

            file_upload_in_progress = false;
            upload_tmr.invalidate();
            upload_hash.clear();
            file_upload_area = 0;
            emit plugin_set_status(false, false);
        }
#endif
        file_upload(&data);
    }
#if 1
    else if (mode == MODE_SET_IMAGE && command == COMMAND_STATE)
    {
            //Response to set image state
            int32_t rc = -1;
            //            message.remove(0, 8);
            QCborStreamReader cbor_reader(data);
            bool good = handleStream_state(cbor_reader, &rc, "");
            //		    qDebug() << "Got " << good << ", " << rc;

            //		    edit_IMG_Log->appendPlainText(QString("Finished #2 in ").append(QString::number(upload_tmr.elapsed())).append("ms"));
            //lbl_IMG_Status->setText("Finished.");

            //file_list_in_progress = false;
            //emit plugin_set_status(false, false);
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
    }
#endif
    else if (mode == MODE_LIST_IMAGES && command == COMMAND_STATE)
    {
//TODO:
            //Response to set image state
            int32_t rc = -1;
            //            message.remove(0, 8);
            QCborStreamReader cbor_reader(data);
            bool good = handleStream_state(cbor_reader, &rc, "");
            //		    qDebug() << "Got " << good << ", " << rc;

            //		    edit_IMG_Log->appendPlainText(QString("Finished #2 in ").append(QString::number(upload_tmr.elapsed())).append("ms"));
            //lbl_IMG_Status->setText("Finished.");

            //file_list_in_progress = false;
            //emit plugin_set_status(false, false);
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
    }
    else
    {
        qDebug() << "Unsupported command received";
    }
}

void smp_group_img_mgmt::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    qDebug() << "error :(";
}

void smp_group_img_mgmt::timeout(smp_message *message)
{
    qDebug() << "timeout :(";
}

void smp_group_img_mgmt::cancel()
{
    if (busy == true)
    {
        busy = false;
        emit status(smp_user_data, STATUS_CANCELLED, nullptr);
    }
}

bool smp_group_img_mgmt::start_image_get(QList<image_state_t> *images)
{
    host_images = images;
//    colview_IMG_Images->previewWidget()->hide();
//    model_image_state.clear();
    host_images->clear();

    QByteArray message;
    QByteArray smp_data;
    QCborStreamWriter smp_stream(&smp_data);

    setup_smp_message(message, smp_stream, false, 0x01, 0x00);
    finish_smp_message(message, smp_stream, smp_data);

    mode = MODE_LIST_IMAGES;
//    file_list_in_progress = true;

    //	    qDebug() << "len: " << message.length();

    smp_message *tmp_message = new smp_message();
    tmp_message->append(message);
    processor->send(tmp_message, 4000, 3);
    //        uart->send(&message);

    return true;
}

bool smp_group_img_mgmt::start_image_set(QByteArray *hash, bool confirm)
{
    QByteArray message;
    QByteArray smp_data;
    QCborStreamWriter smp_stream(&smp_data);

    setup_smp_message(message, smp_stream, true, 0x01, 0x00);

    smp_stream.append("hash");
    smp_stream.append(*hash);
    if (confirm == true)
    {
        smp_stream.append("confirm");
        smp_stream.append(true);
    }

    finish_smp_message(message, smp_stream, smp_data);
    //			    qDebug() << message;
    //			    qDebug() << "hash is " << upload_hash;

    mode = MODE_SET_IMAGE;
    smp_message *tmp_message = new smp_message();
    tmp_message->append(message);
    processor->send(tmp_message, 4000, 3);

    //uart->send(&message);
//    lbl_IMG_Status->setText(QString("Marking image ").append(radio_IMG_Test->isChecked() ? "for test." : "as confirmed."));


    return true;
}

bool smp_group_img_mgmt::start_firmware_update(uint8_t image, QString filename, bool upgrade, QByteArray *image_hash)
{
    //Upload
    QFile file(filename);

    if (!file.open(QFile::ReadOnly))
    {
//todo: error
        return false;
    }

    file_upload_data.clear();
    file_upload_data.append(file.readAll());

    file.close();

    if (extract_hash(&file_upload_data) == false)
    {
//        lbl_IMG_Status->setText("Hash was not found");
        file_upload_data.clear();
//todo: error
        return false;
    }

    //Send start
    upload_image = image;
    file_upload_area = 0;
    mode = MODE_UPLOAD_FIRMWARE;
    upload_tmr.start();

    file_upload(nullptr);

    if (image_hash != nullptr)
    {
        *image_hash = upload_hash;
    }

    return true;
}

bool smp_group_img_mgmt::start_image_erase(uint8_t slot)
{
    return false;
}
