
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
 * This file describes a set of classes that implement graphical user
 * interfaces for some of the configuration meta classes described in meta.h
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "metagui.h"

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QIntValidator>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>

#include <QDialogButtonBox>
#include <QFontDialog>
#include <QColorDialog>
#include <QFileDialog>

#include "meta.h"
#include "metatypes.h"
#include "metaitem.h"
#include "color.h"

#include "lpub.h"

/***********************************************************************
 *
 * Checkbox
 *
 **********************************************************************/

CheckBoxGui::CheckBoxGui(
  QString const  &heading,
  BoolMeta       *_meta,
  QGroupBox      *parent)
{
  meta = _meta;
    
  QHBoxLayout *layout = new QHBoxLayout(this);
  
  if (parent) {
    parent->setLayout(layout);
  } else {
    setLayout(layout);
  }

  check = new QCheckBox(heading,this);
  check->setChecked(meta->value());
  layout->addWidget(check);
  connect(check,SIGNAL(stateChanged(int)),
          this, SLOT(  stateChanged(int)));
}

void CheckBoxGui::stateChanged(int state)
{
  bool checked = meta->value();

  if (state == Qt::Unchecked) {
    checked = false;
  } else if (state == Qt::Checked) {
    checked = true;
  }
  meta->setValue(checked);
  modified = true;
}

void CheckBoxGui::apply(QString &modelName)
{
  if (modified) {
    MetaItem mi;
    mi.setGlobalMeta(modelName,meta);
  }
}

/***********************************************************************
 *
 * BoolRadio
 *
 **********************************************************************/

BoolRadioGui::BoolRadioGui(
  QString const &trueHeading,
  QString const &falseHeading,
  BoolMeta       *_meta,
  QGroupBox      *parent)
{
  meta = _meta;
    
  QVBoxLayout *layout = new QVBoxLayout(this);
  
  parent->setLayout(layout);

  trueRadio = new QRadioButton(trueHeading,this);
  connect(trueRadio,SIGNAL(clicked(bool)),
          this,     SLOT(  trueClicked(bool)));
  layout->addWidget(trueRadio); 
  trueRadio->setChecked(meta->value());

  falseRadio = new QRadioButton(falseHeading,this);
  connect(falseRadio,SIGNAL(clicked(bool)),
          this,      SLOT(  falseClicked(bool)));
  layout->addWidget(falseRadio);
  falseRadio->setChecked( ! meta->value());
}

void BoolRadioGui::trueClicked(bool clicked)
{
  clicked = clicked;
  meta->setValue(true);
  modified = true;
}

void BoolRadioGui::falseClicked(bool clicked)
{
  clicked = clicked;
  meta->setValue(false);
  modified = true;
}

void BoolRadioGui::apply(QString &modelName)
{
  if (modified) {
    MetaItem mi;
    mi.setGlobalMeta(modelName,meta);
  }
}

/***********************************************************************
 *
 * Units
 *
 **********************************************************************/

UnitsGui::UnitsGui(
  QString const &heading,
  UnitsMeta     *_meta,
  QGroupBox     *parent)
{
  meta = _meta;
    
  QHBoxLayout *layout = new QHBoxLayout(this);
  
  if (parent) {
    parent->setLayout(layout);
  } else {
    setLayout(layout);
  }

  if (heading != "") {
    label = new QLabel(heading,this);
    layout->addWidget(label);
  } else {
    label = NULL;
  }

  QString      string;

  string = QString("%1") .arg(meta->valueUnit(0),
                              meta->_fieldWidth,
                              'f',
                              meta->_precision);
  value0 = new QLineEdit(string,this);
  connect(value0,SIGNAL(textEdited(  QString const &)),
          this,  SLOT(  value0Change(QString const &)));
  layout->addWidget(value0);

  string = QString("%1") .arg(meta->valueUnit(1),
                              meta->_fieldWidth,
                              'f',
                              meta->_precision);
  value1 = new QLineEdit(string,this);
  connect(value1,SIGNAL(textEdited(  QString const &)),
          this,  SLOT(  value1Change(QString const &)));
  layout->addWidget(value1);
}

