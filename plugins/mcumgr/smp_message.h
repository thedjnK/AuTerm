/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_message.h
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
#ifndef SMP_MESSAGE_H
#define SMP_MESSAGE_H

#include <QByteArray>
#include <QCborStreamWriter>
#include "smp_error.h"

enum smp_op_t : uint8_t {
    SMP_OP_READ = 0,
    SMP_OP_READ_RESPONSE,
    SMP_OP_WRITE,
    SMP_OP_WRITE_RESPONSE,
};

struct smp_hdr {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    smp_op_t nh_op:3;               /* MGMT_OP_[...] */
    uint8_t  nh_version:2;
    uint8_t  _res1:3;
#else
    uint8_t  _res1:3;
    uint8_t  nh_version:2;
    smp_op_t nh_op:3;               /* MGMT_OP_[...] */
#endif
    uint8_t  nh_flags;              /* Reserved for future flags */
    uint16_t nh_len;                /* Length of the payload */
    uint16_t nh_group;              /* MGMT_GROUP_ID_[...] */
    uint8_t  nh_seq;                /* Sequence number */
    uint8_t  nh_id;                 /* Message ID within group */
};

//Ensure header size is correct
static_assert(sizeof(smp_hdr) == 8);

class smp_message
{
public:
    smp_message();
    void start_message(smp_op_t op, uint8_t version, uint16_t group, uint8_t id);
    //~smp_mesage();
    void append(const QByteArray data);
    void append(const QByteArray *data);
    void clear();
    smp_hdr *get_header(void);
    int size(void);
    int data_size(void);
    bool is_valid(void);
    void set_header(const smp_hdr *data);
    void set_header(const QByteArray data);
    void set_header(const smp_op_t operation, const uint8_t version, const uint8_t flags, const uint16_t length, const uint16_t group, const uint8_t sequence, const uint8_t command);
    QByteArray *data(void);
    QByteArray contents(void);
    static smp_op_t response_op(smp_op_t op);
    void end_message();
    QCborStreamWriter *writer();

private:
    QByteArray buffer;
    bool header_added;
    QCborStreamWriter cbor_writer = QCborStreamWriter(&buffer);
};

#endif // SMP_MESSAGE_H
