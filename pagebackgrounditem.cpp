
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
 * This file represents the page background and is derived from the generic
 * background class described in background.(h,cpp)
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "pagebackgrounditem.h"
#include <QAction>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>
#include <QFileDialog>
#include "commonmenus.h"

void PageBackgroundItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;

  QAction *calloutAction = NULL;
  QAction *ignoreAction = NULL;

  if (page->meta.submodelStack.size() > 0) {
    calloutAction = menu.addAction("Convert to Callout");
    calloutAction->setWhatsThis("Convert to Callout:\n"
                                "  A callout shows how to build these steps in a picture next\n"
                                "  to where it is added the the set you are building");
  
    ignoreAction  = menu.addAction("Ignore this submodel");
    ignoreAction->setWhatsThis("Stops these steps from showing up in your instructions");
  }

  QAction *selectedAction     = menu.exec(event->screenPos());

  if (selectedAction == NULL) {
    return;
  }

  if (selectedAction == calloutAction) {
    convertToCallout(&page->meta);

  } else if (selectedAction == ignoreAction) {
    convertToIgnore(&page->meta);

  }
}
