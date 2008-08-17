
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
 * This file describes a class that is used to implement backannotation
 * of user Gui input into the LDraw file.  Furthermore it implements
 * some functions to provide higher level editing capabilities, such 
 * as adding and removing steps from step groups, adding, moving and
 * deleting dividers.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include <QFont>
#include <QFontDatabase>
#include <QFontDialog>
#include <QColor>
#include <QColorDialog>
#include "metaitem.h"
#include "lpub.h"
#include "color.h"
#include "placementdialog.h"
#include "pliconstraindialog.h"
#include "pairdialog.h"
#include "scaledialog.h"
#include "borderdialog.h"
#include "backgrounddialog.h"
#include "dividerdialog.h"
#include "paths.h"

void MetaItem::setGlobalMeta(
  QString  &topLevelFile,
  LeafMeta *leaf)
{
  QString newMeta = leaf->format(false,true);
  if (leaf->here().modelName != "undefined") {
    replaceMeta(leaf->here(),newMeta);
  } else {
    Where here = sortedGlobalWhere(topLevelFile,newMeta);
    insertMeta(here,newMeta);
  }
}

const QString stepGroupBegin   = "0 !LPUB MULTI_STEP BEGIN";
const QString stepGroupDivider = "0 !LPUB MULTI_STEP DIVIDER";
const QString stepGroupEnd     = "0 !LPUB MULTI_STEP END";
const QString step             = "0 STEP";

Rc MetaItem::scanForwardStepGroup(Where &here, bool &partsAdded)
{
  return scanForward(here,StepMask|StepGroupMask,partsAdded);
}

Rc MetaItem::scanForwardStepGroup(Where &here)
{
  return scanForward(here,StepMask|StepGroupMask);
}

Rc MetaItem::scanBackwardStepGroup(Where &here, bool &partsAdded)
{
  return scanBackward(here,StepMask|StepGroupMask,partsAdded);
}

Rc MetaItem::scanBackwardStepGroup(Where &here)
{
  return scanBackward(here,StepMask|StepGroupMask);
}

//   TOS                   EOS 
//   STEP (END) BEGIN PART STEP (DIVIDER)         PART STEP END
//   STEP (END) xxxxx PART STEP xxxxxxxxx         PART STEP xxx

//   TOS                   EOS 
//   STEP (END) BEGIN PART STEP (DIVIDER)         PART STEP         PART
//   STEP (END) xxxxx PART STEP           (BEGIN) PART STEP         PART

//   TOS                   EOS
//   STEP (END) BEGIN PART STEP (DIVIDER)         PART STEP DIVIDER
//   STEP (END) xxxxx PART STEP           (BEGIN) PART STEP DIVIDER

int MetaItem::removeFirstStep(
  const Where &topOfRanges)
{
  int sum = 0;
  Where secondStep = topOfRanges + 1;         // STEP

  Rc rc = scanForwardStepGroup(secondStep);
  if (rc == StepGroupEndRc) {                 // (END)
    ++secondStep;
    rc = scanForwardStepGroup(secondStep);
  }
  if (rc == StepGroupBeginRc) {               // BEGIN
    Where begin = secondStep++;
    rc = scanForwardStepGroup(secondStep);
    if (rc == StepRc || rc == RotStepRc) {    // STEP
      Where thirdStep = secondStep + 1;
      rc = scanForwardStepGroup(thirdStep);
      Where divider;
      if (rc == StepGroupDividerRc) {         // (DIVIDER)
        divider = thirdStep++;
        rc = scanForwardStepGroup(thirdStep);
      }
      if (rc == StepRc || rc == RotStepRc) {  // STEP
        ++thirdStep;
        rc = scanForwardStepGroup(thirdStep);
        if (rc == StepGroupEndRc) {           // END
          // remove this end and then begin
          deleteMeta(thirdStep);
          if (divider.lineNumber) {
            deleteMeta(divider);
          }
        } else {
          if (divider.lineNumber) {
            replaceMeta(divider,stepGroupBegin);          
          } else {
            appendMeta(secondStep,stepGroupBegin);
          }
        }
        deleteMeta(begin); --sum;
      }
    }
  }
  return sum;
}

void MetaItem::addNextMultiStep(
  const Where &topOfRanges,
  const Where &bottomOfRanges)  // always add at end
{
  Rc rc1;
  
  bool firstChange = true;
  bool partsAdded;
  // TOR            BOR
  // EOF       PART STEP PART EOF
  // EOF BEGIN PART STEP PART     STEP END
  
  // EOF       PART STEP PART     STEP
  // EOF BEGIN PART STEP PART     STEP END
  
  // STEP END PART EOF
  // STEP     PART STEP END
  
  // STEP END PART STEP
  // STEP     PART STEP END

  Where walk = bottomOfRanges + 1;
  rc1 = scanForward(walk,StepMask|StepGroupMask);
  Where end;
  if (rc1 == StepGroupEndRc) {                            // END
    end = walk++;
    rc1 = scanForward(walk,StepMask|StepGroupMask);
  }
  if (rc1 == StepGroupBeginRc) {                          // BEGIN
    firstChange = false;
    beginMacro("addNextStep1");
    removeFirstStep(bottomOfRanges);                      // remove BEGIN
    partsAdded = false;
    rc1 = scanForwardStepGroup(walk,partsAdded);
  }

  // bottomOfRanges - STEP
  // end            - StepGroupEnd
  // walk           - (STEP || EOF)
    
  if (firstChange) {
    beginMacro("addNextStep2");
    firstChange = false;
  }
  if (rc1 == EndOfFileRc && partsAdded) {
    insertMeta(walk,step);
  }
  appendMeta(walk,stepGroupEnd);
  if (end.lineNumber) {
    deleteMeta(end);
  } else {
    walk = topOfRanges + 1;
    rc1 = scanForward(walk,StepMask|StepGroupMask);
    if (rc1 == StepGroupEndRc) {
      appendMeta(walk,stepGroupBegin);
    } else {
      appendMeta(topOfRanges,stepGroupBegin);
    }
  }
  endMacro();
}

