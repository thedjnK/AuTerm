/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_fs_mgmt.h
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
#ifndef SMP_GROUP_FS_MGMT_H
#define SMP_GROUP_FS_MGMT_H

#include "smp_group.h"
#include "smp_error.h"
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>
#include <QFile>

struct hash_checksum_t {
    QString name;
    uint8_t format;
    uint16_t size;
};

class smp_group_fs_mgmt : public smp_group
{
    Q_OBJECT

public:
    smp_group_fs_mgmt(smp_processor *parent);
    void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data);
    void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error);
    void timeout(smp_message *message);
    void cancel();
    bool start_upload(QString file_name, QString destination_name);
    bool start_download(QString file_name, QString destination_name);
    bool start_status(QString file_name, uint32_t *file_size);
    bool start_hash_checksum(QString file_name, QString hash_checksum, QByteArray *result, uint32_t *file_size);
    bool start_supported_hashes_checksums(QList<hash_checksum_t> *hash_checksum_list);
    bool start_file_close();
    static bool error_lookup(int32_t rc, QString *error);
    static bool error_define_lookup(int32_t rc, QString *error);

private:
    bool parse_upload_response(QCborStreamReader &reader, uint32_t *off, bool *off_found);
    bool parse_download_response(QCborStreamReader &reader, uint32_t *off, uint32_t *len, QByteArray *file_data);
    bool parse_status_response(QCborStreamReader &reader, uint32_t *len);
    bool parse_hash_checksum_response(QCborStreamReader &reader, QString *type, QByteArray *hash_checksum, uint32_t *file_size);
    bool parse_supported_hashes_checksums_response(QCborStreamReader &reader, bool in_data, QString *key_name, hash_checksum_t *current_item);
//    bool parse_file_close_response(QCborStreamReader &reader, int32_t *ret, QString *response);
    void upload_chunk();
    void download_chunk();
    void flip_endian(uint8_t *data, uint8_t size);

    QString mode_to_string(uint8_t mode);
    QString command_to_string(uint8_t command);

    //
    uint8_t mode;
    int32_t *return_ret;
    QFile local_file;
    int32_t local_file_size;
    uint32_t file_upload_area;
    QElapsedTimer upload_tmr;
    QString device_file_name;
    QList<hash_checksum_t> *hash_checksum_object;
    QByteArray *hash_checksum_result_object;
    uint32_t *file_size_object;
};

#endif // SMP_GROUP_FS_MGMT_H
