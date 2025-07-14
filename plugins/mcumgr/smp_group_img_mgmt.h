/******************************************************************************
** Copyright (C) 2023-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_img_mgmt.h
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
#ifndef SMP_GROUP_IMG_MGMT_H
#define SMP_GROUP_IMG_MGMT_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "smp_group.h"
#include "smp_error.h"
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
struct slot_state_t {
    uint32_t slot;
    QByteArray version;
    QByteArray hash;
    bool bootable;
    bool pending;
    bool confirmed;
    bool active;
    bool permanent;
    bool splitstatus;
};

struct image_state_t {
    uint32_t image;
    bool image_set;
    QList<slot_state_t> slot_list;
};

struct slot_info_slots_t {
    uint32_t slot;
    uint32_t upload_image_id;
    uint32_t size;
    bool upload_image_id_present;
    bool size_present;
};

struct slot_info_t {
    uint32_t image;
    QList<slot_info_slots_t> slot_data;
    uint32_t max_image_size;
    bool max_image_size_present;
};

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum img_mgmt_upload_match : uint8_t {
    MATCH_NOT_PRESENT = 0,
    MATCH_FAILED,
    MATCH_PASSED
};

//Indicates the endianess of the image, if it matches the host PC or is opposite (or unknown)
enum image_endian_t {
    ENDIAN_BIG,
    ENDIAN_LITTLE,
    ENDIAN_UNKNOWN
};

/******************************************************************************/
// Class definitions
/******************************************************************************/
class smp_group_img_mgmt : public smp_group
{
    Q_OBJECT

public:
    smp_group_img_mgmt(smp_processor *parent);
    void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data) override;
    void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error) override;
    void cancel() override;
    bool start_image_get(QList<image_state_t> *images);
    bool start_image_set(QByteArray *hash, bool confirm, QList<image_state_t> *images);
    bool start_firmware_update(uint8_t image, QString filename, bool upgrade, QByteArray *image_hash);
    bool start_firmware_update(uint8_t image, QString filename, bool upgrade, QByteArray *image_hash, uint32_t first_packet_timeout);
    bool start_image_erase(uint8_t slot);
    bool start_image_slot_info(QList<slot_info_t> *images);

protected:
    void cleanup() override;
    QString mode_to_string(uint8_t mode) override;
    QString command_to_string(uint8_t command) override;

signals:
    void plugin_to_hex(QByteArray *data);

private:
    static bool error_lookup(int32_t rc, QString *error);
    static bool error_define_lookup(int32_t rc, QString *error);
    bool extract_header(QByteArray *file_data, image_endian_t *endian);
    bool extract_hash(QByteArray *file_data, QByteArray *hash);
    bool parse_upload_response(QCborStreamReader &reader, int64_t *new_off, img_mgmt_upload_match *match);
    bool parse_state_response(QCborStreamReader &reader, QString array_name);
    bool parse_slot_info_response(QCborStreamReader &reader, QList<slot_info_t> *images, struct slot_info_t *image_data, struct slot_info_slots_t *slot_data);
    void file_upload(QByteArray *message);

    //
    uint8_t upload_image;
    QByteArray file_upload_data;
    uint32_t file_upload_area;
    QElapsedTimer upload_tmr;
    QByteArray upload_hash;
    image_endian_t upload_endian;
    bool upgrade_only;
    uint8_t upload_repeated_parts;
    uint32_t upload_initial_timeout;
    QList<image_state_t> *host_images;
    QList<slot_info_t> *host_slots;
    image_state_t image_state_buffer;
    slot_state_t slot_state_buffer;
};

#endif // SMP_GROUP_IMG_MGMT_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
