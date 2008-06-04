
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
 * This class represents one step including a step number, and assembly
 * image, possibly a parts list image and zero or more callouts needed for
 * the step.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/
 
#include <QFileInfo>
#include <QDir>
#include <QFile>

#include "step.h"
#include "ranges.h"
#include "ranges_element.h"
#include "render.h"
#include "callout.h"
#include "calloutbackgrounditem.h"
#include "pointer.h"
#include "pointeritem.h"
#include "pli.h"
#include "numberitem.h"
#include "csiitem.h"
#include "resolution.h"
#include "dependencies.h"
#include "paths.h"

/*********************************************************************
 *
 * Create a new step and remember the meta-command state at the time
 * that it was created.
 *
 ********************************************************************/

Step::Step(
  AbstractRangesElement *_parent,
  int      num,            // step number as seen by the user
  Meta    &meta,           // the current state of the meta-commands
  bool     calledOut_,     // if we're a callout
  bool     multiStep,      // we can't be a multi-step
  Pli     &_pli)
{
  parent = _parent;

  submodelLevel = meta.submodelStack.size();

  calledOut = calledOut_;

  stepNumber.number = num;                                  // record step number

  relativeType            = StepType;
  csiPixmap.relativeType  = CsiType;
  csiPixmap.pixmap        = NULL;
  stepNumber.relativeType = StepNumberType;

  if (calledOut) {
    csiPixmap.margin     = meta.LPub.callout.csi.margin;    // assembly meta's
    csiPixmap.placement  = meta.LPub.callout.csi.placement;
    _pli.margin           = meta.LPub.callout.pli.margin;    // PLI info
    _pli.placement        = meta.LPub.callout.pli.placement;
    stepNumber.placement = meta.LPub.callout.stepNum.placement;
    stepNumber.font      = meta.LPub.callout.stepNum.font.value();
    stepNumber.color     = meta.LPub.callout.stepNum.color.value();
    stepNumber.margin    = meta.LPub.callout.stepNum.margin;
    pliPerStep           = meta.LPub.callout.pli.perStep.value();
  } else if (multiStep) {
    csiPixmap.margin     = meta.LPub.multiStep.csi.margin;  // assembly meta's
    csiPixmap.placement  = meta.LPub.multiStep.csi.placement;
    _pli.margin           = meta.LPub.multiStep.pli.margin;
    _pli.placement        = meta.LPub.multiStep.pli.placement;
    stepNumber.placement = meta.LPub.multiStep.stepNum.placement;
    stepNumber.font      = meta.LPub.multiStep.stepNum.font.value();
    stepNumber.color     = meta.LPub.multiStep.stepNum.color.value();
    stepNumber.margin    = meta.LPub.multiStep.stepNum.margin;
    pliPerStep           = meta.LPub.multiStep.pli.perStep.value();
  } else {
    csiPixmap.margin     = meta.LPub.assem.margin;         // assembly meta's
    csiPixmap.placement  = meta.LPub.assem.placement;
    _pli.margin           = meta.LPub.assem.margin;
    _pli.placement        = meta.LPub.pli.placement;
    stepNumber.font      = meta.LPub.stepNumber.font.value();
    stepNumber.color     = meta.LPub.stepNumber.color.value();
    stepNumber.margin    = meta.LPub.stepNumber.margin;
    stepNumber.placement = meta.LPub.stepNumber.placement;
    stepNumber.margin    = meta.LPub.stepNumber.margin;
    pliPerStep           = false;
  }
  pli = _pli;
}

/* step destructor destroys all callouts */

Step::~Step() {
  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];
    delete callout;
  }
  list.clear();
  if (csiPixmap.pixmap) {
    delete csiPixmap.pixmap;
  }
  pli.clear();
}
/*
 * given a set of parts, generate a CSI
 */

int Step::createCsi(
  QString const     &topLevelFile,
  QString const     &addLine,
  QStringList const &csiParts,  // the partially assembles model
  QPixmap           *pixmap,
  Meta              &meta)
{
  int         modelScale = meta.LPub.assem.modelScale.value();
  int         sn = stepNumber.number;
  QFileInfo   topInfo(topLevelFile);
  QString key = QString("%1_%2_%3_%4_%5_%6_%7")
                        .arg(topInfo.baseName())
                        .arg(csiName())
                        .arg(sn)
                        .arg(meta.LPub.page.size.value(0))
                        .arg(resolution)
                        .arg(resolutionType == DPI ? "DPI" : "DPCM")
                        .arg(modelScale);
  QString fileName = QDir::currentPath() + "/" +
                      Paths::assemDir + "/" + key + ".png";
  QFile csi(fileName);

  bool outOfDate = false;
  
  if (csi.exists()) {
    QDateTime lastModified = QFileInfo(fileName).lastModified();    
    QStringList stack = submodelStack();
    stack << parent->modelName();
    if ( ! isOlder(stack,lastModified)) {
      outOfDate = true;
    }
  }

  if ( ! csi.exists() || outOfDate) {

    int        rc;

    // render the partially assembled model

    rc = renderer->renderCsi(addLine,csiParts, fileName, meta);

    if (rc < 0) {
      return rc;
    }
  } 
  pixmap->load(fileName);
  csiPixmap.size[0] = pixmap->width();
  csiPixmap.size[1] = pixmap->height();

  return 0;
}


