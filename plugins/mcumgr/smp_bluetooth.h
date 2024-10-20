/******************************************************************************
** Copyright (C) 2021-2023 Jamie M.
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

#include <QObject>
#include "plugin_mcumgr.h"
#include "smp_transport.h"
#include "smp_message.h"

#include <qbluetoothaddress.h>
#include <qbluetoothdevicediscoveryagent.h>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothdeviceinfo.h>
#include <qbluetoothservicediscoveryagent.h>
#include <QLowEnergyController>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QListWidgetItem>

class smp_bluetooth : public smp_transport
{
    Q_OBJECT

public:
    smp_bluetooth(QObject *parent = nullptr);
    ~smp_bluetooth();
    int connect(void) override;
    int disconnect(bool force) override;
    void open_connect_dialog();
    int is_connected() override;
    int send(smp_message *message);
    void close_connect_dialog();

private slots:
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void deviceUpdated(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields);
    void finished();
    void connected();
    void disconnected();
    void discovery_finished();
    void service_discovered(QBluetoothUuid service_uuid);
    void mcumgr_service_characteristic_changed(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData);
    void mcumgr_service_characteristic_written(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData);
    void mcumgr_service_error(QLowEnergyService::ServiceError error);
    void mcumgr_service_state_changed(QLowEnergyService::ServiceState nNewState);
    void errorz(QLowEnergyController::Error error);

    void timeout_timer();
    void discover_timer_timeout();

    void form_refresh_devices();
    void form_connect_to_device(uint16_t index);
    void form_disconnect_from_device();
    void form_bluetooth_status(bool *scanning, bool *connecting);
    void form_debug_state_changed(bool enabled);
//    void connection_updated(QLowEnergyConnectionParameters parameters);

signals:
//    void read(QByteArray *message);

private:
    void form_min_params();

//    Ui::bluetooth *ui;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent = nullptr;
//    DeviceInfo currentDevice;
    QLowEnergyController *controller = nullptr;
    bool device_connected;
    smp_message received_data;
};

#endif // SMP_BLUETOOTH_H
