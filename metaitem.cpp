
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
#include "rx.h"

void MetaItem::setGlobalMeta(
  QString  &topLevelFile,
  LeafMeta *leaf)
{
  QString newMeta = leaf->format(false);
  if (leaf->here().modelName != "undefined") {
    replaceMeta(leaf->here(),newMeta);
  } else {
    Where here = sortedGlobalWhere(topLevelFile,newMeta);
    insertMeta(here,newMeta);
  }
}

/***********************************************************************
 *
 * Big time MultiStep manipulators
 *
 **********************************************************************/

class StepGroup {
  public:
  Where topOfRanges;
        Where firstStep;
        Where firstDiv;
        Where lastStep1;
        Where lastDiv;
  Where lastStep2;
        Where bottomOfRanges;
  Where current;
  int   numSteps;
  bool  partsAdded;
  bool  endOfFile;

  StepGroup()
  {
    topOfRanges    = -1;
    firstStep      = -1;
    firstDiv       = -1;
    lastStep1      = -1;
    lastDiv        = -1;
    lastStep2      = -1;
    bottomOfRanges = -1;
    numSteps       =  0;
    partsAdded     =  false;
    endOfFile      =  false;
  }
};

Rc MetaItem::scanStepGroup(Where current, StepGroup &group)
{
  enum State {
    StepGroupBeginState,
    FirstStepState,
    FirstDivState,
    LastStepState,
    StepGroupEndState,
    EndOfFileState
  } state = StepGroupBeginState;

  Rc rc;

  group.numSteps = 0;
  group.current = current;

  Where lastDiv = -1;
  
  do {
    rc = scanForward(group.current,StepMask|StepGroupMask,group.partsAdded);
    switch (state) {
      case StepGroupBeginState:
        if (rc == StepGroupBeginRc) {
          group.topOfRanges = group.current; ++group.current;
          state = FirstStepState;
        } else if (rc == StepRc || rc == RotStepRc) {
          if (group.partsAdded) {
            return StepRc;
          }
        } else if (rc == EndOfFileRc) {
          group.endOfFile = true;
          return EndOfFileRc;
        } else {
          return FailureRc;
        }
      break;

      case FirstStepState:
        if (rc == StepRc || rc == RotStepRc) {
          if (group.partsAdded) {
            group.firstStep = group.current; ++group.current;
            state = FirstDivState;
            ++group.numSteps;
          }
        } else {
          return FailureRc;
        }
      break;

      case FirstDivState:
        if (rc == StepGroupDividerRc) {
          group.firstDiv = group.current; ++group.current;
          state = LastStepState;
        } else if (rc == StepRc || rc == RotStepRc) {
          if (group.partsAdded) {
            group.lastStep2 = group.current; ++group.current;
            state = LastStepState;
            ++group.numSteps;
          }
        } else {
          return FailureRc;
        }
      break;

      case LastStepState:
        if (rc == StepRc || rc == RotStepRc) {
          if (group.partsAdded) {
            group.lastStep1 = group.lastStep2;
            group.lastStep2 = group.current; ++group.current;
            group.lastDiv = lastDiv;
            lastDiv = -1;
            ++group.numSteps;
          }
        } else if (rc == StepGroupDividerRc) {
          group.lastDiv = lastDiv;
          lastDiv = group.current; ++group.current;
        } else if (rc == StepGroupEndRc) {
          group.bottomOfRanges = group.current; ++group.current;
          state = StepGroupEndState;
        } else {
          return FailureRc;
        }
      break;
      default:
      break;
    }
  } while(state != StepGroupEndState);

  return StepGroupBeginRc;
}

Rc MetaItem::scanPrevStepGroup(Where current, StepGroup &group)
{
  Rc rc;

  enum State {
    StepGroupEndState,
    StepGroupBeginState,
    DoneState
  } state = StepGroupEndState;

  StepGroup tmp;

  tmp.current = current;

  do {
    rc = scanBackward(tmp.current,StepMask|StepGroupMask, tmp.partsAdded);
    switch (state) {
      case StepGroupEndState:
        if (rc == StepGroupEndRc) {
          if (tmp.partsAdded) {
            group.current = ++tmp.current;
            rc = StepRc;
            state = DoneState;
          } else {
            state = StepGroupBeginState;
            --tmp.current;
          }
        } else if (rc == StepRc || rc == RotStepRc) {
          group.current = --tmp.current;
          state = DoneState;
        } else {
          group.current = Where(current.modelName,0);
          state = DoneState;
        }
      break;
      case StepGroupBeginState:
        if (rc == StepGroupBeginRc) {
          group.current = tmp.current;
          state = DoneState;
        } else {
          --tmp.current;
        }
      break;
      default:
      break;
    }
  } while (state != DoneState);

  group.topOfRanges = group.current;

  scanStepGroup(group.current,group);

  return rc;
}

