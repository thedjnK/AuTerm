/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_json.h
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
#ifndef SMP_JSON_H
#define SMP_JSON_H

#include <QObject>
#include <QCborStreamReader>
#include "smp_message.h"

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum smp_logging_mode_t {
    SMP_LOGGING_MODE_JSON,
    SMP_LOGGING_MODE_YAML,
    SMP_LOGGING_MODE_CBOR,

    SMP_LOGGING_MODE_COUNT
};

class smp_json : public QObject
{
    Q_OBJECT

public:
    explicit smp_json(QObject *parent = nullptr);
    ~smp_json();
    void append_data(bool sent, smp_message *message);
    void set_indent(uint8_t indent);
    void set_mode(enum smp_logging_mode_t mode);

private slots:

signals:
    void log(bool sent, QString *data);

private:
    bool parse_message_json(QCborStreamReader &reader, QString *output, uint16_t indent, bool *outputs, bool list);
    bool parse_message_yaml(QCborStreamReader &reader, QString *output, uint16_t indent, bool *outputs, bool list, bool first_entry);

    uint16_t indent_spaces;
    enum smp_logging_mode_t output_mode;
};

#endif // SMP_JSON_H
