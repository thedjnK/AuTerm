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
#include "bluetooth_setup.h"
#include <QDebug>
#include <QBluetoothUuid>
#include <QLowEnergyConnectionParameters>
#include <QLowEnergyCharacteristic>
#include <QTimer>

//Aim for a connection interval of between 7.5us-30us with a 4 second supervision timeout
static const double connection_interval_min = 7.5;
static const double connection_interval_max = 30;
static const int connection_latency = 0;
static const int connection_supervision_timeout = 4000;

//Default MTU of 490 - less than 512 maximum with a bit of safety
static const int default_mtu = 490;

static QList<QBluetoothDeviceInfo> bluetooth_device_list;
static QList<QBluetoothUuid> services;

static QLowEnergyService *bluetooth_service_mcumgr = nullptr;
static QLowEnergyCharacteristic bluetooth_characteristic_transmit;
static uint16_t mtu;
static uint16_t mtu_max_worked;
static QByteArray sendbuffer;

static bluetooth_setup *bluetooth_window;
static QTimer retry_timer;
static QTimer discover_timer;
static int retry_count;

smp_bluetooth::smp_bluetooth(QObject *parent)
{
    Q_UNUSED(parent);

    bluetooth_window = new bluetooth_setup(plugin_mcumgr::get_main_window());

    QObject::connect(bluetooth_window, SIGNAL(refresh_devices()), this, SLOT(form_refresh_devices()));
    QObject::connect(bluetooth_window, SIGNAL(connect_to_device(uint16_t)), this, SLOT(form_connect_to_device(uint16_t)));
    QObject::connect(bluetooth_window, SIGNAL(disconnect_from_device()), this, SLOT(form_disconnect_from_device()));
    QObject::connect(bluetooth_window, SIGNAL(bluetooth_status(bool*,bool*)), this, SLOT(form_bluetooth_status(bool*,bool*)));

    discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    discoveryAgent->setLowEnergyDiscoveryTimeout(8000);
    QObject::connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    QObject::connect(discoveryAgent, SIGNAL(finished()), this, SLOT(finished()));
    device_connected = false;

    QObject::connect(&retry_timer, SIGNAL(timeout()), this, SLOT(timeout_timer()));
    retry_timer.setInterval(500);
    retry_timer.setSingleShot(true);

    QObject::connect(&discover_timer, SIGNAL(timeout()), this, SLOT(discover_timer_timeout()));
    discover_timer.setInterval(50);
    discover_timer.setSingleShot(true);

    mtu_max_worked = 0;
}
/*
    void error(QBluetoothDeviceDiscoveryAgent::Error error);
    void canceled();
 */

smp_bluetooth::~smp_bluetooth()
{
    delete bluetooth_window;

    if (discoveryAgent != nullptr)
    {
        QObject::disconnect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
        QObject::disconnect(discoveryAgent, SIGNAL(finished()), this, SLOT(finished()));
        delete discoveryAgent;
    }

    if (bluetooth_service_mcumgr != nullptr)
    {
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_changed(QLowEnergyCharacteristic,QByteArray)));
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_written(QLowEnergyCharacteristic,QByteArray)));
        //        connect(bluetooth_service_mcumgr, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(ServiceDescriptorWritten(QLowEnergyDescriptor,QByteArray)));
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(mcumgr_service_error(QLowEnergyService::ServiceError)));
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(mcumgr_service_state_changed(QLowEnergyService::ServiceState)));
        delete bluetooth_service_mcumgr;
    }

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
    log_information() << "Bluetooth discovery finished";
    bluetooth_window->discovery_state(false);
}

void smp_bluetooth::connected()
{
    log_information() << "Connected to Bluetooth device";
    controller->discoverServices();
    device_connected = true;
    mtu = default_mtu;
    mtu_max_worked = 0;
}

