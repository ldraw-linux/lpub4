
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

#ifndef RENDER_H
#define RENDER_H

class QString;
class QStringList;
class Meta;
class RotStepMeta;

class Render
{
  public:
    Render() {}
    virtual ~Render() {};
    virtual int renderCsi(const QStringList &, const QString &, Meta &) = 0;
    virtual int renderPli(const QString &,     const QString &, Meta &) = 0;
  protected:
    float cameraDistance(Meta &meta, float);
    int rotateParts(      RotStepMeta &rotStep,
                    const QStringList &parts,
                          QString     &ldrName);
};

extern Render *renderer;

class LDGLite : public Render
{
  public:
    LDGLite() {}
    virtual ~LDGLite() {}
    virtual int renderCsi(const QStringList &, const QString &, Meta &);
    virtual int renderPli(const QString &,     const QString &, Meta &);
};

class LDView : public Render
{
  public:
    LDView() {}
    virtual ~LDView() {}
    virtual int renderCsi(const QStringList &, const QString &, Meta &);
    virtual int renderPli(const QString &,     const QString &, Meta &);
};


#endif
