/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  AutPlugin.h
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
#ifndef AUTPLUGIN_H
#define AUTPLUGIN_H

#include <QMainWindow>
#include <QSerialPort>
#include <QPushButton>

#define AuTermPluginInterface_iid "org.AuTerm.PluginInterface"

class AutPlugin
{
public:
    virtual QWidget *GetWidget() = 0;
    virtual void setup(QMainWindow *main_window) = 0;
    virtual const QString plugin_about() = 0;
    virtual bool plugin_configuration() = 0;
    virtual void serial_receive(QByteArray *data) = 0;
    virtual void serial_error(QSerialPort::SerialPortError speErrorCode) = 0;
    virtual void serial_bytes_written(qint64 intByteCount) = 0;
    virtual void serial_about_to_close() = 0;
    virtual void serial_opened() = 0;
    virtual void serial_closed() = 0;

signals:
    void show_message_box(QString str_message);
    void serial_transmit(QByteArray *data);
    void plugin_set_status(bool busy, bool hide_terminal_output);
    void plugin_add_open_close_button(QPushButton *button);
    void plugin_to_hex(QByteArray *data);
};

Q_DECLARE_INTERFACE(AutPlugin, AuTermPluginInterface_iid)

#endif // AUTPLUGIN_H
