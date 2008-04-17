
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
 * This class implements the graphical pointers that extend from callouts to
 * assembly images as visual indicators to the builder as to where to 
 * add the completed submodel into partially assembeled final model.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include <QtGui>

#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsLineItem>
#include <QPolygonF>
#include <QGraphicsSceneContextMenuEvent>
#include <math.h>
#include "pointeritem.h"
#include "metaitem.h"
#include "lpub.h"

//---------------------------------------------------------------------------

/* calculate the parameters for the equation of line from two points */

bool rectLineIntersect(
  QPoint  tip,
  QPoint  loc,
  QRect   rect, 
  int     base,
  QPoint &intersect,

  PlacementEnc &placement);

/*
 * This is the constructor of a graphical pointer
 */

CalloutPointerItem::CalloutPointerItem(
  QRect         &callout,
  QRect         &csi,
  Meta          *meta,
  Pointer       *_pointer,
  int            _submodelLevel,
  QGraphicsItem *parent,
  QGraphicsView *_view)

  : QGraphicsItemGroup(parent)
{
  view          = _view;

  calloutRect = callout;
  csiRect       = csi;
  pointer       = *_pointer;
  submodelLevel = _submodelLevel;

  selectedPoint = None;
  QString color = meta->LPub.callout.subModelColor.value(submodelLevel);
  PointerData pointerData = pointer.pointerMeta.value();
  BorderData  border = meta->LPub.callout.border.value();
  BackgroundData backgroundData = meta->LPub.callout.background.value();

  backgroundColor = backgroundData.string;
  borderColor     = border.color;
  borderThickness = border.thickness;
  if (borderThickness) {
    grabSize = 3*borderThickness;
  } else {
    grabSize = toPixels(0.03,DPI);
  }

  float width  = callout.width();
  float height = callout.height();
  float loc = pointerData.loc;

  placement = pointerData.placement;
  base      = pointerData.base;

  points[Loc].setX(0);
  points[Loc].setY(0);

  switch (placement) {
    case TopLeft:
    break;
    case Top:
      loc *= width;
      points[Loc].setX(loc);
    break;
    case TopRight:
      points[Loc].setX(width);
    break;
    case Right:
      loc *= height;
      points[Loc].setX(width);
      points[Loc].setY(loc);
    break;
    case BottomRight:
      points[Loc].setX(width);
      points[Loc].setY(height);
    break;
    case Bottom:
      loc *= width;
      points[Loc].setX(loc);
      points[Loc].setY(height);
    break;
    case BottomLeft:
      points[Loc].setY(height);
    break;
    default:
      loc *= height;
      points[Loc].setY(loc);
    break;
  }
  adjustC1C2();

  points[Tip].setX(pointerData.x*csi.width() + csi.left());
  points[Tip].setY(pointerData.y*csi.height() + csi.top());

  for (int i = 0; i < 4; i++) {
    grab[i] = new QGraphicsRectItem(this);
    grab[i]->setPen(Qt::NoPen);
    grab[i]->setFlag(QGraphicsItem::ItemIsSelectable,true);
    grab[i]->setZValue(100);
    addToGroup(grab[i]);
  }
  grab[Tip]->setToolTip("Click and drag me to move the tip of my pointer");
  grab[Loc]->setToolTip("Click and drag me to move me around");
  grab[C1]->setToolTip("Drag me around to make my pointer bigger or smaller");
  grab[C2]->setToolTip("Drag me around to make my pointer bigger or smaller");

  /*
   * Group triangle, triangle, line, line 
   */

  sizeGrab();

  QPolygonF pointerPoly;

  QColor qColor = LDrawColor::color(color);

  poly1 = new QGraphicsPolygonItem(pointerPoly, this);
  poly1->setPen(qColor);
  poly1->setBrush(qColor);
  poly1->setFlag(QGraphicsItem::ItemIsSelectable,false);
  poly1->setToolTip("Pointer");
  addToGroup(poly1);

  poly2 = new QGraphicsPolygonItem(pointerPoly, this);
  poly2->setPen(qColor);
  poly2->setBrush(qColor);
  poly2->setFlag(QGraphicsItem::ItemIsSelectable,false);
  poly2->setToolTip("Pointer");
  addToGroup(poly2);

  QLineF linef;

  qColor = LDrawColor::color(border.color);

  QPen pen(qColor);
  pen.setWidth(borderThickness);
  pen.setCapStyle(Qt::RoundCap);

  line1 = new QGraphicsLineItem(linef,this);
  line1->setPen(pen);
  line1->setZValue(99);
  line1->setFlag(QGraphicsItem::ItemIsSelectable,false);
  line1->setToolTip("Pointer");
  addToGroup(line1);

  line2 = new QGraphicsLineItem(linef,this);
  line2->setPen(pen);
  line2->setZValue(99);
  line2->setFlag(QGraphicsItem::ItemIsSelectable,false);
  line2->setToolTip("Pointer");
  addToGroup(line2);

  drawPointerPoly();

  setZValue(99);
  setFlag(QGraphicsItem::ItemIsFocusable,true);
}

