/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_processor.h
**
** Notes:   With exception to the crc16() function which is apache 2.0 licensed
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

#include "smp_processor.h"
#include "smp_group.h"

smp_processor::smp_processor(QObject *parent, smp_uart *uart_driver)
{
    uart = uart_driver;
    last_message = nullptr;
    last_message_header = nullptr;
    repeat_times = 0;
    busy = false;

    connect(&repeat_timer, SIGNAL(timeout()), this, SLOT(message_timeout()));
    repeat_timer.setSingleShot(true);
}

smp_processor::~smp_processor()
{
    cleanup();
    group_handlers.clear();
}

bool smp_processor::send(smp_message *message, uint32_t timeout_ms, uint8_t repeats)
{
    if (busy)
    {
        return false;
    }

    last_message = message;
    last_message_header = message->get_header();
    repeat_timer.setInterval(timeout_ms);
    repeat_times = repeats;
    busy = true;

    uart->send(last_message);
    repeat_timer.start();

    return true;
}

bool smp_processor::is_busy()
{
    return busy;
}

void smp_processor::register_handler(uint16_t group, smp_group *handler)
{
    group_handlers.append(smp_group_match_t{group, handler});
}

void smp_processor::unregister_handler(uint16_t group)
{
    uint8_t i = 0;
    while (i < group_handlers.length())
    {
        if (group_handlers[i].group == group)
        {
            group_handlers.removeAt(i);
            break;
        }

        ++i;
    }
}

void smp_processor::cleanup()
{
    if (!busy)
    {
        return;
    }

    repeat_timer.stop();

    if (last_message != nullptr)
    {
        delete last_message;
        last_message = nullptr;
        last_message_header = nullptr;
    }

    repeat_times = 0;
    busy = false;
}

void smp_processor::message_timeout()
{
    if (!busy)
    {
        //No longer busy
        return;
    }

    if (repeat_times == 0)
    {
        //Too many repeats
        uint16_t group = last_message_header->nh_group;
        uint8_t i = 0;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        group = ((group & 0xff) << 8) | ((group & 0xff00) >> 8);
#endif

        //Search for the handler for this group
        while (i < group_handlers.length())
        {
            if (group_handlers[i].group == group)
            {
                break;
            }

            ++i;
        }

        if (i == group_handlers.length())
        {
            //There is no registered handler for this group
            qDebug() << "No registered handler for group " << group << ", cannot send timeout message.";
            cleanup();
        }
        else
        {
            //Keep message pointer valid but cleanup so callback can send a message
            smp_message *backup_message = last_message;
            last_message = nullptr;
            last_message_header = nullptr;

            cleanup();
            group_handlers[i].handler->timeout(backup_message);

            //Delete backup pointer
            delete backup_message;
        }

        return;
    }

    //Resend message
    --repeat_times;
    repeat_timer.start();
    uart->send(last_message);
}

void smp_processor::message_received(smp_message *response)
{
    const smp_hdr *response_header = nullptr;

    if (!busy)
    {
        //Not busy so this message probably isn't wanted anymore
        return;
    }

    //Check if this an expected response
    response_header = response->get_header();

    if (response_header == nullptr)
    {
        //Cannot do anything without a header
        qDebug() << "Invalid response header";
    }
    else if (response_header->nh_group != last_message_header->nh_group)
    {
        qDebug() << "Invalid group, expected " << last_message_header->nh_group << " got " << response_header->nh_group;
    }
    else if (response_header->nh_id != last_message_header->nh_id)
    {
        qDebug() << "Invalid command, expected " << last_message_header->nh_id << " got " << response_header->nh_id;
    }
    else if (response_header->nh_seq != last_message_header->nh_seq)
    {
        qDebug() << "Invalid sequence, expected " << last_message_header->nh_seq << " got " << response_header->nh_seq;
    }
    else if (response_header->nh_op != smp_message::response_op(last_message_header->nh_op))
    {
        qDebug() << "Invalid op, expected " << smp_message::response_op(last_message_header->nh_op) << " got " << response_header->nh_op;
    }
    else
    {
        //Headers look valid
        uint8_t version = response_header->nh_version;
        uint8_t op = response_header->nh_op;
        uint16_t group = response_header->nh_group;
        uint8_t command = response_header->nh_id;
        uint8_t i = 0;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        group = ((group & 0xff) << 8) | ((group & 0xff00) >> 8);
#endif

        //Search for the handler for this group
        while (i < group_handlers.length())
        {
            if (group_handlers[i].group == group)
            {
                break;
            }

            ++i;
        }

        if (i == group_handlers.length())
        {
            //There is no registered handler for this group, clean up
            qDebug() << "No registered handler for group " << group << ", dropping response.";
            this->cleanup();
            return;
        }

        QCborStreamReader cbor_reader(response->contents());
        smp_error_t error;
        error.type = SMP_ERROR_NONE;
        bool parsed = decode_message(cbor_reader, version, 0, nullptr, &error);

        if (!parsed)
        {
            return;
        }

        //Clean up before triggering callback
        this->cleanup();

        if (error.type != SMP_ERROR_NONE)
        {
            //Received either "rc" (legacy) error or "ret" error
            group_handlers[i].handler->receive_error(version, op, group, command, error);
        }
        else
        {
            //No error, good response
            group_handlers[i].handler->receive_ok(version, op, group, command, response->contents());
        }
    }
}

bool smp_processor::decode_message(QCborStreamReader &reader, uint8_t version, uint16_t level, QString *parent, smp_error_t *error)
{
    QString key = "";

    while (!reader.lastError() && reader.hasNext())
    {
        bool keyset = false;
        switch (reader.type())
        {
            case QCborStreamReader::UnsignedInteger:
            case QCborStreamReader::NegativeInteger:
            {
                if (key == "rc" && version == 0 && level == 0)
                {
                    error->rc = reader.toInteger();
                    error->type = SMP_ERROR_RC;
                }
                else if (key == "rc" && version == 1 && level == 1 && parent != nullptr && *parent == "ret")
                {
                    error->rc = reader.toUnsignedInteger();
                    error->type = SMP_ERROR_RET;
                }
                else if (key == "group" && version == 1 && level == 1 && parent != nullptr && *parent == "ret")
                {
                    error->group = reader.toUnsignedInteger();
                    error->type = SMP_ERROR_RET;
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
                    decode_message(reader, version, (level + 1), &key, error);
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
                break;
            }

            if (keyset == false && !key.isEmpty())
            {
                key = "";
            }
        }
    }

    if (reader.lastError())
    {
        qDebug() << "Failed to parse CBOR message: " << reader.lastError().toString();
        return false;
    }

    return true;
}
