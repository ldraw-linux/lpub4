
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

#include "placement.h"
#include "ranges.h"
#include "callout.h"
#include "range.h"
#include "step.h"

void PlacementNum::sizeit()
{
  if (number < 1) {
    size[0] = 0;
    size[1] = 0;
  } else {
    QString string = QString("%1x") .arg(number);
    QFont   f;
    f.fromString(font);
    QFontMetrics fm(f);
    QSize fSize = fm.size(0,string);
    size[0] = fSize.width();
    size[1] = fSize.height();
  }
}

void PlacementNum::sizeit(QString format)
{
  if (number < 1) {
    size[0] = 0;
    size[1] = 0;
  } else {
    QString string = QString(format) .arg(number);
    QGraphicsTextItem gti(string);
    QFont   f;
    f.fromString(font);
    gti.setFont(f);
    size[0] = int(gti.document()->size().width());
    size[1] = int(gti.document()->size().height());
  }
}

void Placement::appendRelativeTo(Placement *element)
{
  if (element->relativeType != PageType) {
    for (int i = 0; i < relativeToList.size(); i++) {
      if (relativeToList[i] == element) {
        return;
      }
    }
    if (relativeToList.size() < 100) {
      relativeToList.append(element);
    }
  }
}

/*
 * we start with a page, and ranges, and
 * we walk through the ranges,range,steps,
 * looking for things that are relative to page.
 * We put these in the p_head list.
 *
 * foreach thing relative to page, we make a list
 * of things that are relative to them.
 */
int rc;
int Placement::relativeTo(
  Placement *pe)
{
  rc = 0;
  
  Step *step = dynamic_cast<Step *>(pe);
  if (step) {
    if (step->csiPixmap.placement.value().relativeTo == relativeType) {
      placeRelative(&step->csiPixmap);
      appendRelativeTo(&step->csiPixmap);
    }

    if (step->pli.placement.value().relativeTo == relativeType) {
      placeRelative(&step->pli);
      appendRelativeTo(&step->pli);
    }

    if (step->stepNumber.placement.value().relativeTo == relativeType) {
      placeRelative(&step->stepNumber);
      appendRelativeTo(&step->stepNumber);
    }

    /* callouts */

    for (int i = 0; i < step->list.size(); i++) {
      if (step->list[i]->relativeType == CalloutType) {
        Callout *callout = step->list[i];
        if (callout->placement.value().relativeTo == relativeType) {
          placeRelative(callout);
          appendRelativeTo(callout);
        }
      }
    } // callouts
    // Everything placed    
  } // if step

  /* try to find relation for things relative to us */
    
  int limit = relativeToList.size();
      
  if (limit < 100) {
    for (int i = 0; i < limit; i++) {
      rc = relativeToList[i]->relativeTo(pe);
      if (rc) {
        break;
      }
    }
  } else {
    rc = -1;
  }

  return rc;
}

int Placement::relativeToMs(
  Placement *placement)
{
  if (placement->relativeType == CalloutType ||
      placement->relativeType == StepGroupType) {
    Steps *steps = dynamic_cast<Steps *>(placement);

    if (steps) {
      if (steps->pli.tsize() && 
          steps->pli.placement.value().relativeTo == relativeType) {
        placeRelative(&steps->pli);
        appendRelativeTo(&steps->pli);
      }
      for (int i = 0; i < steps->list.size(); i++) {
        if (steps->list[i]->relativeType == RangeType) {
          Range *range = dynamic_cast<Range *>(steps->list[i]);
          for (int i = 0; i < range->list.size(); i++) {
            if (range->list[i]->relativeType == StepType) {
              Step *step = dynamic_cast<Step *>(range->list[i]);

              /* callouts */

              for (int i = 0; i < step->list.size(); i++) {
                if (step->list[i]->relativeType == CalloutType) {
                  Callout *callout = dynamic_cast<Callout *>(step->list[i]);

                  PlacementData placementData = callout->placement.value();

                  if ((placementData.relativeTo == PageType ||
                       placementData.relativeTo == StepGroupType) &&
                       placementData.relativeTo == relativeType) {
                    placeRelative(callout);
                    placement->appendRelativeTo(callout);
                  }
                } // if callout
              } // callouts
            } // if step
          } // foreach step
        } // if range
      } // foreach range
    } // if ranges

    /* try to find relation for things relative to us */

    for (int i = 0; i < relativeToList.size(); i++) {
      if (relativeToList[i]->relativeToMs(steps)) {
        return -1;
      }
    }

    // Everything placed

    return 0;
  } else {
    return -1;
  }
}

