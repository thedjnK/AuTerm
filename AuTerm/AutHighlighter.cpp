/******************************************************************************
** Copyright (C) 2016-2017 Laird
** Copyright (C) 2024 Jamie M.
**
** Project: AuTerm
**
** Module: AutHighlighter.cpp
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
#include "AutHighlighter.h"

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
AutHighlighter::AutHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    //Setup patterns
    OutPattern.setPattern("^\\<");
    InPattern.setPattern("^\\>");
    WaitPattern.setPattern("^\\~");
    CommentPattern.setPattern("^//[^\n|\r]*");

    //Set pattern options
    OutPattern.setPatternOptions(QRegularExpression::MultilineOption);
    InPattern.setPatternOptions(QRegularExpression::MultilineOption);
    WaitPattern.setPatternOptions(QRegularExpression::MultilineOption);
    CommentPattern.setPatternOptions(QRegularExpression::MultilineOption);

    //Optimise regular expression patterns
    OutPattern.optimize();
    InPattern.optimize();
    WaitPattern.optimize();
    CommentPattern.optimize();

    //Configure formatting for lines
    LineFormat.setForeground(Qt::red);
    LineFormat.setFontWeight(QFont::Bold);

    //Configure formatting for comments
    CommentFormat.setFontWeight(QFont::Normal);
    CommentFormat.setForeground(Qt::darkGreen);
}

void AutHighlighter::highlightBlock(const QString &text)
{
    QRegularExpressionMatchIterator nextmatch;
    QString texta = QString(text).replace(QChar(0x2028), "\n");

    nextmatch = OutPattern.globalMatch(texta);
    while (nextmatch.hasNext())
    {
        QRegularExpressionMatch ThisMatch = nextmatch.next();
        setFormat(ThisMatch.capturedStart(), ThisMatch.capturedLength(), LineFormat);
    }
    nextmatch = InPattern.globalMatch(texta);
    while (nextmatch.hasNext())
    {
        QRegularExpressionMatch ThisMatch = nextmatch.next();
        setFormat(ThisMatch.capturedStart(), ThisMatch.capturedLength(), LineFormat);
    }
    nextmatch = WaitPattern.globalMatch(texta);
    while (nextmatch.hasNext())
    {
        QRegularExpressionMatch ThisMatch = nextmatch.next();
        setFormat(ThisMatch.capturedStart(), ThisMatch.capturedLength(), LineFormat);
    }
    nextmatch = CommentPattern.globalMatch(texta);
    while (nextmatch.hasNext())
    {
        QRegularExpressionMatch ThisMatch = nextmatch.next();
        setFormat(ThisMatch.capturedStart(), ThisMatch.capturedLength(), CommentFormat);
    }
    setCurrentBlockState(0);
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
