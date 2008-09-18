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

#ifndef POINTERITEMH
#define POINTERITEMH

#include "pointer.h"
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsLineItem>
#include <QGraphicsItemGroup>
#include "metaitem.h"

class QGraphicsView;

class CalloutPointerItem : public QGraphicsItemGroup, public MetaItem
{
public:
  CalloutPointerItem(
    QRect               &callout,
    QRect               &csi,
    Meta                *meta,
    Pointer             *pointer,
    int                  submodelLevel,
    QGraphicsItem       *parent,
    QGraphicsView       *view);

public:
  enum SelectedPoint { C1, Tip, C2, Loc, None };

private:
  QGraphicsView *view;
  QString backgroundColor;
  QString borderColor;
  float   borderThickness;
  int     grabSize;
  QRect   calloutRect;
  QRect   csiRect;
  Pointer pointer;
  int     submodelLevel;

  QRect   rect;       // the rect that encloses polygon
  float   base;       // the base used to calculate C1 and C2
                      // the edge/corner where loc is located
  PlacementEnc placement;
  bool    positionChanged;

  QGraphicsPolygonItem *poly1, *poly2;
  QGraphicsLineItem    *line1, *line2;
  QGraphicsRectItem    *grab[4];
  QPointF points[4];  // the points on the inside polygon

  /*
   *   +--------------------------------------------++
   *   |                                             |
   *   | . +-------------------------------------+   |
   *   |   |                                     | . |
   *   |   |                                     |   |
   *
   *
   *  callout size defines the outside edge of the callout.
   *  When there is a border, the inside rectangle starts
   *  at +thickness,+thickness, and ends at size-thickness,
   *  size-tickness.
   *
   *  Using round end cap caps the ends of the lines that
   *  intersect the callout are at +- tickness/2.  I'm not
   *  sure the affect of thickness is even vs. odd.
   *
   *  Loc should be calculated on the inside rectangle?
   *  The triangles have to go to the edge of the inner
   *  rectangle to obscure the border.
   *
   *  Top Left corner:
   *    Loc is at (thickness,thickness)
   *    C1 = Loc + (base, 0)
   *    C2 = Loc + (0,base)
   *    L1 = C1 - (0,thickness/2)
   *    L2 = C2 + (thickness/2,0)
   *
   *  Top edge:
   *    Loc is at (thickness,thickness) + (width-2*thickness)*loc
   *    C1 = Loc - (base, 0)
   *    C2 = Loc - (base, 0)
   *    L1 = C1 - (0,thickness/2)
   *    L2 = C2 - (0,thickness/2)
   *
   *  Top right corner:
   *    Loc = (width - thickness, thickness)
   *    C1  = Loc - (base,0)
   *    C2  = Loc + (0,base)
   *    L1  = C1 - (0,thickness/2)
   *    L2  = C2 + (thickness/2,0)
   *
   *  Right edge:
   *    Loc = (width - thickness, thickness + (height-2*thickness)*loc
   *    C1  = Loc - (0,base)
   *    C2  = Loc + (0,base)
   *    L1  = C1 + (thickness/2,0)
   *    L2  = C2 + (thickness/2,0)
   *
   *  Bottom right:
   *    Loc = width - thickness, height - thickness
   *    C1  = Loc - (base,0)
   *    C2  = Loc - (0,base)
   *    L1  = C1  + (thickness/2,0)
   *    L2  = C2  + (0,thickness/2)
   *
   *  Bottom:
   *    Loc = thickness,height - thickness
   *    C1  = Loc - (base,0)
   *    C2  = Loc + (base,0)
   */

  enum SelectedPoint selectedPoint;

public:

  void drawTip(QPoint delta);

  /* When we drag the pointer tip, we move the tip relative
   * to the CSI rect, so we must recalculate the offset
   * into the CSI */

  void updatePointer(QPoint &delta);

  /* When the user "Add Pointer", we need to give a default/
     reasonable pointer */

  void defaultPointer();

private:
  /* Drag the tip of the pointer, and calculate a good
   * location for the pointer to connect to the callout. */

  bool autoLocTip();

  /* When we drag the CSI or the pointer's callout, we
   * need recalculate the Location portion of the pointer
   * meta, but the offset remains unchanged */

  void calculatePointerMetaLoc();

  void calculatePointerMeta();

  /* When using menu to add a new pointer, we need to add
   * a new line to the LDraw file. */

  void addPointerMeta();

  /* When we drag the callout or CSI, we need to recalculate
   * the pointer . */

  void drawPointerPoly();
  bool autoLocLoc(QPoint loc);
  void adjustC1C2();
  void sizeGrab();

protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
  void focusInEvent(QFocusEvent *event);
  void focusOutEvent(QFocusEvent *event);
};

#endif
