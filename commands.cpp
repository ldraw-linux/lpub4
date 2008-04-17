/****************************************************************************
**
** Copyright (C) 2007-2008 Kevin Clague. All rights reserved.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "commands.h"
#include "ldrawfiles.h"
#include "lpub.h"

InsertLineCommand::InsertLineCommand(
  LDrawFile     *_ldrawFile,
  const Where   &_here,
  const QString &_line,
  QUndoCommand *parent)
  : QUndoCommand(parent)
{
  setText("insertLine");
  ldrawFile  = _ldrawFile;
  here       = _here;
  line       = _line;
}

void InsertLineCommand::undo()
{
  ldrawFile->deleteLine(here.modelName,here.lineNumber);
  gui->displayFile(ldrawFile,here.modelName,true);
  gui->displayPage();
}

void InsertLineCommand::redo()
{
  ldrawFile->insertLine(here.modelName,here.lineNumber,line);
  gui->displayFile(ldrawFile,here.modelName,true);
  gui->displayPage();
}

AppendLineCommand::AppendLineCommand(
  LDrawFile    *_ldrawFile,
  const Where  &_here,
  const QString &_line,
  QUndoCommand *parent)
  : QUndoCommand(parent)
{
  setText("appendLine");
  ldrawFile = _ldrawFile;
  line      = _line;
  here      = _here;
}

void AppendLineCommand::undo()
{
  ldrawFile->deleteLine(here.modelName,here.lineNumber+1);
  gui->displayFile(ldrawFile,here.modelName,true);
  gui->displayPage();
}

void AppendLineCommand::redo()
{
  ldrawFile->insertLine(here.modelName,here.lineNumber+1,line);
  gui->displayFile(ldrawFile,here.modelName,true);
  gui->displayPage();
}

DeleteLineCommand::DeleteLineCommand(
  LDrawFile    *_ldrawFile,
  const Where  &_here,
  QUndoCommand *parent)
  : QUndoCommand(parent)
{
  setText("deleteLine");
  ldrawFile = _ldrawFile;
  here      = _here;
}

void DeleteLineCommand::undo()
{
  ldrawFile->insertLine(here.modelName,here.lineNumber,deletedLine);
  gui->displayFile(ldrawFile,here.modelName,true);
  gui->displayPage();
}

void DeleteLineCommand::redo()
{
  deletedLine = ldrawFile->readLine(here.modelName, here.lineNumber);
  ldrawFile->deleteLine(here.modelName,here.lineNumber);
  gui->displayFile(ldrawFile,here.modelName,true);
  gui->displayPage();
}

ReplaceLineCommand::ReplaceLineCommand(
  LDrawFile     *_ldrawFile,
  const Where   &_here,
  const QString &_newLine,
  QUndoCommand  *parent)
  : QUndoCommand(parent)
{
  setText("replaceLine");
  ldrawFile  = _ldrawFile;
  here       = _here;
  newLine    = _newLine;
  oldLine    = ldrawFile->readLine(here.modelName,here.lineNumber);
}

void ReplaceLineCommand::undo()
{
  ldrawFile->replaceLine(here.modelName,here.lineNumber,oldLine);
  gui->displayFile(ldrawFile,here.modelName,true);
  gui->displayPage();
}

void ReplaceLineCommand::redo()
{
  ldrawFile->replaceLine(here.modelName,
                         here.lineNumber,
                         newLine);
  gui->displayFile(ldrawFile,here.modelName,true);
  gui->displayPage();
}

ContentsChangeCommand::ContentsChangeCommand(
  LDrawFile     *_ldrawFile,
  const QString &_modelName,
  int            _position,
  const QString &_removedChars,
  const QString &_addedChars,
  QUndoCommand  *parent)
  : QUndoCommand(parent)
{
  setText("userTyped");
  ldrawFile    = _ldrawFile;
  modelName    = _modelName;
  position     = _position;
  removedChars = _removedChars;
  addedChars   = _addedChars;
  isRedo       = false;
}

void ContentsChangeCommand::redo()
{
  ldrawFile->changeContents(
    modelName,
    position,
    removedChars.size(),
    addedChars);

  if ( !isRedo) {
    isRedo = true;
  } else {
    gui->maxPages = -1;
    gui->displayFile(ldrawFile,modelName,true);
    gui->displayPage();
  }
}

void ContentsChangeCommand::undo()
{
  
  gui->maxPages = -1;
  
  ldrawFile->changeContents(
    modelName,
    position,
    addedChars.size(),
    removedChars);
  gui->displayFile(ldrawFile,modelName,true);
  gui->displayPage();
}
