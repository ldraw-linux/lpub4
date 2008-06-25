
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
 * This class encapsulates the external renderers.  For now, this means
 * only ldglite.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include <QtGui>
#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include "render.h"
#include "resolution.h"
#include "meta.h"
#include "math.h"
#include "rx.h"
#include "lpub.h"
#include "lpub_preferences.h"
#include "paths.h"

Render *renderer;

LDGLite ldglite;
LDView  ldview;

QString const Render::getRenderer()
{
  if (renderer == &ldglite) {
    return "LDGLite";
  } else {
    return "LDView";
  }
}

void Render::setRenderer(QString const &name)
{
  if (name == "LDGLite") {
    renderer = &ldglite;
  } else {
    renderer = &ldview;
  }
}

/***************************************************************************
 *
 * The math for zoom factor.  1.0 is true size.
 *
 * 1 LDU is 1/64 of an inch
 *
 * LDGLite produces 72 DPI
 *
 * Camera angle is 0.01
 *
 * What distance do we need to put the camera, given a user chosen DPI,
 * to get zoom factor of 1.0?
 *
 **************************************************************************/

/***************************************************************************
 *
 * LDGLite renderer
 *
 **************************************************************************/

//#define LduDistance 5729.57
#define CA "-ca0.01"

static double pi = 4*atan(1.0);
// the default camera distance for real size
static float LduDistance = 10.0/tan(0.005*pi/180);

float Render::cameraDistance(
  Meta &meta,
  float scale)
{
  float onexone;
  float factor;

  // Do the math in pixels

  onexone  = 20*meta.LPub.resolution.ldu(); // size of 1x1 in units
  onexone *= meta.LPub.resolution.value();  // size of 1x1 in pixels
  onexone *= scale;
  factor   = meta.LPub.page.size.value(0)/onexone; // in pixels;
  
  return factor*LduDistance; 
}

int LDGLite::renderCsi(
  const QString     &addLine,
  const QStringList &csiParts,
  const QString     &pngName,
        Meta        &meta)
{
  int rc;

  /* Create the CSI DAT file */

  QString ldrName = QDir::currentPath() + "/" + Paths::tmpDir + "/csi.ldr";

  rc = rotateParts(addLine, meta.rotStep, csiParts, ldrName);
  if (rc < 0) {
    return rc;
  }

  /* determine camera distance */

  QString cg = "-cg0,0";
  
  QStringList arguments;

  int cd = cameraDistance(meta,meta.LPub.assem.modelScale.value());

  int width = meta.LPub.page.size.value(0);
  int height = meta.LPub.page.size.value(1);

  QString v  = QString("-v%1,%2")   .arg(width)
                                    .arg(height);
  QString o  = QString("-o0,-%1")   .arg(height/6);
  QString mf = QString("-mF%1")     .arg(pngName);
                                    // ldglite always deals in 72 DPI
  QString w  = QString("-W%1")      .arg(int(resolution/72.0+0.5));

  cg = QString("-cg0.0,0.0,%1") .arg(cd);

  arguments << "-l3";
  arguments << "-i2";
  arguments << CA;
  arguments << cg;
  arguments << "-J";
  arguments << v;
  arguments << o;
  arguments << w;
  arguments << "-q";

  QStringList list;
  list = meta.LPub.assem.ldgliteParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
    if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
    }
  }

  arguments << mf;
  arguments << ldrName;
  
  QProcess    ldglite;
  QStringList env = QProcess::systemEnvironment();
  env << "LDRAWDIR=" + Preferences::ldrawPath;
  ldglite.setEnvironment(env);
  ldglite.setWorkingDirectory(QDir::currentPath()+"/"+Paths::tmpDir);
  ldglite.setStandardErrorFile(QDir::currentPath() + "/stderr");
  ldglite.setStandardOutputFile(QDir::currentPath() + "/stdout");
  ldglite.start(Preferences::ldgliteExe,arguments);
  if ( ! ldglite.waitForFinished()) {
    if (ldglite.exitCode() != 0) {
      QByteArray status = ldglite.readAll();
      QString str;
      str.append(status);
      QMessageBox::warning(NULL,
                           QMessageBox::tr("LPub"),
                           QMessageBox::tr("LDGlite failed\n%1") .arg(str));
      return -1;
    }
  }
  QFile::remove(ldrName);
  return 0;
}

  
int LDGLite::renderPli(
  const QString &ldrName,
  const QString &pngName,
  Meta    &meta)
{
  int width  = meta.LPub.page.size.value(0);
  int height = meta.LPub.page.size.value(1);
  
  /* determine camera distance */

  int cd = cameraDistance(meta,meta.LPub.pli.modelScale.value());

  QString cg = QString("-cg%1,%2,%3") .arg(meta.LPub.pli.angle.value(0)) 
                                      .arg(meta.LPub.pli.angle.value(1))
                                      .arg(cd);

  QString v  = QString("-v%1,%2")   .arg(width)
                                    .arg(height);
  QString o  = QString("-o0,-%1")   .arg(height/6);
  QString mf = QString("-mF%1")     .arg(pngName);
                                    // ldglite always deals in 72 DPI
  QString w  = QString("-W%1")      .arg(int(resolution/72.0+0.5));

  QStringList arguments;
  arguments << "-l3";
  arguments << "-i2";
  arguments << CA;
  arguments << cg;
  arguments << "-J";
  arguments << v;
  arguments << o;
  arguments << w;
  arguments << "-q";

  QStringList list;
  list = meta.LPub.pli.ldgliteParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
	if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
	}
  }
  arguments << mf;
  arguments << ldrName;
  
  QProcess    ldglite;
  QStringList env = QProcess::systemEnvironment();
  env << "LDRAWDIR=" + Preferences::ldrawPath;
  ldglite.setEnvironment(env);  
  ldglite.setWorkingDirectory(QDir::currentPath());
  ldglite.setStandardErrorFile(QDir::currentPath() + "/stderr");
  ldglite.setStandardOutputFile(QDir::currentPath() + "/stdout");
  ldglite.start(Preferences::ldgliteExe,arguments);
  if (! ldglite.waitForFinished()) {
    if (ldglite.exitCode()) {
      QByteArray status = ldglite.readAll();
      QString str;
      str.append(status);
      QMessageBox::warning(NULL,
                           QMessageBox::tr("LPub"),
                           QMessageBox::tr("LDGlite failed\n%1") .arg(str));
      return -1;
    }
  }
  return 0;
}


