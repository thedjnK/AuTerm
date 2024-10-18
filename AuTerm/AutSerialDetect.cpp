/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module: AutSerialDetect.cpp
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
#include "AutSerialDetect.h"
#include <QSerialPortInfo>
#include <QAbstractEventDispatcher>
#include <QWidget>

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
AutSerialDetect::AutSerialDetect(QObject *parent): QObject{parent}
{
    QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
}

AutSerialDetect::~AutSerialDetect()
{
    QAbstractEventDispatcher::instance()->removeNativeEventFilter(this);
    stop();
}

void AutSerialDetect::start(QString port)
{
#ifdef _WIN32
    HWND main_window = (HWND)((QWidget *)this->parent())->winId();
    DEV_BROADCAST_DEVICEINTERFACE dev_search;

    memset(&dev_search, 0, sizeof(dev_search));
    dev_search.dbcc_size = sizeof(dev_search);
    dev_search.dbcc_classguid = GUID_DEVCLASS_PORTS;
    dev_search.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    access = RegisterDeviceNotificationA(main_window, &dev_search, DEVICE_NOTIFY_WINDOW_HANDLE);
#endif

    watch_port = port;
    port_set = true;
}

void AutSerialDetect::stop()
{
    if (port_set == false)
    {
        return;
    }

#ifdef _WIN32
    UnregisterDeviceNotification(access);

    access = nullptr;
#endif

    port_set = false;
    watch_port.clear();
}

bool AutSerialDetect::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef _WIN32
    MSG *msg = static_cast<MSG *>(message);
    uint type = msg->message;
    WPARAM param = msg->wParam;

    if (type == WM_DEVICECHANGE && param == DBT_DEVICEARRIVAL)
    {
        DEV_BROADCAST_HDR *hdr = (DEV_BROADCAST_HDR *)msg->lParam;

        if (hdr->dbch_devicetype == DBT_DEVTYP_PORT)
        {
            foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
            {
                if (info.portName() == watch_port)
                {
                    //Port has reconnected, notify the application then prevent receiving more events
                    emit port_reconnected(watch_port);
                    stop();
                }
            }
        }
    }
#endif

    return false;
}
/******************************************************************************/
// END OF FILE
/******************************************************************************/
