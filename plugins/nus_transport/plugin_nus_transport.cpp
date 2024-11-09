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
#include <QVBoxLayout>

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

//TODO: Hardcoded until clarification from qt on multiple bugs that have been raised
    //bluetooth_write_with_response = false; //Write without response
    bluetooth_write_with_response = true; //Write with response
}

void plugin_nus_transport::transport_setup(QWidget *tab)
{
    QVBoxLayout *tmp_widget = new QVBoxLayout(nullptr);
    QGroupBox *window_main_widget;
    QVBoxLayout *vertical_layout = new QVBoxLayout(tab);

    vertical_layout->setSpacing(0);
    vertical_layout->setObjectName("verticalLayout_2");
    vertical_layout->setContentsMargins(2, 2, 2, 2);

    //Very silly workaround to prevent issue with Qt having an initial wrong large sized width
    bluetooth_window = new nus_bluetooth_setup(tmp_widget->widget());
    window_main_widget = bluetooth_window->findChild<QGroupBox *>("groupBox");
    bluetooth_window->setParent(vertical_layout->widget());
    vertical_layout->addWidget(window_main_widget);
    tmp_widget->deleteLater();

    QObject::connect(bluetooth_window, SIGNAL(refresh_devices()), this, SLOT(form_refresh_devices()));
    QObject::connect(bluetooth_window, SIGNAL(connect_to_device(uint16_t,uint8_t)), this, SLOT(form_connect_to_device(uint16_t,uint8_t)));
    QObject::connect(bluetooth_window, SIGNAL(disconnect_from_device()), this, SLOT(form_disconnect_from_device()));
    QObject::connect(bluetooth_window, SIGNAL(bluetooth_status(bool*,bool*)), this, SLOT(form_bluetooth_status(bool*,bool*)));
    QObject::connect(bluetooth_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), parent_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));
    QObject::connect(bluetooth_window, SIGNAL(request_connect()), this, SLOT(form_request_connect()));
    QObject::connect(this, SIGNAL(update_images()), parent_window, SLOT(plugin_force_image_update()));
    QObject::connect(this, SIGNAL(transport_error(int)), parent_window, SLOT(plugin_transport_error(int)));
    QObject::connect(this, SIGNAL(transport_open_close(uint8_t)), parent_window, SLOT(plugin_serial_open_close(uint8_t)));
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

#if !(QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
    QObject::connect(&discover_timer, SIGNAL(timeout()), this, SLOT(discover_timer_timeout()));
    discover_timer.setInterval(50);
    discover_timer.setSingleShot(true);
#endif

    mtu_max_worked = 0;
}

plugin_nus_transport::~plugin_nus_transport()
{
    QObject::disconnect(this, SLOT(form_refresh_devices()));
    QObject::disconnect(this, SLOT(form_connect_to_device(uint16_t,uint8_t)));
    QObject::disconnect(this, SLOT(form_disconnect_from_device()));
    QObject::disconnect(this, SLOT(form_bluetooth_status(bool*,bool*)));
    QObject::disconnect(bluetooth_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), parent_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));
    QObject::disconnect(this, SLOT(form_request_connect()));
    QObject::disconnect(this, SIGNAL(update_images()), parent_window, SLOT(plugin_force_image_update()));
    QObject::disconnect(this, SIGNAL(transport_error(int)), parent_window, SLOT(plugin_transport_error(int)));
    QObject::disconnect(this, SIGNAL(transport_open_close(uint8_t)), parent_window, SLOT(plugin_serial_open_close(uint8_t)));

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
        discoveryAgent = nullptr;
    }

    if (device_connected == true)
    {
        controller->disconnectFromDevice();
        device_connected = false;
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
        QObject::disconnect(controller, SIGNAL(mtuChanged(int)), this, SLOT(mtu_updated(int)));
#else
        QObject::disconnect(controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
#endif
        QObject::disconnect(controller, SIGNAL(stateChanged(QLowEnergyController::ControllerState)), this, SLOT(stateChanged(QLowEnergyController::ControllerState)));
        delete controller;
        controller = nullptr;
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

    bluetooth_window->connection_state(false);

    if (disconnecting_from_device == false)
    {
        emit transport_error(NUS_TRANSPORT_ERROR_UNEXPECTED_DISCONNECT);
    }
    else
    {
        disconnecting_from_device = false;
    }
}

void plugin_nus_transport::discovery_finished()
{
    bluetooth_window->set_status_text("Service scan finished");

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
        bluetooth_window->set_status_text(to_error_string(NUS_TRANSPORT_ERROR_MISSING_NUS_SERVICE));
        controller->disconnectFromDevice();
        emit transport_error(NUS_TRANSPORT_ERROR_MISSING_NUS_SERVICE);
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

        if (send_buffer.length() > 0)
        {
            send_buffer.remove(0, baData.length());

            if (send_buffer.length() > 0)
            {
                bluetooth_service_nus->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu), (bluetooth_write_with_response == false ? QLowEnergyService::WriteWithoutResponse : QLowEnergyService::WriteWithResponse));

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

        if (send_buffer.length() > 0)
        {

            if (mtu < mtu_max_worked)
            {
                mtu = mtu_max_worked;
            }

            bluetooth_service_nus->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu), (bluetooth_write_with_response == false ? QLowEnergyService::WriteWithoutResponse : QLowEnergyService::WriteWithResponse));
        }
    }
}

