/******************************************************************************
** Copyright (C) 2021-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_bluetooth.h
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
#ifndef SMP_BLUETOOTH_H
#define SMP_BLUETOOTH_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QTimer>
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
#include "smp_transport.h"
#include "smp_message.h"
#if defined(GUI_PRESENT)
#include "plugin_mcumgr.h"
#endif

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum BLUETOOTH_FORCE_ADDRESS {
    BLUETOOTH_FORCE_ADDRESS_DEFAULT,
    BLUETOOTH_FORCE_ADDRESS_RANDOM,
    BLUETOOTH_FORCE_ADDRESS_PUBLIC,

    BLUETOOTH_FORCE_ADDRESS_COUNT
};

enum smp_bluetooth_connect_type_t {
    SMP_BLUETOOTH_CONNECT_TYPE_ADDRESS,
    SMP_BLUETOOTH_CONNECT_TYPE_NAME,

    SMP_BLUETOOTH_CONNECT_TYPE_COUNT
};

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
struct smp_bluetooth_config_t {
    //union {
        QString address;
        QString name;
    //};
    enum smp_bluetooth_connect_type_t type;
};

class bluetooth_setup;

/******************************************************************************/
// Class definitions
/******************************************************************************/
class smp_bluetooth : public smp_transport
{
    Q_OBJECT

public:
    smp_bluetooth(QObject *parent = nullptr);
    ~smp_bluetooth();
    int connect(void) override;
    int disconnect(bool force) override;
#if defined(GUI_PRESENT)
    void open_connect_dialog() override;
    void close_connect_dialog() override;
#endif
    int is_connected() override;
    int set_connection_config(struct smp_bluetooth_config_t *configuration);
    smp_transport_error_t send(smp_message *message) override;
    void setup_finished();
    QString to_error_string(int error_code) override;

private slots:
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void deviceUpdated(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields);
    void finished();
    void bluetooth_connected();
    void bluetooth_disconnected();
    void discovery_finished();
    void service_discovered(QBluetoothUuid service_uuid);
    void mcumgr_service_characteristic_changed(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData);
    void mcumgr_service_characteristic_written(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData);
    void mcumgr_service_descriptor_written(const QLowEnergyDescriptor info, const QByteArray value);
    void mcumgr_service_error(QLowEnergyService::ServiceError error);
    void mcumgr_service_state_changed(QLowEnergyService::ServiceState nNewState);
    void errorz(QLowEnergyController::Error error);
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    void mtu_updated(int mtu);
#endif
#if !(QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
    void discover_timer_timeout();
#endif

    void form_refresh_devices();
    void form_connect_to_device(uint16_t index, uint8_t address_type, bool write_with_response);
    void form_disconnect_from_device();
    void form_bluetooth_status(bool *scanning, bool *connecting);
//    void connection_updated(QLowEnergyConnectionParameters parameters);

signals:
//    void read(QByteArray *message);

private:
    void form_min_params();

#if defined(GUI_PRESENT)
    QMainWindow *main_window;
    bluetooth_setup *bluetooth_window;
#endif
    struct smp_bluetooth_config_t bluetooth_config;
    bool bluetooth_config_set;
    bool bluetooth_config_connection_in_progress;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent = nullptr;
//    DeviceInfo currentDevice;
    QLowEnergyController *controller = nullptr;
    bool device_connected;
    smp_message received_data;
    QList<QBluetoothDeviceInfo> bluetooth_device_list;
    QList<QBluetoothUuid> services;
    QLowEnergyService *bluetooth_service_mcumgr;
    QLowEnergyCharacteristic bluetooth_characteristic_transmit;
    QLowEnergyDescriptor bluetooth_descriptor_receive_cccd;
    uint16_t mtu;
    uint16_t mtu_max_worked;
    QByteArray send_buffer;
    int retry_count;
    QLowEnergyService::WriteMode bluetooth_write_mode;
#if !(QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
    QTimer discover_timer;
#endif
    bool ready_to_send;
};

#endif // SMP_BLUETOOTH_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
