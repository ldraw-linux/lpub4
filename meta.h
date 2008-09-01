
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
 * This file describes a set of classes that implement a parse tree for
 * all the meta-commands that LPub supports.  Action metas such as STEP,
 * ROTSTEP, CALLOUT BEGIN, etc. return special return codes.  Configuration
 * metas that imply no action, but specify data for later use, retain
 * the onfiguration information, and return a generic OK return code.
 *
 * The top of tree is the Meta class that is the interface to the traverse
 * function that walks the LDraw model higherarchy.  Meta also tracks
 * locations in files like topOfModel, bottomOfModel, bottomOfSteps,topOfRange,
 * bottomOfRange, topOfStep, bottomOfStep, etc.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef META_H
#define META_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QRegExp>
#include <QTextStream>
#include "where.h"
#include "metatypes.h"
#include "resolution.h"

class QTextStream;

class Meta;
class BranchMeta;

enum Rc {
         InvalidLDrawLineRc = -3,
         RangeErrorRc = -2,
         FailureRc = -1,
         OkRc = 0,
         StepRc,
         RotStepRc,

         StepGroupBeginRc,
         StepGroupDividerRc,
         StepGroupEndRc,

         CalloutBeginRc,
         CalloutPointerRc,
         CalloutDividerRc,
         CalloutEndRc,             

         ClearRc,
         BufferStoreRc,
         BufferLoadRc,
         MLCadSkipBeginRc,
         MLCadSkipEndRc,
         MLCadGroupRc,

         PliBeginIgnRc,
         PliBeginSub1Rc,
         PliBeginSub2Rc,
         PliEndRc,

         PartBeginIgnRc,
         PartEndRc,

         BomBeginIgnRc,
         BomEndRc,

         ReserveSpaceRc,
         PictureAsStep,

         GroupRemoveRc,
         RemoveGroupRc,
         RemovePartRc,
         RemoveNameRc,

         SynthBeginRc,
         SynthEndRc,

         ResolutionRc,
         
         InsertRc,
         InsertPageRc,
         InsertCoverPageRc,

         EndOfFileRc,
};

#define DEFAULT_MARGIN  0.1
#define DEFAULT_MARGINS DEFAULT_MARGIN,DEFAULT_MARGIN
#define DEFAULT_MARGIN_RANGE 0.0,100.0
#define DEFAULT_THICKNESS 1.0/32

/*
 * This abstract class is the root of all meta-command parsing
 * objects.  Each parsing object knows how to intialize itself,
 * parse from the current argv through the end of args, perform
 * a preamble match (used for recognizing meta commands when
 * making changes to the LDraw file, and it also knows how to
 * document itself.
 */

class AbstractMeta
{
public:
  int       pushed;
  bool      global;

                     AbstractMeta()
                     {
                       pushed = 0;
                       global = false;
                     }
  virtual           ~AbstractMeta() { preamble.clear(); }
  QString            preamble;
  
  /* Initialize thyself */

  virtual void init(BranchMeta *parent, 
                    QString name);

  /* Parse thyself */

  virtual Rc parse(QStringList &argv, int index, Where &here) = 0;

  /* Compare argvs against matching string */

  virtual bool preambleMatch(QStringList &, int index, QString &match) = 0;

  /* Document thyself */
  virtual void doc(QTextStream &out, QString preamble);

  virtual void convert(float factor) { factor = factor; }

  /* Undo a push */

  virtual void pop() = 0;
 };

/*------------------------*/

/*
 * This abstract class represents terminal nodes of a meta command
 */

class LeafMeta : public AbstractMeta {
public:
  Where     _here[2];

  LeafMeta()
  { 
    pushed = 0;
    global = false;
  }
  const Where &here()
  {
    return _here[pushed];
  }

  void pop()
  {
    pushed = 0;
  }

  bool preambleMatch(QStringList &argv, int index, QString &match) 
  {
    if (argv.size() != index) {
    }
    return preamble == match;
  }
  
  virtual QString format(bool local, bool global) = 0;
  
  virtual QString format(bool local, bool global, QString);

  virtual void doc(QTextStream &out, QString preamble)  { out << preamble; }
};

/*
 * This class represents non-terminal keywords in a syntax
 */

class BranchMeta : public AbstractMeta {
public:

  /* 
   * This is a list of the possible keywords for this token in
   * the syntax
   */

  QHash<QString, AbstractMeta *> list;
  BranchMeta() {}
  virtual ~BranchMeta();
  
  virtual Rc parse(QStringList &argv, int index, Where &here);
  virtual bool    preambleMatch(QStringList &argv, int index, QString &_preamble);
  virtual void    doc(QTextStream &out, QString preamble);
  virtual void    convert(float factor);
  virtual void    pop();

