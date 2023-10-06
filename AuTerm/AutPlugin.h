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
    virtual void setup_finished()
    {
    }
    virtual void found_plugin(QObject *plugin)
    {
    }

signals:
    void show_message_box(QString str_message);
//#ifdef INCLUDE_SERIALPORT
    void serial_transmit(QByteArray *data);
//#endif
    void plugin_set_status(bool busy, bool hide_terminal_output);
//#ifdef INCLUDE_UI
    void plugin_add_open_close_button(QPushButton *button);
//#endif
    void plugin_to_hex(QByteArray *data);
    void find_plugin(QString name);
};

Q_DECLARE_INTERFACE(AutPlugin, AuTermPluginInterface_iid)

#ifndef AUTERM_APPLICATION
//Plugin-friendly version of main window class
class AutMainWindow
{
signals:
    void plugin_serial_receive(QByteArray *data);
    void plugin_serial_error(QSerialPort::SerialPortError speErrorCode);
    void plugin_serial_bytes_written(qint64 intByteCount);
    void plugin_serial_about_to_close();
    void plugin_serial_opened();
    void plugin_serial_closed();

public slots:
    void
    plugin_serial_transmit(
        QByteArray *data
        );
    void
    plugin_add_open_close_button(
        QPushButton *button
        );
    void
    plugin_to_hex(
        QByteArray *data
        );
    void plugin_save_setting(QString name, QVariant data);
    void plugin_load_setting(QString name, QVariant *data, bool *found);
};
#endif

#endif // AUTPLUGIN_H
