
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
 * This file describes the graphical representation of a number displayed
 * in the graphics scene that is used to describe a building instruction
 * page.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "numberitem.h"
#include "metatypes.h"
#include <QColor>
#include <QPixmap>
#include <QAction>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>
#include "color.h"
#include "name.h"
#include "placementdialog.h"
#include "commonmenus.h"

#include "ranges.h"
#include "step.h"

void NumberItem::setAttributes(
  PlacementType  _relativeType,
  PlacementType  _parentRelativeType,
  Meta          *_meta,
  NumberMeta    &_number,
  const char    *_format,
  int            _value,
  QString       &toolTip,
  QGraphicsItem *_parent,
  QString        _name)
{
  relativeType = _relativeType;
  parentRelativeType = _parentRelativeType;
  meta         =  _meta;
  font         = &_number.font;
  color        = &_number.color;
  margin       = &_number.margin;
  value        = _value;
  name         = _name;

  QFont qfont;
  qfont.fromString(_number.font.value());
  setFont(qfont);

  QString foo;
  foo.sprintf(_format,_value);
  setPlainText(foo);
  setDefaultTextColor(LDrawColor::color(color->value()));
  setToolTip(toolTip);
  setParentItem(_parent);
}

NumberItem::NumberItem()
{
  relativeType = PageNumberType;
  meta = NULL;
  font = NULL;
  color = NULL;
  margin = NULL;
}

NumberItem::NumberItem(
  PlacementType  _relativeType,
  PlacementType  _parentRelativeType,
  Meta          *_meta,
  NumberMeta    &_number,
  const char    *_format,
  int            _value,
  QString       &_toolTip,
  QGraphicsItem *_parent,
  QString        _name)
{
  setAttributes(_relativeType,
                _parentRelativeType,
                _meta,
                _number,
                _format,
                _value,
                _toolTip,
                _parent,
                _name);
}

NumberPlacementItem::NumberPlacementItem()
{
  relativeType = PageNumberType;
  meta = NULL;
  font = NULL;
  color = NULL;
  margin = NULL;
  placement = NULL;
}

NumberPlacementItem::NumberPlacementItem(
  PlacementType  _relativeType,
  PlacementType  _parentRelativeType,
  Meta          *_meta,
  NumberPlacementMeta &_number,
  const char    *_format,
  int            _value,
  QString       &toolTip,
  QGraphicsItem *_parent,
  QString        _name)
{
  setAttributes(_relativeType,
                _parentRelativeType,
                _meta,
                _number,
                _format,
                _value,
                toolTip,
                _parent,
                _name);
}

void NumberPlacementItem::setAttributes(
  PlacementType  _relativeType,
  PlacementType  _parentRelativeType,
  Meta          *_meta,
  NumberPlacementMeta &_number,
  const char    *_format,
  int            _value,
  QString       &toolTip,
  QGraphicsItem *_parent,
  QString        _name)
{
  relativeType = _relativeType;
  parentRelativeType = _parentRelativeType;
  meta         =  _meta;
  font         = &_number.font;
  color        = &_number.color;
  margin       = &_number.margin;
  placement    = &_number.placement;
  value        = _value;
  name         = _name;

  QFont qfont;
  qfont.fromString(_number.font.value());
  setFont(qfont);

  QString foo;
  foo.sprintf(_format,_value);
  setPlainText(foo);
  setDefaultTextColor(LDrawColor::color(color->value()));

  setToolTip(toolTip);
  setParentItem(_parent);
}

PageNumberItem::PageNumberItem(
  Page          *_page,
  Meta          *_meta,
  NumberPlacementMeta &_number,
  const char    *_format,
  int            _value,
  QGraphicsItem *_parent)
{
  page = _page;
  QString toolTip("Page Number");
  setAttributes(PageNumberType,
                SingleStepType,
                _meta,
                _number,
                _format,
                _value,
                toolTip,
                _parent);
}

void PageNumberItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;

  PlacementData placementData = meta->LPub.page.number.placement.value();
  QString name = "Move Page Number";
  QAction *placementAction  = menu.addAction(name);
  placementAction->setWhatsThis(
    commonMenus.naturalLanguagePlacementWhatsThis(PageNumberType,placementData,name));

  QAction *fontAction       = menu.addAction("Change Page Number Font");
  QAction *colorAction      = menu.addAction("Change Page Number Color");
  QAction *marginAction     = menu.addAction("Change Page Number Margins");

  fontAction->setWhatsThis("You can change the font or the size of the page number");
  colorAction->setWhatsThis("You can change the color of the page number");
  marginAction->setWhatsThis("You can change how much empty space their is around the page number");

  QAction *selectedAction   = menu.exec(event->screenPos());

  if (selectedAction == placementAction) {

    changePlacement(PageType,
                    PageNumberType,
                    "Page Number Placement",
                    page->topOfSteps(),
                    page->bottomOfSteps(),
                    placement);

  } else if (selectedAction == fontAction) {

    changeFont(page->topOfSteps(),page->bottomOfSteps(),font);

  } else if (selectedAction == colorAction) {

    changeColor(page->topOfSteps(),page->bottomOfSteps(),color);

  } else if (selectedAction == marginAction) {

    changeMargins("Page Number Margins",
                  page->topOfSteps(),page->bottomOfSteps(),
                  margin);
  }
}

StepNumberItem::StepNumberItem(
  Step          *_step,
  PlacementType  parentRelativeType,
  Meta          *_meta,
  NumberPlacementMeta    &_number,
  const char    *_format,
  int            _value,
  QGraphicsItem *_parent,
  QString        name)
{
  step = _step;
  QString toolTip("Step Number");
  setAttributes(StepNumberType,
                parentRelativeType,
                _meta,
                _number,
                _format,
                _value,
                toolTip,
                _parent,
                name);
}

void StepNumberItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
  QAction *placementAction = commonMenus.placementMenu(menu,name);
  QAction *fontAction      = commonMenus.fontMenu(menu,name);
  QAction *colorAction     = commonMenus.colorMenu(menu,name);
  QAction *marginAction    = commonMenus.marginMenu(menu,name);

  QAction *selectedAction   = menu.exec(event->screenPos());
  
  Where topOfStep = step->topOfStep();
  Where bottomOfStep = step->bottomOfStep();
  Where topOfSteps = step->topOfSteps();
  Where bottomOfSteps = step->bottomOfSteps();
  
  Where top, bottom;
  bool  local;
  MetaItem mi;
  
  switch (parentRelativeType) {
    case StepGroupType:
      top    = step->topOfSteps();
      mi.scanForward(top,StepGroupMask);
      bottom = step->bottomOfSteps();
      local  = false;
    break;
    case CalloutType:
      top    = step->topOfCallout();
      bottom = step->bottomOfCallout();
      local  = false;
    break;
    default:
      top    = step->topOfStep();
      bottom = step->bottomOfStep();
      local  = true;
    break;
  }

  if (selectedAction == placementAction) {

    changePlacement(parentRelativeType,
                    StepNumberType,
                    "Move Step Number", 
                    top,
                    bottom,
                    placement,
                    1,local);

  } else if (selectedAction == fontAction) {

    changeFont(top, bottom, font, 1, local);

  } else if (selectedAction == colorAction) {

    changeColor(top,bottom,color, 1, local);

  } else if (selectedAction == marginAction) {

    changeMargins("Change Step Number Margins",top,bottom,margin,1,local);
  } 
}