void UnitsGui::value0Change(QString const &string)
{
  meta->setValueUnit(string.toFloat(),0);
  modified = true;
}

void UnitsGui::value1Change(QString const &string)
{
  meta->setValueUnit(string.toFloat(),1);
  modified = true;
}

void UnitsGui::setEnabled(bool enable)
{
  if (label) {
    label->setEnabled(enable);
  }
  value0->setEnabled(enable);
  value1->setEnabled(enable);
}

void UnitsGui::apply(QString &modelName)
{
  if (modified) {
    MetaItem mi;
    mi.setGlobalMeta(modelName,meta);
  }
}

/***********************************************************************
 *
 * Float Pair
 *
 **********************************************************************/

FloatsGui::FloatsGui(
  QString const &heading0,
  QString const &heading1,
  FloatPairMeta *_meta,
  QGroupBox     *parent)
{
  meta = _meta;
    
  QHBoxLayout *layout = new QHBoxLayout(this);
  
  if (parent) {
    parent->setLayout(layout);
  } else {
    setLayout(layout);
  }

  if (heading0 == "") {
    label0 = NULL;
  } else {
    label0 = new QLabel(heading0,this);
    layout->addWidget(label0);
  }

  QString      string;

  string = QString("%1") .arg(meta->value(0),
                              meta->_fieldWidth,
                              'f',
                              meta->_precision);
  value0 = new QLineEdit(string,this);
  value0->setInputMask(meta->_inputMask);
  connect(value0,SIGNAL(textEdited(  QString const &)),
          this,  SLOT(  value0Change(QString const &)));
  layout->addWidget(value0);

  if (heading1 == "") {
    label1 = NULL;
  } else {
    label1 = new QLabel(heading1,this);
    layout->addWidget(label1);
  }

  string = QString("%1") .arg(meta->value(1),
                              meta->_fieldWidth,
                              'f',
                              meta->_precision);
  value1 = new QLineEdit(string,this);
  value1->setInputMask(meta->_inputMask);
  connect(value1,SIGNAL(textEdited(  QString const &)),
          this,  SLOT(  value1Change(QString const &)));
  layout->addWidget(value1);
}

void FloatsGui::value0Change(QString const &string)
{
  meta->setValue(string.toFloat(),0);
  modified = true;
}

void FloatsGui::value1Change(QString const &string)
{
  meta->setValue(string.toFloat(),1);
  modified = true;
}

void FloatsGui::setEnabled(bool enable)
{
  if (label0) {
    label0->setEnabled(enable);
  }
  if (label1) {
    label1->setEnabled(enable);
  }
  value0->setEnabled(enable);
  value1->setEnabled(enable);
}

void FloatsGui::apply(QString &modelName)
{
  if (modified) {
    MetaItem mi;
    mi.setGlobalMeta(modelName,meta);
  }
}

/***********************************************************************
 *
 * Float Spin
 *
 **********************************************************************/

DoubleSpinGui::DoubleSpinGui(
  QString const &heading,
  FloatMeta     *_meta,
  float          min,
  float          max,
  float          step,
  QGroupBox     *parent)
{
  meta = _meta;
    
  QHBoxLayout *layout = new QHBoxLayout(this);
  
  if (parent) {
    parent->setLayout(layout);
  } else {
    setLayout(layout);
  }

  if (heading == "") {
    label = NULL;
  } else {
    label = new QLabel(heading,this);
    layout->addWidget(label);
  }

  spin = new QDoubleSpinBox(this);
  layout->addWidget(spin);
  spin->setRange(min,max);
  spin->setSingleStep(step);
  spin->setValue(meta->value());
  connect(spin,SIGNAL(valueChanged(double)),
          this,SLOT  (valueChanged(double)));
}

void DoubleSpinGui::valueChanged(double value)
{
  meta->setValue(value);
  modified = true;
}

