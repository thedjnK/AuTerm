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
#include "smp_message.h"

enum modes : uint8_t {
    MODE_IDLE = 0,
    MODE_UPLOAD_FIRMWARE,
    MODE_LIST_IMAGES,
    MODE_SET_IMAGE,
    MODE_ERASE_IMAGE,
    MODE_SLOT_INFO
};

enum img_mgmt_commands : uint8_t {
    COMMAND_STATE = 0,
    COMMAND_UPLOAD,
    COMMAND_ERASE = 5,
    COMMAND_SLOT_INFO
};

//MCUboot TLV (Tag-Length-Value) related constants
static const QByteArray image_tlv_magic = QByteArrayLiteral("\x07\x69");
static const QByteArray image_tlv_magic_reverse_endian = QByteArrayLiteral("\x69\x07");
static const uint16_t image_tlv_tag_sha256 = 0x10;
static const uint16_t image_tlv_tag_sha384 = 0x11;
static const uint16_t image_tlv_tag_sha512 = 0x12;
static const uint8_t sha256_size = 32;
static const uint8_t sha384_size = 48;
static const uint8_t sha512_size = 64;
static const uint8_t image_tlv_magic_size = 2;
static const uint8_t image_tlv_legnth_offset_1 = 2;
static const uint8_t image_tlv_legnth_offset_2 = 3;
static const uint8_t image_tlv_header_size = 4;
static const uint8_t image_tlv_data_header_size = 4;

//MCUboot header constants
static const uint8_t ih_magic_none[] = { 0xff, 0xff, 0xff, 0xff };
static const uint8_t ih_magic_v1[] = { 0x96, 0xf3, 0xb8, 0x3c };
static const uint8_t ih_magic_v2[] = { 0x96, 0xf3, 0xb8, 0x3d };
static const uint8_t ih_hdr_size_offs = 8;
static const uint8_t ih_protected_tlv_size_offs = 10;
static const uint8_t ih_img_size_offs = 12;

static QStringList smp_error_defines = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "FLASH_CONFIG_QUERY_FAIL" <<
    "NO_IMAGE" <<
    "NO_TLVS" <<
    "INVALID_TLV" <<
    "TLV_MULTIPLE_HASHES_FOUND" <<
    "TLV_INVALID_SIZE" <<
    "HASH_NOT_FOUND" <<
    "NO_FREE_SLOT" <<
    "FLASH_OPEN_FAILED" <<
    "FLASH_READ_FAILED" <<
    "FLASH_WRITE_FAILED" <<
    "FLASH_ERASE_FAILED" <<
    "INVALID_SLOT" <<
    "NO_FREE_MEMORY" <<
    "FLASH_CONTEXT_ALREADY_SET" <<
    "FLASH_CONTEXT_NOT_SET" <<
    "FLASH_AREA_DEVICE_NULL" <<
    "INVALID_PAGE_OFFSET" <<
    "INVALID_OFFSET" <<
    "INVALID_LENGTH" <<
    "INVALID_IMAGE_HEADER" <<
    "INVALID_IMAGE_HEADER_MAGIC" <<
    "INVALID_HASH" <<
    "INVALID_FLASH_ADDRESS" <<
    "VERSION_GET_FAILED" <<
    "CURRENT_VERSION_IS_NEWER" <<
    "IMAGE_ALREADY_PENDING" <<
    "INVALID_IMAGE_VECTOR_TABLE" <<
    "INVALID_IMAGE_TOO_LARGE" <<
    "INVALID_IMAGE_DATA_OVERRUN" <<
    "IMAGE_CONFIRMATION_DENIED" <<
    "IMAGE_SETTING_TEST_TO_ACTIVE_DENIED";

