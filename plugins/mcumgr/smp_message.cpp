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

smp_message::smp_message()
{
    this->header_added = false;
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

const smp_hdr *smp_message::get_header(void)
{
    if (this->buffer.size() < (int)sizeof(struct smp_hdr))
    {
        return NULL;
    }

    return (const smp_hdr *)this->buffer.data();
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
//TODO
return true;
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
