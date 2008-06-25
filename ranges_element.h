
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
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/****************************************************************************
 *
 * This abstract class is used to represent things contained in a ranges.
 * Primarily this is range.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef ranges_elementH
#define ranges_elementH

#include "where.h"
#include "placement.h"
#include "metatypes.h"

/*
 * this is the base class for a range of steps
 */

class Ranges;
class AbstractRangeElement;

class AbstractRangesElement : public Placement {
public:
  Ranges *parent;

  QList<AbstractRangeElement *> list;

  Boundary boundary(AbstractRangeElement *);

  Context      context;

  QString modelName();

  QString path();
  QString csiName();

  QStringList submodelStack();
  Context      &getContext();

  AllocMeta    &allocMeta();

  AllocEnc      allocType();

  const Where &topOfRanges();
  const Where &bottomOfRanges();
  const Where &topOfRange();
  const Where &bottomOfRange();
};

#endif