  BranchMeta &operator= (const BranchMeta & /* unused */)
  {
    return *this;
  }
};

/*
 * This leaf parsing class returns a special return code, typically
 * used for action meta-commands like LPUB PLI BEGIN SUB, or LPUB
 * CALLOUT BEGIN
 */

class RcMeta : public LeafMeta {
public:
  Rc rc;  // the return code
  RcMeta() 
  {
  }
  virtual ~RcMeta() {}
  virtual void    init(BranchMeta *parent, const QString name, Rc _rc=OkRc);
  virtual Rc parse(QStringList &argv, int index, Where &here);
  virtual QString format(bool,bool) { QString foo; return foo; }
  virtual void    doc(QTextStream &out, QString preamble);
};

/*
 * This leaf meta is used when the the rest of the input
 * is a simple integer
 */
  
class IntMeta : public LeafMeta {
private:
  int       _value[2];
  Rc   rc;
  int       base;  // 10 or 16?
public:
  int       _min, _max;
  int       value()
  {
    return _value[pushed];
  }
  IntMeta()
  {
    _value[0] = 0;
    _value[1] = 0;
    _min = 0;
    _max = 0;
  }
  void setRange(int min, int max)
  {
    _min = min;
    _max = max;
  }
  virtual ~IntMeta() {}
  virtual void init(BranchMeta *parent, 
                    const QString name, 
                    Rc _rc=OkRc);
  virtual Rc parse(QStringList &argv, int index, Where &here);
          QString format(bool,bool);
  virtual void    doc(QTextStream &out, QString preamble);
};
/*
 * This is a leaf object for floating point number */
  
class FloatMeta : public LeafMeta {
protected:
  float     _value[2];
  Rc   rc;
public:
  float     _min, _max;
  int       _fieldWidth;
  int       _precision;
  QString   _inputMask;
  virtual float value()
  {
    return _value[pushed];
  }
  virtual void setValue(float value)
  {
    _value[pushed] = value;
  }
  void setRange(float min, float max)
  {
    _min = min;
    _max = max;
  }
  void setFormats(
    int fieldWidth,
    int precision,
    QString inputMask)
  {
    _fieldWidth = fieldWidth;
    _precision  = precision;
    _inputMask  = inputMask;
  }
  FloatMeta()
  {
    _value[0] = 0;
    _min = 0;
    _max = 0;
    _fieldWidth = 6;
    _precision = 4;
    _inputMask = "9.9999";
  }
  virtual ~FloatMeta() {}
  virtual void    init(BranchMeta *parent, 
                       const QString name, 
                       Rc _rc=OkRc);
  virtual Rc parse(QStringList &argv, int index, Where &here);
          QString format(bool,bool);
  virtual void    doc(QTextStream &out, QString preamble);
};

/* This is a leaf object class for two floating point numbers */
  
class FloatPairMeta : public LeafMeta {
protected:
  float     _value[2][2];
  float     _min,_max;
  Rc   rc;
public:
  int       _fieldWidth;
  int       _precision;
  QString   _inputMask;

