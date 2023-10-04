/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_message.cpp
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
#include "smp_message.h"
#include <QIODevice>

const uint8_t SMP_VERSION_1_HEADER = 0x00;
const uint8_t SMP_VERSION_2_HEADER = 0x08;

smp_message::smp_message()
{
    this->header_added = false;
}

void smp_message::start_message(smp_op_t op, uint8_t version, uint16_t group, uint8_t id)
{
    this->buffer.append((char)((version == 1 ? SMP_VERSION_2_HEADER : SMP_VERSION_1_HEADER) | op));  /* Read | Write (0x00 | 0x02) */
    this->buffer.append((char)0x00);  /* Flags */
    this->buffer.append((char)0x00);  /* Length A */
    this->buffer.append((char)0x00);  /* Length B */
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    this->buffer.append((char)(group >> 8));  /* Group A */
    this->buffer.append((char)group);  /* Group B */
#else
    this->buffer.append((char)group);  /* Group A */
    this->buffer.append((char)(group >> 8));  /* Group B */
#endif
    this->buffer.append((char)0x00);  /* Sequence */
    this->buffer.append((char)id);   /* Message ID */

    cbor_writer.device()->seek(this->buffer.length());
    cbor_writer.startMap();

    this->header_added = true;
}

/*smp_message::~smp_message()
{

}*/

void smp_message::append(const QByteArray data)
{
    this->buffer.append(data);
}

void smp_message::append(const QByteArray *data)
{
    this->buffer.append(*data);
}

void smp_message::clear()
{
    this->buffer.clear();
    this->header_added = false;
}

smp_hdr *smp_message::get_header(void)
{
    if (this->buffer.size() < (int)sizeof(struct smp_hdr))
    {
        return NULL;
    }

    return (smp_hdr *)this->buffer.data();
}

int smp_message::size(void)
{
    return this->buffer.size();
}

int smp_message::data_size(void)
{
    if (this->buffer.size() < (int)sizeof(smp_hdr))
    {
        return 0;
    }

    return this->buffer.size() - sizeof(smp_hdr);
}

bool smp_message::is_valid(void)
{
    uint16_t data_size;

    if (this->buffer.size() < sizeof(smp_hdr))
    {
        return false;
    }

    data_size = ((smp_hdr *)this->buffer.data())->nh_len;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    data_size = ((data_size & 0xff) << 8) | ((data_size & 0xff00) >> 8);
#endif

    if (this->buffer.size() >= data_size + sizeof(smp_hdr))
    {
        return true;
    }

//TODO: additional verification?
    return false;
}

void smp_message::set_header(const smp_hdr *data)
{
    if (this->header_added == false)
    {
        this->buffer.insert(0, (char *)data, sizeof(smp_hdr));
        this->header_added = true;
    }
    else
    {
        this->buffer.replace(0, sizeof(smp_hdr), (char *)data);
    }
}

void smp_message::set_header(const QByteArray data)
{
    if (this->header_added == false)
    {
        this->buffer.insert(0, data);
        this->header_added = true;
    }
    else
    {
        this->buffer.replace(0, sizeof(smp_hdr), data);
    }
}

void smp_message::set_header(const smp_op_t operation, const uint8_t version, const uint8_t flags, const uint16_t length, const uint16_t group, const uint8_t sequence, const uint8_t command)
{
    Q_UNUSED(operation);
    Q_UNUSED(version);
    Q_UNUSED(flags);
    Q_UNUSED(length);
    Q_UNUSED(group);
    Q_UNUSED(sequence);
    Q_UNUSED(command);
//TODO: MSVC cannot cope with a simple piece of code like this
#if 0
    const struct smp_hdr data = {
        .nh_op = operation,
        .nh_version = version,
        ._res1 = 0,
        .nh_flags = flags,
        .nh_len = length,
        .nh_group = group,
        .nh_seq = sequence,
        .nh_id = command,
    };

    set_header(&data);
#endif
}

QByteArray *smp_message::data(void)
{
    return &this->buffer;
}

QByteArray smp_message::contents(void)
{
    return this->buffer.mid(sizeof(smp_hdr));
}

smp_op_t smp_message::response_op(smp_op_t op)
{
    return op == SMP_OP_READ ? SMP_OP_READ_RESPONSE : SMP_OP_WRITE_RESPONSE;
}

void smp_message::end_message()
{
    if (this->header_added == true)
    {
        cbor_writer.endMap();

        uint16_t data_size = (uint16_t)(this->buffer.length() - sizeof(sizeof(smp_hdr)));
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        this->buffer[2] = (uint8_t)(data_size >> 8);
        this->buffer[3] = (uint8_t)data_size;
#else
        this->buffer[2] = (uint8_t)data_size;
        this->buffer[3] = (uint8_t)(data_size >> 8);
#endif
    }
}

QCborStreamWriter *smp_message::writer()
{
    return &cbor_writer;
}
