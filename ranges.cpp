
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
 * This class contains one or more individual range's.
 * By itself, this class represents step groups.  Callouts are derived
 * from ranges.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "ranges.h"
#include "ranges_item.h"
#include "range_element.h"
#include "range.h"
#include "step.h"
#include "meta.h"
#include "callout.h"
#include "calloutbackgrounditem.h"
#include "lpub.h"
#include "dividerdialog.h"

Steps::Steps()
{
  relativeType  = SingleStepType;
  pli.steps = this;
  pli.callout = NULL;
}

Steps::Steps(Meta &_meta,QGraphicsView *_view)
{
  meta = _meta;
  view = _view;
}

Steps::~Steps()
{
  for (int i = 0; i < list.size(); i++) {
    delete list[i];
  }
  list.clear();
}

QString Steps::modelName()
{
  return list[0]->topOfRange().modelName;
}

QString Steps::path()
{
  QString thePath;
  for (int i = 0; i < meta.submodelStack.size(); i++) {
    thePath += "/" + QFileInfo(meta.submodelStack[i].modelName).baseName();
  }
  thePath += "/" + QFileInfo(modelName()).baseName();
  return thePath;
}

QString Steps::csiName()
{
  QString thePath;

  for (int i = 0; i < meta.submodelStack.size(); i++) {
    QString lineNum = QString("%1") .arg(meta.submodelStack[i].lineNumber);
    thePath += "_" + QFileInfo(meta.submodelStack[i].modelName).baseName() + "_" + lineNum;
  }
  
  thePath += "_" + QFileInfo(modelName()).baseName();
  return thePath;
}

const Where &Steps::bottomOfStep(
  AbstractStepsElement *me)
{
  for (int i = 0; i < list.size(); i++) {
    if (list[i] == me) {
      if (i < list.size()-1) {
        return list[i+1]->topOfRange();
      }
    }
  }
  return bottom;
}

AbstractStepsElement *Steps::nextRange(
  const AbstractStepsElement *me)
{
  int size = list.size();
  
  for (int i = 0; i < size; i++) {
    if (list[i] == me) {
      if (i < size - 1) {
        return list[i+1];
      } else {
        return NULL;
      }
    }
  }
  return NULL;
}
  

const Where &Steps::topOfSteps()
{
  return list[0]->topOfRange();
}
const Where &Steps::bottomOfSteps()
{
  return bottom;
}
void Steps::setBottomOfSteps(const Where &bos)
{
  bottom = bos;
}

QStringList Steps::submodelStack()
{
  QStringList submodelStack;
  Where filename;
  foreach (filename,meta.submodelStack) {
    submodelStack << filename.modelName;
  }
  return submodelStack;
}

void Steps::append(AbstractStepsElement *re)
{
  list.append(re);
}

AllocEnc Steps::allocType()
{
  return meta.LPub.multiStep.alloc.value();
}

AllocMeta &Steps::allocMeta()
{
  return meta.LPub.multiStep.alloc;
}
/*********************************************
 *
 * ranges.h function
 *
 * ranges.h
 *
 * ranges are used for two reasons
 *   1) single step/multi step pages
 *   2) callouts
 * in both cases, ranges contain a list of individual ranges (Range *)
 *
 ********************************************/

/* This destorys everything in its list, but not itself. */

void Steps::freeSteps()
{
  for (int i = 0; i < list.size(); i++) {
    AbstractStepsElement *re = list[i];
    delete re;
  }
  list.clear();
  relativeType = SingleStepType;
  relativeToList.clear();
}

void Steps::sizeIt(void)
{
  FreeFormData freeFormData;
  AllocEnc     allocEnc;
  if (relativeType == CalloutType) {
    freeFormData = meta.LPub.callout.freeform.value();
    allocEnc = meta.LPub.callout.alloc.value();
  } else {
    freeFormData = meta.LPub.multiStep.freeform.value();
    allocEnc = meta.LPub.multiStep.alloc.value();
  }

  if (freeFormData.mode) {
    if (allocEnc == Vertical) {
      sizeitFreeform(XX,YY);
    } else {
      sizeitFreeform(YY,XX);
    }
  } else {
    if (allocEnc == Vertical) {
      sizeitVert();
    } else {
      sizeitHoriz();
    }
  }
}

/*
 * This provides Vertical packing
 */