const QString stepGroupBegin   = "0 !LPUB MULTI_STEP BEGIN";
const QString stepGroupDivider = "0 !LPUB MULTI_STEP DIVIDER";
const QString stepGroupEnd     = "0 !LPUB MULTI_STEP END";
const QString step             = "0 STEP";

void MetaItem::removeFirstStep(
  StepGroup &group)
{
  if (group.numSteps == 2) {
    deleteMeta(group.bottomOfRanges);
    StepGroup follow;
    scanStepGroup(group.bottomOfRanges,follow);
    if (follow.endOfFile) {
      deleteMeta(group.lastStep2);
    }
    if (group.firstDiv != -1) {
      deleteMeta(group.firstDiv);
    }
    deleteMeta(group.topOfRanges);
  } else {
    if (group.firstDiv == -1) {
      appendMeta(group.firstStep,stepGroupBegin);
    } else {
      replaceMeta(group.firstDiv,stepGroupBegin);
    }
    deleteMeta(group.topOfRanges);
  }
}

int MetaItem::removeLastStep(
  StepGroup &group)
{
  int sum = 0;
  deleteMeta(group.bottomOfRanges); --sum;

  StepGroup follow;
  scanStepGroup(group.bottomOfRanges+1,follow);
  if (follow.endOfFile && ! follow.partsAdded) {
    deleteMeta(group.lastStep2); --sum;
  }

  if (group.numSteps == 2) {
    if (group.firstDiv != -1) {
      deleteMeta(group.firstDiv); --sum;
    }
    deleteMeta(group.topOfRanges); --sum;
  } else {
    if (group.firstDiv == -1) {
      appendMeta(group.lastStep1,stepGroupEnd); ++sum;
    } else {
      replaceMeta(group.lastDiv,stepGroupEnd);
    }
  }
  return sum;
}

void MetaItem::addNextStep(Where topOfRanges)  // always add at end
{
  Rc rc1,rc2;

  StepGroup me,them;
  bool firstChange = true;

  // What do I look like?

  rc1 = scanStepGroup(topOfRanges, me);

  // What about the step after me?

  rc2 = scanStepGroup(me.current, them);

  // if they are a multistep, remove the first step and
  // refigure out what we all look like?

  if (rc2 == StepGroupBeginRc) {
    firstChange = false;
    beginMacro("addNextStep1");
    removeFirstStep(them);
    rc2 = scanStepGroup(me.current, them);
  }

  if (rc1 == StepRc || rc1 == RotStepRc || rc1 == StepGroupBeginRc) {
    if (rc2 == EndOfFileRc) {
      if (them.partsAdded) {
        if (firstChange) {
          beginMacro("addNextStep2");
        }
        insertMeta(them.current,"0 // AddNextStep2\n");
        insertMeta(them.current,step);
        appendMeta(them.current,stepGroupEnd);
        if (rc1 == StepRc || rc1 == RotStepRc) {
          insertMeta(topOfRanges,stepGroupBegin);
        } else {
          deleteMeta(me.bottomOfRanges);
        }
        endMacro();
      }
    } else if (rc2 == StepRc || rc2 == RotStepRc) {
      if (firstChange) {
        beginMacro("addNextStep3");
        insertMeta(them.current+1,"0 // AddNextStep3\n");
      }
      insertMeta(them.current+1,stepGroupEnd);
      if (rc1 == StepRc || rc1 == RotStepRc) {
        if (topOfRanges.lineNumber < 2) {
          topOfRanges = sortedGlobalWhere(them.current.modelName,"ZZZZZZ");
        }
        insertMeta(topOfRanges,stepGroupBegin);
      } else {
        deleteMeta(me.bottomOfRanges);
      }
      endMacro();
    }
  } else if (rc1 == EndOfFileRc && me.partsAdded) {
    if (firstChange) {
      beginMacro("addNextStep5");
    }
    insertMeta(me.current,"0 // AddNextStep5\n");
    appendMeta(me.current,stepGroupEnd);
    appendMeta(me.current,step);
    deleteMeta(me.bottomOfRanges);
    endMacro();
  }
}