void Placement::placeRelative(
  Placement *them)
{
  int disp[2];
  
  volatile PlacementData pld = them->placement.value();

  disp[XX] = (int) (size[XX] * them->placement.value().offsets[XX]);
  disp[YY] = (int) (size[YY] * them->placement.value().offsets[YY]);

  float lmargin[2];
  lmargin[XX] = qMax(margin.value(XX),them->margin.value(XX));
  lmargin[YY] = qMax(margin.value(YY),them->margin.value(YY));

  PlacementData placementData = them->placement.value();

  them->relativeToWidth = size[XX];
  them->relativeToHeight = size[YY];

  if (placementData.preposition == Outside) {
    switch (placementData.placement) {
      case TopLeft:
      case Left:
      case BottomLeft:
        them->loc[XX] = loc[XX] - them->size[XX] - lmargin[XX];
      break;
      case TopRight:
      case Right:
      case BottomRight:
        them->loc[XX] = loc[XX] + size[XX] + lmargin[XX];
      break;
      case Top:
      case Bottom:
        them->loc[XX] = loc[XX];
        switch (placementData.justification) {
          case Center:
            them->loc[XX] += (size[XX]-them->size[XX])/2;
          break;
          case Right:
            them->loc[XX] += size[XX]-them->size[XX];
          break;
          default:
          break;
        }
      break;
      case Center:
        them->loc[XX] = loc[XX];
      break;
      default:
      break;
    }
    switch (placementData.placement) {
      case TopLeft:
      case Top:
      case TopRight:
        them->loc[YY] = loc[YY] - them->size[YY] - lmargin[YY];
      break;
      case BottomLeft:
      case Bottom:
      case BottomRight:
        them->loc[YY] = loc[YY] + size[YY] + lmargin[YY];
      break;
      case Left:
      case Right:
        them->loc[YY] = loc[YY];
        switch(placementData.justification) {
          case Center:
            them->loc[YY] += (size[YY]-them->size[YY])/2;
          break;
          case Bottom:
            them->loc[YY] += size[YY]-them->size[YY];
          break;
          default:
          break;
        }
      break;
      default:
      break;
    }
  } else {
    switch (placementData.placement) {
      case TopLeft:
      case Left:
      case BottomLeft:
        them->loc[XX] = loc[XX] + lmargin[XX];
      break;
      case Top:
      case Bottom:
        them->loc[XX] = loc[XX] + (size[XX]-them->size[XX])/2;
      break;
      case TopRight:
      case Right:
      case BottomRight:
        them->loc[XX] = loc[XX] + size[XX] - them->size[XX] - lmargin[XX];
      break;
      case Center:
        them->loc[XX] = loc[XX] + (size[XX]-them->size[XX])/2;
      break;
      default:
      break;
    }

    switch (placementData.placement) {
      case TopLeft:
      case Top:
      case TopRight:
        them->loc[YY] = loc[YY] + lmargin[YY];
      break;
      case Left:
      case Right:
        them->loc[YY] = loc[YY] + (size[YY] - them->size[YY])/2;
      break;
      case BottomLeft:
      case Bottom:
      case BottomRight:
        them->loc[YY] = loc[YY] + size[YY] - them->size[YY] - lmargin[YY];
      break;
      case Center:
        them->loc[YY] = loc[YY] + (size[YY] - them->size[YY])/2;
      break;
      default:
      break;
    }
  }
  them->loc[XX] += disp[XX];
  them->loc[YY] += disp[YY];
}

