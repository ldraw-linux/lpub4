
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
 * This class implements a set of consecutive steps within either a row
 * or column of a callout or multi-step.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "range.h"
#include "step.h"
#include "reserve.h"
#include "meta.h"
#include <QMenu>

Range::Range(
  Steps        *_parent,
  AllocEnc      _allocType,
  FreeFormMeta  _freeform)
{
  parent       = _parent;
  allocType    = _allocType;
  freeform     = _freeform;
  relativeType = RangeType;
}

Range::~Range()
{
  for (int i = 0; i < list.size(); i++) {
    AbstractRangeElement *re = list[i];
    delete re;
  }
  list.clear();
}

void Range::append(AbstractRangeElement *gi)
{
  list.append(gi);
}

/*********************************************
 *
 * Range functions
 *
 * Range
 *
 * range contains a list of steps (possibly reserve too)
 *
 ********************************************/


/*
 * to size a range Vertically
 *   1. determine the height of each step
 *   2. place the step Vertically
 *   3. determine the width of each column
 *   4. place each step Horizontally
 *      determine width of range
 *   5. place each step Vertically
 *      determine height of range
 *
 * to size a range Horizontally
 *   1. determine the width of each step
 *   2. place the step Horizontally
 *   3. determine the height of each row
 *   4. place each step Vertically
 *      determine height of each range
 *   5. place each step Horizontally
 *      determine width of range
 */

void Range::sizeitVert()
{
  int     rows[NumPlaces];
  int     rowsMargin[NumPlaces][2];
  int     cols[NumPlaces];
  int     colsMargin[NumPlaces][2];

  /* we accumulate the widest of the columns within the steps */

  for (int i = 0; i < NumPlaces; i++) {
    cols[i] = 0;
    colsMargin[i][0] = 0;
    colsMargin[i][1] = 0;
  }

  /* size each step, and place its components Vertically */

  int lastMargin = 0;

  size[YY] = 0;
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      for (int i = 0; i < NumPlaces; i++) {
        rows[i] = 0;
        rowsMargin[i][0] = 0;
        rowsMargin[i][1] = 0;
      }

      /* size the step both Vertically and Horizontally */

      step->sizeitVert(rows,cols,rowsMargin,colsMargin);

      /* place the step components Vertically */

      step->placeitVert(rows,rowsMargin,YY);

      /* accumulate minimum height of column */

      int topMargin, botMargin;

      step->vertMargin(topMargin,botMargin);
      if (topMargin > lastMargin) {
        lastMargin = topMargin;
      }

      size[YY] += step->size[YY] + lastMargin;
    } else if (list[i]->relativeType == ReserveType) {
      Reserve *reserve = dynamic_cast<Reserve *>(list[i]);
      size[YY] += reserve->size[YY];
    }
  }

  size[YY] -= lastMargin;

  /* place all step's components into columns */

  size[XX] = 0;
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      /* place the step's components Horizontally (columns) */

      step->placeitVert(cols,colsMargin,XX);

      if (step->size[XX] > size[XX]) {
        size[XX] = step->size[XX];
      }
    }
  }

  /* Now, for step components that are "inside" */

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      /* place the step's components Horizontally (columns) */

      step->placeInside();
    }
  }
}

/*
 * we know the tallest of all the columns, so evenly
 * space the steps within the column
 */
void Range::placeitVert(int max)
{
  /* determine the spacing needed */

  int spacing;
  int y;

  if (list.size() < 3) {
    spacing = (max - size[YY])/(list.size() + 1);
    y = spacing;
  } else {
    spacing = (max - size[YY])/(list.size() - 1);
    y = 0;
  }

  /* evenly space the steps Vertically */
  int lastMargin = 0;

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      step->offset[YY] = y;
      step->offset[XX] = 0;

      y += step->size[YY] + spacing;

      /* accumulate minimum height of column */

      int topMargin, botMargin;

      step->vertMargin(topMargin,botMargin);
      if (topMargin > lastMargin) {
        lastMargin = topMargin;
      }

      y += lastMargin;
    } else if (list[i]->relativeType == ReserveType) {
      Reserve *reserve = dynamic_cast<Reserve *>(list[i]);
      if (reserve) {
        y += reserve->size[YY];
      }
    }
    size[YY] = y - spacing;
  }
  size[YY] = max;
}

