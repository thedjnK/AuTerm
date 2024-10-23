/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module: AutSerialDetect_linux.h
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
#ifndef AUTSERIALDETECT_LINUX_H
#define AUTSERIALDETECT_LINUX_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "AutSerialDetect_base.h"
#include <QObject>
#include <QTimer>
#include <QThread>
#include <unistd.h>
#include <libudev.h>

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
class AutSerialDetectWorkerThread;

enum FILE_DESCRIPTORS {
    FILE_DESCRIPTOR_READ,
    FILE_DESCRIPTOR_WRITE,
    FILE_DESCRIPTOR_UDEV,

    FILE_DESCRIPTOR_COUNT
};

/******************************************************************************/
// Class definitions
/******************************************************************************/
class AutSerialDetect : public AutSerialDetect_base
{
    Q_OBJECT

public:
    explicit AutSerialDetect(QObject *parent = nullptr);
    ~AutSerialDetect();
    void start(QString port);
    void stop();

private slots:
    void trigger();
    void thread_started(int fd);
    void thread_finished(bool device_found);

private:
    AutSerialDetectWorkerThread *serial_detect_thread;
    int worker_fd;
    QThread workerThread;
};

class AutSerialDetectWorkerThread : public QThread
{
    Q_OBJECT

public:
    void run() override;
    AutSerialDetectWorkerThread();
    ~AutSerialDetectWorkerThread();

public slots:
    void set_device(QString device);

signals:
    void started(int fd);
    void finished(bool device_found);

private:
    struct udev *udev;
    struct udev_device *dev;
    struct udev_monitor *mon;
    int fd[FILE_DESCRIPTOR_COUNT];
    QString expected_device;
};

#endif // AUTSERIALDETECT_LINUX_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
