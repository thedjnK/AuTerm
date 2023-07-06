/******************************************************************************
** Copyright (C) 2015-2017 Laird
**
** Project: AuTerm
**
** Module: LrdLogger.cpp
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

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "LrdLogger.h"

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
LrdLogger::LrdLogger(QWidget *parent) : QWidget(parent)
{
    //Initial values
    mbLogOpen = false;
}

//=============================================================================
//=============================================================================
LrdLogger::~LrdLogger(
    )
{
    if (mbLogOpen == true)
    {
        //Log is open, flush the buffer, close it and delete it's handle
        mpLogFile->flush();
        mbLogOpen = false;
        delete mpStreamOut;
        delete mpLogFile;
    }
}

//=============================================================================
//=============================================================================
unsigned char
LrdLogger::OpenLogFile(
    QString strFilename
    )
{
    //Opens the log file specified
    bool bNewFile = QFile::exists(strFilename);
    if (mbLogOpen == false)
    {
        //Open log file
        mpLogFile = new QFile(strFilename);
        if (!mpLogFile->open(QIODevice::Append | QIODevice::Text))
        {
            //Unable to open file
            return LOG_ERR_ACCESS;
        }
        mpStreamOut = new QDataStream(mpLogFile);
        if (bNewFile == false)
        {
            //Create UTF-8 header
            mpStreamOut->writeRawData("\xEF\xBB\xBF", 3);
        }
        else
        {
            //Add a newline
            mpStreamOut->writeRawData("\r\n", 2);
        }
        mbLogOpen = true;
        return LOG_OK;
    }
    else
    {
        //Log already open
        return LOG_ERR_OPEN_ALREADY;
    }
}

//=============================================================================
//=============================================================================
void
LrdLogger::CloseLogFile(
    )
{
    //Closes the log file
    if (mbLogOpen == true)
    {
        mpLogFile->flush();
        mbLogOpen = false;
        delete mpStreamOut;
        delete mpLogFile;
    }
}

//=============================================================================
//=============================================================================
unsigned char
LrdLogger::WriteLogData(
    QString strData
    )
{
    //Writes a line to the log file
    if (mbLogOpen == true)
    {
        //Log opened
        mpStreamOut->writeRawData(strData.toUtf8(), strData.toUtf8().length());
        return LOG_OK;
    }
    else
    {
        //Log not open
        return LOG_NOT_OPEN;
    }
}

//=============================================================================
//=============================================================================
unsigned char
LrdLogger::WriteRawLogData(
    QByteArray baData
    )
{
    //Writes raw data to the log file
    if (mbLogOpen == true)
    {
        //Log opened
        mpStreamOut->writeRawData(baData, baData.length());
        return LOG_OK;
    }
    else
    {
        //Log not opened
        return LOG_NOT_OPEN;
    }
}

//=============================================================================
//=============================================================================
unsigned short
LrdLogger::GetLogSize(
    )
{
    //Returns the size of the log
    if (mbLogOpen == true)
    {
        //Log open
        return mpLogFile->size();
    }
    else
    {
        //Log not open
        return 0;
    }
}

//=============================================================================
//=============================================================================
void
LrdLogger::ClearLog(
    )
{
    //Clears out the log
    if (mbLogOpen == true)
    {
        //Resize file to be empty
        mpLogFile->flush();
        mpLogFile->resize(0);

        //Write the UTF-8 BOM
        mpStreamOut->writeRawData("\xEF\xBB\xBF", 3);
    }
}

//=============================================================================
//=============================================================================
QString
LrdLogger::GetLogName(
    )
{
    if (mpLogFile->isOpen() == true)
    {
        //Log open, return log file name
        return mpLogFile->fileName();
    }
    else
    {
        //Log not open, return empty string
        return "";
    }
}

//=============================================================================
//=============================================================================
bool
LrdLogger::IsLogOpen(
    )
{
    //Returns true if log is open
    return mbLogOpen;
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
