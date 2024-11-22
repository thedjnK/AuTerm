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

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "smp_transport.h"
#if defined(GUI_PRESENT)
#include "plugin_mcumgr.h"
#include "udp_setup.h"
#endif
#include <QUdpSocket>

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
struct smp_udp_config_t {
    QString hostname;
    uint16_t port;
};

/******************************************************************************/
// Class definitions
/******************************************************************************/
class smp_udp : public smp_transport
{
    Q_OBJECT

public:
    smp_udp(QObject *parent = nullptr);
    ~smp_udp();
    int connect(void) override;
    int disconnect(bool force) override;
#if defined(GUI_PRESENT)
    void open_connect_dialog() override;
    void close_connect_dialog() override;
#endif
    int is_connected() override;
    smp_transport_error_t send(smp_message *message) override;
    void setup_finished();
    int set_connection_config(struct smp_udp_config_t *configuration);

private slots:
    void connect_to_device(QString host, uint16_t port);
    void disconnect_from_device();
    void is_connected(bool *connected);
    void socket_connected();
    void socket_disconnected();
    void socket_about_to_close();
    void socket_bytes_written(qint64 written);
    void socket_error(QAbstractSocket::SocketError error);
    void socket_readyread();

private:
#if defined(GUI_PRESENT)
    udp_setup *udp_window;
    QMainWindow *main_window;
#endif
    struct smp_udp_config_t udp_config;
    bool udp_config_set;
    QUdpSocket *socket;
    bool socket_is_connected;
    smp_message received_data;
};

#endif // SMP_UDP_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