/*
 * Given the location of the Tip (as dragged around by the user)
 * calculate a reasonable placement and Loc for points[Loc]
 */

bool CalloutPointerItem::autoLocTip()
{
  int width  = calloutRect.width();
  int height = calloutRect.height();
  QRect rect(borderThickness+base,
             borderThickness+base,
             width-2*borderThickness-2*base,
             height-2*borderThickness-2*base);
  QPoint intersect;

  int tx,ty,t;

  tx = points[Tip].x();
  ty = points[Tip].y();

  t = base + borderThickness;

  /* if tip is above or below the callout, keep Loc on the side
   * callout */

  if (tx >= rect.left() && tx <= rect.right()) {
    if (ty < 0) {
      intersect = QPoint(tx,0);
      placement = Top;
    } else {
      intersect = QPoint(tx,height);
      placement = Bottom;
    }
  } else if (tx >= 0 && tx < borderThickness + base) {
    if (ty < 0) {
      intersect = QPoint(borderThickness + base,0);
      placement = Top;
    } else {
      intersect = QPoint(borderThickness + base,height);
    }
  } else if (tx >= width - (borderThickness + base) && tx < width) {
    if (ty < 0) {
      intersect = QPoint(width - (borderThickness + base),0);
      placement = Top;
    } else {
      intersect = QPoint(width - (borderThickness + base),height);
      placement = Bottom;
    }

    /* if tip is left or right of callout, keep loc on the
     * side */

  } else if (ty >= rect.top() && ty <= rect.bottom()) {
    if (tx < 0) {
      intersect = QPoint(0,ty);
      placement = Left;
    } else {
      intersect = QPoint(width,ty);
      placement = Right;
    }
  } else if (ty >= 0 && ty < borderThickness + base) {
    if (tx < 0) {
      intersect = QPoint(0,borderThickness + base);
      placement = Left;
    } else {
      intersect = QPoint(width,borderThickness + base);
      placement = Right;
    }
  } else if (ty >= height - (borderThickness + base) && ty < height) {
    if (tx < 0) {
      intersect = QPoint(0,height - borderThickness + base);
      placement = Left;
    } else {
      intersect = QPoint(width,height - borderThickness + base);
      placement = Right;
    }

    /* Figure out which corner */

  } else if (tx < 0) {
    if (ty < 0) {
      intersect = QPoint(0,0);
      placement = TopLeft;
    } else {
      intersect = QPoint(0,height);
      placement = BottomLeft;
    }

  } else {
    if (ty < 0) {
      intersect = QPoint(width,0);
      placement = TopRight;
    } else {
      intersect = QPoint(width,height);
      placement = BottomRight;
    }
  }

  points[Loc] = intersect;
  adjustC1C2();

  return true;
}

/*
 * Given the location of Loc (as dragged around by the user)
 * calculate a reasonable placement and Loc for points[Loc]
 */

bool CalloutPointerItem::autoLocLoc(
  QPoint loc)
{
  QRect rect(0,
             0,
             calloutRect.width(),
             calloutRect.height());

  QPoint intersect;
  PlacementEnc tplacement;

  QPoint tip(points[Tip].x() + 0.5, points[Tip].y() + 0.5);

  if (rectLineIntersect(tip,
                        loc,
                        rect,
                        base+borderThickness,
                        intersect,
                        tplacement)) {
    placement = tplacement;
    points[Loc] = intersect;
    adjustC1C2();
    return true;
  }    
  return false;
}
/*
 * Determine if the mouse pointer is selecting one of the four points in the
 * pointer polygon.
 */

