/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_lorawan.h
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
#ifndef SMP_LORAWAN_H
#define SMP_LORAWAN_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "smp_transport.h"
#if defined(GUI_PRESENT)
#include "plugin_mcumgr.h"
#include "lorawan_setup.h"
#endif
#include <QByteArray>
#include <QtMqtt/QMqttClient>
#include <QtMqtt/QMqttSubscription>
#include <QtMqtt/QMqttMessage>

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
struct smp_lorawan_config_t {
    QString hostname;
    uint16_t port;
    bool tls;
    QString username;
    QString password;
    QString topic;
    uint8_t frame_port;
};

/******************************************************************************/
// Class definitions
/******************************************************************************/
class smp_lorawan : public smp_transport
{
    Q_OBJECT

public:
    smp_lorawan(QObject *parent = nullptr);
    ~smp_lorawan();
    int connect(void) override;
    int disconnect(bool force) override;
#if defined(GUI_PRESENT)
    void open_connect_dialog() override;
    void close_connect_dialog() override;
#endif
    int is_connected() override;
    smp_transport_error_t send(smp_message *message) override;
    void setup_finished();
    uint8_t get_retries() override;
    uint32_t get_timeout() override;
    int set_connection_config(struct smp_lorawan_config_t *configuration);
    QString to_error_string(int error_code) override;

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
#if defined(GUI_PRESENT)
    lorawan_setup *lorawan_window;
    QMainWindow *main_window;
#endif
    struct smp_lorawan_config_t lorawan_config;
    bool lorawan_config_set;
    QMqttClient *mqtt_client;
    QMqttSubscription *mqtt_topic_subscription;
    QString mqtt_topic;
    QMqttTopicName mqtt_downlink_topic;
    bool mqtt_is_connected;
    bool mqtt_is_ready;
    smp_message received_data;
    QByteArray buffered_send_data;
    int mqtt_disconnect_error_code;
};

#endif // SMP_LORAWAN_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
