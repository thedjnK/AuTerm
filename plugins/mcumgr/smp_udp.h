/******************************************************************************
** Copyright (C) 2021-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_udp.h
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
#ifndef SMP_UDP_H
#define SMP_UDP_H

#include "plugin_mcumgr.h"
#include "smp_transport.h"
#include "udp_setup.h"
#include <QUdpSocket>

class smp_udp : public smp_transport
{
    Q_OBJECT

public:
    smp_udp(QObject *parent = nullptr);
    ~smp_udp();
    int connect(void) override;
    int disconnect(bool force) override;
    void open_connect_dialog();
    int is_connected() override;
    int send(smp_message *message);
    void close_connect_dialog();
    void setup_finished();

private slots:
    void connect_to_device(QString host, uint16_t port);
    void socket_readyread();

private:
    udp_setup *udp_window;
    QMainWindow *main_window;
    QUdpSocket *socket;
    bool socket_is_connected;
    smp_message received_data;

//    QString setting_host;
//    uint32_t setting_port;
//    uint32_t setting_mtu;
};

#endif // SMP_UDP_H