//                      TOS
// BEGIN           PART STEP DIVIDER       PART STEP END
// xxxxx           PART STEP xxxxxxx       PART STEP xxx

//                      TOS
// BEGIN PART STEP PART STEP DIVIDER       PART STEP END
// BEGIN PART STEP PART STEP xxxxxxx (END) PART STEP xxx

//                      TOS
// BEGIN PART STEP PART STEP               PART STEP END
// BEGIN PART STEP PART STEP         (END) PART STEP xxx
// $$$$$                $$$  $$$$$$$                 $$$

int MetaItem::removeLastStep(
  const Where &topOfRanges,                       // STEP
  const Where &bottomOfRanges)
{
  int sum = 0;
  
  // Find the top of us
  Where walk = topOfRanges + 1;
  Rc rc = scanForwardStepGroup(walk);
  
  if (rc == StepGroupEndRc) {
    ++walk;
    rc = scanForwardStepGroup(walk);
  }

  Where begin = walk;
  if (rc == StepGroupBeginRc) {                   // BEGIN
    
    // Find the next step
    Where secondStep = walk + 1;
    Rc rc = scanForward(secondStep,StepMask);

    // find the next next step
    if (rc == StepRc || rc == RotStepRc) {        // STEP
      Where thirdStep = secondStep + 1;
      rc = scanForwardStepGroup(thirdStep);
    
      Where divider;
      if (rc == StepGroupDividerRc) {             // (DIVIDER)
        divider = thirdStep;
        ++thirdStep;
        rc = scanForwardStepGroup(thirdStep);
      }
      
  //-----------------------------------------------------
  
      Where lastStep = bottomOfRanges - 1;
      rc = scanBackward(lastStep,StepMask);
 
      if (rc == StepRc || rc == RotStepRc) {        // STEP
        walk = lastStep + 1;
        rc = scanForward(walk,StepMask|StepGroupMask);
        Where divider2;
        if (rc == StepGroupDividerRc) {             // (DIVIDER)
          divider2 = walk++;
          rc = scanForwardStepGroup(walk);
        }
    
        if (rc == StepRc || rc == RotStepRc) {      // STEP
          ++walk;
          rc = scanForwardStepGroup(walk);

          if (rc == StepGroupEndRc) {               // END
            deleteMeta(walk);
            --sum;
            if (secondStep.lineNumber == lastStep.lineNumber) {
              if (divider.lineNumber) {
                --sum;
                deleteMeta(divider);
              }
              --sum;
              deleteMeta(begin);
            } else if (divider2.lineNumber) {
              replaceMeta(divider2,stepGroupEnd);
            } else {
              ++sum;
              appendMeta(lastStep,stepGroupEnd);
            }
          }
        }
      }
    }
  }
  return sum;
}

//                                                TOS 
// SOF                                                 PART STEP     PART (EOF|STEP)
// SOF                                         (BEGIN) PART STEP     PART     (STEP) (END)

//                      TOS
//           SOF   PART STEP                           PART STEP     PART (EOF|STEP)
//           SOF   PART STEP                   (BEGIN) PART STEP     PART     (STEP) (END)

//                      TOS           
//           BEGIN PART STEP                           PART STEP END PART (EOF|STEP)
//           xxxxx PART STEP                   (BEGIN) PART STEP xxx PART     (STEP) (END)  

//                      TOS
//           BEGIN PART STEP         DIVIDER PART STEP END PART (EOF|STEP)
//           xxxxx PART STEP         (BEGIN) PART STEP xxx PART     (STEP) (END)

//                      TOS
// BEGIN PART STEP PART STEP DIVIDER         PART STEP END PART (EOF|STEP)
// BEGIN PART STEP PART STEP (END)   (BEGIN) PART STEP xxx PART     (STEP) (END)

// BEGIN PART STEP PART STEP                 PART STEP END PART (EOF|STEP)
// BEGIN PART STEP PART STEP (END)   (BEGIN) PART STEP xxx PART     (STEP) (END)

