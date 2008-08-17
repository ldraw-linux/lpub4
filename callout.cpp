
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

#include <QtGui>

#include "callout.h"
#include "calloutbackgrounditem.h"
#include "pointer.h"
#include "pointeritem.h"
#include "numberitem.h"
#include "ranges.h"
#include "range.h"
#include "step.h"
#include "lpub.h"
#include "placementdialog.h"
#include "commonmenus.h"

//---------------------------------------------------------------------------

Callout::Callout(
  Meta                 &_meta,
  QGraphicsView        *_view)
{
  view          = _view;
  relativeType  = CalloutType;
  meta = _meta;
  instances = 1;
}

Callout::~Callout()
{
  list.clear();
  pointerList.clear();
}

AllocEnc Callout::allocType()
{
  if (relativeType == CalloutType) {
    return meta.LPub.callout.alloc.value();
  } else {
    return meta.LPub.multiStep.alloc.value();
  }
}

AllocMeta &Callout::allocMeta()
{
  if (relativeType == CalloutType) {
    return meta.LPub.callout.alloc;
  } else {
    return meta.LPub.multiStep.alloc;
  }
}

void Callout::appendPointer(const Where &here, CalloutMeta &attrib)
{
  Pointer *pointer = new Pointer(here,attrib);
  pointerList.append(pointer);
}

void Callout::sizeIt()
{
  if (meta.LPub.callout.alloc.value() == Vertical) {
    sizeitVert();
  } else {
    sizeitHoriz();
  }
}

void Callout::sizeitVert()
{
  Ranges::sizeitVert();
  BorderData borderData = meta.LPub.callout.border.value();

  size[XX] += borderData.margin[XX];
  size[YY] += borderData.margin[YY];

  size[XX] += borderData.thickness;
  size[YY] += borderData.thickness;

  if (instances > 1) {
    instanceCount.number = instances;
    instanceCount.placement = meta.LPub.callout.instance.placement;
    instanceCount.margin    = meta.LPub.callout.instance.margin;
    instanceCount.font = meta.LPub.callout.instance.font.value();
    instanceCount.sizeit();

    PlacementData placementData = meta.LPub.callout.instance.placement.value();

    if (placementData.preposition == Outside) {
      BorderData borderData = meta.LPub.callout.border.value();
      int margin[2] = {borderData.margin[0], borderData.margin[1]};
      placeRelative(&instanceCount,margin);
      switch (placementData.placement) {
        case TopLeft:
        case Left:
        case BottomLeft:
        case TopRight:
        case Right:
        case BottomRight:
          size[XX] += instanceCount.margin.value(XX) + instanceCount.size[XX];
        break;
        default:
        break;
      } 
      switch (placementData.placement) {
        case TopLeft:
        case Top:
        case TopRight:
        case BottomLeft:
        case Bottom:
        case BottomRight:
          size[YY] += instanceCount.margin.value(YY) + instanceCount.size[YY];
        break;
        default:
        break;
      }
    } else {
      placeRelative(&instanceCount);
    }
  }

  size[XX] += borderData.margin[XX];
  size[YY] += borderData.margin[YY];

  size[XX] += borderData.thickness;
  size[YY] += borderData.thickness;
}

void Callout::sizeitHoriz()
{
  Ranges::sizeitHoriz();
  BorderData borderData = meta.LPub.callout.border.value();

  size[XX] += borderData.margin[XX];
  size[YY] += borderData.margin[YY];

  size[XX] += borderData.thickness;
  size[YY] += borderData.thickness;

  if (instances > 1) {
    instanceCount.number = instances;
    instanceCount.placement = meta.LPub.callout.instance.placement;
    instanceCount.margin    = meta.LPub.callout.instance.margin;
    instanceCount.font = meta.LPub.callout.instance.font.value();
    instanceCount.sizeit();

    PlacementData placementData = instanceCount.placement.value();

    if (placementData.preposition == Outside) {
      BorderData borderData = meta.LPub.callout.border.value();
      int margin[2] = {borderData.margin[0], borderData.margin[1]};
      placeRelative(&instanceCount,margin);
      switch (placementData.placement) {
        case TopLeft:
        case Left:
        case BottomLeft:
        case TopRight:
        case Right:
        case BottomRight:
          size[XX] += instanceCount.margin.value(XX) + instanceCount.size[XX];
        break;
        default:
        break;
      }
      switch (placementData.placement) {
        case TopLeft:
        case Top:
        case TopRight:
        case BottomLeft:
        case Bottom:
        case BottomRight:
          size[YY] += instanceCount.margin.value(YY) + instanceCount.size[YY];
        break;
        default:
        break;
      }
    } else {
      placeRelative(&instanceCount);
    }
  }

  size[XX] += borderData.margin[XX];
  size[YY] += borderData.margin[YY];

  size[XX] += borderData.thickness;
  size[YY] += borderData.thickness;
}