void plugin_nus_transport::nus_service_state_changed(QLowEnergyService::ServiceState nNewState)
{
    bool disconnect_from_device = false;
    int error_code = 0;

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
                //Missing TX characteristic
                disconnect_from_device = true;
                error_code = NUS_TRANSPORT_ERROR_MISSING_TX_CHARACTERISTIC;
                log_error() << "Bluetooth transmit characteristic not valid";
            }
            else if (!bluetooth_characteristic_receive.isValid())
            {
                //Missing RX characteristic
                disconnect_from_device = true;
                error_code = NUS_TRANSPORT_ERROR_MISSING_RX_CHARACTERISTIC;
                log_error() << "Bluetooth recieve characteristic not valid";
            }
            else
            {
                //Tx notifications descriptor
                bluetooth_descriptor_receive_cccd = bluetooth_characteristic_receive.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

                if (!bluetooth_descriptor_receive_cccd.isValid())
                {
                    //RX descriptor missing
                    disconnect_from_device = true;
                    error_code = NUS_TRANSPORT_ERROR_MISSING_RX_DESCRIPTOR;
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
        emit transport_error(error_code);
    }
}

void plugin_nus_transport::errorz(QLowEnergyController::Error error)
{
    bool disconnect_from_device = false;
    int error_code = 0;

    switch (error)
    {
        case QLowEnergyController::NoError:
        {
            return;
        }
        case QLowEnergyController::UnknownError:
        {
            disconnect_from_device = true;
            error_code = NUS_TRANSPORT_ERROR_CONTROLLER_UNKNOWN;
            break;
        }
        case QLowEnergyController::UnknownRemoteDeviceError:
        {
            disconnect_from_device = true;
            error_code = NUS_TRANSPORT_ERROR_CONTROLLER_UNKNOWN_REMOTE_DEVICE;
            break;
        }
        case QLowEnergyController::NetworkError:
        {
            disconnect_from_device = true;
            error_code = NUS_TRANSPORT_ERROR_CONTROLLER_NETWORK;
            break;
        }
        case QLowEnergyController::InvalidBluetoothAdapterError:
        {
            disconnect_from_device = true;
            error_code = NUS_TRANSPORT_ERROR_CONTROLLER_INVALID_BLUETOOTH_ADAPTER;
            break;
        }
        case QLowEnergyController::ConnectionError:
        {
            disconnect_from_device = true;
            error_code = NUS_TRANSPORT_ERROR_CONTROLLER_CONNECTION;
            break;
        }
        case QLowEnergyController::AdvertisingError:
        {
            error_code = NUS_TRANSPORT_ERROR_CONTROLLER_ADVERTISING;
            break;
        }
        case QLowEnergyController::RemoteHostClosedError:
        {
            disconnect_from_device = true;
            error_code = NUS_TRANSPORT_ERROR_CONTROLLER_REMOTE_HOST_CLOSED;
            break;
        }
        case QLowEnergyController::AuthorizationError:
        {
            disconnect_from_device = true;
            error_code = NUS_TRANSPORT_ERROR_CONTROLLER_AUTHORISATION;
            break;
        }
        default:
        {
            error_code = NUS_TRANSPORT_ERROR_CONTROLLER_NOT_DEFINED;
            break;
        }
    };

    bluetooth_window->set_status_text(to_error_string(error_code));

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

        emit transport_error(error_code);
    }
}

