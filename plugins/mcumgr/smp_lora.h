/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_lora.h
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
#ifndef SMP_LORA_H
#define SMP_LORA_H

#include "plugin_mcumgr.h"
#include "smp_transport.h"
#include "lorawan_setup.h"
#include <QByteArray>
#include <QtMqtt/QMqttClient>
#include <QtMqtt/QMqttSubscription>
#include <QtMqtt/QMqttMessage>

class smp_lora : public smp_transport
{
    Q_OBJECT

public:
    smp_lora(QObject *parent = nullptr);
    ~smp_lora();
    int connect(void) override;
    int disconnect(bool force) override;
    void open_connect_dialog();
    int is_connected() override;
    int send(smp_message *message);
    void close_connect_dialog();
    void setup_finished();
    uint8_t get_retries() override;
    uint32_t get_timeout() override;

private slots:
    void connect_to_service(QString host, uint16_t port, bool tls, QString username, QString password, QString topic);
    void disconnect_from_service();
    void mqtt_connected();
    void mqtt_disconnected();
    void mqtt_state_changed(QMqttClient::ClientState state);
    void mqtt_error_changed(QMqttClient::ClientError error);
    void mqtt_authentication_requested(const QMqttAuthenticationProperties &p);
    void mqtt_topic_message_received(QMqttMessage message);
    void mqtt_topic_state_changed(QMqttSubscription::SubscriptionState state);

private:
    lorawan_setup *lorawan_window;
    QMainWindow *main_window;
    QMqttClient *mqtt_client;
    QMqttSubscription *mqtt_topic_subscription;
    QString mqtt_topic;
    QMqttTopicName mqtt_downlink_topic;
    bool mqtt_is_connected;
    smp_message received_data;
    QByteArray buffered_send_data;
};

#endif // SMP_LORA_H