void MetaItem::addPrevMultiStep(
  const Where &topOfRangesIn,
  const Where &bottomOfRangesIn)
{
  Rc rc1;

  bool  firstChange = true;
  bool  partsAdded;
  Where topOfRanges = topOfRangesIn;
  Where bottomOfRanges = bottomOfRangesIn;

  Where walk = topOfRanges + 1;
  rc1 = scanForward(walk,StepMask|StepGroupMask,partsAdded);
    
    //                           TOS  end begin                BOR
    // STEP BEGIN PART STEP PART STEP END BEGIN PART STEP PART STEP END

  Where begin;
  
  if (rc1 == StepGroupEndRc) {                        // END
    beginMacro("AddPreviousStep");
    firstChange = false;
      
    Where prevTopOfRanges = topOfRanges-1;
    scanBackward(prevTopOfRanges,StepGroupBeginRc);
    --prevTopOfRanges;
    scanBackward(prevTopOfRanges,StepMask);
    int removed = removeLastStep(prevTopOfRanges,topOfRanges); // shift by removal count
    topOfRanges.lineNumber += removed + 1;
    bottomOfRanges.lineNumber += removed;
    walk = topOfRanges + 1;                            // skip past 
    rc1 = scanForwardStepGroup(walk, partsAdded);
  }
  
  // if I'm the first step of an existing step group

  if (rc1 == StepGroupBeginRc) {                       // BEGIN
    begin = walk++;                                    // remember
    rc1 = scanForward(walk,StepMask,partsAdded);       // next?
  }
    
  //           PREV           TOS  begin                BOR
  // STEP PART STEP END       PART STEP BEGIN PART STEP PART STEP END
  // STEP PART STEP END BEGIN PART STEP       PART STEP PART STEP END

  if (firstChange) {
    beginMacro("AddPreviousStep2");
    firstChange = false;
  }
    
  // Handle end of step/group
  if (rc1 == EndOfFileRc && partsAdded) {  
    insertMeta(walk,step);
  }

  if (begin.lineNumber == 0) {
    appendMeta(walk,stepGroupEnd);
  }
  
  Where prevStep = topOfRanges - 1;
  scanBackward(prevStep,StepMask);

  if (begin.lineNumber) {
    deleteMeta(begin);
    
    Where end = prevStep+1;    
    rc1 = scanForwardStepGroup(end);
    prevStep.lineNumber += rc1 == StepGroupEndRc;
  }
  
  appendMeta(prevStep,stepGroupBegin);

  gui->displayPageNum--;
  if ( ! firstChange) {
    endMacro();
  }
}

void MetaItem::deleteFirstMultiStep(
  const Where &topOfRanges)
{
  beginMacro("removeFirstStep");
  removeFirstStep(topOfRanges);
  endMacro();
}

void MetaItem::deleteLastMultiStep(
  const Where &topOfRanges,
  const Where &bottomOfRanges)
{
  beginMacro("deleteLastMultiStep");
  removeLastStep(topOfRanges,bottomOfRanges);
  endMacro();
}

void MetaItem::addDivider(
  PlacementType parentRelativeType,
  const Where &bottomOfStep,
  RcMeta *divider)
{
  Where   walk = bottomOfStep;
  Rc      rc;
  QString divString = divider->preamble;
  int     mask = parentRelativeType == StepGroupType ? StepMask|StepGroupMask : StepMask|CalloutMask;

  rc = scanForward(walk, mask);

  if (rc == StepRc || rc == RotStepRc) {
    // we can add a divider after meta->context.curStep().lineNumber    

    appendMeta(bottomOfStep,divString); 
  }
}

void MetaItem::deleteDivider(
  PlacementType parentRelativeType, 
  const Where &dividerIn)
{
  Where divider = dividerIn;
  
  if (divider.modelName != "undefined") {
    Rc rc;
    int mask = parentRelativeType == StepGroupType ? StepMask|StepGroupMask : StepMask|CalloutMask;
    Rc expRc = parentRelativeType == StepGroupType ? StepGroupDividerRc : CalloutDividerRc;
    
    ++divider;
    rc = scanForward(divider,mask);

    if (rc == expRc) {
      beginMacro("deleteDivider");
      deleteMeta(divider);
      endMacro();
    }
  }
}

/***********************************************************************
 *
 * These "short-cuts" move steps to the other side of a divider
 *
 **********************************************************************/

const QString calloutDivider = "0 !LPUB CALLOUT DIVIDER";

void MetaItem::addToNext(
  PlacementType parentRelativeType,
  const Where &topOfStep)
{
  if (parentRelativeType == CalloutType) {
    calloutAddToNext(topOfStep);
  } else {
    stepGroupAddToNext(topOfStep);
  }
}

void MetaItem::calloutAddToNext(
  const Where &topOfStep)
{
  Where walk = topOfStep + 1;
  Rc rc = scanForward(walk,StepMask);
  if (rc == StepRc || rc == RotStepRc) {
    ++walk;
    rc = scanForward(walk,StepMask|CalloutMask);
    if (rc == CalloutDividerRc) {
      Where test = topOfStep - 1;
      Rc rc2 = scanBackward(test,StepMask|CalloutMask);
      beginMacro("moveStepPrev");
      deleteMeta(walk);
      if (rc != EndOfFileRc && rc != CalloutDividerRc && rc2 != EndOfFileRc) {
        appendMeta(topOfStep,calloutDivider);
      }
      endMacro();
    }
  }
}

void MetaItem::stepGroupAddToNext(
  const Where &topOfStep)
{
  Where walk = topOfStep + 1;
  Rc rc = scanForward(walk,StepMask|StepGroupMask);
  Where divider;
  if (rc == StepGroupBeginRc || rc == StepGroupDividerRc) {
    divider = walk++;
    rc = scanForward(walk,StepMask|StepGroupMask);
  }
  if (rc == StepRc || rc == RotStepRc) {
    ++walk;
    rc = scanForward(walk,StepMask|StepGroupMask);
    if (rc == StepGroupDividerRc) {
      beginMacro("moveStepPrev");
      deleteMeta(walk);
      if (divider.lineNumber == 0) {
        appendMeta(topOfStep,stepGroupDivider);
      }
      endMacro();
    }
  }
}

