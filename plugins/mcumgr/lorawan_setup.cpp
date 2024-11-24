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

/******************************************************************************/
// Constants
/******************************************************************************/
static const bool default_save_history = false;
static const uint16_t max_history = 10;
static const uint16_t history_tokens = 5;
static const uint16_t history_elements = 6;

enum HISTORY_ENTRY {
    HISTORY_ENTRY_ADDRESS,
    HISTORY_ENTRY_PORT,
    HISTORY_ENTRY_TLS,
    HISTORY_ENTRY_USERNAME,
    HISTORY_ENTRY_PASSWORD,
    HISTORY_ENTRY_TOPIC,

    HISTORY_ENTRY_COUNT
};

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
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

    if (ui->check_save_history->isChecked() == true)
    {
        add_to_history();
    }
}

void lorawan_setup::on_btn_close_clicked()
{
    this->close();
}

void lorawan_setup::on_btn_clear_history_clicked()
{
    ui->combo_history->clear();
    emit plugin_save_setting("mcumgr_lorawan_history", QStringList());
    ui->combo_history->addItem("");
    this->saved_history.clear();
}

void lorawan_setup::on_check_save_history_toggled(bool checked)
{
    ui->combo_history->setEnabled(checked);
    ui->btn_clear_history->setEnabled(checked);
    emit plugin_save_setting("mcumgr_lorawan_save_history", checked);
}

void lorawan_setup::on_combo_history_currentIndexChanged(int index)
{
    QStringList split_data;

    if (index == 0 && ui->combo_history->count() > 1)
    {
        ui->edit_address->clear();
        ui->edit_port->setValue(8883);
        ui->check_tls->setChecked(true);
        ui->edit_username->clear();
        ui->edit_password->clear();
        ui->edit_topic->clear();
        return;
    }
    else if (index <= 0)
    {
        return;
    }

    --index;

    if (index >= saved_history.count())
    {
        log_error() << "Selected LoRaWAN history entry does not exist: " << index;
        return;
    }

    split_data = saved_history.at(index).split(':');
    ui->edit_address->setText(split_data.at(HISTORY_ENTRY_ADDRESS));
    ui->edit_port->setValue(split_data.at(HISTORY_ENTRY_PORT).toUInt());
    ui->check_tls->setChecked(split_data.at(HISTORY_ENTRY_TLS).toUInt());
    ui->edit_username->setText(split_data.at(HISTORY_ENTRY_USERNAME));
    ui->edit_password->setText(split_data.at(HISTORY_ENTRY_PASSWORD));
    ui->edit_topic->setText(split_data.at(HISTORY_ENTRY_TOPIC));
}

void lorawan_setup::load_settings()
{
    QVariant data;
    bool found = false;

    emit plugin_load_setting("mcumgr_lorawan_save_history", &data, &found);

    if (found == true)
    {
        if (data.toBool() == true)
        {
            ui->check_save_history->setChecked(true);
            emit plugin_load_setting("mcumgr_lorawan_history", &data, &found);

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
                        display_items.append(split_item.at(0) % ":" % split_item.at(1));
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
    else
    {
        emit plugin_save_setting("mcumgr_lorawan_save_history", default_save_history);

        if (ui->check_save_history->isChecked() != default_save_history)
        {
            ui->check_save_history->setChecked(default_save_history);
        }
    }
}

void lorawan_setup::add_to_history()
{
    int items = ui->combo_history->count();
    uint8_t tokens = 0;
    int last_token = 0;
    QString full_data = ui->edit_address->text() % ":" % QString::number(ui->edit_port->value()) % ":" % QString::number(ui->check_tls->isChecked()) % ":" % ui->edit_username->text() % ":" % ui->edit_password->text() % ":" % ui->edit_topic->text();
    QString display_data = ui->edit_address->text() % ":" % QString::number(ui->edit_port->value());

    //Ensure there are no : or , elements in any fields as these are used for element delimiters and list delimiters
    while (last_token != -1)
    {
        last_token = full_data.indexOf(':', last_token);

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

    if (full_data.indexOf(',', last_token) != -1)
    {
        return;
    }

    while (items > 0)
    {
        --items;

        if (ui->combo_history->itemText(items) == display_data)
        {
            //Item already in history, update saved history if setting differs by moving position
            if (this->saved_history.at((items - 1)) != full_data)
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

    ui->combo_history->addItem(display_data);
    this->saved_history.append(full_data);

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

    emit plugin_save_setting("mcumgr_lorawan_history", this->saved_history);
    ui->combo_history->setCurrentIndex((ui->combo_history->count() - 1));
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

#ifndef SKIPPLUGIN_LOGGER
void lorawan_setup::set_logger(debug_logger *object)
{
    logger = object;
}
#endif

void lorawan_setup::set_status_text(QString status)
{
    ui->label_status_text->setText(status);
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
