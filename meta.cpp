
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
 * locations in files like topOfModel, bottomOfModel, topOfRanges,topOfRange,
 * bottomOfRange, topOfStep, bottomOfStep, etc.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include <QtGui>
#include <QTextStream>
#include "meta.h"
#include "lpub.h"

/* The token map translates known keywords to values 
 * used by LPub to identify things like placement and such
 */

QHash<QString, int> tokenMap;
void AbstractMeta::init(
  BranchMeta *parent, 
  QString name)
{
  preamble           = parent->preamble + name + " ";
  parent->list[name] = this;
}

void AbstractMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble;
}

BranchMeta::~BranchMeta()
{
  list.clear();
}

Rc BranchMeta::parse(QStringList &argv, int index, Where &here)
{
  Rc rc;
  int     offset;

  if (index < argv.size()) {

    QString foo = argv[index];

    /* Find out if the current argv explicitly matches any of the
     * keywords known to be valid at this point in the meta command */

    QHash<QString, AbstractMeta *>::iterator i = list.find(argv[index]);

    if (i != list.end()) {

      /* We found a match */

      offset = 1;
      rc = OkRc;

      if (argv.size() - index > 1) {
        if (i.value()) {
          if (argv[index+1] == "LOCAL") {
            i.value()->pushed = 1;
            offset = 2;
          }
          if (index + offset >= argv.size()) {
            rc = FailureRc;
          }
        }
      }

      /* Now parse the rest of the argvs */

      if (rc == OkRc) {
        return i.value()->parse(argv,index+offset,here);
      }
    } else {

      /* Failed an explicit match.  Lets try to see if the value
       * matches any of the keywords through regular expressions */

      // LPUB CALLOUT Vertical
      // LPUB CALLOUT LOCAL Vertical

      if (argv[index] == "LOCAL") {
        i.value()->pushed = 1;
        offset = 1;
      } else {
        offset = 0;
      }

      if (index + offset < argv.size()) {
        for (i = list.begin(); i != list.end(); i++) {
          QRegExp rx(i.key());
          if (argv[index + offset].contains(rx)) {

            /* Now parse the rest of the argvs */

            return i.value()->parse(argv,index+offset,here);
          }
        }
      }
    }
  }

  // syntax error 

  QStringList keys;
  QString     key;
  foreach (key, list.keys()) {
    keys << key;
  }
  return FailureRc;
}

/* 
 * Find out if the match string matches the syntax graph up until this
 * point
 */

bool BranchMeta::preambleMatch(QStringList &argv, int index, QString &match)
{
  QHash<QString, AbstractMeta *>::iterator i = list.find(argv[index]);
  if (i == list.end() || index == argv.size()) {
    return false;
  } else {
    return i.value()->preambleMatch(argv,index,match);
  }
}

/* 
 * Tell all the kidlets to convert
 */

void BranchMeta::convert(float factor)
{
  QString key;
  QStringList keys = list.keys();
  foreach(key, keys) {
    list[key]->convert(factor);
  }
}

/*
 * Output documentation information for this node in the syntax
 * graph
 */

void BranchMeta::doc(QTextStream &out, QString preamble)
{
  QString key;
  QStringList keys = list.keys();
  keys.sort();
  foreach(key, keys) {
    list[key]->doc(out, preamble + " " + key);
  }
}

void BranchMeta::pop()
{
  QString key;
  foreach (key,list.keys()) {
    list[key]->pop();
  }
}

/* ------------------ */
void RcMeta::init(BranchMeta *parent, const QString name, Rc _rc)
{ 
  AbstractMeta::init(parent,name);
  rc = _rc;
}
Rc RcMeta::parse(QStringList &argv, int index, Where &here)
{
  if (index != argv.size()) {
  }
  _here[pushed] = here;
  return rc;
}

void RcMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << endl;
}

/* ------------------ */

void IntMeta::init(
  BranchMeta *parent, 
  const QString name, 
  Rc _rc)
{
  AbstractMeta::init(parent,name);
  rc = _rc;
}
Rc IntMeta::parse(QStringList &argv, int index,Where &here)
{
  if (index == argv.size() - 1) {
    bool ok;
    int v;
    v = argv[index].toInt(&ok);
    if (ok) {
      if (v < _min || v > _max) {
        return RangeErrorRc;
      }
      _value[pushed] = v;
      _here[pushed] = here;
      return rc;
    }
  }
  QMessageBox::warning(NULL,
    QMessageBox::tr("LPub"),
    QMessageBox::tr("Expected a whole number but got \"%1\" %2") .arg(argv[index]) .arg(argv.join(" ")));

  return FailureRc;
}
QString IntMeta::format(bool local)
{
  QString foo;
  foo.arg(_value[pushed],0,base);
  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}

void IntMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <integer>" << endl;
}

/* ------------------ */

void FloatMeta::init(
  BranchMeta *parent, 
  const QString name, 
  Rc _rc)
{
  AbstractMeta::init(parent,name);
  rc = _rc;
}
Rc FloatMeta::parse(QStringList &argv, int index,Where &here)
{
  if (index == argv.size() - 1) {
    bool ok;
    float v = argv[index].toFloat(&ok);
    if (ok) {
      if (v < _min || v > _max) {
        return RangeErrorRc;
      }
      _value[pushed] = v;
      _here[pushed] = here;
      return rc;
    }
  }

  QMessageBox::warning(NULL,
    QMessageBox::tr("LPub"),
    QMessageBox::tr("Expected a floating point number but got \"%1\" %2") .arg(argv[index]) .arg(argv.join(" ")));

  return FailureRc;
}
QString FloatMeta::format(bool local)
{
  QString foo;
  foo = QString("%1") .arg(value(),_fieldWidth,'f',_precision);
  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}
void FloatMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <float>" << endl;
}

/* ------------------ */

QString UnitMeta::format(bool local)
{
  QString foo;
  foo = QString("%1") .arg(valueUnit(),_fieldWidth,'f',_precision);
  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}

/* ------------------ */

void FloatPairMeta::init(
  BranchMeta *parent, 
  const QString name,
  Rc _rc)
{
  AbstractMeta::init(parent,name);
  rc = _rc;
}
Rc FloatPairMeta::parse(QStringList &argv, int index,Where &here)
{
  if (argv.size() - index == 2) {
    bool ok[2];
    float v0 = argv[index  ].toFloat(&ok[0]);
    float v1 = argv[index+1].toFloat(&ok[1]);
    if (ok[0] && ok[1]) {
      if (v0 < _min || v0 > _max ||
          v1 < _min || v1 > _max) {
        return RangeErrorRc;
      }
      _value[pushed][0] = v0;
      _value[pushed][1] = v1;
      _here[pushed] = here;
      return rc;
    }
  }

  QMessageBox::warning(NULL,
    QMessageBox::tr("LPub"),
    QMessageBox::tr("Expected two floating point numbers but got \"%1\" \"%2\" %3") .arg(argv[index]) .arg(argv[index+1]) .arg(argv.join(" ")));

  return FailureRc;
}
QString FloatPairMeta::format(bool local)
{
  QString foo = QString("%1 %2") 
    .arg(_value[pushed][0],_fieldWidth,'f',_precision) 
    .arg(_value[pushed][1],_fieldWidth,'f',_precision);

  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}
void FloatPairMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <float> <float>" << endl;
}