/*
 * LPub is able to pack steps together into multi-step pages or callouts.
 *
 * Multiple steps gathered on a page and callouts share a lot of
 * commonality.  They are organized into rows or columns of steps.
 *
 * From this springs two algorithms, the first algorithm is based on
 * similarity between steps, in that that across steps sub-components
 * within steps are placed in sub-columns or sub-rows. This format
 * is common these days in LEGO building instructions.  For lack of
 * a better name, I refer to this modern algorithm as tabular.
 *
 * The other algorithm, which is new to LPub 3, is that of a more
 * free format.
 *
 * These concepts and algorithms are described below.
 *   1. tabular format
 *      a) either Vertically broken down into sub-columns for
 *         csi, pli, stepNumber and/or callouts.
 *      b) or Horizontally broken down into sub-rows for
 *         csi, pli, stepNumber and/or callouts.
 *
 *   2. free form format
 *      a) either Vertically composed into columns of steps
 *      b) or rows of steps
 *
 *      This format does not force PLI's or step numbers
 *      to be organized across steps, but it does force steps themselves
 *      to be organized into columns or rows.
 *
 * The default is tabular format because that is the first algorithm
 * implemented.  This is also the most common algorithm used by LEGO
 * today (2007 AD).
 *
 * The free format is similar to the algorithms used early by LEGO
 * and provides the maximum area compression of building instructions,
 * even if they are possibly harder to follow.
 */

/*
 * the algorithms below implement tabular placement.
 *
 * size - allocate step sub-components into sub-rows or sub-columns.
 * place - determine the rectangle that is needed to totally contain
 *   the subcomponents (CSI, step number, PLI, step-relative callouts.)
 *   Also place the CSI, step number, PLI and step-relative callouts
 *   within the step's rectangle.
 *
 * making all this look nice takes a few passes:
 *   1.  determine the height and width of each step's sub-columns and
 *       sub-rows.
 *   2.  If we're creating a Vertically allocated multi-step or callout
 *       then make all the sub-columns line up.
 *
 *       If we're creating a Horizontally allocated multi-step or callout
 *       them make all the sub-rows line up.
 *
 * from here we've sized each of the steps.
 *
 * From here, we sum up the the height of each column or row, depending on
 * whether we're creating a Vertical or Horizontal multi-step/callout.  We
 * also keep track of the tallest (widest) column/row within the sets of rows,
 * and how wide (tall) the multi-step/callout is.
 *
 * Now we know the enclosing rectangle for the entire multi-step or callout.
 * Given this we can place the multi-step or callout conglomeration relative
 * to the thing they are to be placed next to.
 *
 * Multi-steps can only be placed relative to the page.
 *
 * Callouts can be place relative to CSI, PLI, step-number, multi-step, or
 * page.
 */

/*
 * Size the set of ranges by sizing each range
 * and then placing them relative to each other
 */

/*
 * Think of the possible placement as a two dimensional table, of
 * places where something can be placed within a step's rectangle.
 *
 *  CCCCCCCCCCC
 *  CSSSSSSSSSC
 *  CSCCCCCCCSC
 *  CSCPPPPPCSC
 *  CSCPCCCPCSC
 *  CSCPCACPCSC
 *  CSCPCCCPCSC
 *  CSCPPPPPCSC
 *  CSCCCCCCCSC
 *  CSSSSSSSSSC
 *  CCCCCCCCCCC
 *
 *  The table below represents either the Horizontal slice
 *  going through the CSI (represented by A for assembly),
 *  or the Vertical slice going through the CSI.
 *
 *  C0 - callout relative to step number
 *  S - step number relative to csi
 *  C1 - callout relative to PLI
 *  P - pli relative to csi
 *  C2 - callout relative to csi
 *  A - csi
 *  C3 - callout relative to csi
 *  P - pli relative to csi
 *  C4 - callout relative to PLI
 *  S - step number relative to csi
 *  C5 - callout relative to step number
 */

/*
 * this tells us where to place the stepNumber when placing
 * relative to csi
 */

const int stepNumberPlace[NumPlaces][2] =
{
  { TblSn0, TblSn0 }, // TopLeft
  { TblCsi,  TblSn0 }, // Top
  { TblSn1, TblSn0 }, // TopRight
  { TblSn1, TblCsi  }, // Right
  { TblSn1, TblSn1 }, // BOT_RIGHT
  { TblCsi,  TblSn1 }, // BOT
  { TblSn0, TblSn1 }, // BOT_LEFT
  { TblSn0, TblCsi  }, // Left
};

/*
 * this tells us where to place the pli when placing
 * relative to csi
 */

