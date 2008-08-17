
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

#include "step.h"
#include "ranges.h"
#include "ranges_element.h"
#include "callout.h"
#include "csiitem.h"
#include "metaitem.h"
#include <QMenu>
#include <QAction>
#include <QGraphicsSceneContextMenuEvent>
#include "commonmenus.h"

CsiItem::CsiItem(
  Step          *_step,
  Meta          *_meta,
  QPixmap       &pixmap,
  int            _submodelLevel,
  QGraphicsItem *parent,
  PlacementType  _parentRelativeType)
{
  step    = _step;
  
  assem = &_meta->LPub.assem;
  if (_parentRelativeType == StepGroupType) {
    divider = &_meta->LPub.multiStep.divider;
  } else if (_parentRelativeType == CalloutType) {
    divider = &_meta->LPub.callout.divider;
  } else {
    divider = NULL;
  }
  meta  =  _meta;
  parentRelativeType = _parentRelativeType;
  setPixmap(pixmap);
  setParentItem(parent);
  setTransformationMode(Qt::SmoothTransformation);
  submodelLevel = _submodelLevel;

  setToolTip(step->path());

  setFlag(QGraphicsItem::ItemIsSelectable,true);
  if (parentRelativeType == SingleStepType) {
    setFlag(QGraphicsItem::ItemIsMovable,true);
  }
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
     (parentRelativeType == StepGroupType &&  (boundary & EndOfRanges)))) {
    addNextAction = menu.addAction("Add Next Step");
    addNextAction->setWhatsThis("Add Next Step:\n  Add the first step of the next page to this page");
  }

  QAction *addPrevAction = NULL;
  if ( step->stepNumber.number > 1 &&
     (parentRelativeType == SingleStepType ||
     (parentRelativeType == StepGroupType && (boundary & StartOfRanges)))) {
    addPrevAction = menu.addAction("Add Previous Step");
    addPrevAction->setWhatsThis("Add Previous Step:\n  Add the last step of the previous page to this page");
  }

  QAction *removeAction = NULL;
  if (parentRelativeType == StepGroupType &&
     (boundary & (StartOfRanges | EndOfRanges))) {
    removeAction = menu.addAction("Remove this Step");
    if (boundary & StartOfRanges) {
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
    if ((boundary & StartOfRange) && ! (boundary & StartOfRanges)) {
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

    if ((boundary & EndOfRange) && ! (boundary & EndOfRanges)) {
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
    if ( ! (boundary & EndOfRange) && ! (boundary & EndOfRanges)) {
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
  Where topOfRanges  = step->topOfRanges();
  Where bottomOfRanges = step->bottomOfRanges();
  Where begin = topOfRanges;
  
  if (parentRelativeType == StepGroupType) {
    MetaItem mi;
    mi.scanForward(begin,StepGroupMask);
  }

  if (selectedAction == addPrevAction) {
    addPrevMultiStep(topOfRanges,bottomOfRanges);

  } else if (selectedAction == addNextAction) {
    addNextMultiStep(topOfRanges,bottomOfRanges);

  } else if (selectedAction == removeAction) {
    if (boundary & StartOfRanges) {
      deleteFirstMultiStep(topOfRanges);
    } else {
      deleteLastMultiStep(topOfRanges,bottomOfRanges);
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
                  bottomOfRanges,
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
                    bottomOfRanges, 
                    &meta->LPub.assem.modelScale, 
                    1,allowLocal);
  } else if (selectedAction == marginsAction) {

    switch (parentRelativeType) {
      case StepGroupType:
        changeMargins("Assembly Margins",
                      topOfStep, 
                      bottomOfStep, 
                      &meta->LPub.multiStep.csi.margin);
      break;
      case CalloutType:
        changeMargins("Assembly Margins",
                      topOfStep, 
                      bottomOfStep, 
                      &meta->LPub.callout.csi.margin);
      break;
      case SingleStepType:
        changeMargins("Assembly Margins",
                      topOfStep, 
                      bottomOfStep, 
                      &meta->LPub.assem.margin);
      break;
      default:
      break;
    }
  }
}

void CsiItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mousePressEvent(event);
  if (parentRelativeType == SingleStepType) {
    positionChanged = false;
    position = pos();
  }
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