void MetaItem::addToPrev(
  PlacementType parentRelativeType,
  const Where &topOfStep)
{
  if (parentRelativeType == CalloutType) {
    calloutAddToPrev(topOfStep);
  } else {
    stepGroupAddToPrev(topOfStep);
  }
}

void MetaItem::calloutAddToPrev(
  const Where &topOfStep)
{  
  Where walk = topOfStep + 1;
  Rc rc;
    
  //   1      2         3*
  // TOS PART STEP PART STEP DIVIDER PART EOF
  // TOS PART STEP PART STEP         PART EOF

  //     1    2                 3    
  // SOF PART STEP DIVIDER PART STEP         PART EOF
  // SOF PART STEP         PART STEP DIVIDER PART EOF

  rc = scanForward(walk,StepMask|CalloutMask);
  if (rc == CalloutDividerRc) {
    Where divider = walk++;
    rc = scanForward(walk,StepMask|CalloutMask);
    if (rc == StepRc || RotStepRc) {
      ++walk;
      rc = scanForward(walk,StepMask|CalloutMask);
      beginMacro("moveStepNext");
      if (rc != EndOfFileRc && rc != CalloutDividerRc) {
        appendMeta(walk,calloutDivider);
      }
      deleteMeta(divider);
      endMacro();
    }
  }
}

void MetaItem::stepGroupAddToPrev(
  const Where &topOfStep)
{  
  //   1      2         3*
  // TOS PART STEP PART STEP DIVIDER PART EOF
  // TOS PART STEP PART STEP         PART EOF

  //     1    2                 3    
  // SOF PART STEP DIVIDER PART STEP         PART EOF
  // SOF PART STEP         PART STEP DIVIDER PART EOF

  Where walk = topOfStep + 1;                   // STEP
  Rc rc = scanForward(walk,StepMask|StepGroupMask);
  if (rc == StepGroupDividerRc) {               // DIVIDER
    Where divider = walk++;
    rc = scanForward(walk,StepMask|StepGroupMask);
    if (rc == StepRc || rc == RotStepRc) {      // STEP
      Where nextStep = walk++;
      rc = scanForward(walk,StepMask|StepGroupMask);
      beginMacro("moveStepNext");
      if (rc != StepGroupEndRc && rc != StepGroupDividerRc) {
        appendMeta(nextStep,stepGroupDivider);
      }
      deleteMeta(divider);
      endMacro();
    }
  }
}

void MetaItem::convertToIgnore(Meta *meta)
{
  gui->maxPages = -1;

  Where calledOut = meta->submodelStack[meta->submodelStack.size() - 1];
  Where here = calledOut+1;
  beginMacro("ignoreSubmodel");
  insertMeta(here,      "0 !LPUB PART END");
  insertMeta(calledOut, "0 !LPUB PART BEGIN IGNORE");
  endMacro();
}

/*******************************************************************************
 *
 *
 ******************************************************************************/
void MetaItem::setMetaTopOf(
  const Where &topOf,
  const Where &bottomOf,
  LeafMeta    *meta,
  int          append,
  bool         local,
  bool         global)
{
  int  lineNumber = meta->here().lineNumber;
  bool metaInRange;

  metaInRange = meta->here().modelName == topOf.modelName
   && lineNumber >= topOf.lineNumber 
   && lineNumber <= bottomOf.lineNumber;

  if (metaInRange) {
    QString line = meta->format(meta->pushed,meta->global);
    replaceMeta(meta->here(),line);
  } else {
    if (local) {
      local = LocalDialog::getLocal(LPUB, "Change only this step?",gui);
    }
    QString line = meta->format(local,global);

    if (topOf.lineNumber == 0) {
      QString line = gui->readLine(topOf);
      QStringList argv;
      split(line,argv);
      if (argv.size() >= 1 && argv[0] != "0") {
        insertMeta(topOf,"0");
      }
    }

    Where topOfFile = topOf;
    topOfFile.lineNumber += append;
    insertMeta(topOfFile, line);
  }
}

void MetaItem::setMetaBottomOf(
  const Where &topOf,
  const Where &bottomOf,
  LeafMeta    *meta,
  bool         local,
  bool         global)
{
  int  lineNumber = meta->here().lineNumber;
  bool metaInRange;

  metaInRange = meta->here().modelName == topOf.modelName
   && lineNumber >= topOf.lineNumber 
   && lineNumber <= bottomOf.lineNumber;

  if (metaInRange) {
    QString line = meta->format(meta->pushed,meta->global);
    replaceMeta(meta->here(),line);
  } else {
    if (local) {
      local = LocalDialog::getLocal(LPUB, "Change only this step?",gui);
    }
    QString line = meta->format(local, global);

    int numLines = gui->subFileSize(topOf.modelName);
    bool append = bottomOf.lineNumber + 1 == numLines;

    if (append) {
      QString line = gui->readLine(bottomOf);
      QStringList argv;
      split(line,argv);
      if (argv.size() == 2 && argv[0] == "0" 
          && (argv[1] == "STEP" || argv[1] == "ROTSTEP")) {
        append = false;
      }
    }
    if (append) {
      appendMeta(bottomOf,line);
    } else {
      insertMeta(bottomOf, line);
    }
  }
}