const int pliPlace[NumPlaces][2] =
{
  { TblPli0, TblPli0 }, // TopLeft
  { TblCsi,   TblPli0 }, // Top
  { TblPli1, TblPli0 }, // TopRight
  { TblPli1, TblCsi   }, // Right
  { TblPli1, TblPli1 }, // BOT_RIGHT
  { TblCsi,   TblPli1 }, // BOT
  { TblPli0, TblPli1 }, // BOT_LEFT
  { TblPli0, TblCsi   }, // Left
};

/*
 * this tells us where to place a callout when placing
 * relative to csi
 */

const int coPlace[NumPlaces][2] =
{
  { TblCo2, TblCo2 }, // TopLeft
  { TblCsi,  TblCo2 }, // Top
  { TblCo3, TblCo2 }, // TopRight
  { TblCo3, TblCsi  }, // Right
  { TblCo3, TblCo3 }, // BOT_RIGHT
  { TblCsi,  TblCo3 }, // BOT
  { TblCo2, TblCo3 }, // BOT_LEFT
  { TblCo2, TblCsi  }, // Left
};

/*
 * this tells us the row/col offset when placing
 * relative to something other than csi
 */

const int relativePlace[NumPlaces][2] =
{
  { -1, -1 },
  {  0, -1 },
  {  1, -1 },
  {  1,  0 },
  {  1,  1 },
  {  0,  1 },
  { -1,  1 },
  { -1,  0 },
};

void maxMargin(
  MarginsMeta &margin,
  int tbl[2],
  int marginRows[][2],
  int marginCols[][2])
{
  if (margin.value(XX) > marginCols[tbl[XX]][0]) {
    marginCols[tbl[XX]][0] = margin.value(XX);
  }
  if (margin.value(XX) > marginCols[tbl[XX]][1]) {
    marginCols[tbl[XX]][1] = margin.value(XX);
  }
  if (margin.value(YY) > marginRows[tbl[YY]][0]) {
    marginRows[tbl[YY]][0] = margin.value(YY);
  }
  if (margin.value(YY) > marginRows[tbl[YY]][1]) {
    marginRows[tbl[YY]][1] = margin.value(YY);
  }
}

/*
 * This is the first pass of sizing a step.
 *
 *   locate the proper row/col in the placement table (see above)
 *   for each component (csi, pli, stepNumber, callout) in the step
 *
 *     locate the proper row/col for those relative to CSI (absolute)
 *
 *     locate the proper row/col for those relative to (pli,stepNumber)
 *
 *   determine the largest dimensions for each row/col in the table
 *
 *   record the height of this step
 *
 *   determine the pixel offset for each row/col in the table
 *
 *   place the components Vertically in pixel units using row
 */

