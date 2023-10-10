/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  debug_logger.cpp
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
#ifndef SKIPPLUGIN_LOGGER

#include "debug_logger.h"
#include <QDebug>

debug_logger::debug_logger(QObject *parent)
    : QIODevice{parent}
{
    QIODevice::open(QIODevice::WriteOnly);
    plugin_active = false;
}

qint64 debug_logger::readData(char *data, qint64 maxlen)
{
    return 0;
}

qint64 debug_logger::writeData(const char *data, qint64 len)
{
    if (plugin_active == true)
    {
        emit logger_log(logger_type, logger_title, data);
        return len;
    }

    qDebug() << data;
    return len;
}

void debug_logger::find_logger_plugin(const QObject *main_window)
{
    plugin_data logger;

    connect(this, SIGNAL(find_plugin(QString,plugin_data*)), main_window, SLOT(find_plugin(QString,plugin_data*)));
    emit find_plugin("logger", &logger);
    disconnect(this, SIGNAL(find_plugin(QString,plugin_data*)), main_window, SLOT(find_plugin(QString,plugin_data*)));

    if (logger.found == false)
    {
        return;
    }

    logger_pointer = logger.object;
    plugin_active = true;

    connect(this, SIGNAL(logger_log(log_level_types,QString,QString)), logger_pointer, SLOT(log_message(log_level_types,QString,QString)));
    connect(this, SIGNAL(logger_set_visible(bool)), logger_pointer, SLOT(set_enabled(bool)));
}

void debug_logger::set_options(QString title, log_level_types type)
{
    logger_title = title;
    logger_type = type;
}

#endif