  virtual float value(int i)
  {
    return _value[pushed][i];
  }
  void setValue(float v, int which)
  {
    _value[pushed][which] = v;
  }
  void setValues(float v1, float v2)
  {
    _value[pushed][0] = v1;
    _value[pushed][1] = v2;
  }
  void setRange(
    float min,
    float max)
  {
    _min = min;
    _max = max;
  }
  void setFormats(
    int fieldWidth,
    int precision,
    QString inputMask)
  {
    _fieldWidth = fieldWidth;
    _precision  = precision;
    _inputMask  = inputMask;
  }
  FloatPairMeta()
  {
    _value[0][0] = 0;
    _value[0][1] = 0;
    _min = 0;
    _max = 0;
    _fieldWidth = 6;
    _precision = 4;
    _inputMask = "9.9999";
  }
  virtual ~FloatPairMeta() {};
  virtual void    init(BranchMeta *parent, 
                    const QString name,
                    Rc _rc=OkRc);
  virtual Rc parse(QStringList &argv, int index, Where &here);
  virtual QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

/*
 * This leaf meta is used when using real world measuring units
 */
  
class UnitMeta : public FloatMeta {
public:
  virtual float valueUnit()
  {
    return FloatMeta::value();
  }
  virtual void setValueUnit(float value)
  {
    _value[pushed] = value;
  }
  virtual void setValueInches(float value)
  {
    if (resolutionType == DPI) {
      _value[pushed] = value;
    } else {
      _value[pushed] = inches2centimeters(value);
    }
  }
  virtual float value()
  {
    return _value[pushed]*resolution;
  }
  virtual void convert(float factor)
  {
    _value[0] *= factor;
  }
  UnitMeta()
  {
    _value[0] = 0;
    _min = 0;
    _max = 0;
    _fieldWidth = 6;
    _precision = 4;
    _inputMask = "9.9999";
  }
  QString format(bool, bool);
  virtual ~UnitMeta() {}
};
  
class UnitsMeta : public FloatPairMeta {
private:
public:
  virtual float valueUnit(int which)
  {
    return FloatPairMeta::value(which);
  }
  virtual void  setValueUnit(float v, int which)
  {
    FloatPairMeta::setValue(v,which);
  }
  virtual void  setValueUnits(float v1, float v2)
  {
    FloatPairMeta::setValues(v1,v2);
  }
  virtual void setValueInches(float v, int which)
  {
    if (resolutionType == DPI) {
      _value[pushed][which] = v;
    } else {
      _value[pushed][which] = inches2centimeters(v);
    }
  }
  virtual void setValuesInches(float v1, float v2) {
    if (resolutionType == DPI) {
      setValueUnits(v1,v2);
    } else {
      setValueUnits(inches2centimeters(v1),
                    inches2centimeters(v2));
    }
  }
  virtual float value(int which)
  {
    return _value[pushed][which]*resolution;
  }
  virtual void convert(float factor) {
    _value[0][0] *= factor;
    _value[0][1] *= factor;
  }
  UnitsMeta()
  {
    _value[0][0] = 0;
    _value[0][1] = 0;
    _min = 0;
    _max = 0;
    _fieldWidth = 6;
    _precision = 2;
    _inputMask = "999.99";
  }
  virtual ~UnitsMeta() {}
//  virtual QString text(bool abbrev);
};
  
class MarginsMeta : public UnitsMeta {
private:
public:
  MarginsMeta()
  {
    if (resolutionType == DPI) {
      _value[0][0] = DEFAULT_MARGIN;
      _value[0][1] = DEFAULT_MARGIN;
    } else {
      _value[0][0] = inches2centimeters(DEFAULT_MARGIN);
      _value[0][1] = inches2centimeters(DEFAULT_MARGIN);
    }
    _min = 0;
    _max = 100;
    _fieldWidth = 6;
    _precision = 4;
    _inputMask = "9.9999";
  }
  virtual ~MarginsMeta() {}
};


/* This leaf class is used for strings */

class StringMeta : public LeafMeta {
private:
protected:
  QString _value[2];
  Rc rc;
  QString delim;
  
public:
  QString value()
  {
    return _value[pushed];
  }
  void setValue(QString value)
  {
    _value[pushed] = value;
  }
  StringMeta() 
  {
  }
  virtual ~StringMeta() {}
  virtual void    init(BranchMeta *parent,
                    QString name, 
                    Rc _rc=OkRc, 
                    QString _delim="\"");
  virtual Rc parse(QStringList &argv, int index, Where &here);
          QString format(bool,bool);
  void    pop() 
  { 
    if (pushed) {
      _value[1].clear(); 
      pushed = 0;
    }
  }
  virtual void doc(QTextStream &out, QString preamble);
};

/* This leaf class is for multiple strings */

class StringListMeta : public LeafMeta {
private:
  QStringList _value[2];
  QString     delim;
  Rc     rc;
public:
  QString value(int i)
  {
    if (i >= _value[pushed].size()) {
      i = _value[pushed].size() - 1;
    }
    if (i >= 0) {
      return _value[pushed][i];
    } else {
      return "";
    }
  }
  void setValue(QString value)
  {
    _value[0] << value;
  }
  StringListMeta() 
  { 
  }
  virtual ~StringListMeta() {}
  virtual void init(BranchMeta *parent,
                    QString name,
                    Rc _rc=OkRc, 
                    QString _delim = "\"");
  virtual Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  void    pop() 
  { 
    if (pushed) {
      _value[1].clear(); 
      pushed = 0;
    }
  }
  virtual void doc(QTextStream &out, QString preamble);
};

/* This leaf class is used for fonts */

class FontMeta : public StringMeta {
private:
  
public:
  QString valueUnit()
  {
    return _value[pushed];
  }
  void setValueUnit(QString value)
  {
    _value[pushed] = value;
  }
  void setValueInches(QString value)
  {
    if (resolutionType == DPI) {
      _value[pushed] = value;
    } else {
      // 0 family
      // 1 pointSizeF
      // 2 pixelSize
      // 3 styleHint
      // 4 weight
      // 5 underline
      // 6 strikeout
      // 7 strikeOut
      // 8 fixedPitch
      // 9 rawMode

      QStringList list = _value[pushed].split(",");

      // points = 1/72
      // height = points/72 

      float units;
      units = list[1].toFloat()/72.0;  // now we have inches
      units *= 02.54;

      list[1] = QString("%1") .arg(int(units+0.5));

      QString pixels = list.join(",");

      _value[pushed] = pixels;
    }
  }
  QString value()
  {
    // 0 family
    // 1 pointSizeF
    // 2 pixelSize
    // 3 styleHint
    // 4 weight
    // 5 underline
    // 6 strikeout
    // 7 strikeOut
    // 8 fixedPitch
    // 9 rawMode

    QStringList list = _value[pushed].split(",");

    // points = 1/72
    // height = points/72 

    float units;
    units = list[1].toFloat()/72.0;  // now we have inches

    switch (resolutionType) {
      case DPI:
      break;
      case DPCM:
        units *= 02.54;
      break;
    }
    units *= resolution;

    list[1] = QString("%1") .arg(int(units+0.5));

    QString pixels = list.join(",");

    return pixels;
  }
  void setValue(QString value)
  {
    _value[pushed] = value;
  }
  FontMeta() 
  {
    _value[0] = "Arial,16,-1,75,0,0,0,0,0";
  }
  virtual ~FontMeta() {}
};

/* This leaf class is used for fonts */

class FontListMeta : public LeafMeta {
private:
  QStringList _value[2];
  QString     delim;
  Rc          rc;
public:
  QString valueUnit(int i)
  {
    if (i >= _value[pushed].size()) {
      i = _value[pushed].size() - 1;
    }
    if (i >= 0) {
      return _value[pushed][i];
    } else {
      return "";
    }
  }
  void setValueUnit(QString value)
  {
    _value[pushed] << value;
  }
  QString value(int i)
  {
    // 0 family
    // 1 pointSizeF
    // 2 pixelSize
    // 3 styleHint
    // 4 weight
    // 5 underline
    // 6 strikeout
    // 7 strikeOut
    // 8 fixedPitch
    // 9 rawMode

    if (i > _value[pushed].size()) {
      i = _value[pushed].size() - 1;
    }
    if (i < 0) {
      return "";
    }

    QStringList list = _value[pushed][i].split(",");

    // points = 1/72
    // height = points/72 

    float units;
    units = list[1].toFloat()/72.0;  // now we have inches

    switch (resolutionType) {
      case DPI:
      break;
      case DPCM:
        units *= 02.54;
      break;
    }
    units *= resolution;

    list[1] = QString("%1") .arg(int(units+0.5));

    QString pixels = list.join(",");

    return pixels;
  }
  FontListMeta() {}
  virtual ~FontListMeta() {}
  virtual void init(BranchMeta *parent,
                    QString name,
                    Rc _rc=OkRc, 
                    QString _delim = "\"");
  virtual Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  void    pop() 
  { 
    if (pushed) {
      _value[1].clear(); 
      pushed = 0;
    }
  }
  virtual void doc(QTextStream &out, QString preamble);
};

/* This leaf is to catch booleans (TRUE or FALSE) */

class BoolMeta : public LeafMeta {
private:
  bool  _value[2];
public:
  bool  value()
  {
    return _value[pushed];
  }
  void setValue(bool value)
  {
    _value[pushed] = value;
  }
  BoolMeta () 
  {
  }
  virtual ~BoolMeta() {}
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

/* This class is used to parse placement information */

class PlacementMeta  : public LeafMeta
{
private:
  PlacementData _value[2];
public:
  PlacementData value()
  {
    return _value[pushed];
  }
  void setValue(PlacementData &value)
  {
    _value[pushed] = value;
  }
  void setValue(
    PlacementEnc   placement, 
    PlacementEnc   justification, 
    PlacementType  relativeTo, 
    PrepositionEnc preposition)
  {
    _value[pushed].placement = placement;
    _value[pushed].justification = justification;
    _value[pushed].relativeTo = relativeTo;
    _value[pushed].preposition = preposition;
  }
  void setValue(
    PlacementEnc   placement, 
    PlacementType  relativeTo, 
    PrepositionEnc preposition)
  {
    _value[pushed].placement = placement;
    _value[pushed].relativeTo = relativeTo;
    _value[pushed].preposition = preposition;
  }
  void setValue(int offset[2])
  {
    _value[pushed].offsets[0] = offset[0];
    _value[pushed].offsets[1] = offset[1];
  }
  PlacementMeta();
  virtual ~PlacementMeta() {}
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  QString formatOffset(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

/* This class is used to parse background data */

class BackgroundMeta : public LeafMeta
{
private:
  BackgroundData _value[2];
public:
  BackgroundData value()
  {
    return _value[pushed];
  }
  void setValue(BackgroundData &value)
  {
    _value[pushed] = value;
  }
  void setValue(
    Background type, 
    QString string)
  {
    _value[pushed].type = type;
    _value[pushed].string = string;
  }
  void setValue(
    Background type)
  {
    _value[pushed].type = type;
  }
  BackgroundMeta() 
  { 
  }
  virtual ~BackgroundMeta() {}
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  void    pop()
  {
    if (pushed) {
      _value[1].string.clear();
      pushed = 0;
    }
  }
  virtual void doc(QTextStream &out, QString preamble);
  virtual QString text();
};

/* This leaf class is used to parse border metas */

class BorderMeta : public LeafMeta
{
private:
  BorderData _value[2];  // come of this is in units
public:
  BorderData value()
  {
    BorderData borderData = _value[pushed];
    borderData.margin[0]*=resolution;
    borderData.margin[1]*=resolution;
    borderData.thickness*=resolution;
    return borderData;
  }
  BorderData valueUnit()
  {
    return _value[pushed];
  }
  void setValueUnit(BorderData &borderData)
  {
    _value[pushed] = borderData;
  }
  void setValueInches(BorderData &borderData)
  {
    if (resolutionType == DPCM) {
      _value[pushed] = borderData;
    } else {
      BorderData cmBorder = borderData;
      cmBorder.thickness = inches2centimeters(cmBorder.thickness);
      cmBorder.margin[0] = inches2centimeters(cmBorder.margin[0]);
      cmBorder.margin[1] = inches2centimeters(cmBorder.margin[1]);
    }
  }
  virtual void convert(float factor)
  {
    _value[0].thickness *= factor;
    _value[0].margin[0] *= factor;
    _value[0].margin[1] *= factor;
  }
  BorderMeta() 
  {
    if (resolutionType == DPI) {
      _value[0].thickness = DEFAULT_THICKNESS;
    } else {
      _value[0].thickness = inches2centimeters(DEFAULT_THICKNESS);
    }
    _value[0].margin[0] = 0;
    _value[0].margin[1] = 0;
  }
  virtual ~BorderMeta() { }
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  void    pop() 
  { 
    if (pushed) {
      _value[1].color.clear();
      pushed = 0;
    }
  }
  virtual void doc(QTextStream &out, QString preamble);
  virtual QString text();
};

/*
 * This class parses LPub POINTER meta commands
 */

class PointerMeta : public LeafMeta
{
private:
  PointerData _value[2];
public:
  PointerData valueUnit()
  {
    return _value[pushed];
  }
  PointerData value()
  {
    PointerData pointerData = _value[pushed];
    pointerData.base *= resolution;
    return pointerData;
  }
  void setValue(
    PlacementEnc placement, 
    float loc, 
    float base, 
    float x, 
    float y)
  {
    _value[pushed].placement = placement;
    _value[pushed].loc       = loc;
    _value[pushed].base      = base/resolution;
    _value[pushed].x         = x;
    _value[pushed].y         = y;
  }
  void setValueUnit(
    PlacementEnc placement, 
    float loc, 
    float base, 
    float x, 
    float y)
  {
    _value[pushed].placement = placement;
    _value[pushed].loc       = loc;
    _value[pushed].base      = base;
    _value[pushed].x         = x;
    _value[pushed].y         = y;
  }
  virtual void convert(float factor)
  {
    _value[0].base *= factor;
  }
  PointerMeta();
  virtual ~PointerMeta() {}
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

/*
 * This class detects FREEFORM keywords */

class FreeFormMeta : public LeafMeta
{
private:
  FreeFormData _value[2];
public:
  FreeFormData value()
  {
    return _value[pushed];
  }
  FreeFormMeta();
  virtual ~FreeFormMeta() {}
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

/*
 * This class parses (Vertical|Horizontal)
 */

class AllocMeta : public LeafMeta
{
private:
  AllocEnc type[2];
public:
  AllocEnc value()
  {
    return type[pushed];
  }
  void setValue(AllocEnc value)
  {
    type[pushed] = value;
  }
  AllocMeta();
  virtual ~AllocMeta() {}
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

/*
 * INSERT
 *   ((TOP|BOTTOM) (LEFT|CENTER|RIGHT)|
 *     (LEFT|RIGHT) (TOP|CENTER|BOTTOM)|
 *     (TOP_LEFT|TOP_RIGHT|BOTTOM_RIGHT|BOTTOM_LEFT))
 *       (PAGE|ASSEM|STEP_NUMBER|STEP_GROUP|....) (INSIDE|OUTSIDE) (OFFSET X Y) (MARGIN X Y)
 *         (PICTURE "name" (SCALE x))|
 *          TEXT|
 *          ARROW HX HY TX TY|
 *          BOM)
 *
 *              . ' (hox hoy) (hafting outside)
 *          . '  /
 *      . '     /
 *   tx---------hix (hafting center)
 *      ` .     \ 
 *          ` .  \
 *              ` .
 */

class InsertMeta : public LeafMeta
{
private:
  InsertData _value[2];
public:
  InsertMeta() 
  {
  }
  InsertData &value()
  {
    return _value[pushed];
  }
  virtual ~InsertMeta() {}
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

class AlignmentMeta : public LeafMeta
{
private:
  Qt::Alignment _value[2];
public:
  AlignmentMeta()
  {
    _value[0] = Qt::AlignLeft;
  }
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

class TextMeta : public BranchMeta
{
public:
  FontMeta       font;
  StringMeta     color;
  AlignmentMeta  alignment;

  TextMeta();
  virtual ~TextMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

class ArrowHeadMeta : LeafMeta
{
private:
  qreal _value[2][4];
public:
  ArrowHeadMeta()
  {
    _value[0][0] = 0.0;
    _value[0][1] = 0.25;
    _value[0][2] = 3.0/8;
    _value[0][3] = 0.25;
  }
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

class ArrowEndMeta : public LeafMeta
{
  bool _value[2]; // false = square, true = round
public:
  ArrowEndMeta()
  {
    _value[pushed] = false;
  }
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

/*
 * This meta parses DIVIDER keywords
 */

class SepMeta : public LeafMeta
{
private:
  SepData _value[2];
public:
  SepData valueUnit()
  {
    return _value[pushed];
  }
  SepData value()
  {
    SepData sepData = _value[pushed];
    sepData.thickness *= resolution;
    sepData.margin[0] *= resolution;
    sepData.margin[1] *= resolution;
    return sepData;
  }
  void setValueUnit(QString color, 
                float thickness,
                float margin0,
                float margin1)
  {
    _value[pushed].color = color;
    _value[pushed].thickness = thickness;
    _value[pushed].margin[0] = margin0;
    _value[pushed].margin[1] = margin1;
  }
  void setValueInches(QString color,
                      float thickness,
                      float margin0,
                      float margin1)
  {
    if (resolutionType == DPI) {
      thickness = inches2centimeters(thickness);
      margin0   = inches2centimeters(margin0);
      margin1   = inches2centimeters(margin1);
    }
    _value[pushed].color = color;
    _value[pushed].thickness = thickness;
    _value[pushed].margin[0] = margin0;
    _value[pushed].margin[1] = margin1;
  }
  void setValueUnit(SepData &sepData)
  {
    _value[pushed] = sepData;
  }
  virtual void convert(float factor)
  {
    _value[0].thickness *= factor;
    _value[0].margin[0] *= factor;
    _value[0].margin[1] *= factor;
  }
  SepMeta();
  virtual ~SepMeta() { }
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool local,bool);
  void pop()
  {
    if (pushed) {
      _value[1].color.clear();
      pushed = 0;
    }
  }
  virtual void doc(QTextStream &out, QString preamble);
};

class CalloutCsiMeta : public BranchMeta
{
public:
  PlacementMeta placement;
  MarginsMeta   margin;
  CalloutCsiMeta();
  virtual ~CalloutCsiMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

class CalloutPliMeta : public BranchMeta
{
public:
  PlacementMeta placement;
  MarginsMeta   margin;
  BoolMeta      perStep;
  CalloutPliMeta();
  virtual ~CalloutPliMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

class NumberMeta : public BranchMeta
{
public:
  int         number;
  FontMeta    font;
  StringMeta  color;
  MarginsMeta margin;
  void setValue(int _value)
  {
    number = _value;
  }
  NumberMeta();
  virtual ~NumberMeta() {}
  virtual void init(BranchMeta *parent, 
                    QString name);
};

class NumberPlacementMeta : public NumberMeta
{
public:
  PlacementMeta  placement;
  NumberPlacementMeta();
  virtual ~NumberPlacementMeta() {}
  virtual void init(BranchMeta *parent, 
                    QString name);
};

/*------------------------*/

class RemoveMeta : public BranchMeta
{
public:
  StringMeta group;
  StringMeta parttype;
  StringMeta partname;
  RemoveMeta() 
  {
  }
  virtual ~RemoveMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};
  
/* This class is used to parse PART IGNORE in PLI metas
 * do we want to try to use values instead of the PART
 * keyword
 */

class PartMeta : public BranchMeta
{
public:
  MarginsMeta margin;

  PartMeta();
  virtual ~PartMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/* 
 * This class is used to parse PLI substitute
 * commands 
 */

class SubMeta : public LeafMeta
{
private:
  SubData _value;
public:
  SubData value()
  {
    return _value;
  }
  SubMeta() 
  { 
  }
  virtual ~SubMeta() {}
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

/* This class parses PLI constraints */

class ConstrainMeta : public LeafMeta
{
private:
  ConstrainData _value[2];
public:
  ConstrainData valueUnit()
  {
    return _value[pushed];
  }
  ConstrainData value()
  {
    ConstrainData constrainData = _value[pushed];
    if (constrainData.type == PliConstrainWidth ||
        constrainData.type == PliConstrainHeight) {
      constrainData.constraint *= resolution;
    }
    return _value[pushed];
  }
  void setValueUnit(ConstrainData &value)
  {
    _value[pushed] = value;
  }
  virtual void convert(float factor)
  {
    if (_value[0].type == PliConstrainWidth ||
        _value[0].type == PliConstrainHeight) {
      _value[0].constraint *= factor;
    }
  }
  ConstrainMeta();
  virtual ~ConstrainMeta() {}
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

/*------------------------*/

class PliBeginMeta : public BranchMeta
{
public:
  RcMeta        ignore;
  SubMeta       sub;
  PliBeginMeta() 
  {
  }
  virtual ~PliBeginMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

// 0 LPUB PART BEGIN // (IGN|IGNORE)

class PartBeginMeta : public BranchMeta
{
public:
  RcMeta          ignore;
  PartBeginMeta() 
  {
  }
  virtual ~PartBeginMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

// 0 LPUB PART

class PartIgnMeta : public BranchMeta
{
public:
  PartBeginMeta begin;
  RcMeta        end;
  PartIgnMeta() 
  {
  }
  virtual ~PartIgnMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

class BomBeginMeta : public BranchMeta
{
public:
  RcMeta        ignore;
  BomBeginMeta() 
  {
  }
  virtual ~BomBeginMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/* This class is to parse MLCad's rotation step */

class RotStepMeta : public LeafMeta
{
private:
  RotStepData _value;
public:
  RotStepData value()
  {
    return _value;
  }
  void clear()
  {
    _value.type.clear();
  }
  RotStepMeta() { 
    _value.type.clear(); 
  }
  virtual ~RotStepMeta() {}
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  virtual void doc(QTextStream &out, QString preamble);
};

/* This class is to parse MLCad's buffer exchange */

class BuffExchgMeta : public LeafMeta
{
private:
  BuffExchgData _value;
public:
  QString value()
  {
    return _value.buffer;
  }
  BuffExchgMeta() 
  { 
  }
  virtual ~BuffExchgMeta() { }
  Rc parse(QStringList &argv, int index, Where &here);
  QString format(bool,bool);
  void    pop() { pushed = 0; }
  virtual void doc(QTextStream &out, QString preamble);
};

/*---------------------------------------------------------------
 * The Top Level LPub Metas
 *---------------------------------------------------------------*/

class PageMeta : public BranchMeta
{
public:
  // top    == top of page
  // bottom == bottom of page
  UnitsMeta      size;
  MarginsMeta    margin;
  BorderMeta     border;
  BackgroundMeta background;
  BoolMeta       dpn;
  BoolMeta       togglePnPlacement;
  NumberPlacementMeta number;
  NumberPlacementMeta instanceCount;
  StringListMeta subModelColor;

  PageMeta();
  virtual ~PageMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

class AssemMeta : public BranchMeta
{
public:
  // top    == last step
  // bottom == cur step
  MarginsMeta   margin;
  PlacementMeta placement;
  FloatMeta     modelScale;
  StringMeta    ldviewParms;
  StringMeta    ldgliteParms;
  AssemMeta();
  virtual ~AssemMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

class PliMeta  : public BranchMeta
{
public:
  BorderMeta     border;
  BackgroundMeta background;
  MarginsMeta    margin;
  NumberMeta     instance;
  NumberMeta     annotate;
  PlacementMeta  placement;
  ConstrainMeta  constrain;
  FloatMeta      modelScale;
  FloatPairMeta  angle;
  PartMeta       part;
  PliBeginMeta   begin;
  RcMeta         end;
  BoolMeta       includeSubs;
  BoolMeta       show;
  StringListMeta subModelColor;     // FIXME: we need a dialog for submodel level color
  FontListMeta   subModelFont;
  StringListMeta subModelFontColor;
  StringMeta     ldviewParms;
  StringMeta     ldgliteParms;
  BoolMeta       pack;
  BoolMeta       sort;

  PliMeta();
  virtual ~PliMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

class BomMeta  : public PliMeta
{
public:
  BomMeta();
  virtual ~BomMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

class CalloutMeta  : public BranchMeta
{
public:
  // top    == start of callout
  // bottom == end of callout
  // Placement needs to be outside callout begin/end

  PlacementMeta  placement;     // outside

  MarginsMeta    margin;
  CalloutCsiMeta csi;               
  CalloutPliMeta pli;
  NumberPlacementMeta stepNum;
  SepMeta        sep;
  FreeFormMeta   freeform;
  RcMeta         begin;
  RcMeta         divider;
  RcMeta         end;
  AllocMeta      alloc;
  AllocMeta      silent_alloc;
  NumberPlacementMeta instance;
  BorderMeta     border;
  BackgroundMeta background;
  PointerMeta    pointer;
  StringListMeta subModelColor;
  FontListMeta   subModelFont;
  StringListMeta subModelFontColor;
  CalloutMeta();
  virtual ~CalloutMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

class MultiStepMeta : public BranchMeta
{
public:
  // top    == start of multistep
  // bot    == bottom of multistep
  PlacementMeta  placement;
  MarginsMeta    margin;
  CalloutCsiMeta csi;
  CalloutPliMeta pli;
  NumberPlacementMeta stepNum;
  SepMeta        sep;
  FreeFormMeta   freeform;
  RcMeta         begin;
  RcMeta         divider;
  RcMeta         end;
  AllocMeta      alloc;
  FontListMeta   subModelFont;
  StringListMeta subModelFontColor;
  MultiStepMeta();
  virtual ~MultiStepMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

/*
 * Resolution meta
 */
  
class ResolutionMeta : public LeafMeta {
private:
public:
  ResolutionType type()
  {
    return resolutionType;
  }
  float value()
  {
    return resolution;
  }
  void setValue(float _resolution)
  {
    resolution = _resolution;
  }
  void setValue(ResolutionType _type, float _resolution)
  {
    resolutionType = _type;
    resolution     = _resolution;
  }
  float ldu()
  {
    if (resolutionType == DPI) {
      return 1.0/64;
    } else {
      return inches2centimeters(1.0/64.0);
    }
  }
  ResolutionMeta() 
  {
  }
  virtual ~ResolutionMeta() {}
  virtual void init(BranchMeta *parent, 
                    QString name);
  virtual Rc parse(QStringList &argv, int index, Where &here);
          QString format(bool,bool);
  virtual void    doc(QTextStream &out, QString preamble);
};

class LPubMeta : public BranchMeta
{
public:
  ResolutionMeta resolution;
  PageMeta       page;
  AssemMeta      assem;
  NumberPlacementMeta stepNumber;
  CalloutMeta    callout;
  MultiStepMeta  multiStep;
  PliMeta        pli;
  PliMeta        bom;
  RemoveMeta     remove;
  FloatMeta      reserve;
  PartIgnMeta    partSub;
  InsertMeta     insert;
  LPubMeta();
  virtual ~LPubMeta() {};
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

class MLCadMeta : public BranchMeta
{
public:
  RcMeta     MLCadSB;
  RcMeta     MLCadSE;
  StringMeta MLCadGrp;
  MLCadMeta() {}
  virtual ~MLCadMeta() {}
  virtual void init(BranchMeta *parent, QString name);
  virtual Rc parse(QStringList &argv, int index, Where &here);
};

/*------------------------*/

class LSynthMeta : public BranchMeta
{
public:
  RcMeta        begin;
  RcMeta        end;
  RcMeta        synthesized;
  virtual ~LSynthMeta() {}
  virtual void init(BranchMeta *parent, QString name);
};

/*------------------------*/

class SubmodelStack
{
public:
  SubmodelStack(QString _modelName, int _lineNumber, int _stepNumber)
  {
    modelName = _modelName;
    lineNumber = _lineNumber;
    stepNumber = _stepNumber;
  }
  SubmodelStack()
  {
    modelName = "undefined";
    lineNumber = 0;
    stepNumber = 0;
  }
  QString modelName;
  int     lineNumber;
  int     stepNumber;
};

class Meta : public BranchMeta
{
public:
  LPubMeta      LPub;
  RcMeta        step;
  RcMeta        clear;
  RotStepMeta   rotStep;
  BuffExchgMeta bfx;
  MLCadMeta     MLCad;
  LSynthMeta    LSynth;

  QList<SubmodelStack>  submodelStack;

           Meta();
  virtual ~Meta();
          Rc    parse(QString &line, Where &here);
          bool  preambleMatch(QString &line, QString &preamble);
  virtual void  init(BranchMeta *parent, QString name);
  virtual void  pop();
  void  doc();
private:
  void mkargv(QString &input,QStringList &output, Where here);
};

#endif
