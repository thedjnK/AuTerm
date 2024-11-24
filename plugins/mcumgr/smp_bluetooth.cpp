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
#if defined(GUI_PRESENT)
#include "bluetooth_setup.h"
#endif

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

enum smp_bluetooth_error_t {
    SMP_BLUETOOTH_ERROR_NONE,
    SMP_BLUETOOTH_ERROR_SERVICE_OPERATION_ERROR,
    SMP_BLUETOOTH_ERROR_SERVICE_DESCRIPTOR_WRITE_ERROR,
    SMP_BLUETOOTH_ERROR_SERVICE_UNKNOWN_ERROR,
    SMP_BLUETOOTH_ERROR_SERVICE_CHARACTERISTIC_READ_ERROR,
    SMP_BLUETOOTH_ERROR_SERVICE_DESCRIPTOR_READ_ERROR,
    SMP_BLUETOOTH_ERROR_CONTROLLER_UNKNOWN_ERROR,
    SMP_BLUETOOTH_ERROR_CONTROLLER_UNKNOWN_REMOTE_DEVICE_ERROR,
    SMP_BLUETOOTH_ERROR_CONTROLLER_NETWORK_ERROR,
    SMP_BLUETOOTH_ERROR_CONTROLLER_INVALID_BLUETOOTH_ADAPTER_ERROR,
    SMP_BLUETOOTH_ERROR_CONTROLLER_CONNECTION_ERROR,
    SMP_BLUETOOTH_ERROR_CONTROLLER_REMOTE_HOST_CLOSED_ERROR,
    SMP_BLUETOOTH_ERROR_CONTROLLER_AUTHORISATION_ERROR,

    SMP_BLUETOOTH_ERROR_COUNT
};

smp_bluetooth::smp_bluetooth(QObject *parent)
{
    Q_UNUSED(parent);

#if defined(GUI_PRESENT)
    main_window = plugin_mcumgr::get_main_window();
    bluetooth_window = new bluetooth_setup(main_window);

    QObject::connect(bluetooth_window, SIGNAL(refresh_devices()), this, SLOT(form_refresh_devices()));
    QObject::connect(bluetooth_window, SIGNAL(connect_to_device(uint16_t,uint8_t,bool)), this, SLOT(form_connect_to_device(uint16_t,uint8_t,bool)));
    QObject::connect(bluetooth_window, SIGNAL(disconnect_from_device()), this, SLOT(form_disconnect_from_device()));
    QObject::connect(bluetooth_window, SIGNAL(bluetooth_status(bool*,bool*)), this, SLOT(form_bluetooth_status(bool*,bool*)));
    QObject::connect(bluetooth_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), main_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));
#endif

    bluetooth_service_mcumgr = nullptr;
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
    ready_to_send = false;
    bluetooth_config_set = false;
    bluetooth_config_connection_in_progress = false;
}
/*
    void error(QBluetoothDeviceDiscoveryAgent::Error error);
    void canceled();
 */

smp_bluetooth::~smp_bluetooth()
{
#if defined(GUI_PRESENT)
    QObject::disconnect(this, SLOT(form_refresh_devices()));
    QObject::disconnect(this, SLOT(form_connect_to_device(uint16_t,uint8_t,bool)));
    QObject::disconnect(this, SLOT(form_disconnect_from_device()));
    QObject::disconnect(this, SLOT(form_bluetooth_status(bool*,bool*)));
    QObject::disconnect(bluetooth_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), main_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));

    delete bluetooth_window;
