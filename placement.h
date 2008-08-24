
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
 * This class implements a fundamental class for placing things relative to
 * other things.  This concept is the cornerstone of LPub's meta commands
 * for describing what building instructions should look like without having
 * to specify inches, centimeters or pixels.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef placementH
#define placementH


#include <QGraphicsItem>
#include <QPixmap>
#include "meta.h"

class QGraphicsScene;

//---------------------------------------------------------------------------
//
// ccccccccccc
// csssssssssc
// cscccccccsc
// cscpppppcsc
// cscpcccpcsc
// cscpcacpcsc
// cscpcccpcsc
// cscpppppcsc
// cscccccccsc
// csssssssssc
// ccccccccccc
//
//---------------------------------------------------------------------------

enum Boundary {
  StartOfSteps       = 1,
  StartOfRange       = 2,
  EndOfRange         = 4,
  EndOfSteps         = 8,
  StartAndEndOfSteps = 9,
  StartAndEndOfRange = 6,
  Middle             = 16
};

enum {
  TblCo0 = 0,
  TblSn0,
  TblCo1,
  TblPli0,
  TblCo2,
  TblCsi,
  TblCo3,
  TblPli1,
  TblCo4,
  TblSn1,
  TblCo5,
  NumPlaces
};

enum dim {
  XX = 0,
  YY = 1
};

class Placement : QObject {
  Q_OBJECT
  public:
    int           size[2];       // How big am I?
    int           offset[2];     // Where do I live within my group
    int           tbl[2];        // Where am I in my grid?
    PlacementType relativeType;  // What am I?
    PlacementMeta placement;     // Where am I placed?
    MarginsMeta   margin;        // How much room do I need?

    QList<Placement *> relativeToList; // things placed relative to me

    Placement()
    {
      size[0]   = 0;
      size[1]   = 0;
      offset[0] = 0;
      offset[1] = 0;
      tbl[0]    = 0;
      tbl[1]    = 0;
    }

    virtual ~Placement()
    {
      relativeToList.empty();
    }

    void appendRelativeTo(Placement *element);

    int  relativeTo(
      Placement *step);

    int relativeToMs(
      Placement *callout);

    void placeRelative(
      Placement *placement);

    void placeRelative(
      Placement *placement,
      int        margin[]);

    void justifyX(
      int origin,
      int height);

    void justifyY(
      int origin,
      int height);
};

class PlacementImage : public Placement {
  public:
    QImage    *image;
    qreal      scale;
    PlacementImage()
    {
    }
};

class PlacementPixmap : public Placement {
  public:
    QPixmap   *pixmap;
    PlacementPixmap()
    {
    }
};

class PlacementNum : public Placement {
  public:
    QString str;
    QString font;
    QString color;
    int  number;

    PlacementNum()
    {
      number = 0;
    }
    void format(char *format)
    {
      str.sprintf(format,number);
    }
    void sizeit();
    void sizeit(QString fmt);
};

#endif
