/******************************************************************************
** Copyright (C) 2021-2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_bluetooth.cpp
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
#include "smp_bluetooth.h"
#include <QDebug>
#include <QBluetoothUuid>
#include <QLowEnergyConnectionParameters>
#include <QLowEnergyCharacteristic>
#include <QTimer>

#include "bluetooth_setup.h"

QList<QBluetoothDeviceInfo> bluetooth_device_list;
QList<QBluetoothUuid> services;

QLowEnergyService *bluetooth_service_mcumgr;
QLowEnergyCharacteristic bluetooth_characteristic_transmit;
uint16_t mtu;
uint16_t mtu_max_worked;
QByteArray sendbuffer;

bluetooth_setup *bluetooth_window;
QTimer retry_timer;
int retry_count;

smp_bluetooth::smp_bluetooth(QObject *parent)
{
    bluetooth_window = new bluetooth_setup(nullptr);

    QObject::connect(bluetooth_window, SIGNAL(refresh_devices()), this, SLOT(form_refresh_devices()));
    QObject::connect(bluetooth_window, SIGNAL(connect_to_device(uint16_t)), this, SLOT(form_connect_to_device(uint16_t)));
    QObject::connect(bluetooth_window, SIGNAL(disconnect_from_device()), this, SLOT(form_disconnect_from_device()));
    QObject::connect(bluetooth_window, SIGNAL(min_params()), this, SLOT(form_min_params()));

    discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    discoveryAgent->setLowEnergyDiscoveryTimeout(8000);
    QObject::connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    QObject::connect(discoveryAgent, SIGNAL(finished()), this, SLOT(finished()));
    device_connected = false;

    QObject::connect(&retry_timer, SIGNAL(timeout()), this, SLOT(timeout_timer()));
    retry_timer.setInterval(500);
    retry_timer.setSingleShot(true);
    mtu_max_worked = 0;
}
/*
    void error(QBluetoothDeviceDiscoveryAgent::Error error);
    void canceled();
 */

smp_bluetooth::~smp_bluetooth()
{
    delete bluetooth_window;
    QObject::disconnect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    QObject::disconnect(discoveryAgent, SIGNAL(finished()), this, SLOT(finished()));
    delete discoveryAgent;
    if (controller != nullptr)
    {
        QObject::disconnect(controller, SIGNAL(connected()), this, SLOT(connected()));
        QObject::disconnect(controller, SIGNAL(disconnected()), this, SLOT(disconnected()));
        QObject::disconnect(controller, SIGNAL(discoveryFinished()), this, SLOT(discovery_finished()));
        QObject::disconnect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(service_discovered(QBluetoothUuid)));
//        disconnect(controller, SIGNAL(connectionUpdated(QLowEnergyConnectionParameters)), this, SLOT(connection_updated(QLowEnergyConnectionParameters)));
        delete controller;
    }
    bluetooth_device_list.clear();
}

void smp_bluetooth::deviceDiscovered(const QBluetoothDeviceInfo &info)
{
    bluetooth_device_list.append(info);
//    ui->plainTextEdit->appendPlainText(QString("d: ").append(info.name()).append(", ").append(QString::number(info.rssi())).append(", ").append(info.address().toString()).append(", ").append(info.deviceUuid().toString()).append("\n"));
    QString device = QString(info.address().toString()).append(" ").append(info.name());
    bluetooth_window->add_device(&device);
}

void smp_bluetooth::deviceUpdated(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields)
{

}

void smp_bluetooth::finished()
{
    bluetooth_window->add_debug("finished");
    bluetooth_service_mcumgr->discoverDetails();
}

void smp_bluetooth::connected()
{
    bluetooth_window->add_debug("Connected!");
    controller->discoverServices();
    device_connected = true;
    mtu = 500;
    mtu_max_worked = 0;
}

void smp_bluetooth::disconnected()
{
    bluetooth_window->add_debug("Disconnected!");
    device_connected = false;
    mtu_max_worked = 0;
}