#endif

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
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(mcumgr_service_descriptor_written(QLowEnergyDescriptor,QByteArray)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(errorOccurred(QLowEnergyService::ServiceError)), this, SLOT(mcumgr_service_error(QLowEnergyService::ServiceError)));
#else
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(mcumgr_service_error(QLowEnergyService::ServiceError)));
#endif
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(mcumgr_service_state_changed(QLowEnergyService::ServiceState)));
        delete bluetooth_service_mcumgr;
        bluetooth_service_mcumgr = nullptr;
    }

    if (controller != nullptr)
    {
        QObject::disconnect(controller, SIGNAL(connected()), this, SLOT(bluetooth_connected()));
        QObject::disconnect(controller, SIGNAL(disconnected()), this, SLOT(bluetooth_disconnected()));
        QObject::disconnect(controller, SIGNAL(discoveryFinished()), this, SLOT(discovery_finished()));
        QObject::disconnect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(service_discovered(QBluetoothUuid)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        QObject::disconnect(controller, SIGNAL(errorOccurred(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
        QObject::disconnect(controller, SIGNAL(mtuChanged(int)), this, SLOT(mtu_updated(int)));
#else
        QObject::disconnect(controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
#endif
        delete controller;
        controller = nullptr;
    }

    bluetooth_device_list.clear();
}

void smp_bluetooth::deviceDiscovered(const QBluetoothDeviceInfo &info)
{
    bluetooth_device_list.append(info);
#if defined(GUI_PRESENT)
    QString device = QString(info.address().toString()).append(" ").append(info.name());
    bluetooth_window->add_device(&device);
#endif

    if (bluetooth_config_connection_in_progress == true)
    {
        if ((bluetooth_config.type == SMP_BLUETOOTH_CONNECT_TYPE_ADDRESS && bluetooth_config.address == info.address().toString()) || (bluetooth_config.type == SMP_BLUETOOTH_CONNECT_TYPE_NAME && bluetooth_config.name == info.name()))
        {
            //Connect to device as this is what we are looking for, prevent connecting to more
            bluetooth_config_connection_in_progress = false;
            form_connect_to_device((bluetooth_device_list.length() - 1), BLUETOOTH_FORCE_ADDRESS_DEFAULT, true);
        }
    }
}

void smp_bluetooth::deviceUpdated(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields)
{
}

void smp_bluetooth::finished()
{
#if defined(GUI_PRESENT)
    bluetooth_window->set_status_text("Discovery finished");
    bluetooth_window->discovery_state(false);
#endif
}

void smp_bluetooth::bluetooth_connected()
{
#if defined(GUI_PRESENT)
    bluetooth_window->set_status_text("Connected");
    bluetooth_window->connection_state(true);
#endif
    controller->discoverServices();
    device_connected = true;
    mtu = default_mtu;
    mtu_max_worked = 0;
}

void smp_bluetooth::bluetooth_disconnected()
{
#if defined(GUI_PRESENT)
    bluetooth_window->set_status_text("Disconnected");
    bluetooth_window->connection_state(false);
#endif
    device_connected = false;
    mtu_max_worked = 0;
    ready_to_send = false;
    bluetooth_config_connection_in_progress = false;

    if (bluetooth_service_mcumgr != nullptr)
    {
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_changed(QLowEnergyCharacteristic,QByteArray)));
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_written(QLowEnergyCharacteristic,QByteArray)));
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(mcumgr_service_descriptor_written(QLowEnergyDescriptor,QByteArray)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(errorOccurred(QLowEnergyService::ServiceError)), this, SLOT(mcumgr_service_error(QLowEnergyService::ServiceError)));
#else
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(mcumgr_service_error(QLowEnergyService::ServiceError)));
#endif
        QObject::disconnect(bluetooth_service_mcumgr, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(mcumgr_service_state_changed(QLowEnergyService::ServiceState)));
        delete bluetooth_service_mcumgr;
        bluetooth_service_mcumgr = nullptr;
    }

#if 0
    if (controller != nullptr)
    {
        QObject::disconnect(controller, SIGNAL(connected()), this, SLOT(bluetooth_connected()));
        QObject::disconnect(controller, SIGNAL(disconnected()), this, SLOT(bluetooth_disconnected()));
        QObject::disconnect(controller, SIGNAL(discoveryFinished()), this, SLOT(discovery_finished()));
        QObject::disconnect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(service_discovered(QBluetoothUuid)));
        QObject::disconnect(controller, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(errorz(QLowEnergyController::Error)));
        delete controller;
        controller = nullptr;
    }
#endif

    emit disconnected();
}

void smp_bluetooth::discovery_finished()
{
#if defined(GUI_PRESENT)
    bluetooth_window->set_status_text("Service scan finished");
#endif
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
#if defined(GUI_PRESENT)
        bluetooth_window->set_status_text("Error: SMP service not found");
#endif
        controller->disconnectFromDevice();
    }
    else
    {
        //Connect MCUmgr server signals
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_changed(QLowEnergyCharacteristic,QByteArray)));
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(mcumgr_service_characteristic_written(QLowEnergyCharacteristic,QByteArray)));
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(mcumgr_service_descriptor_written(QLowEnergyDescriptor,QByteArray)));
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(errorOccurred(QLowEnergyService::ServiceError)), this, SLOT(mcumgr_service_error(QLowEnergyService::ServiceError)));
#else
        QObject::connect(bluetooth_service_mcumgr, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(mcumgr_service_error(QLowEnergyService::ServiceError)));
