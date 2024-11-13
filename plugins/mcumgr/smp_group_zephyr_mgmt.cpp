/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  smp_group_zephyr_mgmt.cpp
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
#include "smp_group_zephyr_mgmt.h"

enum modes : uint8_t {
    MODE_IDLE = 0,
    MODE_STORAGE_ERASE,
};

enum zephyr_mgmt_commands : uint8_t {
    COMMAND_STORAGE_ERASE = 0,
};

static QStringList smp_error_defines = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "FLASH_OPEN_FAILED" <<
    "FLASH_CONFIG_QUERY_FAIL" <<
    "FLASH_ERASE_FAILED";

static QStringList smp_error_values = QStringList() <<
    //Error index starts from 2 (no error and unknown error are common and handled in the base code)
    "Opening of the flash area has failed" <<
    "Querying the flash area parameters has failed" <<
    "Erasing the flash area has failed";

smp_group_zephyr_mgmt::smp_group_zephyr_mgmt(smp_processor *parent) : smp_group(parent, "SETTINGS", SMP_GROUP_ID_ZEPHYR, error_lookup, error_define_lookup)
{
    mode = MODE_IDLE;
}

void smp_group_zephyr_mgmt::receive_ok(uint8_t version, uint8_t op, uint16_t group, uint8_t command, QByteArray data)
{
    Q_UNUSED(op);
    //    qDebug() << "Got ok: " << version << ", " << op << ", " << group << ", "  << command << ", " << data;

    if (mode == MODE_IDLE)
    {
        log_error() << "Unexpected response, not busy";
        emit status(smp_user_data, STATUS_ERROR, "Unexpected response, settings mgmt not busy");
    }
    else if (group != SMP_GROUP_ID_ZEPHYR)
    {
        log_error() << "Unexpected group " << group << ", not " << SMP_GROUP_ID_ZEPHYR;
        emit status(smp_user_data, STATUS_ERROR, "Unexpected group, not settings mgmt");
    }
    else
    {
        uint8_t finished_mode = mode;
        mode = MODE_IDLE;

        if (version != smp_version)
        {
            //The target device does not support the SMP version being used, adjust for duration of transfer and raise a warning to the parent
            smp_version = version;
            emit version_error(version);
        }

        if (finished_mode == MODE_STORAGE_ERASE && command == COMMAND_STORAGE_ERASE)
        {
            //Response to read
            emit status(smp_user_data, STATUS_COMPLETE, nullptr);
        }
        else
        {
            log_error() << "Unsupported command received";
        }
    }
}

void smp_group_zephyr_mgmt::receive_error(uint8_t version, uint8_t op, uint16_t group, uint8_t command, smp_error_t error)
{
    Q_UNUSED(version);
    Q_UNUSED(op);
    Q_UNUSED(group);
    Q_UNUSED(error);

    bool cleanup = true;
    log_error() << "error :(";

    if (command == COMMAND_STORAGE_ERASE && mode == MODE_STORAGE_ERASE)
    {
        //TODO
        emit status(smp_user_data, status_error_return(error), smp_error::error_lookup_string(&error));
    }
    else
    {
        //Unexpected response operation for mode
        emit status(smp_user_data, STATUS_ERROR, QString("Unexpected error (Mode: %1, op: %2)").arg(mode_to_string(mode), command_to_string(command)));
    }

    if (cleanup == true)
    {
        mode = MODE_IDLE;
    }
}

void smp_group_zephyr_mgmt::cancel()
{
    if (mode != MODE_IDLE)
    {
        mode = MODE_IDLE;

        emit status(smp_user_data, STATUS_CANCELLED, nullptr);
    }
}

bool smp_group_zephyr_mgmt::start_storage_erase(void)
{
    smp_message *tmp_message = new smp_message();
    tmp_message->start_message(SMP_OP_WRITE, smp_version, SMP_GROUP_ID_ZEPHYR, COMMAND_STORAGE_ERASE, 0);
    tmp_message->end_message();

    mode = MODE_STORAGE_ERASE;

    //	    qDebug() << "len: " << message.length();

    if (check_message_before_send(tmp_message) == false)
    {
        return false;
    }

    return handle_transport_error(processor->send(tmp_message, smp_timeout, smp_retries, true));
}

QString smp_group_zephyr_mgmt::mode_to_string(uint8_t mode)
{
    switch (mode)
    {
    case MODE_IDLE:
        return "Idle";
    case MODE_STORAGE_ERASE:
        return "Erasing storage";
    default:
        return "Invalid";
    }
}

QString smp_group_zephyr_mgmt::command_to_string(uint8_t command)
{
    switch (command)
    {
    case COMMAND_STORAGE_ERASE:
        return "Erasing storage";
    default:
        return "Invalid";
    }
}

bool smp_group_zephyr_mgmt::error_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_values.length())
    {
        *error = smp_error_values.at(rc);
        return true;
    }

    return false;
}

bool smp_group_zephyr_mgmt::error_define_lookup(int32_t rc, QString *error)
{
    rc -= smp_version_2_error_code_start;

    if (rc < smp_error_defines.length())
    {
        *error = smp_error_defines.at(rc);
        return true;
    }

    return false;
}

void smp_group_zephyr_mgmt::cleanup()
{
    mode = MODE_IDLE;
}