void MetaItem::changePlacement(
  PlacementType  parentType,
  PlacementType  relativeType,
  QString        title,
  const Where   &topOfRanges,
  const Where   &bottomOfRanges,
  PlacementMeta *placement,
  int            append,
  bool           useLocal) 
{
  PlacementData placementData = placement->value();
  bool ok;
  ok = PlacementDialog
       ::getPlacement(parentType,relativeType,placementData,title);

  if (ok) {
    placement->setValue(placementData);
    setMetaTopOf(topOfRanges,bottomOfRanges,placement,append,useLocal);
  }
}

void MetaItem::changePlacementOffset(
  Where          defaultWhere,
  PlacementMeta *placement,
  bool           local,
  bool           global)
{
  QString newMetaString = placement->format(local,global);

  if (placement->here().modelName == "undefined") {

    Where walk = defaultWhere + 1;

    bool partsAdded;

    if (scanBackward(walk,StepMask,partsAdded) == EndOfFileRc) {
      Meta tmpMeta;
      defaultWhere = firstLine(defaultWhere.modelName);
    }
    appendMeta(defaultWhere,newMetaString);
  } else {
    replaceMeta(placement->here(),newMetaString);
  }
}

void MetaItem::changeBackground(
  QString         title,
  const Where    &topOfStep,
  const Where    &bottomOfStep,
  BackgroundMeta *background,
  int             append,
  bool            local)
{
  BackgroundData backgroundData = background->value();
  bool ok;
  ok = BackgroundDialog::getBackground(backgroundData,title,gui);

  if (ok) {
    background->setValue(backgroundData);
    setMetaTopOf(topOfStep,bottomOfStep,background,append,local);
  }
}

void MetaItem::changeConstraint(
  QString        title,
  const Where   &topOfStep,
  const Where   &bottomOfStep,
  ConstrainMeta *constraint,
  int            append,
  bool           local)
{
  ConstrainData constrainData = constraint->value();
  bool ok;
  ok = ConstrainDialog::getConstraint(constrainData,title,gui);

  if (ok) {
    constraint->setValueUnit(constrainData);
    setMetaTopOf(topOfStep,bottomOfStep,constraint,append,local);
  }
}

void MetaItem::changeFont(
  const Where   &topOfStep,
  const Where   &bottomOfStep,
  FontMeta      *font,
  int            append,
  bool           local)
{
  bool ok;
  QFont _font;
  QFontDatabase _fdb;

  _font.fromString(font->valueUnit());

  _font = QFontDialog::getFont(&ok,_font);

  if (ok) {
    font->setValueUnit(_font.toString());
    setMetaTopOf(topOfStep,bottomOfStep,font,append,local);
  }
}

void MetaItem::changeColor(
  const Where &topOfStep,
  const Where &bottomOfStep,
  StringMeta *color,
  int         append,
  bool        local)
{
  QColor _color = LDrawColor::color(color->value());
  _color = QColorDialog::getColor(_color,NULL);

  if (_color.isValid()) {
    color->setValue(_color.name());
    setMetaTopOf(topOfStep,bottomOfStep,color,append,local);
  }
}

void MetaItem::changeMargins(
  QString        title,
  const Where   &topOfStep,
  const Where   &bottomOfStep,
  MarginsMeta   *margin,
  int            append,
  bool           local)
{ 
  float values[2];

  values[0] = margin->valueUnit(0);
  values[1] = margin->valueUnit(1);

  bool ok   = UnitsDialog::getUnits(values,title,gui);

  if (ok) {
    margin->setValueUnits(values[0],values[1]);
    setMetaTopOf(topOfStep,bottomOfStep,margin,append,local);
  }
}

void MetaItem::changeUnits(
  QString       title,
  const Where  &topOfStep,
  const Where  &bottomOfStep,
  UnitsMeta    *units,
  int           append,
  bool          local)
{
  float values[2];

  values[0] = units->valueUnit(0);
  values[1] = units->valueUnit(1);

  bool ok   = UnitsDialog::getUnits(values,title,gui);
  
  if (ok) {
    units->setValueUnits(values[0],values[1]);
    setMetaTopOf(topOfStep,bottomOfStep,units,append,local);
  }
}

void MetaItem::changeViewAngle(
  QString        title,
  const Where   &topOfStep,
  const Where   &bottomOfStep,
  FloatPairMeta *va,
  int            append,
  bool           local)
{
  float floats[2];
  floats[0] = va->value(0);
  floats[1] = va->value(1);
  bool ok = FloatPairDialog::getFloatPair(
              floats,
              title,
              "Lattitude",
              "Longitude",
              gui);

  if (ok) {
    va->setValues(floats[0],floats[1]);
    setMetaTopOf(topOfStep,bottomOfStep,va,append,local);
  }
}

void MetaItem::changeFloat(
  QString      title,
  QString      label,
  const Where &topOfStep,
  const Where &bottomOfStep,
  FloatMeta   *floatMeta,
  int          append,
  bool         local)
{
  float data;
  bool ok = FloatDialog::getFloat(title,
                                  label,
                                  floatMeta,
                                  data);

  if (data < floatMeta->_min) {
    data = floatMeta->_min;
  } else if (data > floatMeta->_max) {
    data = floatMeta->_max;
  }

  if (ok) {
    floatMeta->setValue(data);
    setMetaTopOf(topOfStep,bottomOfStep,floatMeta,append,local);
  }
}

