/******************************************************************************
** Copyright (C) 2015-2018 Laird
**
** Project: AuTerm
**
** Module: LrdLogger.h
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
#ifndef LRDLOGGER_H
#define LRDLOGGER_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QWidget>
#include <QFile>
#include <QDataStream>

/******************************************************************************/
// Constants
/******************************************************************************/
const qint8 LOG_OK               = 0; //Success
const qint8 LOG_ERR_OPEN_ALREADY = 1; //Log already open
const qint8 LOG_ERR_ACCESS       = 2; //Access denied to log file
const qint8 LOG_NOT_OPEN         = 3; //Log file not open

/******************************************************************************/
// Class definitions
/******************************************************************************/
class LrdLogger : public QWidget
{
    Q_OBJECT
public:
    explicit LrdLogger(
        QWidget *parent = 0
        );
    ~LrdLogger(
        );
    unsigned char
    OpenLogFile(
        QString strFilename
        );
    void
    CloseLogFile(
        );
    unsigned char
    WriteLogData(
        QString strData
        );
    unsigned char
    WriteRawLogData(
        QByteArray baData
        );
    unsigned short
    GetLogSize(
        );
    void
    ClearLog(
        );
    QString
    GetLogName(
        );
    bool
    IsLogOpen(
        );

private:
    bool mbLogOpen; //True when log file is open
    QFile *mpLogFile; //Contains the handle of log file
    QDataStream *mpStreamOut; //Contains the handle to the data steam
};

#endif // LRDLOGGER_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
