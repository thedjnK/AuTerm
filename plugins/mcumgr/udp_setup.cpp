/******************************************************************************
** Copyright (C) 2023 Jamie M.
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
#include "udp_setup.h"
#include "ui_udp_setup.h"

udp_setup::udp_setup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::udp_setup)
{
    ui->setupUi(this);

    //Always appear in front of AuTerm window
    this->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
}

udp_setup::~udp_setup()
{
    delete ui;
}

void udp_setup::on_btn_connect_clicked()
{
    if (ui->edit_address->text().length() == 0)
    {
        return;
    }

    emit connect_to_device(ui->edit_address->text(), ui->edit_port->value());

    this->close();

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
}

void udp_setup::on_check_save_history_toggled(bool checked)
{
    ui->combo_history->setEnabled(checked);
    ui->btn_clear_history->setEnabled(checked);

    emit plugin_save_setting("mcumgr_udp_save_history", checked);
}

void udp_setup::on_combo_history_currentIndexChanged(int index)
{
    if (index == 0)
    {
        return;
    }

    QString data = ui->combo_history->itemText(index);

    ui->edit_address->setText(data.left(data.lastIndexOf(":")));
    ui->edit_port->setValue(data.mid((data.lastIndexOf(":") + 1), (data.length() - data.lastIndexOf(":") - 1)).toUInt());
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
            emit plugin_load_setting("mcumgr_udp_history", &data, &found);

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
}

void udp_setup::add_to_history()
{
    QStringList history;
    int items = ui->combo_history->count();
    QString data = QString("%1:%2").arg(ui->edit_address->text(), QString::number(ui->edit_port->value()));

    while (items > MAX_UDP_HISTORY)
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

    emit plugin_save_setting("mcumgr_udp_history", history);
}
