
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
 * This file describes the graphical representation of a number displayed
 * in the graphics scene that is used to describe a building instruction
 * page.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef NUMBERITEM_H
#define NUMBERITEM_H

#include <QGraphicsTextItem>
#include "where.h"
#include "placement.h"
#include "metaitem.h"

class Where;

class NumberItem : public QGraphicsTextItem, public MetaItem
{
public:
  PlacementType  relativeType;
  PlacementType  parentRelativeType;
  Meta          *meta;
  FontMeta      *font;
  StringMeta    *color;
  MarginsMeta   *margin;
  int            value;
  QString        name;

  NumberItem();
  
  NumberItem(
    PlacementType  relativeType,
    PlacementType  parentRelativeType,
    Meta          *meta,
    NumberMeta    &number,
    const char    *format,
    int            _value,
    QString       &toolTip,
    QGraphicsItem *parent,
    QString        name = "");

  void setAttributes(
    PlacementType  relativeType,
    PlacementType  parentRelativeType,
    Meta          *meta,
    NumberMeta    &number,
    const char    *format,
    int            _value,
    QString       &toolTip,
    QGraphicsItem *parent,
    QString        name = "");

  void setFlags( GraphicsItemFlag flag, bool value)
  {
    QGraphicsTextItem::setFlag(flag,value);
  }
};

class NumberPlacementItem : public QGraphicsTextItem, public MetaItem
{
public:
  PlacementType  relativeType;
  PlacementType  parentRelativeType;
  Meta          *meta;
  FontMeta      *font;
  StringMeta    *color;
  MarginsMeta   *margin;
  PlacementMeta *placement;
  int            value;
  QString        name;

  bool           positionChanged;
  QPointF        position;

  NumberPlacementItem();
  
  NumberPlacementItem(
    PlacementType  relativeType,
    PlacementType  parentRelativeType,
    Meta          *meta,
    NumberPlacementMeta    &number,
    const char    *format,
    int            _value,
    QString       &toolTip,
    QGraphicsItem *parent,
    QString        name = "");

  void setAttributes(
    PlacementType  relativeType,
    PlacementType  parentRelativeType,
    Meta          *meta,
    NumberPlacementMeta &number,
    const char    *format,
    int            _value,
    QString       &toolTip,
    QGraphicsItem *parent,
    QString        name = "");

  void setFlags( GraphicsItemFlag flag, bool value)
  {
    QGraphicsTextItem::setFlag(flag,value);
  }

protected:
};

class PageNumberItem : public NumberPlacementItem
{
public:
  PageNumberItem(
    Meta          *meta,
    NumberPlacementMeta &number,
    const char    *format,
    int            _value,
    QGraphicsItem *parent);
protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};

class StepNumberItem : public NumberPlacementItem
{
  Where topOfRanges;
public:
  StepNumberItem(
    PlacementType  parentRelativeType,
    Where          topOfRanges,
    Meta          *meta,
    NumberPlacementMeta &number,
    const char    *format,
    int            _value,
    QGraphicsItem *parent,
    QString        name = "Step Number ");
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};
#endif
