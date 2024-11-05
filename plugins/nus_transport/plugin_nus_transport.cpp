/******************************************************************************
** Copyright (C) 2021-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  plugin_nus_transport.cpp
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
#include "plugin_nus_transport.h"
#include <QGroupBox>
#include <QTabWidget>

/******************************************************************************/
// Constants
/******************************************************************************/
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

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
void plugin_nus_transport::setup(QMainWindow *main_window)
{
    parent_window = main_window;
}

void plugin_nus_transport::transport_setup(QWidget *tab)
{
    bluetooth_window = new nus_bluetooth_setup(tab);

    QObject::connect(bluetooth_window, SIGNAL(refresh_devices()), this, SLOT(form_refresh_devices()));
    QObject::connect(bluetooth_window, SIGNAL(connect_to_device(uint16_t,uint8_t)), this, SLOT(form_connect_to_device(uint16_t,uint8_t)));
    QObject::connect(bluetooth_window, SIGNAL(disconnect_from_device()), this, SLOT(form_disconnect_from_device()));
    QObject::connect(bluetooth_window, SIGNAL(bluetooth_status(bool*,bool*)), this, SLOT(form_bluetooth_status(bool*,bool*)));
    QObject::connect(bluetooth_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), parent_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));
    QObject::connect(this, SIGNAL(update_images()), parent_window, SLOT(plugin_force_image_update()));
    QObject::connect(this, SIGNAL(transport_error(int)), parent_window, SLOT(plugin_transport_error(int)));

    bluetooth_window->show();
}

plugin_nus_transport::plugin_nus_transport()
{
#ifndef SKIPPLUGIN_LOGGER
    logger = new debug_logger(this);
#endif

    bluetooth_service_nus = nullptr;
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

plugin_nus_transport::~plugin_nus_transport()
{
    QObject::disconnect(this, SLOT(form_refresh_devices()));
    QObject::disconnect(this, SLOT(form_connect_to_device(uint16_t,uint8_t)));
    QObject::disconnect(this, SLOT(form_disconnect_from_device()));
    QObject::disconnect(this, SLOT(form_bluetooth_status(bool*,bool*)));
    QObject::disconnect(bluetooth_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), parent_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));
    QObject::disconnect(this, SIGNAL(update_images()), parent_window, SLOT(plugin_force_image_update()));
    QObject::disconnect(this, SIGNAL(transport_error(int)), parent_window, SLOT(plugin_transport_error(int)));

#ifndef SKIPPLUGIN_LOGGER
    delete logger;
    logger = nullptr;
#endif

    delete bluetooth_window;

    if (discoveryAgent != nullptr)
    {
        QObject::disconnect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
        QObject::disconnect(discoveryAgent, SIGNAL(finished()), this, SLOT(finished()));
        delete discoveryAgent;
    }

    if (bluetooth_service_nus != nullptr)
    {
        QObject::disconnect(bluetooth_service_nus, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(nus_service_characteristic_changed(QLowEnergyCharacteristic,QByteArray)));
        QObject::disconnect(bluetooth_service_nus, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(nus_service_characteristic_written(QLowEnergyCharacteristic,QByteArray)));
        QObject::disconnect(bluetooth_service_nus, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(nus_service_descriptor_written(QLowEnergyDescriptor,QByteArray)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        QObject::disconnect(bluetooth_service_nus, SIGNAL(errorOccurred(QLowEnergyService::ServiceError)), this, SLOT(nus_service_error(QLowEnergyService::ServiceError)));
#else
        QObject::disconnect(bluetooth_service_nus, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(nus_service_error(QLowEnergyService::ServiceError)));
#endif
        QObject::disconnect(bluetooth_service_nus, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(nus_service_state_changed(QLowEnergyService::ServiceState)));
        delete bluetooth_service_nus;
    }

    if (controller != nullptr)
    {
        QObject::disconnect(controller, SIGNAL(connected()), this, SLOT(connected()));
        QObject::disconnect(controller, SIGNAL(disconnected()), this, SLOT(disconnected()));
        QObject::disconnect(controller, SIGNAL(discoveryFinished()), this, SLOT(discovery_finished()));
        QObject::disconnect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(service_discovered(QBluetoothUuid)));
        QObject::disconnect(controller, SIGNAL(connectionUpdated(QLowEnergyConnectionParameters)), this, SLOT(connection_updated(QLowEnergyConnectionParameters)));
        delete controller;
    }

    bluetooth_device_list.clear();
}

void plugin_nus_transport::deviceDiscovered(const QBluetoothDeviceInfo &info)
{
    bluetooth_device_list.append(info);
    QString device = QString(info.address().toString()).append(" ").append(info.name());
    bluetooth_window->add_device(&device);
}