int Step::sizeitVert(
  int  rows[],         // accumulate sub-row heights here
  int  cols[],         // accumulate sub-col widths here
  int  marginRows[][2],// accumulate sub-row margin heights here
  int  marginCols[][2])// accumulate sub-col margin widths here
{

  // size up each callout

  for (int i = 0; i < list.size(); i++) {
    list[i]->sizeIt();
  }

  // size up the step number

  stepNumber.sizeit();

  /****************************************************/
  /* figure out who is placed in which row and column */
  /****************************************************/

  csiPixmap.tbl[XX] = TblCsi;
  csiPixmap.tbl[YY] = TblCsi;

  /* Lets start with the absolutes (those relative to the CSI) */

  PlacementData pliPlacement = pli.placement.value();

  if (pliPlacement.relativeTo == CsiType) {
    if (pliPlacement.preposition == Outside) {
      pli.tbl[XX] = pliPlace[pliPlacement.placement][XX];
      pli.tbl[YY] = pliPlace[pliPlacement.placement][YY];
    } else {
      pli.tbl[XX] = TblCsi;
      pli.tbl[YY] = TblCsi;
    }
  }

  PlacementData stepNumberPlacement = stepNumber.placement.value();

  if (stepNumberPlacement.relativeTo == PartsListType && ! pliPerStep) {
    stepNumberPlacement.relativeTo = CsiType;
  }

  if (stepNumberPlacement.relativeTo == CsiType) {
    if (stepNumberPlacement.preposition == Outside) {
      stepNumber.tbl[XX] = stepNumberPlace[stepNumberPlacement.placement][XX];
      stepNumber.tbl[YY] = stepNumberPlace[stepNumberPlacement.placement][YY];
    } else {
      stepNumber.tbl[XX] = TblCsi;
      stepNumber.tbl[YY] = TblCsi;
    }
  }

  /* Now lets place things relative to others row/columns */

  /* first the known entities */

  if (pliPlacement.relativeTo == StepNumberType) {
    if (pliPerStep) {
      pli.tbl[XX] = stepNumber.tbl[XX]+relativePlace[pliPlacement.placement][XX];
      pli.tbl[YY] = stepNumber.tbl[YY]+relativePlace[pliPlacement.placement][YY];
    } else {
      stepNumber.tbl[XX] = stepNumberPlace[stepNumberPlacement.placement][XX];
      stepNumber.tbl[YY] = stepNumberPlace[stepNumberPlacement.placement][YY];
    }
  }

  if (stepNumberPlacement.relativeTo == PartsListType) {
    stepNumber.tbl[XX] = 
      pli.tbl[XX]+relativePlace[stepNumberPlacement.placement][XX];
    stepNumber.tbl[YY] = 
      pli.tbl[YY]+relativePlace[stepNumberPlacement.placement][YY];
  }

  /* now place the callouts relative to the known (CSI, PLI, SN) */

  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];

    PlacementData calloutPlacement = callout->placement.value();

    int rp = calloutPlacement.placement;
    switch (calloutPlacement.relativeTo) {
      case CsiType:
        callout->tbl[XX] = coPlace[rp][XX];
        callout->tbl[YY] = coPlace[rp][YY];
      break;
      case PartsListType:
        callout->tbl[XX] = pli.tbl[XX] + relativePlace[rp][XX];
        callout->tbl[YY] = pli.tbl[YY] + relativePlace[rp][YY];
      break;
      case StepNumberType:
        callout->tbl[XX] = stepNumber.tbl[XX] + relativePlace[rp][XX];
        callout->tbl[YY] = stepNumber.tbl[YY] + relativePlace[rp][YY];
      break;
      default:
      break;
    }
  }

  /************************************************/
  /* Determine the biggest in each column and row */
  /************************************************/

  if (csiPixmap.size[XX] > cols[TblCsi]) {
    cols[TblCsi] = csiPixmap.size[XX];
  }
  if (csiPixmap.size[YY] > rows[TblCsi]) {
    rows[TblCsi] = csiPixmap.size[YY];
  }

  maxMargin(csiPixmap.margin,csiPixmap.tbl,marginRows,marginCols);

  if (pli.size[XX] > cols[pli.tbl[XX]]) {
    cols[pli.tbl[XX]] = pli.size[XX];
  }
  if (pli.size[YY] > rows[pli.tbl[YY]]) {
    rows[pli.tbl[YY]] = pli.size[YY];
  }

  maxMargin(pli.margin,pli.tbl,marginRows,marginCols);

  if (stepNumber.size[XX] > cols[stepNumber.tbl[XX]]) {
    cols[stepNumber.tbl[XX]] = stepNumber.size[XX];
  }
  if (stepNumber.size[YY] > rows[stepNumber.tbl[YY]]) {
    rows[stepNumber.tbl[YY]] = stepNumber.size[YY];
  }
 
  maxMargin(stepNumber.margin,stepNumber.tbl,marginRows,marginCols);

  /******************************************************************/
  /* Determine col/row and margin for each callout that is relative */
  /* to step components (e.g. not page or multiStep)               */
  /******************************************************************/

  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];

    PlacementData calloutPlacement = callout->placement.value();

    switch (calloutPlacement.relativeTo) {
      case CsiType:
      case PartsListType:
      case StepNumberType:

        if (callout->size[XX] > cols[callout->tbl[XX]]) {
          cols[callout->tbl[XX]] = callout->size[XX];
        }
        if (callout->size[YY] > rows[callout->tbl[YY]]) {
          rows[callout->tbl[YY]] = callout->size[YY];
        }

        maxMargin(callout->margin,
                   callout->tbl,
                   marginRows,
                   marginCols);
      break;
      default:
      break;
    }
  }

  return 0;
}

void Step::vertMargin(int &top, int &bot)
{
  top = csiPixmap.margin.value(YY);
  bot = top;
  int top_tbl = TblCsi;
  int bot_tbl = TblCsi;

  if (stepNumber.tbl[YY] < TblCsi) {
    top = stepNumber.margin.value(YY);
    top_tbl = stepNumber.tbl[YY];
  } else if (stepNumber.tbl[YY] == TblCsi) {
    int margin = stepNumber.margin.value(YY);
    top = qMax(top,margin);
    bot = qMax(bot,margin);
  } else {
    bot = stepNumber.margin.value(YY);
    bot_tbl = stepNumber.tbl[YY];
  }

  if (pli.size[YY]) {
    if (pli.tbl[YY] < TblCsi) {
      top = pli.margin.value(YY);
      top_tbl = pli.tbl[YY];
    } else if (stepNumber.tbl[YY] == TblCsi) {
      int margin = pli.margin.value(YY);
      top = qMax(top,margin);
      bot = qMax(bot,margin);
    } else {
      bot = pli.margin.value(YY);
      bot_tbl = pli.tbl[YY];
    }
  }

  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];
    if (callout->tbl[YY] < TblCsi) {
      top = callout->margin.value(YY);
      top_tbl = callout->tbl[YY];
    } else if (stepNumber.tbl[YY] == TblCsi) {
      int margin = callout->margin.value(YY);
      top = qMax(top,margin);
      bot = qMax(bot,margin);
    } else {
      bot = callout->margin.value(YY);
      bot_tbl = callout->tbl[YY];
    }
  }
}