void Placement::placeRelative(
  Placement   *them,
  int          margin[2])
{
  int lmargin[2];
  lmargin[XX] = them->margin.value(XX);
  lmargin[YY] = them->margin.value(YY);
  lmargin[XX] = qMax(margin[XX],lmargin[XX]);
  lmargin[YY] = qMax(margin[YY],lmargin[YY]);

  PlacementData placementData = them->placement.value();

  them->relativeToWidth = size[XX];
  them->relativeToHeight = size[YY];

  if (placementData.preposition == Outside) {
    switch (placementData.placement) {
      case TopLeft:
      case Left:
      case BottomLeft:
        them->loc[XX] = loc[XX] - them->size[XX] - lmargin[XX];
      break;
      case TopRight:
      case Right:
      case BottomRight:
        them->loc[XX] = loc[XX] + size[XX] + lmargin[XX];
      break;
      case Top:
      case Bottom:
        them->loc[XX] = loc[XX];
        switch (placementData.justification) {
          case Center:
            them->loc[XX] += (size[XX]-them->size[XX])/2;
          break;
          case Right:
            them->loc[XX] += size[XX]-them->size[XX];
          break;
          default:
          break;
        }
      break;
      case Center:
        them->loc[XX] = loc[XX];
      break;
      default:
      break;
    }
    switch (placementData.placement) {
      case TopLeft:
      case Top:
      case TopRight:
        them->loc[YY] = loc[YY] - them->size[YY] - lmargin[YY];
      break;
      case BottomLeft:
      case Bottom:
      case BottomRight:
        them->loc[YY] = loc[YY] + size[YY] + lmargin[YY];
      break;
      case Left:
      case Right:
        them->loc[YY] = loc[YY];
        switch(placementData.justification) {
          case Center:
            them->loc[YY] += (size[YY]-them->size[YY])/2;
          break;
          case Bottom:
            them->loc[YY] += size[YY]-them->size[YY];
          break;
          default:
          break;
        }
      break;
      default:
      break;
    }
  } else {
    switch (placementData.placement) {
      case TopLeft:
      case Left:
      case BottomLeft:
        them->loc[XX] = loc[XX] + lmargin[XX];
      break;
      case Top:
      case Bottom:
        them->loc[XX] = loc[XX] + (size[XX]-them->size[XX])/2;
      break;
      case TopRight:
      case Right:
      case BottomRight:
        them->loc[XX] = loc[XX] + size[XX] - them->size[XX] - lmargin[XX];
      break;
      case Center:
        them->loc[XX] = loc[XX] + (size[XX]-them->size[XX])/2;
      break;
      default:
      break;
    }

    switch (placementData.placement) {
      case TopLeft:
      case Top:
      case TopRight:
        them->loc[YY] = loc[YY] + lmargin[YY];
      break;
      case Left:
      case Right:
        them->loc[YY] = loc[YY] + (size[YY] - them->size[YY])/2;
      break;
      case BottomLeft:
      case Bottom:
      case BottomRight:
        them->loc[YY] = loc[YY] + size[YY] - them->size[YY] - lmargin[YY];
      break;
      case Center:
        them->loc[YY] = loc[YY] + (size[YY]-them->size[YY])/2;
      break;
      default:
      break;
    }
  }
  int disp[2];

  disp[XX] = (int) (size[XX] * them->placement.value().offsets[XX]);
  disp[YY] = (int) (size[YY] * them->placement.value().offsets[YY]);
  them->loc[XX] += disp[XX];
  them->loc[YY] += disp[YY];
}

void Placement::justifyX(
  int          origin,
  int          height)
{
  switch (placement.value().placement) {
    case Top:
    case Bottom:
      switch (placement.value().justification) {
        case Left:
          loc[XX] = origin;
        break;
        case Center:
          loc[XX] = origin + (height - size[XX])/2;
        break;
        case Right:
          loc[XX] = origin + height - size[XX];
        break;
        default:
        break;
      }
    break;
    default:
    break;
  }
}

void Placement::justifyY(
  int          origin,
  int          height)
{
  switch (placement.value().placement) {
    case Left:
    case Right:
      switch (placement.value().justification) {
        case Top:
          loc[YY] = origin;
        break;
        case Center:
          loc[YY] = origin + (height - size[YY])/2;
        break;
        case Bottom:
          loc[YY] = origin + height - size[YY];
        break;
        default:
        break;
      }
    break;
    default:
    break;
  }
}

InsertPixmapItem::InsertPixmapItem(
  QPixmap    &pixmap,
  InsertMeta &insertMeta,
  QGraphicsItem *parent)
      
  : QGraphicsPixmapItem(pixmap,parent),
    insertMeta(insertMeta)
{
  InsertData insertData = insertMeta.value();

  size[0] = pixmap.width() *insertData.picScale;
  size[1] = pixmap.height()*insertData.picScale;
  setFlag(QGraphicsItem::ItemIsSelectable,true);
  setFlag(QGraphicsItem::ItemIsMovable,true);
      
  origWidth  = size[0]; // pixmap.width();
  origHeight = size[1]; // pixmap.height();
  
  setTransformationMode(Qt::SmoothTransformation);
  
  grabSize = toPixels(0.03,DPI);
  for (int i = 0; i < 8; i++) {
    grab[i] = NULL;
  }
  oldScale = insertData.picScale;
  oldScale = 1.0;
}

void InsertPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{     
  QGraphicsItem::mousePressEvent(event);
  positionChanged = false;
  position = pos();
  placeGrabs();
} 
  
void InsertPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{ 
  QGraphicsItem::mouseMoveEvent(event);
  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {
    positionChanged = true;
    placeGrabs();
  }   
}

void InsertPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseReleaseEvent(event);

  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {

    QPointF newPosition;

    // back annotate the movement of the PLI into the LDraw file.
    newPosition = pos() - position;
    
    if (newPosition.x() || newPosition.y()) {
      positionChanged = true;

      InsertData insertData = insertMeta.value();
      
      volatile float offset[2] = { newPosition.x()/relativeToWidth, newPosition.y()/relativeToHeight };
      
      insertData.offsets[0] += offset[0];
      insertData.offsets[1] += offset[1];

      insertMeta.setValue(insertData);

      changeInsertOffset(&insertMeta);
    }
  }
}

QVariant InsertPixmapItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
  if (grab[0] && change == ItemSelectedChange) {
    for (int i = 0; i < 4; i++) {
      grab[i]->setVisible(value.toBool());
    }
  }
  return QGraphicsItem::itemChange(change,value);
}

void InsertPixmapItem::placeGrabs()
{
  QRectF rect = sceneBoundingRect();
  int left = rect.left();
  int top  = rect.top();
  int width = rect.width();
  int height = rect.height();

  points[TopLeft]     = QPointF(left,top);
  points[TopRight]    = QPointF(left + width,top);
  points[BottomRight] = QPointF(left + width, top + height);
  points[BottomLeft]  = QPointF(left, top + height);

  if (grab[0] == NULL) {
    for (int i = 0; i < 4; i++) {
      grab[i] = new IpiGrab(this);
      grab[i]->setParentItem(parentItem());
      grab[i]->setFlag(QGraphicsItem::ItemIsSelectable,true);
      grab[i]->setFlag(QGraphicsItem::ItemIsMovable,true);
      grab[i]->setZValue(100);
      QPen pen(Qt::black);
      grab[i]->setPen(pen);
      grab[i]->setBrush(Qt::black);
      grab[i]->setRect(0,0,grabSize,grabSize);
    }
  }

  for (int i = 0; i < 4; i++) {
    grab[i]->setPos(points[i].x()-grabSize/2,points[i].y()-grabSize/2);
  }
}

void InsertPixmapItem::whatPoint(IpiGrab *grabbed)
{
  for (int i = 0; i < 4; i++) {
    if (grabbed == grab[i]) {
      selectedPoint = SelectedPoint(i);
      break;
    }
  }
  positionChanged = true;
  position = pos();
}

