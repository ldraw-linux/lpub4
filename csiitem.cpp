
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
#include <QMenu>
#include <QAction>
#include <QGraphicsRectItem>
#include <QGraphicsSceneContextMenuEvent>

#include "lpub.h"
#include "step.h"
#include "ranges.h"
#include "ranges_element.h"
#include "callout.h"
#include "calloutbackgrounditem.h"
#include "csiitem.h"
#include "metaitem.h"
#include "commonmenus.h"

CsiItem::CsiItem(
  Step          *_step,
  Meta          *_meta,
  QPixmap       &pixmap,
  int            _submodelLevel,
  QGraphicsItem *parent,
  PlacementType  _parentRelativeType)
  :
  QGraphicsPixmapItem(pixmap,parent)
{
  step = _step;
  meta = _meta;
  submodelLevel = _submodelLevel;
  parentRelativeType = _parentRelativeType;
  
  assem = &meta->LPub.assem;
  if (parentRelativeType == StepGroupType) {
    divider = &meta->LPub.multiStep.divider;
  } else if (parentRelativeType == CalloutType) {
    divider = &meta->LPub.callout.divider;
  } else {
    divider = NULL;
  }

  origWidth = pixmap.width();
  origHeight = pixmap.height();

  setTransformationMode(Qt::SmoothTransformation);

  setToolTip(step->path());

  if (parentRelativeType == SingleStepType) {
    setFlag(QGraphicsItem::ItemIsSelectable,true);
    setFlag(QGraphicsItem::ItemIsMovable,true);
  }
  grabSize = toPixels(0.03,DPI);
  modelScale = meta->LPub.assem.modelScale;
  for (int i = 0; i < 8; i++) {
    grab[i] = NULL;
  }
  oldScale = 1.0;
}

void CsiItem::setFlag(GraphicsItemFlag flag, bool value)
{
  QGraphicsItem::setFlag(flag,value);
}

void CsiItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;

  MetaItem mi;
  int numSteps = mi.numSteps(step->top.modelName);
  Boundary boundary = step->boundary();

  QAction *addNextAction = NULL;
  if (step->stepNumber.number != numSteps &&
     (parentRelativeType == SingleStepType ||
     (parentRelativeType == StepGroupType &&  (boundary & EndOfSteps)))) {
    addNextAction = menu.addAction("Add Next Step");
    addNextAction->setWhatsThis("Add Next Step:\n  Add the first step of the next page to this page");
  }

  QAction *addPrevAction = NULL;
  if ( step->stepNumber.number > 1 &&
     (parentRelativeType == SingleStepType ||
     (parentRelativeType == StepGroupType && (boundary & StartOfSteps)))) {
    addPrevAction = menu.addAction("Add Previous Step");
    addPrevAction->setWhatsThis("Add Previous Step:\n  Add the last step of the previous page to this page");
  }

  QAction *removeAction = NULL;
  if (parentRelativeType == StepGroupType &&
     (boundary & (StartOfSteps | EndOfSteps))) {
    removeAction = menu.addAction("Remove this Step");
    if (boundary & StartOfSteps) {
      removeAction->setWhatsThis("Remove this Step:\n  Move this step from this page to the previous page");
    } else {
      removeAction->setWhatsThis("Remove this Step:\n  Move this step from this page to the next page");
    }
  }

  QAction *movePrevAction = NULL;
  QAction *moveNextAction = NULL;
  QAction *addDividerAction = NULL;
  QAction *allocAction = NULL;

  AllocEnc allocType = step->parent->allocType();
  if (parentRelativeType == StepGroupType || parentRelativeType == CalloutType) {
    if ((boundary & StartOfRange) && ! (boundary & StartOfSteps)) {
      if (allocType == Vertical) {
        movePrevAction = menu.addAction("Add to Previous Column");
        movePrevAction->setWhatsThis(
          "Add to Previous Column:\n"
          "  Move this step to the previous column");
      } else {
        movePrevAction = menu.addAction("Add to Previous Row");
        movePrevAction->setWhatsThis(
          "Add to Previous Row:\n"
          "  Move this step to the previous row\n");
      }
    }

    if ((boundary & EndOfRange) && ! (boundary & EndOfSteps)) {
      if (allocType == Vertical) {
        moveNextAction = menu.addAction("Add to Next Column");
        moveNextAction->setWhatsThis(
          "Add to Next Colum:\n"
          "  Remove this step from its current column,\n"
          "  and put it in the column to the right");
      } else {
        moveNextAction = menu.addAction("Add to Next Row");
        moveNextAction->setWhatsThis(
          "Add to Next Row:\n"
          "  Remove this step from its current column,\n"
          "  and put it in the row above");
      }
    }
    if ( ! (boundary & EndOfRange) && ! (boundary & EndOfSteps)) {
      addDividerAction = menu.addAction("Add Divider After Step");
      if (allocType == Vertical) {
        addDividerAction->setWhatsThis(
          "Add Divider After Step:\n"
          "  Put the step(s) after this into a new column");
      } else {
        addDividerAction->setWhatsThis(
          "Add Divider After Step:\n"
          "  Put the step(s) after this into a new row");
      } 
    }
    if (allocType == Vertical) {
      allocAction = menu.addAction("Display as Rows");
      allocAction->setWhatsThis(
        "Display as Rows:\n"
        "  Change this whole set of steps from columns of steps\n"
        "  to rows of steps");
    } else {
      allocAction = menu.addAction("Display as Columns");
      allocAction->setWhatsThis(
        "Display as Columns:\n"
        "  Change this whole set of steps from rows of steps\n"
        "  to columns of steps");
    }
  }

  QAction *placementAction = NULL;

  if (parentRelativeType == SingleStepType) {
    placementAction = menu.addAction("Move This Step");
    placementAction->setWhatsThis(   
      "Move This Step:\n"
      "  Move this assembly step image using a dialog (window)\n"
      "  with buttons.  You can also move this step image around\n"
      "  by clicking and dragging it using the mouse.");
  }

  QAction *scaleAction = menu.addAction("Change Scale");
  scaleAction->setWhatsThis("Change Scale:\n"
    "  You can change the size of this assembly image using the scale\n"
    "  dialog (window).  A scale of 1.0 is true size.  A scale of 2.0\n"
    "  doubles the size of your model.  A scale of 0.5 makes your model\n"
    "  half real size\n");
       
  QAction *marginsAction = menu.addAction("Change Assembly Margins");

  switch (parentRelativeType) {
    case SingleStepType:
      marginsAction->setWhatsThis("Change Assembly Margins:\n"
        "  Margins are the empty space around this assembly picture.\n"
        "  You can change the margins if things are too close together,\n"
        "  or too far apart. ");
    break;
    case StepGroupType:
      marginsAction->setWhatsThis("Change Assembly Margins:\n"
        "  Margins are the empty space around this assembly picture.\n"
        "  You can change the margins if things are too close together,\n"
        "  or too far apart. You can change the margins around the\n"
        "  whole group of steps, by clicking the menu button with your\n"
        "  cursor near this assembly image, and using that\n"
        "  \"Change Step Group Margins\" menu");
    break;
    case CalloutType:
      marginsAction->setWhatsThis("Change Assembly Margins:\n"
        "  Margins are the empty space around this assembly picture.\n"
        "  You can change the margins if things are too close together,\n"
        "  or too far apart. You can change the margins around callout\n"
        "  this step is in, by putting your cursor on the background\n"
        "  of the callout, clicking the menu button, and using that\n"
        "  \"Change Callout Margins\" menu");
    break;
    default:
    break;
  }

  QAction *selectedAction = menu.exec(event->screenPos());

  if ( ! selectedAction ) {
    return;
  }
  
  Callout *callout = step->callout();
  
  Where topOfStep    = step->topOfStep();
  Where bottomOfStep = step->bottomOfStep();
  Where topOfSteps  = step->topOfSteps();
  Where bottomOfSteps = step->bottomOfSteps();
  Where begin = topOfSteps;
  
  if (parentRelativeType == StepGroupType) {
    MetaItem mi;
    mi.scanForward(begin,StepGroupMask);
  }

  if (selectedAction == addPrevAction) {
    addPrevMultiStep(topOfSteps,bottomOfSteps);

  } else if (selectedAction == addNextAction) {
    addNextMultiStep(topOfSteps,bottomOfSteps);

  } else if (selectedAction == removeAction) {
    if (boundary & StartOfSteps) {
      deleteFirstMultiStep(topOfSteps);
    } else {
      deleteLastMultiStep(topOfSteps,bottomOfSteps);
    }
  } else if (selectedAction == movePrevAction) {

    addToPrev(parentRelativeType,topOfStep);

  } else if (selectedAction == moveNextAction) {

    addToNext(parentRelativeType,topOfStep);
              
  } else if (selectedAction == addDividerAction) {
    addDivider(parentRelativeType,bottomOfStep,divider);
  } else if (selectedAction == allocAction) {
    if (parentRelativeType == StepGroupType) {
      changeAlloc(begin,
                  bottomOfSteps,
                  step->allocMeta());
    } else {
      changeAlloc(callout->topOfCallout(),
                  callout->bottomOfCallout(),
                  step->allocMeta());
    }

  } else if (selectedAction == placementAction) {
    changePlacement(parentRelativeType,
                    CsiType,
                    "Assembly Placement",
                    topOfStep, 
                    bottomOfStep,
                    &meta->LPub.assem.placement);
  } else if (selectedAction == scaleAction) {
    bool allowLocal = parentRelativeType != StepGroupType && 
                      parentRelativeType != CalloutType;
    changeFloatSpin("Assembly",
                    "Model Size",
                    begin, 
                    bottomOfSteps, 
                    &meta->LPub.assem.modelScale, 
                    1,allowLocal);
  } else if (selectedAction == marginsAction) {

    MarginsMeta *margins;
    
    switch (parentRelativeType) {
      case StepGroupType:
        margins = &meta->LPub.multiStep.csi.margin;
      break;
      case CalloutType:
        margins = &meta->LPub.callout.csi.margin;
      break;
      default:
        margins = &meta->LPub.assem.margin;
      break;
    }
    changeMargins("Assembly Margins",
                  topOfStep, 
                  bottomOfStep, 
                  margins);
  }
}

void CsiItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mousePressEvent(event);
  if (parentRelativeType == SingleStepType) {
    positionChanged = false;
    position = pos();
    placeGrabs();
  }
  gui->showLine(step->topOfStep());
}

void CsiItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {
    QPoint delta(pos().x() - position.x() + 0.5,
                 pos().y() - position.y() + 0.5);

    if (delta.x() || delta.y()) {
      for (int i = 0; i < step->list.size(); i++) {
        Callout *callout = step->list[i];
        callout->drawTips(delta);
      }      
      positionChanged = true;
      placeGrabs();
    }
    QGraphicsPixmapItem::mouseMoveEvent(event);
  }
}

void CsiItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseReleaseEvent(event);

  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {
    if (positionChanged) {

      beginMacro(QString("DragCsi"));

      QPointF delta(pos().x() - position.x(),
                    pos().y() - position.y());  

      // back annotate the movement of the CSI into the LDraw file.
      PlacementData placementData = meta->LPub.assem.placement.value();
      placementData.offsets[0] += delta.x()/meta->LPub.page.size.value(0);
      placementData.offsets[1] += delta.y()/meta->LPub.page.size.value(1);
      meta->LPub.assem.placement.setValue(placementData);

      QPoint deltaI(delta.x(),delta.y());

      if (step) {

        for (int i = 0; i < step->list.size(); i++) {
          Callout *callout = step->list[i];
          callout->updatePointers(deltaI);
        }

        changePlacementOffset(step->topOfStep(),&meta->LPub.assem.placement);  
      }
      endMacro();
    }
  }
}

void CsiItem::placeGrabs()
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
      grab[i] = new CsiGrab(this);
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

void CsiItem::whatPoint(CsiGrab *grabbed)
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

void CsiItem::resize(QPointF grabbed)
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
    
    QPointF size = QPointF(width,height);
    
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
    points[BottomRight] = pos() + size;
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

void CsiItem::changeModelScale()
{
  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {
    if (positionChanged) {

      beginMacro(QString("DragCsi"));

      QPointF delta(pos() - position);
      qreal deltaX = (sceneBoundingRect().width() - origWidth)/2;
      qreal deltaY = (sceneBoundingRect().height() - origHeight)/2;
      
      delta.setX(delta.x() + deltaX);
      delta.setY(delta.y() + deltaY);

      // back annotate the movement of the CSI into the LDraw file.
      PlacementData placementData = meta->LPub.assem.placement.value();
      placementData.offsets[0] += delta.x()/meta->LPub.page.size.value(0);
      placementData.offsets[1] += delta.y()/meta->LPub.page.size.value(1);
      meta->LPub.assem.placement.setValue(placementData);

      QPoint deltaI(delta.x(),delta.y());

      if (step) {
        for (int i = 0; i < step->list.size(); i++) {
          Callout *callout = step->list[i];
          callout->updatePointers(deltaI);
        }

        changePlacementOffset(step->topOfStep(),&meta->LPub.assem.placement);  
      }
      
      qreal scale = modelScale.value();
      scale *= oldScale;
      modelScale.setValue(scale);
      
      changeFloat(step->topOfStep(),step->bottomOfStep(),&modelScale);
      
      endMacro();
    }
  }
}

QVariant CsiItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
  if (grab[0] && change == ItemSelectedChange) {
    for (int i = 0; i < 4; i++) {
      grab[i]->setVisible(value.toBool());
    }
  }
  return QGraphicsItem::itemChange(change,value);
}

void CsiGrab::mousePressEvent(QGraphicsSceneMouseEvent * /* event */)
{
  csiItem->whatPoint(this);
}
void CsiGrab::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  csiItem->resize(event->scenePos());
}
void CsiGrab::mouseReleaseEvent(QGraphicsSceneMouseEvent * /* event */)
{
  csiItem->changeModelScale();
}

