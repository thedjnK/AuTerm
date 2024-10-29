/******************************************************************************
** Copyright (C) 2023-2024 Jamie M.
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

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QDialog>
#include <QVariant>
#include "debug_logger.h"

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class udp_setup;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class udp_setup : public QDialog
{
    Q_OBJECT

public:
    explicit udp_setup(QWidget *parent = nullptr);
    ~udp_setup();
    void load_settings();
    void load_pixmaps();
#ifndef SKIPPLUGIN_LOGGER
    void set_logger(debug_logger *object);
#endif

private slots:
    void on_btn_connect_clicked();
    void on_btn_close_clicked();
    void on_btn_clear_history_clicked();
    void on_check_save_history_toggled(bool checked);
    void on_combo_history_currentIndexChanged(int index);

signals:
    void connect_to_device(QString host, uint16_t port);
    void disconnect_from_device();
    void is_connected(bool *connected);
    void plugin_save_setting(QString name, QVariant data);
    void plugin_load_setting(QString name, QVariant *data, bool *found);
    void plugin_get_image_pixmap(QString name, QPixmap **pixmap);

private:
    void add_to_history();

    Ui::udp_setup *ui;
    QPixmap *red_circle;
    QPixmap *green_circle;
    QStringList saved_history;
#ifndef SKIPPLUGIN_LOGGER
    debug_logger *logger;
#endif
};

#endif // UDP_SETUP_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
