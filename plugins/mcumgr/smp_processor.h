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
#include "smp_message.h"
#include "smp_uart.h"

#include <QTimer>
#include <QElapsedTimer>
#include <QCborStreamReader>

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
    smp_processor(QObject *parent, smp_uart *uart_driver);
    ~smp_processor();
    bool send(smp_message *message, uint32_t timeout_ms, uint8_t repeats);
    bool is_busy();
    void register_handler(uint16_t group, smp_group *handler);
    void unregister_handler(uint16_t group);

private:
    void cleanup();
    bool decode_message(QCborStreamReader &reader, uint8_t version, uint16_t level, QString *parent, smp_error_t *error);

public slots:
    void message_timeout();
    void message_received(smp_message *message);

private:
    smp_uart *uart;
    smp_message *last_message;
    const smp_hdr *last_message_header;
    QTimer repeat_timer;
    uint8_t repeat_times;
    bool busy;
    QList<smp_group_match_t> group_handlers;
};

#endif // smp_processor_H
