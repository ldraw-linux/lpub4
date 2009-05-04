
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
 * This file describes some of the compound data types managed by meta
 * classes such as background, border, number, etc.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef METATYPES_H
#define METATYPES_H

#include <QString>
#include <QStringList>
#include <QPointF>

enum AllocEnc {
  Horizontal = 0,
  Vertical
};

enum RectPlacement{

  TopLeftOutsideCorner, 
  TopLeftOutside, 
  TopOutside, 
  TopRightOutSide, 
  TopRightOutsideCorner,

  LeftTopOutside,
  TopLeftInsideCorner, 
  TopInside,
  TopRightInsideCorner,
  RightTopOutside,

  LeftOutside,
  LeftInside,
  CenterCenter,
  RightInside,
  RightOutside,

  LeftBottomOutside,
  BottomLeftInsideCorner,
  BottomInside,
  BottomRightInsideCorner,
  RightBottomOutside,

  BottomLeftOutsideCorner,
  BottomLeftOutside,
  BottomOutside,
  BottomRightOutside,
  BottomRightOutsideCorner,

  NumSpots
};

enum PlacementEnc {
  TopLeft,
  Top,
  TopRight,
  Right,
  BottomRight,
  Bottom,
  BottomLeft,
  Left,
  Center,
  NumPlacements,
};

enum PrepositionEnc {
  Inside = 0,
  Outside
};

enum PlacementType {
  PageType,
  CsiType,
  StepGroupType,
  StepNumberType,
  PartsListType,
  CalloutType,
  PageNumberType,
  SingleStepType,
  SubmodelInstanceCountType,
  StepType,
  RangeType,
  ReserveType,
  BomType,
  CoverPageType,
  NumRelatives
};

class PlacementData
{
public:
  PlacementEnc   placement;
  PlacementEnc   justification;
  PlacementType  relativeTo;
  PrepositionEnc preposition;
  RectPlacement  rectPlacement;

  float offsets[2];
  PlacementData()
  {
    rectPlacement = TopLeftOutsideCorner;
    offsets[0] = 0;
    offsets[1] = 0;
  }
#ifdef OPEREQ
  PlacementData &operator=(const PlacementData &rhs)
  {
    if (this != &rhs) {
      placement     = rhs.placement;
      justification = rhs.justification;
      relativeTo    = rhs.relativeTo;
      preposition   = rhs.preposition;
      rectPlacement = rhs.rectPlacement;
    }
    return *this;
  }
#endif
};

class PointerData
{
public:
  PlacementEnc placement;
  float loc;  // fraction of side/top/bottom o callout
  float x;    // fraction of CSI size
  float y;
  float base; // in units
#ifdef OPEREQ
  PointerData &operator=(const PointerData &rhs)
  {
    if (this != &rhs) {
      placement = rhs.placement;
      loc       = rhs.loc;
      x         = rhs.x;
      y         = rhs.y;
      base      = rhs.base;
    }
    return *this;
  }
#endif
};

class RotStepData
{
public:
  double  rots[3];
  QString type;
  RotStepData()
  {
    type = "";
    rots[0] = 0;
    rots[1] = 0;
    rots[2] = 0;
  }
#ifdef OPEREQ
  RotStepData &operator=(const RotStepData &rhs)
  {
    if (this != &rhs) {
      type    = rhs.type;
      rots[0] = rhs.rots[0];
      rots[1] = rhs.rots[1];
      rots[2] = rhs.rots[2];
    } 
    return *this;  
  }
#endif
};

class BuffExchgData
{
public:
  QString buffer;
  QString type;
#ifdef OPEREQ
  BuffExchgData &operator=(const BuffExchgData &rhs)
  {
    if (this != &rhs) {
      buffer = rhs.buffer;
      type   = rhs.type;
    } 
    return *this;  
  }
#endif
};