static QStringList smp_error_values = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "Failed to query flash area configuration" <<
    "There is no image in the slot" <<
    "The image in the slot has no TLVs (tag, length, value)" <<
    "The image in the slot has an invalid TLV type and/or length" <<
    "The image in the slot has multiple hash TLVs, which is invalid" <<
    "The image in the slot has an invalid TLV size" <<
    "The image in the slot does not have a hash TLV, which is required"  <<
    "There is no free slot to place the image" <<
    "Flash area opening failed" <<
    "Flash area reading failed" <<
    "Flash area writing failed" <<
    "Flash area erase failed" <<
    "The provided slot is not valid" <<
    "Insufficient heap memory (malloc failed)" <<
    "The flash context is already set" <<
    "The flash context is not set" <<
    "The device for the flash area is NULL" <<
    "The offset for a page number is invalid" <<
    "The offset parameter was not provided and is required" <<
    "The length parameter was not provided and is required" <<
    "The image length is smaller than the size of an image header" <<
    "The image header magic value does not match the expected value" <<
    "The hash parameter provided is not valid" <<
    "The image load address does not match the address of the flash area" <<
    "Failed to get version of currently running application" <<
    "The currently running application is newer than the version being uploaded" <<
    "There is already an image operating pending" <<
    "The image vector table is invalid" <<
    "The image it too large to fit" <<
    "The amount of data sent is larger than the provided image size" <<
    "Confirmation of image has been denied" <<
    "Setting test to active slot is not allowed";

image_state_t image_state_buffer;
slot_state_t slot_state_buffer;

smp_group_img_mgmt::smp_group_img_mgmt(smp_processor *parent) : smp_group(parent, "IMG", SMP_GROUP_ID_IMG, error_lookup, error_define_lookup)
{
    mode = MODE_IDLE;
}

bool smp_group_img_mgmt::extract_header(QByteArray *file_data, image_endian_t *endian)
{
    uint8_t mcuboot_magic[sizeof(ih_magic_none)];

    if (file_data->length() < (int)sizeof(mcuboot_magic))
    {
        return false;
    }

    memcpy(mcuboot_magic, file_data->data(), sizeof(mcuboot_magic));

    if (memcmp(mcuboot_magic, ih_magic_v2, sizeof(mcuboot_magic)) == 0 || memcmp(mcuboot_magic, ih_magic_v1, sizeof(mcuboot_magic)) == 0)
    {
        log_debug() << "Image is big endian";
        *endian = ENDIAN_BIG;
    }
    else if (memcmp(mcuboot_magic, ih_magic_none, sizeof(mcuboot_magic)) == 0)
    {
        //As header is empty, it is not possible to know if this is big or little endian
        log_debug() << "Image endianess is not known";
        *endian = ENDIAN_UNKNOWN;
    }
    else
    {
        uint8_t reversed_mcuboot_magic[sizeof(ih_magic_none)] = { mcuboot_magic[3], mcuboot_magic[2], mcuboot_magic[1], mcuboot_magic[0] };

        if (memcmp(reversed_mcuboot_magic, ih_magic_v2, sizeof(mcuboot_magic)) == 0 || memcmp(reversed_mcuboot_magic, ih_magic_v1, sizeof(mcuboot_magic)) == 0)
        {
            log_debug() << "Image is little endian";
            *endian = ENDIAN_LITTLE;
        }
        else
        {
            log_debug() << "Image does not have MCUboot header";
            return false;
        }
    }

    return true;
}

