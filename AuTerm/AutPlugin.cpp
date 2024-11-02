/******************************************************************************
** Copyright (C) 2023-2024 Jamie M.
**
** Project: AuTerm
**
** Module:  AutPlugin.cpp
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
#include "AutPlugin.h"
#include <QPluginLoader>

//For static builds, plugins need to be imported
#ifdef QT_STATIC
#ifdef STATICPLUGIN_MCUMGR
//MCUmgr plugin
Q_IMPORT_PLUGIN(plugin_mcumgr)
#endif

#ifdef STATICPLUGIN_LOGGER
//logger plugin
Q_IMPORT_PLUGIN(plugin_logger)
#endif

#ifdef STATICPLUGIN_TRANSPORT_ECHO
//Dummy echo transport plugin
Q_IMPORT_PLUGIN(plugin_echo_transport)
#endif
#endif