void MetaItem::changeFloatSpin(
  QString      title,
  QString      label,
  const Where &topOfStep,
  const Where &bottomOfStep,
  FloatMeta   *floatMeta,
  int          append,
  bool         local)
{
  float data = floatMeta->value();
  bool ok = DoubleSpinDialog::getFloat(
                                  data,
                                  floatMeta->_min,
                                  floatMeta->_max,
                                  0.1,
                                  title,
                                  label,
                                  gui);
  if (ok) {
    floatMeta->setValue(data);
    setMetaTopOf(topOfStep,bottomOfStep,floatMeta,append,local);
  }
}

void MetaItem::changeBorder(
  QString      title,
  const Where &topOfStep,
  const Where &bottomOfStep,
  BorderMeta  *border,
  int          append,
  bool         local)
{
  BorderData borderData = border->valueUnit();
  bool ok = BorderDialog::getBorder(borderData,title);

  if (ok) {

    border->setValueUnit(borderData);
    setMetaTopOf(topOfStep,bottomOfStep,border,append,local);
  }
}

void MetaItem::changeBool(
  const Where &topOfRanges,
  const Where &bottomOfRanges,
  BoolMeta    *boolMeta,
  int          append,
  bool         local)   // allow local metas
{
  boolMeta->setValue( ! boolMeta->value());
  setMetaTopOf(topOfRanges,bottomOfRanges,boolMeta,append,local);
}


void MetaItem::changeDivider(
  QString       title,
  const Where  &topOfStep,
  const Where  &bottomOfStep,
  SepMeta      *sepMeta,
  int           append,
  bool          local)
{
  SepData     sepData = sepMeta->valueUnit();
  bool ok = DividerDialog::getDivider(sepData,title,gui);

  if (ok) {
    sepMeta->setValueUnit(sepData);
    setMetaTopOf(topOfStep,bottomOfStep,sepMeta,append,local);
  }
}

void MetaItem::changeAlloc(
  const Where &topOfRanges,
  const Where &bottomOfRanges,
  AllocMeta   &alloc,
  int          append)
{
  AllocEnc allocType = alloc.value();
  alloc.setValue(allocType == Vertical ? Horizontal : Vertical);
  setMetaTopOf(topOfRanges,bottomOfRanges,&alloc,append,false);
}

/***************************************************************************/

Rc MetaItem::scanForward(Where &here,int mask)
{
  bool partsAdded;

  return scanForward(here, mask, partsAdded);
}

Rc  MetaItem::scanForward(
  Where &here,
  int    mask,
  bool  &partsAdded)
{
  Meta tmpMeta;
  int  numLines  = gui->subFileSize(here.modelName);
  Rc   rc;
  partsAdded = false;

  for ( ; here < numLines; here++) {

    QString line = gui->readLine(here);
    QStringList tokens;

    split(line,tokens);

    if (tokens.size() > 0 && tokens[0].size() == 1 &&
        tokens[0][0] >= '1' && tokens[0][0] <= '5') {
      partsAdded = true;
    } else {
      rc = tmpMeta.parse(line,here);

      if (rc == StepGroupEndRc && mask == StepGroupEndMask) {
        return StepGroupEndRc;
      } else if (rc < ClearRc) {
      
        int tmask = (1 << rc) & mask;
      
        if (tmask) {
          return rc;
        } else if ((mask & StepMask) && rc == StepRc || rc == RotStepRc) {
          // ignore extra STEPS
          if (partsAdded) {
            if ((1 << rc) & mask) {
              return rc;
            }
          }
          partsAdded = false;
        }
      }
    }
  }
  return EndOfFileRc;
}

Rc MetaItem::scanBackward(Where &here,int mask)
{
  bool partsAdded;

  return scanBackward(here, mask, partsAdded);
}

Rc MetaItem::scanBackward(
  Where &here,
  int    mask,
  bool  &partsAdded)
{
  Meta tmpMeta;

  for ( ; here >= 0; here--) {

    QString line = gui->readLine(here);
    QStringList tokens;

    if (isHeader(line)) {
      return EndOfFileRc;
    }
    split(line,tokens);

    if (tokens.size() > 0 && tokens[0].size() == 1 &&
        tokens[0][0] >= '1' && tokens[0][0] <= '5') {
      partsAdded = true;
    } else {
      Rc rc = tmpMeta.parse(line,here);
      if ((mask & StepMask) && rc == StepRc || rc == RotStepRc) {
        if (partsAdded && ((1 << rc) & mask)) {
          return rc;
        }
        partsAdded = false;
      } else if (rc < ClearRc && ((1 << rc) & mask)) {
        return rc;
      }
    }
  }
  return EndOfFileRc;
}

void MetaItem::insertMeta(const Where &here, const QString &line)
{
  gui->insertLine(here, line);
}
void MetaItem::appendMeta(const Where &here, const QString &line)
{
  gui->appendLine(here, line);
}

void MetaItem::replaceMeta(const Where &here, const QString &line)
{
  gui->replaceLine(here,line);
}
void MetaItem::deleteMeta(const Where &here)
{
  gui->deleteLine(here);
}

void MetaItem::beginMacro(QString name)
{
  gui->beginMacro(name);
}

void MetaItem::endMacro()
{
  gui->endMacro();
}

Where MetaItem::firstLine(
  QString modelName)
{
  Where foo = sortedGlobalWhere(modelName,"ZZZZZ");
  return --foo;
}