void plugin_nus_transport::connection_updated(QLowEnergyConnectionParameters parameters)
{
    log_debug() << "Bluetooth connection parameters: " << parameters.minimumInterval() << "-" << parameters.maximumInterval() << ", latency: " << parameters.latency() << ", timeout: " << parameters.supervisionTimeout();
}

void plugin_nus_transport::stateChanged(QLowEnergyController::ControllerState state)
{
    log_debug() << "Bluetooth controller state: " << state;
}

void plugin_nus_transport::nus_service_error(QLowEnergyService::ServiceError error)
{
    int error_code = 0;

    switch (error)
    {
        case QLowEnergyService::NoError:
        {
            return;
        }
        case QLowEnergyService::CharacteristicWriteError:
        {
            log_error() << "Bluetooth characteristic write failed with MTU " << mtu;

//TODO: refactor code when various qt bugs are replied to asking why mtu update and other functions do literally nothing on the only apparent supported OS linux
            if (mtu > 20)
            {
                if (mtu >= 100)
                {
                    mtu -= 32;
                }
                else if (mtu >= 40)
                {
                    mtu = 20;
                }
                else
                {
                    mtu -= 16;
                }

                bluetooth_service_nus->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu), (bluetooth_write_with_response == false ? QLowEnergyService::WriteWithoutResponse : QLowEnergyService::WriteWithResponse));
                return;
            }
            else
            {
                send_buffer.clear();
                log_error() << "Unable to write Bluetooth characteristic with minimal MTU size, this connection is unusable";
                error_code = NUS_TRANSPORT_ERROR_SERVICE_CHARACTERISTIC_WRITE;
                break;
            }
        }
        case QLowEnergyService::OperationError:
        {
            error_code = NUS_TRANSPORT_ERROR_SERVICE_OPERATION;
            break;
        }
        case QLowEnergyService::DescriptorWriteError:
        {
            error_code = NUS_TRANSPORT_ERROR_SERVICE_DESCRIPTOR_WRITE;
            break;
        }
        case QLowEnergyService::UnknownError:
        {
            error_code = NUS_TRANSPORT_ERROR_SERVICE_UNKNOWN;
            break;
        }
        case QLowEnergyService::CharacteristicReadError:
        {
            error_code = NUS_TRANSPORT_ERROR_SERVICE_CHARACTERISTIC_READ;
            break;
        }
        case QLowEnergyService::DescriptorReadError:
        {
            error_code = NUS_TRANSPORT_ERROR_SERVICE_DESCRIPTOR_READ;
            break;
        }
        default:
        {
            error_code = NUS_TRANSPORT_ERROR_SERVICE_NOT_DEFINED;
            break;
        }
    };

    if (error_code != 0)
    {
        bluetooth_window->set_status_text(to_error_string(NUS_TRANSPORT_ERROR_MISSING_NUS_SERVICE));
        controller->disconnectFromDevice();
        emit transport_error(error_code);
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
        QObject::disconnect(controller, SIGNAL(stateChanged(QLowEnergyController::ControllerState)), this, SLOT(stateChanged(QLowEnergyController::ControllerState)));
        delete controller;
        controller = nullptr;
    }

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
    QObject::connect(controller, SIGNAL(stateChanged(QLowEnergyController::ControllerState)), this, SLOT(stateChanged(QLowEnergyController::ControllerState)));

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