/***************************************************************************
 * This routine is used for tabular multi-steps.  It is used to determine
 * the location of csi, pli, stepNumber, and step relative callouts.
 ***************************************************************************/

void Step::placeitVert(
  int rows[],
  int rowsMargin[][2],
  int y)
{
  /*********************************/
  /* determine the margins needed  */
  /*********************************/

  int margins[NumPlaces];

  for (int i = 0; i < NumPlaces; i++) {
    margins[i] = 0;
  }

  for (int i = TblSn0; i < TblCo5; i++) {
    if (rows[i]) {
      for (int j = i + 1; j < NumPlaces; j++) {
        if (rows[j]) {
          margins[i] = qMax(rowsMargin[i][1],rowsMargin[j][0]);
        }
      }
    }
  }

  /*********************************/
  /* record the origin of each row */
  /*********************************/

  int origins[NumPlaces];

  int origin = 0;

  for (int i = 0; i < NumPlaces; i++) {
    origins[i] = origin;

    if (rows[i]) {
      origin += rows[i] + margins[i];
    }
  }

  size[y] = origin;

  /*******************************************/
  /* Now place the components in pixel units */
  /*******************************************/

  csiPixmap.offset[y] = origins[TblCsi] + (rows[TblCsi] - csiPixmap.size[y])/2;

  pli.offset[y] = origins[pli.tbl[y]];
  stepNumber.offset[y]  = origins[stepNumber.tbl[y]];

  switch (y) {
    case XX:
      pli.justifyX(origins[pli.tbl[y]],rows[pli.tbl[y]]);
      stepNumber.justifyX(origins[stepNumber.tbl[y]],rows[stepNumber.tbl[y]]);
    break;
    case YY:
      pli.justifyY(origins[pli.tbl[y]],rows[pli.tbl[y]]);
      stepNumber.justifyY(origins[stepNumber.tbl[y]],rows[stepNumber.tbl[y]]);
    break;
  }

  /* place the callouts that are relative to step components */

  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];

    PlacementData calloutPlacement = callout->placement.value();

    switch (calloutPlacement.relativeTo) {
      case CsiType:
      case PartsListType:
      case StepNumberType:
        callout->offset[y] = origins[callout->tbl[y]];
        if (y == XX) {
          callout->justifyX(origins[callout->tbl[y]],
                               rows[callout->tbl[y]]);
        } else {
          callout->justifyY(origins[callout->tbl[y]],
                               rows[callout->tbl[y]]);
        }
      break;
      default:
      break;
    }
  }
}