Where MetaItem::sortedGlobalWhere(
  QString modelName,
  QString metaString)
{
  Meta tmpMeta;
  return sortedGlobalWhere(tmpMeta,modelName,metaString);
}  
  
Where MetaItem::sortedGlobalWhere(
  Meta   &tmpMeta,
  QString modelName,
  QString metaString)
{
  Where walk = Where(modelName,0);
  int maxLines = gui->subFileSize(modelName);
  QRegExp lines1_5("^\\s*[1-5]");
  
  bool header = true;

  /* Skip all of the header lines */

  for ( ; walk < maxLines; walk++)
  {
    QString line = gui->readLine(walk);

    if (walk.lineNumber == 0) {
      QStringList argv;
      split(line,argv);
      if (argv[0] != "0") {
        insertMeta(walk,"0");
        maxLines++;
      }
    }
    
    if (walk > 0) {
      if (header) {
        header &= isHeader(line);
        if ( ! header) {
          break;
        }
      }
    }
  }

  /* We're on our first non-header line */
  
  QRegExp global("LPUB\\s+.* GLOBAL ");
  QString line;
  for ( ; walk < maxLines; walk++) {
    QString prevLine = line;

    line = gui->readLine(walk);

    // if we hit a part add, we're done

    if (line.contains(lines1_5)) {
      return walk;
    }

    // if this line breaks the alphabetical rule,
    // we'll stop, because we've probably run
    // into a meta for the first step in the model

    if (0 && line < prevLine) {
      return walk;
    }
    
    if (line.contains(global)) {
      continue;
    }

    // These metas are related to the first step
    // so we'll stop if we hit one of those.

    Rc rc = tmpMeta.parse(line,walk);

    switch (rc) {
      case PliBeginSub1Rc:
      case PliBeginSub2Rc:
      case PliBeginIgnRc:
      case PartBeginIgnRc:
      case MLCadSkipBeginRc:
      case SynthBeginRc:
      case ClearRc:
      case BufferStoreRc:
      case MLCadGroupRc:
      case GroupRemoveRc:
      case RemoveGroupRc:
      case RemovePartRc:
      case RemoveNameRc:
      case ReserveSpaceRc:
      case CalloutBeginRc:
      case StepGroupBeginRc:
      case EndOfFileRc:
      case RotStepRc:
      case StepRc:
        return walk;
      break;
      default:
      break;
    }

    // Stop if it is time to add the line    

    if (line > metaString) {
      return walk - 1;
    }
  }

  return walk;  
}

int MetaItem::numSteps(QString modelName)
{
  return gui->numSteps(modelName);
}

/***********************************************************************
 *
 * Callout tools
 *
 **********************************************************************/
 
float determinant(
  QStringList tokens)
{

  /* 5  6  7
     8  9 10
    11 12 13 */
    
  float a = tokens[5].toFloat();
  float b = tokens[6].toFloat();
  float c = tokens[7].toFloat();
  float d = tokens[8].toFloat();
  float e = tokens[9].toFloat();
  float f = tokens[10].toFloat();
  float g = tokens[11].toFloat();
  float h = tokens[12].toFloat();
  float i = tokens[13].toFloat();
  
  return a*e*i - a*f*h - b*d*i + b*f*g + c*d*h - c*e*g;
}
 
bool equivalentAdds(
  QString const &first,
  QString const &second)
{
  QStringList firstTokens, secondTokens;
  bool        firstMirror, secondMirror;
  
  firstMirror = false;
  secondMirror = false;
  
  split(first,firstTokens);
  split(second,secondTokens);
  
  firstMirror = determinant(firstTokens) < 0;
  secondMirror = determinant(secondTokens) < 0;
  
  return firstMirror == secondMirror && firstTokens[14] == secondTokens[14];
}

void MetaItem::convertToCallout(
  Meta *meta,
  const QString &modelName)
{
  gui->maxPages = -1;

  beginMacro("convertToCallout");

  /* Scan the file and remove any multi-step stuff */

  int  numLines  = gui->subFileSize(modelName);
  Where walk(modelName,numLines-1);
  QRegExp ms("^\\s*0\\s+\\!*LPUB\\s+MULTI_STEP\\s+");

  do {
    QString line = gui->readLine(walk);
    if (line.contains(ms)) {
      deleteMeta(walk);
    }
  } while (--walk >= 0);

  /* Now scan the lines following this line, to see if there is another
   * part just like this one that needs to be added as a callout
   * multiplier */

  Where calledOut = meta->submodelStack[meta->submodelStack.size() - 1];
  QString firstLine = gui->readLine(calledOut);
  walk = calledOut+1;
  numLines = gui->subFileSize(walk.modelName);
  for ( ; walk.lineNumber < numLines; walk++) {
    QString line = gui->readLine(walk);
    QStringList argv;
    split(line,argv);
    if (argv.size() == 2 && argv[0] == "0") {
      if (argv[1] == "STEP" || argv[1] == "ROTSTEP") {
        break;
      }
    } else if (argv.size() >= 3 && argv[0] == "0"
                                && (argv[1] == "LPUB" ||
                                    argv[1] == "!LPUB")
                                && argv[2] == "PLI") {
      break;
    } else if (argv.size() == 15 && argv[0] == "1") {
      if (gui->isSubmodel(argv[14])) {
        if (equivalentAdds(firstLine,line)) {
        } else {
          break;
        }
      }
    }
  }

  insertMeta(walk,     "0 !LPUB CALLOUT END");
  insertMeta(calledOut,"0 !LPUB CALLOUT BEGIN");
  nestCallouts(modelName);
  endMacro();
}