void plugin_nus_transport::form_request_connect()
{
    QTabWidget *selector_tab = parent_window->findChild<QTabWidget *>("selector_Tab");
    QWidget *tab_term = selector_tab->findChild<QWidget *>("tab_Term");

    if (selector_tab == nullptr || tab_term == nullptr)
    {
        log_error() << "Either selector_Tab or tab_Term were not found";
        return;
    }

    if (device_connected == true)
    {
        emit transport_open_close(1);
    }

    emit transport_open_close(0);

    if (selector_tab->currentIndex() != selector_tab->indexOf(tab_term))
    {
        selector_tab->setCurrentIndex(selector_tab->indexOf(tab_term));
    }
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
    return "AuTerm NUS transport plugin\r\nCopyright 2024 Jamie M.\r\n\r\nCan be used to communicate with Nordic UART Service devices over Bluetooth.\r\n\r\nBuilt using Qt " QT_VERSION_STR;
}

bool plugin_nus_transport::plugin_configuration()
{
    return false;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool plugin_nus_transport::open(QIODeviceBase::OpenMode mode)
#else
bool plugin_nus_transport::open(QIODevice::OpenMode mode)
#endif
{
    Q_UNUSED(mode);

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
        log_error() << "Cannot write, not connected to Bluetooth device";
        return 0;
    }

    send_buffer.append(data);

    if (send_now == true && ready_to_send == true)
    {
        if (mtu < mtu_max_worked)
        {
            mtu = mtu_max_worked;
        }

        bluetooth_service_nus->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu), (bluetooth_write_with_response == false ? QLowEnergyService::WriteWithoutResponse : QLowEnergyService::WriteWithResponse));
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
    switch (error)
    {
        case NUS_TRANSPORT_ERROR_MISSING_NUS_SERVICE:
        {
            return "Missing NUS service";
        }
        case NUS_TRANSPORT_ERROR_MISSING_TX_CHARACTERISTIC:
        {
            return "Missing TX characteristic";
        }
        case NUS_TRANSPORT_ERROR_MISSING_RX_CHARACTERISTIC:
        {
            return "Missing RX characteristic";
        }
        case NUS_TRANSPORT_ERROR_MISSING_RX_DESCRIPTOR:
        {
            return "Missing RX descriptor";
        }
        case NUS_TRANSPORT_ERROR_MINIMAL_MTU_WRITE_FAILED:
        {
            return "Minimal MTU write failed, connection is unusable";
        }
        case NUS_TRANSPORT_ERROR_UNEXPECTED_DISCONNECT:
        {
            return "Unexpected disconection";
        }
        case NUS_TRANSPORT_ERROR_CONTROLLER_UNKNOWN:
        {
            return "Unknown controller error";
        }
        case NUS_TRANSPORT_ERROR_CONTROLLER_UNKNOWN_REMOTE_DEVICE:
        {
            return "Unknown remote device controller error";
        }
        case NUS_TRANSPORT_ERROR_CONTROLLER_NETWORK:
        {
            return "Network error";
        }
        case NUS_TRANSPORT_ERROR_CONTROLLER_INVALID_BLUETOOTH_ADAPTER:
        {
            return "Invalud Bluetooth adapter";
        }
        case NUS_TRANSPORT_ERROR_CONTROLLER_CONNECTION:
        {
            return "Connection error";
        }
        case NUS_TRANSPORT_ERROR_CONTROLLER_ADVERTISING:
        {
            return "Advertising error";
        }
        case NUS_TRANSPORT_ERROR_CONTROLLER_REMOTE_HOST_CLOSED:
        {
            return "Remote host closed";
        }
        case NUS_TRANSPORT_ERROR_CONTROLLER_AUTHORISATION:
        {
            return "Authorisation error";
        }
        case NUS_TRANSPORT_ERROR_CONTROLLER_NOT_DEFINED:
        {
            return "Other undefined error";
        }
        default:
        {
            return "Invalid NUS error code";
        }
    };
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