void MetaItem::addPrevStep(
  Where topOfRanges)
{
  Rc rc1,rc2;

  StepGroup me,them;

  bool firstChange = true;

  rc1 = scanStepGroup(topOfRanges,me);
  rc2 = scanPrevStepGroup(topOfRanges-1, them);

  if (rc2 == StepGroupBeginRc) {
    firstChange = false;
    beginMacro("addPrevStep");
    topOfRanges.lineNumber += removeLastStep(them);
    rc1 = scanStepGroup(topOfRanges, me);
    rc2 = scanPrevStepGroup(topOfRanges-1, them);
  }

  // PARTS  // them
  // STEP
  // BEGIN  // me
  // PARTS
  // STEP
  // PARTS
  // END

  // begin
  // PARTS  // them
  // STEP
  // PARTS  // me
  // STEP
  // END

  // PARTS  // them
  // STEP
  // PARTS  // me

  // begin
  // PARTS  // them
  // STEP
  // PARTS  // me
  // step
  // end

  if (firstChange) {
    beginMacro("addPrevStep");
    firstChange = false;
  }

  // make sure we have STEP/END
  if (rc1 == EndOfFileRc || rc1 == StepRc || rc1 == RotStepRc) {

    insertMeta(me.current + (rc1 == StepRc || rc1 == RotStepRc),stepGroupEnd);

    if (rc1 == EndOfFileRc) {
      appendMeta(me.current,step);
    }

    // if we have a begin, we need to remove it
  } else if (rc1 == StepGroupBeginRc) {
    deleteMeta(me.topOfRanges);
  }
  if (rc2 == EndOfFileRc) {
    Where foo = sortedGlobalWhere(them.current.modelName,"ZZZZZZ");
    insertMeta(foo,stepGroupBegin);
  } else {
    insertMeta(them.topOfRanges,stepGroupBegin);
  }
  if ( ! firstChange) {
    endMacro();
  }
}

void MetaItem::deleteFirstMultiStep(
  Where     topOfRanges)
{
  StepGroup group;

  Rc rc = scanStepGroup(topOfRanges,group);

  if (rc == StepGroupBeginRc) {
    beginMacro("removeFirstStep");
    removeFirstStep(group);
    endMacro();
  }
}
void MetaItem::deleteLastMultiStep(
  Where topOfRanges)
{
  StepGroup group;

  Rc rc = scanStepGroup(topOfRanges,group);

  if (rc == StepGroupBeginRc) {
    beginMacro("deleteLastMultiStep");
    removeLastStep(group);
    endMacro();
  }
}

void MetaItem::addMultiStepDivider(
  Where   bottomOfStep,
  RcMeta *divider)
{
  Where   walk = bottomOfStep+1;
  Rc      rc;
  QString divString = divider->preamble;

  rc = scanForward(walk, 
              StepMask|
              StepGroupDividerMask|
              StepGroupEndMask);

  if (rc == StepRc || rc == RotStepRc) {
    // we can add a divider after meta->context.curStep().lineNumber
    walk = bottomOfStep+1;
    insertMeta(walk,divString); 
  }
}

void MetaItem::deleteMultiStepDivider(Where divider)
{
  if (divider.modelName != "undefined") {
    Rc rc;
    rc = scanForward(divider,StepMask|StepGroupDividerMask|StepGroupEndMask);

    if (rc == StepGroupDividerRc) {
      deleteMeta(divider);
    }
  }
}

/***********************************************************************
 *
 * Callout tools
 *
 **********************************************************************/
 
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
  
  for (int i = 5; i < 14; i++) {
    if (firstTokens[i].toFloat() < 0) {
	  firstMirror = true;
	}
	if (secondTokens[i].toFloat() < 0) {
	  secondMirror = true;
	}
  }
  
  return firstMirror == secondMirror && firstTokens[14] == secondTokens[14];
}

