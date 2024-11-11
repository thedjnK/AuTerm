/******************************************************************************
** Copyright (C) 2023-2024 Jamie M.
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

    /* If the plugin has a widget, it should be returned here (optional) */
    virtual QWidget *plugin_widget()
    {
        return nullptr;
    }
    /* Called when plugin is initialised to pass main window object */
    virtual void setup(QMainWindow *main_window) = 0;
    /* Returns plugin information to display */
    virtual const QString plugin_about() = 0;
    /* Open plugin configuration (if supported), returns true if supported or false if not */
    virtual bool plugin_configuration() = 0;
    /* Called after all plugins have been initialised to make GUI changes or locate other plugins (optional) */
    virtual void setup_finished()
    {
    }
    /* Type of plugin, must be one specific type which is not unknown */
    virtual PluginType plugin_type() = 0;
    /* Returns the plugin QObject */
    virtual QObject *plugin_object() = 0;
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

    /* Called when transport is being initialised, tab is a dedicated tab in the config terminal where the transport configuration should be placed */
    virtual void transport_setup(QWidget *tab) = 0;
    /* Called when the transport should be opened */
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    virtual bool open(QIODeviceBase::OpenMode mode) = 0;
#else
    virtual bool open(QIODevice::OpenMode mode) = 0;
#endif
    /* Called when the transport should be closed */
    virtual void close() = 0;
    /* Called to see if the transport has been opened succesfully, return true if open or false otherwise */
    virtual bool isOpen() const = 0;
    /* Called to see if the transport is being opened but is not yet ready for data transmission, return true if opening or false otherwise */
    virtual bool isOpening() const = 0;
    /* Called to get the number of data bits that the transport uses, also used for speed test feature */
    virtual QSerialPort::DataBits dataBits() const = 0;
    /* Called to get the number of stop bits that the transport uses, also used for speed test feature */
    virtual StopBits stopBits() const = 0;
    /* Called to get the parity that the transport uses, also used for speed test feature */
    virtual QSerialPort::Parity parity() const = 0;
    /* Called to write data to the send buffer and out of the transport, return value is the number of bytes written but is unused, so can return 0 */
    virtual qint64 write(const QByteArray &data) = 0;
    /* Called to check how much data is in the receive buffer, returns size in bytes */
    virtual qint64 bytesAvailable() const = 0;
    /* Called to read data from without remove it from the receive buffer, up to length maxlen bytes */
    virtual QByteArray peek(qint64 maxlen) = 0;
    /* Called to read data from and remove it from the receive buffer, up to length maxlen bytes */
    virtual QByteArray read(qint64 maxlen) = 0;
    /* Called to read all data from and remove it from the receive buffer */
    virtual QByteArray readAll() = 0;
    /* Called to clear data from the send buffer, receive buffer of both buffers */
    virtual bool clear(QSerialPort::Directions directions = QSerialPort::AllDirections) = 0;
    /* Called when BREAK should be enabled or disabled on the transport, true = asserted and false = deasserted */
    virtual bool setBreakEnabled(bool set = true)
    {
        Q_UNUSED(set);
        return false;
    }
    /* Called when request to send should be enabled or disabled on the transport, true = asserted and false = deasserted */
    virtual bool setRequestToSend(bool set)
    {
        Q_UNUSED(set);
        return false;
    }
    /* Called when data terminal ready should be enabled or disabled on the transport, true = asserted and false = deasserted */
    virtual bool setDataTerminalReady(bool set)
    {
        Q_UNUSED(set);
        return false;
    }
    /* Returns the status of pins or signals equivalent to RS232 status, return `QSerialPort::NoSignal` if not supported */
    virtual QSerialPort::PinoutSignals pinoutSignals() = 0;
    /* Returns a string of what the specified error means (which is raised in the `errorOccurred` signal) */
    virtual QString to_error_string(int error) = 0;
    /* Returns a string which is the name of the transport which is displayed in the GUI */
    virtual QString transport_name() const = 0;
    /* If plugin supports an equivalent to the RS232 BREAK feature, return true if supported (optional) */
    virtual bool supports_break()
    {
        return false;
    }
    /* If plugin supports an equivalent to the RS232 request to send feature, return true if supported (optional) */
    virtual bool supports_request_to_send()
    {
        return false;
    }
    /* If plugin supports an equivalent to the RS232 data terminal ready feature, return true if supported (optional) */
    virtual bool supports_data_terminal_ready()
    {
        return false;
    }
    /* Returns a string which is the string which should be displayed in the GUI for the active connection (when transport is connected) */
    virtual QString connection_display_name() = 0;

