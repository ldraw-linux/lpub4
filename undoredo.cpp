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

#include "lpub.h"
#include "commands.h"

void Gui::insertLine(const Where &here, const QString &line, QUndoCommand *parent)
{
  if (ldrawFile.contains(here.modelName)) {
    undoStack->push(new InsertLineCommand(&ldrawFile,here,line,parent));
  }
}

void Gui::appendLine(const Where &here, const QString &line, QUndoCommand *parent)
{
  if (ldrawFile.contains(here.modelName)) {
    undoStack->push(new AppendLineCommand(&ldrawFile,here,line,parent));
  }
}
      
void Gui::replaceLine(const Where &here, const QString &line, QUndoCommand *parent)
{
  if (ldrawFile.contains(here.modelName) && 
      here.lineNumber < ldrawFile.size(here.modelName)) {

    undoStack->push(new ReplaceLineCommand(&ldrawFile,here,line,parent));
  }
}

void Gui::deleteLine(const Where &here, QUndoCommand *parent)
{
  if (ldrawFile.contains(here.modelName) && 
      here.lineNumber < ldrawFile.size(here.modelName)) {
    undoStack->push(new DeleteLineCommand(&ldrawFile,here,parent));
  }
}
QString Gui::readLine(const Where &here)
{
  return ldrawFile.readLine(here.modelName,here.lineNumber);
}
bool Gui::isSubmodel(const QString &modelName)
{
  return ldrawFile.contains(modelName);
}

void Gui::beginMacro(QString name)
{
  ++macroNesting;
  undoStack->beginMacro(name);
}

void Gui::endMacro()
{
  undoStack->endMacro();
  --macroNesting;
  displayPage();
  displayFile(&ldrawFile,curSubFile,true);
}

void Gui::contentsChange(
  const QString &fileName,
  int      position,
  int      _charsRemoved,
  const QString &charsAdded)
{
  QString  charsRemoved;

  /* Calculate the characters removed from the LDrawFile */

  if (_charsRemoved && ldrawFile.contains(fileName)) {

    QString contents = ldrawFile.contents(fileName).join("\n");

    charsRemoved = contents.mid(position,_charsRemoved);
  }
  
  undoStack->push(new ContentsChangeCommand(&ldrawFile,
                                            fileName,
                                            position,
                                            charsRemoved,
                                            charsAdded));
}

void Gui::undo()
{
  macroNesting++;
  undoStack->undo();
  macroNesting--;
  displayPage();
  displayFile(&ldrawFile,curSubFile,true);
}

void Gui::redo()
{
  macroNesting++;
  undoStack->redo();
  macroNesting--;
  displayPage();
  displayFile(&ldrawFile,curSubFile,true);
}

void Gui::canRedoChanged(bool enabled)
{
  redoAct->setEnabled(enabled);
}

void Gui::canUndoChanged(bool enabled)
{
  undoAct->setEnabled(enabled);
}
void Gui::cleanChanged(bool cleanState)
{
  saveAct->setDisabled( cleanState);
}