void plugin_nus_transport::deviceUpdated(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields)
{
}

void plugin_nus_transport::finished()
{
    bluetooth_window->set_status_text("Discovery finished");
    bluetooth_window->discovery_state(false);
}

void plugin_nus_transport::connected()
{
    bluetooth_window->set_status_text("Connected");
    controller->discoverServices();
    device_connected = true;
    mtu = default_mtu;
    mtu_max_worked = 0;
    bluetooth_window->connection_state(true);
    emit update_images();
}

void plugin_nus_transport::disconnected()
{
    bluetooth_window->set_status_text("Disconnected");
    device_connected = false;
    ready_to_send = false;
    mtu_max_worked = 0;

    log_debug() << "disconnected";

    if (bluetooth_service_nus != nullptr)
    {
        QObject::disconnect(bluetooth_service_nus, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(nus_service_characteristic_changed(QLowEnergyCharacteristic,QByteArray)));
        QObject::disconnect(bluetooth_service_nus, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(nus_service_characteristic_written(QLowEnergyCharacteristic,QByteArray)));
        QObject::disconnect(bluetooth_service_nus, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(nus_service_descriptor_written(QLowEnergyDescriptor,QByteArray)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        QObject::disconnect(bluetooth_service_nus, SIGNAL(errorOccurred(QLowEnergyService::ServiceError)), this, SLOT(nus_service_error(QLowEnergyService::ServiceError)));
#else
        QObject::disconnect(bluetooth_service_nus, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(nus_service_error(QLowEnergyService::ServiceError)));
#endif
        QObject::disconnect(bluetooth_service_nus, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(nus_service_state_changed(QLowEnergyService::ServiceState)));
        delete bluetooth_service_nus;
        bluetooth_service_nus = nullptr;
    }

    if (controller != nullptr)
    {
        QObject::disconnect(controller, SIGNAL(connected()), this, SLOT(connected()));
        QObject::disconnect(controller, SIGNAL(disconnected()), this, SLOT(disconnected()));
        QObject::disconnect(controller, SIGNAL(discoveryFinished()), this, SLOT(discovery_finished()));
        QObject::disconnect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(service_discovered(QBluetoothUuid)));
        QObject::disconnect(controller, SIGNAL(connectionUpdated(QLowEnergyConnectionParameters)), this, SLOT(connection_updated(QLowEnergyConnectionParameters)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        QObject::disconnect(controller, SIGNAL(errorOccurred(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
#else
        QObject::disconnect(controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
#endif
        delete controller;
        controller = nullptr;
    }

    bluetooth_window->connection_state(false);

    if (disconnecting_from_device == false)
    {
        emit transport_error(0);
    }
    else
    {
        disconnecting_from_device = false;
    }
}

void plugin_nus_transport::discovery_finished()
{
    bluetooth_window->set_status_text("Service scan finished");
//bluetooth_service_nus = controller->createServiceObject(QBluetoothUuid(QString("8D53DC1D-1DB7-4CD3-868B-8A527460AA84")));

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

    if (!bluetooth_service_nus)
    {
        bluetooth_window->set_status_text("Error: NUS service not found");
        controller->disconnectFromDevice();
    }
    else
    {
        //Connect MCUmgr server signals
        QObject::connect(bluetooth_service_nus, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(nus_service_characteristic_changed(QLowEnergyCharacteristic,QByteArray)));
        QObject::connect(bluetooth_service_nus, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(nus_service_characteristic_written(QLowEnergyCharacteristic,QByteArray)));
        QObject::connect(bluetooth_service_nus, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(nus_service_descriptor_written(QLowEnergyDescriptor,QByteArray)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        QObject::connect(bluetooth_service_nus, SIGNAL(errorOccurred(QLowEnergyService::ServiceError)), this, SLOT(nus_service_error(QLowEnergyService::ServiceError)));
#else
        QObject::connect(bluetooth_service_nus, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(nus_service_error(QLowEnergyService::ServiceError)));
#endif
        QObject::connect(bluetooth_service_nus, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(nus_service_state_changed(QLowEnergyService::ServiceState)));

        //Request minimum connection interval
        form_min_params();

        //Discover service details
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        bluetooth_service_nus->discoverDetails();
#else
        //Use a timer for this to work around a Qt 5.15.10 bug on windows
        discover_timer.start();
#endif
    }
}

void plugin_nus_transport::service_discovered(QBluetoothUuid service_uuid)
{
    services.append(service_uuid);
    log_debug() << "Discovered Bluetooth service: " << service_uuid.toString();

    if (service_uuid == QBluetoothUuid(QString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")))
    {
        bluetooth_service_nus = controller->createServiceObject(service_uuid);
        log_debug() << "Got wanted service";
    }
}

void plugin_nus_transport::nus_service_characteristic_changed(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData)
{
    log_debug() << "Bluetooth service characteristic changed";
    if (lecCharacteristic == bluetooth_characteristic_receive)
    {
        received_data.append(baData);
        emit readyRead();
    }
}

void plugin_nus_transport::nus_service_characteristic_written(QLowEnergyCharacteristic lecCharacteristic, QByteArray baData)
{
    log_debug() << "Bluetooth service characteristic written";

    if (lecCharacteristic == bluetooth_characteristic_transmit)
    {
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
                bluetooth_service_nus->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));
                log_debug() << "Bluetooth service characteristic write of " << (send_buffer.length() > mtu ? mtu : send_buffer.length()) << " bytes";
            }
        }

        emit bytesWritten(baData.length());
    }
}

void plugin_nus_transport::nus_service_descriptor_written(const QLowEnergyDescriptor info, const QByteArray value)
{
    if (info == bluetooth_descriptor_receive_cccd && ready_to_send == false)
    {
        ready_to_send = true;

//        form_min_params();

        if (send_buffer.length() > 0)
        {
            retry_count = 0;

            if (mtu < mtu_max_worked)
            {
                mtu = mtu_max_worked;
            }

            bluetooth_service_nus->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));
        }
    }
}

void plugin_nus_transport::nus_service_state_changed(QLowEnergyService::ServiceState nNewState)
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

        if (svcBLEService && svcBLEService->serviceUuid() == QBluetoothUuid(QString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")))
        {
            bluetooth_characteristic_transmit = bluetooth_service_nus->characteristic(QBluetoothUuid(QString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")));
            bluetooth_characteristic_receive = bluetooth_service_nus->characteristic(QBluetoothUuid(QString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")));

            if (!bluetooth_characteristic_transmit.isValid())
            {
                //Missing Tx characteristic
                disconnect_from_device = true;
                log_error() << "Bluetooth transmit characteristic not valid";
            }
            else if (!bluetooth_characteristic_receive.isValid())
            {
                //Missing Tx characteristic
                disconnect_from_device = true;
                log_error() << "Bluetooth recieve characteristic not valid";
            }
            else
            {
                //Tx notifications descriptor
                bluetooth_descriptor_receive_cccd = bluetooth_characteristic_receive.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

                if (!bluetooth_descriptor_receive_cccd.isValid())
                {
                    //Tx descriptor missing
                    disconnect_from_device = true;
                    log_error() << "Bluetooth recieve descriptor not valid";
                }
                else
                {
                    //Enable Tx descriptor notifications
                    bluetooth_service_nus->writeDescriptor(bluetooth_descriptor_receive_cccd, QByteArray::fromHex("0100"));
                }
            }
        }
    }

    if (disconnect_from_device == true)
    {
        controller->disconnectFromDevice();
    }
}

void plugin_nus_transport::errorz(QLowEnergyController::Error error)
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
            controller->disconnectFromDevice();
        }
        else
        {
            bluetooth_window->discovery_state(false);
            disconnected();
        }

        emit transport_error(error);
    }
}

void plugin_nus_transport::connection_updated(QLowEnergyConnectionParameters parameters)
{
    log_debug() << "Bluetooth connection parameters: " << parameters.minimumInterval() << "-" << parameters.maximumInterval() << ", latency: " << parameters.latency() << ", timeout: " << parameters.supervisionTimeout();
}

void plugin_nus_transport::nus_service_error(QLowEnergyService::ServiceError error)
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

                bluetooth_service_nus->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));
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

void plugin_nus_transport::form_refresh_devices()
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

void plugin_nus_transport::form_connect_to_device(uint16_t index, uint8_t address_type)
{
    disconnecting_from_device = false;
    ready_to_send = false;

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
        QObject::disconnect(controller, SIGNAL(connectionUpdated(QLowEnergyConnectionParameters)), this, SLOT(connection_updated(QLowEnergyConnectionParameters)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        QObject::disconnect(controller, SIGNAL(errorOccurred(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
        QObject::disconnect(controller, SIGNAL(mtuChanged(int)), this, SLOT(mtu_updated(int)));
#else
        QObject::disconnect(controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
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
    QObject::connect(controller, SIGNAL(connectionUpdated(QLowEnergyConnectionParameters)), this, SLOT(connection_updated(QLowEnergyConnectionParameters)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    QObject::connect(controller, SIGNAL(errorOccurred(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
    QObject::connect(controller, SIGNAL(mtuChanged(int)), this, SLOT(mtu_updated(int)));
#else
    QObject::connect(controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
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

void plugin_nus_transport::form_disconnect_from_device()
{
    if (device_connected == true)
    {
        disconnecting_from_device = true;
        controller->disconnectFromDevice();
    }
}

void plugin_nus_transport::timeout_timer()
{
    bluetooth_service_nus->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));
}

void plugin_nus_transport::form_min_params()
{
    QLowEnergyConnectionParameters params;

    params.setIntervalRange(connection_interval_min, connection_interval_max);
    params.setLatency(connection_latency);
    params.setSupervisionTimeout(connection_supervision_timeout);
    controller->requestConnectionUpdate(params);
}

#if !(QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
void plugin_nus_transport::discover_timer_timeout()
{
    bluetooth_service_nus->discoverDetails();
}
#endif

void plugin_nus_transport::form_bluetooth_status(bool *scanning, bool *connecting)
{
    *scanning = discoveryAgent->isActive();
    *connecting = device_connected;
}

void plugin_nus_transport::setup_finished()
{
#ifndef SKIPPLUGIN_LOGGER
    logger->find_logger_plugin(parent_window);
    bluetooth_window->set_logger(logger);
#endif
    bluetooth_window->load_pixmaps();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
void plugin_nus_transport::mtu_updated(int mtu)
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

QWidget *plugin_nus_transport::GetWidget()
{
    return nullptr;
}

const QString plugin_nus_transport::plugin_about()
{
    return "AuTerm NUS transport plugin\r\nCopyright 2024 Jamie M.\r\n\r\nCan be used to communicate with Nordic UART Service devices over Bluetooth.\r\n\r\nUNFINISHED INITIAL TEST USE ONLY, NOT REPRESENTATIVE OF FINAL PRODUCT.\r\n\r\nBuilt using Qt " QT_VERSION_STR;
}

bool plugin_nus_transport::plugin_configuration()
{
    return false;
}

bool plugin_nus_transport::open(QIODeviceBase::OpenMode mode)
{
    if (device_connected == true)
    {
        controller->disconnectFromDevice();
    }

    return bluetooth_window->connect();
}

void plugin_nus_transport::close()
{
    if (device_connected == true)
    {
        disconnecting_from_device = true;
        emit aboutToClose();
        controller->disconnectFromDevice();
        send_buffer.clear();
        received_data.clear();
    }
}

bool plugin_nus_transport::isOpen() const
{
    return device_connected;
}

bool plugin_nus_transport::isOpening() const
{
    return (controller == nullptr ? false : true);
}

QSerialPort::DataBits plugin_nus_transport::dataBits() const
{
    return QSerialPort::Data8;
}

AutTransportPlugin::StopBits plugin_nus_transport::stopBits() const
{
    return NoStop;
}

QSerialPort::Parity plugin_nus_transport::parity() const
{
    return QSerialPort::NoParity;
}

qint64 plugin_nus_transport::write(const QByteArray &data)
{
    bool send_now = send_buffer.isEmpty();

    if (device_connected == false)
    {
        log_error() << "not connected";
        return 0;
    }

    send_buffer.append(data);

    if (send_now == true && ready_to_send == true)
    {
        retry_count = 0;

        if (mtu < mtu_max_worked)
        {
            mtu = mtu_max_worked;
        }

        bluetooth_service_nus->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));
    }

    return 0;
}

qint64 plugin_nus_transport::bytesAvailable() const
{
    return received_data.length();
}

QByteArray plugin_nus_transport::peek(qint64 maxlen)
{
    return received_data.left(maxlen);
}

QByteArray plugin_nus_transport::read(qint64 maxlen)
{
    QByteArray data = received_data.left(maxlen);

    received_data.remove(0, maxlen);
    return data;
}

QByteArray plugin_nus_transport::readAll()
{
    QByteArray data = received_data;

    received_data.clear();
    return data;
}

bool plugin_nus_transport::clear(QSerialPort::Directions directions)
{
    if ((directions & QSerialPort::Input) != 0)
    {
        received_data.clear();
    }

    if ((directions & QSerialPort::Output) != 0)
    {
        send_buffer.clear();
    }

    return true;
}

QSerialPort::PinoutSignals plugin_nus_transport::pinoutSignals()
{
    if (device_connected == true)
    {
        return QSerialPort::ClearToSendSignal;
    }

    return QSerialPort::NoSignal;
}

QString plugin_nus_transport::to_error_string(int error)
{
return 0;
}

QString plugin_nus_transport::transport_name() const
{
    return "NUS";
}

AutPlugin::PluginType plugin_nus_transport::plugin_type()
{
    return AutPlugin::Transport;
}

QObject *plugin_nus_transport::plugin_object()
{
    return this;
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
