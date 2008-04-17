
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

#include <QtGui>
#include <QSettings>
#include "paths.h"
#include "name.h"

Paths paths;

QString Paths::ldrawPath   = "c:/ldraw";
QString Paths::lpubPath    = ".";
QString Paths::ldglitePath = "c:/ldraw/apps/ldglite";
QString Paths::ldgliteExe  = "c:/ldraw/apps/ldglite/ldglite.exe";
QString Paths::ldviewPath  = "c:/ldraw/apps/ldview";
QString Paths::ldviewExe   = "c:/ldraw/apps/ldview/ldview.exe";
QString Paths::pliFile;

QString Paths::lpubDir   = "LPub";
QString Paths::tmpDir    = "LPub/tmp";
QString Paths::outputDir = "LPub/output";
QString Paths::assemDir  = "LPub/assem";
QString Paths::partsDir  = "LPub/parts";

void Paths::getLDrawPath(
  bool     refind,
  QWidget *parent)
{
  QFileInfo fileInfo;
  QSettings settings(LPUB,SETTINGS);

  if ( ! refind) {

    QDir cwd(ldrawPath);

    if (cwd.exists()) {
      return;
    }

    ldrawPath = settings.value("LDRAWDIR").toString();

    fileInfo.setFile(ldrawPath);

    if (fileInfo.exists()) {
      return;
    }

    QStringList environment = QProcess::systemEnvironment();
    for (int i = 0; i < environment.size(); i++) {
      QRegExp rx("^LDRAWDIR=(.*)$");
      if (environment[i].contains(rx)) {
        ldrawPath = rx.cap(1);
        fileInfo.setFile(ldrawPath);
        if (fileInfo.exists()) {
          settings.setValue("LDRAWDIR",ldrawPath);
          return;
        }
      }
    }
  }

  do {
    ldrawPath = QFileDialog::getExistingDirectory(parent,
                  QFileDialog::tr("Locate LDraw Directory"),
                  "/",
                  QFileDialog::ShowDirsOnly | 
                  QFileDialog::DontResolveSymlinks);
    fileInfo.setFile(ldrawPath);
  } while ( ! fileInfo.exists());

  settings.setValue("LDRAWDIR",ldrawPath);
}

QString Paths::getLdglitePath(bool *ok, bool refind, QWidget *parent)
{
  QString exe = getPath(refind,
                        ldgliteExe, 
                        LDGLITEEXE, 
                        ".exe",
                        "Please locate program ldglite.exe", 
                        ok, 
                        parent);
  if (*ok) {
    ldgliteExe = exe;
  }
  return exe;
}

QString Paths::getLdviewPath(bool *ok, bool refind, QWidget *parent)
{
  QString exe = getPath(refind,
                        ldviewExe, 
                        LDVIEWEXE, 
#ifdef _APPLE_
                        ".app",
#else
                        ".exe",
#endif
                        "Please locate program ldview.exe", 
                        ok, 
                        parent);
  if (*ok) {
    ldviewExe = exe;
  }
  return exe;
}

QString Paths::getPliPath(bool *ok, bool refind, QWidget *parent)
{
 
  /* Try existing */

  if ( ! refind) {
    QFileInfo fileInfo;

    fileInfo.setFile(pliFile);

    if (fileInfo.exists()) {
      *ok = true;
      return pliFile;
    }

    /* Try settings */

    QSettings settings(LPUB,SETTINGS);
    pliFile = settings.value(PLIFILE).toString();
    fileInfo.setFile(pliFile);

    if (fileInfo.exists()) {
      *ok = true;
      return pliFile;
    }
    
    pliFile = lpubPath + "/extras/pli.mpd";

    fileInfo.setFile(pliFile);
    if (fileInfo.exists()) {
      settings.setValue(PLIFILE,pliFile);
      *ok = true;
      return pliFile;
    }
  }

  QString exe = getPath(refind,
                        pliFile, 
                        PLIFILE, 
                        "",
                        "Please locate the Parts List MPD file", 
                        ok, 
                        parent);
  if (*ok) {
    pliFile = exe;
  }
  return exe;
}

void Paths::getLPubPath()
{
  QDir cwd(QDir::currentPath());

  if (cwd.dirName() == "release" || cwd.dirName() == "debug") {
    cwd.cdUp();
  }
  lpubPath = cwd.absolutePath();
}

/*
 * 1.  try existing path
 * 2.  try settings
 * 3.  try path
 * 4.  resort to file dialog
 */

QString Paths::getPath(
  bool    refind,
  QString exe, 
  QString name, 
  QString suffix,
  QString title,
  bool *ok, 
  QWidget *parent)
{
  QFileInfo fileInfo;
  QString app;
  QSettings settings(LPUB,SETTINGS);
 
  /* Try existing */

  if ( ! refind) {

    fileInfo.setFile(exe);

    if (fileInfo.exists()) {
      *ok = true;
      return exe;
    }

    /* Try settings */

    app = settings.value(name).toString();
    fileInfo.setFile(app);

    if (fileInfo.exists()) {
      *ok = true;
      return app;
    }

    /* Try path */

    fileInfo.setFile(exe);
  
    app = fileInfo.baseName();

    // see if exe is already in our path

    QStringList environment = QProcess::systemEnvironment();
    for (int i = 0; i < environment.size(); i++) {
      QRegExp rx("PATH^=(.*)$");
      if (environment[i].contains(rx)) {
        QStringList paths = rx.cap(1).split(";");
        for (int j = 0; j < paths.size(); j++) {
          QString possible = paths[j] + "/" + app;
          fileInfo.setFile(possible);
          if (fileInfo.exists()) {
            settings.setValue(name,possible);
            *ok = true;
            return possible;
          }
        }
      }
    }
  }

  // ask the user to find it.

  QFileDialog dialog(parent);

  fileInfo.setFile(exe);
  app = fileInfo.baseName();

  dialog.setWindowTitle(title);
  dialog.setFileMode(QFileDialog::ExistingFile);
  if (suffix == ".exe" || suffix == ".app") {
    dialog.setFilter("Program (" + app + suffix + ")");
  } else {
    dialog.setFilter("LDraw File (*.ldr;*.mpd;*.dat;*.LDR;*.MPD;*.DAT)");
  }

  if (dialog.exec()) {
    QStringList selectedFiles = dialog.selectedFiles();

    if (selectedFiles.size() == 1) {
      QString selectedFile = selectedFiles[0];
    
      fileInfo.setFile(selectedFile);

      *ok = true;
      settings.setValue(name,selectedFile);
      return fileInfo.absoluteFilePath();
    }
  }
  *ok = false;
  exe.clear();
  return exe;
}
