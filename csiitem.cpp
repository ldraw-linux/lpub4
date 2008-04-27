
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
  Context       &_context,
  int            _submodelLevel,
  QGraphicsItem *parent,
  PlacementType  _parentRelativeType)
{
  step = _step;
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
  context = _context;
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
  int numSteps = mi.numSteps(step->context.topOfFile().modelName);
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
        movePrevAction = menu.addAction("Move Step Left One Column");
        movePrevAction->setWhatsThis(
          "Move Step Left One Column:\n"
          "  Remove this step from its current column,\n"
          "  and put it in column to the left");
      } else {
        movePrevAction = menu.addAction("Move Step Up One Row");
        movePrevAction->setWhatsThis(
          "Move Step Up One Row:\n"
          "  Remove this step from its current row,\n"
          "  and put in the row above");
      }
    }

    if ((boundary & EndOfRange) && ! (boundary & EndOfRanges)) {
      if (allocType == Vertical) {
        moveNextAction = menu.addAction("Move Step Right One Column");
        moveNextAction->setWhatsThis(
          "Move Step Right One Column:\n"
          "  Remove this step from its current column,\n"
          "  and put it in the column to the right");
      } else {
        moveNextAction = menu.addAction("Move Step Down One Row");
        moveNextAction->setWhatsThis(
          "Move Step Down One Row:\n"
          "  Remove this step from its current row,\n"
          "  and put it in the row below");
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

  QAction *scaleAction = menu.addAction("Change Size");
  scaleAction->setWhatsThis("Change Size:\n"
    "  You can change the size of this assembly image using the scale\n"
    "  dialog (window).  You can make the render window larger, which\n"
    "  makes this assembly image larger, and the final image has more\n"
    "  pixels (e.g. higher resolution.  You can make the render window\n"
    "  smaller");
       
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

  if (selectedAction == addPrevAction) {
    if (parentRelativeType == StepGroupType) {
      addPrevStep(step->context.topOfRanges());
    } else {
      addPrevStep(step->context.topOfStep());
    }
  } else if (selectedAction == addNextAction) {
    if (parentRelativeType == StepGroupType) {
      addNextStep(step->topOfRanges());
    } else {
      addNextStep(step->context.topOfStep());
    }

  } else if (selectedAction == removeAction) {
    if (boundary & StartOfRanges) {
      deleteFirstMultiStep(step->topOfRanges());
    } else {
      deleteLastMultiStep(step->context.topOfRanges());
    }
  } else if (selectedAction == movePrevAction) {

    moveStepPrev(parentRelativeType,
                 step->context.bottomOfRanges(),
                 step->context.bottomOfStep());

  } else if (selectedAction == moveNextAction) {

    moveStepNext(parentRelativeType,
                 step->context.topOfRanges(),
                 step->context.bottomOfRange(),
                 step->context.topOfStep(),
                 step->context.bottomOfStep());

  } else if (selectedAction == addDividerAction) {
    if (parentRelativeType == StepGroupType) {
      addMultiStepDivider(step->context.bottomOfStep(),divider);
    } else {
      addCalloutDivider(step->context.bottomOfStep(),divider);
    }        

  } else if (selectedAction == allocAction) {
    changeAlloc(step->parent->getContext().topOfRanges(),
                step->parent->getContext().bottomOfRanges(),
                step->parent->allocMeta());

  } else if (selectedAction == placementAction) {
    changePlacement(parentRelativeType,
                    CsiType,
                    "Assembly Placement",
                    step->context.topOfStep(), 
                    step->context.bottomOfStep(),
                    &meta->LPub.assem.placement);
  } else if (selectedAction == scaleAction) {
    switch (parentRelativeType) {
      case StepGroupType:
        changeFloatSpin("Assembly","Model Size",step->context.topOfRanges(), &meta->LPub.assem.modelSize, false);
      break;
      case CalloutType:
        changeFloatSpin("Assembly","Model Size",step->topOfRanges(), &meta->LPub.assem.modelSize, false);
      break;
      default:
        changeFloatSpin("Assembly","Model Size",step->context.topOfStep(), &meta->LPub.assem.modelSize);
      break;
    }
  } else if (selectedAction == marginsAction) {

    switch (parentRelativeType) {
      case StepGroupType:
        changeMargins("Assembly Margins",step->context.topOfStep(), &meta->LPub.multiStep.csi.margin);
      break;
      case CalloutType:
        changeMargins("Assembly Margins",step->context.topOfStep(), &meta->LPub.callout.csi.margin);
      break;
      case SingleStepType:
        changeMargins("Assembly Margins",step->context.topOfStep(), &meta->LPub.assem.margin);
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

        changePlacementOffset(step->context.topOfStep(),&meta->LPub.assem.placement);  
      }
      endMacro();
    }
  }
}
