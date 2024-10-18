/******************************************************************************
** Copyright (C) 2016-2018 The Qt Company with modifications by Laird
**
** Project: AuTerm
**
** Module: AutCodeEditor.h
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
#ifndef AUTCODEEDITOR_H
#define AUTCODEEDITOR_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QPlainTextEdit>
#include <QObject>
#include <QtWidgets>

/******************************************************************************/
// Constants
/******************************************************************************/
const qint8 IndicationAreaWidth = 11;

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
QT_END_NAMESPACE
class LineNumberArea;

/******************************************************************************/
// Class definitions
/******************************************************************************/
class AutCodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    AutCodeEditor(QWidget *parent = 0);
    ~AutCodeEditor();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void ClearBadLines();
    void AddBadLine(unsigned int LineNumber);
    void SetExecutionLine(int LineNumber);
    void SetExecutionLineStatus(bool bStatus);

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;

private slots:
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    QWidget *mwidIndicationArea;
    QList<unsigned int> mlistInvLines; //Contains a list of invalid lines
    int mintCLine; //Current execution line (if there is one)
    bool mbLineFail; //True if the current line is where the script failed or false if it is currently executing
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(AutCodeEditor *ceNewEditor) : QWidget(ceNewEditor)
    {
        mceEditor = ceNewEditor;
    }

    QSize sizeHint() const Q_DECL_OVERRIDE
    {
        return QSize(IndicationAreaWidth, 0);
    }

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE
    {
        mceEditor->lineNumberAreaPaintEvent(event);
    }

private:
    AutCodeEditor *mceEditor; //Handle for code editor
};

#endif // AUTCODEEDITOR_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
