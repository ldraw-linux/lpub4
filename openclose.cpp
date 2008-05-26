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

#include <QFileDialog>
#include <QDir>
#include <QTextEdit>
#include <QUndoStack>

#include "lpub.h"
#include "lpub_preferences.h"
#include "editwindow.h"
#include "paths.h"

void Gui::open()
{
  if (maybeSave()) {
    QString fileName = QFileDialog::getOpenFileName(
      this,
      tr("Open LDraw File"),
      Preferences::ldrawPath + "\\MODELS",
      tr("LDraw Files (*.DAT;*.LDR;*.MPD;*.dat;*.ldr;*.mpd)"));

    if (!fileName.isEmpty()) {
        openFile(fileName);
        displayPage();
        return;
      }
    }
  return;
}

void Gui::save()
{
  if (isMpd()) {
    watcher.removePath(curFile);
  } else {
    QStringList list = ldrawFile.subFileOrder();
    QString foo;
    foreach (foo,list) {
      QString bar = QDir::currentPath() + "/" + foo;
      watcher.removePath(bar);
    }
  }
  if (curFile.isEmpty()) {
    saveAs();
  } else {
    saveFile(curFile);
  }

  if (isMpd()) {
    watcher.addPath(curFile);
  } else {
    QStringList list = ldrawFile.subFileOrder();
    QString foo;
    foreach (foo,list) {
      QString bar = QDir::currentPath() + "/" + foo;
      watcher.addPath(bar);
    }
  }
}

void Gui::saveAs()
{
  if (curFile != "") {
    if (isMpd()) {
      watcher.removePath(curFile);
    } else {
      QStringList list = ldrawFile.subFileOrder();
      QString foo; 
      foreach (foo,list) {
        QString bar = QDir::currentPath() + "/" + foo;
        watcher.removePath(bar);
      }
    }
  }
  QString fileName = QFileDialog::getSaveFileName(this);
  if (fileName.isEmpty()) {
    return;
  }
  saveFile(fileName); 
  if (curFile != "") {
    if (isMpd()) {
      watcher.addPath(curFile);
    } else {
      QStringList list = ldrawFile.subFileOrder();
      QString foo;
      foreach (foo,list) {
        QString bar = QDir::currentPath() + "/" + foo;
        watcher.addPath(bar);
      }
    }
  }
} 

bool Gui::maybeSave()
{
  if ( ! undoStack->isClean() ) {
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr(LPUB),
            tr("The document has been modified.\n"
                "Do you want to save your changes?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save) {
      save();
      return true;
    } else if (ret == QMessageBox::Cancel) {
      return false;
    }
  }
  return true;
}

bool Gui::saveFile(const QString &fileName)
{
  bool rc;
  rc = ldrawFile.saveFile(fileName);
  setCurrentFile(fileName);
  undoStack->setClean();
  if (rc) {
    statusBar()->showMessage(tr("File saved"), 2000);
  }
  return rc;
}

void Gui::closeFile()
{
  ldrawFile.empty();
  undoStack->clear();
  editWindow->textEdit()->document()->clear();
  editWindow->textEdit()->document()->setModified(false);
  mpdCombo->setMaxCount(0);
  mpdCombo->setMaxCount(1000);
}

void Gui::openRecentFile()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    QString fileName = action->data().toString();
    clearPage(KpageView,KpageScene);
    QFileInfo info(fileName);
    QDir::setCurrent(info.absolutePath());
    openFile(fileName);
    setCurrentFile(fileName);
    Paths::mkdirs();
    displayPage();
  }
}

/***************************************************************************
 * File opening closing stuff
 **************************************************************************/

void Gui::openFile(QString &fileName)
{
  if (curFile != "") {
    if (isMpd()) {
      watcher.removePath(curFile);
    } else { 
      QStringList list = ldrawFile.subFileOrder();
      QString foo;
      foreach (foo,list) {
        QString bar = QDir::currentPath() + "/" + foo;
        watcher.removePath(bar);
      }
    }
  }

  clearPage(KpageView,KpageScene);
  closeFile();
  displayPageNum = 1;
  QFileInfo info(fileName);
  QDir::setCurrent(info.absolutePath());
  Paths::mkdirs();
  ldrawFile.loadFile(fileName);
  mpdCombo->setMaxCount(0);
  mpdCombo->setMaxCount(1000);
  mpdCombo->addItems(ldrawFile.subFileOrder());
  setCurrentFile(fileName);
  displayFile(&ldrawFile,ldrawFile.topLevelFile());
  undoStack->setClean();
  curFile = fileName;
  QStringList list = ldrawFile.subFileOrder();
  QString foo;

  if (isMpd()) {
    watcher.addPath(curFile);
  } else {
    QStringList list = ldrawFile.subFileOrder();
    QString foo;
    foreach (foo,list) {
      QString bar = QDir::currentPath() + "/" + foo;
      watcher.addPath(bar);
    }
  }
}

void Gui::updateRecentFileActions()
{
  QSettings settings(LPUB,SETTINGS);
  QStringList files = settings.value("recentFileList").toStringList();

  int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

  for (int i = 0; i < numRecentFiles; i++) {
    QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
    recentFilesActs[i]->setText(text);
    recentFilesActs[i]->setData(files[i]);
    recentFilesActs[i]->setVisible(true);
  }
  for (int j = numRecentFiles; j < MaxRecentFiles; j++) {
    recentFilesActs[j]->setVisible(false);
  }
  separatorAct->setVisible(numRecentFiles > 0);
}

QString Gui::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

void Gui::setCurrentFile(const QString &fileName)
{
  QString shownName;
  if (fileName.size() == 0)
      shownName = "An LDraw Building Instruction Editor";
  else
      shownName = strippedName(fileName);

  setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr(LPUB)));

  if (fileName.size() > 0) {
    QSettings settings(LPUB, SETTINGS);
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll("");
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles) {
      files.removeLast();
    }
    settings.setValue("recentFileList", files);
  }
  updateRecentFileActions();
}

void Gui::fileChanged(const QString &path)
{
  QString msg = QString(tr("The file \"%1\" contents have changed.  Reload?"))
                        .arg(path);
  int ret = QMessageBox::warning(this,tr(LPUB),msg,
              QMessageBox::Apply | QMessageBox::No,
              QMessageBox::Apply);
  if (ret == QMessageBox::Apply) {
    QString fileName = path;
    openFile(fileName);
    drawPage(KpageView,KpageScene);
  }
}