/* ------------------ */
void StringMeta::init(
  BranchMeta *parent,
  QString name, 
  Rc _rc, 
  QString _delim)
{
  AbstractMeta::init(parent,name);
  rc = _rc;
  delim = _delim;
}
Rc StringMeta::parse(QStringList &argv, int index,Where &here)
{
  if (argv.size() - index == 1) {
    _value[pushed] = argv[index];
    _here[pushed] = here;
    return OkRc;
  }
  
  QMessageBox::warning(NULL,
    QMessageBox::tr("LPub"),
    QMessageBox::tr("Expected a string after \"%1\"") .arg(argv.join(" ")));

  return FailureRc;
}
QString StringMeta::format(bool local)
{
  QString foo = delim + _value[pushed] + delim;
  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}

void StringMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <\"string\">" << endl;
}

/* ------------------ */

void StringListMeta::init(
  BranchMeta *parent,
  QString name,
  Rc _rc, 
  QString _delim)
{
  AbstractMeta::init(parent,name);
  rc = _rc;
  delim = _delim;
}
Rc StringListMeta::parse(QStringList &argv, int index,Where &here)
{
  _value[pushed].clear();
  for (int i = index; i < argv.size(); i++) {
    _value[pushed] << argv[i];
  }
  _here[pushed] = here;
  return OkRc;
}
QString StringListMeta::format(bool local)
{
  QString foo;
  for (int i = 0; i < _value[pushed].size() ; i++) {
    foo += delim + _value[pushed][i] + delim + " ";
  }
  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}

void StringListMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <\"string\"> <\"string\"> ....." << endl;
}

/* ------------------ */

void FontListMeta::init(
  BranchMeta *parent,
  QString     name,
  Rc          _rc, 
  QString     _delim)
{
  AbstractMeta::init(parent,name);
  rc    = _rc;
  delim = _delim;
}
Rc FontListMeta::parse(QStringList &argv, int index,Where &here)
{
  _value[pushed].clear();
  for (int i = index; i < argv.size(); i++) {
    _value[pushed] << argv[i];
  }
  _here[pushed] = here;
  return OkRc;
}
QString FontListMeta::format(bool local)
{
  QString foo;
  for (int i = 0; i < _value[pushed].size() ; i++) {
    foo += delim + _value[pushed][i] + delim + " ";
  }
  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}

void FontListMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <\"string\"> <\"string\"> ....." << endl;
}

/* ------------------ */

Rc BoolMeta::parse(QStringList &argv, int index,Where &here)
{
  QRegExp rx("^(TRUE|FALSE)$");
  if (index == argv.size() - 1 && argv[index].contains(rx)) {
    _value[pushed] = argv[index] == "TRUE";
    _here[pushed] = here;
    return OkRc;
  }
  
  QMessageBox::warning(NULL,
    QMessageBox::tr("LPub"),
    QMessageBox::tr("Expected TRUE or FALSE \"%1\" %2") .arg(argv[index]) .arg(argv.join(" ")));

  return FailureRc;
}
QString BoolMeta::format(bool local)
{
  QString foo (_value[pushed] ? "TRUE" : "FALSE");

  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}

void BoolMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <TRUE|FALSE>" << endl;
}

/* ------------------ */ 

QString placementNames[] =
{
  "TOP_LEFT", "TOP", "TOP_RIGHT", "RIGHT",
  "BOTTOM_RIGHT", "BOTTOM", "BOTTOM_LEFT", "LEFT", "CENTER"
};

QString relativeNames[] = 
{
  "PAGE","ASSEM","MULTI_STEP","STEP_NUMBER","PLI","CALLOUT"
};

QString prepositionNames[] = 
{
  "INSIDE", "OUTSIDE"
};

PlacementMeta::PlacementMeta()
{
  _value[0].placement = TopLeft;
  _value[0].justification = Center;
  _value[0].relativeTo = PageType;
  _value[0].preposition = Inside;
  _value[0].offsets[0] = 0;
  _value[0].offsets[1] = 0;
}
Rc PlacementMeta::parse(QStringList &argv, int index,Where &here)
{
  PlacementEnc   _placement, _justification;
  PlacementType  _relativeTo;
  PrepositionEnc _preposition;
  float _offsets[2];
  Rc rc = FailureRc;
  QString foo;

  _placement     = _value[pushed].placement;
  _justification = _value[pushed].justification;
  _relativeTo    = _value[pushed].relativeTo;
  _preposition   = _value[pushed].preposition;
  _offsets[0]    = 0;
  _offsets[1]    = 0;

  if (argv[index] == "OFFSET") {
    index++;
    if (argv.size() - index == 2) {
      bool ok[2];
      argv[index  ].toFloat(&ok[0]);
      argv[index+1].toFloat(&ok[1]);
      if (ok[0] && ok[1]) {
        _value[pushed].offsets[0] = argv[index  ].toFloat(&ok[0]);
        _value[pushed].offsets[1] = argv[index+1].toFloat(&ok[1]);
        _here[pushed] = here;
        return OkRc;
      }
    }
  }  

  QRegExp rx("^(TOP|BOTTOM)$");
  if (argv[index].contains(rx)) {
    _placement = PlacementEnc(tokenMap[argv[index++]]);
    if (index < argv.size()) {
      rx.setPattern("^(LEFT|CENTER|RIGHT)$");
      if (argv[index].contains(rx)) {
        _justification = PlacementEnc(tokenMap[argv[index++]]);
        rc = OkRc;
      } else {
        rx.setPattern("^(PAGE|ASSEM|MULTI_STEP|STEP_NUMBER|PLI|CALLOUT)$");
        if (argv[index].contains(rx)) {
          rc = OkRc;
        }
      }
    } 
  } else {
    rx.setPattern("^(LEFT|RIGHT)$");
    if (argv[index].contains(rx)) {
      _placement = PlacementEnc(tokenMap[argv[index++]]);
      if (index < argv.size()) {
        rx.setPattern("^(TOP|CENTER|BOTTOM)$");
        if (argv[index].contains(rx)) {
          _justification = PlacementEnc(tokenMap[argv[index++]]);
          rc = OkRc;
        }
      }
    } else {
      rx.setPattern("^(TOP_LEFT|TOP_RIGHT|BOTTOM_LEFT|BOTTOM_RIGHT|CENTER)$");
      if (argv[index].contains(rx)) {
        _placement = PlacementEnc(tokenMap[argv[index++]]);
        _justification = Center;
        rc = OkRc;
      }
    }
  }

  if (rc == OkRc && index < argv.size()) {
    rx.setPattern("^(PAGE|ASSEM|MULTI_STEP|STEP_NUMBER|PLI|CALLOUT)$");
    if (argv[index].contains(rx)) {
      _relativeTo = PlacementType(tokenMap[argv[index++]]);
      if (_relativeTo == PageType) {
        _preposition = Inside;
      } else {
        _preposition = Outside;
      }
      if (index < argv.size()) {
        rx.setPattern("^(INSIDE|OUTSIDE)$");
        if (argv[index].contains(rx)) {
          _preposition = PrepositionEnc(tokenMap[argv[index++]]);
          rc = OkRc;
        } 
        if (argv.size() - index == 2) {
          bool ok[2];
          argv[index  ].toFloat(&ok[0]);
          argv[index+1].toFloat(&ok[1]);
          if (ok[0] && ok[1]) {
            _offsets[0] = argv[index  ].toFloat(&ok[0]);
            _offsets[1] = argv[index+1].toFloat(&ok[1]);
            rc = OkRc;
          }
        }
      } else {
        rc = OkRc;
      }
    }
    if (rc != OkRc) {
      return rc;
    }

    _value[pushed].placement = _placement;
    _value[pushed].justification = _justification;
    _value[pushed].relativeTo = _relativeTo;
    _value[pushed].preposition = _preposition;
    _value[pushed].offsets[0] = _offsets[0];
    _value[pushed].offsets[1] = _offsets[1];
    _here[pushed] = here;

    return OkRc;
  }
  return rc;
}

