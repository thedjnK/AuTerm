/******************************************************************************
** Copyright (C) 2016 The Qt Company with modifications by Laird
**
** Project: AuTerm
**
** Module: AutCodeEditor.cpp
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
#include "AutCodeEditor.h"

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
AutCodeEditor::AutCodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    //Create indication area widget
    mwidIndicationArea = new LineNumberArea(this);

    //Connect signals to slots
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    //Set the margins for the viewport
    setViewportMargins(IndicationAreaWidth, 0, 0, 0);

    //Run line highlighter at startup
    highlightCurrentLine();

    //No current execution line
    mintCLine = -1;

    //Line isn't a marking a failure
    mbLineFail = false;
}

AutCodeEditor::~AutCodeEditor()
{
    //Disconnect all signals
    disconnect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    disconnect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    //Delete indication area widget
    delete mwidIndicationArea;
}

void AutCodeEditor::updateLineNumberArea(const QRect &rectArea, int intdy)
{
    if (intdy)
    {
        mwidIndicationArea->scroll(0, intdy);
    }
    else
    {
        mwidIndicationArea->update(0, rectArea.y(), mwidIndicationArea->width(), rectArea.height());
    }
}

void AutCodeEditor::resizeEvent(QResizeEvent *e)
{
    //Runs when code editor has been resized
    QPlainTextEdit::resizeEvent(e);
    QRect rectContents = contentsRect();
    mwidIndicationArea->setGeometry(QRect(rectContents.left(), rectContents.top(), IndicationAreaWidth, rectContents.height()));
}

void AutCodeEditor::highlightCurrentLine()
{
    //Highlights current line
    QList<QTextEdit::ExtraSelection> extraSelections;

    //Only highlight if not read only
    if (!isReadOnly())
    {
        //Set the highlight colour to yellow
        QTextEdit::ExtraSelection selTextLine;
        selTextLine.format.setBackground(QColor(Qt::yellow).lighter(160));
        selTextLine.format.setProperty(QTextFormat::FullWidthSelection, true);
        selTextLine.cursor = textCursor();
        selTextLine.cursor.clearSelection();
        extraSelections.append(selTextLine);
    }

    //Pass the selection list back
    setExtraSelections(extraSelections);
}

void AutCodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    //Paint the invalid/execution lines in
    QPainter pntPainter(mwidIndicationArea);
    pntPainter.fillRect(event->rect(), Qt::lightGray);

    //Get first block (line) and calculate the top and bottom positions
    QTextBlock tbTextBlock = firstVisibleBlock();
    int intBlockNumber = tbTextBlock.blockNumber();
    int intTop = (int)blockBoundingGeometry(tbTextBlock).translated(contentOffset()).top();
    int intBottom = intTop + (int)blockBoundingRect(tbTextBlock).height();

    //Set the painter for a bad line
    pntPainter.setPen(Qt::red);
    pntPainter.setBrush(Qt::red);

    while (tbTextBlock.isValid() && intTop <= event->rect().bottom())
    {
        if (tbTextBlock.isVisible() && intBottom >= event->rect().top())
        {
            if (mlistInvLines.contains(intBlockNumber))
            {
                //Paint line as bad
                pntPainter.drawEllipse(1, intTop+4, mwidIndicationArea->width()-3, fontMetrics().height()-7);
            }
            else if (intBlockNumber == mintCLine)
            {
                //Paint line as currently being executed or failed
                if (mbLineFail == true)
                {
                    //Failed
                    pntPainter.setPen(Qt::black);
                    pntPainter.setBrush(Qt::black);
                }
                else
                {
                    //Executing
                    pntPainter.setPen(Qt::green);
                    pntPainter.setBrush(Qt::green);
                }
                pntPainter.drawEllipse(1, intTop+4, mwidIndicationArea->width()-3, fontMetrics().height()-7);
                pntPainter.setPen(Qt::red);
                pntPainter.setBrush(Qt::red);
            }
        }

        //Move to next text block (line)
        tbTextBlock = tbTextBlock.next();
        intTop = intBottom;
        intBottom = intTop + (int)blockBoundingRect(tbTextBlock).height();
        ++intBlockNumber;
    }
}

void AutCodeEditor::keyPressEvent(QKeyEvent *e)
{
    //Key has been presses
    if (e->key() == Qt::Key_Return && e->modifiers() == Qt::ShiftModifier)
    {
        //Removes the daft shift-enter newline that is actually a line separator.
        e->setModifiers(Qt::NoModifier);
    }

    //Pass back to object
    QPlainTextEdit::keyPressEvent(e);
}

void AutCodeEditor::ClearBadLines()
{
    //Clears list of invalid lines
    mlistInvLines.clear();
    mbLineFail = false;
}

void AutCodeEditor::AddBadLine(unsigned int uintLineNumber)
{
    //Marks a line as being invalid
    mlistInvLines.append(uintLineNumber);
}

void AutCodeEditor::SetExecutionLine(int intLineNumber)
{
    //Sets the current line that is being executed and repaints the object
    mintCLine = intLineNumber;
    this->repaint();
}

void AutCodeEditor::SetExecutionLineStatus(bool bStatus)
{
    //Sets the status of the execution line colouring
    mbLineFail = bStatus;
    this->repaint();
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