void MetaItem::nestCallouts(
  const QString &modelName)
{
  Where walk(modelName,1);

  int numLines = gui->subFileSize(walk.modelName);

  bool partIgnore = false;
  bool callout = false;

  for ( ; walk.lineNumber < numLines; ++walk) {

    QString line = gui->readLine(walk);

    QStringList argv;

    split(line,argv);

    if (argv.size() >= 2 && argv[0] == "0") {
      if (argv[1] == "WRITE") {
        argv.removeAt(1);
      } else if (argv[1] == "GHOST") {
        argv.removeAt(1);
        argv.removeAt(0);
      }

      if (argv[1] == "LPUB" || argv[1] == "!LPUB") {
        if (argv.size() == 5 && argv[2] == "PART" 
                             && argv[3] == "BEGIN" 
                             && argv[4] == "IGN") {
          partIgnore = true;
        } else if (argv.size() == 5 && argv[2] == "PART" 
                                    && argv[3] == "END") {
          partIgnore = false;
        } else if (argv.size() == 4 && argv[2] == "CALLOUT"
                                    && argv[3] == "BEGIN") {
          callout = true;
        } else if (argv.size() == 4 && argv[2] == "CALLOUT"
                                    && argv[3] == "END") {
          callout = false;
        } else if (argv.size() >= 3 && argv[3] == "MULTI_STEP") {
          deleteMeta(walk);
          --numLines;
          --walk;
        }
      }
    } else if ( ! callout && ! partIgnore) {
      if (argv.size() == 15 && argv[0] == "1") {
        if (gui->isSubmodel(argv[14])) {
          // We've got to call this submodel out
          insertMeta(walk, "0 !LPUB CALLOUT BEGIN");
          walk.lineNumber += 2;
          ++numLines;
          nestCallouts(argv[14]);
          for ( ; walk.lineNumber < numLines; walk++) {
            QString line = gui->readLine(walk);
            QStringList argv;
            split(line,argv);
            if (argv.size() == 2 && argv[0] == "0") {
              if (argv[1] == "STEP" || argv[1] == "ROTSTEP") {
                break;
              } else if (argv.size() >= 3
                         && (argv[1] == "LPUB" ||
                             argv[1] == "!LPUB")
                         && argv[2] == "MULTI_STEP") {
                --walk;
                --numLines;
              }
            } else if (argv.size() == 15 && argv[0] == "1") {
              if (gui->isSubmodel(argv[14])) {
                if (argv[14] == modelName) {
                } else {
                  break;
                }
              }
            }
          }
          insertMeta(walk,     "0 !LPUB CALLOUT END");
          ++walk;
          ++numLines;
        }
      }
    }
  }
}

void MetaItem::removeCallout(
  const QString &modelName,
  const Where   &topOfCallout,
  const Where   &bottomOfCallout)
{
  gui->maxPages = -1;

  /* scan the called out model and remove any dividers */

  int  numLines = gui->subFileSize(modelName);

  Where walk(modelName,numLines-1);
  Rc rc;

  beginMacro("removeCallout");

  do {
    rc = scanBackward(walk,CalloutDividerMask);
    if (rc == CalloutDividerRc) {
      deleteMeta(walk);
    }
  } while (rc != EndOfFileRc);

  QRegExp callout("^\\s*0\\s+\\!*LPUB\\s+CALLOUT");
  QString line;

  for (walk = bottomOfCallout;
       walk >= topOfCallout.lineNumber;
       walk--)
  {
    line = gui->readLine(walk);
    if (line.contains(callout)) {
      deleteMeta(walk);
    }
  }
  endMacro();
}

void MetaItem::unnestCallouts(
  const QString &modelName)
{
  Where walk(modelName,1);

  int numLines = gui->subFileSize(walk.modelName);

  bool partIgnore = false;
  bool callout = false;

  for ( ; walk.lineNumber < numLines; ++walk) {

    QString line = gui->readLine(walk);

    QStringList argv;

    split(line,argv);

    if (argv.size() >= 2 && argv[0] == "0") {
      if (argv[1] == "WRITE") {
        argv.removeAt(1);
      } else if (argv[1] == "GHOST") {
        argv.removeAt(1);
        argv.removeAt(0);
      }

      if (argv[1] == "LPUB" || argv[1] == "!LPUB") {
        if (argv.size() >= 3 && argv[2] == "CALLOUT"
                             && argv[3] == "BEGIN") {
          callout = true;
          deleteMeta(walk);
          --numLines;
          --walk;
        } else if (argv.size() == 4 && argv[2] == "CALLOUT"
                                    && argv[3] == "END") {
          callout = false;
          deleteMeta(walk);
          --numLines;
          --walk;
        }
      }
    } else if ( ! callout && ! partIgnore) {
      if (argv.size() == 15 && argv[0] == "1") {
        if (gui->isSubmodel(argv[14]) && callout) {
          unnestCallouts(argv[14]);
        }
      }
    }
  }
}

void MetaItem::updatePointer(
  const Where &here, PointerMeta *pointer)
{
  if (here.modelName != "undefined") {
    QString repLine = pointer->format(false,false);
    replaceMeta(here,repLine);
  }
}

void MetaItem::deletePointer(const Where &here)
{
  deleteMeta(here);
}
