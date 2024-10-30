/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_lorawan.cpp
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
#include "smp_lorawan.h"
#include <QInputDialog>

smp_lorawan::smp_lorawan(QObject *parent)
{
    Q_UNUSED(parent);

    main_window = plugin_mcumgr::get_main_window();
    lorawan_window = new lorawan_setup(main_window);

    mqtt_client = new QMqttClient(this);
    mqtt_topic_subscription = nullptr;
    mqtt_is_connected = false;

    QObject::connect(mqtt_client, SIGNAL(connected()), this, SLOT(mqtt_connected()));
    QObject::connect(mqtt_client, SIGNAL(disconnected()), this, SLOT(mqtt_disconnected()));
    QObject::connect(mqtt_client, SIGNAL(stateChanged(QMqttClient::ClientState)), this, SLOT(mqtt_state_changed(QMqttClient::ClientState)));
    QObject::connect(mqtt_client, SIGNAL(errorChanged(QMqttClient::ClientError)), this, SLOT(mqtt_error_changed(QMqttClient::ClientError)));
    QObject::connect(mqtt_client, SIGNAL(authenticationRequested(QMqttAuthenticationProperties)), this, SLOT(mqtt_authentication_requested(QMqttAuthenticationProperties)));
    QObject::connect(lorawan_window, SIGNAL(connect_to_service(QString,uint16_t,bool,QString,QString,QString)), this, SLOT(connect_to_service(QString,uint16_t,bool,QString,QString,QString)));
    QObject::connect(lorawan_window, SIGNAL(disconnect_from_service()), this, SLOT(disconnect_from_service()));
    QObject::connect(lorawan_window, SIGNAL(plugin_save_setting(QString,QVariant)), main_window, SLOT(plugin_save_setting(QString,QVariant)));
    QObject::connect(lorawan_window, SIGNAL(plugin_load_setting(QString,QVariant*,bool*)), main_window, SLOT(plugin_load_setting(QString,QVariant*,bool*)));
    QObject::connect(lorawan_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), main_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));
}

smp_lorawan::~smp_lorawan()
{
    QObject::disconnect(this, SLOT(mqtt_connected()));
    QObject::disconnect(this, SLOT(mqtt_disconnected()));
    QObject::disconnect(this, SLOT(mqtt_state_changed(QMqttClient::ClientState)));
    QObject::disconnect(this, SLOT(mqtt_error_changed(QMqttClient::ClientError)));
    QObject::disconnect(this, SLOT(mqtt_authentication_requested(QMqttAuthenticationProperties)));
    QObject::disconnect(this, SLOT(connect_to_service(QString,uint16_t,bool,QString,QString,QString)));
    QObject::disconnect(this, SLOT(disconnect_from_service()));
    QObject::disconnect(lorawan_window, SIGNAL(plugin_save_setting(QString,QVariant)), main_window, SLOT(plugin_save_setting(QString,QVariant)));
    QObject::disconnect(lorawan_window, SIGNAL(plugin_load_setting(QString,QVariant*,bool*)), main_window, SLOT(plugin_load_setting(QString,QVariant*,bool*)));
    QObject::disconnect(lorawan_window, SIGNAL(plugin_get_image_pixmap(QString,QPixmap**)), main_window, SLOT(plugin_get_image_pixmap(QString,QPixmap**)));

    if (mqtt_topic_subscription != nullptr)
    {
        QObject::disconnect(this, SLOT(mqtt_topic_message_received(QMqttMessage)));
        QObject::disconnect(this, SLOT(mqtt_topic_state_changed(QMqttSubscription::SubscriptionState)));

        mqtt_topic_subscription->unsubscribe();
        delete mqtt_topic_subscription;
        mqtt_topic_subscription = nullptr;
    }

    if (mqtt_is_connected == true)
    {
        mqtt_client->disconnectFromHost();
        mqtt_is_connected = false;
    }

    if (lorawan_window->isVisible())
    {
        lorawan_window->close();
    }

    delete lorawan_window;
    delete mqtt_client;
}

int smp_lorawan::connect(void)
{
    if (mqtt_is_connected == true)
    {
        return SMP_TRANSPORT_ERROR_ALREADY_CONNECTED;
    }

    return SMP_TRANSPORT_ERROR_OK;
}

int smp_lorawan::disconnect(bool force)
{
    if (mqtt_is_connected == false)
    {
        return SMP_TRANSPORT_ERROR_NOT_CONNECTED;
    }

    mqtt_client->disconnectFromHost();
    received_data.clear();

    return SMP_TRANSPORT_ERROR_OK;
}

void smp_lorawan::open_connect_dialog()
{
    lorawan_window->show();
}

int smp_lorawan::is_connected()
{
    if (mqtt_is_connected == true)
    {
        return 1;
    }

    return 0;
}