#endif
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
            bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu), bluetooth_write_mode);
            log_debug() << "Bluetooth service characteristic write of " << (send_buffer.length() > mtu ? mtu : send_buffer.length()) << " bytes";
        }
    }
}

void smp_bluetooth::mcumgr_service_descriptor_written(const QLowEnergyDescriptor info, const QByteArray value)
{
    if (info == bluetooth_descriptor_receive_cccd && ready_to_send == false)
    {
        ready_to_send = true;

        if (send_buffer.length() > 0)
        {
            //Send out pre-buffered data to device
            if (bluetooth_write_mode == QLowEnergyService::WriteWithResponse)
            {
                bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));
                log_debug() << "Bluetooth service characteristic write with response of " << (send_buffer.length() > mtu ? mtu : send_buffer.length()) << " bytes";
            }
            else
            {
                while (send_buffer.length() > 0)
                {
                    uint16_t send_size = mtu > send_buffer.length() ? send_buffer.length() : mtu;

                    bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(send_size), QLowEnergyService::WriteWithoutResponse);
                    send_buffer.remove(0, send_size);
                    log_debug() << "Bluetooth service characteristic write without response of " << send_size << " bytes";
                }
            }
        }

        emit connected();
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
                bluetooth_descriptor_receive_cccd = bluetooth_characteristic_transmit.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

                if (!bluetooth_descriptor_receive_cccd.isValid())
                {
                    //Tx descriptor missing
                    disconnect_from_device = true;
                    log_error() << "Bluetooth transmit descriptor not valid";
                }
                else
                {
                    //Enable Tx descriptor notifications
                    bluetooth_service_mcumgr->writeDescriptor(bluetooth_descriptor_receive_cccd, QByteArray::fromHex("0100"));
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
    int internal_error_code = SMP_BLUETOOTH_ERROR_NONE;
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
            internal_error_code = SMP_BLUETOOTH_ERROR_CONTROLLER_UNKNOWN_ERROR;
            err = "Unknown error";
            break;
        }
        case QLowEnergyController::UnknownRemoteDeviceError:
        {
            disconnect_from_device = true;
            internal_error_code = SMP_BLUETOOTH_ERROR_CONTROLLER_UNKNOWN_REMOTE_DEVICE_ERROR;
            err = "Unknown remote device";
            break;
        }
        case QLowEnergyController::NetworkError:
        {
            disconnect_from_device = true;
            internal_error_code = SMP_BLUETOOTH_ERROR_CONTROLLER_NETWORK_ERROR;
            err = "Network error";
            break;
        }
        case QLowEnergyController::InvalidBluetoothAdapterError:
        {
            disconnect_from_device = true;
            internal_error_code = SMP_BLUETOOTH_ERROR_CONTROLLER_INVALID_BLUETOOTH_ADAPTER_ERROR;
            err = "Invalid bluetooth adapter";
            break;
        }
        case QLowEnergyController::ConnectionError:
        {
            disconnect_from_device = true;
            internal_error_code = SMP_BLUETOOTH_ERROR_CONTROLLER_CONNECTION_ERROR;
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
            internal_error_code = SMP_BLUETOOTH_ERROR_CONTROLLER_REMOTE_HOST_CLOSED_ERROR;
            err = "Remote host closed";
            break;
        }
        case QLowEnergyController::AuthorizationError:
        {
            disconnect_from_device = true;
            internal_error_code = SMP_BLUETOOTH_ERROR_CONTROLLER_AUTHORISATION_ERROR;
            err = "Authorisation error";
            break;
        }
        default:
        {
            err = "Other";
            break;
        }
    };

#if defined(GUI_PRESENT)
    bluetooth_window->set_status_text(err);
