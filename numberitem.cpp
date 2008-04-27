
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
  Meta          *_meta,
  NumberPlacementMeta &_number,
  const char    *_format,
  int            _value,
  QGraphicsItem *_parent)
{
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

  QAction *fontAction       = menu.addAction("Edit Page Number Font");
  QAction *colorAction      = menu.addAction("Edit Page Number Color");
  QAction *marginAction     = menu.addAction("Edit Page Number Margins");
  QAction *selectedAction   = menu.exec(event->screenPos());

  fontAction->setWhatsThis("You can change the font or the size of the page number");
  colorAction->setWhatsThis("You can change the color of the page number");
  marginAction->setWhatsThis("You can change how much empty space their is around the page number");

  if (selectedAction == placementAction) {

    changePlacement(PageType,
                    PageNumberType,
                    "Page Number Placement",
                    meta->context.topOfStep(),
                    meta->context.bottomOfStep(),
                    placement);

  } else if (selectedAction == fontAction) {

    changeFont(meta->context.topOfStep(),font);

  } else if (selectedAction == colorAction) {

    changeColor(meta->context.topOfStep(),color);

  } else if (selectedAction == marginAction) {

    QString foo = PlacementDialog::relativeToName(relativeType);

    changeMargins("Page Number Margins",meta->context.topOfStep(),margin);
  }
}

StepNumberItem::StepNumberItem(
  PlacementType  parentRelativeType,
  const Where   &_topOfRanges,
  const Where   &_bottomOfRanges,
  Meta          *_meta,
  NumberPlacementMeta    &_number,
  const char    *_format,
  int            _value,
  QGraphicsItem *_parent,
  QString        name)
{
  topOfRanges = _topOfRanges;
  bottomOfRanges = _bottomOfRanges;
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

  if (selectedAction == placementAction) {

    switch (parentRelativeType) {
      case StepGroupType:
        changePlacement(parentRelativeType,
                        StepNumberType,
                        "Move Step Number", 
                        topOfRanges,
                        bottomOfRanges,
                        placement,
                        false);
      break;
      case CalloutType:
        changePlacement(parentRelativeType,
                        StepNumberType,
                        "Move Step Number", 
                        topOfRanges,
                        bottomOfRanges,
                        placement,
                        false);
      break;
      case SingleStepType:
        changePlacement(parentRelativeType,
                        StepNumberType,
                        "Move Step Number", 
                        meta->context.topOfStep(),
                        meta->context.bottomOfStep(),
                        placement,
                        true);
      break;
      default:
      break;
    }
  } else if (selectedAction == fontAction) {

    switch (parentRelativeType) {
      case StepGroupType:
        changeFont(topOfRanges, font,false);
      break;
      case CalloutType:
        changeFont(topOfRanges-1, font, false);
      break;
      case SingleStepType:
        changeFont(meta->context.topOfStep(),font);
      break;
      default:
      break;
    }
  } else if (selectedAction == colorAction) {

    switch (parentRelativeType) {
      case StepGroupType:
        changeColor(topOfRanges,color,false);
      break;
      case CalloutType:
        changeColor(topOfRanges-1,color,false);
      break;
      case SingleStepType:
        changeColor(meta->context.topOfStep(),color);
      break;
      default:
      break;
    }
  } else if (selectedAction == marginAction) {

    switch (parentRelativeType) {
      case StepGroupType:
        changeMargins("Change Step Number Margins",topOfRanges, margin, false);
      break;
      case CalloutType:
        changeMargins("Change Step Number Margins",topOfRanges-1, margin, false);
      break;
      case SingleStepType:
        changeMargins("Change Step Number Margins",meta->context.topOfStep(), margin);
      break;
      default:
      break;
    }
  } 
}