int Step::sizeitHoriz(
  int  rows[],         // accumulate sub-row heights here
  int  cols[],         // accumulate sub-col widths here
  int  marginRows[][2],// accumulate sub-row margin heights here
  int  marginCols[][2])// accumulate sub-col margin widths here
{

  // size up each callout

  for (int i = 0; i < list.size(); i++) {
    list[i]->sizeIt();
  }

  // size up the step number

  stepNumber.sizeit();

  /****************************************************/
  /* figure out who is placed in which row and column */
  /****************************************************/

  /* Lets start with the absolutes (those relative to the CSI) */

  PlacementData pliPlacement = pli.placement.value();

  if (pliPlacement.relativeTo == CsiType) {
    if (pliPlacement.preposition == Outside) {
      pli.tbl[XX] = pliPlace[pliPlacement.placement][XX];
      pli.tbl[YY] = pliPlace[pliPlacement.placement][YY];
    } else {
      pli.tbl[XX] = TblCsi;
      pli.tbl[YY] = TblCsi;
    }
  }

  PlacementData stepNumberPlacement = stepNumber.placement.value();

  if (stepNumberPlacement.relativeTo == PartsListType && ! pliPerStep) {
    stepNumberPlacement.relativeTo = CsiType;
  }

  if (stepNumberPlacement.relativeTo == CsiType) {
    if (stepNumberPlacement.preposition == Outside) {
      stepNumber.tbl[XX] = stepNumberPlace[stepNumberPlacement.placement][XX];
      stepNumber.tbl[YY] = stepNumberPlace[stepNumberPlacement.placement][YY];
    } else {
      stepNumber.tbl[XX] = TblCsi;
      stepNumber.tbl[YY] = TblCsi;
    }
  }

  /* Now lets place the relative to others row/columns */

  /* first the known entities */

  if (pliPlacement.relativeTo == StepNumberType) {
    if (pliPerStep) {
      pli.tbl[XX] = stepNumber.tbl[XX] + relativePlace[pliPlacement.placement][XX];
      pli.tbl[YY] = stepNumber.tbl[YY] + relativePlace[pliPlacement.placement][YY];
    } else {
      stepNumber.tbl[XX] = stepNumberPlace[stepNumberPlacement.placement][XX];
      stepNumber.tbl[YY] = stepNumberPlace[stepNumberPlacement.placement][YY];
    }
  }

  if (stepNumberPlacement.relativeTo == PartsListType) {
    stepNumber.tbl[XX] = 
      pli.tbl[XX] + relativePlace[stepNumberPlacement.placement][XX];
    stepNumber.tbl[YY] = 
      pli.tbl[YY] + relativePlace[stepNumberPlacement.placement][YY];
  }

  /* now place the callouts relative to the known (CSI, PLI, SN) */

  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];

    PlacementData calloutPlacement = callout->placement.value();

    int rp = calloutPlacement.placement;
    switch (calloutPlacement.relativeTo) {
      case CsiType:
        callout->tbl[XX] = coPlace[rp][XX];
        callout->tbl[YY] = coPlace[rp][YY];
      break;
      case PartsListType:
        callout->tbl[XX] = pli.tbl[XX] + relativePlace[rp][XX];
        callout->tbl[YY] = pli.tbl[YY] + relativePlace[rp][YY];
      break;
      case StepNumberType:
        callout->tbl[XX] = stepNumber.tbl[XX] + relativePlace[rp][XX];
        callout->tbl[YY] = stepNumber.tbl[YY] + relativePlace[rp][YY];
      break;
      default:
      break;
    }
  }

  /************************************************/
  /* Determine the biggest in each column and row */
  /************************************************/

  if (csiPixmap.size[XX] > cols[TblCsi]) {
    cols[TblCsi] = csiPixmap.size[XX];
  }
  if (csiPixmap.size[YY] > rows[TblCsi]) {
    rows[TblCsi] = csiPixmap.size[YY];
  }

  maxMargin(csiPixmap.margin,csiPixmap.tbl,marginRows,marginCols);

  if (pli.size[XX] > cols[pli.tbl[XX]]) {
    cols[pli.tbl[XX]] = pli.size[XX];
  }
  if (pli.size[YY] > rows[pli.tbl[YY]]) {
    rows[pli.tbl[YY]] = pli.size[YY];
  }

  maxMargin(pli.margin,pli.tbl,marginRows,marginCols);

  if (stepNumber.size[XX] > cols[stepNumber.tbl[XX]]) {
    cols[stepNumber.tbl[XX]] = stepNumber.size[XX];
  }
  if (stepNumber.size[YY] > rows[stepNumber.tbl[YY]]) {
    rows[stepNumber.tbl[YY]] = stepNumber.size[YY];
  }

  maxMargin(stepNumber.margin,stepNumber.tbl,marginRows,marginCols);

  /******************************************************************/
  /* Determine col/row and margin for each callout that is relative */
  /* to step components (e.g. not page or multiStep)               */
  /******************************************************************/

  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];

    PlacementData calloutPlacement = callout->placement.value();

    switch (calloutPlacement.relativeTo) {
      case CsiType:
      case PartsListType:
      case StepNumberType:

        if (callout->size[XX] > cols[callout->tbl[XX]]) {
          cols[callout->tbl[XX]] = callout->size[XX];
        }
        if (callout->size[YY] > rows[callout->tbl[YY]]) {
          rows[callout->tbl[YY]] = callout->size[YY];
        }

        maxMargin(callout->margin,
                   callout->tbl,
                   marginRows,
                   marginCols);
      break;
      default:
      break;
    }
  }

  return 0;
}

/***************************************************************************
 * This routine is used for tabular multi-steps.  It is used to determine
 * the location of csi, pli, stepNumber, and step relative callouts.
 *
 * It is used for both X and Y dimensions (based on input y)
 ***************************************************************************/

void Step::placeitHoriz(
  int rows[],
  int rowsMargin[][2],
  int x) // y can really be either XX or YY
{
  /*********************************/
  /* determine the margins needed  */
  /*********************************/

  int margins[NumPlaces];

  for (int i = 0; i < NumPlaces; i++) {
    margins[i] = 0;
  }

  for (int i = TblSn0; i < TblCo5; i++) {
    if (rows[i]) {
      for (int j = i + 1; j < NumPlaces; j++) {
        if (rows[j]) {
          margins[i] = qMax(rowsMargin[i][1],rowsMargin[j][0]);
        }
      }
    }
  }

  /*********************************/
  /* record the origin of each row */
  /*********************************/

  int origins[NumPlaces];

  int origin = 0;

  for (int i = 0; i < NumPlaces; i++) {
    origins[i] = origin;
    if (rows[i]) {
      origin += rows[i] + margins[i];
    }
  }

  size[x] = origin;

  /*******************************************/
  /* Now place the components in pixel units */
  /*******************************************/

  csiPixmap.offset[x] = origins[TblCsi] + (rows[TblCsi] - csiPixmap.size[x])/2;
  pli.offset[x] = origins[pli.tbl[x]];
  stepNumber.offset[x]  = origins[stepNumber.tbl[x]];

  switch (x) {
    case XX:
      pli.justifyX(origins[pli.tbl[x]],rows[pli.tbl[x]]);
      stepNumber.justifyX(origins[stepNumber.tbl[x]],rows[stepNumber.tbl[x]]);
    break;
    case YY:
      pli.justifyY(origins[pli.tbl[x]],rows[pli.tbl[x]]);
      stepNumber.justifyY(origins[stepNumber.tbl[x]],rows[stepNumber.tbl[x]]);
    break;
    default:
    break;
  }

  /* place the callouts that are relative to step components */

  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];
    PlacementData calloutPlacement = callout->placement.value();
    switch (calloutPlacement.relativeTo) {
      case CsiType:
      case PartsListType:
      case StepNumberType:
        callout->offset[x] = origins[callout->tbl[x]];
        if (x == XX) {
          callout->justifyX(origins[callout->tbl[x]],
                             rows[callout->tbl[x]]);
        } else {
          callout->justifyY(origins[callout->tbl[x]],
                             rows[callout->tbl[x]]);
        }
      break;
      default:
      break;
    }
  }
}

