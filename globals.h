
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
 * This header file describes the global setup dialogs used to configure
 * global settings for page, assembly, parts lists, callouts, step groups,
 * and projects.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QDialog>

/*****************************************************************
 *
 * Global to page
 *
 ****************************************************************/

class GlobalPagePrivate;
class GlobalPageDialog : public QDialog
{
  Q_OBJECT
private:
  GlobalPagePrivate *data;

public:
  GlobalPageDialog(QString &topLevelModel);
  ~GlobalPageDialog() {};
  static void getPageGlobals(QString topLevelFile);

public slots:
  void accept();
  void cancel();
};

/*****************************************************************
 *
 * Global to single step assem and step number
 *
 ****************************************************************/

class GlobalAssemPrivate;
class GlobalAssemDialog : public QDialog
{
  Q_OBJECT
private:
  GlobalAssemPrivate *data;

public:
  GlobalAssemDialog(QString &topLevelFile);
  ~GlobalAssemDialog() {}
  static void getAssemGlobals(QString topLevelFile);

public slots:
  void accept();
  void cancel();
};

/*****************************************************************
 *
 * Global to pli
 *
 ****************************************************************/

class GlobalPliPrivate;
class GlobalPliDialog : public QDialog
{
  Q_OBJECT
private:
  GlobalPliPrivate *data;

public:
  GlobalPliDialog(QString &topLevelFile);
  ~GlobalPliDialog() {}
  static void getPliGlobals(QString topLevelFile);

public slots:
  void accept();
  void cancel();
};

/*****************************************************************
 *
 * Global to callout
 *
 ****************************************************************/

class GlobalCalloutPrivate;
class GlobalCalloutDialog : public QDialog
{
  Q_OBJECT
private:
  GlobalCalloutPrivate *data;

public:
  GlobalCalloutDialog(QString &topLevelFile);
  ~GlobalCalloutDialog() {}
  static void getCalloutGlobals(QString topLevelFile);

public slots:
  void accept();
  void cancel();
};

/*****************************************************************
 *
 * Global to multiStep
 *
 ****************************************************************/

class GlobalMultiStepPrivate;
class GlobalMultiStepDialog : public QDialog
{
  Q_OBJECT
private:
  GlobalMultiStepPrivate *data;

public:
  GlobalMultiStepDialog(QString &topLevelFile);
  ~GlobalMultiStepDialog() {}
  static void getMultiStepGlobals(QString topLevelFile);

public slots:
  void accept();
  void cancel();
};


/*****************************************************************
 *
 * Global to project
 *
 ****************************************************************/

class GlobalProjectPrivate;
class GlobalProjectDialog : public QDialog
{
  Q_OBJECT
private:  
  GlobalProjectPrivate *data;

public:

  GlobalProjectDialog(const QString &topLevelFile);
  ~GlobalProjectDialog() {};  
  static void getProjectGlobals(const QString topLevelFile);
  
public slots:
  void accept();
  void cancel();
};
#endif