void smp_bluetooth::disconnected()
{
    log_information() << "Disconnected from Bluetooth device";
    device_connected = false;
    mtu_max_worked = 0;

    if (bluetooth_service_mcumgr != nullptr)
    {
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_changed(QLowEnergyCharacteristic,QByteArray)));
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_written(QLowEnergyCharacteristic,QByteArray)));
        //        connect(bluetooth_service_mcumgr, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(ServiceDescriptorWritten(QLowEnergyDescriptor,QByteArray)));
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(mcumgr_service_error(QLowEnergyService::ServiceError)));
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(mcumgr_service_state_changed(QLowEnergyService::ServiceState)));

        delete bluetooth_service_mcumgr;

        bluetooth_service_mcumgr = nullptr;
    }

    if (controller != nullptr)
    {
        QObject::disconnect(controller, SIGNAL(connected()), this, SLOT(connected()));
        QObject::disconnect(controller, SIGNAL(disconnected()), this, SLOT(disconnected()));
        QObject::disconnect(controller, SIGNAL(discoveryFinished()), this, SLOT(discovery_finished()));
        QObject::disconnect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(service_discovered(QBluetoothUuid)));
        QObject::disconnect(controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));

        delete controller;

        controller = nullptr;
    }

    bluetooth_window->connection_state(false);
}

void smp_bluetooth::discovery_finished()
{
    log_information() << "Bluetooth service scan finished";
    //bluetooth_service_mcumgr = controller->createServiceObject(QBluetoothUuid(QString("8D53DC1D-1DB7-4CD3-868B-8A527460AA84")));

    if (!bluetooth_service_mcumgr)
    {
        log_error() << "Bluetooth SMP service not found";
        controller->disconnectFromDevice();
    }
    else
    {
        //Connect MCUmgr server signals
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_changed(QLowEnergyCharacteristic,QByteArray)));
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_written(QLowEnergyCharacteristic,QByteArray)));
//        connect(bluetooth_service_mcumgr, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(ServiceDescriptorWritten(QLowEnergyDescriptor,QByteArray)));
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(mcumgr_service_error(QLowEnergyService::ServiceError)));
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(mcumgr_service_state_changed(QLowEnergyService::ServiceState)));

        //Request minimum connection interval
        form_min_params();

        //Discover service details - use a timer for this to work around a Qt 5.15.10 bug on windows
//        bluetooth_service_mcumgr->discoverDetails();
        discover_timer.start();
    }
}

void smp_bluetooth::service_discovered(QBluetoothUuid service_uuid)
{
    services.append(service_uuid);
    log_debug() << "Discovered Bluetooth service: " << service_uuid.toString();

    if (service_uuid == QBluetoothUuid(QString("8D53DC1D-1DB7-4CD3-868B-8A527460AA84")))
    {
        bluetooth_service_mcumgr = controller->createServiceObject(service_uuid);
    }
}

void smp_bluetooth::mcumgr_service_characteristic_changed(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData)
{
    log_debug() << "Bluetooth service characteristic changed";

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
    log_debug() << "Bluetooth service characteristic written";

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
            log_debug() << "Bluetooth service characteristic write of " << (sendbuffer.length() > mtu ? mtu : sendbuffer.length()) << " bytes";
        }
    }
}

void smp_bluetooth::mcumgr_service_state_changed(QLowEnergyService::ServiceState nNewState)
{
    log_debug() << "Bluetooth service state changed: " << nNewState;

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
                log_error() << "Bluetooth transmit characteristic not valid";
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
                log_error() << "Bluetooth transmit descriptor not valid";
            }

            //Enable Tx descriptor notifications
            bluetooth_service_mcumgr->writeDescriptor(descTXDesc, QByteArray::fromHex("0100"));
        }
    }
}