QString PlacementMeta::format(bool local)
{
  QString foo;

  switch (_value[pushed].placement) {
    case Top:
    case Bottom:
    case Right:
    case Left:
      foo = placementNames[_value[pushed].placement] + " "
          + placementNames[_value[pushed].justification] + " "
          + relativeNames [_value[pushed].relativeTo] + " "
          + prepositionNames[_value[pushed].preposition];
    break;
    default:
      foo = placementNames[_value[pushed].placement] + " "
          + relativeNames [_value[pushed].relativeTo] + " "
          + prepositionNames[_value[pushed].preposition];
  }
  if (_value[pushed].offsets[0] || _value[pushed].offsets[1]) {
    QString bar = QString(" %1 %2") .arg(_value[pushed].offsets[0]) 
                                    .arg(_value[pushed].offsets[1]);
    foo += bar;
  }
  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}

void PlacementMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " (TOP|BOTTOM) (LEFT|CENTER|RIGHT) (PAGE|ASSEM (INSIDE|OUTSIDE)|MULTI_STEP|STEP_NUMBER|PLI|CALLOUT)" << endl;
  out << preamble << " (LEFT|RIGHT) (TOP|CENTER|BOTTOM) (PAGE|ASSEM (INSIDE|OUTSIDE)|MULTI_STEP|STEP_NUMBER|PLI|CALLOUT)" << endl;
  out << preamble << " (TOP_LEFT|TOP_RIGHT|BOTTOM_LEFT|BOTTOM_RIGHT) (PAGE|ASSEM (INSIDE|OUTIDE)|MULTI_STEP|STEP_NUMBER|PLI|CALLOUT)" << endl;
}
/* ------------------ */ 

Rc BackgroundMeta::parse(QStringList &argv, int index,Where &here)
{
  Rc rc = FailureRc;

  if (argv.size() - index == 1) {
    if (argv[index] == "TRANS" || argv[index] == "TRANSPARENT") {
      _value[pushed].type = BgTransparent;
      rc = OkRc;
    } else if (argv[index] == "SUBMODEL_BACKGROUND_COLOR") {
      _value[pushed].type = BgSubmodelColor;
      rc = OkRc;
    } else {
      _value[pushed].type = BgImage;
      _value[pushed].string = argv[index];
      _value[pushed].stretch = false;
      rc = OkRc;
    }
  } else if (argv.size() - index == 2) {
    if (argv[index] == "COLOR") {
      _value[pushed].type = BgColor;
      _value[pushed].string = argv[index+1];
      rc = OkRc;
    } else if (argv[index] == "PICTURE") {
      _value[pushed].type = BgImage;
      _value[pushed].string = argv[index+1];
      _value[pushed].stretch = false;
      rc = OkRc;
    }
  } else if (argv.size() - index == 3) {
    if (argv[index] == "PICTURE" && argv[index+2] == "STRETCH") {
      _value[pushed].type = BgImage;
      _value[pushed].string = argv[index+1];
      _value[pushed].stretch = true;
      rc = OkRc;
    }
  }
  if (rc == OkRc) {
    _here[pushed] = here;
    return rc;
  } else {
      
    QMessageBox::warning(NULL,
      QMessageBox::tr("LPub"),
      QMessageBox::tr("Malformed background \"%1\"") .arg(argv.join(" ")));

    return FailureRc;
  }
}
QString BackgroundMeta::format(bool local)
{
  QString foo;
  switch (_value[pushed].type) {
    case BgTransparent:
      foo = "TRANSPARENT";
    break;
    case BgSubmodelColor:
      foo = "SUBMODEL_BACKGROUND_COLOR";
    break;
    case BgColor:
      foo = "COLOR \"" + _value[pushed].string + "\"";
    break;
    case BgImage:
      foo = "PICTURE \"" + _value[pushed].string + "\"";
      if (_value[pushed].stretch) {
        foo += " STRETCH";
      }
    break;
  }

  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}

void BackgroundMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " (TRANSPARENT|SUBMODEL_BACKGROUND_COLOR|COLOR <color>|PICTURE (STRETCH) <\"picture\">)" << endl;
}

QString BackgroundMeta::text()
{
  BackgroundData background = value();
  switch (background.type) {
    case BgTransparent:
      return "Transparent";
    break;
    case BgImage:
      return "Picture " + background.string;
    break;
    case BgColor:
      return "Color " + background.string;
    break;
    default:
    break;
  }
  return "Submodel level color";
}

/* ------------------ */ 

Rc BorderMeta::parse(QStringList &argv, int index,Where &here)
{
  Rc rc = FailureRc;

  if (argv[index] == "NONE" && argv.size() - index >= 1) {
    _value[pushed].type = BdrNone;
    index++;
    rc = OkRc;
  } else if (argv[index] == "SQUARE" && argv.size() - index >= 3) {
    bool ok;
    QString foo = argv[index+2];
    argv[index+2].toFloat(&ok);
    if (ok) {
      _value[pushed].type = BdrSquare;
      _value[pushed].color = argv[index+1];
      _value[pushed].thickness = argv[index+2].toFloat(&ok);
      index += 3;
      rc = OkRc;
    }
  } else if (argv[index] == "ROUND" && argv.size() - index >= 4) {
    bool ok[2];
    argv[index+2].toFloat(&ok[0]);
    argv[index+3].toInt(&ok[1]);
    if (ok[0] && ok[1]) {
      _value[pushed].type = BdrRound;
      _value[pushed].color = argv[index+1];
      _value[pushed].thickness = argv[index+2].toFloat(&ok[0]);
      _value[pushed].radius    = argv[index+3].toInt(&ok[0]);
      index += 4;
      rc = OkRc;
    }
  } 
  if (rc == OkRc && argv.size() - index == 3) {
    if (argv[index] == "MARGINS") {
      bool ok[2];
      argv[index + 1].toFloat(&ok[0]);
      argv[index + 2].toFloat(&ok[1]);
      if (ok[0] && ok[1]) {
        _value[pushed].margin[0] = argv[index + 1].toFloat(&ok[0]);
        _value[pushed].margin[1] = argv[index + 2].toFloat(&ok[1]);
        index += 3;
      } else {
        rc = FailureRc;
      }
    } else {
      rc = FailureRc;
    }
  }
  if (rc == OkRc) {
    _here[pushed] = here;
  }
  return rc;
}

QString BorderMeta::format(bool local)
{
  QString foo;
  switch (_value[pushed].type) {
    case BdrNone:
      foo = "NONE";
    break;
    case BdrSquare:
      foo = QString("SQUARE %1 %2") 
              .arg(_value[pushed].color) 
              .arg(_value[pushed].thickness);
    break;
    case BdrRound:
      foo = QString("ROUND %1 %2 %3") 
              .arg(_value[pushed].color) 
              .arg(_value[pushed].thickness) 
              .arg(_value[pushed].radius);
    break;
  }
  QString bar = QString(" MARGINS %1 %2")
                  .arg(_value[pushed].margin[0])
                  .arg(_value[pushed].margin[1]);
  foo += bar;

  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}

void BorderMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " (NONE|SQUARE <color> <thickness>|ROUND <color> <thickness> <radius>) MARGINS <x> <y>" << endl;
}

QString BorderMeta::text()
{
  BorderData border = value();
  QString thickness;
  switch (border.type) {
    case BgTransparent:
      return "No Border";
    break;
    case BdrSquare:
      thickness = QString("%1")
        .arg(border.thickness,4,'f',3);
      return "Square Corners, thickess " + thickness + " " + units2abbrev();
    break;
    default:
    break;
  }
  thickness = QString("%1") .arg(border.thickness,4,'f',3);
  return "Round Corners, thickess " + thickness + " " + units2abbrev();
}

/* ------------------ */ 

PointerMeta::PointerMeta()
{
  _value[0].placement = TopLeft;
  _value[0].loc       = 0;
  _value[0].x         = 0.5;
  _value[0].y         = 0.5;
  _value[0].base      = 0.125;
}

/*
 * (TopLeft|TopRight|BottomRight|BottomLeft) <x> <y> (<base>)
 *
 * (Top|Right|Bottom|Left) <loc> <x> <y> (<base>)
 */

Rc PointerMeta::parse(QStringList &argv, int index,Where &here)
{
  float _loc = 0, _x = 0, _y = 0, _base = -1;
  int   n_tokens = argv.size() - index;
  QString foo1 = argv[index];
  QString foo2 = argv[index+1];
  bool    fail = true;

  if (argv.size() - index > 0) {
    QRegExp rx("^(TOP_LEFT|TOP_RIGHT|BOTTOM_LEFT|BOTTOM_RIGHT)$");
    if (argv[index].contains(rx) && n_tokens == 4) {
      _loc = 0;
      bool ok[3];
      _x    = argv[index+1].toFloat(&ok[0]);
      _y    = argv[index+2].toFloat(&ok[1]);
      _base = argv[index+3].toFloat(&ok[2]);
      fail  = ! (ok[0] && ok[1] && ok[2]);
    }
    if (argv[index].contains(rx) && n_tokens == 3) {
      _loc = 0;
      bool ok[2];
      _x    = argv[index+1].toFloat(&ok[0]);
      _y    = argv[index+2].toFloat(&ok[1]);
      fail  = ! (ok[0] && ok[1]);
    }
    rx.setPattern("^(TOP|BOTTOM|LEFT|RIGHT)$");
    if (argv[index].contains(rx) && n_tokens == 5) {
      _loc = 0;
      bool ok[4];
      _loc  = argv[index+1].toFloat(&ok[0]);
      _x    = argv[index+2].toFloat(&ok[1]);
      _y    = argv[index+3].toFloat(&ok[2]);
      _base = argv[index+4].toFloat(&ok[3]);
      fail  = ! (ok[0] && ok[1] && ok[2] && ok[3]);
    }
    if (argv[index].contains(rx) && n_tokens == 4) {
      _loc = 0;
      bool ok[3];
      _loc  = argv[index+1].toFloat(&ok[0]);
      _x    = argv[index+2].toFloat(&ok[1]);
      _y    = argv[index+3].toFloat(&ok[2]);
      fail  = ! (ok[0] && ok[1] && ok[2]);
    }
  }
  if ( ! fail) {
    _value[pushed].placement = PlacementEnc(tokenMap[argv[index]]);
    _value[pushed].loc       = _loc;
    _value[pushed].x         = _x;
    _value[pushed].y         = _y;
    if (_base > 0) {
      _value[pushed].base = _base;
    } else if (_value[pushed].base == 0) {
      _value[pushed].base = 1.0/8;
    }
    _here[0] = here;
    _here[1] = here;
    return CalloutPointerRc;
  } else {
      
    QMessageBox::warning(NULL,
      QMessageBox::tr("LPub"),
      QMessageBox::tr("Malformed callout pointer \"%1\"") .arg(argv.join(" ")));

    return FailureRc;
  }
}
QString PointerMeta::format(bool local)
{
  local = local;
  QString foo;
  switch(_value[pushed].placement) {
    case TopLeft:
    case TopRight:
    case BottomRight:
    case BottomLeft:
      foo = QString("%1 %2 %3 %4") 
        .arg(placementNames[_value[pushed].placement])
        .arg(_value[pushed].x,0,'f',3) 
        .arg(_value[pushed].y,0,'f',3) 
        .arg(_value[pushed].base);
    break;
    default:
      foo = QString("%1 %2 %3 %4 %5") 
        .arg(placementNames[_value[pushed].placement])
        .arg(_value[pushed].loc,0,'f',3) 
        .arg(_value[pushed].x,  0,'f',3) 
        .arg(_value[pushed].y,  0,'f',3) 
        .arg(_value[pushed].base);
    break;
  }
  return foo;
}

void PointerMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " (TOP_LEFT|TOP_RIGHT|BOTTOM_LEFT|BOTTOM_RIGHT) <floatX> <floatY> <intBase>" << endl;
  out << preamble << " (TOP|BOTTOM|LEFT|RIGHT) <floatLoc> <floatX> <floatY> <intBase>" << endl;
}

/* ------------------ */ 
FreeFormMeta::FreeFormMeta()
{
  _value[0].mode = false;
}
Rc FreeFormMeta::parse(QStringList &argv, int index,Where &here)
{
  Rc rc = FailureRc;
  if (argv.size() - index == 1 && argv[index] == "FALSE") {
    _value[pushed].mode = false;
    rc = OkRc;
  } else if (argv.size() - index == 2) {
    _value[pushed].mode = true;
    QRegExp rx("^(STEP_NUMBER|ASSEM|PLI)$");
    if (argv[index].contains(rx)) {
      rx.setPattern("^(LEFT|RIGHT|TOP|BOTTOM|CENTER)$");
      if (argv[index+1].contains(rx)) {
        _value[pushed].base = PlacementEnc(tokenMap[argv[index]]);
        _value[pushed].justification = PlacementEnc(tokenMap[argv[index+1]]);
        rc = OkRc;
      }
    }
  }
  if (rc == OkRc) {
    _here[pushed] = here;
  }
  return rc;
}
QString FreeFormMeta::format(bool local)
{
  QString foo = local ? "LOCAL " : "";

  if (_value[pushed].mode) {
    return preamble + foo + relativeNames[_value[pushed].base] + " " + 
                            placementNames[_value[pushed].justification];
  } else {
    return preamble + foo + "FALSE";
  }
}
void FreeFormMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " (FALSE|(STEP_NUMBER|ASSEM|PLI) (LEFT|RIGHT|TOP|BOTTOM|CENTER))" << endl;
}

/* ------------------ */ 

