/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  bluetooth_setup.h
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
#ifndef BLUETOOTH_SETUP_H
#define BLUETOOTH_SETUP_H

#include <QDialog>
#include <QListWidgetItem>

namespace Ui {
    class bluetooth_setup;
}

class bluetooth_setup : public QDialog
{
    Q_OBJECT

public:
    explicit bluetooth_setup(QWidget *parent = nullptr);
    ~bluetooth_setup();
    void clear_devices();
    void add_device(QString *data);
    void add_debug(QString data);

private slots:
    void on_btn_refresh_clicked();
    void on_btn_connect_clicked();
    void on_btn_disconnect_clicked();
    void on_btn_close_clicked();
    void on_list_devices_itemDoubleClicked(QListWidgetItem *item);

    void on_btn_min_params_clicked();

signals:
    void refresh_devices();
    void connect_to_device(uint16_t index);
    void disconnect_from_device();
    void min_params();

private:
    Ui::bluetooth_setup *ui;
};

#endif // BLUETOOTH_SETUP_H