int smp_lorawan::send(smp_message *message)
{
    QJsonObject json_object;
    QJsonArray json_object_downlink_array;
    uint16_t processed = 0;
    uint16_t fragment_size = lorawan_window->get_fragment_size();
    bool confirmed_download = lorawan_window->get_confirmed_downlinks();

    if (mqtt_is_connected == false)
    {
        return SMP_TRANSPORT_ERROR_NOT_CONNECTED;
    }

    if (!buffered_send_data.isEmpty())
    {
        return SMP_TRANSPORT_ERROR_OK;
    }

    received_data.clear();

    while (processed < message->size())
    {
        QJsonObject json_object_downlink;
        uint16_t chunk_size = message->size() - processed;

        if (chunk_size > fragment_size)
        {
            chunk_size = fragment_size;
        }

        json_object_downlink.insert("f_port", lorawan_window->get_frame_port());
        json_object_downlink.insert("frm_payload", QString(message->data()->mid(processed, chunk_size).toBase64()));

        if (confirmed_download == true)
        {
            json_object_downlink.insert("confirmed", true);
        }

        json_object_downlink_array.append(json_object_downlink);

        processed += chunk_size;
    }

    json_object.insert("downlinks", json_object_downlink_array);
    mqtt_client->publish(mqtt_downlink_topic, QJsonDocument(json_object).toJson());

    return SMP_TRANSPORT_ERROR_OK;
}

void smp_lorawan::connect_to_service(QString host, uint16_t port,  bool tls, QString username, QString password, QString topic)
{
    mqtt_client->setHostname(host);
    mqtt_client->setPort(port);
    mqtt_client->setUsername(username);
    mqtt_client->setPassword(password);

    if (tls == true)
    {
        mqtt_client->connectToHostEncrypted(QSslConfiguration::defaultConfiguration());
    }
    else
    {
        mqtt_client->connectToHost();
    }

    mqtt_topic = QString(topic).append("/up");
    mqtt_downlink_topic = QString(topic).append("/down/push");
    lorawan_window->set_connection_options_enabled(false);
}

void smp_lorawan::close_connect_dialog()
{
    if (lorawan_window->isVisible())
    {
        lorawan_window->close();
    }
}

void smp_lorawan::setup_finished()
{
#ifndef SKIPPLUGIN_LOGGER
    lorawan_window->set_logger(logger);
#endif

    lorawan_window->load_settings();
    lorawan_window->load_pixmaps();
}

void smp_lorawan::mqtt_connected()
{
    log_debug() << "MQTT connected";
    mqtt_is_connected = true;
}

void smp_lorawan::mqtt_disconnected()
{
    log_debug() << "MQTT disconnected";
    mqtt_is_connected = false;

    if (mqtt_topic_subscription != nullptr)
    {
        QObject::disconnect(this, SLOT(mqtt_topic_message_received(QMqttMessage)));
        QObject::disconnect(this, SLOT(mqtt_topic_state_changed(QMqttSubscription::SubscriptionState)));

        mqtt_topic_subscription->unsubscribe();
        delete mqtt_topic_subscription;
        mqtt_topic_subscription = nullptr;
    }

    lorawan_window->set_connection_options_enabled(true);
}

void smp_lorawan::mqtt_state_changed(QMqttClient::ClientState state)
{
    switch (state)
    {
        case QMqttClient::Disconnected:
        {
            mqtt_is_connected = false;
            lorawan_window->set_connection_state(false);
            return;
        }
        case QMqttClient::Connected:
        {
            mqtt_is_connected = true;
            mqtt_topic_subscription = mqtt_client->subscribe(mqtt_topic);

            QObject::connect(mqtt_topic_subscription, SIGNAL(messageReceived(QMqttMessage)), this, SLOT(mqtt_topic_message_received(QMqttMessage)));
            QObject::connect(mqtt_topic_subscription, SIGNAL(stateChanged(QMqttSubscription::SubscriptionState)), this, SLOT(mqtt_topic_state_changed(QMqttSubscription::SubscriptionState)));

            lorawan_window->set_connection_state(true);
            log_debug() << "Subscribed to " << mqtt_topic;
            return;
        }
        default:
        {
            return;
        }
    };
}

