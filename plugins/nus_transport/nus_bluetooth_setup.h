/******************************************************************************
** Copyright (C) 2023-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  nus_bluetooth_setup.h
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
#ifndef NUS_BLUETOOTH_SETUP_H
#define NUS_BLUETOOTH_SETUP_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QWidget>
#include <QListWidgetItem>
#include <debug_logger.h>

/******************************************************************************/
// Enum typedefs
/******************************************************************************/
enum BLUETOOTH_FORCE_ADDRESS {
    BLUETOOTH_FORCE_ADDRESS_DEFAULT,
    BLUETOOTH_FORCE_ADDRESS_RANDOM,
    BLUETOOTH_FORCE_ADDRESS_PUBLIC,

    BLUETOOTH_FORCE_ADDRESS_COUNT
};

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class nus_bluetooth_setup;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class nus_bluetooth_setup : public QWidget
{
    Q_OBJECT

public:
    explicit nus_bluetooth_setup(QWidget *parent = nullptr);
    ~nus_bluetooth_setup();
    void clear_devices();
    void add_device(QString *data);
    void discovery_state(bool started);
    void connection_state(bool connected);
    void set_status_text(QString status);
    void load_pixmaps();
    bool connect();
#ifndef SKIPPLUGIN_LOGGER
    void set_logger(debug_logger *object);
#endif

private slots:
    void on_btn_refresh_clicked();
    void on_list_devices_itemDoubleClicked(QListWidgetItem *item);
    void on_list_devices_currentRowChanged(int row);

signals:
    void refresh_devices();
    void connect_to_device(uint16_t index, uint8_t address_type);
    void disconnect_from_device();
    void bluetooth_status(bool *scanning, bool *connecting);
    void plugin_get_image_pixmap(QString name, QPixmap **pixmap);

private:
    Ui::nus_bluetooth_setup *ui;
    QPixmap *red_circle;
    QPixmap *green_circle;
    bool connected;
#ifndef SKIPPLUGIN_LOGGER
    debug_logger *logger;
#endif
};

#endif // NUS_BLUETOOTH_SETUP_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