void Steps::sizeitVert(void)
{
  if (relativeType == CalloutType && list.size() == 1) {
    if (list[0]->relativeType == RangeType) {
      Range *range = dynamic_cast<Range *>(list[0]);
      if (range && range->list.size() == 1) {
        if (range->list[0]->relativeType == StepType) {
          Step *step = dynamic_cast<Step *>(range->list[0]);
          step->stepNumber.number = -1;
        }
      }
    }
  }

  /*
   * Size each range, determining its width and height
   * accumulating its width, and finding the maximum height
   */

  size[XX] = 0;
  size[YY] = 0;

  SepData divider;

  if (relativeType == CalloutType) {
    divider = meta.LPub.callout.sep.value();
  } else {
    divider = meta.LPub.multiStep.sep.value();
  }
  

  /* foreach range */

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == RangeType) {
      Range *range = dynamic_cast<Range *>(list[i]);
      if (range) {

        range->sizeitVert();

        /* find the tallest range */

        if (range->size[YY] > size[YY]) {
          size[YY] = range->size[YY];
        }

        /* place each range Horizontally */

        range->offset[YY] = 0;
        range->offset[XX] = size[XX];

        size[XX] += range->size[XX];

        if (i + 1 < list.size()) {

          /* accumulate total width of ranges */

          size[XX] += 2*divider.margin[XX] + divider.thickness;
        }
      }
    }
  }

  /*
   * Each range is placed, but now we need to
   * evenly space the steps within a range
   */

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == RangeType) {
      Range *range = dynamic_cast<Range *>(list[i]);
      if (range) {

        /* space steps within column based on tallest column */

        range->placeitVert(size[YY]);
      }
    }
  }
}

/*
 * This provides Horizontal packing
 */

void Steps::sizeitHoriz(void)
{
  if (relativeType == CalloutType && list.size() == 1) {
    if (list[0]->relativeType == RangeType) {
      Range *range = dynamic_cast<Range *>(list[0]);
      if (range && range->list.size() == 1) {
        if (range->list[0]->relativeType == StepType) {
          Step *step = dynamic_cast<Step *>(range->list[0]);
          step->stepNumber.number = -1;
        }
      }
    }
  }

  /*
   * Size each range, determining its width and height
   * accumulating its width, and finding the maximum height
   */

  size[XX] = 0;
  size[YY] = 0;

  SepData divider;

  if (relativeType == CalloutType) {
    divider = meta.LPub.callout.sep.value();
  } else {
    divider = meta.LPub.multiStep.sep.value();
  }

  /* foreach range */

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == RangeType) {
      Range *range = dynamic_cast<Range *>(list[i]);
      if (range) {

        range->sizeitHoriz();

        /* find the widest range */

        if (range->size[XX] > size[XX]) {
          size[XX] = range->size[XX];
        }

        /* place each range Horizontally */

        range->offset[XX] = 0;
        range->offset[YY] = size[YY];

        size[YY] += range->size[YY];

        if (i + 1 < list.size()) {

          /* accumulate total width of ranges */

          size[YY] += 2*divider.margin[YY] + divider.thickness;
        }
      }
    }
  }

  /*
   * Each range is placed, but nowe we need to
   * evenly space the steps within a range
   */

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == RangeType) {
      Range *range = dynamic_cast<Range *>(list[i]);
      if (range) {

        /* space steps within column based on tallest column */

        range->placeitHoriz(size[XX]);
      }
    }
  }
}

void Steps::sizeitFreeform(
  int xx,
  int yy)
{
  if (relativeType == CalloutType && list.size() == 1) {
    if (list[0]->relativeType == RangeType) {
      Range *range = dynamic_cast<Range *>(list[0]);
      if (range && range->list.size() == 1) {
        if (range->list[0]->relativeType == StepType) {
          Step *step = dynamic_cast<Step *>(range->list[0]);
          step->stepNumber.number = -1;
        }
      }
    }
  }

  /*
   * Size each range, determining its width and height
   * accumulating its width, and finding the maximum height
   */

  size[xx] = 0;
  size[yy] = 0;

  int lastMargin = 0;

  /* foreach range */

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == RangeType) {
      Range *range = dynamic_cast<Range *>(list[i]);
      if (range) {

//      range->sizeitFreeform(xx,yy,freeform.base,freeform.justification);

        /* find the tallest range */

        if (range->size[yy] > size[yy]) {
          size[yy] = range->size[yy];
        }

        /* place each range Horizontally */

        range->offset[yy] = 0;
        range->offset[xx] = size[xx];

        if (relativeType == StepGroupType) {
          Range *realRange = dynamic_cast<Range *>(range);
          realRange->offset[xx] += realRange->leftAdjust;
        }

        /* accumulate total width of ranges */

        lastMargin = range->margin.value(xx);
        size[xx] += range->size[xx] + lastMargin;
      }
    }
  }
  size[xx] -= lastMargin;

  /*
   * Each range is placed, but now we need to
   * evenly space the steps within a range
   */
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == RangeType) {
      Range *range = dynamic_cast<Range *>(list[i]);
      if (range) {

        /* space steps within column based on tallest column */

//      range->placeitFreeform(xx,yy,size[yy],freeform.justification);
      }
    }
  }
}