void smp_lorawan::mqtt_error_changed(QMqttClient::ClientError error)
{
    switch (error)
    {
        case QMqttClient::NoError:
        {
            return;
        }
        case QMqttClient::InvalidProtocolVersion:
        {
            log_error() << "MQTT error: Invalid protocol version";
            break;
        }
        case QMqttClient::IdRejected:
        {
            log_error() << "MQTT error: ID rejcted";
            break;
        }
        case QMqttClient::ServerUnavailable:
        {
            log_error() << "MQTT error: Server unavailable";
            break;
        }
        case QMqttClient::BadUsernameOrPassword:
        {
            log_error() << "MQTT error: Invalid username/password";
            break;
        }
        case QMqttClient::NotAuthorized:
        {
            log_error() << "MQTT error: Not authorised";
            break;
        }
        case QMqttClient::TransportInvalid:
        {
            log_error() << "MQTT error: Invalid transport";
            break;
        }
        case QMqttClient::ProtocolViolation:
        {
            log_error() << "MQTT error: Protocol violation";
            break;
        }
        case QMqttClient::UnknownError:
        {
            log_error() << "MQTT error: Unknown error";
            break;
        }
        case QMqttClient::Mqtt5SpecificError:
        {
            log_error() << "MQTT error: MQTT version 5 specific error";
            break;
        }
        default:
        {
            log_error() << "MQTT error: Unhandled error code: " << error;
            break;
        }
    };

    mqtt_is_connected = false;
    mqtt_client->disconnectFromHost();
}

void smp_lorawan::mqtt_authentication_requested(const QMqttAuthenticationProperties &)
{
    log_warning() << "MQTT authentication requested: not supported, dropping connection";
    mqtt_is_connected = false;
    mqtt_client->disconnectFromHost();
}

void smp_lorawan::mqtt_topic_message_received(QMqttMessage message)
{
    QByteArray decoded_packet;
    QJsonParseError json_error;
    QJsonDocument json_document = QJsonDocument::fromJson(message.payload(), &json_error);
    QJsonObject json_object;

    if (json_document.isNull())
    {
        log_error() << "MQTT topic message JSON decoding failed: " << json_error.errorString();
        return;
    }

    json_object = json_document.object();

    if (!json_object.contains("uplink_message"))
    {
        log_error() << "Invalid MQTT JSON message received: no \"uplink_message\" field";
        return;
    }

    json_object = json_object["uplink_message"].toObject();

    if (!json_object.contains("f_port"))
    {
        log_error() << "Invalid MQTT JSON message received: no \"f_port\" field";
        return;
    }

    if (json_object["f_port"].toInteger() != lorawan_window->get_frame_port())
    {
        log_information() << "Received MQTT JSON message for different port: " << json_object["f_port"].toInteger();
        return;
    }

    if (!json_object.contains("frm_payload"))
    {
        log_information() << "Received MQTT JSON message without payload";
        return;
    }

    decoded_packet = QByteArray::fromBase64(json_object["frm_payload"].toString().toLatin1());

    //Check for duplicate data from retransmissions
    if (received_data.size() >= decoded_packet.size() && received_data.data()->right(decoded_packet.size()) == decoded_packet)
    {
        log_information() << "Received duplicate LoRaWAN packet, ignoring.";
        return;
    }

    received_data.append(decoded_packet);

    if (received_data.is_valid() == true)
    {
        emit receive_waiting(&received_data);
        received_data.clear();
    }

    if (lorawan_window->get_auto_fragment_size() == true)
    {
        //Also get device's data rate to calculate maximum size of messages
        if (!json_object.contains("settings"))
        {
            log_error() << "Invalid MQTT JSON message received: no \"settings\" field";
            return;
        }

        json_object = json_object["settings"].toObject();

        if (!json_object.contains("data_rate"))
        {
            log_error() << "Invalid MQTT JSON message received: no \"data_rate\" field";
            return;
        }

        json_object = json_object["data_rate"].toObject();

        if (!json_object.contains("lora"))
        {
            log_error() << "Invalid MQTT JSON message received: no \"lora\" field";
            return;
        }

        json_object = json_object["lora"].toObject();

        if (!json_object.contains("spreading_factor"))
        {
            log_error() << "Invalid MQTT JSON message received: no \"spreading_factor\" field";
            return;
        }

        log_error() << json_object["spreading_factor"].toInteger();
    }
}

void smp_lorawan::mqtt_topic_state_changed(QMqttSubscription::SubscriptionState state)
{
    switch (state)
    {
        case QMqttSubscription::Unsubscribed:
        case QMqttSubscription::SubscriptionPending:
        case QMqttSubscription::UnsubscriptionPending:
        {
            //Not interested in these events
            return;
        }
        case QMqttSubscription::Subscribed:
        {
            log_information() << "MQTT topic state subscription complete";
            return;
        }
        case QMqttSubscription::Error:
        {
            log_error() << "MQTT topic state subscription failed";
            return;
        }
        default:
        {
            log_error() << "Unhandled MQTT topic state change: " << state;
        }
    };
}

void smp_lorawan::disconnect_from_service()
{
    if (mqtt_is_connected == false)
    {
        lorawan_window->set_connection_state(false);
        lorawan_window->set_connection_options_enabled(true);
        return;
    }

    mqtt_client->disconnectFromHost();
}

uint8_t smp_lorawan::get_retries()
{
    return lorawan_window->get_resends();
}

uint32_t smp_lorawan::get_timeout()
{
    return lorawan_window->get_timeout();
}
