
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
 * The class described in this file is the graphical representation of
 * a step's construction step image (CSI), or assembly image.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef csiH
#define csiH

#include <QGraphicsPixmapItem>
#include <QSize>
#include <QRect>
#include "meta.h"
#include "metaitem.h"

class Step;
class CsiGrab;

class CsiItem : public QGraphicsPixmapItem, public MetaItem
{
public:
  Meta          *meta;
  AssemMeta     *assem;
  RcMeta        *divider;
  PlacementType  parentRelativeType;
  bool           multiStep;
  Step          *step;
  int            submodelLevel;
  FloatMeta      modelScale;

  QPointF        position;
  bool           positionChanged;
  int            grabSize;
  
  qreal          origWidth;
  qreal          origHeight;
  qreal          oldScale;

  CsiGrab       *grab[4];
  QPointF        points[4];

  enum SelectedPoint { TopLeft, TopRight, BottomRight, BottomLeft} selectedPoint;

  CsiItem(Step          *_step,
          Meta          *_meta,
          QPixmap       &pixmap,
          int            _submodelLevel,
          QGraphicsItem *parent,
          PlacementType  _parentRelativeType);

  void setFlag(GraphicsItemFlag flag, bool value);

  void placeGrabs();
  void whatPoint(CsiGrab *);
  void resize(QPointF);
  void changeModelScale();

private:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
  QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

class CsiGrab : public QGraphicsRectItem
{
public:
  CsiItem *csiItem;
  CsiGrab(
    CsiItem *csiItem)
    :
    csiItem(csiItem)
  {
    setFlag(QGraphicsItem::ItemIsMovable,true);
  }
private:
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
};


#endif

