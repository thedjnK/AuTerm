/******************************************************************************
** Copyright (C) 2023-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  udp_setup.cpp
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
#include "udp_setup.h"
#include "ui_udp_setup.h"

/******************************************************************************/
// Constants
/******************************************************************************/
static const uint16_t max_history = 10;
static const uint16_t history_tokens = 1;
static const uint16_t history_elements = 2;

enum HISTORY_ENTRY {
    HISTORY_ENTRY_ADDRESS,
    HISTORY_ENTRY_PORT,

    HISTORY_ENTRY_COUNT
};

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
udp_setup::udp_setup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::udp_setup)
{
    ui->setupUi(this);

    //Always appear in front of AuTerm window
    this->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

    red_circle = nullptr;
    green_circle = nullptr;
}

udp_setup::~udp_setup()
{
    delete ui;
}

void udp_setup::on_btn_connect_clicked()
{
    bool connected;

    emit is_connected(&connected);

    if (connected == true)
    {
        emit disconnect_from_device();

        if (red_circle != nullptr)
        {
            ui->label_status_image->setPixmap(*red_circle);
        }

        ui->label_status->setText("Not connected");
        ui->btn_connect->setText("&Connect");
        return;
    }

    if (ui->edit_address->text().length() == 0)
    {
        return;
    }

    emit connect_to_device(ui->edit_address->text(), ui->edit_port->value());

    if (green_circle != nullptr)
    {
        ui->label_status_image->setPixmap(*green_circle);
    }

    ui->label_status->setText("Connected");
    ui->btn_connect->setText("&Disconnect");

    if (ui->check_save_history->isChecked() == true)
    {
        add_to_history();
    }
}

void udp_setup::on_btn_close_clicked()
{
    this->close();
}

void udp_setup::on_btn_clear_history_clicked()
{
    ui->combo_history->clear();
    emit plugin_save_setting("mcumgr_udp_history", QStringList());
    ui->combo_history->addItem("");
    this->saved_history.clear();
}

void udp_setup::on_check_save_history_toggled(bool checked)
{
    ui->combo_history->setEnabled(checked);
    ui->btn_clear_history->setEnabled(checked);
    emit plugin_save_setting("mcumgr_udp_save_history", checked);
}

void udp_setup::on_combo_history_currentIndexChanged(int index)
{
    QStringList split_data;

    if (index == 0 && ui->combo_history->count() > 1)
    {
        ui->edit_address->clear();
        ui->edit_port->setValue(8883);
        return;
    }
    else if (index <= 0)
    {
        return;
    }

    --index;

    if (index >= saved_history.count())
    {
        log_error() << "Selected UDP history entry does not exist: " << index;
        return;
    }

    split_data = saved_history.at(index).split(':');
    ui->edit_address->setText(split_data.at(HISTORY_ENTRY_ADDRESS));
    ui->edit_port->setValue(split_data.at(HISTORY_ENTRY_PORT).toUInt());
}

void udp_setup::load_settings()
{
    QVariant data;
    bool found = false;

    emit plugin_load_setting(QString("mcumgr_udp_save_history"), &data, &found);

    if (found == true)
    {
        if (data.toBool() == true)
        {
            ui->check_save_history->setChecked(true);
            emit plugin_load_setting("mcumgr_udp_history", &data, &found);

            if (found == true)
            {
                QStringList display_items;
                uint8_t i = 0;

                this->saved_history = data.toStringList();

                if (this->saved_history.count() > max_history)
                {
                    this->saved_history.remove(0, (this->saved_history.count() - max_history));
                }

                while (i < this->saved_history.length())
                {
                    QStringList split_item = this->saved_history.at(i).split(':');

                    if (split_item.length() == history_elements)
                    {
                        display_items.append(this->saved_history.at(i));
                    }
                    else
                    {
                        this->saved_history.remove(i);
                        --i;
                    }

                    ++i;
                }

                ui->combo_history->addItems(display_items);
            }
        }
        else
        {
            ui->check_save_history->setChecked(false);
            ui->combo_history->setEnabled(false);
        }
    }
}

void udp_setup::add_to_history()
{
    int items = ui->combo_history->count();
    uint8_t tokens = 0;
    int last_token = 0;
    QString data = QString("%1:%2").arg(ui->edit_address->text(), QString::number(ui->edit_port->value()));

    //Ensure there are no : or , elements in any fields as these are used for element delimiters and list delimiters
    while (last_token != -1)
    {
        last_token = data.indexOf(':', last_token);

        if (last_token != -1)
        {
            ++last_token;
            ++tokens;

            if (tokens > history_tokens)
            {
                return;
            }
        }
    }

    if (tokens != history_tokens)
    {
        return;
    }

    if (data.indexOf(',', last_token) != -1)
    {
        return;
    }

    while (items > 0)
    {
        --items;

        if (ui->combo_history->itemText(items) == data)
        {
            //Item already in history, update saved history if setting differs by moving position
            if (this->saved_history.at((items - 1)) != data)
            {
                ui->combo_history->removeItem(items);
                this->saved_history.remove((items - 1));
                break;
            }
            else
            {
                ui->combo_history->setCurrentIndex(items);
                return;
            }
        }
    }

    ui->combo_history->addItem(data);
    ui->combo_history->setCurrentIndex((ui->combo_history->count() - 1));
    this->saved_history.append(data);

    items = this->saved_history.count();

    if (items > max_history)
    {
        this->saved_history.remove(0, (items - max_history));

        while (items > max_history)
        {
            ui->combo_history->removeItem(1);
            --items;
        }
    }

    emit plugin_save_setting("mcumgr_udp_history", this->saved_history);
}

void udp_setup::load_pixmaps()
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
void udp_setup::set_logger(debug_logger *object)
{
    logger = object;
}
#endif

/******************************************************************************/
// END OF FILE
/******************************************************************************/
