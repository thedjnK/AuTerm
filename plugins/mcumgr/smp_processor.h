/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_processor.h
**
** Notes:   With exception to the crc16() function which is apache 2.0 licensed
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
#ifndef smp_processor_H
#define smp_processor_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QCborStreamReader>
#include "smp_message.h"
#include "smp_transport.h"
#include "debug_logger.h"
#if defined(PLUGIN_MCUMGR_JSON)
#include "smp_json.h"
#endif

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum custom_message_callback_t {
    CUSTOM_MESSAGE_CALLBACK_OK,
    CUSTOM_MESSAGE_CALLBACK_ERROR,
    CUSTOM_MESSAGE_CALLBACK_TIMEOUT,
    CUSTOM_MESSAGE_CALLBACK_TRANSPORT_DISCONNECTED,
    CUSTOM_MESSAGE_CALLBACK_CANCELLED,

    CUSTOM_MESSAGE_CALLBACK_COUNT
};

//Forward declaration due to reverse dependency
class smp_group;

struct smp_group_match_t {
    uint16_t group;
    smp_group *handler;
};

class smp_processor : public QObject
{
    Q_OBJECT

public:
    smp_processor(QObject *parent);
    ~smp_processor();
#ifndef SKIPPLUGIN_LOGGER
    void set_logger(debug_logger *object);
#endif
    smp_transport_error_t send(smp_message *message, uint32_t timeout_ms, uint8_t repeats, bool allow_version_check);
    bool is_busy();
    void register_handler(uint16_t group, smp_group *handler);
    void unregister_handler(uint16_t group);
    void set_transport(smp_transport *transport_object);
    uint16_t max_message_data_size(uint16_t mtu);
#if defined(PLUGIN_MCUMGR_JSON)
    void set_json(smp_json *json);
#endif
    void set_message_logging(bool enabled);
    void set_custom_message(bool enabled);
    void cancel();

private:
    void cleanup();
    bool decode_message(QCborStreamReader &reader, uint8_t version, uint16_t level, QString *parent, smp_error_t *error);

public slots:
    void message_timeout();
    void message_received(smp_message *message);
    void transport_disconnect(int error_code);

signals:
    void custom_message_callback(enum custom_message_callback_t type, smp_error_t *data);

private:
    uint8_t sequence;
    smp_transport *transport;
    smp_message *last_message;
    smp_hdr *last_message_header;
    bool last_message_version_check;
    uint8_t last_message_version;
    QTimer repeat_timer;
    uint8_t repeat_times;
    bool busy;
    QList<smp_group_match_t> group_handlers;
#if defined(PLUGIN_MCUMGR_JSON)
    smp_json *json_object;
#endif
    bool message_logging;
    bool custom_message;

#ifndef SKIPPLUGIN_LOGGER
    debug_logger *logger;
#endif
};

#endif // smp_processor_H