void Steps::addGraphicsItems(
  int ox,
  int oy,
  QGraphicsItem *parent)
{
  QGraphicsItem *backDrop;
  QRectF rect = QRectF(ox + offset[XX], oy + offset[YY],size[XX],size[YY]);
  backDrop = new MultiStepRangesBackgroundItem(this,rect,parent,&meta);

  AllocEnc allocEnc;

  if (relativeType == CalloutType) {
    allocEnc = meta.LPub.callout.alloc.value();
  } else {
    allocEnc = meta.LPub.multiStep.alloc.value();
  }

  if (allocEnc == Vertical) {
    addGraphicsItemsVert(ox,oy,backDrop);
  } else {
    addGraphicsItemsHoriz(ox,oy,backDrop);
  }
}

/*
 * For MultiStep                Callout
 *       TransparentBackground    ColorBackground/Border
 * 
 *     Range
 *       TransparentRect
 */

void Steps::addGraphicsItemsVert(
  int offset_x,
  int offset_y,
  QGraphicsItem *parent)
{

  if (pli.tsize()) {
    pli.addPli(meta.submodelStack.size(), parent);
  }

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == RangeType) {
      Range *range = dynamic_cast<Range *>(list[i]);
      if (range) {
        QGraphicsItem *rb;

        if (relativeType == CalloutType) {
          rb = parent;
        } else {       
          rb = new MultiStepRangeBackgroundItem(this,range,&meta,
                    offset_x + offset[XX],
                    offset_y + offset[YY],
                    parent);
        }
        range->addGraphicsItemsVert(
          offset_x + offset[XX], offset_y + offset[YY], &meta, relativeType, rb);

        if (list.size() > 1 && i < list.size() - 1) {
          // add divider here
          int size = range->list.size();
          if (size) {
            Step *step = dynamic_cast<Step *>(range->list[size-1]);          
            if (step) {
              DividerItem *divider = new DividerItem(
                              step,
                             &meta,
                              offset_x + offset[XX] + range->offset[XX] + range->size[XX],
                              offset_y + offset[YY] + range->offset[YY]);
              divider->setParentItem(parent);
            }
          }
        }
      }
    }
  }
}

/*
 *
 */

void Steps::addGraphicsItemsHoriz(
  int offset_x,
  int offset_y,
  QGraphicsItem *parent)
{
  if (pli.tsize()) {
    pli.addPli(meta.submodelStack.size(), parent);
  }

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == RangeType) {
      Range *range = dynamic_cast<Range *>(list[i]);
      if (range) {
        QGraphicsItem *rb;

        if (relativeType == CalloutType) {
          rb = parent;
        } else {       
          rb = new MultiStepRangeBackgroundItem(this,range,&meta,
                    offset_x + offset[XX],
                    offset_y + offset[YY],
                    parent);
        }

        range->addGraphicsItemsHoriz(
          offset_x + offset[XX], offset_y + offset[YY], &meta, relativeType, rb);

        if (list.size() > 1 && i < list.size() - 1) {
          int size = range->list.size();
          
          if (size) {
            Step *step = dynamic_cast<Step *>(range->list[size-1]);
            if (step) {
              DividerItem *divider = new DividerItem(
                         step,
                        &meta,
                          offset_x + offset[XX] + range->offset[XX],
                          offset_y + offset[YY] + range->offset[YY] + range->size[YY]);
              divider->setParentItem(parent);
            }
          }
        }
      }
    }
  }
}

Boundary Steps::boundary(AbstractStepsElement *me)
{
  if (list.size() == 1) {
    return StartAndEndOfSteps;
  } else {
    if (list[0] == me) {
      return StartAndEndOfSteps;
    }
    if (list[list.size()-1] == me) {
      return EndOfSteps;
    }
  }
  return Middle;
}