void DoubleSpinGui::setEnabled(bool enable)
{
  if (label) {
    label->setEnabled(enable);
  }
  spin->setEnabled(enable);
}

void DoubleSpinGui::apply(QString &modelName)
{
  if (modified) {
    MetaItem mi;
    mi.setGlobalMeta(modelName,meta);
  }
}

/***********************************************************************
 *
 * Constraint
 *
 **********************************************************************/

ConstrainGui::ConstrainGui(
  QString const &heading,
  ConstrainMeta *_meta,
  QGroupBox     *parent)
{
  meta = _meta;

  QHBoxLayout *layout;

  layout = new QHBoxLayout(this);
  
  if (parent) {
    parent->setLayout(layout);
  } else {
    setLayout(layout);
  }

  if (heading != "") {
    headingLabel = new QLabel(heading,this);
    layout->addWidget(headingLabel);
  } else {
    headingLabel = NULL;
  }

  QString        string;

  ConstrainData constraint = meta->value();

  string = "";

  switch (constraint.type) {
    case PliConstrainArea:
    case PliConstrainSquare:
    break;
    case PliConstrainWidth:
    case PliConstrainHeight:
      string = QString("%1") .arg(constraint.constraint,
                                  4,'f',1);
    break;
    case PliConstrainColumns:
      string = QString("%1") .arg(int(constraint.constraint));
    break;
  }

  combo = new QComboBox(this);
  combo->addItem("Area");
  combo->addItem("Square");
  combo->addItem("Width");
  combo->addItem("Height");
  combo->addItem("Columns");
  combo->setCurrentIndex(int(constraint.type));
  connect(combo,SIGNAL(currentIndexChanged(QString const &)),
          this, SLOT(  typeChange(         QString const &)));
  layout->addWidget(combo);

  /* Constraint */

  value = new QLineEdit(string,this);
  value->setInputMask("009.9");
  connect(value,SIGNAL(textEdited( QString const &)),
          this, SLOT(  valueChange(QString const &)));
  layout->addWidget(value);
  enable();
}

void ConstrainGui::typeChange(QString const &type)
{
  ConstrainData constraint = meta->valueUnit();
  QString string = "";
  if (type == "Area") {
    constraint.type = PliConstrainArea;
  } else if (type == "Square") {
    constraint.type = PliConstrainSquare;
  } else if (type == "Width") {
    string = QString("%1") .arg(constraint.constraint,
                                4,'f',1);
    constraint.type = PliConstrainWidth;
  } else if (type == "Height") {
    string = QString("%1") .arg(constraint.constraint,
                                4,'f',1);
    constraint.type = PliConstrainHeight;
  } else {
    string = QString("%1") .arg(int(constraint.constraint));
    constraint.type = PliConstrainColumns;
  }
  value->setText(string);
  meta->setValueUnit(constraint);
  enable();
  modified = true;
}

void ConstrainGui::valueChange(QString const &value)
{
  ConstrainData constraint = meta->valueUnit();
  constraint.constraint = value.toFloat();
  meta->setValueUnit(constraint);
  modified = true;
}

void ConstrainGui::setEnabled(bool enable)
{
  if (headingLabel) {
    headingLabel->setEnabled(enable);
  }
  combo->setEnabled(enable);
  value->setEnabled(enable);
}

void ConstrainGui::enable()
{
  ConstrainData constraint = meta->valueUnit();
  switch (constraint.type) {
    case PliConstrainArea:
      value->setDisabled(true);
    break;
    case PliConstrainSquare:
      value->setDisabled(true);
    break;
    default:
      value->setEnabled(true);
    break;
  } 
}

void ConstrainGui::apply(QString &modelName)
{
  if (modified) {
    MetaItem mi;
    mi.setGlobalMeta(modelName,meta);
  }
}

/***********************************************************************
 *
 * Number
 *
 **********************************************************************/

