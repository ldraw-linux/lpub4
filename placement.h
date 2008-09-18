
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


#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QSize>
#include <QRect>

#include "meta.h"
#include "metaitem.h"

class QPixmap;
//class QGraphicsScene;

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

class Placement {
  public:
    int           size[2];       // How big am I?
    int           loc[2];        // Where do I live within my group
    int           tbl[2];        // Where am I in my grid?
    PlacementType relativeType;  // What am I?
    PlacementMeta placement;     // Where am I placed?
    MarginsMeta   margin;        // How much room do I need?
    int           relativeToWidth;
    int           relativeToHeight;

    QList<Placement *> relativeToList; // things placed relative to me

    Placement()
    {
      size[0] = 0;
      size[1] = 0;
      loc[0]  = 0;
      loc[1]  = 0;
      tbl[0]  = 0;
      tbl[1]  = 0;
      relativeToWidth = 1;
      relativeToHeight = 1;
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

class IpiGrab;

class InsertPixmapItem : public QGraphicsPixmapItem, public MetaItem, public Placement
{
  public:
    QPointF    position;
    bool       positionChanged;
    int        grabSize;
    qreal      origWidth;
    qreal      origHeight;
    qreal      oldScale;
    IpiGrab   *grab[4];
    QPointF    points[4];

    enum SelectedPoint { TopLeft, TopRight, BottomRight, BottomLeft} selectedPoint;

    InsertMeta insertMeta;

    InsertPixmapItem();
    
    InsertPixmapItem(
      QPixmap    &pixmap,
      InsertMeta &insertMeta,
      QGraphicsItem *parent = 0);
    void placeGrabs();
    void whatPoint(IpiGrab *);
    void resize(QPointF);
    void changePicScale();
    
  protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

class IpiGrab : public QGraphicsRectItem
{
public:
  InsertPixmapItem *ipiItem;
  IpiGrab(
    InsertPixmapItem *ipiItem)
    :
    ipiItem(ipiItem)
  {
    setFlag(QGraphicsItem::ItemIsMovable,true);
  }
private:
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
};

class PlacementPixmap : public Placement {
  public:
    QPixmap   *pixmap;

    PlacementPixmap()
    {
    }
};

#if 0

class PlacementPixmapItem : public QGraphicsPixmapItem, public Placement, public MetaItem
{
  public:
    QPointF       position;
    bool          positionChanged;

    PlacementPixmapItem();
    
    PlacementPixmapItem(
      QPixmap       &pixmap,
      PlacementMeta &_placement,
      QGraphicsItem *parent = 0)
      
      : QGraphicsPixmapItem(pixmap,parent)
    {
      placement = _placement;
      
      PlacementData placementData = placement.value();

      size[0] = pixmap.width();
      size[1] = pixmap.height();
      setFlag(QGraphicsItem::ItemIsSelectable,true);
      setFlag(QGraphicsItem::ItemIsMovable,true);
    }
  protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
};

#endif

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