void Range::sizeitHoriz()
{
  int     rows[NumPlaces];
  int     rowsMargin[NumPlaces][2];
  int     cols[NumPlaces];
  int     colsMargin[NumPlaces][2];

  /* we accumulate the tallest of the rows within the steps */

  for (int i = 0; i < NumPlaces; i++) {
    rows[i] = 0;
    rowsMargin[i][0] = 0;
    rowsMargin[i][1] = 0;
  }

  /* size each step, and place its components Horizontally */

  int lastMargin = 0;

  size[XX] = 0;
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      for (int i = 0; i < NumPlaces; i++) {
        cols[i] = 0;
        colsMargin[i][0] = 0;
        colsMargin[i][1] = 0;
      }

      /* size the step both Vertically and Horizontally */

      step->sizeitHoriz(rows,cols,rowsMargin,colsMargin);

      /* place the step components Horizontally */

      step->placeitHoriz(cols,colsMargin,XX);

      /* accumulate minimum width of row */

      int topMargin, botMargin;

      step->vertMargin(topMargin,botMargin);
      if (topMargin > lastMargin) {
        lastMargin = topMargin;
      }

      size[XX] += step->size[XX] + lastMargin;
    } else if (list[i]->relativeType == ReserveType) {
      Reserve *reserve = dynamic_cast<Reserve *>(list[i]);
      size[XX] += reserve->size[XX];
    }
  }
  size[XX] -= lastMargin;

  /* place all step's components into rows */

  size[YY] = 0;
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      /* place the step's components Horizontally (columns) */

      step->placeitHoriz(rows,rowsMargin,YY);

      if (step->size[YY] > size[YY]) {
        size[YY] = step->size[YY];
      }
    }
  }

  /* Now, for step components that are "inside" */

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      /* place the step's components Horizontally (columns) */

      step->placeInside();
    }
  }
}

/*
 * we know the tallest of all the columns, so evenly
 * space the steps within the column
 */
void Range::placeitHoriz(int max)
{
  /* determine the spacing needed */

  int spacing,x;

  if (list.size() < 3) {
    spacing = (max - size[XX])/(list.size() + 1);
    x = spacing;
  } else {
    spacing = (max - size[XX])/(list.size() - 1);
    x = 0;
  }

  /* evenly space the steps Vertically */

  int lastMargin = 0;
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      step->offset[XX] = x;
      step->offset[YY] = 0;

      x += step->size[XX] + spacing;

      int topMargin, botMargin;

      step->vertMargin(topMargin,botMargin);
      if (topMargin > lastMargin) {
        lastMargin = topMargin;
      }

      x += lastMargin;
    } else if (list[i]->relativeType == ReserveType) {
      Reserve *reserve = dynamic_cast<Reserve *>(list[i]);
      if (reserve) {
        x += reserve->size[XX];
      }
    }
    size[XX] = x - spacing;
  }
  size[XX] = max;
}

/*****************************************************************************
 *
 * freeform
 *
 ****************************************************************************/

void Range::sizeitFreeform(
  int xx,
  int yy,
  int base,
  int justification)
{
  int maxLeft = 50000, maxRight = - 50000;
  int left, right;

  size[xx] = 0;
  size[yy] = margin.value(yy);

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      step->sizeitFreeform(xx,yy,base, justification, left, right);
      if (left < maxLeft) {
        maxLeft = left;
      }
      if (right > maxRight) {
        maxRight = right;
      }
      size[yy] += step->size[yy] + step->margin.value(yy);
    } else if (list[i]->relativeType == ReserveType) {
      Reserve *reserve = dynamic_cast<Reserve *>(list[i]);

      size[yy] += reserve->size[yy];
    }
  }

  leftAdjust = -maxLeft;

  size[xx] += maxRight - maxLeft + 2*margin.value(xx);
}

/*
 * we know the tallest of all the columns, so evenly
 * space the steps within the column
 */
void Range::placeitFreeform(
  int xx,
  int yy,
  int max,
  int justification)
{
  /* determine the spacing needed */

  int spacing = (max - size[yy])/(list.size() + 1);

  int y = margin.value(yy) + spacing;

  /* evenly space the steps Vertically */

  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);

      step->offset[yy] = y;

      /* Horizontal placement */

      switch (justification) {
        case Left:
        case Top:
          step->offset[xx] = margin.value(xx);
        break;
        case Center:
          step->offset[xx] = size[xx]/2 + margin.value(xx);
        break;
        case Right:
        case Bottom:
          step->offset[xx] = size[xx] + margin.value(xx);
        break;
      }

      y += step->size[yy] + spacing + step->margin.value(yy);
    } else if (list[i]->relativeType == ReserveType) {
      Reserve *reserve = dynamic_cast<Reserve *>(list[i]);
      y += reserve->size[yy];
    }
    size[yy] = y - spacing;
  }
  size[yy] = max;
}

/******************************************************************************
 * Qt Graphics Scene routines
 *****************************************************************************/

void Range::addGraphicsItemsVert(
  int xx,
  int yy,
  Meta *meta,
  PlacementType  parentRelativeType,
  QGraphicsItem *parent)
{
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);
      step->addGraphicsItems(xx+offset[XX],yy+offset[YY],meta,parentRelativeType,parent);
    }
  }
}

void Range::addGraphicsItemsHoriz(
  int xx,
  int yy,
  Meta *meta,
  PlacementType  parentRelativeType,
  QGraphicsItem *parent)
{
  for (int i = 0; i < list.size(); i++) {
    if (list[i]->relativeType == StepType) {
      Step *step = dynamic_cast<Step *>(list[i]);
      step->addGraphicsItems(xx+offset[XX],yy+offset[YY],meta,parentRelativeType,parent);
    }
  }
}