NumberGui::NumberGui(
  NumberMeta *_meta,
  QGroupBox  *parent)
{
  meta = _meta;

  QGridLayout *grid;

  grid = new QGridLayout(this);

  if (parent) {
    parent->setLayout(grid);
  }

  fontLabel = new QLabel("Font",this);
  grid->addWidget(fontLabel,0,0);
  
  fontExample = new QLabel("1234",this);
  QFont font;
  font.fromString(meta->font.valueUnit());
  fontExample->setFont(font);
  grid->addWidget(fontExample,0,1);

  fontButton = new QPushButton("Change",this);
  connect(fontButton,SIGNAL(clicked(   bool)),
          this,      SLOT(  browseFont(bool)));
  grid->addWidget(fontButton,0,2);

  colorLabel = new QLabel("Color",this);
  grid->addWidget(colorLabel,1,0);
  
  colorExample = new QLabel(this);
  colorExample->setFrameStyle(QFrame::Sunken|QFrame::Panel);
  colorExample->setPalette(QPalette(meta->color.value()));
  colorExample->setAutoFillBackground(true);
  grid->addWidget(colorExample,1,1);

  colorButton = new QPushButton("Change");
  connect(colorButton,SIGNAL(clicked(    bool)),
          this,       SLOT(  browseColor(bool)));
  grid->addWidget(colorButton,1,2);

  marginsLabel = new QLabel("Margins",this);
  grid->addWidget(marginsLabel,2,0);
  
  QString string;

  string = QString("%1") .arg(meta->margin.valueUnit(0),
                              5,'f',4);
  value0 = new QLineEdit(string,this);
  connect(value0,SIGNAL(textEdited(   QString const &)),
          this,  SLOT(  value0Changed(QString const &)));
  grid->addWidget(value0,2,1);
  
  string = QString("%1") .arg(meta->margin.valueUnit(1),
                              5,'f',4);
  value1 = new QLineEdit(string,this);
  connect(value1,SIGNAL(textEdited(   QString const &)),
          this,  SLOT(  value1Changed(QString const &)));
  grid->addWidget(value1,2,2);

  fontModified = false;
  colorModified = false;
  marginsModified = false;
}

void NumberGui::browseFont(bool clicked)
{
  clicked = clicked;
  QFont font;
  font.fromString(meta->font.valueUnit());
  bool ok;
  font = QFontDialog::getFont(&ok,font);

  if (ok) {
    meta->font.setValueUnit(font.toString());
    fontExample->setFont(font);
    fontModified = true;
  }
}

void NumberGui::browseColor(bool clicked)
{
  clicked = clicked;
  QColor qcolor = LDrawColor::color(meta->color.value());
  QColor newColor = QColorDialog::getColor(qcolor,this);
  if (qcolor != newColor) {
    colorExample->setPalette(QPalette(newColor));
    colorExample->setAutoFillBackground(true);
    meta->color.setValue(newColor.name());
    colorModified = true;
  }
}

void NumberGui::value0Changed(QString const &string)
{
  meta->margin.setValueUnit(string.toFloat(),0);
  marginsModified = true;
}

void NumberGui::value1Changed(QString const &string)
{
  meta->margin.setValueUnit(string.toFloat(),1);
  marginsModified = true;
}

void NumberGui::apply(
  QString &topLevelFile)
{
  MetaItem mi;
  mi.beginMacro("Settings");
  
  if (fontModified) {
    mi.setGlobalMeta(topLevelFile,&meta->font);
  }
  if (colorModified) {
    mi.setGlobalMeta(topLevelFile,&meta->color);
  }
  if (marginsModified) {
    mi.setGlobalMeta(topLevelFile,&meta->margin);
  }  
  mi.endMacro();
}

/***********************************************************************
 *
 * Background
 *
 **********************************************************************/