void MetaItem::convertToCallout(Meta *meta)
{
  gui->maxPages = -1;

  beginMacro("convertToCallout");

  /* Scan the file and remove any multi-step stuff */

  QString modelName = meta->context.topOfFile().modelName;
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

void MetaItem::addCalloutDivider(
  Where   bottomOfStep,
  RcMeta *divider)
{
  Where   walk = bottomOfStep+1;
  Rc      rc;
  QString divString = divider->preamble;
  bool    partsAdded;

  rc = scanForward(walk, StepMask|CalloutDividerMask,partsAdded);

  if (rc == StepRc || rc == RotStepRc || (rc == EndOfFileRc && partsAdded)) {
    // we can add a divider after meta->context.curStep().lineNumber    
    walk = bottomOfStep+1;
    insertMeta(walk,divString); 
  }
}

void MetaItem::removeCallout(
  Context &context)
{
  gui->maxPages = -1;

  /* scan the called out model and remove any dividers */

  QString modelName = context.topOfFile().modelName;

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

  for (walk = context.bottomOfRanges();
       walk >= context.topOfRanges().lineNumber;
       walk--)
  {
    line = gui->readLine(walk);
    if (line.contains(callout)) {
      deleteMeta(walk);
    }
  }
  //unnestCallouts(modelName);
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

void MetaItem::updatePointer(Where here, PointerMeta *pointer)
{
  if (here.modelName != "undefined") {
    QString repLine = pointer->preamble + pointer->format(true);
    replaceMeta(here,repLine);
  }
}

void MetaItem::deletePointer(Where here)
{
  deleteMeta(here);
}

/***********************************************************************
 *
 * These "short-cuts" move steps to the other side of a divider
 *
 **********************************************************************/

const QString calloutDivider = "0 !LPUB CALLOUT DIVIDER";

void MetaItem::moveStepPrev(
  PlacementType type,
  Where bottomOfRanges,
  Where bottomOfStep)
{
  //              ****
  // STEP         STEP DIVIDER STEP
  // STEP DIVIDER STEP DIVIDER STEP
  beginMacro("moveStepPrev");

  Where walk;
  if (bottomOfStep + 1 != bottomOfRanges) {
    walk = ++bottomOfStep;
    if (type == StepGroupType) {
      if (scanForward(walk,StepMask|StepGroupMask) != StepGroupDividerRc) {
        insertMeta(bottomOfStep,stepGroupDivider);
      }
    } else {
      if (scanForward(walk,StepMask|CalloutDividerMask) != CalloutDividerRc) {
        insertMeta(bottomOfStep,calloutDivider);
      }
    }
  }
  if (type == StepGroupType) {
    walk = --bottomOfStep;
    if (scanBackward(walk,StepGroupDividerMask) == StepGroupDividerRc) {
      deleteMeta(walk);
    }
  } else {
    walk = --bottomOfStep;
    if (scanBackward(walk,CalloutDividerMask) == CalloutDividerRc) {
      deleteMeta(walk);
    }
  }
  endMacro();
}

void MetaItem::moveStepNext(
  PlacementType type,
  Where topOfRanges,
  Where bottomOfRange,
  Where topOfStep,
  Where bottomOfStep)
{
  //              ****
  // STEP DIVIDER STEP DIVIDER STEP END
  // STEP         STEP DIVIDER STEP END
  beginMacro("moveStepNext");
  deleteMeta(bottomOfRange);
  
  if (topOfStep != topOfRanges) {
    Where walk = --bottomOfStep;

    if (type == StepGroupType) {
      if (scanBackward(walk,StepMask|StepGroupMask) != StepGroupDividerRc) {
        insertMeta(topOfStep,stepGroupDivider);
      }
    } else {
      if (scanBackward(walk,StepMask|CalloutDividerMask) != CalloutDividerRc) {
        insertMeta(topOfStep,calloutDivider);
      }
    }
  }
  endMacro();
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
  bool         local)
{
  int  lineNumber = meta->here().lineNumber;
  bool metaInRange;

  metaInRange = meta->here().modelName == topOf.modelName
   && lineNumber >= topOf.lineNumber 
   && lineNumber <= bottomOf.lineNumber;

  if (metaInRange) {
    QString line = meta->format(meta->pushed);
    replaceMeta(meta->here(),line);
  } else {
    if (local) {
      local = LocalDialog::getLocal(LPUB, "Change only this step?",gui);
    }
    QString line = meta->format(local);

    if (topOf.lineNumber == 0) {
      QString line = gui->readLine(topOf);
      QStringList argv;
      split(line,argv);
      if (argv.size() >= 1 && argv[0] != "0") {
        insertMeta(topOf,"0");
      }
    }
    appendMeta(topOf, line);
  }
}

void MetaItem::setMetaBottomOf(
  const Where &topOf,
  const Where &bottomOf,
  LeafMeta    *meta,
  bool         local)
{
  int  lineNumber = meta->here().lineNumber;
  bool metaInRange;

  metaInRange = meta->here().modelName == topOf.modelName
   && lineNumber >= topOf.lineNumber 
   && lineNumber <= bottomOf.lineNumber;

  if (metaInRange) {
    QString line = meta->format(meta->pushed);
    replaceMeta(meta->here(),line);
  } else {
    if (local) {
      local = LocalDialog::getLocal(LPUB, "Change only this step?",gui);
    }
    QString line = meta->format(local);

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
  bool           useLocal) 
{
  PlacementData placementData = placement->value();
  bool ok;
  ok = PlacementDialog
       ::getPlacement(parentType,relativeType,placementData,title);

  if (ok) {
    placement->setValue(placementData);
	if (relativeType == PartsListType && 
	   (parentType == StepGroupType || parentType == CalloutType)) {
      setMetaBottomOf(topOfRanges,bottomOfRanges,placement,useLocal);
	} else {
      setMetaTopOf(topOfRanges,bottomOfRanges,placement,useLocal);
	}
  }
}

void MetaItem::changePlacementOffset(
  Where          defaultWhere,
  PlacementMeta *placement,
  bool           local)
{
  QString newMetaString = placement->format(local);

  if (placement->here().modelName == "undefined") {

    Where walk = defaultWhere + 1;

    bool partsAdded;

    if (scanBackward(walk,StepMask,partsAdded) == EndOfFileRc) {
      Meta tmpMeta;
      defaultWhere = sortedGlobalWhere(tmpMeta,defaultWhere.modelName,"ZZZZ")-1;
    }
    insertMeta(defaultWhere,newMetaString);
  } else {
    replaceMeta(placement->here(),newMetaString);
  }
}

void MetaItem::changeBackground(
  QString         title,
  const Where    &topOfStep,
  const Where    &bottomOfStep,
  BackgroundMeta *background,
  bool            local)
{
  BackgroundData backgroundData = background->value();
  bool ok;
  ok = BackgroundDialog::getBackground(backgroundData,title,gui);

  if (ok) {

    background->setValue(backgroundData);
    setMetaBottomOf(topOfStep,bottomOfStep,background,local);
  }
}

void MetaItem::changeConstraint(
  QString        title,
  const Where   &topOfStep,
  const Where   &bottomOfStep,
  ConstrainMeta *constraint,
  bool           local)
{
  ConstrainData constrainData = constraint->value();
  bool ok;
  ok = ConstrainDialog::getConstraint(constrainData,title,gui);

  if (ok) {

    constraint->setValueUnit(constrainData);
    setMetaBottomOf(topOfStep,bottomOfStep,constraint,local);
  }
}

void MetaItem::changeFont(
  const Where   &topOfStep,
  const Where   &bottomOfStep,
  FontMeta      *font,
  bool           local)
{
  bool ok;
  QFont _font;
  QFontDatabase _fdb;

  _font.fromString(font->valueUnit());

  _font = QFontDialog::getFont(&ok,_font);

  if (ok) {

    font->setValueUnit(_font.toString());
    setMetaBottomOf(topOfStep,bottomOfStep,font,local);
  }
}

void MetaItem::changeColor(
  const Where &topOfStep,
  const Where &bottomOfStep,
  StringMeta *color,
  bool        local)
{
  QColor _color = LDrawColor::color(color->value());
  _color = QColorDialog::getColor(_color,NULL);

  if (_color.isValid()) {
    color->setValue(_color.name());
    setMetaBottomOf(topOfStep,bottomOfStep,color,local);
  }
}

void MetaItem::changeMargins(
  QString        title,
  const Where   &topOfStep,
  const Where   &bottomOfStep,
  MarginsMeta   *margin,
  bool           local)
{
  float values[2];

  values[0] = margin->valueUnit(0);
  values[1] = margin->valueUnit(1);

  bool ok   = UnitsDialog::getUnits(
                values,
                title,
                gui);

  if (ok) {
    margin->setValueUnits(values[0],values[1]);
    setMetaBottomOf(topOfStep,bottomOfStep,margin,local);
  }
}

void MetaItem::changeUnits(
  QString       title,
  const Where  &topOfStep,
  const Where  &bottomOfStep,
  UnitsMeta    *units,
  bool          local)
{
  float values[2];

  values[0] = units->valueUnit(0);
  values[1] = units->valueUnit(1);

  bool ok   = UnitsDialog::getUnits(
                values,
                title,
                gui);
  if (ok) {
    units->setValueUnits(values[0],values[1]);
    setMetaBottomOf(topOfStep,bottomOfStep,units,local);
  }
}

void MetaItem::changeViewAngle(
  QString        title,
  const Where   &topOfStep,
  const Where   &bottomOfStep,
  FloatPairMeta *va,
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
    setMetaBottomOf(topOfStep,bottomOfStep,va,local);
  }
}

void MetaItem::changeFloat(
  QString      title,
  QString      label,
  const Where &topOfStep,
  const Where &bottomOfStep,
  FloatMeta   *floatMeta,
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
    setMetaBottomOf(topOfStep,bottomOfStep,floatMeta,local);
  }
}

void MetaItem::changeFloatSpin(
  QString      title,
  QString      label,
  const Where &topOfStep,
  const Where &bottomOfStep,
  FloatMeta   *floatMeta,
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
    setMetaBottomOf(topOfStep,bottomOfStep,floatMeta,local);
  }
}

