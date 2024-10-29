/******************************************************************************
** Copyright (C) 2021-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_udp.cpp
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
#include "smp_udp.h"
#include <QNetworkDatagram>
#include <QInputDialog>

smp_udp::smp_udp(QObject *parent)
{
    Q_UNUSED(parent);

    main_window = plugin_mcumgr::get_main_window();
    udp_window = new udp_setup(main_window);

    socket = new QUdpSocket(this);
    socket_is_connected = false;

    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(socket_readyread()));

    QObject::connect(udp_window, SIGNAL(connect_to_device(QString,uint16_t)), this, SLOT(connect_to_device(QString,uint16_t)));
    QObject::connect(udp_window, SIGNAL(plugin_save_setting(QString,QVariant)), main_window, SLOT(plugin_save_setting(QString,QVariant)));
    QObject::connect(udp_window, SIGNAL(plugin_load_setting(QString,QVariant*,bool*)), main_window, SLOT(plugin_load_setting(QString,QVariant*,bool*)));
}

smp_udp::~smp_udp()
{
    QObject::disconnect(this, SLOT(socket_readyread()));

    QObject::disconnect(this, SLOT(connect_to_device(QString,uint16_t)));
    QObject::disconnect(udp_window, SIGNAL(plugin_save_setting(QString,QVariant)), main_window, SLOT(plugin_save_setting(QString,QVariant)));
    QObject::disconnect(udp_window, SIGNAL(plugin_load_setting(QString,QVariant*,bool*)), main_window, SLOT(plugin_load_setting(QString,QVariant*,bool*)));

    if (socket_is_connected == true)
    {
        socket->disconnectFromHost();
        socket_is_connected = false;
    }

    if (udp_window->isVisible())
    {
        udp_window->close();
    }

    delete udp_window;
    delete socket;
}

int smp_udp::connect(void)
{
    if (socket_is_connected == true)
    {
        return SMP_TRANSPORT_ERROR_ALREADY_CONNECTED;
    }

    //TODO

    return SMP_TRANSPORT_ERROR_OK;
}

int smp_udp::disconnect(bool force)
{
    if (socket_is_connected == false)
    {
        return SMP_TRANSPORT_ERROR_NOT_CONNECTED;
    }

    socket->disconnectFromHost();
    socket_is_connected = false;
//    socket_received_data.clear();
    received_data.clear();

    return SMP_TRANSPORT_ERROR_OK;
}

void smp_udp::open_connect_dialog()
{
    udp_window->show();
}

int smp_udp::is_connected()
{
    if (socket_is_connected == true)
    {
        return 1;
    }

    return 0;
}

int smp_udp::send(smp_message *message)
{
    if (socket_is_connected == false)
    {
        return SMP_TRANSPORT_ERROR_NOT_CONNECTED;
    }

    socket->write(*message->data());

    return SMP_TRANSPORT_ERROR_OK;
}

void smp_udp::socket_readyread()
{
    while (socket->hasPendingDatagrams())
    {
        QNetworkDatagram datagram = socket->receiveDatagram();
        received_data.append(datagram.data());

        //Check if there is a full packet
        if (received_data.is_valid() == true)
        {
            emit receive_waiting(&received_data);
            received_data.clear();
        }
    }
}

void smp_udp::connect_to_device(QString host, uint16_t port)
{
    socket->connectToHost(host, port);
    socket_is_connected = true;
    //TODO: need to alert parent
}

void smp_udp::close_connect_dialog()
{
    if (udp_window->isVisible())
    {
        udp_window->close();
    }
}

void smp_udp::setup_finished()
{
    udp_window->load_settings();
}