BackgroundGui::BackgroundGui(
  BackgroundMeta *_meta,
  QGroupBox      *parent)
{
  QString        string;
  QComboBox     *combo;
  QGridLayout   *grid;
  QVBoxLayout   *vert;

  meta = _meta;

  BackgroundData background = meta->value();

  switch (background.type) {
    case BgImage:
      picture = background.string;
    break;
    case BgColor:
      color = background.string;
    break;
    default:
    break;
  }

  grid = new QGridLayout(this);
  parent->setLayout(grid);

  combo = new QComboBox(this);
  combo->addItem("None");
  combo->addItem("Picture");
  combo->addItem("Solid Color");
  combo->addItem("Submodel Level Color");
  combo->setCurrentIndex(int(background.type));
  connect(combo,SIGNAL(currentIndexChanged(QString const &)),
          this, SLOT(  typeChange(         QString const &)));
  grid->addWidget(combo, 0, 0);

  /* Color */

  colorLabel = new QLabel(this);
  colorLabel->setFrameStyle(QFrame::Sunken|QFrame::Panel);
  colorLabel->setPalette(QPalette(color));
  colorLabel->setAutoFillBackground(true);
  grid->addWidget(colorLabel,0,1);
  
  colorButton = new QPushButton("Change",this);
  connect(colorButton,SIGNAL(clicked(    bool)),
          this,       SLOT(  browseColor(bool)));
  grid->addWidget(colorButton,0,2);

  /* Image */

  pictureEdit = new QLineEdit(picture,this);
  connect(pictureEdit,SIGNAL(textEdited(   QString const &)),
          this,       SLOT(  pictureChange(QString const &)));
  grid->addWidget(pictureEdit,1,0);

  pictureButton = new QPushButton("Browse",this);
  connect(pictureButton,SIGNAL(clicked(     bool)),
          this,         SLOT(  browsePicture(bool)));
  grid->addWidget(pictureButton,1,1);

  /* Fill */

  fill = new QGroupBox("Fill",this);

  vert = new QVBoxLayout(this);
  fill->setLayout(vert);
  grid->addWidget(fill,2,0,1,3);

  stretchRadio = new QRadioButton("Stretch Picture");
  connect(stretchRadio,SIGNAL(clicked(bool)),
          this,        SLOT(  stretch(bool)));
  vert->addWidget(stretchRadio);
  tileRadio    = new QRadioButton("Tile Picture");
  connect(tileRadio,SIGNAL(clicked(bool)),
          this,     SLOT(  stretch(bool)));
  vert->addWidget(tileRadio);

  enable();
}

void BackgroundGui::enable()
{
  BackgroundData background = meta->value();

  stretchRadio->setChecked(background.stretch);
  tileRadio->setChecked( !background.stretch);

  switch (background.type) {
    case BgImage:
      colorButton->setEnabled(false);
      pictureEdit->setEnabled(true);
      pictureButton->setEnabled(true);
      fill->setEnabled(true);
    break;
    case BgColor:
      colorLabel->setPalette(QPalette(color));
      // colorLabel->setAutoFillBackground(true);
      colorButton->setEnabled(true);
      pictureEdit->setEnabled(false);
      pictureButton->setEnabled(false);
      fill->setEnabled(false);
    break;
    default:
      colorButton->setEnabled(false);
      pictureEdit->setEnabled(false);
      pictureButton->setEnabled(false);
      fill->setEnabled(false);
    break;
  }
}

void BackgroundGui::typeChange(QString const &type)
{
  BackgroundData background = meta->value();

  if (type == "None") {
    background.type = BgTransparent;
  } else if (type == "Picture") {
    background.type = BgImage;
  } else if (type == "Solid Color") {
    background.type = BgColor;
  } else {
    background.type = BgSubmodelColor;
  }
  meta->setValue(background);
  enable();
  modified = true;
}
void BackgroundGui::pictureChange(QString const &pic)
{
  BackgroundData background = meta->value();
  background.string = pic;
  meta->setValue(background);
  modified = true;
}