void MetaItem::changeBorder(
  QString      title,
  const Where &topOfStep,
  const Where &bottomOfStep,
  BorderMeta  *border,
  bool         local)
{
  BorderData borderData = border->valueUnit();
  bool ok = BorderDialog::getBorder(borderData,title);

  if (ok) {

    border->setValueUnit(borderData);
    setMetaBottomOf(topOfStep,bottomOfStep,border,local);
  }
}

void MetaItem::changeBool(
  const Where &topOfRanges,
  const Where &bottomOfRanges,
  BoolMeta    *boolMeta,
  bool         local)   // allow local metas
{
  boolMeta->setValue( ! boolMeta->value());
  setMetaTopOf(topOfRanges,bottomOfRanges,boolMeta,local);
}


void MetaItem::changeDivider(
  QString       title,
  const Where  &topOfStep,
  const Where  &bottomOfStep,
  SepMeta      *sepMeta,
  bool          local)
{
  SepData     sepData = sepMeta->valueUnit();
  bool ok = DividerDialog::getDivider(sepData,title,gui);

  if (ok) {
    sepMeta->setValueUnit(sepData);
    setMetaBottomOf(topOfStep,bottomOfStep,sepMeta,local);
  }
}

void MetaItem::changeAlloc(
  const Where &topOfRanges,
  const Where &bottomOfRanges,
  AllocMeta   &alloc)
{
  AllocEnc allocType = alloc.value();
  alloc.setValue(allocType == Vertical ? Horizontal : Vertical);
  setMetaBottomOf(topOfRanges,bottomOfRanges,&alloc,true);
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
  
  partsAdded = false;

  for ( ; here < numLines; here++) {

    QString line = gui->readLine(here);
    QStringList tokens;

    split(line,tokens);

    if (tokens.size() > 0 && tokens[0].size() == 1 &&
        tokens[0][0] >= '1' && tokens[0][0] <= '5') {
      partsAdded = true;
    } else {
      Rc rc = tmpMeta.parse(line,here,partsAdded);

      if (rc == StepRc || rc == RotStepRc) {
        // ignore extra STEPS
        if (partsAdded) {
          if ((1 << rc) & mask) {
            return rc;
          }
        }
        partsAdded = false;
      } else if (rc < ClearRc && ((1 << rc) & mask)) {
        return rc;
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

    split(line,tokens);

    if (tokens.size() > 0 && tokens[0].size() == 1 &&
        tokens[0][0] >= '1' && tokens[0][0] <= '5') {
      partsAdded = true;
    } else {
      Rc rc = tmpMeta.parse(line,here,partsAdded);
      if (rc == StepRc || rc == RotStepRc) {
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

    int i;
    for (i = 0; i < LDrawHeaderRx.size(); i++) {
      QRegExp rx(LDrawHeaderRx[i]);
      if (line.contains(rx)) {
        break;
      }
    }
    if (i == LDrawHeaderRx.size() && walk > 0) {
      break;
    }
  }

  /* We're on our first non-header line */

  QString line;
  for ( ; walk < maxLines; walk++)
  {
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
      return walk;
    }
  }

  return walk;  
}

int MetaItem::numSteps(QString modelName)
{
  return gui->numSteps(modelName);
}
