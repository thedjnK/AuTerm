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

smp_processor::smp_processor(QObject *parent)
{
    Q_UNUSED(parent);

    sequence = 0;
    last_message = nullptr;
    last_message_header = nullptr;
    repeat_times = 0;
    busy = false;
#if defined(PLUGIN_MCUMGR_JSON)
    json_object = nullptr;
#endif
    message_logging = false;
    custom_message = false;

    connect(&repeat_timer, SIGNAL(timeout()), this, SLOT(message_timeout()));
    repeat_timer.setSingleShot(true);
}

smp_processor::~smp_processor()
{
    cleanup();
    disconnect(this, SLOT(message_timeout()));
    group_handlers.clear();

    //Ensure object is deleted if it was part-way through creation
    if (last_message != nullptr)
    {
        delete last_message;
        last_message = nullptr;
        last_message_header = nullptr;
    }
}

#ifndef SKIPPLUGIN_LOGGER
void smp_processor::set_logger(debug_logger *object)
{
    logger = object;
}
#endif

smp_transport_error_t smp_processor::send(smp_message *message, uint32_t timeout_ms, uint8_t repeats, bool allow_version_check)
{
    smp_transport_error_t transport_error = SMP_TRANSPORT_ERROR_OK;

    if (busy)
    {
        transport_error = SMP_TRANSPORT_ERROR_PROCESSOR_BUSY;
    }
    else if (transport->is_connected() == 0)
    {
        transport_error = SMP_TRANSPORT_ERROR_NOT_CONNECTED;
    }

    if (transport_error == SMP_TRANSPORT_ERROR_OK)
    {
        last_message = message;
        last_message_header = message->get_header();

        //Set message sequence
        last_message_header->nh_seq = sequence;
        last_message_version_check = allow_version_check;
        last_message_version = last_message_header->nh_version;
        repeat_timer.setInterval(timeout_ms);
        repeat_times = repeats;
        busy = true;

        transport_error = transport->send(last_message);

        if (transport_error == SMP_TRANSPORT_ERROR_OK)
        {
            repeat_timer.start();
            ++sequence;

#if defined(PLUGIN_MCUMGR_JSON)
            if (json_object != nullptr && message_logging == true)
            {
                json_object->append_data(true, message);
            }
#endif
        }
    }

    if (transport_error != SMP_TRANSPORT_ERROR_OK)
    {
        cleanup();
    }

    return transport_error;
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
        uint16_t group = last_message_header->nh_group;
        uint8_t i = 0;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        group = ((group & 0xff) << 8) | ((group & 0xff00) >> 8);
#endif

        if (!custom_message)
        {
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
                log_error() << "No registered handler for group " << group << ", cannot send timeout message.";
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
        }
        else
        {
            emit custom_message_callback(CUSTOM_MESSAGE_CALLBACK_TIMEOUT, nullptr);

            custom_message = false;
        }

        return;
    }

    //If this is a version 2 message, try sending a version 1 packet to see if version 2 is unsupported by the server
    if (last_message_version_check == true && last_message_version == 1)
    {
        if (last_message_header->nh_version == last_message_version)
        {
            last_message_header->nh_version = 0;
        }
        else
        {
            last_message_header->nh_version = 1;
        }
    }

    //Resend message
    --repeat_times;
    repeat_timer.start();
    transport->send(last_message);
}

void smp_processor::message_received(smp_message *response)
{
    const smp_hdr *response_header = nullptr;

    log_debug() << "got message";

    if (!busy)
    {
        //Not busy so this message probably isn't wanted anymore
        log_error() << "Received message when not awaiting for a repsonse";
        return;
    }

    //Check if this an expected response
    response_header = response->get_header();

    if (response_header == nullptr)
    {
        //Cannot do anything without a header
        log_error() << "Invalid response header";
    }
    else if (response_header->nh_group != last_message_header->nh_group)
    {
        log_error() << "Invalid group, expected " << last_message_header->nh_group << " got " << response_header->nh_group;
    }
    else if (response_header->nh_id != last_message_header->nh_id)
    {
        log_error() << "Invalid command, expected " << last_message_header->nh_id << " got " << response_header->nh_id;
    }
    else if (response_header->nh_seq != last_message_header->nh_seq)
    {
        log_error() << "Invalid sequence, expected " << last_message_header->nh_seq << " got " << response_header->nh_seq;
    }
    else if (response_header->nh_op != smp_message::response_op(last_message_header->nh_op))
    {
        log_error() << "Invalid op, expected " << smp_message::response_op(last_message_header->nh_op) << " got " << response_header->nh_op;
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

        if (!custom_message)
        {
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
                log_error() << "No registered handler for group " << group << ", dropping response.";
                this->cleanup();
                return;
            }
        }

        QCborStreamReader cbor_reader(response->contents());
        smp_error_t error;
        error.type = SMP_ERROR_NONE;
        bool parsed = decode_message(cbor_reader, version, 0, nullptr, &error);

        if (!parsed)
        {
            log_error() << "parse failed";
            return;
        }

        //Clean up before triggering callback
        this->cleanup();

        if (!custom_message)
        {
            if (error.type != SMP_ERROR_NONE)
            {
                //Received either "rc" (legacy/SMP version 1) error or "err" error (SMP version 2)
                group_handlers[i].handler->receive_error(version, op, group, command, error);
            }
            else
            {
                //No error, good response
                group_handlers[i].handler->receive_ok(version, op, group, command, response->contents());
            }
        }
        else
        {
            if (error.type != SMP_ERROR_NONE)
            {
                emit custom_message_callback(CUSTOM_MESSAGE_CALLBACK_ERROR, &error);
            }
            else
            {
                emit custom_message_callback(CUSTOM_MESSAGE_CALLBACK_OK, nullptr);
            }
        }

#if defined(PLUGIN_MCUMGR_JSON)
        if (json_object != nullptr && (message_logging || custom_message))
        {
            json_object->append_data(false, response);
        }
#endif
    }

    custom_message = false;
}