void BackgroundGui::browsePicture(bool)
{
  BackgroundData background = meta->value();

  QString foo = QFileDialog::getOpenFileName(
    gui,
    tr("Choose Picture File"),
    picture,
    tr("Picture Files (*.png;*.jpg)"));
  if (foo != "") {
    picture = foo;
    background.string = foo;
    pictureEdit->setText(foo);
    meta->setValue(background);
    modified = true;
  }
}
void BackgroundGui::browseColor(bool)
{
  BackgroundData background = meta->value();

  QColor qcolor = LDrawColor::color(color);
  QColor newColor = QColorDialog::getColor(qcolor,this);
  if (qcolor != newColor) {
    color = newColor.name();
    background.string = newColor.name();
    colorLabel->setPalette(QPalette(newColor));
    colorLabel->setAutoFillBackground(true);
    meta->setValue(background);
    modified = true;
  }
}
void BackgroundGui::stretch(bool checked)
{
  BackgroundData background = meta->value();
  background.stretch = checked;
  meta->setValue(background);
  modified = true;
}  
void BackgroundGui::tile(bool checked)
{
  BackgroundData background = meta->value();
  background.stretch = ! checked;
  meta->setValue(background);
  modified = true;
}  

void BackgroundGui::apply(QString &modelName)
{
  if (modified) {
    MetaItem mi;
    mi.setGlobalMeta(modelName,meta);
  }
}

/***********************************************************************
 *
 * Border
 *
 **********************************************************************/

BorderGui::BorderGui(
  BorderMeta *_meta, 
  QGroupBox *parent)
{
  meta = _meta;

  BorderData border = meta->valueUnit();

  QString        string;
  QComboBox     *combo;
  QGridLayout   *grid;

  grid = new QGridLayout(this);
  parent->setLayout(grid);

  /* Combo */

  combo = new QComboBox(this);
  combo->addItem("Borderless");
  combo->addItem("Square Corners");
  combo->addItem("Round Corners");
  combo->setCurrentIndex(int(border.type));
  connect(combo,SIGNAL(currentIndexChanged(QString const &)),
          this, SLOT(  typeChange(         QString const &)));
  grid->addWidget(combo,0,0);

  /* Thickness */

  thicknessLabel = new QLabel("Width",this);
  grid->addWidget(thicknessLabel,0,1);

  string = QString("%1") .arg(border.thickness,5,'f',4);
  thicknessEdit = new QLineEdit(string,this);
  thicknessEdit->setInputMask("9.9000");
  connect(thicknessEdit,SIGNAL(textEdited(     QString const &)),
          this,         SLOT(  thicknessChange(QString const &)));
  grid->addWidget(thicknessEdit,0,2);

  /* Color */

  colorLabel = new QLabel(this);
  colorLabel->setFrameStyle(QFrame::Sunken|QFrame::Panel);
  colorLabel->setPalette(QPalette(border.color));
  colorLabel->setAutoFillBackground(true);
  grid->addWidget(colorLabel,1,0);
  
  colorButton = new QPushButton("Change",this);
  connect(colorButton,SIGNAL(clicked(    bool)),
          this,       SLOT(  browseColor(bool)));
  grid->addWidget(colorButton,1,1);

  /* Radius */

  spinLabel = new QLabel("Radius",this);
  grid->addWidget(spinLabel,2,0);

  spin = new QSpinBox(this);
  spin->setRange(0,100);
  spin->setSingleStep(5);
  spin->setValue(border.radius);
  grid->addWidget(spin,2,1);
  connect(spin,SIGNAL(valueChanged(int)),
          this,SLOT(  radiusChange(int)));

  /* Margins */

  QLabel *label;
  
  label = new QLabel("Margins",this);
  grid->addWidget(label,3,0);

  QLineEdit *lineEdit;

  string = QString("%1") .arg(border.margin[0],5,'f',4);
  lineEdit = new QLineEdit(string,this);
  grid->addWidget(lineEdit,3,1);
  connect(lineEdit,SIGNAL(textEdited(QString const &)),
          this,    SLOT(marginXChange(QString const &)));

  string = QString("%1") .arg(border.margin[1],5,'f',4);
  lineEdit = new QLineEdit(string,this);
  grid->addWidget(lineEdit,3,2);
  connect(lineEdit,SIGNAL(textEdited(QString const &)),
          this,    SLOT(marginYChange(QString const &)));  

  enable();
}
void BorderGui::enable()
{
  BorderData border = meta->value();

  switch (border.type) {
    case BdrNone:
      thicknessLabel->setEnabled(false);
      thicknessEdit->setEnabled(false);
      colorButton->setEnabled(false);
      spin->setEnabled(false);
      spinLabel->setEnabled(false);
    break;
    case BdrSquare:
      thicknessLabel->setEnabled(true);
      thicknessEdit->setEnabled(true);
      colorButton->setEnabled(true);
      spin->setEnabled(false);
      spinLabel->setEnabled(false);
    break;
    default:
      thicknessLabel->setEnabled(true);
      thicknessEdit->setEnabled(true);
      colorButton->setEnabled(true);
      spin->setEnabled(true);
      spinLabel->setEnabled(true);
    break;
  }
}

