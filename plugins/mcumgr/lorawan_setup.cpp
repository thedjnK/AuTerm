/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module:  lorawan_setup.cpp
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
#include "lorawan_setup.h"
#include "ui_lorawan_setup.h"

lorawan_setup::lorawan_setup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::lorawan_setup)
{
    ui->setupUi(this);

    //Always appear in front of AuTerm window
    this->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

    red_circle = nullptr;
    green_circle = nullptr;
}

lorawan_setup::~lorawan_setup()
{
    delete ui;
}

void lorawan_setup::on_btn_connect_clicked()
{
    if (is_connected == true)
    {
        emit disconnect_from_service();
        return;
    }

    if (ui->edit_address->text().length() == 0)
    {
        return;
    }

    emit connect_to_service(ui->edit_address->text(), ui->edit_port->value(), ui->check_tls->isChecked(), ui->edit_username->text(), ui->edit_password->text(), ui->edit_topic->text());

#if 0
    if (ui->check_save_history->isChecked() == true)
    {
        add_to_history();
    }
#endif
}

void lorawan_setup::on_btn_close_clicked()
{
    this->close();
}

void lorawan_setup::on_btn_clear_history_clicked()
{
#if 0
    ui->combo_history->clear();
    emit plugin_save_setting("mcumgr_lora_history", QStringList());
    ui->combo_history->addItem("");
#endif
}

void lorawan_setup::on_check_save_history_toggled(bool checked)
{
#if 0
    ui->combo_history->setEnabled(checked);
    ui->btn_clear_history->setEnabled(checked);

    emit plugin_save_setting("mcumgr_lora_save_history", checked);
#endif
}

void lorawan_setup::on_combo_history_currentIndexChanged(int index)
{
#if 0
    if (index == 0)
    {
        return;
    }

    QString data = ui->combo_history->itemText(index);

    ui->edit_address->setText(data.left(data.lastIndexOf(":")));
    ui->edit_port->setValue(data.mid((data.lastIndexOf(":") + 1), (data.length() - data.lastIndexOf(":") - 1)).toUInt());
#endif
}

void lorawan_setup::load_settings()
{
#if 0
    QVariant data;
    bool found = false;

    emit plugin_load_setting(QString("mcumgr_lora_save_history"), &data, &found);

    if (found == true)
    {
        if (data.toBool() == true)
        {
            emit plugin_load_setting("mcumgr_lora_history", &data, &found);

            if (found == true)
            {
                ui->combo_history->addItems(data.toStringList());
            }
        }
        else
        {
            ui->check_save_history->setChecked(false);
            ui->combo_history->setEnabled(false);
        }
    }
#endif
}

void lorawan_setup::add_to_history()
{
#if 0
    QStringList history;
    int items = ui->combo_history->count();
    QString data = QString("%1:%2").arg(ui->edit_address->text(), QString::number(ui->edit_port->value()));

    while (items > MAX_LORAWAN_HISTORY)
    {
        --items;

        ui->combo_history->removeItem(items);
    }

    while (items > 0)
    {
        if (ui->combo_history->itemText(items) == data)
        {
            //Item already in history
            ui->combo_history->setCurrentIndex(items);
            return;
        }

        --items;
    }

    ui->combo_history->insertItem(1, data);
    ui->combo_history->setCurrentIndex(0);

    items = ui->combo_history->count() - 1;

    while (items > 0)
    {
        history.append(ui->combo_history->itemText(items));

        --items;
    }

    emit plugin_save_setting("mcumgr_lora_history", history);
#endif
}

bool lorawan_setup::get_confirmed_downlinks()
{
    return ui->check_confirmed_downlinks->isChecked();
}

uint8_t lorawan_setup::get_frame_port()
{
    return ui->edit_frame_port->value();
}

uint16_t lorawan_setup::get_fragment_size()
{
    return ui->edit_fragment_size->value();
}

bool lorawan_setup::get_auto_fragment_size()
{
    return ui->check_auto_adjust_size->isChecked();
}

void lorawan_setup::set_connection_options_enabled(bool enabled)
{
    ui->combo_history->setEnabled(enabled);
    ui->edit_address->setEnabled(enabled);
    ui->edit_port->setEnabled(enabled);
    ui->check_tls->setEnabled(enabled);
    ui->edit_username->setEnabled(enabled);
    ui->edit_password->setEnabled(enabled);
    ui->edit_topic->setEnabled(enabled);
}

void lorawan_setup::load_pixmaps()
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

void lorawan_setup::set_connection_state(bool connected)
{
    is_connected = connected;

    if (is_connected == true)
    {
        ui->btn_connect->setText("Disconnect");

        if (green_circle != nullptr)
        {
            ui->label_status_image->setPixmap(*green_circle);
        }

        ui->label_status_text->setText("Connected");
    }
    else
    {
        ui->btn_connect->setText("Connect");

        if (red_circle != nullptr)
        {
            ui->label_status_image->setPixmap(*red_circle);
        }

        ui->label_status_text->setText("Disconnected");
    }
}

uint8_t lorawan_setup::get_resends()
{
    return ui->edit_resends->value();
}

uint32_t lorawan_setup::get_timeout()
{
    return ui->edit_timeout->value() * 1000;
}