bool smp_processor::decode_message(QCborStreamReader &reader, uint8_t version, uint16_t level, QString *parent, smp_error_t *error)
{
    QString key = "";
    bool keyset = true;

    while (!reader.lastError() && reader.hasNext())
    {
        if (keyset == false && !key.isEmpty())
        {
            key = "";
        }

        keyset = false;

        switch (reader.type())
        {
            case QCborStreamReader::UnsignedInteger:
            case QCborStreamReader::NegativeInteger:
            {
                if (key == "rc" && version == 1 && level == 2 && parent != nullptr && *parent == "err")
                {
                    error->rc = reader.toUnsignedInteger();
                    error->type = SMP_ERROR_RET;
                }
                else if (key == "group" && version == 1 && level == 2 && parent != nullptr && *parent == "err")
                {
                    error->group = reader.toUnsignedInteger();
                    error->type = SMP_ERROR_RET;
                }
                else if (key == "rc" && level == 1)
                {
                    error->rc = reader.toInteger();
                    error->type = SMP_ERROR_RC;
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
        }
    }

    if (reader.lastError())
    {
        log_error() << "Failed to parse CBOR message: " << reader.lastError().toString();
        return false;
    }

    //Check if an error was received with value 0, which is not an error and is a success code
    if (level == 0 && error->type != SMP_ERROR_NONE && error->rc == 0)
    {
        error->type = SMP_ERROR_NONE;
    }

    return true;
}

void smp_processor::set_transport(smp_transport *transport_object)
{
    transport = transport_object;
}

uint16_t smp_processor::max_message_data_size(uint16_t mtu)
{
    return transport->max_message_data_size(mtu);
}

#if defined(PLUGIN_MCUMGR_JSON)
void smp_processor::set_json(smp_json *json)
{
    json_object = json;
}
#endif

void smp_processor::set_message_logging(bool enabled)
{
    message_logging = enabled;
}

void smp_processor::set_custom_message(bool enabled)
{
    custom_message = enabled;
}

void smp_processor::transport_disconnect(int error_code)
{
    uint16_t group;
    uint8_t i = 0;

    if (sender() != transport)
    {
        log_error() << "Non-active transport emitted error: " << sender() << ", code: " << error_code;
        return;
    }

    if (!busy)
    {
        //No longer busy
        return;
    }

    group = last_message_header->nh_group;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    group = ((group & 0xff) << 8) | ((group & 0xff00) >> 8);
#endif

    if (!custom_message)
    {
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
            log_error() << "No registered handler for group " << group << ", cannot send transport disconnected message.";
            cleanup();
        }
        else
        {
            //Keep message pointer valid but cleanup so callback can send a message
            smp_message *backup_message = last_message;
            last_message = nullptr;
            last_message_header = nullptr;

            cleanup();
            group_handlers[i].handler->transport_disconnected(backup_message, transport, error_code);

            //Delete backup pointer
            delete backup_message;
        }
    }
    else
    {
        emit custom_message_callback(CUSTOM_MESSAGE_CALLBACK_TRANSPORT_DISCONNECTED, nullptr);

        custom_message = false;
    }
}

void smp_processor::cancel()
{
    uint16_t group;
    uint8_t i = 0;

    if (!busy)
    {
        //No longer busy
        return;
    }

    group = last_message_header->nh_group;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    group = ((group & 0xff) << 8) | ((group & 0xff00) >> 8);
#endif

    if (!custom_message)
    {
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
            log_error() << "No registered handler for group " << group << ", cannot send cancelled message.";
            cleanup();
        }
        else
        {
            //Keep message pointer valid but cleanup so callback can send a message
            smp_message *backup_message = last_message;
            last_message = nullptr;
            last_message_header = nullptr;

            cleanup();
            group_handlers[i].handler->cancelled(backup_message);

            //Delete backup pointer
            delete backup_message;
        }
    }
    else
    {
        emit custom_message_callback(CUSTOM_MESSAGE_CALLBACK_CANCELLED, nullptr);

        custom_message = false;
    }
}