void BorderGui::typeChange(QString const &type)
{
  BorderData border = meta->valueUnit();

  if (type == "Borderless") {
    border.type = BdrNone;
  } else if (type == "Square Corners") {
    border.type = BdrSquare;
  } else {
    border.type = BdrRound;
  }
  meta->setValueUnit(border);
  enable();
  modified = true;
}
void BorderGui::browseColor(bool)
{
  BorderData border = meta->valueUnit();

  QColor color = LDrawColor::color(border.color);
  QColor newColor = QColorDialog::getColor(color,this);
  if (color != newColor) {
    border.color = newColor.name();
    meta->setValueUnit(border);
    colorLabel->setPalette(QPalette(newColor));
    modified = true;
  }
}
void BorderGui::thicknessChange(QString const &thickness)
{
  BorderData border = meta->valueUnit();
  border.thickness = thickness.toFloat();
  meta->setValueUnit(border);
  modified = true;
}
void BorderGui::radiusChange(int value)
{
  BorderData border = meta->valueUnit();
  border.radius = value;
  meta->setValueUnit(border);
  modified = true;
}

void BorderGui::apply(QString &modelName)
{
  if (modified) {
    MetaItem mi;
    mi.setGlobalMeta(modelName,meta);
  }
}
void BorderGui::marginXChange(
  QString const &string)
{
  BorderData border = meta->valueUnit();
  border.margin[0] = string.toFloat();
  meta->setValueUnit(border);
  modified = true;
}
void BorderGui::marginYChange(
  QString const &string)
{
  BorderData border = meta->valueUnit();
  border.margin[1] = string.toFloat();
  meta->setValueUnit(border);
  modified = true;
}

/***********************************************************************
 *
 * Separator
 *
 **********************************************************************/
