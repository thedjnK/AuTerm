/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module: AutSerialDetect_linux.cpp
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
#include "AutSerialDetect_linux.h"
#include <QSerialPortInfo>
#include <QWidget>
#include <QDebug>

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
AutSerialDetect::AutSerialDetect(QObject *parent): AutSerialDetect_base{parent}
{
    port_set = false;
    worker_fd = 0;

    serial_detect_thread = new AutSerialDetectWorkerThread();
    connect(serial_detect_thread, SIGNAL(finished(bool)), this, SLOT(thread_finished(bool)));
    connect(serial_detect_thread, SIGNAL(started(int)), this, SLOT(thread_started(int)));
}

AutSerialDetect::~AutSerialDetect()
{
    stop();

    if (worker_fd != 0)
    {
        close(worker_fd);
        worker_fd = 0;
    }

    disconnect(this, SLOT(thread_finished(bool)));
    disconnect(this, SLOT(thread_started(int)));

    if (serial_detect_thread != nullptr)
    {
        delete(serial_detect_thread);
        serial_detect_thread = nullptr;
    }
}

void AutSerialDetect::start(QString port)
{
    if (serial_detect_thread->isRunning() == true || port_set == true)
    {
        stop();
    }

    serial_detect_thread->start();
    QMetaObject::invokeMethod(serial_detect_thread, "set_device", Qt::QueuedConnection, Q_ARG(QString, port));

    watch_port = port;
    port_set = true;
}

void AutSerialDetect::stop()
{
    if (serial_detect_thread->isRunning())
    {
        FILE *stream;
        stream = fdopen(worker_fd, "w");
        fprintf(stream, "\n");
        fclose(stream);

        if (worker_fd != 0)
        {
           close(worker_fd);
           worker_fd = 0;
        }

        serial_detect_thread->quit();
        serial_detect_thread->wait(QDeadlineTimer::Forever);
    }

    if (port_set == false)
    {
        return;
    }

    port_set = false;
    watch_port.clear();
}

void AutSerialDetect::trigger()
{
    FILE *stream;

    if (worker_fd != 0)
    {
        stream = fdopen(worker_fd, "w");
        fprintf(stream, "\n");
        fclose(stream);
        close(worker_fd);
    }
}

void AutSerialDetect::thread_started(int fd)
{
    worker_fd = fd;
}

void AutSerialDetect::thread_finished(bool device_found)
{
    serial_detect_thread->quit();
    serial_detect_thread->wait(QDeadlineTimer::Forever);

    if (worker_fd != 0)
    {
        close(worker_fd);
        worker_fd = 0;
    }

    if (device_found == true)
    {
        emit port_reconnected(watch_port);
    }

    port_set = false;
    watch_port.clear();
}

void AutSerialDetectWorkerThread::run()
{
    FILE *stream;
    bool device_found = false;
    int select_fd;

    //Create pipe between worker thread and main window
    pipe(fd);
    emit started(fd[FILE_DESCRIPTOR_WRITE]);

    //Setup udev for monitoring
    udev = udev_new();

    if (udev == nullptr)
    {
        qDebug() << "udev_new creation failed";
        goto finish;
    }

    //Filter only on tty dfevices from udev
    mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "tty", NULL);
    udev_monitor_enable_receiving(mon);
    fd[FILE_DESCRIPTOR_UDEV] = udev_monitor_get_fd(mon);

    //Open receiver file descriptor with main window for communication of thread termination
    stream = fdopen(fd[FILE_DESCRIPTOR_READ], "r");

    select_fd = (fd[FILE_DESCRIPTOR_UDEV] > fd[FILE_DESCRIPTOR_READ] ? fd[FILE_DESCRIPTOR_UDEV] : fd[FILE_DESCRIPTOR_READ]) + 1;

    while (1) {
        fd_set fds;
        int ret;

        FD_ZERO(&fds);
        FD_SET(fd[FILE_DESCRIPTOR_UDEV], &fds);
        FD_SET(fd[FILE_DESCRIPTOR_READ], &fds);

        ret = select(select_fd, &fds, NULL, NULL, NULL);

        if (ret > 0)
        {
            if (FD_ISSET(fd[FILE_DESCRIPTOR_UDEV], &fds))
            {
                dev = udev_monitor_receive_device(mon);

                if (dev)
                {
                    //Only process "add" events where the port name matches the port we are expecting
                    const char *device_action = udev_device_get_action(dev);
                    const char *device_name = udev_device_get_sysname(dev);

                    if (device_action != nullptr && strcmp(device_action, "add") == 0 && expected_device == QLatin1String(device_name))
                    {
                        device_found = true;
                    }

                    udev_device_unref(dev);
                    dev = nullptr;

                    if (device_found == true)
                    {
                        break;
                    }
                }
            }
            else if (FD_ISSET(fd[FILE_DESCRIPTOR_READ], &fds))
            {
                //Read dummy data and exit to allow thread to terminate gracefully
                while (fgetc(stream) != EOF);
                break;
            }
        }
    }

    //Cleanup of read file descriptor with main window, the write one will be cleaned up by the main window
    fclose(stream);
    close(fd[FILE_DESCRIPTOR_READ]);
    fd[FILE_DESCRIPTOR_READ] = 0;

    udev_monitor_unref(mon);
    udev_unref(udev);
    mon = nullptr;
    udev = nullptr;

finish:
    emit finished(device_found);
}

void AutSerialDetectWorkerThread::set_device(QString device)
{
    expected_device = device;
}

AutSerialDetectWorkerThread::AutSerialDetectWorkerThread()
{
    udev = nullptr;
    dev = nullptr;
    mon = nullptr;
}

AutSerialDetectWorkerThread::~AutSerialDetectWorkerThread()
{
    if (dev != nullptr)
    {
        udev_device_unref(dev);
        dev = nullptr;
    }

    if (mon != nullptr)
    {
        udev_monitor_unref(mon);
        mon = nullptr;
    }

    if (udev != nullptr)
    {
        udev_unref(udev);
        udev = nullptr;
    }

    if (fd[FILE_DESCRIPTOR_READ] != 0)
    {
        close(fd[FILE_DESCRIPTOR_READ]);
        fd[FILE_DESCRIPTOR_READ] = 0;
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
