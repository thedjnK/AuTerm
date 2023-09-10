/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  udp_setup.h
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
#ifndef UDP_SETUP_H
#define UDP_SETUP_H

#include <QDialog>

namespace Ui
{
    class udp_setup;
}

class udp_setup : public QDialog
{
    Q_OBJECT

public:
    explicit udp_setup(QWidget *parent = nullptr);
    ~udp_setup();

private slots:
    void on_btn_connect_clicked();
    void on_btn_close_clicked();
    void on_check_save_history_stateChanged(int arg1);
    void on_btn_clear_history_clicked();

private:
    Ui::udp_setup *ui;
};

#endif // UDP_SETUP_H
