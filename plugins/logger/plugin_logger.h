/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  plugin_logger.h
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
#ifndef PLUGIN_LOGGER_H
#define PLUGIN_LOGGER_H

#include <QWidget>
#include <QColor>
#include "AutPlugin.h"

namespace Ui {
    class plugin_logger;
}

enum log_level_types {
    log_level_error = 0,
    log_level_warning,
    log_level_information,
    log_level_debug,
};

class plugin_logger : public QWidget, AutPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AuTermPluginInterface_iid FILE "plugin_logger.json")
    Q_INTERFACES(AutPlugin)

public:
    explicit plugin_logger(QWidget *parent = nullptr);
    ~plugin_logger();
    QWidget *GetWidget();
    void setup(QMainWindow *main_window);
    const QString plugin_about();
    bool plugin_configuration();

public slots:
    void log_level_enabled(enum log_level_types type, bool *enabled);
    void log_message(enum log_level_types type, QString sender, QString message);
    void set_enabled(bool enabled);

private slots:
    void on_btn_clear_clicked();
    void on_btn_copy_clicked();

private:
    Ui::plugin_logger *ui;
    const QColor error_text_colour = QColor::fromRgb(255, 10, 10);
    const QColor warning_text_colour = QColor::fromRgb(225, 190, 0);
    const QColor information_text_colour = QColor::fromRgb(10, 200, 10);
    const QColor debug_text_colour = QColor::fromRgb(120, 120, 120);
};

#endif // PLUGIN_LOGGER_H
