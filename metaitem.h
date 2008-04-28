
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

#ifndef METAITEM_H
#define METAITEM_H

#include <QString>
#include "meta.h"

enum ScanMask {

  StepMask = (1 << StepRc)|(1 << RotStepRc),
  CalloutBeginMask = (1 << CalloutBeginRc),
  CalloutDividerMask = (1 << CalloutDividerRc),
  CalloutEndMask = (1 << CalloutEndRc),
  CalloutMask = CalloutBeginMask|CalloutDividerMask|CalloutEndMask,

  StepGroupBeginMask = (1 << StepGroupBeginRc),
  StepGroupDividerMask = (1 << StepGroupDividerRc),
  StepGroupEndMask = (1 << StepGroupEndRc),

  StepGroupMask = StepGroupBeginMask|StepGroupDividerMask|StepGroupEndMask,
};


class StepGroup;

class MetaItem {
public:
  void setGlobalMeta(QString &topLevelFile, LeafMeta *leaf);

  void convertToCallout(       Meta *);
  void nestCallouts(           const QString & );
  void addCalloutDivider (     Where,   RcMeta *divider);
  void removeCallout(          Context &);
  void unnestCallouts(         const QString & );
  void updatePointer(          Where,   PointerMeta *pointer);
  void deletePointer(          Where);

  void addNextStep(            Where topOfStep);
  void addPrevStep(            Where topOfStep);

  void deleteFirstMultiStep(   Where);
  void deleteLastMultiStep(    Where);

  void addMultiStepDivider (   Where, RcMeta *divider);
  void deleteMultiStepDivider( Where);

  void moveStepPrev(           PlacementType, Where, Where);
  void moveStepNext(           PlacementType, Where, Where, Where, Where);

  void convertToIgnore(        Meta *);

  void changePlacement( PlacementType parentType,
                        PlacementType placedType, 
                        QString, 
                        const Where &top,
                        const Where &bottom, 
                        PlacementMeta *, 
                        bool checkLocal = true);

  void changeAlloc(     const Where &, 
                        const Where &, 
				        AllocMeta   &);
  void changeBool(      const Where &, 
                        const Where &, 
                        BoolMeta    *, 
                        bool allowLocal = false);

  void changeFont(      const Where &, 
                        const Where &,
                        FontMeta  *,
                        bool checkLocal = true);  
  
  void changeColor(     const Where &, 
                        const Where &,
                        StringMeta  *,   
                        bool checkLocal = true);

  void changeBackground(QString, 
                        const Where &, 
                        const Where &,
                        BackgroundMeta*, 
                        bool checkLocal = true);

  void changeBorder(    QString, 
                        const Where &, 
                        const Where &,
                        BorderMeta  *,   
                        bool checkLocal = true);

  void changeViewAngle( QString, 
                        const Where &, 
                        const Where &, 
                        FloatPairMeta *, 
                        bool checkLocal = true);

  void changeConstraint(QString, 
                        const Where &, 
                        const Where &, 
                        ConstrainMeta *, 
                        bool checkLocal = true);

  void changeDivider(   QString, 
                        const Where &, 
                        const Where &, 
                        SepMeta *,   
                        bool checkLocal = true);

  void changeMargins(   QString, 
                        const Where &, 
                        const Where &, 
                        MarginsMeta *,   
                        bool checkLocal = true);

  void changeFloat(     QString, 
                        QString, 
                        const Where &, 
                        const Where &, 
                        FloatMeta *, 
                        bool checkLocal = true);

  void changeFloatSpin( QString, 
                        QString, 
                        const Where &, 
                        const Where &, 
                        FloatMeta *, 
                        bool checkLocal = true);

  void changeUnits(     QString,          
                        const Where &, 
                        const Where &, 
                        UnitsMeta *, 
                        bool checkLocal = true);

  void setMetaTopOf(    const Where &,
                        const Where &,
                        LeafMeta *,
                        bool);

  void setMetaBottomOf( const Where &,
                        const Where &,
                        LeafMeta *,
                        bool);

  void changePlacementOffset(Where defaultconst, PlacementMeta *placement, bool local = true);  

  void replaceMeta(const Where &here, const QString &line);
  void insertMeta( const Where &here, const QString &line);
  void appendMeta( const Where &here, const QString &line);
  void deleteMeta( const Where &here);
  void beginMacro( QString name);
  void endMacro();
  Where sortedGlobalWhere(QString modelName,QString metaString);
  Where sortedGlobalWhere(Meta &tmpMeta,QString modelName,QString metaString);

  Rc   scanForward( Where &here, int mask, bool &partsAdded);
  Rc   scanBackward(Where &here, int mask, bool &partsAdded);
  Rc   scanForward( Where &here, int mask);
  Rc   scanBackward(Where &here, int mask);

  Rc   scanStepGroup(Where current, StepGroup &group);
  Rc   scanPrevStepGroup(Where current, StepGroup &group);
  void removeFirstStep(StepGroup &group);
  int  removeLastStep(StepGroup &group);

  int  numSteps(QString modelName);
};

#endif
