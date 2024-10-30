/******************************************************************************
** Copyright (C) 2021-2024 Jamie M.
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

//Aim for a connection interval of between 7.5us-30us with a 4 second supervision timeout, allow connection latency for battery powered devices
static const double connection_interval_min = 7.5;
static const double connection_interval_max = 30;
static const int connection_latency = 2;
static const int connection_supervision_timeout = 4000;

//Default MTU of 490 - less than 512 maximum with a bit of safety
static const int default_mtu = 490;
static const int min_mtu = 20;
static const int max_mtu = 509;

//ATT overhead of messages
static const int mtu_atu_overhead = 3;

smp_bluetooth::smp_bluetooth(QObject *parent)
{
    Q_UNUSED(parent);

    main_window = plugin_mcumgr::get_main_window();
    bluetooth_window = new bluetooth_setup(main_window);

    QObject::connect(bluetooth_window, SIGNAL(refresh_devices()), this, SLOT(form_refresh_devices()));
    QObject::connect(bluetooth_window, SIGNAL(connect_to_device(uint16_t,uint8_t)), this, SLOT(form_connect_to_device(uint16_t,uint8_t)));
    QObject::connect(bluetooth_window, SIGNAL(disconnect_from_device()), this, SLOT(form_disconnect_from_device()));
    QObject::connect(bluetooth_window, SIGNAL(bluetooth_status(bool*,bool*)), this, SLOT(form_bluetooth_status(bool*,bool*)));
    QObject::connect(bluetooth_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), main_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));

    bluetooth_service_mcumgr = nullptr;
    discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    discoveryAgent->setLowEnergyDiscoveryTimeout(8000);
    QObject::connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    QObject::connect(discoveryAgent, SIGNAL(finished()), this, SLOT(finished()));
    device_connected = false;

    QObject::connect(&retry_timer, SIGNAL(timeout()), this, SLOT(timeout_timer()));
    retry_timer.setInterval(500);
    retry_timer.setSingleShot(true);

#if !(QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
    QObject::connect(&discover_timer, SIGNAL(timeout()), this, SLOT(discover_timer_timeout()));
    discover_timer.setInterval(50);
    discover_timer.setSingleShot(true);
#endif

    mtu_max_worked = 0;
}
/*
    void error(QBluetoothDeviceDiscoveryAgent::Error error);
    void canceled();
 */

smp_bluetooth::~smp_bluetooth()
{
    QObject::disconnect(this, SLOT(form_refresh_devices()));
    QObject::disconnect(this, SLOT(form_connect_to_device(uint16_t,uint8_t)));
    QObject::disconnect(this, SLOT(form_disconnect_from_device()));
    QObject::disconnect(this, SLOT(form_bluetooth_status(bool*,bool*)));
    QObject::disconnect(bluetooth_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), main_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));

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
    QString device = QString(info.address().toString()).append(" ").append(info.name());
    bluetooth_window->add_device(&device);
}

void smp_bluetooth::deviceUpdated(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields)
{
}

void smp_bluetooth::finished()
{
    bluetooth_window->set_status_text("Discovery finished");
    bluetooth_window->discovery_state(false);
}

void smp_bluetooth::connected()
{
    bluetooth_window->set_status_text("Connected");
    controller->discoverServices();
    device_connected = true;
    mtu = default_mtu;
    mtu_max_worked = 0;
    bluetooth_window->connection_state(true);
}

void smp_bluetooth::disconnected()
{
    bluetooth_window->set_status_text("Disconnected");
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
    bluetooth_window->set_status_text("Service scan finished");
//bluetooth_service_mcumgr = controller->createServiceObject(QBluetoothUuid(QString("8D53DC1D-1DB7-4CD3-868B-8A527460AA84")));

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    mtu = (controller->mtu() - mtu_atu_overhead);

    if (mtu < min_mtu)
    {
        mtu = min_mtu;
    }
    else if (mtu > max_mtu)
    {
        mtu = max_mtu;
    }

    log_debug() << "Bluetooth MTU: " << this->mtu;
#endif

    if (!bluetooth_service_mcumgr)
    {
        bluetooth_window->set_status_text("Error: SMP service not found");
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

        //Discover service details
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        bluetooth_service_mcumgr->discoverDetails();
#else
        //Use a timer for this to work around a Qt 5.15.10 bug on windows
        discover_timer.start();
#endif
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
}

void smp_bluetooth::mcumgr_service_characteristic_written(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData)
{
    log_debug() << "Bluetooth service characteristic written";

    if (baData.length() > mtu_max_worked)
    {
        mtu_max_worked = baData.length();
    }

    retry_count = 0;

    if (send_buffer.length() > 0)
    {
        send_buffer.remove(0, baData.length());

        if (send_buffer.length() > 0)
        {
            bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));
            log_debug() << "Bluetooth service characteristic write of " << (send_buffer.length() > mtu ? mtu : send_buffer.length()) << " bytes";
        }
    }
}