void Callout::addGraphicsItems(
  int            offsetX,
  int            offsetY,
  QRect         &csiRect,
  QGraphicsItem *parent)
{
  QRect calloutRect(offsetX + offset[XX],offsetY + offset[YY],size[XX],size[YY]);

  background = new CalloutBackgroundItem(
                     this,
                     calloutRect,
                     csiRect,
                     parentRelativeType,
                    &meta,
                     meta.submodelStack.size(),
                     path(),
                     parent,
                     view);

  background->setPos(offsetX + offset[XX],offsetY + offset[YY]);

  int saveX = offset[XX];
  int saveY = offset[YY];

  BorderData borderData = meta.LPub.callout.border.value();

  offset[XX] = borderData.margin[0];
  offset[YY] = borderData.margin[1];
 
  if (meta.LPub.callout.alloc.value() == Vertical) {
    addGraphicsItemsVert(0,
                         borderData.thickness,
                         background);
  } else {
    addGraphicsItemsHoriz(borderData.thickness,
                          0,
                          background);
  }
  offset[XX] = saveX;
  offset[YY] = saveY;
}

void Callout::addGraphicsItemsVert(
  int            offsetX,
  int            offsetY,
  QGraphicsItem *parent)
{
  int margin;

  if (instanceCount.number > 1) {
    PlacementData placementData = instanceCount.placement.value();
    switch (placementData.placement) {
      case TopLeft:
      case Left:
      case BottomLeft:
        margin   = meta.LPub.callout.instance.margin.value(XX);
        offsetX += instanceCount.size[XX] + margin;
      break;
      default:
      break;
    }

    switch (placementData.placement) {
      case TopLeft:
      case Top:
      case TopRight:
        margin   = meta.LPub.callout.instance.margin.value(YY);
        offsetY += instanceCount.size[YY] + margin;
      break;
      default:
      break;
    }
    CalloutInstanceItem *item = new CalloutInstanceItem(
      this,
     &meta, 
      "%dx",
      instanceCount.number,
      parent);
    item->setPos(offsetX + instanceCount.offset[0], offsetY + instanceCount.offset[1]);
  }

  Ranges::addGraphicsItemsVert(offsetX,
                               offsetY,
                               parent);
}

void Callout::addGraphicsItemsHoriz(
  int            offsetX,
  int            offsetY,
  QGraphicsItem *parent)
{
  if (instanceCount.number > 1) {
    PlacementData placementData = instanceCount.placement.value();
    switch (placementData.placement) {
      case TopLeft:
      case Left:
      case BottomLeft:
        offsetX += instanceCount.size[XX] + meta.LPub.callout.instance.margin.value(XX);
      break;
      default:
      break;
    }

    switch (placementData.placement) {
      case TopLeft:
      case Top:
      case TopRight:
        offsetY += instanceCount.size[YY] + meta.LPub.callout.instance.margin.value(YY);
      break;
      default:
      break;
    }
    CalloutInstanceItem *item = new CalloutInstanceItem(
      this,&meta, "%dx",instanceCount.number,parent);
    item->setPos(offsetX + instanceCount.offset[0], offsetY + instanceCount.offset[1]);
  }

  Ranges::addGraphicsItemsHoriz(offsetX,
                                offsetY,
                                parent);
}