void CalloutPointerItem::mousePressEvent  (QGraphicsSceneMouseEvent *event)
{
  positionChanged = false;
  setFlag(QGraphicsItem::ItemIsMovable,true);
  QGraphicsItem::mousePressEvent(event);

  //setFlag(QGraphicsItem::ItemIsMovable,false);
  // determine closest corner (c1, top, c2, loc
  // c1 - affects base (and therefore c2)
  // c2 - affects base (and therefore c1)
  // loc - affects loc
  // tip - affects x,y
  float  c1Dist, c2Dist, locDist, tipDist, mDist, mX, mY, dX, dY;

  sizeGrab();

  QPointF mp;
  
  mp = view->mapFromScene(event->scenePos());
  
  mX = mp.x();
  mY = mp.y();

  mp = mapToScene(points[C1]);
  mp = view->mapFromScene(mp);
  dX = mX - mp.x(); dY = mY - mp.y();  c1Dist = dX*dX+dY*dY;
  mp = mapToScene(points[C2]);
  mp = view->mapFromScene(mp);
  dX = mX - mp.x(); dY = mY - mp.y();  c2Dist = dX*dX+dY*dY;
  mp = mapToScene(points[Tip]);
  mp = view->mapFromScene(mp);
  dX = mX - mp.x(); dY = mY - mp.y(); tipDist = dX*dX+dY*dY;
  mp = mapToScene(points[Loc]);
  mp = view->mapFromScene(mp);
  dX = mX - mp.x(); dY = mY - mp.y(); locDist = dX*dX+dY*dY;

  mDist = c1Dist;
  selectedPoint = C1;

  if (c2Dist < mDist) {
    mDist = c2Dist;
    selectedPoint = C2;
  }
  if (locDist < mDist) {
    mDist = locDist;
    selectedPoint = Loc;
  }
  if (tipDist < mDist) {
    mDist = tipDist;
    selectedPoint = Tip;
  }
  if (mDist > grabSize*grabSize) {
    selectedPoint = None;
    QGraphicsItem::mousePressEvent(event);
  }
}
      /*
       *     c1 c2  c1 c2
       *    +--+------+--+
       *  c1|            | c1
       *    +            +
       *  c2|            | c2
       *    |            |
       *    |            |
       *  c1|            | c1
       *    +            +
       *  c2|            | c2
       *    +--+------+--+
       *     c1 c2  c1 c2
       *
       */

      /*     c1        c1
       *    +------------+
       *  c2|            | c2
       *    |            |
       *    |            |   
       *    |            |
       *    |            |
       *    |            |   
       *    |            |
       *  c2|            | c2
       *    +------------+
       *     c1        c1
       */

/*
 * Mouse move.  What we do depends on what was selected by the mouse click.
 *
 * If pointer's tip is selected then allow the tip to move anywhere within the
 * CSI.
 *
 * If the pointer's location (where the pointer is attached to the callout)
 * is selected then allow the pointer to move to any valid edges or corner
 * (constrained by base).
 *
 * If the two outside corners are selected, then the user is trying to adjust the
 * base.
 */

void CalloutPointerItem::mouseMoveEvent   (QGraphicsSceneMouseEvent *event)
{
  if (flags() & QGraphicsItem::ItemIsMovable) {
    QPoint scenePos(event->pos().x(),event->pos().y());
    QPoint intersect;

    bool changed = false;

    QRect rect(0,0,calloutRect.width(),calloutRect.height());

    int c, m;

    switch (selectedPoint) {
      case C1:
      case C2:
        /*
         * C1 and C2 can affect BASE, but we must prevent C1 and C2 from exceeding
         * the dimensions of the side of the callout
         */
        if (selectedPoint == C1) {
          switch (placement) {
            case TopLeft:
            case BottomLeft:
              c = scenePos.x();
              m = rect.width();
            break;
            case Top:
            case Bottom:
            case TopRight:
            case BottomRight:
              c = points[Loc].x() - scenePos.x();
              m = rect.width();
            break;
            case Left:
            case Right:
              c = points[Loc].y()-scenePos.y();
              m = rect.height();
            break;
            default:
            break;
          }
          /*
           * +C1|C1:C2|C1+
           * C2         C2
           * __         __
           * C1         C1   
           * C2         C2
           * __         __
           * C2         C2
           * +C1|C1:C2|C1+
           *
           * C1 is Always X oriented unless Left or Right
           * C2 is always Y oriented unless Top or Bottom
           */
        } else {
          switch (placement) {
            case TopLeft:
            case TopRight:
              c = scenePos.y();
              m = rect.height();
            break;
            case Left:
            case Right:
              c = scenePos.y() - points[Loc].y();
              m = rect.height();
            break;
            case BottomLeft:
            case BottomRight:
              c = points[Loc].y() - scenePos.y();
              m = rect.height();
            break;
            case Top:
            case Bottom:
              c = points[Loc].x()-scenePos.x();
              m = rect.width();
            break;
            default:
            break;
          }
        } 
        if (c >= borderThickness && c <= m - borderThickness) {
          base = c;
          adjustC1C2();
          changed = true;
        }
      break;
      /*
       * Loc must track along an edge.  If you draw a line from tip to the mouse
       * cursor, the line intersects with the edge of the callout one or two places.
       * The single intersection is at corners, and the dual intersection is not
       * on corners.  In the case of dual intersection, the one that is closest
       * to tip is the one used.
       * 
       * If the tip to mouse line does not intersect with the callout, do nothing.
       * If the top to mouse line only intersects with corner, then force corner
       * placement.
       *
       * If the the tip to mouse line intersects twice, then the closest intersect is
       * used.  if the distance between the intersect and the corner is less than
       * base, react as thought we were at corner.
       */

      case Loc:
        if (autoLocLoc(scenePos)) {
          changed = true;
        }
      break;
      case Tip:
        points[Tip] = scenePos;
        if (autoLocTip()) {
          changed = true;
        }
      break;
      default:
      break;
    }
    if (changed) {
      drawPointerPoly();
      positionChanged = true;
    }
  }
}

void CalloutPointerItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseReleaseEvent(event);

  if ((flags() & QGraphicsItem::ItemIsMovable) && positionChanged) {

    mouseMoveEvent(event);
    calculatePointerMeta();

    // back annotate pointer shape back into the LDraw file.

    MetaItem::updatePointer(pointer.here, &pointer.pointerMeta);
  }
}

void CalloutPointerItem::focusInEvent(QFocusEvent *)
{
  QPen pen(Qt::black);
  QBrush black(Qt::black);
  QBrush gray(Qt::gray);

  sizeGrab();

  for (int i = C1; i < 4; i++) {
    grab[i]->setPen(pen);
    if (i == Loc || i == Tip) {
      grab[i]->setBrush(black);
    } else {
      grab[i]->setBrush(gray);
    }
  }
}

void CalloutPointerItem::focusOutEvent(QFocusEvent *)
{
  for (int i = 0; i < None; i++) {
    grab[i]->setPen(Qt::NoPen);
    grab[i]->setBrush(Qt::NoBrush);
  }
  selectedPoint = None;
  setFlag(QGraphicsItem::ItemIsMovable,false);
}

void CalloutPointerItem::adjustC1C2()
{

/*
  +C1|C1:C2|C1+
  C2         C2
  __         __
  C1         C1   
  C2         C2
  __         __
  C2         C2
  +C1|C1:C2|C1+
 */

  switch (placement) {
    case Top:
    case Bottom:
      points[C1].setX(points[Loc].x()-base);
      points[C1].setY(points[Loc].y());
      points[C2].setX(points[Loc].x()+base);
      points[C2].setY(points[Loc].y());
    break;
    case Left:
    case Right:
      points[C1].setX(points[Loc].x());
      points[C1].setY(points[Loc].y()-base);
      points[C2].setX(points[Loc].x());
      points[C2].setY(points[Loc].y()+base);
    break;
    case TopLeft:
      points[C1].setX(points[Loc].x()+base);
      points[C1].setY(points[Loc].y());
      points[C2].setX(points[Loc].x());
      points[C2].setY(points[Loc].y()+base);
    break;
    case TopRight:
      points[C1].setX(points[Loc].x()-base);
      points[C1].setY(points[Loc].y());
      points[C2].setX(points[Loc].x());
      points[C2].setY(points[Loc].y()+base);
    break;
    case BottomLeft:
      points[C1].setX(points[Loc].x()+base);
      points[C1].setY(points[Loc].y());
      points[C2].setX(points[Loc].x());
      points[C2].setY(points[Loc].y()-base);
    break;
    case BottomRight:
      points[C1].setX(points[Loc].x()-base);
      points[C1].setY(points[Loc].y());
      points[C2].setX(points[Loc].x());
      points[C2].setY(points[Loc].y()-base);
    break;
    default:
    break;
  }
}