void smp_bluetooth::discovery_finished()
{
    bluetooth_window->add_debug("Finished!");
    bluetooth_service_mcumgr = controller->createServiceObject(QBluetoothUuid(QString("8D53DC1D-1DB7-4CD3-868B-8A527460AA84")));

    if (!bluetooth_service_mcumgr)
    {
        bluetooth_window->add_debug("SMP service not found.");
    }
    else
    {
        //Connect MCUmgr server signals
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_changed(QLowEnergyCharacteristic,QByteArray)));
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_written(QLowEnergyCharacteristic,QByteArray)));
//        connect(bluetooth_service_mcumgr, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(ServiceDescriptorWritten(QLowEnergyDescriptor,QByteArray)));
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(mcumgr_service_error(QLowEnergyService::ServiceError)));
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(mcumgr_service_state_changed(QLowEnergyService::ServiceState)));

        //Discover service details
        //bluetooth_service_mcumgr->discoverDetails();
    }
}

void smp_bluetooth::service_discovered(QBluetoothUuid service_uuid)
{
    services.append(service_uuid);
    bluetooth_window->add_debug(QString("Service! ").append(service_uuid.toString()));
}

void smp_bluetooth::mcumgr_service_characteristic_changed(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData)
{
    bluetooth_window->add_debug(QString("CHANGED!!"));

    received_data.append(&baData);

    if (received_data.is_valid() == true)
    {
        emit receive_waiting(&received_data);
        received_data.clear();
    }
    qDebug() << baData;
//    emit read(&baData);
}

void smp_bluetooth::mcumgr_service_characteristic_written(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData)
{
    bluetooth_window->add_debug(QString("WRITTEN!!"));
    qDebug() << baData;

    if (baData.length() > mtu_max_worked)
    {
        mtu_max_worked = baData.length();
    }

    retry_count = 0;

    if (sendbuffer.length() > 0)
    {
        sendbuffer.remove(0, baData.length());

        if (sendbuffer.length() > 0)
        {
            bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, sendbuffer.left(mtu));
            bluetooth_window->add_debug(QString("Writing ").append(QString::number(mtu)));
        }
    }
}

void smp_bluetooth::mcumgr_service_state_changed(QLowEnergyService::ServiceState nNewState)
{
    bluetooth_window->add_debug(QString("State: ").append(QString::number(nNewState)));

    //Service state changed
    if (nNewState == QLowEnergyService::ServiceDiscovered)
    {
        QLowEnergyService *svcBLEService = qobject_cast<QLowEnergyService *>(sender());
        if (svcBLEService && svcBLEService->serviceUuid() == QBluetoothUuid(QString("8D53DC1D-1DB7-4CD3-868B-8A527460AA84")))
        {
            bluetooth_characteristic_transmit = bluetooth_service_mcumgr->characteristic(QBluetoothUuid(QString("DA2E7828-FBCE-4E01-AE9E-261174997C48")));

            if (!bluetooth_characteristic_transmit.isValid())
            {
                //Missing Tx characteristic
//                if (bDisconnectActive == false)
//                {
//                    bDisconnectActive = true;
//                    lecBLEController->disconnectFromDevice();
//                }
                bluetooth_window->add_debug("TX not valid");
            }

            //Tx notifications descriptor
            const QLowEnergyDescriptor descTXDesc = bluetooth_characteristic_transmit.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

            if (!descTXDesc.isValid())
            {
                //Tx descriptor missing
//                if (bDisconnectActive == false)
//                {
//                    bDisconnectActive = true;
//                    lecBLEController->disconnectFromDevice();
//                }
                bluetooth_window->add_debug("tx DESC not valid");
            }

            //Enable Tx descriptor notifications
            bluetooth_service_mcumgr->writeDescriptor(descTXDesc, QByteArray::fromHex("0100"));
        }
    }
    else if (nNewState == QLowEnergyService::DiscoveryRequired)
    {
//        bluetooth_service_mcumgr->discoverDetails();
    }
}