bool smp_group_img_mgmt::extract_hash(QByteArray *file_data, QByteArray *hash)
{
    bool found = false;
    bool hash_found = false;

    uint16_t hdr_size;
    uint16_t protected_tlv_size;
    uint32_t img_size;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    if (upload_endian != ENDIAN_BIG)
#else
    if (upload_endian != ENDIAN_LITTLE)
#endif
    {
        hdr_size = (uint8_t)file_data->at(ih_hdr_size_offs);
        hdr_size |= (uint16_t)((uint8_t)file_data->at(ih_hdr_size_offs + 1)) << 8;

        protected_tlv_size = (uint8_t)file_data->at(ih_protected_tlv_size_offs);
        protected_tlv_size |= (uint16_t)((uint8_t)file_data->at(ih_protected_tlv_size_offs + 1)) << 8;

        img_size = (uint8_t)file_data->at(ih_img_size_offs);
        img_size |= (uint32_t)((uint8_t)file_data->at(ih_img_size_offs + 1)) << 8;
        img_size |= (uint32_t)((uint8_t)file_data->at(ih_img_size_offs + 2)) << 16;
        img_size |= (uint32_t)((uint8_t)file_data->at(ih_img_size_offs + 3)) << 24;
    }
    else
    {
        hdr_size = (uint8_t)file_data->at(ih_hdr_size_offs + 1);
        hdr_size |= (uint16_t)((uint8_t)file_data->at(ih_hdr_size_offs)) << 8;

        protected_tlv_size = (uint8_t)file_data->at(ih_protected_tlv_size_offs + 1);
        protected_tlv_size |= (uint16_t)((uint8_t)file_data->at(ih_protected_tlv_size_offs)) << 8;

        img_size = (uint8_t)file_data->at(ih_img_size_offs + 3);
        img_size |= (uint32_t)((uint8_t)file_data->at(ih_img_size_offs + 2)) << 8;
        img_size |= (uint32_t)((uint8_t)file_data->at(ih_img_size_offs + 1)) << 16;
        img_size |= (uint32_t)((uint8_t)file_data->at(ih_img_size_offs)) << 24;
    }

    if (img_size >= INT32_MAX)
    {
        log_error() << "Decoded image size is not valid: " << img_size;
        return false;
    }

    int32_t pos = hdr_size + protected_tlv_size + img_size;
    uint16_t tlv_area_length;

    while ((pos + image_tlv_magic_size) < file_data->length())
    {
        if ((upload_endian != ENDIAN_BIG && file_data->mid(pos, image_tlv_magic_size) == image_tlv_magic) || (upload_endian == ENDIAN_BIG && file_data->mid(pos, image_tlv_magic_size) == image_tlv_magic_reverse_endian))
        {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            if (upload_endian != ENDIAN_BIG)
#else
            if (upload_endian != ENDIAN_LITTLE)
#endif
            {
                tlv_area_length = (uint8_t)file_data->at(pos + image_tlv_legnth_offset_1);
                tlv_area_length |= (uint16_t)((uint8_t)file_data->at(pos + image_tlv_legnth_offset_2)) << 8;
            }
            else
            {
                tlv_area_length = (uint8_t)file_data->at(pos + image_tlv_legnth_offset_2);
                tlv_area_length |= (uint16_t)((uint8_t)file_data->at(pos + image_tlv_legnth_offset_1)) << 8;
            }

            found = true;
            // qDebug() << "Found magic tlv, size:" << tlv_area_length;
            break;
        }

        ++pos;
    }

    if (found == true)
    {
        int32_t new_pos = pos + image_tlv_header_size;

        while (new_pos < pos + tlv_area_length)
        {
            //TODO: TLVs are > 8-bit
            uint8_t type = file_data->at(new_pos);
            uint16_t local_length;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            if (upload_endian != ENDIAN_BIG)
#else
            if (upload_endian != ENDIAN_LITTLE)
#endif
            {
                local_length = (uint8_t)file_data->at(new_pos + image_tlv_legnth_offset_1);
                local_length |= (uint16_t)((uint8_t)file_data->at(new_pos + image_tlv_legnth_offset_2)) << 8;
            }
            else
            {
                local_length = (uint8_t)file_data->at(new_pos + image_tlv_legnth_offset_2);
                local_length |= (uint16_t)((uint8_t)file_data->at(new_pos + image_tlv_legnth_offset_1)) << 8;
            }

            //		    qDebug() << "Type " << type << ", length " << local_length;

            if (type == image_tlv_tag_sha256 || type == image_tlv_tag_sha384 || type == image_tlv_tag_sha512)
            {
                uint8_t hash_size = (type == image_tlv_tag_sha256 ? sha256_size : (type == image_tlv_tag_sha384 ? sha384_size : sha512_size));

                if (hash_found == true)
                {
                    //Duplicate hash has been found
                    log_error() << "Duplicate hash found";
                    return false;
                }

                if (local_length == hash_size)
                {
                    //We have the hash we wanted
                    *hash = file_data->mid((new_pos + 4), local_length);
                    hash_found = true;
                }
                else
                {
                    //Invalid length hash found
                    log_error() << "Invalid length hash found";
                }
            }

            new_pos += local_length + 4;
        }
    }

    return hash_found;
}

