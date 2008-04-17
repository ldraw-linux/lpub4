
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

enum AllocEnc {
  Horizontal = 0,
  Vertical
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
  StepType,
  RangeType,
  ReserveType,
  NumRelatives
};

enum Background {
  BgTransparent,
  BgImage,
  BgColor,
  BgSubmodelColor
};

enum Border {
  BdrNone,
  BdrSquare,
  BdrRound
};

enum PliConstrain {
  PliConstrainArea,
  PliConstrainSquare,
  PliConstrainWidth,
  PliConstrainHeight,
  PliConstrainColumns
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
};

class BuffExchgData
{
public:
  QString buffer;
  QString type;
};

class PlacementData
{
public:
  PlacementEnc   placement;
  PlacementEnc   justification;
  PlacementType  relativeTo;
  PrepositionEnc preposition;
  float offsets[2];
};

class BackgroundData
{
public:
  Background type;
  QString    string;
  bool       stretch;
};

class BorderData
{
public:
  Border  type;
  QString color;
  float   thickness;  // in units 
  float   radius;     // in units
  float   margin[2];  // in units
};

class SubData
{
public:
  QString color;
  QString part;
  int     type;
};

class PointerData
{
public:
  PlacementEnc placement;
  float loc;  // fraction of side/top/bottom o callout
  float x;    // fraction of CSI size
  float y;
  float base; // in units
};

class FreeFormData
{
public:
  bool         mode;
  PlacementEnc base;
  PlacementEnc justification;
};

class ConstrainData
{
public:
  PliConstrain type;
  float        constraint;
};

class SepData
{
public:
  float   thickness;  // in units
  QString color;
  float   margin[2];  // in units
};
  
#endif
