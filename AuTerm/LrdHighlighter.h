/******************************************************************************
** Copyright (C) 2016-2017 Laird
**
** Project: AuTerm
**
** Module: LrdHighlighter.h
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
#ifndef LRDHIGHLIGHTER_H
#define LRDHIGHLIGHTER_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

/******************************************************************************/
// Class definitions
/******************************************************************************/
class LrdHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    LrdHighlighter(
        QTextDocument *parent = 0
        );

protected:
    void
    highlightBlock(
        const QString &text
        ) Q_DECL_OVERRIDE;

private:
    QRegularExpression OutPattern; //Matches sending data lines
    QRegularExpression InPattern; //Matches receiving data lines
    QRegularExpression WaitPattern; //Matches time waiting lines
    QRegularExpression CommentPattern; //Matches comment lines
    QTextCharFormat LineFormat; //Format for valid lines
    QTextCharFormat CommentFormat; //Format for comment lines
};

#endif // LRDHIGHLIGHTER_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