#endif

    if (disconnect_from_device == true)
    {
        emit smp_transport::error(internal_error_code);
        disconnect(true);

        if (device_connected == false)
        {
#if defined(GUI_PRESENT)
            bluetooth_window->discovery_state(false);
            bluetooth_window->connection_state(false);
#endif
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

smp_transport_error_t smp_bluetooth::send(smp_message *message)
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

    if (ready_to_send == true)
    {
        if (bluetooth_write_mode == QLowEnergyService::WriteWithResponse)
        {
            bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));
            log_debug() << "Bluetooth service characteristic write with response of " << (send_buffer.length() > mtu ? mtu : send_buffer.length()) << " bytes";
        }
        else
        {
            while (send_buffer.length() > 0)
            {
                uint16_t send_size = mtu > send_buffer.length() ? send_buffer.length() : mtu;

                bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(send_size), QLowEnergyService::WriteWithoutResponse);
                send_buffer.remove(0, send_size);
                log_debug() << "Bluetooth service characteristic write without response of " << send_size << " bytes";
            }
        }
    }

    return SMP_TRANSPORT_ERROR_OK;
}

void smp_bluetooth::mcumgr_service_error(QLowEnergyService::ServiceError error)
{
    if (error == QLowEnergyService::CharacteristicWriteError)
    {
        log_error() << "Bluetooth characteristic write failed with MTU " << mtu;

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

            bluetooth_service_mcumgr->writeCharacteristic(bluetooth_characteristic_transmit, send_buffer.left(mtu));
        }
        else
        {
            send_buffer.clear();
            log_error() << "Unable to write Bluetooth characteristic with minimal MTU size, this connection is unusable";
#if defined(GUI_PRESENT)
            bluetooth_window->set_status_text("Minimal MTU write failed, connection is unusable");
#endif
        }
    }
    else if (error == QLowEnergyService::OperationError || error == QLowEnergyService::DescriptorWriteError || error == QLowEnergyService::UnknownError || error == QLowEnergyService::CharacteristicReadError || error == QLowEnergyService::DescriptorReadError)
    {
        int internal_error_code = SMP_BLUETOOTH_ERROR_NONE;
        switch (error)
        {
            case QLowEnergyService::OperationError:
            {
                internal_error_code = SMP_BLUETOOTH_ERROR_SERVICE_OPERATION_ERROR;
                break;
            }
            case QLowEnergyService::DescriptorWriteError:
            {
                internal_error_code = SMP_BLUETOOTH_ERROR_SERVICE_DESCRIPTOR_WRITE_ERROR;
                break;
            }
            case QLowEnergyService::UnknownError:
            {
                internal_error_code = SMP_BLUETOOTH_ERROR_SERVICE_UNKNOWN_ERROR;
                break;
            }
            case QLowEnergyService::CharacteristicReadError:
            {
                internal_error_code = SMP_BLUETOOTH_ERROR_SERVICE_CHARACTERISTIC_READ_ERROR;
                break;
            }
            case QLowEnergyService::DescriptorReadError:
            {
                internal_error_code = SMP_BLUETOOTH_ERROR_SERVICE_DESCRIPTOR_READ_ERROR;
                break;
            }
            default:
            {
                break;
            }
        };

        emit smp_transport::error(internal_error_code);
    }
    else if (error != QLowEnergyService::NoError)
    {
        log_warning() << "Unhandled Bluetooth low energy service error code: " << error;
    }
}

void smp_bluetooth::form_refresh_devices()
{
    if (device_connected == true)
    {
        return;
    }

#if defined(GUI_PRESENT)
    bluetooth_window->clear_devices();
#endif
    bluetooth_device_list.clear();

    if (discoveryAgent->isActive())
    {
        discoveryAgent->stop();
    }

    discoveryAgent->start();
#if defined(GUI_PRESENT)
    bluetooth_window->discovery_state(true);
    bluetooth_window->set_status_text("Scanning...");
#endif
}

void smp_bluetooth::form_connect_to_device(uint16_t index, uint8_t address_type, bool write_with_response)
{
    if (discoveryAgent->isActive())
    {
        discoveryAgent->stop();
    }

    if (controller)
    {
        QObject::disconnect(controller, SIGNAL(connected()), this, SLOT(bluetooth_connected()));
        QObject::disconnect(controller, SIGNAL(disconnected()), this, SLOT(bluetooth_disconnected()));
        QObject::disconnect(controller, SIGNAL(discoveryFinished()), this, SLOT(discovery_finished()));
        QObject::disconnect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(service_discovered(QBluetoothUuid)));
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
    QObject::connect(controller, SIGNAL(connected()), this, SLOT(bluetooth_connected()));
    QObject::connect(controller, SIGNAL(disconnected()), this, SLOT(bluetooth_disconnected()));
    QObject::connect(controller, SIGNAL(discoveryFinished()), this, SLOT(discovery_finished()));
    QObject::connect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(service_discovered(QBluetoothUuid)));
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

    ready_to_send = false;
    controller->connectToDevice();
    bluetooth_write_mode = (write_with_response == true ? QLowEnergyService::WriteWithResponse : QLowEnergyService::WriteWithoutResponse);
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
    if (device_connected == true)
    {
        return SMP_TRANSPORT_ERROR_ALREADY_CONNECTED;
    }

    if (bluetooth_config_set == false)
    {
        return SMP_TRANSPORT_ERROR_INVALID_CONFIGURATION;
    }

    bluetooth_config_connection_in_progress = true;
    form_refresh_devices();

    return SMP_TRANSPORT_ERROR_OK;
}

