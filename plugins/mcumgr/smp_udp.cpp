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

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "smp_udp.h"
#include <QNetworkDatagram>

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
smp_udp::smp_udp(QObject *parent)
{
    Q_UNUSED(parent);

#if defined(GUI_PRESENT)
    main_window = plugin_mcumgr::get_main_window();
    udp_window = new udp_setup(main_window);

    QObject::connect(udp_window, SIGNAL(connect_to_device(QString,uint16_t)), this, SLOT(connect_to_device(QString,uint16_t)));
    QObject::connect(udp_window, SIGNAL(disconnect_from_device()), this, SLOT(disconnect_from_device()));
    QObject::connect(udp_window, SIGNAL(is_connected(bool*)), this, SLOT(is_connected(bool*)));
    QObject::connect(udp_window, SIGNAL(plugin_save_setting(QString,QVariant)), main_window, SLOT(plugin_save_setting(QString,QVariant)));
    QObject::connect(udp_window, SIGNAL(plugin_load_setting(QString,QVariant*,bool*)), main_window, SLOT(plugin_load_setting(QString,QVariant*,bool*)));
    QObject::connect(udp_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), main_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));
#endif

    socket = new QUdpSocket(this);
    socket_is_connected = false;
    udp_config_set = false;

    QObject::connect(socket, SIGNAL(connected()), this, SLOT(socket_connected()));
    QObject::connect(socket, SIGNAL(disconnected()), this, SLOT(socket_disconnected()));
    QObject::connect(socket, SIGNAL(aboutToClose()), this, SLOT(socket_about_to_close()));
    QObject::connect(socket, SIGNAL(bytesWritten(qint64)), this, SLOT(socket_bytes_written(qint64)));
    QObject::connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(socket_error(QAbstractSocket::SocketError)));
    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(socket_readyread()));
}

smp_udp::~smp_udp()
{
    QObject::disconnect(this, SLOT(socket_connected()));
    QObject::disconnect(this, SLOT(socket_disconnected()));
    QObject::disconnect(this, SLOT(socket_readyread()));

#if defined(GUI_PRESENT)
    QObject::disconnect(this, SLOT(is_connected(bool*)));
    QObject::disconnect(this, SLOT(connect_to_device(QString,uint16_t)));
    QObject::disconnect(this, SLOT(disconnect_from_device()));
    QObject::disconnect(udp_window, SIGNAL(plugin_save_setting(QString,QVariant)), main_window, SLOT(plugin_save_setting(QString,QVariant)));
    QObject::disconnect(udp_window, SIGNAL(plugin_load_setting(QString,QVariant*,bool*)), main_window, SLOT(plugin_load_setting(QString,QVariant*,bool*)));
    QObject::disconnect(udp_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), main_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));
#endif

    if (socket_is_connected == true)
    {
        socket->disconnectFromHost();
        socket_is_connected = false;
    }

#if defined(GUI_PRESENT)
    if (udp_window->isVisible())
    {
        udp_window->close();
    }

    delete udp_window;
#endif
    delete socket;
}

int smp_udp::connect(void)
{
    if (socket_is_connected == true)
    {
        return SMP_TRANSPORT_ERROR_ALREADY_CONNECTED;
    }

    if (udp_config_set == false)
    {
        return SMP_TRANSPORT_ERROR_INVALID_CONFIGURATION;
    }

    connect_to_device(udp_config.hostname, udp_config.port);

    return SMP_TRANSPORT_ERROR_OK;
}

int smp_udp::disconnect(bool force)
{
    Q_UNUSED(force);

    if (socket_is_connected == false)
    {
        return SMP_TRANSPORT_ERROR_NOT_CONNECTED;
    }

    socket->disconnectFromHost();
    socket_is_connected = false;
    received_data.clear();

    return SMP_TRANSPORT_ERROR_OK;
}

#if defined(GUI_PRESENT)
void smp_udp::open_connect_dialog()
{
    udp_window->show();
}

void smp_udp::close_connect_dialog()
{
    if (udp_window->isVisible())
    {
        udp_window->close();
    }
}
#endif

int smp_udp::is_connected()
{
    if (socket_is_connected == true)
    {
        return 1;
    }

    return 0;
}

smp_transport_error_t smp_udp::send(smp_message *message)
{
    if (socket_is_connected == false)
    {
        return SMP_TRANSPORT_ERROR_NOT_CONNECTED;
    }

    socket->write(*message->data());

    return SMP_TRANSPORT_ERROR_OK;
}

void smp_udp::socket_connected()
{
    emit connected();
}

void smp_udp::socket_disconnected()
{
    emit disconnected();
}

void smp_udp::socket_about_to_close()
{
}

void smp_udp::socket_bytes_written(qint64 written)
{
//    qDebug() << "written: " << written;
}

void smp_udp::socket_error(QAbstractSocket::SocketError error)
{
//    qDebug() << "error: " << error;
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

void smp_udp::setup_finished()
{
#if defined(GUI_PRESENT)
#ifndef SKIPPLUGIN_LOGGER
    udp_window->set_logger(logger);
#endif
    udp_window->load_settings();
    udp_window->load_pixmaps();
#endif
}

void smp_udp::disconnect_from_device()
{
    disconnect(false);
}

void smp_udp::is_connected(bool *connected)
{
    *connected = socket_is_connected;
}

int smp_udp::set_connection_config(struct smp_udp_config_t *configuration)
{
    if (socket_is_connected == true)
    {
        return SMP_TRANSPORT_ERROR_ALREADY_CONNECTED;
    }

    udp_config.hostname = configuration->hostname;
    udp_config.port = configuration->port;
    udp_config_set = true;

    return SMP_TRANSPORT_ERROR_OK;
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
