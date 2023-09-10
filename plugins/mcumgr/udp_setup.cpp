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
}

udp_setup::~udp_setup()
{
    delete ui;
}

void udp_setup::on_btn_connect_clicked()
{

}

void udp_setup::on_btn_close_clicked()
{

}

void udp_setup::on_check_save_history_stateChanged(int arg1)
{

}

void udp_setup::on_btn_clear_history_clicked()
{

}

