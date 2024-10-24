/******************************************************************************
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module:  lorawan_setup.h
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
#ifndef LORAWAN_SETUP_H
#define LORAWAN_SETUP_H

#include <QDialog>
#include <QVariant>

#define MAX_LORAWAN_HISTORY 10

namespace Ui
{
    class lorawan_setup;
}

class lorawan_setup : public QDialog
{
    Q_OBJECT

public:
    explicit lorawan_setup(QWidget *parent = nullptr);
    ~lorawan_setup();
    void load_settings();
    bool get_confirmed_downlinks();
    uint8_t get_frame_port();
    uint16_t get_fragment_size();
    bool get_auto_fragment_size();
    uint8_t get_resends();
    uint32_t get_timeout();
    void set_connection_options_enabled(bool enabled);
    void load_pixmaps();
    void set_connection_state(bool connected);

private slots:
    void on_btn_connect_clicked();
    void on_btn_close_clicked();
    void on_btn_clear_history_clicked();
    void on_check_save_history_toggled(bool checked);
    void on_combo_history_currentIndexChanged(int index);

signals:
    void connect_to_service(QString host, uint16_t port, bool tls, QString username, QString password, QString topic);
    void disconnect_from_service();
    void plugin_save_setting(QString name, QVariant data);
    void plugin_load_setting(QString name, QVariant *data, bool *found);
    void plugin_get_image_pixmap(QString name, QPixmap **pixmap);

private:
    void add_to_history();

    Ui::lorawan_setup *ui;
    QPixmap *red_circle;
    QPixmap *green_circle;
    bool is_connected;
};

#endif // LORAWAN_SETUP_H
