/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group.h
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
#ifndef SMP_GROUP_H
#define SMP_GROUP_H

#include "smp_message.h"
#include "smp_processor.h"

class smp_group
{
public:
    smp_group(smp_processor *parent, uint16_t group_id)
    {
        processor = parent;
        processor->register_handler(group_id, this);
    }
    virtual void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data) = 0;
    virtual void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error) = 0;
    virtual void timeout(smp_message *message) = 0;

private:
    smp_processor *processor;
};

#endif // SMP_GROUP_H