void Callout::sizeitFreeform(
  int xx,
  int yy)
{
  Ranges::sizeitFreeform(xx,yy);

  size[XX] += 2*meta.LPub.callout.border.value().thickness;
  size[YY] += 2*meta.LPub.callout.border.value().thickness;

  if (instanceCount.number > 1) {
    instanceCount.sizeit("%1x");
    PlacementData placementData = instanceCount.placement.value();
    if (placementData.preposition == Outside) {

      switch (placementData.placement) {
        case TopLeft:
        case Left:
        case BottomLeft:
        case TopRight:
        case Right:
        case BottomRight:
          size[XX] += margin.value(XX) + instanceCount.size[XX];
        break;
        default:
        break;
      }
      switch (placementData.placement) {
        case TopLeft:
        case Top:
        case TopRight:
        case BottomLeft:
        case Bottom:
        case BottomRight:
          size[YY] += margin.value(YY) + instanceCount.size[YY];
        break;
        default:
        break;
      }
      placementData.preposition = Inside;
      instanceCount.placement.setValue(placementData);

      placeRelative(&instanceCount);

      placementData = instanceCount.placement.value();
      placementData.preposition = Inside;
      instanceCount.placement.setValue(placementData);
    } else {
      size[YY] += margin.value(YY);
      placeRelative(&instanceCount);
    }
  }
}

CalloutInstanceItem::CalloutInstanceItem(
  Callout             *_callout,
  Meta                *_meta,
  const char          *_format,
  int                  _value,
  QGraphicsItem       *_parent)
{
  callout = _callout;
  QString toolTip("Number of times model in callout is used on this page");
  setAttributes(PageNumberType,
                CalloutType,
                _meta,
                _meta->LPub.callout.instance,
                _format,
                _value,
                toolTip,
                _parent);
}

void Callout::addGraphicsPointerItem(
  int      calloutOffsetX,
  int      calloutOffsetY,
  int      csiOffsetX,
  int      csiOffsetY,
  int      csiOffset[2],
  int      csiSize[2],
  Pointer *pointer,
  QGraphicsItem *parent)
{
  QRect    calloutRect(calloutOffsetX,calloutOffsetY,size[XX],size[YY]);
  QRect    csiRect(csiOffsetX + csiOffset[XX] - calloutOffsetX,
                   csiOffsetY + csiOffset[YY] - calloutOffsetY,
                   csiSize[XX], csiSize[YY]);

  CalloutPointerItem *t = 
    new CalloutPointerItem(
          calloutRect,
          csiRect,
         &meta,
          pointer,
          meta.submodelStack.size(),
          parent,
          view);
  graphicsPointerList.append(t);
}

void Callout::updatePointers(QPoint &delta)
{
  for (int i = 0; i < graphicsPointerList.size(); i++) {
    CalloutPointerItem *pointer = graphicsPointerList[i];
    pointer->updatePointer(delta);
  }
}

void Callout::drawTips(QPoint &delta)
{
  for (int i = 0; i < graphicsPointerList.size(); i++) {
    CalloutPointerItem *pointer = graphicsPointerList[i];
    pointer->drawTip(delta);
  }
}

void CalloutInstanceItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
  QString ci = "Callout Instance Count";
  QAction *fontAction   = commonMenus.fontMenu(menu,ci);
  QAction *colorAction  = commonMenus.colorMenu(menu,ci);
  QAction *marginAction = commonMenus.marginMenu(menu,ci);

  QAction *selectedAction   = menu.exec(event->screenPos());

  if (selectedAction == NULL) {
    return;
  }

  if (selectedAction == fontAction) {

    changeFont(callout->topOfCallout(), callout->bottomOfCallout(),font);

  } else if (selectedAction == colorAction) {

    changeColor(callout->topOfCallout(), callout->bottomOfCallout(),color);

  } else if (selectedAction == marginAction) {

    changeMargins("Times Used Margin",
                  callout->topOfCallout(), callout->bottomOfCallout(),
                  margin);
  }
}

void CalloutInstanceItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseReleaseEvent(event);
  QPointF newPosition;
  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable) && positionChanged) {
    // back annotate the movement of the PLI into the LDraw file.
    newPosition = pos() - position;
    PlacementData placementData = placement->value();
    placementData.offsets[0] += newPosition.x()/meta->LPub.page.size.value(0);
    placementData.offsets[1] += newPosition.y()/meta->LPub.page.size.value(1);
    placement->setValue(placementData);
    changePlacementOffset(callout->topOfCallout(),placement);
  }
}