void InsertPixmapItem::resize(QPointF grabbed)
{
  // recalculate corners Y
  switch (selectedPoint) {
    case TopLeft:
    case TopRight:
      points[TopLeft].setY(grabbed.y());
      points[TopRight].setY(grabbed.y());
    break;
    case BottomLeft:
    case BottomRight:
      points[BottomLeft].setY(grabbed.y());
      points[BottomRight].setY(grabbed.y());
    break;
    default:
    break;
  }
  // relaculate corners X
  switch (selectedPoint) {
    case TopLeft:
    case BottomLeft:
      points[TopLeft].setX(grabbed.x());
      points[BottomLeft].setX(grabbed.x());
    break;
    case TopRight:
    case BottomRight:
      points[TopRight].setX(grabbed.x());
      points[BottomRight].setX(grabbed.x());
    break;
    default:
    break;
  }
  qreal  rawWidth, rawHeight;

  // calculate box raw size
  switch (selectedPoint) {
    case TopLeft:
      rawWidth = points[BottomRight].x() - grabbed.x();
      rawHeight = points[BottomRight].y() - grabbed.y();
    break;
    case TopRight:
      rawWidth = grabbed.x()-points[BottomLeft].x();
      rawHeight = points[BottomLeft].y() - grabbed.y();
    break;
    case BottomRight:
      rawWidth = grabbed.x() - points[TopLeft].x();
      rawHeight = grabbed.y() - points[TopLeft].y();
    break;
    default:
      rawWidth = points[TopRight].x() - grabbed.x();
      rawHeight = grabbed.y() - points[TopRight].y();
    break;
  }
  
  if (rawWidth > 0 && rawHeight > 0) {

    // Force aspect ratio to match original aspect ratio of picture
    // ratio = width/height
    // width = height * ratio
    qreal  pixmapSizeX = pixmap().size().width();
    qreal  pixmapSizeY = pixmap().size().height();
    
    qreal width = rawHeight * pixmapSizeX / pixmapSizeY;
    qreal height = rawWidth * pixmapSizeY / pixmapSizeX;
    
    if (width * rawHeight < rawWidth * height) {
      height = rawHeight;
    } else {
      width = rawWidth;
    }
    
    // Place the scaled box

    switch (selectedPoint) {
      case TopLeft:
        setPos(points[BottomRight].x()-width,points[BottomRight].y()-height);
      break;
      case TopRight:
        setPos(points[BottomLeft].x(),points[BottomLeft].y()-height);
      break;
      case BottomRight:
        setPos(points[TopLeft]);
      break;
      default:
        setPos(points[TopRight].x()-width,points[TopRight].y());
      break;
    }
    
    // Calculate corners
    
    points[TopLeft] = pos();
    points[TopRight] = QPointF(pos().x() + width,pos().y());
    points[BottomRight] = pos() + QPointF(width,height);
    points[BottomLeft] = QPointF(pos().x(),pos().y()+height);
  
    for (int i = 0; i < 4; i++) {
      grab[i]->setPos(points[i].x()-grabSize/2,points[i].y()-grabSize/2);
    }
    
    // Unscale from last time
    
    volatile qreal newScale = width/origWidth;
    scale(1.0/oldScale,1.0/oldScale);
    
    // Scale it to the new scale
    
    scale(newScale,newScale);
    oldScale = newScale;
  }
}

void InsertPixmapItem::changePicScale()
{
  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {
    if (positionChanged) {

      beginMacro(QString("DragIpi"));
      
      InsertData insertData = insertMeta.value();
      if (insertData.placement == Center) {

        QPointF delta(pos() - position);
        volatile qreal deltax = delta.x();
        volatile qreal deltay = delta.y();
        volatile qreal deltaX = (sceneBoundingRect().width() - origWidth)/2;
        volatile qreal deltaY = (sceneBoundingRect().height() - origHeight)/2;
      
        delta.setX(deltax + deltaX);
        delta.setY(deltay + deltaY);
      
        // Back annotate to LDraw file
        insertData.offsets[0] += delta.x()/relativeToWidth;
        insertData.offsets[1] += delta.y()/relativeToHeight;
      }
      insertData.picScale *= oldScale;
      insertMeta.setValue(insertData);
      
      changeInsertOffset(&insertMeta);
      
      endMacro();
    }
  }
}

void IpiGrab::mousePressEvent(QGraphicsSceneMouseEvent * /* event */)
{
  ipiItem->whatPoint(this);
}

void IpiGrab::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  ipiItem->resize(event->scenePos());
}

void IpiGrab::mouseReleaseEvent(QGraphicsSceneMouseEvent * /* event */)
{
  ipiItem->changePicScale();
}


#if 0

void PlacementPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{     
  QGraphicsItem::mousePressEvent(event);
  positionChanged = false;
  position = pos();
} 
  
void PlacementPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{ 
  QGraphicsItem::mouseMoveEvent(event);
  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {
    positionChanged = true;
  }   
}

void PlacementPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseReleaseEvent(event);

  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {

    QPointF newPosition;

    // back annotate the movement of the PLI into the LDraw file.
    newPosition = pos() - position;
    
    if (newPosition.x() || newPosition.y()) {
      positionChanged = true;

      PlacementData placementData = placement.value();
      
      float offset[2] = { newPosition.x()/relativeToWidth, newPosition.y()/relativeToHeight };
      
      placementData.offsets[0] += offset[0];
      placementData.offsets[1] += offset[1];

      placement.setValue(placementData);

      changePlacementOffset(&placement,true,false);
    }
  }
}

#endif






