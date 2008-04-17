
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
* sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/****************************************************************************
 *
 * This file represents the page background and is derived from the generic
 * background class described in background.(h,cpp)
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef BACKGROUNDITEM_H
#define BACKGROUNDITEM_H

#include <QGraphicsScene>
#include "backgrounditem.h"
#include "ranges.h"

class PageBackgroundItem : public BackgroundItem
{
  private:
    PlacementType  relativeType;
    QPixmap *pixmap;
    Ranges  *page;

  public:
    PageBackgroundItem(
      Ranges   *ranges)
    {
      page = ranges;

      int width,height;

      relativeType = ranges->relativeType;
      width = ranges->meta.LPub.page.size.value(0);
      height= ranges->meta.LPub.page.size.value(1);
      pixmap = new QPixmap(width,height);

      QString toolTip("");
      setBackground(pixmap,
                    PageType,
                   &ranges->meta,
                    ranges->meta.LPub.page.background,
                    ranges->meta.LPub.page.border,
                    ranges->meta.LPub.page.margin,
                    ranges->meta.LPub.page.subModelColor,
                    ranges->meta.submodelStack.size(),
                    toolTip);
      setPixmap(*pixmap);
      delete pixmap;
    }
    ~PageBackgroundItem()
    {
//      delete pixmap;
    }
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};

#endif