void smp_bluetooth::errorz(QLowEnergyController::Error error)
{
    bluetooth_window->add_debug(QString::number(error));
}

int smp_bluetooth::is_connected()
{
    if (device_connected == true)
    {
        return 1;
    }

    return 0;
}

/*
void smp_bluetooth::connection_updated(QLowEnergyConnectionParameters parameters)
{
}
*/

int smp_bluetooth::send(smp_message *message)
{
    retry_count = 0;
    sendbuffer.clear();

    if (mtu < mtu_max_worked)
    {
        mtu = mtu_max_worked;
    }

    sendbuffer.append(*message->data());
    bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, sendbuffer.left(mtu));
    bluetooth_window->add_debug(QString("Writing ").append(QString::number(mtu)).append(sendbuffer.left(mtu)));

    return 0;
}

void smp_bluetooth::mcumgr_service_error(QLowEnergyService::ServiceError error)
{
    if (error == QLowEnergyService::CharacteristicWriteError)
    {
        qDebug() << "send failed with mtu " << mtu;

        ++retry_count;

        if (retry_count > 2)
        {
            retry_count = 0;

            if (mtu >= 25)
            {
                if (mtu >= 100)
                {
                    mtu -= 32;
                }
                else
                {
                    mtu -= 16;
                }

                bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, sendbuffer.left(mtu));
            }
            else
            {
                sendbuffer.clear();
                bluetooth_window->add_debug("bluetooth_device_listed");
            }
        }
        else
        {
            retry_timer.start();
        }
    }
}

//void smp_bluetooth::on_pushButton_9_clicked()
//{
//    ui->plainTextEdit->appendPlainText(QString::number(controller->mtu()));
//}

void smp_bluetooth::form_refresh_devices()
{
    bluetooth_window->clear_devices();
    bluetooth_device_list.clear();

    if (discoveryAgent->isActive())
    {
        discoveryAgent->stop();
    }

    discoveryAgent->start();
}

void smp_bluetooth::form_connect_to_device(uint16_t index)
{
    bluetooth_window->add_debug("burp");
    if (!controller) {
        bluetooth_window->add_debug("burp2");
            // Connecting signals and slots for connecting to LE services.
        //        QBluetoothAddress bluetooth_device_list = QBluetoothAddress(item->text().left(item->text().indexOf(" ")));
        controller = QLowEnergyController::createCentral(bluetooth_device_list.at(index));
        //        controller = QLowEnergyController::createCentral();
        QObject::connect(controller, SIGNAL(connected()), this, SLOT(connected()));
        QObject::connect(controller, SIGNAL(disconnected()), this, SLOT(disconnected()));
        QObject::connect(controller, SIGNAL(discoveryFinished()), this, SLOT(discovery_finished()));
        QObject::connect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(service_discovered(QBluetoothUuid)));
        QObject::connect(controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
        //         connect(controller, SIGNAL(connectionUpdated(QLowEnergyConnectionParameters)), this, SLOT(connection_updated(QLowEnergyConnectionParameters)));
    }

    //     if (isRandomAddress())
    //     {
    controller->setRemoteAddressType(QLowEnergyController::RandomAddress);
    //     }
    //     else
    //     {
    //         controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
    //     }
    controller->connectToDevice();
}

void smp_bluetooth::form_disconnect_from_device()
{
    controller->disconnectFromDevice();
}


int smp_bluetooth::connect()
{
    bluetooth_window->show();
    discoveryAgent->start();
    return 0;
}

int smp_bluetooth::disconnect(bool force)
{
    return 0;
}

void smp_bluetooth::timeout_timer()
{
    bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, sendbuffer.left(mtu));
}

void smp_bluetooth::form_min_params()
{
    QLowEnergyConnectionParameters params;
    params.setIntervalRange(7.5, 30);
    params.setLatency(0);
    params.setSupervisionTimeout(4000);

    controller->requestConnectionUpdate(params);
}