bool smp_group_img_mgmt::parse_upload_response(QCborStreamReader &reader, int64_t *new_off, img_mgmt_upload_match *match)
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
                if (key == "match")
                {
                    bool match_value = reader.toBool();

                    if (match_value == true)
                    {
                        *match = MATCH_FAILED;
                    }
                    else
                    {
                        *match = MATCH_PASSED;
                    }
                }

                reader.next();
                break;
            }

            case QCborStreamReader::UnsignedInteger:
            case QCborStreamReader::NegativeInteger:
            {
                if (key == "off")
                {
                    //			    qDebug() << "found off";
                    *new_off = reader.toInteger();
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
                    parse_upload_response(reader, new_off, match);
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

bool smp_group_img_mgmt::parse_state_response(QCborStreamReader &reader, QString array_name)
{
    QString array_name_dupe = array_name;
    QString key = "";
    bool keyset = true;

    image_state_buffer.image = 0;
    image_state_buffer.image_set = false;
    image_state_buffer.slot_list.clear();
    image_state_buffer.item = nullptr;
    slot_state_buffer.slot = 0;
    slot_state_buffer.version.clear();
    slot_state_buffer.hash.clear();
    slot_state_buffer.bootable = false;
    slot_state_buffer.pending = false;
    slot_state_buffer.confirmed = false;
    slot_state_buffer.active = false;
    slot_state_buffer.permanent = false;
    slot_state_buffer.splitstatus = false;
    slot_state_buffer.item = nullptr;

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
                bool *index = NULL;
                if (key == "bootable")
                {
                    index = &slot_state_buffer.bootable;
                }
                else if (key == "pending")
                {
                    index = &slot_state_buffer.pending;
                }
                else if (key == "confirmed")
                {
                    index = &slot_state_buffer.confirmed;
                }
                else if (key == "active")
                {
                    index = &slot_state_buffer.active;
                }
                else if (key == "permanent")
                {
                    index = &slot_state_buffer.permanent;
                }
                else if (key == "splitStatus")
                {
                    index = &slot_state_buffer.splitstatus;
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
            {
                //	handleFixedWidth(reader);
                if (key == "image")
                {
                    image_state_buffer.image = reader.toUnsignedInteger();
                    image_state_buffer.image_set = true;
                }
                else if (key == "slot")
                {
                    slot_state_buffer.slot = reader.toUnsignedInteger();
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
                    slot_state_buffer.hash = data;
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
                    log_error() << "Error decoding string";
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
                        slot_state_buffer.version = data.toUtf8();
                    }
                }

                break;
            }

            case QCborStreamReader::Array:
            case QCborStreamReader::Map:
            {
                if (reader.type() == QCborStreamReader::Array)
                {
                    array_name_dupe = key;
                }

                reader.enterContainer();

                while (reader.lastError() == QCborError::NoError && reader.hasNext())
                {
                    log_debug() << "container/map";
                    parse_state_response(reader, array_name_dupe);
                }

                if (reader.lastError() == QCborError::NoError)
                {
                    log_debug() << "leave";
                    reader.leaveContainer();

                    if (array_name == "images")
                    {
                        image_state_t *image_state_ptr = nullptr;

                        if (host_images != nullptr && host_images->length() > 0)
                        {
                            uint8_t i = 0;
                            while (i < host_images->length())
                            {
                                if (host_images->at(i).image_set == image_state_buffer.image_set && (image_state_buffer.image_set == false || host_images->at(i).image == image_state_buffer.image))
                                {
                                    image_state_ptr = &((*host_images)[i]);
                                    break;
                                }

                                ++i;
                            }
                        }

                        if (host_images != nullptr && image_state_ptr == nullptr)
                        {
                            if (image_state_buffer.image_set == true)
                            {
                                image_state_buffer.item = new QStandardItem(QString("Image ").append(QString::number(image_state_buffer.image)));
                            }
                            else
                            {
                                image_state_buffer.item = new QStandardItem("Images");
                            }

                            host_images->append(image_state_buffer);
                            image_state_ptr = &host_images->last();
                        }

                        if (image_state_ptr != nullptr)
                        {
                            slot_state_buffer.item = new QStandardItem(QString("Slot ").append(QString::number(slot_state_buffer.slot)));
                            image_state_ptr->slot_list.append(slot_state_buffer);
                            image_state_ptr->item->appendRow(slot_state_buffer.item);
                        }
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

    return (reader.lastError() ? false : true);
}

bool smp_group_img_mgmt::parse_slot_info_response(QCborStreamReader &reader, QList<slot_info_t> *images, struct slot_info_t *image_data, struct slot_info_slots_t *slot_data)
{
    QString key = "";
    bool keyset = true;
    bool entered_map = false;

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
            uint32_t *index = NULL;
            bool *variable_found = NULL;

            if (slot_data != nullptr)
            {
                if (key == "slot")
                {
                    index = &slot_data->slot;
                }
                else if (key == "size")
                {
                    index = &slot_data->size;
                    variable_found = &slot_data->size_present;
                }
                else if (key == "upload_image_id")
                {
                    index = &slot_data->upload_image_id;
                    variable_found = &slot_data->upload_image_id_present;
                }
            }
            else if (image_data != nullptr)
            {
                if (key == "image")
                {
                    index = &image_data->image;
                }
                else if (key == "max_image_size")
                {
                    index = &image_data->max_image_size;
                    variable_found = &image_data->max_image_size_present;
                }
            }

            if (index != NULL)
            {
                *index = reader.toUnsignedInteger();

                if (variable_found != NULL)
                {
                    *variable_found = true;
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
            }

            break;
        }

        case QCborStreamReader::Map:
        {
            if (slot_data == nullptr && image_data == nullptr && entered_map == false)
            {
                reader.enterContainer();
                entered_map = true;
            }
            else
            {
                reader.next();
            }

            break;
        }

        case QCborStreamReader::Array:
        {
            if (slot_data == nullptr || image_data == nullptr)
            {
                reader.enterContainer();

                while (reader.lastError() == QCborError::NoError && reader.hasNext() && reader.type() == QCborStreamReader::Map)
                {
                    if (image_data == nullptr && key == "images")
                    {
                        //Parsing images
                        struct slot_info_t current_image_data;

                        current_image_data.image = 0;
                        current_image_data.max_image_size = 0;
                        current_image_data.max_image_size_present = false;
                        current_image_data.slot_data.clear();

                        reader.enterContainer();
                        //log_debug() << "container/map";
                        parse_slot_info_response(reader, NULL, &current_image_data, NULL);
                        reader.leaveContainer();

                        images->append(current_image_data);
                    }
                    else if (slot_data == nullptr && key == "slots")
                    {
                        //Parsing slots
                        struct slot_info_slots_t current_slot_data;

                        current_slot_data.slot = 0;
                        current_slot_data.size = 0;
                        current_slot_data.size_present = false;
                        current_slot_data.upload_image_id = 0;
                        current_slot_data.upload_image_id_present = false;

                        reader.enterContainer();
                        //log_debug() << "container/map";
                        parse_slot_info_response(reader, NULL, NULL, &current_slot_data);
                        reader.leaveContainer();

                        image_data->slot_data.append(current_slot_data);
                    }
                }

                if (reader.lastError() == QCborError::NoError)
                {
                    log_debug() << "leave";
                    reader.leaveContainer();
                }
            }
            else
            {
                reader.next();
            }
            break;
        }

        default:
        {
            reader.next();
        }
        }
    }

    if (entered_map == true)
    {
        reader.leaveContainer();
    }

    return (reader.lastError() ? false : true);
}

void smp_group_img_mgmt::file_upload(QByteArray *message)
{
    int64_t off = -1;
    bool good = true;

    if (message != nullptr)
    {
        img_mgmt_upload_match match = MATCH_NOT_PRESENT;
        QCborStreamReader cbor_reader(*message);
        good = parse_upload_response(cbor_reader, &off, &match);

    //    qDebug() << "rc = " << rc << ", off = " << off;

        if (off != -1 /*&& rc != 9*/)
        {
            if (off < this->file_upload_area)
            {
                ++upload_repeated_parts;

                if (upload_repeated_parts > 3)
                {
                    //Repeated lower offset 3 times, going in loop uploading the same thing over and over
//TODO:
log_error() << "Going in circles...";
                }
            }
            else
            {
                upload_repeated_parts = 0;
            }

            this->file_upload_area = off;
        }
        else
        {
            upload_repeated_parts = 0;
        }
        //    qDebug() << "good is " << good;

        if (this->file_upload_area != 0)
        {
            emit progress(smp_user_data, this->file_upload_area * 100 / this->file_upload_data.length());
            //progress_IMG_Complete->setValue(this->file_upload_area * 100 / this->file_upload_data.length());
        }
    }

    if (good == true)
    {
        //Upload next chunk
        uint max_size = processor->max_message_data_size(smp_mtu);

        if (this->file_upload_area >= (uint32_t)this->file_upload_data.length())
        {
            float blah = this->file_upload_data.length();
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

            blah = this->file_upload_data.length() / (float)(this->upload_tmr.elapsed() / 1000);
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

            mode = MODE_IDLE;
            this->upload_image = 0;
            this->file_upload_data.clear();
            this->upload_tmr.invalidate();
            this->upload_hash.clear();
            this->file_upload_area = 0;
            this->upgrade_only = false;
//                emit plugin_set_status(false, false);
//                lbl_IMG_Status->setText("Finished.");
            emit progress(smp_user_data, 100);
            emit status(smp_user_data, STATUS_COMPLETE, QString("~").append(QString::number(blah)).append(bob).append("ps throughput"));

            return;
        }

        smp_message *tmp_message = new smp_message();
        tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_IMG, COMMAND_UPLOAD, 2 + (this->file_upload_area == 0 ? ((this->upload_image != 0 ? 1 : 0) + 2 + (this->upgrade_only == true ? 1 : 0)): 0));

        if (this->file_upload_area == 0)
        {
            //Initial packet, extra data is needed: generate upload hash
            QByteArray session_hash = QCryptographicHash::hash(this->file_upload_data, QCryptographicHash::Sha256);

            if (this->upload_image != 0)
            {
                tmp_message->writer()->append("image");
                tmp_message->writer()->append(this->upload_image);
            }

            tmp_message->writer()->append("len");
            tmp_message->writer()->append(this->file_upload_data.length());
            tmp_message->writer()->append("sha");
            tmp_message->writer()->append(session_hash);

            if (this->upgrade_only == true)
            {
                tmp_message->writer()->append("upgrade");
                tmp_message->writer()->append(true);
            }
        }

        tmp_message->writer()->append("off");
        tmp_message->writer()->append(this->file_upload_area);
        tmp_message->writer()->append("data");

        //CBOR element header is 2 bytes with 1 byte end token
        max_size = max_size - tmp_message->size() - 3;

        tmp_message->writer()->append(this->file_upload_data.mid(this->file_upload_area, max_size));

        //	    qDebug() << "off: " << this->file_upload_area << ", left: " << this->file_upload_data.length();

        tmp_message->end_message();

        //      qDebug() << "len: " << tmp_message->data()->length();

        processor->send(tmp_message, smp_timeout, smp_retries, (this->file_upload_area == 0 ? true : false));
    }
    else
    {
        mode = MODE_IDLE;
    }
}

void smp_group_img_mgmt::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    Q_UNUSED(op);

//    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        log_error() << "Unexpected response, not busy";
        emit status(smp_user_data, STATUS_ERROR, "Unexpected response, shell mgmt not busy");
    }
    else if (group != SMP_GROUP_ID_IMG)
    {
        log_error() << "Unexpected group " << group << ", not " << SMP_GROUP_ID_IMG;
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

        if (mode == MODE_UPLOAD_FIRMWARE && command == COMMAND_UPLOAD)
        {
#if 0
            if (command == 0x00)
            {
                //Response to set image state
                int32_t rc = -1;
                QCborStreamReader cbor_reader(*data);
                bool good = parse_state_response(cbor_reader, &rc, "");
                //		    qDebug() << "Got " << good << ", " << rc;

                //		    edit_IMG_Log->appendPlainText(QString("Finished #2 in ").append(QString::number(this->upload_tmr.elapsed())).append("ms"));
                lbl_IMG_Status->setText(QString("Finished #2 in ").append(QString::number(this->upload_tmr.elapsed())).append("ms"));

                file_upload_in_progress = false;
                this->upload_tmr.invalidate();
                this->upload_hash.clear();
                this->file_upload_area = 0;
                emit plugin_set_status(false, false);
            }
#endif
            file_upload(&data);
        }
        else if (mode == MODE_SET_IMAGE && command == COMMAND_STATE)
        {
                //Response to set image state
                //            message.remove(0, 8);
//TODO:
            if (host_images != nullptr)
            {
                host_images->clear();
            }

            QCborStreamReader cbor_reader(data);
            bool good = parse_state_response(cbor_reader, "");
            //		    qDebug() << "Got " << good << ", " << rc;

            //		    edit_IMG_Log->appendPlainText(QString("Finished #2 in ").append(QString::number(this->upload_tmr.elapsed())).append("ms"));
            //lbl_IMG_Status->setText("Finished.");

            //file_list_in_progress = false;
            //emit plugin_set_status(false, false);
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
            mode = MODE_IDLE;
        }
        else if (mode == MODE_LIST_IMAGES && command == COMMAND_STATE)
        {
//TODO:
            //Response to set image state
            //            message.remove(0, 8);
            QCborStreamReader cbor_reader(data);
            bool good = parse_state_response(cbor_reader, "");
            //		    qDebug() << "Got " << good << ", " << rc;

            //		    edit_IMG_Log->appendPlainText(QString("Finished #2 in ").append(QString::number(this->upload_tmr.elapsed())).append("ms"));
            //lbl_IMG_Status->setText("Finished.");

            //file_list_in_progress = false;
            //emit plugin_set_status(false, false);
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
            mode = MODE_IDLE;
        }
        else if (mode == MODE_ERASE_IMAGE && command == COMMAND_ERASE)
        {
            //Response to erase image
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
            mode = MODE_IDLE;
        }
        else if (mode == MODE_SLOT_INFO && command == COMMAND_SLOT_INFO)
        {
            //Response to slot info
            QCborStreamReader cbor_reader(data);
            bool good = parse_slot_info_response(cbor_reader, host_slots, NULL, NULL);
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
            mode = MODE_IDLE;
        }
        else
        {
            log_error() << "Unsupported command received";
            mode = MODE_IDLE;
        }
    }
}