/*
 * This method is independent of Horizontal/Vertical multi-step/callout
 * allocation, or tabular vs. freeform mode.
 */

void Step::addGraphicsItems(
  int             offsetX,
  int             offsetY,
  Meta           *meta,
  PlacementType   parentRelativeType,
  QGraphicsItem  *parent)
{
  offsetX += offset[XX];
  offsetY += offset[YY];

  if (csiPixmap.pixmap) {
    CsiItem *csiItem = new CsiItem(this,
                                   meta,
                                  *csiPixmap.pixmap, 
                                   context,
                                   submodelLevel,
                                   parent,
                                   parentRelativeType);
    csiItem->setPos(offsetX + csiPixmap.offset[XX],
                    offsetY + csiPixmap.offset[YY]);
  }
  if (pli.tsize()) {
    pli.addPli(meta, submodelLevel, parent);
    pli.setPos(offsetX + pli.offset[XX],
               offsetY + pli.offset[YY]);
  }

  if (stepNumber.number > 0) {
    StepNumberItem *sn; 
    if (calledOut) {
      sn = new StepNumberItem(parentRelativeType,
                              meta->context.topOfRanges(),
                              meta->context.bottomOfRanges(),
                              meta,meta->LPub.callout.stepNum,
                              "%d", 
                              stepNumber.number,
                              parent);
    } else {
      sn = new StepNumberItem(parentRelativeType,
                              meta->context.topOfRanges(),
                              meta->context.bottomOfRanges(),
                              meta,
                              meta->LPub.multiStep.stepNum,
                              "%d", 
                              stepNumber.number,
                              parent);
    }
    sn->setPos(offsetX + stepNumber.offset[XX],
               offsetY + stepNumber.offset[YY]);
  }

  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];
    PlacementData placementData = callout->placement.value();
    QRect rect(csiPixmap.offset[XX],
               csiPixmap.offset[YY],
               csiPixmap.size[XX],
               csiPixmap.size[YY]);

    switch (placementData.relativeTo) {
      case CsiType:
      case PartsListType:
      case StepNumberType:
        callout->addGraphicsItems(offsetX,offsetY,rect,parent);
        for (int i = 0; i < callout->pointerList.size(); i++) {
          Pointer *pointer = callout->pointerList[i];
          callout->addGraphicsPointerItem(offsetX+callout->offset[XX],
                                          offsetY+callout->offset[YY],
                                          offsetX,
                                          offsetY,
                                          csiPixmap.offset,
                                          csiPixmap.size,
                                          pointer,
                                          callout->background);
        }
      break;

      default:
        callout->addGraphicsItems(0,0,rect,parent);
        for (int i = 0; i < callout->pointerList.size(); i++) {
          Pointer *pointer = callout->pointerList[i];
          callout->addGraphicsPointerItem(0,
                                          0,
                                          0,
                                          0,
                                          csiPixmap.offset,
                                          csiPixmap.size,
                                          pointer,
                                          callout->background);
        }
      break;
    }
  }
}

void Step::placeInside()
{
  if (pli.placement.value().preposition == Inside) {
    switch (pli.placement.value().relativeTo) {
      case CsiType:
        csiPixmap.placeRelative(&pli);
      break;
      case PartsListType:
      break;
      case StepNumberType:
        stepNumber.placeRelative(&pli);
      break;
      default:
      break;
    }
  }
  if (stepNumber.placement.value().preposition == Inside) {
    switch (pli.placement.value().relativeTo) {
      case CsiType:
        csiPixmap.placeRelative(&stepNumber);
      break;
      case PartsListType:
        pli.placeRelative(&stepNumber);
      break;
      case StepNumberType:
      break;
      default:
      break;
    }
  }

  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];
    PlacementData placementData = callout->placement.value();

    /* Offset Callouts */

    int relativeToSize[2];

    relativeToSize[XX] = 0;
    relativeToSize[YY] = 0;

    switch (placementData.relativeTo) {
      case CsiType:
        relativeToSize[XX] = csiPixmap.size[XX];
        relativeToSize[YY] = csiPixmap.size[YY];
      break;
      case PartsListType:
        relativeToSize[XX] = pli.size[XX];
        relativeToSize[YY] = pli.size[YY];
      break;
      case StepNumberType:
      break;
      default:
      break;
    }
    callout->offset[XX] += relativeToSize[XX]*placementData.offsets[XX];
    callout->offset[YY] += relativeToSize[YY]*placementData.offsets[YY];
  }
}