ConstrainMeta::ConstrainMeta()
{
  _value[0].type = PliConstrainArea;
}
Rc ConstrainMeta::parse(QStringList &argv, int index,Where &here)
{
  Rc rc = FailureRc;
  bool ok;
  QRegExp rx;
  switch(argv.size() - index) {
    case 1:
      rx.setPattern("^(AREA|SQUARE)$");
      if (argv[index].contains(rx)) {
        _value[pushed].type = PliConstrain(tokenMap[argv[index]]);
        rc = OkRc;
      }
    break;
    case 2:
      argv[index+1].toInt(&ok);
      if (ok) {
        rx.setPattern("^(WIDTH|HEIGHT|COLS)$");
        if (argv[index].contains(rx)) {
          _value[pushed].type = PliConstrain(tokenMap[argv[index]]);
          _value[pushed].constraint = argv[index+1].toFloat(&ok);
          rc = OkRc;
        } 
      }
    break;
  }
  if (rc == OkRc) {
    _here[pushed] = here;
  }      
  return rc;
}
QString ConstrainMeta::format(bool local)
{
  QString foo;
  switch (_value[pushed].type) {
    case PliConstrainArea:
      foo = "AREA";
    break;
    case PliConstrainSquare:
      foo = "SQUARE";
    break;
    case PliConstrainWidth:
      foo = QString("WIDTH %1") .arg(_value[pushed].constraint);
    break;
    case PliConstrainHeight:
      foo = QString("HEIGHT %1") .arg(_value[pushed].constraint);
    break;
    default:
      foo = QString("COLS %1") .arg(_value[pushed].constraint);
    break;
  }
  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}
void ConstrainMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " (AREA|SQUARE|(WIDTH|HEIGHT|COLS) <integer>)" << endl;
}

/* ------------------ */ 

AllocMeta::AllocMeta()
{
  type[0] = Vertical;
}
Rc AllocMeta::parse(QStringList &argv, int index, Where &here)
{
  QRegExp rx("^(HORIZONTAL|VERTICAL)$");
  if (argv.size() - index == 1 && argv[index].contains(rx)) {
    type[pushed] = AllocEnc(tokenMap[argv[index]]);
    _here[pushed] = here;
    return OkRc;
  }
      
  QMessageBox::warning(NULL,
    QMessageBox::tr("LPub"),
    QMessageBox::tr("Expected HORIZONTAL or VERTICAL got \"%1\" in \"%2\"") .arg(argv[index]) .arg(argv.join(" ")));

  return FailureRc;
}
QString AllocMeta::format(bool local)
{
  QString foo = local ? "LOCAL " : "";
  if (type[pushed] == Horizontal) {
    return preamble + foo + "HORIZONTAL";
  } else {
    return preamble + foo + "VERTICAL";
  }
}
void AllocMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " (HORIZONTAL|VERTICAL)" << endl;
}

/* ------------------ */ 
SepMeta::SepMeta()
{
  _value[0].thickness = DEFAULT_THICKNESS;
  _value[0].color = "black";
  _value[0].margin[0] = DEFAULT_MARGIN;
  _value[0].margin[1] = DEFAULT_MARGIN;
}
Rc SepMeta::parse(QStringList &argv, int index,Where &here)
{
  bool ok[3];
  if (argv.size() - index == 4) {
    argv[index  ].toFloat(&ok[0]);
    argv[index+2].toFloat(&ok[1]);
    argv[index+3].toFloat(&ok[2]);

    if (ok[0] && ok[1] && ok[2]) {
      _value[pushed].thickness = argv[index].toFloat(&ok[0]);
      _value[pushed].color     = argv[index+1];
      _value[pushed].margin[0] = argv[index+2].toFloat(&ok[0]);
      _value[pushed].margin[1] = argv[index+3].toFloat(&ok[0]);
      _here[pushed] = here;
      return OkRc;
    }
  }
        
  QMessageBox::warning(NULL,
    QMessageBox::tr("LPub"),
    QMessageBox::tr("Malformed separator \"%1\"") .arg(argv.join(" ")));

  return FailureRc;
}
QString SepMeta::format(bool local)
{
  QString foo = QString("%1 %2 %3 %4") 
   .arg(_value[pushed].thickness) 
   .arg(_value[pushed].color) 
   .arg(_value[pushed].margin[0]) 
   .arg(_value[pushed].margin[1]);

  if (local) {
    foo = "LOCAL " + foo;
  }
  return preamble + foo;
}
void SepMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <intThickness> <color> <marginX> <marginY>" << endl;
}

/* ------------------ */ 
Rc InsertMeta::parse(QStringList &argv, int index,Where &here)
{
  Rc rc = FailureRc;
  if (argv.size() - index == 3) {
    bool ok[2];
    argv[index+1].toFloat(&ok[0]);
    argv[index+2].toFloat(&ok[1]);
    if (ok[0] && ok[1]) {
      PicMeta pic;
      pic.fileName = argv[index];
      pic.position[0] = argv[index+1].toFloat(&ok[0]);
      pic.position[1] = argv[index+2].toFloat(&ok[1]);
      pic.here       = here;
      list << pic;
      rc = OkRc;
    }
  }
  return rc;
}

QString InsertMeta::format(bool local)
{
  return format(local,0);
}

QString InsertMeta::format(bool local, int i)
{
  local = local;
  if (i < list.size()) {
    QString foo = QString(" %1 %2") 
                    .arg(list[i].position[0]) .arg(list[i].position[1]);
    return preamble + list[i].fileName + foo;
  } else {
    QString foo;
    return foo;
  }
}
void InsertMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <\"filename\"> <floatX> <floatY>" << endl;
}

/* ------------------ */ 

CalloutCsiMeta::CalloutCsiMeta()
{
}

void CalloutCsiMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  placement.init(this,"PLACEMENT");
  margin.init   (this,"MARGINS");
}

/* ------------------ */ 

CalloutPliMeta::CalloutPliMeta()
{
  placement.setValue(Top,Center,CsiType,Outside);
  perStep.setValue(true);
}

void CalloutPliMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  placement.init(this,"PLACEMENT");
  margin.init(   this,"MARGINS");
  perStep.init  (this,"PER_STEP");
}

/* ------------------ */ 

void PliBeginMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  ignore.init(this, "IGN",    PliBeginIgnRc);
  sub.init   (this, "(SUB)");
}

/* ------------------ */ 

NumberMeta::NumberMeta()
{
  color.setValue("black");
  // font - default
}

void NumberMeta::init(
  BranchMeta *parent, 
  QString name)
{
  AbstractMeta::init(parent, name);
  color.init    (this, "FONT_COLOR");
  font.init     (this, "FONT",OkRc, "\"");
  margin.init   (this, "MARGINS");
}

NumberPlacementMeta::NumberPlacementMeta()
{
  placement.setValue(Right,Top,PartsListType,Outside);

  color.setValue("black");
  // font - default
}

void NumberPlacementMeta::init(
  BranchMeta *parent, 
  QString name)
{
  AbstractMeta::init(parent, name);
  placement.init(this, "PLACEMENT");
  color.init    (this, "FONT_COLOR");
  font.init     (this, "FONT",OkRc, "\"");
  margin.init   (this, "MARGINS");
}

/* ------------------ */ 

void RemoveMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  group.init(   this,"GROUP",RemoveGroupRc);
  parttype.init(this,"PART", RemovePartRc);
  partname.init(this,"NAME", RemoveNameRc);
}

/* ------------------ */

PartMeta::PartMeta()
{
}

void PartMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent,name);
  margin.init(this,"MARGINS");
}

/* ------------------ */ 

Rc SubMeta::parse(QStringList &argv, int index,Where &here)
{
  Rc rc = FailureRc;
  int argc = argv.size() - ++index;

  if (argc == 1) {
    _value.part = argv[index];
    _value.color = "";
    _value.type = rc = PliBeginSub1Rc;
  } else if (argc == 2) {
    _value.part  = argv[index];
    _value.color = argv[index+1];
    _value.type = rc = PliBeginSub2Rc;
  }
  if (rc != FailureRc) {
    _here[0] = here;
    _here[1] = here;
  }
  return rc;
}
QString SubMeta::format(bool local)
{
  local = local;
  if (_value.type == PliBeginSub1Rc) {
    return preamble + _value.part;
  } else {
    return preamble + _value.color + " " + _value.part;
  }
}

void SubMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <part> <color>" << endl;
  out << preamble << " <part>" << endl;
}

/* ------------------ */ 

void PartBeginMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  ignore.init(this, "IGN",   PartBeginIgnRc);
}

/* ------------------ */ 

void PartIgnMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  begin.init(this, "BEGIN");
  end  .init(this, "END", PartEndRc);
}

/* ------------------ */ 

Rc RotStepMeta::parse(QStringList &argv, int index,Where &here)
{
  if (index + 4 == argv.size()) {
    bool ok[3];
    argv[index+0].toFloat(&ok[0]);
    argv[index+1].toFloat(&ok[1]);
    argv[index+2].toFloat(&ok[2]);
    ok[0] &= ok[1] & ok[2];
    QRegExp rx("^(ABS|REL|ADD)$");
    if (ok[0] && argv[index+3].contains(rx)) {
      _value.rots[0] = argv[index+0].toFloat(&ok[0]);
      _value.rots[1] = argv[index+1].toFloat(&ok[1]);
      _value.rots[2] = argv[index+2].toFloat(&ok[2]);
      _value.type = argv[index+3];
      _here[0] = here;
      _here[1] = here;
      return RotStepRc;
    } 
  } else if (argv.size()-index == 1 && argv[index] == "END") {
    _value.type.clear();
    _here[0] = here;
    _here[1] = here;
    return RotStepRc;
  }
    
  QMessageBox::warning(NULL,
    QMessageBox::tr("LPub"),
    QMessageBox::tr("Malformed rotation step \"%1\"") .arg(argv.join(" ")));

  return FailureRc;
}
QString RotStepMeta::format(bool local)
{
  local = local;
  QString foo = QString("%1 %2 %3 %4") 
    .arg(_value.rots[0]) .arg(_value.rots[1])
    .arg(_value.rots[2]) .arg(_value.type);
  return preamble + foo;
}

void RotStepMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <rotX> <rotY> <rotZ> <ABS|REL|ADD>" << endl;
}

/* ------------------ */ 
Rc BuffExchgMeta::parse(QStringList &argv, int index,Where &here)
{
  if (index + 2 == argv.size()) {
    QRegExp b("^[A-Z]$");
    QRegExp t("^(STORE|RETRIEVE)$");
    if (argv[index].contains(b) && argv[index+1].contains(t)) {
      _value.buffer = argv[index];
      _here[0] = here;
      _here[1] = here;
      if (argv[index+1] == "RETRIEVE") {
        return BufferLoadRc;
      } else {
        return BufferStoreRc;
      }
    } 
  }
      
  QMessageBox::warning(NULL,
    QMessageBox::tr("LPub"),
    QMessageBox::tr("Malformed buffer exchange \"%1\"") .arg(argv.join(" ")));

  return FailureRc;
}
QString BuffExchgMeta::format(bool local)
{
  local = local;
  return preamble + _value.buffer + " " + _value.type;
}

void BuffExchgMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << " <bufferName> <STORE|RETRIEVE>" << endl;
}

/*---------------------------------------------------------------
 * The Top Level LPub Metas
 *---------------------------------------------------------------*/

PageMeta::PageMeta()
{
  size.setValueUnits(8.5,11.0);
  size.setRange(1,100);
  size.setFormats(6,4,"9.9999");

  BorderData borderData;
  borderData.type = BdrNone;
  borderData.color = "Black";
  borderData.thickness = 0;
  borderData.radius = 0;
  borderData.margin[0] = DEFAULT_MARGIN;
  borderData.margin[1] = DEFAULT_MARGIN;
  border.setValueUnit(borderData);

  background.setValue(BgSubmodelColor);
  dpn.setValue(true);
  togglePnPlacement.setValue(false);
  number.placement.setValue(BottomRight,PageType,Inside);
  number.color.setValue("black");
  number.font.setValueUnit("Arial,20,-1,75,0,0,0,0,0");

  subModelColor.setValue("0xffffff");
  subModelColor.setValue("0xffffcc");
  subModelColor.setValue("0xffcccc");
  subModelColor.setValue("0xccccff");
}

void PageMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  size.init         (this, "SIZE");
  margin.init       (this, "MARGINS");
  border.init       (this, "BORDER");
  background.init   (this, "BACKGROUND");
  inserts.init      (this, "INSERT");
  dpn.init          (this, "DISPLAY_PAGE_NUMBER");
  togglePnPlacement.init(this,"TOGGLE_PAGE_NUMBER_PLACEMENT");
  number.init       (this, "NUMBER");
  subModelColor.init(this,"SUBMODEL_BACKGROUND_COLOR");
}

/* ------------------ */ 
AssemMeta::AssemMeta()
{
  placement.setValue(Center,Center,PageType,Inside);
  modelSize.setRange(1.0,10000.0);
  modelSize.setFormats(7,1,"99999.9");
  modelSize.setValue(12.0);
  ldgliteParms.setValue("-fh -w1");
  ldviewParms.setValue("");
}
void AssemMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  margin.init       (this,"MARGINS");
  placement.init    (this,"PLACEMENT");
  modelSize.init    (this,"MODEL_SIZE");
  ldviewParms.init  (this,"LDGLITE_PARMS");
  ldgliteParms.init (this,"LDVIEW_PARMS");
}

/* ------------------ */ 

PliMeta::PliMeta()
{
  placement.setValue(Right,Top,StepNumberType,Outside);
  ConstrainData constraint;
  constraint.type = PliConstrainArea;
  constraint.constraint = 0;
  constrain.setValueUnit(constraint);
  BorderData borderData;
  borderData.type = BdrRound;
  borderData.color = "Black";
  borderData.thickness = DEFAULT_THICKNESS;
  borderData.radius = 20;
  borderData.margin[0] = DEFAULT_MARGIN;
  borderData.margin[1] = DEFAULT_MARGIN;
  border.setValueUnit(borderData);
  background.setValue(BgColor,"0xffffff");
  margin.setValueUnits(0.0,0.0);
  // instance - default
  // annotate - default
  modelSize.setRange(1.0,10000.0);
  modelSize.setFormats(7,1,"99999.9");
  modelSize.setValue(12.0);
  angle.setValues(23,-45);
  angle.setRange(-360.0,360.0);
  angle.setFormats(6,1,"#999.9");
  show.setValue(true);
  ldgliteParms.setValue("-fh -w1");
  ldviewParms.setValue("");
  includeSubs.setValue(false);
  subModelColor.setValue("0xffffff");
  subModelColor.setValue("0xffffcc");
  subModelColor.setValue("0xffcccc");
  subModelColor.setValue("0xccccff");
  part.margin.setValueUnits(0.05,0.03);
  instance.margin.setValueUnits(0.0,0.0);
  annotate.margin.setValueUnits(0.0,0.0);
}

void PliMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  placement    .init(this,"PLACEMENT");
  constrain    .init(this,"CONSTRAIN");
  border       .init(this,"BORDER");
  background   .init(this,"BACKGROUND");
  margin       .init(this,"MARGINS");
  instance     .init(this,"INSTANCE_COUNT");
  annotate     .init(this,"ANNOTATE");
  modelSize    .init(this,"MODEL_SIZE");
  angle        .init(this,"VIEW_ANGLE");
  show         .init(this,"SHOW");
  ldviewParms  .init(this,"LDVIEW_PARMS");
  ldgliteParms .init(this,"LDGLITE_PARMS");
  includeSubs  .init(this,"INCLUDE_SUBMODELS");
  subModelColor.init(this,"SUBMODEL_BACKGROUND_COLOR");
  part         .init(this,"PART");
  begin        .init(this,"BEGIN");
  end          .init(this,"END",           PliEndRc);
}

/* ------------------ */ 

void BomBeginMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  ignore.init(this,"IGN",BomBeginIgnRc);
}

/* ------------------ */ 

BomMeta::BomMeta()
{
  BorderData borderData;
  borderData.type = BdrNone;
  borderData.color = "Black";
  borderData.thickness = DEFAULT_THICKNESS;
  borderData.radius = 20;
  borderData.margin[0] = DEFAULT_MARGIN;
  borderData.margin[1] = DEFAULT_MARGIN;
  border.setValueUnit(borderData);
  background.setValue(BgTransparent);
  ConstrainData constraint;
  constraint.type = PliConstrainColumns;
  constraint.constraint = 5;
  constrain.setValueUnit(constraint);
  pack.setValue(false);
  sort.setValue(true);
}

void BomMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  border.init(    this,"BORDER");
  background.init(this,"BACKGROUND");
  margin.init(    this,"MARGINS");
  instance.init(  this,"INSTANCE_COUNT");
  annotate.init(  this,"ANNOTATE");
  constrain.init( this,"CONSTRAIN");
  placement.init( this,"PLACEMENT");
  part.init(      this,"PART_TEXT");
  pack.init(      this,"PACK");
  sort.init(      this,"SORT");
  begin.init(     this,"BEGIN");
  end.init(       this,"END",BomEndRc);
}

/* ------------------ */ 
CalloutMeta::CalloutMeta()
{
  stepNum.color.setValue("black");
  // stepNum.font - default
  stepNum.placement.setValue(Left,Top,PartsListType,Outside);
  sep.setValueUnit("Black",DEFAULT_THICKNESS,DEFAULT_MARGINS);
  BorderData borderData;
  borderData.type = BdrSquare;
  borderData.color = "Black";
  borderData.thickness = DEFAULT_THICKNESS;
  borderData.radius = 20;
  borderData.margin[0] = DEFAULT_MARGIN;
  borderData.margin[1] = DEFAULT_MARGIN;
  border.setValueUnit(borderData);
  // subModelFont - default
  instance.color.setValue("black");
  // instance - default
  instance.placement.setValue(Right, Bottom, CalloutType,Outside);
  background.setValue(BgSubmodelColor);
  subModelColor.setValue("0xffffff");
  subModelColor.setValue("0xffffcc");
  subModelColor.setValue("0xffcccc");
  subModelColor.setValue("0xccccff");
  subModelFontColor.setValue("black");
  placement.setValue(Right,Center,CsiType,Inside);
  // freeform
  alloc.setValue(Vertical);
  pli.perStep.setValue(true);
}

void CalloutMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  margin     .init(this,      "MARGINS");
  stepNum    .init(this,      "STEP_NUMBER");
  sep        .init(this,      "SEPARATOR");
  border     .init(this,      "BORDER");
  subModelFont.init (this,    "SUBMODEL_FONT");
  instance   .init(this,      "INSTANCE_COUNT");
  background .init(this,      "BACKGROUND");
  subModelColor.init(this,    "SUBMODEL_BACKGROUND_COLOR");
  subModelFontColor.init(this,"SUBMODEL_FONT_COLOR");
  placement  .init(this,      "PLACEMENT");
  freeform   .init(this,      "FREEFORM");
  alloc      .init(this,      "ALLOC");
  pointer    .init(this,      "POINTER");
  silent_alloc.init(this,     "^(HORIZONTAL|VERTICAL)");

  begin      .init(this,      "BEGIN",   CalloutBeginRc);
  divider    .init(this,      "DIVIDER", CalloutDividerRc);
  end        .init(this,      "END",     CalloutEndRc);
  csi        .init(this,      "ASSEM");
  pli        .init(this,      "PLI");
}

/* ------------------ */ 

MultiStepMeta::MultiStepMeta()
{
  stepNum.placement.setValue(Left,Top,PartsListType,Outside);
  stepNum.color.setValue("black");
  // stepNum.font - default
  placement.setValue(Center,Center,PageType,Inside);
  sep.setValueUnit("black",DEFAULT_THICKNESS,DEFAULT_MARGINS);
  // subModelFont - default
  subModelFontColor.setValue("black");
  // freeform
  alloc.setValue(Vertical);
  pli.placement.setValue(Left,Top,CsiType,Outside);
  pli.perStep.setValue(true);
}

void MultiStepMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  margin   .init(this,    "MARGINS");
  stepNum  .init(this,    "STEP_NUMBER");
  placement.init(this,    "PLACEMENT");
  sep      .init(this,    "SEPARATOR");
  subModelFont.init (this,"SUBMODEL_FONT");
  subModelFontColor.init(this,
                          "SUBMODEL_FONT_COLOR");
  freeform .init(this,    "FREEFORM");
  alloc    .init(this,    "ALLOC");
  csi      .init(this,    "ASSEM");
  pli      .init(this,    "PLI");

  begin    .init(this,    "BEGIN",  StepGroupBeginRc);
  divider  .init(this,    "DIVIDER",StepGroupDividerRc);
  end      .init(this,    "END",    StepGroupEndRc);
}

/* ------------------ */ 

void ResolutionMeta::init(
  BranchMeta *parent, 
  const QString name)
{
  AbstractMeta::init(parent,name);
}

Rc ResolutionMeta::parse(QStringList &argv, int index, Where &here)
{
  int tokens = argv.size() - index;
  if (tokens == 0) {
    return FailureRc;
  }
  if (tokens == 2 && argv[index+1] == "DPI") {
    _here[0] = here;
    resolution = argv[index].toFloat();
    resolutionType  = DPI;
  } else if (tokens == 2 && argv[index] == "DPCM") {
    _here[0] = here;
    resolution = argv[index].toFloat();
    resolutionType = DPCM;
  } else {
    return FailureRc;
  }
  return ResolutionRc;
}

QString ResolutionMeta::format(bool unused)
{
  unused = unused;
  QString res;
  switch (resolutionType) {
    case DPI:
      res = QString("%1 ") .arg(resolution,0,'f',0);
      return preamble + res + "DPI";
    case DPCM:
      res = QString("%1 ") .arg(resolution,0,'f',0);
      return preamble + res + "DPCM";
  }
  return "";
} 

void ResolutionMeta::doc(QTextStream &out, QString preamble)
{
  out << preamble << "<integer> (DPI|DPCM)" << endl;
}

LPubMeta::LPubMeta()
{
  stepNumber.placement.setValue(TopLeft,PageType,Inside);
  stepNumber.color.setValue("black");
  // stepNumber - default 
}

void LPubMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  page                   .init(this,"PAGE");
  assem                  .init(this,"ASSEM");
  stepNumber             .init(this,"STEP_NUMBER");
  callout                .init(this,"CALLOUT");
  multiStep              .init(this,"MULTI_STEP");
  pli                    .init(this,"PLI");
  bom                    .init(this,"BOM");
  remove                 .init(this,"REMOVE");
  reserve                .init(this,"RESERVE", ReserveSpaceRc);
  partSub                .init(this,"PART");
  resolution             .init(this,"RESOLUTION");
}