void smp_bluetooth::errorz(QLowEnergyController::Error error)
{
    bool disconnect_from_device = false;
    QString err;

    switch (error)
    {
        case QLowEnergyController::NoError:
        {
            err = "No error";
            break;
        }
        case QLowEnergyController::UnknownError:
        {
            disconnect_from_device = true;
            err = "Unknown error";
            break;
        }
        case QLowEnergyController::UnknownRemoteDeviceError:
        {
            disconnect_from_device = true;
            err = "Unknown remote device";
            break;
        }
        case QLowEnergyController::NetworkError:
        {
            disconnect_from_device = true;
            err = "Network error";
            break;
        }
        case QLowEnergyController::InvalidBluetoothAdapterError:
        {
            disconnect_from_device = true;
            err = "Invalud bluetooth adapter";
            break;
        }
        case QLowEnergyController::ConnectionError:
        {
            disconnect_from_device = true;
            err = "Connection error";
            break;
        }
        case QLowEnergyController::AdvertisingError:
        {
            err = "Advertising error";
            break;
        }
        case QLowEnergyController::RemoteHostClosedError:
        {
            disconnect_from_device = true;
            err = "Remote host closed";
            break;
        }
        case QLowEnergyController::AuthorizationError:
        {
            err = "Authorisation error";
            break;
        }
        default:
        {
            err = "Other";
            break;
        }
    };
//    bluetooth_window->add_debug(QString::number(error));
    log_debug() << err;

    if (disconnect_from_device == true)
    {
        if (device_connected == true)
        {
            disconnect(true);
        }
        else
        {
            bluetooth_window->discovery_state(false);
            bluetooth_window->connection_state(false);
        }
    }
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
    if (device_connected == false)
    {
        return SMP_TRANSPORT_ERROR_NOT_CONNECTED;
    }

    retry_count = 0;
    sendbuffer.clear();

    if (mtu < mtu_max_worked)
    {
        mtu = mtu_max_worked;
    }

    sendbuffer.append(*message->data());
    bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, sendbuffer.left(mtu));

    //bluetooth_window->add_debug(QString("Writing ").append(QString::number(mtu)).append(sendbuffer.left(mtu)));
    log_debug() << "Bluetooth service characteristic write of " << (sendbuffer.length() > mtu ? mtu : sendbuffer.length()) << " bytes";

    return SMP_TRANSPORT_ERROR_OK;
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
                //bluetooth_window->add_debug("bluetooth_device_listed");
            }
        }
        else
        {
            retry_timer.start();
        }
    }
}

void smp_bluetooth::form_refresh_devices()
{
    if (device_connected == true)
    {
        return;
    }

    bluetooth_window->clear_devices();
    bluetooth_device_list.clear();

    if (discoveryAgent->isActive())
    {
        discoveryAgent->stop();
    }

    discoveryAgent->start();
    bluetooth_window->discovery_state(true);
}

void smp_bluetooth::form_connect_to_device(uint16_t index)
{
    if (discoveryAgent->isActive())
    {
        discoveryAgent->stop();
    }

    if (controller)
    {
        QObject::disconnect(controller, SIGNAL(connected()), this, SLOT(connected()));
        QObject::disconnect(controller, SIGNAL(disconnected()), this, SLOT(disconnected()));
        QObject::disconnect(controller, SIGNAL(discoveryFinished()), this, SLOT(discovery_finished()));
        QObject::disconnect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(service_discovered(QBluetoothUuid)));
        QObject::disconnect(controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));

        delete controller;

        controller = nullptr;
    }

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

    //     if (isRandomAddress())
    //     {
    controller->setRemoteAddressType(QLowEnergyController::RandomAddress);
    //     }
    //     else
    //     {
    //         controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
    //     }
    controller->connectToDevice();

    bluetooth_window->connection_state(true);
}

void smp_bluetooth::form_disconnect_from_device()
{
    if (device_connected == true)
    {
        controller->disconnectFromDevice();
    }
}

int smp_bluetooth::connect()
{
    //TODO
    return SMP_TRANSPORT_ERROR_UNSUPPORTED;
}

int smp_bluetooth::disconnect(bool force)
{
    if (controller != nullptr && device_connected == true)
    {
        controller->disconnectFromDevice();
    }
    else
    {
        bluetooth_window->connection_state(false);
    }

    return SMP_TRANSPORT_ERROR_OK;
}

void smp_bluetooth::open_connect_dialog()
{
    bluetooth_window->show();

    if (device_connected == false)
    {
        bluetooth_window->clear_devices();
        bluetooth_device_list.clear();
        discoveryAgent->start();
        bluetooth_window->discovery_state(true);
    }
}

void smp_bluetooth::timeout_timer()
{
    bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, sendbuffer.left(mtu));
}

void smp_bluetooth::form_min_params()
{
    QLowEnergyConnectionParameters params;
    params.setIntervalRange(connection_interval_min, connection_interval_max);
    params.setLatency(connection_latency);
    params.setSupervisionTimeout(connection_supervision_timeout);

    controller->requestConnectionUpdate(params);
}

void smp_bluetooth::discover_timer_timeout()
{
    bluetooth_service_mcumgr->discoverDetails();
}

void smp_bluetooth::close_connect_dialog()
{
    if (bluetooth_window->isVisible())
    {
        bluetooth_window->close();
    }
}

void smp_bluetooth::form_bluetooth_status(bool *scanning, bool *connecting)
{
    *scanning = discoveryAgent->isActive();
    *connecting = device_connected;
}