class InsertData : public PlacementData
{
public:  
  enum InsertType
  {
    InsertPicture,
    InsertText,
    InsertArrow,
    InsertBom
  } type;
  float          margins[2];
  QString        picName;
  qreal          picScale;
  QStringList    text;
  QPointF        arrow[2];
  InsertData()
  {
    offsets[0] = 0;
    offsets[1] = 0;
    margins[0] = 0;
    margins[1] = 0;
    arrow[0] = QPointF(0,0);
    arrow[1] = QPointF(0,0);
  }
#ifdef OPEREQ
  InsertData &operator=(const InsertData &rhs)
  {
    if (this != &rhs) {
      type       = rhs.type;
      offsets[0] = rhs.offsets[0];
      offsets[1] = rhs.offsets[1];
      margins[0] = rhs.margins[0];
      margins[1] = rhs.margins[1];
      picName    = rhs.picName;
      picScale   = rhs.picScale;
      text       = rhs.text;
      arrow[0]   = rhs.arrow[0];
      arrow[1]   = rhs.arrow[1];
    } 
    return *this;  
  }
#endif
};

class BackgroundData
{
public:
  enum Background {
    BgTransparent,
    BgImage,
    BgColor,
    BgSubmodelColor
  } type;
  QString    string;
  bool       stretch;
#ifdef OPEREQ
  BackgroundData &operator=(const BackgroundData &rhs)
  {
    if (this != &rhs) {
      type = rhs.type;
      string = rhs.string;
      stretch = rhs.stretch;
    }
    return *this;
  }
#endif
};

class BorderData
{
public:
  enum Border {
    BdrNone = 0,
    BdrSquare,
    BdrRound
  } type;
  QString color;
  float   thickness;  // in units 
  float   radius;     // in units
  float   margin[2];  // in units
  
  BorderData()
  {
    thickness = 0.125;
    margin[0] = 0;
    margin[1] = 0;
    radius = 15;
    type = BdrNone;
    color = "Black";
  }
#ifdef OPEREQ
  BorderData &operator=(const BorderData &rhs)
  {
    if (this != &rhs) {
      type = type;
      color     = rhs.color;
      thickness = rhs.thickness;
      radius    = rhs.radius;
      margin[0] = rhs.margin[0];
      margin[1] = rhs.margin[1];
    } 
    return *this;  
  }
#endif
};

class SubData
{
public:
  QString color;
  QString part;
  int     type;
#ifdef OPEREQ
  SubData &operator=(const SubData &rhs)
  {
    if (this != &rhs) {
      color = rhs.color;
      part  = rhs.part;
      type  = rhs.type;
    }
    return *this;
  }
#endif
};

class FreeFormData
{
public:
  bool         mode;
  PlacementEnc base;
  PlacementEnc justification;
#ifdef OPEREQ
  FreeFormData &operator=(const FreeFormData &rhs)
  {
    if (this != &rhs) {
      mode          = rhs.mode;
      base          = rhs.base;
      justification = rhs.justification;
    }
    return *this;
  }
#endif
};

class ConstrainData
{
public:
  enum PliConstrain {
    PliConstrainArea,
    PliConstrainSquare,
    PliConstrainWidth,
    PliConstrainHeight,
    PliConstrainColumns
  } type;
  float        constraint;
#ifdef OPEREQ
  ConstrainData &operator=(const ConstrainData &rhs)
  {
    if (this != &rhs) {
      type       = rhs.type;
      constraint = rhs.constraint;
    }
    return *this;
  }
#endif
};

class SepData
{
public:
  float   thickness;  // in units
  QString color;
  float   margin[2];  // in units
  SepData()
  {
    thickness = 0.125;
    margin[0] = 0;
    margin[1] = 0;
  }
#ifdef OPEREQ
  SepData &operator=(const SepData &rhs)
  {
    if (this != &rhs) {
      thickness = rhs.thickness;
      color     = rhs.color;
      margin[0] = rhs.margin[0];
      margin[1] = rhs.margin[1];
    }
    return *this;
  }
#endif
};

  
#endif
