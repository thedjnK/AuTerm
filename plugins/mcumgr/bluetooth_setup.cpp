/******************************************************************************
** Copyright (C) 2023-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  bluetooth_setup.cpp
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
#include "bluetooth_setup.h"
#include "ui_bluetooth_setup.h"

bluetooth_setup::bluetooth_setup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::bluetooth_setup)
{
    ui->setupUi(this);

    //Always appear in front of AuTerm window
    this->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

    ui->btn_connect->setEnabled(false);
    ui->btn_refresh->setEnabled(false);

    connected = false;
    red_circle = nullptr;
    green_circle = nullptr;
}

bluetooth_setup::~bluetooth_setup()
{
    delete ui;
}

void bluetooth_setup::on_btn_refresh_clicked()
{
    emit refresh_devices();
}

void bluetooth_setup::on_btn_connect_clicked()
{
    bool scanning;
    bool connecting;

    emit bluetooth_status(&scanning, &connecting);

    if (connecting == false)
    {
        if (ui->list_devices->selectedItems().count() == 1)
        {
            ui->btn_connect->setEnabled(false);
            ui->label_status->setText("Connecting...");
            emit connect_to_device(ui->list_devices->currentRow(), (ui->radio_address_type_public->isChecked() ? BLUETOOTH_FORCE_ADDRESS_PUBLIC : (ui->radio_address_type_random->isChecked() ? BLUETOOTH_FORCE_ADDRESS_RANDOM : BLUETOOTH_FORCE_ADDRESS_DEFAULT)));
        }
    }
    else
    {
        emit disconnect_from_device();
    }
}

void bluetooth_setup::on_btn_close_clicked()
{
    this->close();
}

void bluetooth_setup::on_list_devices_itemDoubleClicked(QListWidgetItem *)
{
    on_btn_connect_clicked();
}

void bluetooth_setup::clear_devices()
{
    ui->list_devices->clear();
}

void bluetooth_setup::add_device(QString *data)
{
    ui->list_devices->addItem(*data);
}

void bluetooth_setup::discovery_state(bool started)
{
    ui->btn_refresh->setEnabled(!started);
}

void bluetooth_setup::connection_state(bool connected)
{
    ui->btn_refresh->setEnabled(!connected);
    ui->btn_connect->setText((connected == true ? "&Disconnect" : "&Connect"));
    this->connected = connected;

    if (ui->list_devices->selectedItems().length() == 0 && connected == false)
    {
        ui->btn_connect->setEnabled(false);
    }
    else if (connected == true)
    {
        ui->btn_connect->setEnabled(true);

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

void bluetooth_setup::on_list_devices_currentRowChanged(int row)
{
    if (row >= 0)
    {
        bool scanning;
        bool connecting;

        emit bluetooth_status(&scanning, &connecting);

        ui->btn_connect->setEnabled(true);

        if (connecting != this->connected)
        {
            ui->btn_connect->setText((connecting == true ? "&Disconnect" : "&Connect"));
            this->connected = connecting;
        }
    }
    else
    {
        ui->btn_connect->setEnabled(false);
    }
}

void bluetooth_setup::set_status_text(QString status)
{
    ui->label_status->setText(status);
}

void bluetooth_setup::load_pixmaps()
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

#ifndef SKIPPLUGIN_LOGGER
void bluetooth_setup::set_logger(debug_logger *object)
{
    logger = object;
}
#endif

/******************************************************************************/
// END OF FILE
/******************************************************************************/