void smp_group_img_mgmt::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    Q_UNUSED(version);
    Q_UNUSED(op);
    Q_UNUSED(group);
    Q_UNUSED(error);

    bool cleanup = true;
    log_error() << "error :(";

    if (command == COMMAND_STATE && mode == MODE_LIST_IMAGES)
    {
        //TODO
        //error.type == SMP_ERROR_RC && error.rc ==
//            STATUS_UNSUPPORTED
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_STATE && mode == MODE_SET_IMAGE)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_UPLOAD && mode == MODE_UPLOAD_FIRMWARE)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_ERASE && mode == MODE_ERASE_IMAGE)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else if (command == COMMAND_SLOT_INFO && mode == MODE_SLOT_INFO)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else
    {
        //Unexpected response operation for mode
        emit status(smp_user_data, STATUS_ERROR, QString("Unexpected error (Mode: %1, op: %2)").arg(mode_to_string(mode), command_to_string(command)));
    }

    if (cleanup == true)
    {
        if (mode == MODE_UPLOAD_FIRMWARE)
        {
            upload_image = 0;
            file_upload_data.clear();
            file_upload_area = 0;
            upload_tmr.invalidate();
            upload_hash.clear();
            upgrade_only = false;
        }

        mode = MODE_IDLE;
    }
}

