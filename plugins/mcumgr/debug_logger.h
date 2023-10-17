/******************************************************************************
** Copyright (C) 2023 Jamie M.
**
** Project: AuTerm
**
** Module:  debug_logger.h
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
#ifndef DEBUG_LOGGER_H
#define DEBUG_LOGGER_H

#include <QDebug>

#ifndef SKIPPLUGIN_LOGGER
#include <QIODevice>
#include "../plugins/logger/plugin_logger.h"

#define PLUGIN_NAME "mcumgr"
#define LOG_OBJECT logger

#define log_error() LOG_OBJECT->set_options(PLUGIN_NAME, log_level_error); QDebug(LOG_OBJECT)
#define log_warning() LOG_OBJECT->set_options(PLUGIN_NAME, log_level_warning); QDebug(LOG_OBJECT)
#define log_information() LOG_OBJECT->set_options(PLUGIN_NAME, log_level_information); QDebug(LOG_OBJECT)
#define log_debug() LOG_OBJECT->set_options(PLUGIN_NAME, log_level_debug); QDebug(LOG_OBJECT)
#else
#define log_error() qDebug()
#define log_warning() qDebug()
#define log_information() qDebug()
#define log_debug() qDebug()
#endif

#ifndef SKIPPLUGIN_LOGGER
class debug_logger : public QIODevice
{
    Q_OBJECT
public:
    explicit debug_logger(QObject *parent = nullptr);
    void find_logger_plugin(const QObject *main_window);
    void set_options(QString title, log_level_types type);

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

signals:
    void find_plugin(QString name, plugin_data *plugin);
    void logger_log(enum log_level_types type, QString sender, QString message);
    void logger_set_visible(bool enabled);

private:
    bool plugin_active;
    const QObject *logger_pointer;
    QString logger_title;
    log_level_types logger_type;
};
#endif

#endif // DEBUG_LOGGER_H
