/******************************************************************************
** Copyright (C) 2021-2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_transport.h
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
#ifndef SMP_TRANSPORT_H
#define SMP_TRANSPORT_H

#include <QObject>
#include "smp_message.h"
#include "debug_logger.h"
//#include <QAbstractSocket>
//#include <smp_settings.h>

#define DEFAULT_TRANSPORT_RETRIES 3
#define DEFAULT_TRANSPORT_TIMEOUT_MS 3000

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum smp_transport_error_t {
    SMP_TRANSPORT_ERROR_OK = 0,
    SMP_TRANSPORT_ERROR_UNSUPPORTED = -1,
    SMP_TRANSPORT_ERROR_NOT_CONNECTED = -2,
    SMP_TRANSPORT_ERROR_ALREADY_CONNECTED = -3,
    SMP_TRANSPORT_ERROR_NO_DATA = -4,
    SMP_TRANSPORT_ERROR_PROCESSOR_BUSY = -5,
    SMP_TRANSPORT_ERROR_INVALID_CONFIGURATION = -6,
    SMP_TRANSPORT_ERROR_OPEN_FAILED = -7,
};


class smp_transport : public QObject
{
    Q_OBJECT

public:
    virtual int connect(void)
    {
        return SMP_TRANSPORT_ERROR_UNSUPPORTED;
    }

    virtual int disconnect(bool force)
    {
        Q_UNUSED(force);
        return SMP_TRANSPORT_ERROR_UNSUPPORTED;
    }

#ifndef SKIPPLUGIN_LOGGER
    void set_logger(debug_logger *object)
    {
        logger = object;
    }
#endif

    virtual int is_connected(void)
    {
        return SMP_TRANSPORT_ERROR_UNSUPPORTED;
    }
    virtual smp_transport_error_t send(smp_message *message) = 0;
//    virtual int receive(QByteArray *data, uint16_t max_size) = 0;

#if defined(GUI_PRESENT)
    virtual void open_connect_dialog()
    {
    }

    virtual void close_connect_dialog()
    {
    }
#endif

    virtual uint16_t max_message_data_size(uint16_t mtu)
    {
        return mtu;
    }

    virtual uint8_t get_retries()
    {
        return DEFAULT_TRANSPORT_RETRIES;
    }

    virtual uint32_t get_timeout()
    {
        return DEFAULT_TRANSPORT_TIMEOUT_MS;
    }

    virtual QString to_error_string(int error_code)
    {
        Q_UNUSED(error_code);
        return "";
    }

signals:
    void connected();
    void disconnected();
    void error(int error_code);
    void bytes_written(qint64 bytes);
    void receive_waiting(smp_message *message);

protected:
#ifndef SKIPPLUGIN_LOGGER
    debug_logger *logger;
#endif
};

#endif // SMP_TRANSPORT_H
