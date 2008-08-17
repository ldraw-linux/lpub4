
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

#ifndef LDRAWFILES_H
#define LDRAWFILES_H

#include <QMessageBox>
#include <QStringList>
#include <QString>
#include <QMap>
#include <QDateTime>
#include <QList>
#include <QRegExp>

extern QList<QRegExp> LDrawHeaderRegExp;

class LDrawSubFile {
  public:
    QStringList _contents;
    bool        _modified;
    QDateTime   _datetime;
    int         _numSteps;

    LDrawSubFile()
    {
    }

    LDrawSubFile(
      const QStringList &contents,
            QDateTime   &datetime)
    {
      _contents << contents;
      _datetime = datetime;
      _modified = false;
      _numSteps = 0;
    }
    ~LDrawSubFile()
    {
      _contents.clear();
    }
};

class LDrawFile {
  private:
    QMap<QString, LDrawSubFile> _subFiles;
    QStringList                  _subFileOrder;
    QStringList                  _emptyList;
    QString                      _emptyString;
    bool                                _mpd;
    void insert(const QString     &fileName, 
                      QStringList &contents, 
                      QDateTime   &datetime)
    {
      QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);

      if (i != _subFiles.end()) {
        _subFiles.erase(i);
      }
      LDrawSubFile subFile(contents,datetime);
      _subFiles.insert(fileName,subFile);
      _subFileOrder << fileName;
    }

  public:
    LDrawFile();
    ~LDrawFile()
    {
      _subFiles.empty();
    }
    bool isMpd()
    {
      return _mpd;
    }
    QString topLevelFile()
    {
      if (_subFileOrder.size()) {
        return _subFileOrder[0];
      } else {
        return _emptyString;
      }
    }
    void setNumSteps(const QString &fileName, const int numSteps)
    {
      QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);
      if (i != _subFiles.end()) {
        i.value()._numSteps = numSteps;
      }      
    }
    int numSteps(const QString &fileName)
    {
      QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);
      if (i != _subFiles.end()) {
        return i.value()._numSteps;
      }
      return 0;
    }
    bool contains(const QString &file)
    {
      return _subFiles.contains(file);
    }
    void empty();
    bool modified()
    {
      QString key;
      bool    modified = false;
      foreach(key,_subFiles.keys()) {
        modified |= _subFiles[key]._modified;
      }
      return modified;
    }
    bool modified(const QString &fileName)
    {
      QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);
      if (i != _subFiles.end()) {
        return i.value()._modified;
      } else {
        return false;
      }
    }
    QStringList contents(const QString &fileName)
    {
      QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);

      if (i != _subFiles.end()) {
        QStringList contents = i.value()._contents;
        QString string = contents.join("\n");
        return contents;
      } else {
        return _emptyList;
      }
    }
    void setContents(const QString     &fileName, 
                     const QStringList &contents)
    {
      QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);

      if (i != _subFiles.end()) {
        i.value()._modified = true;
        //i.value()._datetime = QDateTime::currentDateTime();
        i.value()._contents = contents;
      }
    }
    bool older(const QStringList &submodelStack, 
               const QDateTime &datetime)
    {
      QString fileName;
      foreach (fileName, submodelStack) {
        QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);
        if (i != _subFiles.end()) {
          QDateTime fileDatetime = i.value()._datetime;
          if (fileDatetime > datetime) {
            return false;
          }
        } else {
          return false;
        }
      }
      return true;
    }
    void loadFile(const QString &fileName);
    void loadMPDFile(const QString &fileName, QDateTime &datetime);
    void loadLDRFile(const QString &path, const QString &fileName);

    QStringList subFileOrder() {
      return _subFileOrder;
    }

    QString readLine(const QString &fileName, int lineNumber)
    {
      QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);

      if (i != _subFiles.end()) {
        return i.value()._contents[lineNumber];
      }
      QString empty;
      return empty;
    }

    void insertLine(const QString &fileName, int lineNumber, const QString &line)
    {
      QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);

      if (i != _subFiles.end()) {
        i.value()._contents.insert(lineNumber,line);
        i.value()._modified = true;
 //       i.value()._datetime = QDateTime::currentDateTime();
      }
    }
      
    void replaceLine(const QString &fileName, int lineNumber, const QString &line)
    {
      QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);

      if (i != _subFiles.end()) {
        i.value()._contents[lineNumber] = line;
        i.value()._modified = true;
//        i.value()._datetime = QDateTime::currentDateTime();
      }
    }

    void deleteLine(const QString &fileName, int lineNumber)
    {
      QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);

      if (i != _subFiles.end()) {
        i.value()._contents.removeAt(lineNumber);
        i.value()._modified = true;
//        i.value()._datetime = QDateTime::currentDateTime();
      }
    }

    void changeContents(const QString &fileName, 
                              int      position, 
                              int      charsRemoved, 
                        const QString &charsAdded)
    {
      QString all = contents(fileName).join("\n");
      all.remove(position,charsRemoved);
      all.insert(position,charsAdded);
      setContents(fileName,all.split("\n"));
    }

    int size(const QString &fileName)
    {
      QMap<QString, LDrawSubFile>::iterator i = _subFiles.find(fileName);

      if (i != _subFiles.end()) {
        return i.value()._contents.size();
      }
      return 0;
    }

    bool saveFile(const QString &fileName);
    bool saveMPDFile(const QString &filename);
    bool saveLDRFile(const QString &filename);
    void writeToTmp(const QString &fileName, const QStringList &);
};

int split(const QString &line, QStringList &argv);
bool isHeader(QString &line);


#endif