/*********************************************************************
 *
 * This section implements a second, more freeform version of packing
 * steps into callouts and multiSteps.
 *
 * Steps being oriented into sub-columns or sub-rows with
 * step columns or rows, this rendering technique dues not necessarily
 * get you the most compact result.
 *
 *  In single step per page the placement algorithm is very flexible.
 * Anything can be placed relative to anything, as long as the placement
 * relationships lead everyone back to the page, then all things will
 * be placed.
 *
 * In free-form placement, some placement component is the root of all
 * placement (CSI, PLI, STEP_NUMBER).  All other placement components
 * are placed relative to the base, or placed relative to things that
 * are placed relative to the root.
 *
 ********************************************************************/

void Step::sizeitFreeform(
  int xx,
  int yy,
  int relativeBase,
  int relativeJustification,
  int &left,
  int &right)
{
  relativeJustification = relativeJustification;
  // size up each callout

  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];
    if (callout->meta.LPub.callout.freeform.value().mode) {
      callout->sizeitFreeform(xx,yy);
    } else {
      callout->sizeIt();
    }
  }

  // place each callout

  for (int i = 0; i < list.size(); i++) {
    Callout *callout = list[i];

    if (callout->meta.LPub.callout.freeform.value().mode) {
      if (callout->meta.LPub.callout.freeform.value().justification == Left ||
          callout->meta.LPub.callout.freeform.value().justification == Top) {
        callout->offset[xx] = callout->size[xx];
      }
    } else {
      callout->sizeIt();
    }
  }

  // size up the step number

  stepNumber.sizeit();

  // place everything relative to the base

  int offsetX = 0, sizeX = 0;

  PlacementData placementData;

  switch (relativeBase) {
    case CsiType:
	  placementData = csiPixmap.placement.value();
	  placementData.relativeTo = PageType;
	  csiPixmap.placement.setValue(placementData);
      csiPixmap.relativeTo(this);
      offsetX = csiPixmap.offset[xx];
      sizeX   = csiPixmap.size[yy];
    break;
    case PartsListType:
	  placementData = pli.placement.value();
	  placementData.relativeTo = PageType;
	  pli.placement.setValue(placementData);
      pli.relativeTo(this);
      offsetX = pli.offset[xx];
      sizeX   = pli.size[yy];
    break;
    case StepNumberType:
	  placementData = stepNumber.placement.value();
	  placementData.relativeTo = PageType;
	  stepNumber.placement.setValue(placementData);
      stepNumber.relativeTo(this);
      offsetX = stepNumber.offset[xx];
      sizeX   = stepNumber.size[xx];
    break;
  }

  // FIXME: when we get here for callouts that are to to the left of the CSI
  // the outermost box is correctly placed, but within there the CSI is
  // in the upper left hand corner, even if it has a callout to the left of
  // it
  //
  // Have to determine the leftmost edge of any callouts
  //   Left of CSI
  //   Left edge of Top|Bottom Center or Right justified - we need place

  // size the step

  for (int dim = XX; dim <= YY; dim++) {

    int min = 500000;
    int max = 0;

    if (csiPixmap.offset[dim] < min) {
      min = csiPixmap.offset[dim];
    }
    if (csiPixmap.offset[dim] + csiPixmap.size[dim] > max) {
      max = csiPixmap.offset[dim] + csiPixmap.size[dim];
    }
    if (pli.offset[dim] < min) {
      min = pli.offset[dim];
    }
    if (pli.offset[dim] + pli.size[dim] > max) {
      max = pli.offset[dim] + pli.size[dim];
    }
    if (stepNumber.offset[dim] < min) {
      min = stepNumber.offset[dim];
    }
    if (stepNumber.offset[dim] + stepNumber.size[dim] > max) {
      max = stepNumber.offset[dim] + stepNumber.size[dim];
    }

    for (int i = 0; i < list.size(); i++) {
      Callout *callout = list[i];
      if (callout->offset[dim] < min) {
        min = callout->offset[dim];
      }
      if (callout->offset[dim] + callout->size[dim] > max) {
        max = callout->offset[dim] + callout->size[dim];
      }
    }

    if (calledOut) {
      csiPixmap.offset[dim] -= min;
      pli.offset[dim]        -= min;
      stepNumber.offset[dim] -= min;

      for (int i = 0; i < list.size(); i++) {
        Callout *callout = list[i];
        callout->offset[dim] -= min;
      }
    }

    size[dim] = max - min;

    if (dim == XX) {
      left = min;
      right = max;
    }
  }

  /* Now make all things relative to the base */

  csiPixmap.offset[xx]  -= offsetX + sizeX;
  pli.offset[xx]        -= offsetX + sizeX;
  stepNumber.offset[xx] -= offsetX + sizeX;

  for (int i = 0; i < list.size(); i++) {
    list[i]->offset[xx] -= offsetX + sizeX;
  }
}
