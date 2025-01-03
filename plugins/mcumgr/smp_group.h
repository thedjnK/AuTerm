/******************************************************************************
** Copyright (C) 2023-2024 Jamie M.
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

#include <QObject>
#include <QStringBuilder>
#include "smp_message.h"
#include "smp_processor.h"
#include "smp_error.h"
#include "debug_logger.h"

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum group_status : uint8_t {
    STATUS_COMPLETE = 0,
    STATUS_ERROR,
    STATUS_UNSUPPORTED,
    STATUS_TIMEOUT,
    STATUS_CANCELLED,
    STATUS_PROCESSOR_TRANSPORT_ERROR,
    STATUS_MESSAGE_TOO_LARGE,
    STATUS_TRANSPORT_DISCONNECTED
};

enum smp_group_ids : uint16_t {
    SMP_GROUP_ID_OS = 0,
    SMP_GROUP_ID_IMG,
    SMP_GROUP_ID_STATS,
    SMP_GROUP_ID_SETTINGS,
    SMP_GROUP_ID_FS = 8,
    SMP_GROUP_ID_SHELL,
    SMP_GROUP_ID_ENUM,
    SMP_GROUP_ID_ZEPHYR = 63,
    SMP_GROUP_ID_USER_DEFINED,
};

class smp_group : public QObject
{
    Q_OBJECT

public:
    smp_group(smp_processor *parent, QString group_name, uint16_t group_id, smp_error_lookup error_lookup_function, smp_error_define_lookup error_define_lookup_function)
    {
        processor = parent;
        processor->register_handler(group_id, this);
        smp_error::register_error_lookup_function(group_id, this);
        name = group_name;
        error_lookup = error_lookup_function;
        error_define_lookup = error_define_lookup_function;
    }

    void set_parameters(uint8_t version, uint16_t mtu, uint8_t retries, uint32_t timeout, uint8_t user_data)
    {
        smp_version = version;
        smp_mtu = mtu;
        smp_retries = retries;
        smp_timeout = timeout;
        smp_user_data = user_data;
    }

    bool lookup_error(int32_t rc, QString *error)
    {
        if (error_lookup != nullptr)
        {
            return error_lookup(rc, error);
        }

        return false;
    }

    bool lookup_error_define(int32_t rc, QString *define)
    {
        if (error_define_lookup != nullptr)
        {
            bool good = error_define_lookup(rc, define);

            if (good == true)
            {
                define->prepend(QString(name).append("_MGMT_"));
            }

            return good;
        }

        return false;
    }

    bool check_message_before_send(smp_message *tmp_message)
    {
        if (tmp_message->data()->length() > processor->max_message_data_size(smp_mtu))
        {
            //Message is larger than the transport provides
            QString response = QString("Message too large for transport, ") % QString::number(tmp_message->data()->length()) % " vs " % QString::number(processor->max_message_data_size(smp_mtu)) % " bytes";

            delete tmp_message;
            cleanup();
            emit status(smp_user_data, STATUS_MESSAGE_TOO_LARGE, response);
            return false;
        }
        else
        {
            return true;
        }
    }

    bool handle_transport_error(smp_transport_error_t transport_error)
    {
        if (transport_error == SMP_TRANSPORT_ERROR_OK)
        {
            return true;
        }
        else
        {
            QString response = "Message send failed, ";

            switch (transport_error)
            {
                case SMP_TRANSPORT_ERROR_NOT_CONNECTED:
                {
                    response.append("transport not connected");
                    break;
                }
                case SMP_TRANSPORT_ERROR_PROCESSOR_BUSY:
                {
                    response.append("processor busy");
                    break;
                }
                default:
                {
                    response.append("unknown error: ").append(QString::number(transport_error));
                    break;
                }
            }

            cleanup();
            emit status(smp_user_data, STATUS_PROCESSOR_TRANSPORT_ERROR, response);
            return false;
        }
    }

    void timeout(smp_message *message)
    {
        Q_UNUSED(message);
        QString response = QString("Timeout (Mode: %1)").arg(mode_to_string(mode));

        log_error() << "MCUmgr command timed out";
        cleanup();
        emit status(smp_user_data, STATUS_TIMEOUT, response);
    }

    void transport_disconnected(smp_message *message, smp_transport *transport, int error_code)
    {
        Q_UNUSED(message);
        Q_UNUSED(transport);
        Q_UNUSED(error_code);
        QString response = QString("Transport disconnected (Mode: %1)").arg(mode_to_string(mode));

        log_error() << "MCUmgr transport disconnected";
        cleanup();
        emit status(smp_user_data, STATUS_TRANSPORT_DISCONNECTED, response);
    }

    void cancelled(smp_message *message)
    {
        Q_UNUSED(message);
        QString response = QString("Cancelled (Mode: %1)").arg(mode_to_string(mode));

        log_error() << "MCUmgr command cancelled";
        cleanup();
        emit status(smp_user_data, STATUS_TRANSPORT_DISCONNECTED, response);
    }

#ifndef SKIPPLUGIN_LOGGER
    void set_logger(debug_logger *object)
    {
        logger = object;
    }
#endif

    virtual void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data) = 0;
    virtual void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error) = 0;
    virtual void cancel() = 0;

protected:
    static enum group_status status_error_return(smp_error_t error)
    {
        if (error.type == SMP_ERROR_RC)
        {
            if (error.rc == SMP_RC_ERROR_ENOTSUP)
            {
                return STATUS_UNSUPPORTED;
            }
        }

        return STATUS_ERROR;
    }

    virtual void cleanup() = 0;
    virtual QString mode_to_string(uint8_t mode) = 0;
    virtual QString command_to_string(uint8_t command) = 0;

signals:
    void status(uint8_t user_data, group_status status, QString error_string);
    void progress(uint8_t user_data, uint8_t percent);
    void version_error(uint8_t supported_version);

protected:
    QString name;
    smp_processor *processor;
    uint8_t smp_version;
    uint16_t smp_mtu;
    uint8_t smp_retries;
    uint32_t smp_timeout;
    uint8_t smp_user_data;
    smp_error_lookup error_lookup;
    smp_error_define_lookup error_define_lookup;
    uint8_t mode;
#ifndef SKIPPLUGIN_LOGGER
    debug_logger *logger;
#endif
};

#endif // SMP_GROUP_H
