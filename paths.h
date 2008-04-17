
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

#ifndef PATHS_H
#define PATHS_H

#include <QString>
#include <QDir>

#define LDRAWPATH       "LDRAWPATH"
#define LDGLITEEXE      "LDGLITEEXE"
#define LDVIEWEXE       "LDVIEWEXE"
#define PLIFILE         "PLIFILE"

class Paths {
private:
    static QString getPath(
      bool    refind,
      QString exe, 
      QString name, 
      QString suffix,
      QString title, 
       bool *ok, 
      QWidget *parent = 0);

public:

    /* Absolute needs */
       
    static QString ldrawPath;
    static void    getLDrawPath(bool refind = false, QWidget *parent=0);

    static QString ldgliteExe;
    static QString getLdglitePath(bool *ok, bool refind = false, QWidget *parent=0);

    static QString ldviewExe;
    static QString getLdviewPath(bool *ok, bool refind = false, QWidget *parent=0);

    static QString pliFile;
    static QString getPliPath(bool *ok, bool refind = false, QWidget *parent=0);

    static QString lpubPath;
    static void    getLPubPath();

    static void mkdirs()
    {
      QDir dir;
      dir.mkdir(lpubDir);
      dir.mkdir(tmpDir);
      dir.mkdir(outputDir);
      dir.mkdir(assemDir);
      dir.mkdir(partsDir);
    }
    static QString lpubDir;
    static QString tmpDir;
    static QString outputDir;
    static QString assemDir;
    static QString partsDir;
};
#endif
