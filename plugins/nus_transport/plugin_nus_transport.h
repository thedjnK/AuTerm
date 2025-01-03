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
// Enum typedefs
/******************************************************************************/
enum NUS_TRANSPORT_ERRORS {
    NUS_TRANSPORT_ERROR_NONE,
    NUS_TRANSPORT_ERROR_MISSING_NUS_SERVICE,
    NUS_TRANSPORT_ERROR_MISSING_TX_CHARACTERISTIC,
    NUS_TRANSPORT_ERROR_MISSING_RX_CHARACTERISTIC,
    NUS_TRANSPORT_ERROR_MISSING_RX_DESCRIPTOR,
    NUS_TRANSPORT_ERROR_MINIMAL_MTU_WRITE_FAILED,
    NUS_TRANSPORT_ERROR_UNEXPECTED_DISCONNECT,
    NUS_TRANSPORT_ERROR_CONTROLLER_UNKNOWN,
    NUS_TRANSPORT_ERROR_CONTROLLER_UNKNOWN_REMOTE_DEVICE,
    NUS_TRANSPORT_ERROR_CONTROLLER_NETWORK,
    NUS_TRANSPORT_ERROR_CONTROLLER_INVALID_BLUETOOTH_ADAPTER,
    NUS_TRANSPORT_ERROR_CONTROLLER_CONNECTION,
    NUS_TRANSPORT_ERROR_CONTROLLER_ADVERTISING,
    NUS_TRANSPORT_ERROR_CONTROLLER_REMOTE_HOST_CLOSED,
    NUS_TRANSPORT_ERROR_CONTROLLER_AUTHORISATION,
    NUS_TRANSPORT_ERROR_CONTROLLER_NOT_DEFINED,
    NUS_TRANSPORT_ERROR_SERVICE_CHARACTERISTIC_READ,
    NUS_TRANSPORT_ERROR_SERVICE_CHARACTERISTIC_WRITE,
    NUS_TRANSPORT_ERROR_SERVICE_DESCRIPTOR_READ,
    NUS_TRANSPORT_ERROR_SERVICE_DESCRIPTOR_WRITE,
    NUS_TRANSPORT_ERROR_SERVICE_OPERATION,
    NUS_TRANSPORT_ERROR_SERVICE_UNKNOWN,
    NUS_TRANSPORT_ERROR_SERVICE_NOT_DEFINED,

    NUS_TRANSPORT_ERROR_COUNT
};

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
    void setup(QMainWindow *main_window) override;
    void transport_setup(QWidget *tab) override;
    const QString plugin_about() override;
    bool plugin_configuration() override;
    static QMainWindow *get_main_window();
    void setup_finished() override;
    PluginType plugin_type() override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool open(QIODeviceBase::OpenMode mode) override;
#else
    bool open(QIODevice::OpenMode mode) override;
#endif
    void close() override;
    bool isOpen() const override;
    bool isOpening() const override;
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
    QObject *plugin_object() override;
    QString connection_display_name() override;

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
#if !(QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
    void discover_timer_timeout();
#endif
    void form_refresh_devices();
    void form_connect_to_device(uint16_t index, uint8_t address_type);
    void form_disconnect_from_device();
    void form_bluetooth_status(bool *scanning, bool *connecting);
    void form_request_connect();
    void connection_updated(QLowEnergyConnectionParameters parameters);
    void stateChanged(QLowEnergyController::ControllerState state);

signals:
    void readyRead();
    void errorOccurred(int error);
    void bytesWritten(qint64 bytes);
    void aboutToClose();
    void update_images();
    void transport_error(int error);
    void transport_open_close(uint8_t mode);

private:
    void form_min_params();

    QMainWindow *parent_window;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent = nullptr;
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
    bool disconnecting_from_device;
    bool ready_to_send;
    bool bluetooth_write_with_response;
    QString connected_device_name;
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