void smp_group_img_mgmt::timeout(smp_message *message)
{
    log_error() << "timeout :(";

    if (mode == MODE_UPLOAD_FIRMWARE)
    {
        upload_image = 0;
        file_upload_data.clear();
        file_upload_area = 0;
        upload_tmr.invalidate();
        upload_hash.clear();
        upgrade_only = false;
    }

    //TODO:
    emit status(smp_user_data, STATUS_TIMEOUT, QString("Timeout (Mode: %1)").arg(mode_to_string(mode)));

    mode = MODE_IDLE;
}

void smp_group_img_mgmt::cancel()
{
    if (mode != MODE_IDLE)
    {
        if (mode == MODE_UPLOAD_FIRMWARE)
        {
            upload_image = 0;
            file_upload_data.clear();
            file_upload_area = 0;
            upload_tmr.invalidate();
            upload_hash.clear();
            upgrade_only = false;
        }

        mode = MODE_IDLE;

        emit status(smp_user_data, STATUS_CANCELLED, nullptr);
    }
}

bool smp_group_img_mgmt::start_image_get(QList<image_state_t> *images)
{
    host_images = images;
//    colview_IMG_Images->previewWidget()->hide();
//    model_image_state.clear();
    host_images->clear();

    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_IMG, COMMAND_STATE, 0);
    tmp_message->end_message();

    mode = MODE_LIST_IMAGES;

    //	    qDebug() << "len: " << message.length();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_img_mgmt::start_image_set(QByteArray *hash, bool confirm, QList<image_state_t> *images)
{
    host_images = images;

    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_IMG, COMMAND_STATE, (confirm == true ? 2 : 1));

    tmp_message->writer()->append("hash");
    tmp_message->writer()->append(*hash);

    if (confirm == true)
    {
        tmp_message->writer()->append("confirm");
        tmp_message->writer()->append(true);
    }

    //			    qDebug() << message;
    //			    qDebug() << "hash is " << this->upload_hash;
    tmp_message->end_message();

    mode = MODE_SET_IMAGE;
    processor->send(tmp_message, smp_timeout, smp_retries, true);