/***************************************************************************
 *
 * LDView renderer
 *
 **************************************************************************/

int LDView::renderCsi(
  const QString     &addLine,
  const QStringList &csiParts,
  const QString     &pngName,
        Meta        &meta)
{
  int rc;

  /* Create the CSI DAT file */

  QString ldrName = QDir::currentPath() + "/" + Paths::tmpDir + "/csi.ldr";

  rc = rotateParts(addLine,meta.rotStep, csiParts, ldrName);

  if (rc < 0) {
    return rc;
  }

  /* determine camera distance */
  
  QStringList arguments;

  int cd = cameraDistance(meta,meta.LPub.assem.modelScale.value());
  int width = meta.LPub.page.size.value(0);
  int height = meta.LPub.page.size.value(1);

  QString w  = QString("-SaveWidth=%1") .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString s  = QString("-SaveSnapShot=%1") .arg(pngName);

  QString cg = QString("-cg0.0,0.0,%1") .arg(cd);

  arguments << CA;
  arguments << cg;
  arguments << "-SaveAlpha=1";
  arguments << "-AutoCrop=1";
  arguments << "-ShowHighlightLines=1";
  arguments << "-ConditionalHighlights=1";
  arguments << "-SaveZoomToFit=0";
  arguments << "-SubduedLighting=1";
  arguments << "-UseSpecular=0";
  arguments << "-LightVector=0,1,1";
  arguments << "-SaveActualSize=0";
  arguments << w;
  arguments << h;
  arguments << s;

  QStringList list;
  list = meta.LPub.assem.ldviewParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
    if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
    }
  }
  arguments << ldrName;
  
  QProcess    ldview;
  ldview.setEnvironment(QProcess::systemEnvironment());
  ldview.setWorkingDirectory(QDir::currentPath()+"/"+Paths::tmpDir);
  ldview.start(Preferences::ldviewExe,arguments);

  if ( ! ldview.waitForFinished()) {
    if (ldview.exitCode() != 0 || 1) {
      QByteArray status = ldview.readAll();
      QString str;
      str.append(status);
      QMessageBox::warning(NULL,
                           QMessageBox::tr("LPub"),
                           QMessageBox::tr("LDView failed\n%1") .arg(str));
      return -1;
    }
  }
  QFile::remove(ldrName);
  return 0;
}

  
int LDView::renderPli(
  const QString &ldrName,
  const QString &pngName,
  Meta    &meta)
{
  int width  = meta.LPub.page.size.value(0);
  int height = meta.LPub.page.size.value(1);
  
  QFileInfo fileInfo(ldrName);
  
  if ( ! fileInfo.exists()) {
    return -1;
  }

  /* determine camera distance */
  
  int cd = cameraDistance(meta,meta.LPub.pli.modelScale.value());

  QString cg = QString("-cg%1,%2,%3") .arg(meta.LPub.pli.angle.value(0)) 
                                      .arg(meta.LPub.pli.angle.value(1))
                                      .arg(cd);
  QString w  = QString("-SaveWidth=%1")  .arg(width);
  QString h  = QString("-SaveHeight=%1") .arg(height);
  QString s  = QString("-SaveSnapShot=%1") .arg(pngName);

  QStringList arguments;
  arguments << CA;
  arguments << cg;
  arguments << "-SaveAlpha=1";
  arguments << "-AutoCrop=1";
  arguments << "-ShowHighlightLines=1";
  arguments << "-ConditionalHighlights=1";
  arguments << "-SaveZoomToFit=0";
  arguments << "-SubduedLighting=1";
  arguments << "-UseSpecular=0";
  arguments << "-LightVector=0,1,1";
  arguments << "-SaveActualSize=0";
  arguments << w;
  arguments << h;
  arguments << s;

  QStringList list;
  list = meta.LPub.pli.ldviewParms.value().split("\\s+");
  for (int i = 0; i < list.size(); i++) {
    if (list[i] != "" && list[i] != " ") {
      arguments << list[i];
    }
  }
  arguments << ldrName;

  QProcess    ldview;
  ldview.setEnvironment(QProcess::systemEnvironment());
  ldview.setWorkingDirectory(QDir::currentPath());
  ldview.start(Preferences::ldviewExe,arguments);
  if ( ! ldview.waitForFinished()) {
    if (ldview.exitCode() != 0) {
      QByteArray status = ldview.readAll();
      QString str;
      str.append(status);
      QMessageBox::warning(NULL,
                           QMessageBox::tr("LPub"),
                           QMessageBox::tr("LDView failed\n%1") .arg(str));
      return -1;
    }
  }
  return 0;
}
