/******************************************************************************
** Copyright (C) 2023 Jamie M.
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
    if (ui->list_devices->selectedItems().count() == 1)
    {
        emit connect_to_device(ui->list_devices->currentRow());
    }
}

void bluetooth_setup::on_btn_disconnect_clicked()
{
    emit disconnect_from_device();
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

void bluetooth_setup::add_debug(QString data)
{
    ui->edit_debug->appendPlainText(data);
}

void bluetooth_setup::on_btn_min_params_clicked()
{
    emit min_params();
}