//    lbl_IMG_Status->setText(QString("Marking image ").append(radio_IMG_Test->isChecked() ? "for test." : "as confirmed."));

    return true;
}

bool smp_group_img_mgmt::start_firmware_update(uint8_t image, QString filename, bool upgrade, QByteArray *image_hash)
{
    //Upload
    QFile file(filename);

    if (!file.open(QFile::ReadOnly))
    {
        emit status(smp_user_data, STATUS_ERROR, "File open failed");
        return false;
    }

    this->file_upload_data.clear();
    this->file_upload_data.append(file.readAll());

    file.close();

    if (extract_header(&this->file_upload_data, &upload_endian) == false)
    {
        this->file_upload_data.clear();
        emit status(smp_user_data, STATUS_ERROR, "MCUboot header was not found");
        return false;
    }
    else if (extract_hash(&this->file_upload_data, &this->upload_hash) == false)
    {
        this->file_upload_data.clear();
        emit status(smp_user_data, STATUS_ERROR, "Hash was not found");
        return false;
    }

    //Send start
    mode = MODE_UPLOAD_FIRMWARE;
    this->upload_image = image;
    this->file_upload_area = 0;
    this->upgrade_only = upgrade;
    this->upload_tmr.start();

    file_upload(nullptr);

    if (image_hash != nullptr)
    {
        *image_hash = this->upload_hash;
    }

    return true;
}