QPoint locQPoint(
  QPointF      p,
  PlacementEnc placement,
  float        thickness)
{
  switch (placement) {
    case TopLeft: // top
      return QPoint(p.x()+thickness, p.y()+thickness);
    case Top:
      return QPoint(p.x(),           p.y()+thickness);
    case TopRight:
      return QPoint(p.x()-thickness, p.y()+thickness);
    break;
    case Right:
      return QPoint(p.x()-thickness, p.y());
    break;
    case BottomRight: // bottom
      return QPoint(p.x()-thickness, p.y()-thickness);
    case Bottom:
      return QPoint(p.x(),           p.y()-thickness);
    case BottomLeft:
      return QPoint(p.x()+thickness, p.y()-thickness);
    break;
    default: // left
      return QPoint(p.x()+thickness, p.y());
    break;
  }
}

/*
  +C1|C1:C2|C1+
  C2         C2
  __         __
  C1         C1   
  C2         C2
  __         __
  C2         C2
  +C1|C1:C2|C1+
 */

QPoint c1QPoint(
  QPointF      p,
  PlacementEnc placement,
  float        thickness)
{
  switch (placement) {
    case TopLeft: // top
    case Top:
    case TopRight:
      return QPoint(p.x(),           p.y()+thickness);
    break;
    case Right:
      return QPoint(p.x()-thickness, p.y());
    break;
    case BottomRight: // bottom
    case Bottom:
    case BottomLeft:
      return QPoint(p.x(),           p.y()-thickness);
    break;
    default: // left
      return QPoint(p.x()+thickness, p.y());
    break;
  }
}

/*
  +C1|C1:C2|C1+
  C2         C2
  __         __
  C1         C1   
  C2         C2
  __         __
  C2         C2
  +C1|C1:C2|C1+
 */

QPoint c2QPoint(
  QPointF      p,
  PlacementEnc placement,
  float        thickness)
{
  switch (placement) {
    case BottomLeft:
    case TopLeft:
    case Left:
      return QPoint(p.x()+thickness, p.y());
    break;
    case Top:
      return QPoint(p.x(),           p.y()+thickness);
    break;
    case TopRight:
    case Right:
    case BottomRight:
      return QPoint(p.x()-thickness, p.y());
    break;
    default: // bottom
      return QPoint(p.x(),           p.y()-thickness);
    break;
  }
}

void CalloutPointerItem::drawPointerPoly()
{
  sizeGrab();
  QPolygonF pointerPoly;

  QPoint c1  = c1QPoint(points[C1],placement,borderThickness);
  QPoint c2  = c2QPoint(points[C2],placement,borderThickness);
  QPoint loc = QPoint((c1.x()+c2.x())/2,(c1.y()+c2.y())/2);

  pointerPoly << loc;
  pointerPoly << c1;
  pointerPoly << points[Tip];
  pointerPoly << loc;

  removeFromGroup(poly1);
  poly1->setPolygon(pointerPoly);
  addToGroup(poly1);

  pointerPoly[1] = c2;

  removeFromGroup(poly2);
  poly2->setPolygon(pointerPoly);
  addToGroup(poly2);

  c1 = c1QPoint(points[C1],placement,borderThickness/2);

  QLineF linef = QLineF(c1,points[Tip]);

  removeFromGroup(line1);
  line1->setLine(linef);
  addToGroup(line1);

  c2 = c2QPoint(points[C2],placement,borderThickness/2);

  linef = QLineF(c2,points[Tip]);

  removeFromGroup(line2);
  line2->setLine(linef);
  addToGroup(line2);

  QRectF rect = sceneBoundingRect();

  view->updateSceneRect(sceneBoundingRect());
}

void CalloutPointerItem::sizeGrab()
{
  // This little dittie gets us the four grab boxes for the pointer
  // We take 0,0 and 5,5, from view and map it into scene, so the
  // grap points are always the same size

  removeFromGroup(grab[C1]);
  QPoint c1 = c1QPoint(points[C1],placement,borderThickness/2);
  grab[C1]->setRect(c1.x()-grabSize/2,c1.y()-grabSize/2,grabSize,grabSize);
  addToGroup(grab[C1]);

  removeFromGroup(grab[C2]);
  QPoint c2 = c2QPoint(points[C2],placement,borderThickness/2);
  grab[C2]->setRect(c2.x()-grabSize/2,c2.y()-grabSize/2,grabSize,grabSize);
  addToGroup(grab[C2]);

  removeFromGroup(grab[Loc]);
  QPoint loc = locQPoint(points[Loc],placement,borderThickness/2);
  grab[Loc]->setRect(loc.x()-grabSize/2,loc.y()-grabSize/2,grabSize,grabSize);
  addToGroup(grab[Loc]);

  removeFromGroup(grab[Tip]);
  grab[Tip]->setRect(points[Tip].x()-grabSize/2,points[Tip].y()-grabSize/2,
                     grabSize,grabSize);
  addToGroup(grab[Tip]);
}