/* ------------------ */ 

void MLCadMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  MLCadSB .init(this,"SKIP_BEGIN", MLCadSkipBeginRc);
  MLCadSE .init(this,"SKIP_END",   MLCadSkipEndRc);
  MLCadGrp.init(this,"BTG",        MLCadGroupRc);
}

Rc MLCadMeta::parse(QStringList &argv, int index,Where &here)
{
  Rc rc;

  QHash<QString, AbstractMeta *>::iterator i = list.find(argv[index]);
  if (i == list.end() || index == argv.size()) {
    rc = OkRc;
  } else {
    rc = i.value()->parse(argv,index+1,here);
  }
  return rc;
}

/* ------------------ */ 

void LSynthMeta::init(BranchMeta *parent, QString name)
{
  AbstractMeta::init(parent, name);
  begin      .init(this, "BEGIN",       SynthBeginRc);
  end        .init(this, "END",         SynthEndRc);
  synthesized.init(this, "SYNTHESIZED");
}

/* ------------------ */ 

Meta::Meta(
  QString topLevelFile)
{
  context.setTopOfModel(topLevelFile);
  QString empty;
  preamble = "0 ";
  init(NULL,empty);
  
  if (tokenMap.size() == 0) {
    tokenMap["TOP_LEFT"]     = TopLeft;
    tokenMap["TOP"]          = Top;
    tokenMap["TOP_RIGHT"]    = TopRight;
    tokenMap["RIGHT"]        = Right;
    tokenMap["BOTTOM_RIGHT"] = BottomRight;
    tokenMap["BOTTOM"]       = Bottom;
    tokenMap["BOTTOM_LEFT"]  = BottomLeft;
    tokenMap["LEFT"]         = Left;
    tokenMap["CENTER"]       = Center;

    tokenMap["INSIDE"]       = Inside;
    tokenMap["OUTSIDE"]      = Outside;
 
    tokenMap["PAGE"]         = PageType;
    tokenMap["ASSEM"]        = CsiType;
    tokenMap["MULT_STEP"]    = StepGroupType;
    tokenMap["STEP_GROUP"]   = StepGroupType;
    tokenMap["STEP_NUMBER"]  = StepNumberType;
    tokenMap["PLI"]          = PartsListType;

    tokenMap["AREA"]         = PliConstrainArea;
    tokenMap["SQUARE"]       = PliConstrainSquare;
    tokenMap["WIDTH"]        = PliConstrainWidth;
    tokenMap["HEIGHT"]       = PliConstrainHeight;
    tokenMap["COLS"]         = PliConstrainColumns;

    tokenMap["VERTICAL"]     = Vertical;
    tokenMap["HORIZONTAL"]   = Horizontal;
  }
}

Meta::~Meta()
{
}

void Meta::init(BranchMeta *parent, QString name)
{
  parent = parent;
  name = name;
  LPub   .init(this,"!LPUB");
  step   .init(this,"STEP",    StepRc);
  clear  .init(this,"CLEAR",   ClearRc);
  rotStep.init(this,"ROTSTEP");
  bfx    .init(this,"BUFEXCHG");
  MLCad  .init(this,"MLCAD");
  LSynth .init(this,"SYNTH");
}

void Meta::mkargv(
  QString     &line,
  QStringList &argv,
  Where        here)
{
  QString chopped  = line;

  /* Parse the input line into argv[] */
  
  int soq = chopped.indexOf("\"");
  if (soq == -1) {
    argv << chopped.split(" ",QString::SkipEmptyParts);
  } else {
    while (chopped.size()) {
      soq = chopped.indexOf("\"");
      if (soq == -1) {
        argv << chopped.split(" ",QString::SkipEmptyParts);
        chopped.clear();
      } else {
        // we found a double quote
        QString left = chopped.left(soq);

        left = left.trimmed();
  
        argv << left.split(" ",QString::SkipEmptyParts);

        chopped = chopped.mid(soq+1);
  
        soq = chopped.indexOf("\"");
  
        if (soq == -1) {
          if (here.modelName != "undefined") {
            gui->parseError("Failed to find matching \".",here);
          }
          QMessageBox::warning(NULL,QMessageBox::tr("LPub"),
            QMessageBox::tr("Failed to find matching qoute in %1") .arg(chopped));
          argv.clear();
          return;
        }
        argv << chopped.left(soq);
  
        chopped = chopped.mid(soq+1);
  
        if (chopped == "\"") {
          chopped.clear();
        }
      }
    }
  }
}

Rc Meta::parse(
  QString  &line,
  Where    &here,
  bool      partsAdded)
{
  QStringList argv;

  QRegExp bgt("^\\s*0\\s+(MLCAD)\\s+(BTG)\\s+(.*)$");

  if (line.contains(bgt)) {
    argv << "MLCAD" << "BTG" << bgt.cap(3);
  } else {

    /* Parse the input line into argv[] */

    mkargv(line,argv,here);

    if (argv.size() > 0) {
      argv.removeFirst();
    }
    if (argv.size() > 0 && argv[0] == "WRITE") {
      argv.removeFirst();
    }
  	if (argv.size()) {
	    if (argv[0] == "LPUB") {
        argv[0] = "!LPUB";
      }
	    if (argv[0] == "PLIST") {
	      return  LPub.pli.parse(argv,1,here);
	    }
	  }
  }

  if (argv.size() > 0 && list.contains(argv[0])) {

    /* parse it up */

    Rc rc = BranchMeta::parse(argv,0,here);

    if (rc == FailureRc) {
      QMessageBox::warning(NULL,QMessageBox::tr("LPub"),
                              QMessageBox::tr("Parse failed %1:%2\n%3")
                              .arg(here.modelName) .arg(here.lineNumber) .arg(line));
    } else {
      if (partsAdded && (rc == StepRc || rc == RotStepRc)) {
        context.setBottomOfStep(here);
      } else if (rc == StepGroupBeginRc) {
        context.setTopOfRanges(here);
        context.setTopOfRange(here);
      } else if (rc == StepGroupDividerRc) {
        context.setBottomOfRange(here);
      } else if (rc == StepGroupEndRc) {
        context.setBottomOfRanges(here);
        context.setBottomOfRange(here);
        context.setBottomOfStep(here);
      }
    }
    return rc;
  }
  return OkRc;
}

bool Meta::preambleMatch(
  QString &line,
  QString &preamble)
{
  QStringList argv;

  /* Parse the input line into argv[] */

  mkargv(line,argv,Where());

  if (argv.size() > 0) {
    argv.removeFirst();
  }

  if (argv.size() && argv[0] == "WRITE") {
    argv.removeFirst();
  }

  if (argv.size() && list.contains(argv[0])) {
    return BranchMeta::preambleMatch(argv,0,preamble);
  } else {
    return false;
  }
}

void Meta::pop()
{
  BranchMeta::pop();
}

void Meta::doc()
{
  QString fileName = QDir::currentPath() + "/lpubMeta.ldr";
  QFile docFile(fileName);
  if ( ! docFile.open(QFile::WriteOnly | QFile::Text)) {
    return;
  }
  QTextStream out(&docFile);
  QString key;
  QStringList keys = list.keys();
  keys.sort();
  foreach(key, keys) {
    list[key]->doc(out, "0 " + key);
  }
}