bool smp_group_img_mgmt::start_image_erase(uint8_t slot)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_IMG, COMMAND_ERASE, 1);

    tmp_message->writer()->append("slot");
    tmp_message->writer()->append(slot);

    //			    qDebug() << message;
    //			    qDebug() << "hash is " << this->upload_hash;
    tmp_message->end_message();

    mode = MODE_ERASE_IMAGE;
    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

bool smp_group_img_mgmt::start_image_slot_info(QList<slot_info_t> *images)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_READ, smp_version, SMP_GROUP_ID_IMG, COMMAND_SLOT_INFO, 0);

    //			    qDebug() << message;
    //			    qDebug() << "hash is " << this->upload_hash;
    tmp_message->end_message();

    mode = MODE_SLOT_INFO;
    host_slots = images;
    host_slots->clear();

    processor->send(tmp_message, smp_timeout, smp_retries, true);

    return true;
}

QString smp_group_img_mgmt::mode_to_string(uint8_t mode)
{
    switch (mode)
    {
        case MODE_IDLE:
            return "Idle";
        case MODE_UPLOAD_FIRMWARE:
            return "Upload firmware";
        case MODE_LIST_IMAGES:
            return "List image state";
        case MODE_SET_IMAGE:
            return "Set image state";
        case MODE_ERASE_IMAGE:
            return "Erase image";
        case MODE_SLOT_INFO:
            return "Slot info";
        default:
            return "Invalid";
    }
}

QString smp_group_img_mgmt::command_to_string(uint8_t command)
{
    switch (command)
    {
        case COMMAND_UPLOAD:
            return "Upload firmware";
        case COMMAND_STATE:
            return "Image state";
        case COMMAND_ERASE:
            return "Erase image";
        case COMMAND_SLOT_INFO:
            return "Slot info";
        default:
            return "Invalid";
    }
}

bool smp_group_img_mgmt::error_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_values.length())
    {
        *error = smp_error_values.at(rc);
        return true;
    }

    return false;
}

bool smp_group_img_mgmt::error_define_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_defines.length())
    {
        *error = smp_error_defines.at(rc);
        return true;
    }

    return false;
}