int smp_bluetooth::disconnect(bool force)
{
    Q_UNUSED(force);

    if (controller != nullptr && device_connected == true)
    {
        controller->disconnectFromDevice();
    }
    else
    {
#if defined(GUI_PRESENT)
        bluetooth_window->connection_state(false);
#endif
    }

    return SMP_TRANSPORT_ERROR_OK;
}

#if defined(GUI_PRESENT)
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

void smp_bluetooth::close_connect_dialog()
{
    if (bluetooth_window->isVisible())
    {
        bluetooth_window->close();
    }
}
#endif

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

void smp_bluetooth::form_bluetooth_status(bool *scanning, bool *connecting)
{
    *scanning = discoveryAgent->isActive();
    *connecting = device_connected;
}

void smp_bluetooth::setup_finished()
{
#if defined(GUI_PRESENT)
#ifndef SKIPPLUGIN_LOGGER
    bluetooth_window->set_logger(logger);
#endif
    bluetooth_window->load_pixmaps();
#endif
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

int smp_bluetooth::set_connection_config(struct smp_bluetooth_config_t *configuration)
{
    if (device_connected == true)
    {
        return SMP_TRANSPORT_ERROR_ALREADY_CONNECTED;
    }

    bluetooth_config.address = configuration->address;
    bluetooth_config.name = configuration->name;
    bluetooth_config.type = configuration->type;
    bluetooth_config_set = true;

    return SMP_TRANSPORT_ERROR_OK;
}

QString smp_bluetooth::to_error_string(int error_code)
{
    switch (error_code)
    {
        case SMP_BLUETOOTH_ERROR_SERVICE_OPERATION_ERROR:
        {
            return "Bluetooth service operation error";
        }
        case SMP_BLUETOOTH_ERROR_SERVICE_DESCRIPTOR_WRITE_ERROR:
        {
            return "Bluetooth service descriptor write error";
        }
        case SMP_BLUETOOTH_ERROR_SERVICE_UNKNOWN_ERROR:
        {
            return "Bluetooth service unknown error";
        }
        case SMP_BLUETOOTH_ERROR_SERVICE_CHARACTERISTIC_READ_ERROR:
        {
            return "Bluetooth service characteristic read error";
        }
        case SMP_BLUETOOTH_ERROR_SERVICE_DESCRIPTOR_READ_ERROR:
        {
            return "Bluetooth service descriptor read error";
        }
        case SMP_BLUETOOTH_ERROR_CONTROLLER_UNKNOWN_ERROR:
        {
            return "Bluetooth controller unknown error";
        }
        case SMP_BLUETOOTH_ERROR_CONTROLLER_UNKNOWN_REMOTE_DEVICE_ERROR:
        {
            return "Bluetooth controller unknown remote device error";
        }
        case SMP_BLUETOOTH_ERROR_CONTROLLER_NETWORK_ERROR:
        {
            return "Bluetooth controller network error";
        }
        case SMP_BLUETOOTH_ERROR_CONTROLLER_INVALID_BLUETOOTH_ADAPTER_ERROR:
        {
            return "Bluetooth controller invalid bluetooth adapter error";
        }
        case SMP_BLUETOOTH_ERROR_CONTROLLER_CONNECTION_ERROR:
        {
            return "Bluetooth controller connection error";
        }
        case SMP_BLUETOOTH_ERROR_CONTROLLER_REMOTE_HOST_CLOSED_ERROR:
        {
            return "Bluetooth controller remote host closed error";
        }
        case SMP_BLUETOOTH_ERROR_CONTROLLER_AUTHORISATION_ERROR:
        {
            return "Bluetooth controller authorisation error";
        }
        default:
        {
            return "";
        }
    };
}
