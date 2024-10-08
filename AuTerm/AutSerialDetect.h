/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module: AutSerialDetect.h
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
#ifndef AUTSERIALDETECT_H
#define AUTSERIALDETECT_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QAbstractNativeEventFilter>

#ifdef _WIN32
#include <Windows.h>
#include <WinUser.h>
#include <Dbt.h>
#include <devguid.h>
#endif

/******************************************************************************/
// Class definitions
/******************************************************************************/
class AutSerialDetect : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit AutSerialDetect(QObject *parent = nullptr);
    ~AutSerialDetect();
    void start(QString port);
    void stop();
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) Q_DECL_OVERRIDE;

signals:
    void port_reconnected(QString port);

private:
    QString watch_port;
    bool port_set;

#ifdef _WIN32
    HDEVNOTIFY access;
#endif
};

#endif // AUTSERIALDETECT_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
