/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module:  plugin_echo_transport.h
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
#ifndef PLUGIN_ECHO_TRANSPORT_H
#define PLUGIN_ECHO_TRANSPORT_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include "AutPlugin.h"

/******************************************************************************/
// Class definitions
/******************************************************************************/
class plugin_echo_transport : public QObject, public AutTransportPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AuTermPluginInterface_iid FILE "plugin_echo_transport.json")
    Q_INTERFACES(AutTransportPlugin)

public:
    void setup(QMainWindow *main_window) override;
    void transport_setup(QWidget *tab) override;
    const QString plugin_about() override;
    bool plugin_configuration() override;
    PluginType plugin_type() override;
    QObject *plugin_object() override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool open(QIODeviceBase::OpenMode mode) override;
#else
    bool open(QIODevice::OpenMode mode) override;
#endif
    void close() override;
    bool isOpen() const override;
    QSerialPort::DataBits dataBits() const override;
    StopBits stopBits() const override;
    QSerialPort::Parity parity() const override;
    qint64 write(const QByteArray &data) override;
    qint64 bytesAvailable() const override;
    QByteArray peek(qint64 maxlen) override;
    QByteArray read(qint64 maxlen) override;
    QByteArray readAll() override;
    bool clear(QSerialPort::Directions directions = QSerialPort::AllDirections) override;
    QSerialPort::PinoutSignals pinoutSignals() override;
    QString to_error_string(int error) override;
    QString transport_name() const override;
    QString connection_display_name() override;

signals:
    void readyRead();
    void errorOccurred(int error);
    void bytesWritten(qint64 bytes);
    void aboutToClose();

private:
    QMainWindow *parent_window;
    bool device_connected;
    QByteArray send_buffer;
};

#endif // PLUGIN_ECHO_TRANSPORT_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
