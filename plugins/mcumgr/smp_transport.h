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
//#include <QAbstractSocket>
//#include <smp_settings.h>

enum {
    SMP_TRANSPORT_ERROR_OK = 0,
    SMP_TRANSPORT_ERROR_UNSUPPORTED = -1,
    SMP_TRANSPORT_ERROR_NOT_CONNECTED = -2,
    SMP_TRANSPORT_ERROR_ALREADY_CONNECTED = -3,
    SMP_TRANSPORT_ERROR_NO_DATA = -4,
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

    virtual int is_connected(void)
    {
        return SMP_TRANSPORT_ERROR_UNSUPPORTED;
    }
    virtual int send(smp_message *message) = 0;
//    virtual int receive(QByteArray *data, uint16_t max_size) = 0;

    virtual void close_connect_dialog()
    {
    }

    virtual uint16_t max_message_data_size(uint16_t mtu)
    {
        return mtu;
    }

signals:
//    void connected();
//    void disconnected();
//    void error(int error_code);
//    void send_complete();
//    void receive_waiting();
//    void device_found();
//    void device_scan_finished();
//    void device_scan_error();
//    void bytes_written(qint64 bytes);
    void receive_waiting(smp_message *message);
};

#endif // SMP_TRANSPORT_H
