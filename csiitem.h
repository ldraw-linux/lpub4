
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
#include "metaitem.h"

class Step;

class CsiItem : public QGraphicsPixmapItem, public MetaItem
{
public:
  Meta          *meta;
  AssemMeta     *assem;
  RcMeta        *divider;
  PlacementType  parentRelativeType;
  bool           multiStep;
  Context        context;
  Step          *step;
  int            submodelLevel;

  QPointF        position;
  bool           positionChanged;

  CsiItem(Step          *_step,
          Meta          *_meta,
          QPixmap       &pixmap,
          Context       &_context,
          int            _submodelLevel,
          QGraphicsItem *parent,
          PlacementType  _parentRelativeType);

  void setFlag(GraphicsItemFlag flag, bool value);
private:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
};


#endif

