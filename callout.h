
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
 * This file describes the data structure that represents an LPub callout
 * (the steps of a submodel packed together and displayed next to the
 * assembly where the submodel is used).  
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef CALLOUTH
#define CALLOUTH

#include "ranges.h"
#include "placement.h"
#include "where.h"
#include "numberitem.h"

#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsLineItem>
#include <QGraphicsItemGroup>

class Pointer;
class CalloutPointerItem;
class CalloutBackgroundItem;
class QGraphicsView;
/*
 * There can be more than one callout per step, so we add
 * the ability to be in a list here
 */

class AbstractRangeElement;
class Callout : public Ranges {
  public:
    AbstractRangeElement  *parent;
    PlacementType          parentRelativeType;
    QGraphicsView         *view;
    int                    instances;
    PlacementNum           instanceCount;

    QList<Pointer *>            pointerList; /* Pointers and arrows */
    QList<CalloutPointerItem *> graphicsPointerList;

    CalloutBackgroundItem *background;

    Callout(
      AbstractRangeElement *_parent,
      PlacementType         _parentRelativeType,
      Meta                 &_meta,
      QGraphicsView        *_view);

    virtual AllocEnc allocType();

    virtual AllocMeta &allocMeta();

    virtual ~Callout();

    void appendPointer(Where &here, CalloutMeta &attrib);

    void addGraphicsItems(
      int   offsetX, int offsetY, 
      QRect &csiRect,
      QGraphicsItem *parent);

    void addGraphicsPointerItem(
      int calloutOffsetX,
      int calloutOffsetY,
      int csiOffsetX,
      int csiOffsetY,
      int csiOffset[2],
      int csiSize[2],
      Pointer *pointer,
      QGraphicsItem *parent);

    virtual void sizeIt();
    virtual void sizeitVert();
    virtual void addGraphicsItemsVert( 
                   int x, int y, QGraphicsItem *parent);

    virtual void sizeitHoriz();
    virtual void addGraphicsItemsHoriz(
                   int x, int y, QGraphicsItem *parent);

            void sizeitFreeform(int xx, int yy);
    virtual void drawTips(QPoint &delta);
    virtual void updatePointers(QPoint &delta);
};

class CalloutInstanceItem : public NumberPlacementItem
{
  Where defaultWhere;
public:
  CalloutInstanceItem(
    Meta                *meta,
    const char          *format,
    int                  _value,
    Where               &defaultWhere,
    QGraphicsItem       *parent);
protected:
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};

#endif
