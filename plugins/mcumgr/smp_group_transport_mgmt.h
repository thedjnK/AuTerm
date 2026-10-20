/******************************************************************************
** Copyright (C) 2026 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_transport_mgmt.h
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
#ifndef SMP_GROUP_TRANSPORT_MGMT_H
#define SMP_GROUP_TRANSPORT_MGMT_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "smp_group.h"
#include "smp_error.h"
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>

enum transport_config_type_t {
    TRANSPORT_CONFIG_TYPE_UNSIGNED_INTEGER,
    TRANSPORT_CONFIG_TYPE_SIGNED_INTEGER,
    TRANSPORT_CONFIG_TYPE_BOOLEAN,
    TRANSPORT_CONFIG_TYPE_TEXT_STRING,
    TRANSPORT_CONFIG_TYPE_BYTE_STRING,

    TRANSPORT_CONFIG_TYPE_COUNT
};

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
struct transport_status_t {
    uint32_t supported;
    uint32_t active;
    bool bridged;
};

struct transport_transports_t {
    uint32_t id;
    QString name;
};

struct transport_modes_t {
    uint32_t id;
    QString description;
    bool incoming;
    bool outgoing;
};

struct transport_config_details_t {
    QString name;
    enum transport_config_type_t type;
    bool required;
};

/******************************************************************************/
// Local class, struct, union typedefs
/******************************************************************************/
typedef QMap<QString, QVariant> transport_connect_params_t;

/******************************************************************************/
// Class definitions
/******************************************************************************/
class smp_group_transport_mgmt : public smp_group
{
    Q_OBJECT

public:
    smp_group_transport_mgmt(smp_processor *parent);
    void receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data) override;
    void receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error) override;
    void cancel() override;

    bool start_connect(uint8_t transport, transport_connect_params_t *items);
    bool start_disconnect(uint8_t transport);
    bool start_status(struct transport_status_t *response);
    bool start_list_transports(QList<transport_transports_t> *response);
    bool start_details(uint8_t transport, QList<transport_modes_t> *response);
    bool start_config_details(uint8_t transport, uint8_t mode, QList<transport_config_details_t> *response);
    static QString config_type_to_string(enum transport_config_type_t type, bool *valid);

protected:
    void cleanup() override;
    QString mode_to_string(uint8_t mode) override;
    QString command_to_string(uint8_t command) override;

private:
    static bool error_lookup(int32_t rc, QString *error);
    static bool error_define_lookup(int32_t rc, QString *error);
    bool parse_status_response(QCborStreamReader &reader, struct transport_status_t *response);
    bool parse_list_transports_response(QCborStreamReader &reader, uint8_t layers, QList<struct transport_transports_t> *transports, bool *transports_found);
    bool parse_details_response(QCborStreamReader &reader, uint8_t layers, QList<struct transport_modes_t> *modes, bool *modes_found);
    bool parse_config_details_response(QCborStreamReader &reader, uint8_t layers, QList<struct transport_config_details_t> *configs, bool *configs_found);

    //
    struct transport_status_t *status_response;
    QList<struct transport_transports_t> *transports_response;
    QList<struct transport_modes_t> *modes_response;
    QList<struct transport_config_details_t> *config_details_response;
};

#endif // SMP_GROUP_TRANSPORT_MGMT_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
