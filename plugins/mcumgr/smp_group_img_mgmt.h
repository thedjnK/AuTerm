/******************************************************************************
** Copyright (C) 2023 Jamie M.
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

#include "smp_group.h"
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>
#include <QStandardItem>

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
    QStandardItem *item;
};

struct image_state_t {
    uint32_t image;
    bool image_set;
    QList<slot_state_t> slot_list;
    QStandardItem *item;
};

enum img_mgmt_upload_match : uint8_t {
    MATCH_NOT_PRESENT = 0,
    MATCH_FAILED,
    MATCH_PASSED
};

class smp_group_img_mgmt : public smp_group
{
    Q_OBJECT

public:
    smp_group_img_mgmt(smp_processor *parent);
    void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data);
    void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error);
    void timeout(smp_message *message);
    void cancel();
    bool start_image_get(QList<image_state_t> *images);
    bool start_image_set(QByteArray *hash, bool confirm);
    bool start_firmware_update(uint8_t image, QString filename, bool upgrade, QByteArray *image_hash);
    bool start_image_erase(uint8_t slot);
    static bool error_lookup(int32_t rc, QString *error);

signals:
    void plugin_to_hex(QByteArray *data);

private:
    bool extract_hash(QByteArray *file_data, QByteArray *hash);
    bool parse_upload_response(QCborStreamReader &reader, int64_t *new_off, img_mgmt_upload_match *match);
    bool parse_state_response(QCborStreamReader &reader, QString array_name);
    void file_upload(QByteArray *message);
    QString mode_to_string(uint8_t mode);
    QString command_to_string(uint8_t command);

    //
    uint8_t mode;
    uint8_t upload_image;
    QByteArray file_upload_data;
    uint32_t file_upload_area;
    QElapsedTimer upload_tmr;
    QByteArray upload_hash;
    bool upgrade_only;
    uint8_t upload_repeated_parts;
    QList<image_state_t> *host_images;
};

#endif // SMP_GROUP_IMG_MGMT_H
