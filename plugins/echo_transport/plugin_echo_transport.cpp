/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module:  plugin_echo_transport.cpp
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
#include "plugin_echo_transport.h"
#include <QLabel>
#include <QVBoxLayout>

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
void plugin_echo_transport::setup(QMainWindow *main_window)
{
    parent_window = main_window;
}

void plugin_echo_transport::transport_setup(QWidget *tab)
{
    QVBoxLayout *vertical_layout = new QVBoxLayout(tab);
    QLabel *plugin_text = new QLabel(tab);

    vertical_layout->setSpacing(2);
    vertical_layout->setContentsMargins(6, 6, 6, 6);
    vertical_layout->addWidget(plugin_text);
    plugin_text->setText("Echo transport has no configuration");
}

const QString plugin_echo_transport::plugin_about()
{
    return "Dummy echo transport plugin";
}

bool plugin_echo_transport::plugin_configuration()
{
    return false;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool plugin_echo_transport::open(QIODeviceBase::OpenMode mode)
#else
bool plugin_echo_transport::open(QIODevice::OpenMode mode)
#endif
{
    device_connected = true;
    return true;
}

void plugin_echo_transport::close()
{
    send_buffer.clear();
    device_connected = false;
    emit aboutToClose();
}

bool plugin_echo_transport::isOpen() const
{
    return device_connected;
}

QSerialPort::DataBits plugin_echo_transport::dataBits() const
{
    return QSerialPort::Data8;
}

AutTransportPlugin::StopBits plugin_echo_transport::stopBits() const
{
    return NoStop;
}

QSerialPort::Parity plugin_echo_transport::parity() const
{
    return QSerialPort::NoParity;
}

qint64 plugin_echo_transport::write(const QByteArray &data)
{
    send_buffer.append(data);
    emit bytesWritten(data.length());
    emit readyRead();
    return 0;
}

qint64 plugin_echo_transport::bytesAvailable() const
{
    return send_buffer.length();
}

QByteArray plugin_echo_transport::peek(qint64 maxlen)
{
    return send_buffer.left(maxlen);
}

QByteArray plugin_echo_transport::read(qint64 maxlen)
{
    QByteArray data = send_buffer.left(maxlen);

    send_buffer.remove(0, maxlen);
    return data;
}

QByteArray plugin_echo_transport::readAll()
{
    QByteArray data = send_buffer;

    send_buffer.clear();
    return data;
}

bool plugin_echo_transport::clear(QSerialPort::Directions directions)
{
    if ((directions & QSerialPort::Input) != 0)
    {
        send_buffer.clear();
    }
    return true;
}

QSerialPort::PinoutSignals plugin_echo_transport::pinoutSignals()
{
    return QSerialPort::ClearToSendSignal;
}

QString plugin_echo_transport::to_error_string(int error)
{
    return 0;
}

QString plugin_echo_transport::transport_name() const
{
    return "Echo";
}

AutPlugin::PluginType plugin_echo_transport::plugin_type()
{
    return AutPlugin::Transport;
}

QObject *plugin_echo_transport::plugin_object()
{
    return this;
}

QString plugin_echo_transport::connection_display_name()
{
    return "Echo loopback";
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
