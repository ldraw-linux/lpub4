
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
 * This is an abstract class used to represent things contained within a
 * range.  Some derived classes include step, and reserve.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef range_elementH
#define range_elementH

#include "placement.h"
#include "where.h"

/*
 * This is the base class for step and reserve
 */

class AbstractRangesElement;

class AbstractRangeElement : public Placement {
  public:
    AbstractRangesElement *parent;

    AbstractRangeElement()
    {
    }
    virtual ~AbstractRangeElement() {}

    Boundary boundary();

    Context context;
    const Where &topOfStep()
    {
      return context.topOfStep();
    }
    const Where &bottomOfStep()
    {
      return context.bottomOfStep();
    }
    const Where &topOfFile()
    {
      return context.topOfFile();
    }
    const Where &topOfRanges();
    const Where &bottomOfRanges();

    QString path();
	QString csiName();
    QStringList submodelStack();
};

#endif