/* Meta related stuff */

void CalloutPointerItem::drawTip(QPoint delta)
{
  points[Tip] += delta;
  autoLocTip();
  drawPointerPoly();
  points[Tip] -= delta;
}

void CalloutPointerItem::defaultPointer()
{
  int tx = csiRect.left() + csiRect.width()/2;
  int ty = csiRect.top()  + csiRect.height()/2;
  QPoint t(tx,ty);

  points[Tip] = t;
  autoLocTip();
  drawPointerPoly();
  calculatePointerMeta();
  addPointerMeta();
}
 
void CalloutPointerItem::calculatePointerMetaLoc()
{
  float loc;

  switch (placement) {
    case TopLeft:
    case TopRight:
    case BottomLeft:
    case BottomRight:
      loc = 0;
    break;
    case Top:
    case Bottom:
      loc = ((float) points[Loc].x())/calloutRect.width();
    break;
    case Left:
    case Right:
      loc = ((float) points[Loc].y())/calloutRect.height();
    break;
    default:
    break;
  }

  PointerData pointerData = pointer.pointerMeta.value();
  pointer.pointerMeta.setValue(
    placement,
    loc,
    base,
    pointerData.x,
    pointerData.y);
}

void CalloutPointerItem::calculatePointerMeta()
{
  calculatePointerMetaLoc();

  PointerData pointerData = pointer.pointerMeta.value();
  pointerData.x = (float(points[Tip].x()) - csiRect.left())/csiRect.width();
  pointerData.y = (float(points[Tip].y()) - csiRect.top())/csiRect.height();
  pointer.pointerMeta.setValue(
    placement,
    pointerData.loc,
    base,
    pointerData.x,
    pointerData.y);
}

void CalloutPointerItem::updatePointer(
  QPoint &delta)
{
  points[Tip] += delta;
  autoLocTip();
  drawPointerPoly();
  calculatePointerMetaLoc();

  MetaItem::updatePointer(pointer.here, &pointer.pointerMeta);
}

void CalloutPointerItem::addPointerMeta()
{
  QString metaString = pointer.pointerMeta.preamble + pointer.pointerMeta.format(false);
  Where here = pointer.here+1;
  insertMeta(here,metaString);
  gui->displayPage();
}

void CalloutPointerItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;

  QAction *removeAction = menu.addAction("Delete this Pointer");
  removeAction->setWhatsThis(            "Delete this Pointer:\n"
                                         "Deletes this pointer from the callout");

  QAction *selectedAction   = menu.exec(event->screenPos());
  if (selectedAction == removeAction) {
    deletePointer(pointer.here);
  }
}


/* calculate the parameters for the equation of line from two points */

bool lineEquation(
  QPoint point[2],
  qreal &a,
  qreal &b,
  qreal &c)
{
  qreal xlk = point[0].x() - point[1].x();
  qreal ylk = point[0].y() - point[1].y();
  qreal rsq = xlk*xlk+ylk*ylk;
  if (rsq < 1) {
    return false;
  }
  qreal rinv = 1.0/sqrt(rsq);
  a = -ylk*rinv;
  b =  xlk*rinv;
  c = (point[1].x()*point[0].y() - point[0].x()*point[1].y())*rinv;
  return true;
}

/* calculate the intersection of two lines */

bool lineIntersect(
  qreal a1, qreal b1, qreal c1,
  qreal a2, qreal b2, qreal c2,
  QPoint &intersect)
{
  qreal det = a1*b2 - a2*b1;
  if (fabs(det) < 1e-6) {
    return false;
  } else {
    qreal dinv = 1.0/det;
    intersect.setX((b1*c2 - b2*c1)*dinv);
    intersect.setY((a2*c1 - a1*c2)*dinv);
    return true;
  }
}

/* determine if a line intersects a Horizontal line segment, if so return the intersection */

