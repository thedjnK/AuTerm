/******************************************************************************
** Copyright (C) 2023-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  nus_bluetooth_setup.cpp
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
#include "nus_bluetooth_setup.h"
#include "ui_nus_bluetooth_setup.h"

nus_bluetooth_setup::nus_bluetooth_setup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::nus_bluetooth_setup)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Widget);
    this->setParent(parent);
    connected = false;
    red_circle = nullptr;
    green_circle = nullptr;

#ifdef _WIN32
    /* Remove address selection on windows as it is useless - delete instead of
     * delete later as using delete later causes the GUI to mess up
     */
    delete ui->radio_address_type_public;
    delete ui->radio_address_type_random;
    delete ui->radio_address_type_default;
    delete ui->label_2;
    delete ui->horizontalLayout;
#endif
}

nus_bluetooth_setup::~nus_bluetooth_setup()
{
    delete ui;
}

void nus_bluetooth_setup::on_btn_refresh_clicked()
{
    emit refresh_devices();
}

void nus_bluetooth_setup::on_list_devices_itemDoubleClicked(QListWidgetItem *)
{
    //Request a connect, which will perform a disconnect (if connected) then a connect
    emit request_connect();
}

void nus_bluetooth_setup::clear_devices()
{
    ui->list_devices->clear();
}

void nus_bluetooth_setup::add_device(QString *data)
{
    ui->list_devices->addItem(*data);
}

void nus_bluetooth_setup::discovery_state(bool started)
{
}

void nus_bluetooth_setup::connection_state(bool connected)
{
    this->connected = connected;

    if (ui->list_devices->selectedItems().length() == 0 && connected == false)
    {
    }
    else if (connected == true)
    {
        if (green_circle != nullptr)
        {
            ui->label_status_image->setPixmap(*green_circle);
        }
    }

    if (connected == false)
    {
        if (red_circle != nullptr)
        {
            ui->label_status_image->setPixmap(*red_circle);
        }
    }
}

void nus_bluetooth_setup::on_list_devices_currentRowChanged(int row)
{
    if (row >= 0)
    {
        bool scanning;
        bool connecting;

        emit bluetooth_status(&scanning, &connecting);

        if (connecting != this->connected)
        {
            this->connected = connecting;
        }
    }
}

void nus_bluetooth_setup::set_status_text(QString status)
{
    ui->label_status->setText(status);
}

void nus_bluetooth_setup::load_pixmaps()
{
    if (red_circle == nullptr)
    {
        //Fetch pixmaps for green and red status circles
        emit plugin_get_image_pixmap("RedCircle", &red_circle);
        emit plugin_get_image_pixmap("GreenCircle", &green_circle);

        if (red_circle != nullptr)
        {
            ui->label_status_image->setPixmap(*red_circle);
        }
    }
}

bool nus_bluetooth_setup::connect()
{
    if (ui->list_devices->selectedItems().count() == 1)
    {
        bool scanning;
        bool connecting;

        emit bluetooth_status(&scanning, &connecting);

        if (connecting == true)
        {
            emit disconnect_from_device();
        }

        if (ui->list_devices->selectedItems().count() == 1)
        {
            ui->label_status->setText("Connecting...");
            emit connect_to_device(ui->list_devices->currentRow(),
#ifdef _WIN32
                                   BLUETOOTH_FORCE_ADDRESS_DEFAULT);
#else
                                   (ui->radio_address_type_public->isChecked() ? BLUETOOTH_FORCE_ADDRESS_PUBLIC : (ui->radio_address_type_random->isChecked() ? BLUETOOTH_FORCE_ADDRESS_RANDOM : BLUETOOTH_FORCE_ADDRESS_DEFAULT)));
#endif
        }

        return true;
    }

    return false;
}

#ifndef SKIPPLUGIN_LOGGER
void nus_bluetooth_setup::set_logger(debug_logger *object)
{
    logger = object;
}
#endif

/******************************************************************************/
// END OF FILE
/******************************************************************************/
