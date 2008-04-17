
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
 * This derived class allows the user to create empty spaces within
 * callouts and multi-steps.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef reserveH
#define reserveH

#include <QGraphicsScene>
#include <QGraphicsItem>
#include "meta.h"
#include "where.h"
#include "range_element.h"

class Reserve : public AbstractRangeElement {
  public:
    Where here;

    Reserve(
      Where    &_here,

      LPubMeta &meta)
    {
      here = _here;

      float space;
      space = meta.reserve.value();
      relativeType = ReserveType;
      if (meta.multiStep.alloc.value() == Horizontal) {
        size[XX] = int(meta.page.size.value(0) * space);
        size[YY] = 0;
      } else {
        size[XX] = 0;
        size[YY] = int(meta.page.size.value(1) * space);
      }
    }
};

#endif
