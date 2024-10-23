/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module: AutSerialDetect_base.h
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
#ifndef AUTSERIALDETECT_BASE_H
#define AUTSERIALDETECT_BASE_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>

/******************************************************************************/
// Class definitions
/******************************************************************************/
class AutSerialDetect_base : public QObject
{
    Q_OBJECT

public:
    explicit AutSerialDetect_base(QObject *parent = nullptr): QObject{parent}
    {
    }
    virtual void start(QString port) = 0;
    virtual void stop() = 0;

signals:
    void port_reconnected(QString port);

protected:
    QString watch_port;
    bool port_set;
};

#endif // AUTSERIALDETECT_BASE_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
