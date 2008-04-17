
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

/****************************************************************************
 *
 * This class reads in, manages and writes out LDraw files.  While being
 * edited an MPD file, or a top level LDraw files and any sub-model files
 * are maintained in memory using this class.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "ldrawfiles.h"
#include <QtGui>
#include <QFile>
#include "name.h"
#include "rx.h"
#include "paths.h"

QStringList LDrawHeaderRx;

void LDrawFile::empty()
{
    _subFiles.clear();
    _subFileOrder.clear();
    _mpd = false;
}

void LDrawFile::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(NULL, 
                             QMessageBox::tr(LPUB),
                             QMessageBox::tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    // get rid of what's there before we load up new stuff

    empty();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    if (QFileInfo(fileName).suffix() == "mpd") {
      QDateTime datetime = QFileInfo(fileName).lastModified();
      loadMPDFile(fileName,datetime);
    } else {
      QFileInfo fileInfo(fileName);
      loadLDRFile(fileInfo.absolutePath(),fileInfo.fileName());
    }

    QApplication::restoreOverrideCursor();
}

void LDrawFile::loadMPDFile(const QString &fileName, QDateTime &datetime)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(NULL, 
                             QMessageBox::tr(LPUB),
                             QMessageBox::tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);

    QStringList contents;
    QString     mpdName;
    QRegExp sof("^\\s*0\\s+FILE\\s+(.*)$");
    QRegExp eof("^\\s*0\\s+NOFILE\\s*$");

    while ( ! in.atEnd()) {

      const QString line = in.readLine(0);

      if (line.contains(sof)) {
        if ( ! mpdName.isEmpty()) {
          insert(mpdName,contents,datetime);
          writeToTmp(mpdName,contents);
        }
        contents.clear();
        mpdName = sof.cap(1);

      } else if (line.contains(eof)) {
        if ( ! mpdName.isEmpty()) {
          insert(mpdName,contents,datetime);
          writeToTmp(mpdName,contents);
        }
        mpdName.clear();

      } else if ( ! mpdName.isEmpty() && line != "") {
        contents << line;
      }
    }

    if ( ! mpdName.isEmpty() && ! contents.isEmpty()) {
      insert(mpdName,contents,datetime);
      writeToTmp(mpdName,contents);
    }
    _mpd = true;
}

void LDrawFile::loadLDRFile(const QString &path, const QString &fileName)
{
    if (_subFiles[fileName]._contents.isEmpty()) {

      QString fullName(path + "/" + fileName);

      QFile file(fullName);
      if ( ! file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(NULL, 
                             QMessageBox::tr(LPUB),
                             QMessageBox::tr("Cannot read file %1:\n%2.")
                             .arg(fullName)
                             .arg(file.errorString()));
        return;
      }

      /* Read it in the first time to put into fileList in order of 
         appearance */

      QTextStream in(&file);
      QStringList contents;

      while ( ! in.atEnd()) {
        QString line = in.readLine(0);
        contents << line;
      }

      QDateTime datetime = QFileInfo(fullName).lastModified();
    
      insert(fileName,contents,datetime);
      writeToTmp(fileName,contents);

      /* read it a second time to find submodels */

      for (int i = 0; i < contents.size(); i++) {
        QString line = contents.at(i);
        QStringList tokens;

        split(line,tokens);

        if (line[0] == '1' && tokens.size() == 15) {
          const QString subModel = tokens[tokens.size()-1];
          fullName = path + "/" + subModel;
          if (QFile::exists(fullName)) {
            loadLDRFile(path,subModel);
          }
        }
      }
      _mpd = false;
    }
}

bool LDrawFile::saveFile(const QString &fileName)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool rc;
    if (_mpd) {
      rc = saveMPDFile(fileName);
    } else {
      rc = saveLDRFile(fileName);
    }
    QApplication::restoreOverrideCursor();
    return rc;
}

bool LDrawFile::saveMPDFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(NULL, 
                             QMessageBox::tr(LPUB),
                             QMessageBox::tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);

    for (int i = 0; i < _subFileOrder.size(); i++) {
      QString subFileName = _subFileOrder[i];
      QMap<QString, LDrawSubFile>::iterator f = _subFiles.find(subFileName);
      if (f != _subFiles.end()) {
        out << "0 FILE " << subFileName << endl;
        for (int j = 0; j < f.value()._contents.size(); j++) {
          out << f.value()._contents[j] << endl;
        }
        out << "0 NOFILE " << endl;
      }
    }
    return true;
}

bool LDrawFile::saveLDRFile(const QString &fileName)
{
    QString path = QFileInfo(fileName).path();
    QFile file;

    for (int i = 0; i < _subFileOrder.size(); i++) {
      QString writeFileName;
      if (i == 0) {
        writeFileName = fileName;
      } else {
        writeFileName = path + "/" + _subFileOrder[i];
      }
      file.setFileName(writeFileName);

      QMap<QString, LDrawSubFile>::iterator f = _subFiles.find(_subFileOrder[i]);
      if (f != _subFiles.end()) {
        if (f.value()._modified) {
          if (!file.open(QFile::WriteOnly | QFile::Text)) {
            QMessageBox::warning(NULL, 
              QMessageBox::tr(LPUB),
              QMessageBox::tr("Cannot write file %1:\n%2.")
              .arg(writeFileName)
              .arg(file.errorString()));
            return false;
          }
          QTextStream out(&file);
          for (int j = 0; j < f.value()._contents.size(); j++) {
            out << f.value()._contents[j] << endl;
          }
          file.close();
        }
      }
    }
    return true;
}

void LDrawFile::writeToTmp(
  const QString &fileName,
  const QStringList &contents)
{
  QString fname = QDir::currentPath() + "/" + Paths::tmpDir + "/" + fileName;
  QFile file(fname);
  if ( ! file.open(QFile::WriteOnly|QFile::Text)) {
    QMessageBox::warning(NULL,QMessageBox::tr("LPub"),
    QMessageBox::tr("Failed to open %1 for writing: %2") 
      .arg(fname) .arg(file.errorString()));
  } else {
    QTextStream out(&file);
    for (int i = 0; i < contents.size(); i++) {
      out << contents[i] << endl;
    }
    file.close();
  }
}
