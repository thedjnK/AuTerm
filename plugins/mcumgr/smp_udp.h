/******************************************************************************
** Copyright (C) 2021-2023 Jamie M.
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
    int is_connected() override;
    int send(smp_message *message);
    void close_connect_dialog();
    void setup_finished();
/*    int receive(QByteArray *data, uint16_t max_size) override;
    int receive_data_size() override;
    int get_mtu() override;
    QString get_error_string(int error_code) override;
    const struct setting_types_internal *items(uint16_t *items) override;
    int send_bytes_waiting(void);
    int has_data(void);*/

private slots:
    void connect_to_device(QString host, uint16_t port);
//    void socket_abouttoclose();
    void socket_readyread();
//    void socket_byteswritten(qint64 bytes);
//    void socket_connected();
//    void socket_disconnected();
//    void socket_statechanged(QAbstractSocket::SocketState state);
//    void socket_error(QAbstractSocket::SocketError error);
//    void socket_readchannelfinished();

signals:
//    void receive_waiting(smp_message *message);

private:
    udp_setup *udp_window;
    QMainWindow *main_window;
    QUdpSocket *socket;
    bool socket_is_connected;
//    QByteArray socket_received_data;
    smp_message received_data;

//    QString setting_host;
//    uint32_t setting_port;
//    uint32_t setting_mtu;
};

#endif // SMP_UDP_H