SepGui::SepGui(
  SepMeta   *_meta,
  QGroupBox *parent)
{
  meta = _meta;

  QGridLayout *grid = new QGridLayout(this);
  parent->setLayout(grid);

  QLabel    *label;
  QLineEdit *lineEdit;
  QPushButton *button;

  label = new QLabel("Width",this);
  grid->addWidget(label,0,0);

  QString string;

  SepData sep = meta->valueUnit();

  string = QString("%1") .arg(sep.thickness,
                              5,'f',4);
  lineEdit = new QLineEdit(string,this);
  connect(lineEdit,SIGNAL(textEdited(QString const &)),
          this,    SLOT(  thicknessChange(QString const &)));
  grid->addWidget(lineEdit,0,1);

  colorExample = new QLabel(this);
  colorExample->setFrameStyle(QFrame::Sunken|QFrame::Panel);
  colorExample->setPalette(QPalette(sep.color));
  colorExample->setAutoFillBackground(true);
  grid->addWidget(colorExample,1,0);
  
  button = new QPushButton("Change");
  connect(button,SIGNAL(clicked(bool)),
          this,  SLOT(  browseColor(bool)));
  grid->addWidget(button,1,1);
  
  label = new QLabel("Margins",this);
  grid->addWidget(label,2,0);

  string = QString("%1") .arg(sep.margin[0],5,'f',4);
  lineEdit = new QLineEdit(string,this);
  grid->addWidget(lineEdit,2,1);
  connect(lineEdit,SIGNAL(textEdited(QString const &)),
          this,    SLOT(marginXChange(QString const &)));

  string = QString("%1") .arg(sep.margin[1],5,'f',4);
  lineEdit = new QLineEdit(string,this);
  grid->addWidget(lineEdit,2,2);
  connect(lineEdit,SIGNAL(textEdited(QString const &)),
          this,    SLOT(marginYChange(QString const &)));  

}
void SepGui::thicknessChange(
  QString const &string)
{
  SepData sep = meta->valueUnit();
  sep.thickness = string.toFloat();
  meta->setValueUnit(sep);
  modified = true;
}
void SepGui::browseColor(
  bool clicked)
{
  clicked = clicked;
  SepData sep = meta->valueUnit();

  QColor color = LDrawColor::color(sep.color);
  QColor newColor = QColorDialog::getColor(color,this);
  if (color != newColor) {
    sep.color = newColor.name();
    meta->setValueUnit(sep);
    colorExample->setPalette(QPalette(newColor));
    modified = true;
  }
}
void SepGui::marginXChange(
  QString const &string)
{
  SepData sep = meta->valueUnit();
  sep.margin[0] = string.toFloat();
  meta->setValueUnit(sep);
  modified = true;
}
void SepGui::marginYChange(
  QString const &string)
{
  SepData sep = meta->valueUnit();
  sep.margin[1] = string.toFloat();
  meta->setValueUnit(sep);
  modified = true;
}
void SepGui::apply(QString &modelName)
{
  if (modified) {
    MetaItem mi;
    mi.setGlobalMeta(modelName,meta);
  }
}

/***********************************************************************
 *
 * Resolution
 *
 **********************************************************************/

ResolutionGui::ResolutionGui(
  ResolutionMeta   *_meta,
  QGroupBox *parent)
{
  meta = _meta;

  QGridLayout *grid = new QGridLayout(this);
  parent->setLayout(grid);

  QLabel    *label;

  label = new QLabel("Units",this);
  grid->addWidget(label,0,0);

  type  = meta->type();
  value = meta->value();

  QComboBox *combo;

  combo = new QComboBox(this);
  combo->addItem("Dots Per Inch");
  combo->addItem("Dots Per Centimeter");
  combo->setCurrentIndex(int(type));
  connect(combo,SIGNAL(currentIndexChanged(QString const &)),
          this, SLOT(  unitsChange(        QString const &)));
  grid->addWidget(combo,0,1);

  QString string;

  string = QString("%1") .arg(int(value),4);
  valueEdit = new QLineEdit(string,this);
  valueEdit->setInputMask("9999");
  connect(valueEdit,SIGNAL(textEdited( QString const &)),
          this,     SLOT(  valueChange(QString const &)));
  grid->addWidget(valueEdit,0,2);
}

void ResolutionGui::unitsChange(QString const &units)
{
  if (units == "DPI") {
    type = DPI;
  } else {
    type = DPCM;
  }

  float tvalue;

  if (type == meta->type()) {
    tvalue = value;
  } else if (type == DPI) {
    tvalue = centimeters2inches(value)+0.5;
  } else {
    tvalue = inches2centimeters(value)+0.5;
  }

  QString string = QString("%1") .arg(int(tvalue));
  valueEdit->setText(string);
}
void ResolutionGui::valueChange(
  QString const &string)
{
  value = string.toFloat();
}
void ResolutionGui::differences()
{
  if (type == meta->type()) {
    if (value != meta->value()) {
      meta->setValue(type,value);
      modified = true;
    }
  } else if (type == DPI) {
    // We must convert all units in project to inches
    meta->setValue(type,value);
    modified = true;
  } else {
    // We must convert all units in project to centimeters
    meta->setValue(type,value);
    modified = true;
  }
}
void ResolutionGui::apply(QString &modelName)
{
  differences();
  if (modified) {
    MetaItem mi;
    mi.setGlobalMeta(modelName,meta);
  }
}