bool lineIntersectHorizSeg(
  qreal a1, qreal b1, qreal c1,
  QPoint seg[2], QPoint &intersect)
{
  qreal a2, b2, c2;
  if ( ! lineEquation(seg,a2,b2,c2)) {
    return false;
  }

  if ( ! lineIntersect(a1,b1,c1,a2,b2,c2,intersect)) {
    return false;
  }

  return intersect.x() >= seg[0].x() && intersect.x() <= seg[1].x();
}

/* determine if a line intersects a Vertical line segment, if so return the intersection */

bool lineIntersectVertSeg(
  qreal a1, qreal b1, qreal c1,
  QPoint seg[2], QPoint &intersect)
{
  qreal a2, b2, c2;
  if ( ! lineEquation(seg,a2,b2,c2)) {
    return false;
  }

  if ( ! lineIntersect(a1,b1,c1,a2,b2,c2,intersect)) {
    return false;
  }

  return intersect.y() >= seg[0].y() && intersect.y() <= seg[1].y();
}
/*
 * Given Tip and Loc that form a line and an rectangle in 2D space, calculate the
 * intersection of the line and rect that is closes to tip.
 *
 * iv_top && iv_left
 * iv_top && iv_bottom
 * iv_top && iv_right
 * iv_left && iv_bottom
 * iv_left && iv_right
 * iv_bottom && iv_right
 *
 * The line will always intersect two sides, unless it just hits the corners.
 *
 */