void smp_bluetooth::mcumgr_service_state_changed(QLowEnergyService::ServiceState nNewState)
{
    bool disconnect_from_device = false;
    log_debug() << "Bluetooth service state changed: " << nNewState;

    //Service state changed
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    if (nNewState == QLowEnergyService::RemoteServiceDiscovered)
#else
    if (nNewState == QLowEnergyService::ServiceDiscovered)
#endif
    {
        QLowEnergyService *svcBLEService = qobject_cast<QLowEnergyService *>(sender());

        if (svcBLEService && svcBLEService->serviceUuid() == QBluetoothUuid(QString("8D53DC1D-1DB7-4CD3-868B-8A527460AA84")))
        {
            bluetooth_characteristic_transmit = bluetooth_service_mcumgr->characteristic(QBluetoothUuid(QString("DA2E7828-FBCE-4E01-AE9E-261174997C48")));

            if (!bluetooth_characteristic_transmit.isValid())
            {
                //Missing Tx characteristic
                disconnect_from_device = true;
                log_error() << "Bluetooth transmit characteristic not valid";
            }
            else
            {
                //Tx notifications descriptor
                const QLowEnergyDescriptor descTXDesc = bluetooth_characteristic_transmit.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

                if (!descTXDesc.isValid())
                {
                    //Tx descriptor missing
                    disconnect_from_device = true;
                    log_error() << "Bluetooth transmit descriptor not valid";
                }
                else
                {
                    //Enable Tx descriptor notifications
                    bluetooth_service_mcumgr->writeDescriptor(descTXDesc, QByteArray::fromHex("0100"));
                }
            }
        }
    }

    if (disconnect_from_device == true)
    {
        controller->disconnectFromDevice();
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
            return;
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
    bluetooth_window->set_status_text(err);

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
    send_buffer.clear();

    if (mtu < mtu_max_worked)
    {
        mtu = mtu_max_worked;
    }

    send_buffer.append(*message->data());
    bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));

    //bluetooth_window->add_debug(QString("Writing ").append(QString::number(mtu)).append(send_buffer.left(mtu)));
    log_debug() << "Bluetooth service characteristic write of " << (send_buffer.length() > mtu ? mtu : send_buffer.length()) << " bytes";

    return SMP_TRANSPORT_ERROR_OK;
}

void smp_bluetooth::mcumgr_service_error(QLowEnergyService::ServiceError error)
{
    if (error == QLowEnergyService::CharacteristicWriteError)
    {
        log_error() << "Bluetooth characteristic write failed with MTU " << mtu;

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

                bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));
            }
            else
            {
                send_buffer.clear();
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

    bluetooth_window->set_status_text("Scanning...");
}

void smp_bluetooth::form_connect_to_device(uint16_t index, uint8_t address_type)
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        QObject::disconnect(controller, SIGNAL(mtuChanged(int)), this, SLOT(mtu_updated(int)));
#endif

        delete controller;
        controller = nullptr;
    }

    // Connecting signals and slots for connecting to LE services.
    controller = QLowEnergyController::createCentral(bluetooth_device_list.at(index));
    QObject::connect(controller, SIGNAL(connected()), this, SLOT(connected()));
    QObject::connect(controller, SIGNAL(disconnected()), this, SLOT(disconnected()));
    QObject::connect(controller, SIGNAL(discoveryFinished()), this, SLOT(discovery_finished()));
    QObject::connect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(service_discovered(QBluetoothUuid)));
    QObject::connect(controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    QObject::connect(controller, SIGNAL(mtuChanged(int)), this, SLOT(mtu_updated(int)));
#endif

    if (address_type == BLUETOOTH_FORCE_ADDRESS_RANDOM)
    {
        controller->setRemoteAddressType(QLowEnergyController::RandomAddress);
    }
    else if (address_type == BLUETOOTH_FORCE_ADDRESS_PUBLIC)
    {
        controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
    }

    controller->connectToDevice();
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
    bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));
}

void smp_bluetooth::form_min_params()
{
    QLowEnergyConnectionParameters params;
    params.setIntervalRange(connection_interval_min, connection_interval_max);
    params.setLatency(connection_latency);
    params.setSupervisionTimeout(connection_supervision_timeout);

    controller->requestConnectionUpdate(params);
}

#if !(QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
void smp_bluetooth::discover_timer_timeout()
{
    bluetooth_service_mcumgr->discoverDetails();
}
#endif

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

void smp_bluetooth::setup_finished()
{
#ifndef SKIPPLUGIN_LOGGER
    bluetooth_window->set_logger(logger);
#endif
    bluetooth_window->load_pixmaps();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
void smp_bluetooth::mtu_updated(int mtu)
{
    this->mtu = (mtu - mtu_atu_overhead);

    if (this->mtu < min_mtu)
    {
        this->mtu = min_mtu;
    }
    else if (this->mtu > max_mtu)
    {
        this->mtu = max_mtu;
    }

    log_debug() << "Bluetooth MTU updated to: " << this->mtu;
}
#endif
