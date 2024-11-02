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

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QMainWindow>
#include <QSerialPort>
#include <QPushButton>

/******************************************************************************/
// Defines
/******************************************************************************/
#define AuTermPluginInterface_iid "org.AuTerm.PluginInterface"

/******************************************************************************/
// Class definitions
/******************************************************************************/
class AutPlugin
{
public:
    enum PluginType
    {
        Unknown,
        Feature,
        Transport,
    };

    virtual QWidget *GetWidget() = 0;
    virtual void setup(QMainWindow *main_window) = 0;
    virtual const QString plugin_about() = 0;
    virtual bool plugin_configuration() = 0;
    virtual void setup_finished()
    {
    }
    virtual PluginType plugin_type() = 0;
    virtual QObject *plugin_object() = 0;

signals:
    void show_message_box(QString str_message);
    void serial_transmit(QByteArray *data);
    void plugin_set_status(bool busy, bool hide_terminal_output);
    void plugin_add_open_close_button(QPushButton *button);
    void plugin_to_hex(QByteArray *data);
    void find_plugin(QString name, struct plugin_data *plugin);
};

Q_DECLARE_INTERFACE(AutPlugin, AuTermPluginInterface_iid)

//Transport plugin class, can be used for an alternative shell transport instead of UART
class AutTransportPlugin : public AutPlugin
{
public:
    enum StopBits
    {
        NoStop = 0,
        OneStop = QSerialPort::OneStop,
        OneAndHalfStop = QSerialPort::OneAndHalfStop,
        TwoStop = QSerialPort::TwoStop
    };

    virtual bool open(QIODeviceBase::OpenMode mode) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual QSerialPort::DataBits dataBits() const = 0;
    virtual StopBits stopBits() const = 0;
    virtual QSerialPort::Parity parity() const = 0;
    virtual qint64 write(const QByteArray &data) = 0;
    virtual qint64 bytesAvailable() const = 0;
    virtual QByteArray peek(qint64 maxlen) = 0;
    virtual QByteArray read(qint64 maxlen) = 0;
    virtual QByteArray readAll() = 0;
    virtual bool clear(QSerialPort::Directions directions = QSerialPort::AllDirections) = 0;
    virtual bool setBreakEnabled(bool set = true) = 0;
    virtual bool setRequestToSend(bool set) = 0;
    virtual bool setDataTerminalReady(bool set) = 0;
    virtual QSerialPort::PinoutSignals pinoutSignals() = 0;
    virtual QString to_error_string(int error) = 0;
    virtual QString transport_name() const = 0;
    virtual bool supports_break() const = 0;
    virtual bool supports_request_to_send() const = 0;
    virtual bool supports_data_terminal_ready() const = 0;

signals:
    void readyRead();
    void errorOccurred(int error);
    void bytesWritten(qint64 bytes);
    void aboutToClose();
};

Q_DECLARE_INTERFACE(AutTransportPlugin, AuTermPluginInterface_iid)

//Struct which holds plugin data when a plugin requests details on another plugin
struct plugin_data {
    const QObject *object;
    const AutPlugin *plugin;
    bool found;
};

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
    void plugin_set_status(bool busy, bool hide_terminal_output, bool *accepted);
    void find_plugin(QString name, plugin_data *plugin);
    void plugin_serial_transmit(QByteArray *data);
    void plugin_add_open_close_button(QPushButton *button);
    void plugin_serial_open_close(uint8_t mode);
    void plugin_serial_is_open(bool *open);
    void plugin_to_hex(QByteArray *data);
    void plugin_save_setting(QString name, QVariant data);
    void plugin_load_setting(QString name, QVariant *data, bool *found);
    void plugin_get_image_pixmap(QString name, QPixmap **pixmap);
};
#endif

#endif // AUTPLUGIN_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