bool rectLineIntersect(
  QPoint        tip,
  QPoint        loc,
  QRect         rect, 
  int           base,
  QPoint       &intersect,
  PlacementEnc &placement)
{
  int width = rect.width();
  int height = rect.height();
  int tipX = tip.x();
  int tipY = tip.y();
  int tx,ty;

  QPoint seg[2];
  QPoint intersect_top,intersect_bottom,intersect_left,intersect_right;
  bool   iv_top, iv_bottom,iv_left,iv_right;

  /* calculate formula for line between tip and {C1,C2,Loc) */

  seg[0] = tip;
  seg[1] = loc;
  qreal a, b, c;
  if ( ! lineEquation(seg,a,b,c)) {
    return false;
  }

  /* top Horizontal line */

  seg[0].setX(0);
  seg[1].setX(rect.width());
  seg[0].setY(rect.top());
  seg[1].setY(rect.top());
  iv_top = lineIntersectHorizSeg(a,b,c,seg,intersect_top);

  tx = tipX - intersect_top.x();
  ty = tipY - intersect_top.y();
  int top_dist = tx*tx+ty*ty;

  /* bottom Horizontal line */

  seg[0].setY(rect.height());
  seg[1].setY(rect.height());

  iv_bottom = lineIntersectHorizSeg(a,b,c,seg,intersect_bottom);

  tx = tipX - intersect_bottom.x();
  ty = tipY - intersect_bottom.y();
  int bot_dist = tx*tx+ty*ty;

  /* left Vertical line */

  seg[0].setX(0);
  seg[1].setX(0);
  seg[0].setY(rect.top());
  seg[1].setY(rect.height());

  iv_left = lineIntersectVertSeg(a,b,c,seg,intersect_left);

  tx = tipX - intersect_left.x();
  ty = tipY - intersect_left.y();
  int left_dist = tx*tx+ty*ty;

  /* right Vertical line */

  seg[0].setX(rect.width());
  seg[1].setX(rect.width());

  iv_right = lineIntersectVertSeg(a,b,c,seg,intersect_right);

  tx = tipX - intersect_right.x();
  ty = tipY - intersect_right.y();
  int right_dist = tx*tx+ty*ty;

  if (iv_top && iv_bottom) {

    /* Is mouse tip closer to top or bottom? */

    if (top_dist < bot_dist) {

      /* tip closer to top */

      if (intersect_top.x() < base) {
        placement = TopLeft;
        intersect.setX(0);
      } else if (width - intersect_top.x() < base) {
        placement = TopRight;
        intersect.setX(width);
      } else {
        placement = Top;
        int x = intersect_top.x();
        x = x < base
              ? base
              : x > width - base
                  ? width - base
                  : x;
        intersect.setX(x);
      }
      intersect.setY(0);
    } else {
      if (intersect_bottom.x() < base) {
        placement = BottomLeft;
        intersect.setX(0);
      } else if (width - intersect_bottom.x() < base) {
        placement = BottomRight;
        intersect.setX(width);
      } else {
        placement = Bottom;
        int x = intersect_top.x();
        x = x < base
              ? base
              : x > width - base
                  ? width - base
                  : x;
        intersect.setY(intersect_bottom.y());
        intersect.setX(x);
      }
      intersect.setY(height);
    }
  } else if (iv_left && iv_right) {

    /* Is the tip closer to right or left? */

    if (left_dist < right_dist) {

      /* closer to left */

      if (intersect_left.y() < base) {
        placement = TopLeft;
        intersect.setY(0);
      } else if (height - intersect_left.y() < base) {
        placement = BottomLeft;
        intersect.setY(height);
      } else {
        placement = Left;
        int y = intersect_left.y();
        y = y < base
              ? base
              : y > height - base
                ? height - base
                : y;
        intersect.setY(intersect_left.y());
      }
      intersect.setX(0);
    } else {
      if (intersect_right.y() < base && tipY < 0) {
        placement = TopRight;
        intersect.setY(0);
      } else if (height - intersect_right.y() < base && tipY > height) {
        placement = BottomRight;
        intersect.setY(height);
      } else {
        placement = Right;
        int y = intersect_right.y();
        y = y < base
                ? base
                : y > height - base
                  ? height - base
                  : y;
        intersect.setY(y);
      }
      intersect.setX(width);
    }
  } else if (iv_top && iv_left) {

    if (top_dist <= left_dist) {

      /* tip above the rectangle - line going down/left */

      if (width - intersect_top.x() < base) {
        placement = TopRight;
        intersect.setX(width);
      } else {
        placement = Top; 
        intersect.setX(intersect_top.x());
        if (intersect.x() < base) {
          intersect.setX(base);
        }
      }
      intersect.setY(0);
    } else {

      /* tip to left of rectangle - line going up/right*/
     
      if (height - intersect_left.y() < base) {
        placement = BottomLeft;
        intersect.setY(height);
      } else {
        placement = Left;
        intersect.setY(intersect_left.y());
        if (intersect.y() < base) {
          intersect.setY(base);
        }
      }
      intersect.setX(0);
    }
  } else if (iv_top && iv_right) {

    if (top_dist <= right_dist) {

      /* tip above the rectangle - line going down/right */

      if (intersect_top.x() < base) {
        placement = TopLeft;
        intersect.setX(0);
      } else {
        placement = Top; 
        intersect.setX(intersect_top.x());
        if (width - intersect.x() < base) {
          intersect.setX(width - base);
        }
      }
      intersect.setY(0);
    } else {

      /* tip to right of rectangle - line going up/left*/
     
      if (height - intersect_left.y() < base) {
        placement = BottomRight;
        intersect.setY(height);
      } else {
        placement = Right;
        intersect.setY(intersect_right.y());
        if (intersect.y() < base) {
          intersect.setY(base);
        }
      }
      intersect.setX(width);
    }
  } else if (iv_bottom && iv_right) {

    if (bot_dist <= right_dist) {

      /* tip below the rectangle - line going up/right */

      if (intersect_bottom.x() < base) {
        placement = BottomLeft;
        intersect.setX(0);
      } else {
        placement = Bottom; 
        intersect.setX(intersect_bottom.x());
        if (width - intersect.x() < base) {
          intersect.setX(width - base);
        }
      }
      intersect.setY(height);
    } else {

      /* tip to right of rectangle - line going down/left */
     
      if (intersect_right.y() < base) {
        placement = TopRight;
        intersect.setY(0);
      } else {
        placement = Right;
        intersect.setY(intersect_right.y());
        if (height - intersect.y() < base) {
          intersect.setY(height - base);
        }
      }
      intersect.setX(width);
    }
  } else if (iv_bottom && iv_left) {

    if (bot_dist <= left_dist) {

      /* tip below the rectangle - line going up/left */

      if (width - intersect_bottom.x() < base) {
        placement = BottomRight;
        intersect.setX(width);
      } else {
        placement = Bottom; 
        intersect.setX(intersect_bottom.x());
        if (intersect.x() < base) {
          intersect.setX(base);
        }
      }
      intersect.setY(height);
    } else {

      /* tip left of the rectangle */
     
      if (intersect_left.y() < base) {
        placement = TopLeft;
        intersect.setY(0);
      } else {
        placement = Left;
        intersect.setY(intersect_left.y());
        if (height - intersect.y() < base) {
          intersect.setY(height - base);
        }
      }
      intersect.setX(0);
    }
  } else {
    /* Bah! The user isn't anywhere near the rectangle */
    return false;
  }
  return true;
}
