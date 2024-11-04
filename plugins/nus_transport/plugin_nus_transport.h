/******************************************************************************
** Copyright (C) 2021-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  plugin_nus_transport.h
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
#ifndef PLUGIN_NUS_TRANSPORT_H
#define PLUGIN_NUS_TRANSPORT_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QTimer>
#include <QListWidgetItem>
#include <QBluetoothUuid>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyConnectionParameters>
#include <QLowEnergyCharacteristic>
#include <qbluetoothaddress.h>
#include <qbluetoothdevicediscoveryagent.h>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothdeviceinfo.h>
#include <qbluetoothservicediscoveryagent.h>
#include "AutPlugin.h"
#include "nus_bluetooth_setup.h"

/******************************************************************************/
// Class definitions
/******************************************************************************/
class plugin_nus_transport : public QObject, public AutTransportPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AuTermPluginInterface_iid FILE "plugin_nus_transport.json")
    Q_INTERFACES(AutTransportPlugin)

public:
    plugin_nus_transport();
    ~plugin_nus_transport();
    QWidget *GetWidget();
    void setup(QMainWindow *main_window);
    void transport_setup(QWidget *tab);
    const QString plugin_about();
    bool plugin_configuration();
    static QMainWindow *get_main_window();
    void setup_finished();
    PluginType plugin_type();
    bool open(QIODeviceBase::OpenMode mode) override;
    void close();
    bool isOpen() const;
    bool isOpening() const;
    QSerialPort::DataBits dataBits() const;
    StopBits stopBits() const;
    QSerialPort::Parity parity() const;
    qint64 write(const QByteArray &data);
    qint64 bytesAvailable() const;
    QByteArray peek(qint64 maxlen);
    QByteArray read(qint64 maxlen);
    QByteArray readAll();
    bool clear(QSerialPort::Directions directions = QSerialPort::AllDirections);
    QSerialPort::PinoutSignals pinoutSignals();
    QString to_error_string(int error);
    QString transport_name() const;
    QObject *plugin_object();

private slots:
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void deviceUpdated(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields);
    void finished();
    void connected();
    void disconnected();
    void discovery_finished();
    void service_discovered(QBluetoothUuid service_uuid);
    void nus_service_characteristic_changed(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData);
    void nus_service_characteristic_written(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData);
    void nus_service_descriptor_written(const QLowEnergyDescriptor info, const QByteArray value);
    void nus_service_error(QLowEnergyService::ServiceError error);
    void nus_service_state_changed(QLowEnergyService::ServiceState nNewState);
    void errorz(QLowEnergyController::Error error);
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    void mtu_updated(int mtu);
#endif
    void timeout_timer();
#if !(QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
    void discover_timer_timeout();
#endif
    void form_refresh_devices();
    void form_connect_to_device(uint16_t index, uint8_t address_type);
    void form_disconnect_from_device();
    void form_bluetooth_status(bool *scanning, bool *connecting);
//    void connection_updated(QLowEnergyConnectionParameters parameters);

signals:
    void readyRead();
    void errorOccurred(int error);
    void bytesWritten(qint64 bytes);
    void aboutToClose();
    void update_images();
    void transport_error(int error);

private:
    void form_min_params();

    QMainWindow *parent_window;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent = nullptr;
//    DeviceInfo currentDevice;
    QLowEnergyController *controller = nullptr;
    bool device_connected;
    QByteArray received_data;
    QList<QBluetoothDeviceInfo> bluetooth_device_list;
    QList<QBluetoothUuid> services;
    QLowEnergyService *bluetooth_service_nus;
    QLowEnergyCharacteristic bluetooth_characteristic_transmit;
    QLowEnergyCharacteristic bluetooth_characteristic_receive;
    QLowEnergyDescriptor bluetooth_descriptor_receive_cccd;
    uint16_t mtu;
    uint16_t mtu_max_worked;
    QByteArray send_buffer;
    nus_bluetooth_setup *bluetooth_window;
    QTimer retry_timer;
    int retry_count;
    bool disconnecting_from_device;
    bool ready_to_send;
#if !(QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
    QTimer discover_timer;
#endif
#ifndef SKIPPLUGIN_LOGGER
    debug_logger *logger;
#endif
};

#endif // PLUGIN_NUS_TRANSPORT_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
