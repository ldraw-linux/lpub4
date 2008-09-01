
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
#include <QList>
#include <QRegExp>
#include "name.h"
#include "paths.h"

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
    
    // allow files ldr suffix to allow for MPD
    
    bool mpd = false;

    QTextStream in(&file);

    QStringList contents;
    QString     mpdName;
    QRegExp sof("^\\s*0\\s+FILE\\s+(.*)$");
    QRegExp part("^\\s*1\\s+.*$");

    while ( ! in.atEnd()) {
      const QString line = in.readLine(0);
      if (line.contains(sof)) {
        mpd = true;
        break;
      }
      if (line.contains(part)) {
        mpd = false;
        break;
      }
    }
    
    file.close();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    if (mpd) {
      QDateTime datetime = QFileInfo(fileName).lastModified();
      loadMPDFile(fileName,datetime);
    } else {
      QFileInfo fileInfo(fileName);
      loadLDRFile(fileInfo.absolutePath(),fileInfo.fileName());
    }
    
    countInstances(topLevelFile(),false);
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
  
  QByteArray Line = line.toAscii();
  
  volatile char LIne[256];
  
  int i;
  for (i = 0; i < Line.size(); i++) {
    LIne[i] = Line[i];
  }
  LIne[i] = '\0';

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
 
bool LDrawFile::mirrored(
  const QStringList &tokens)
{  
  /* 5  6  7
     8  9 10
    11 12 13 */
    
  float a = tokens[5].toFloat();
  float b = tokens[6].toFloat();
  float c = tokens[7].toFloat();
  float d = tokens[8].toFloat();
  float e = tokens[9].toFloat();
  float f = tokens[10].toFloat();
  float g = tokens[11].toFloat();
  float h = tokens[12].toFloat();
  float i = tokens[13].toFloat();
  
  return a*e*i - a*f*h - b*d*i + b*f*g + c*d*h - c*e*g < 0;
}

void LDrawFile::countInstances(const QString &fileName, bool isMirrored)
{
  bool partsAdded = false;
  
  QMap<QString, LDrawSubFile>::iterator f = _subFiles.find(fileName);
  if (f != _subFiles.end()) {
    int j = f->_contents.size();
    for (int i = 0; i < j; i++) {
      QStringList tokens;
      split(f->_contents[i],tokens);
      
      /* Sorry, but models that are callouts are not counted as instances */
      
      if (tokens.size() == 4 && 
          tokens[0] == "0" && 
          (tokens[1] == "LPUB" || tokens[1] == "!LPUB") && 
          tokens[2] == "CALLOUT" && 
          tokens[3] == "BEGIN") {

        for (++i; i < j; i++) {
          split(f->_contents[i],tokens);
          if (tokens.size() == 4 &&
              tokens[0] == "0" && 
              (tokens[1] == "LPUB" || tokens[1] == "!LPUB") && 
              tokens[2] == "CALLOUT" && 
              tokens[4] == "END") {
            
            break;
          }
        }
      } else if (tokens.size() >= 2 && tokens[0] == "0" && 
                (tokens[1] == "STEP" || tokens[1] == "ROTSTEP")) {
        f->_numSteps += isMirrored && f->_mirrorInstances == 0 ||
                      ! isMirrored && f->_instances == 0;
        partsAdded = false;
      } else if (tokens.size() == 15 && tokens[0] == "1") {
        countInstances(tokens[14],mirrored(tokens));
        partsAdded = true;
      }
    }
    f->_numSteps += partsAdded &&
                    (   isMirrored && f->_mirrorInstances == 0 ||
                      ! isMirrored && f->_instances == 0);
  }
  if (isMirrored) {
    ++f->_mirrorInstances;
  } else {
    ++f->_instances;
  }
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

int split(const QString &line, QStringList &argv)
{
  QString     chopped = line;
  int         p = 0;
  int         length = chopped.length();

  if (p == length) {
    return 0;
  }
  
  while (chopped[p] == ' ') {
    if (++p == length) {
      return -1;
    }
  }
  
  if (chopped[p] == '1') {

    argv << "1";
    p += 2;
    if (p >= length) {
      return -1;
    }
    while (chopped[p] == ' ') {
      if (++p >= length) {
        return -1;
      }
    }
    
    if (chopped.mid(p,5) == "GHOST") {
      p += 6;
      if (p >= length) {
        return -1;
      }
      while (chopped[p] == ' ') {
        if (++p >= length) {
          return -1;
        }
      }
    }
    
    // color x y z a b c d e f g h i
    
    for (int i = 0; i < 13; i++) {
      QString token;
      
      while (chopped[p] != ' ') {
        token += chopped[p];
        if (++p >= length) {
          return -1;
        }
      }
      argv << token;
      while (chopped[p] == ' ') {
        if (++p >= length) {
          return -1;
        }
      }
    }
    
    argv << chopped.mid(p);
  } else if (chopped[p] == '0' || chopped[p] >= '2' && chopped[p] <= '5') {
    chopped = chopped.mid(p);  
    argv << chopped.split(" ",QString::SkipEmptyParts);
  }
  
  return 0;
}

QList<QRegExp> LDrawHeaderRegExp;

LDrawFile::LDrawFile()
{
    {
       LDrawHeaderRegExp
       << QRegExp("^\\s*0\\s+Author[^\n]*") 
       << QRegExp("^\\s*0\\s+!CATEGORY[^\n]*")
       << QRegExp("^\\s*0\\s+!CMDLINE[^\n]*")
       << QRegExp("^\\s*0\\s+!COLOUR[^\n]*")
       << QRegExp("^\\s*0\\s+!HELP[^\n]*")
       << QRegExp("^\\s*0\\s+!HISTORY[^\n]*")
       << QRegExp("^\\s*0\\s+!KEYWORDS[^\n]*")
       << QRegExp("^\\s*0\\s+!LDRAW_ORG[^\n]*")
       << QRegExp("^\\s*0\\s+LDRAW_ORG[^\n]*")
       << QRegExp("^\\s*0\\s+!LICENSE[^\n]*")
       << QRegExp("^\\s*0\\s+Name[^\n]*")
       << QRegExp("^\\s*0\\s+Official[^\n]*")
       << QRegExp("^\\s*0\\s+Unofficial[^\n]*") 
       << QRegExp("^\\s*0\\s+Un-official[^\n]*") 
       << QRegExp("^\\s*0\\s+Original LDraw[^\n]*")
       << QRegExp("^\\s*0\\s+~Moved to[^\n]*")
       << QRegExp("^\\s*0\\s+ROTATION[^\n]*");
    }
}

bool isHeader(QString &line)
{
  int size = LDrawHeaderRegExp.size();
   
  for (int i = 0; i < size; i++) {
    if (line.contains(LDrawHeaderRegExp[i])) {
      return true;
    }
  }
  
  return false;
}
   