signals:
    /* Transport should emit this when data is waiting in the receive buffer */
    void readyRead();
    /* Transport should emit this when there has been an error and the transport is closing (error is a transport specific error code where 0 = no error (and will not close the transport), the errors must be mapped in `to_error_string`) */
    //TODO: not used yet
    void errorOccurred(int error);
    /* Transport should emit this when data from the send buffer has been successfully written to the target device */
    void bytesWritten(qint64 bytes);
    /* Transport should emit this when the transport is about to close */
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
    /* Emitted when a transport has received data (only processed when transport has been successfully locked using `plugin_set_status`) */
    void plugin_serial_receive(QByteArray *data);
    /* Emitted when a serial port error has occurred and the transport has been closed */
    //TODO: deal with other transport errors
    void plugin_serial_error(QSerialPort::SerialPortError speErrorCode);
    /* Emitted when bytes have been successfully written to the target device from the send buffer, intByteCount = bytes written */
    void plugin_serial_bytes_written(qint64 intByteCount);
    /* Emitted when a transport is about to close */
    void plugin_serial_about_to_close();
    /* Emitted when a transport has opened or is opening */
    void plugin_serial_opened();
    /* Emitted when a transport has closed */
    void plugin_serial_closed();

public slots:
    /* Used to request mutual access to the transport/serial port, this must be used before sending and to get received data, busy = true if requesting lock or false to return lock, hide_terminal_output = true if data transmit/receive should not be displayed in the terminal view or flase to show, accepted = if lock was granted, true if so or false if lock was not acquired */
    void plugin_set_status(bool busy, bool hide_terminal_output, bool *accepted);
    /* Used to find a plugin with the specified name, plugin will be the plugin if found, or nullptr if not */
    void find_plugin(QString name, plugin_data *plugin);
    /* Used to transmit data to the opened transport/serial port */
    void plugin_serial_transmit(QByteArray *data);
    /* Used to convert the passed push button into a button that can be used to open/close the selected transport */
    void plugin_add_open_close_button(QPushButton *button);
    /* Used to open or close the transport, mode = 0 to open transport, 1 to close transport */
    void plugin_serial_open_close(uint8_t mode);
    /* Used to check if a transport is open, open will be updated with true if it is or false otherwise */
    void plugin_serial_is_open(bool *open);
    /* Used to convert hex data into hex-encoded ASCII representation (which can be displayed to the user) */
    void plugin_to_hex(QByteArray *data);
    /* Used to save a plugin-specific setting, name is the value of the configuration item (unique per transport) and data is the setting */
    void plugin_save_setting(QString name, QVariant data);
    /* Used to load a plugin-specific setting, name is the value of the configuration item (unique per transport), if found then data will be updated with the value and found will be set true, otherwise found will be false */
    void plugin_load_setting(QString name, QVariant *data, bool *found);
    /* Used to get an image pixmap from the resources file, supported names = EmptyCircle, RedCircle, GreenCircle, AuTerm16. If found, pixmap will be updated with the pointer to the pixmap data, note: this is pixmap data as used by the main window, do not delete it when cleaning up as the main window will do that */
    void plugin_get_image_pixmap(QString name, QPixmap **pixmap);
    /* Used to force an update of the pinout status in the GUI (useful for transports that require time to open after they have opened successfully) */
    void plugin_force_image_update();
    /* Used to open the message box with the specified message */
    void plugin_show_message_box(QString message);
    /* Used by transport plugins to raise an error */
    //TODO: move to errorOccurred
    void plugin_transport_error(int error);
};
#endif

#endif // AUTPLUGIN_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
