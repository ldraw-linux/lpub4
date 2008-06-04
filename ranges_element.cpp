
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
 * This abstract class is used to represent things contained in a ranges.
 * Primarily this is range.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include "ranges_element.h"
#include "ranges.h"

Context &AbstractRangesElement::getContext()
{
  return parent->meta.context;
}

AllocEnc AbstractRangesElement::allocType()
{
  return parent->allocType();
}

AllocMeta &AbstractRangesElement::allocMeta()
{
  return parent->allocMeta();
}

const Where &AbstractRangesElement::topOfRanges()
{
  return parent->topOfRanges();
}  

const Where &AbstractRangesElement::bottomOfRanges()
{
  return parent->bottomOfRanges();
}  

QString AbstractRangesElement::path()
{
  return parent->path();
}

QString AbstractRangesElement::csiName()
{
  return parent->csiName();
}

QStringList AbstractRangesElement::submodelStack()
{
  return parent->submodelStack();
}

QString AbstractRangesElement::modelName()
{
  return parent->modelName();
}

Boundary AbstractRangesElement::boundary(AbstractRangeElement *me)
{
  Boundary myBoundary = parent->boundary(this);

  switch (myBoundary) {
    case StartOfRanges:
      if (list.size() == 1) {
        return Boundary(StartOfRanges|StartOfRange|EndOfRange);
      }
      if (list[0] == me) {
        return Boundary(StartOfRanges|StartOfRange);
      }
    break;
    case EndOfRanges:
      if (list.size() == 1) {
        return Boundary(StartOfRange|EndOfRange|EndOfRanges);
      }
      if (list[list.size()-1] == me) {
        return Boundary(EndOfRange|EndOfRanges);
      }
    break;
    case StartAndEndOfRanges:
      if (list.size() == 1) {
        return StartAndEndOfRanges;
      } else if (list[0] == me) {
        return Boundary(StartOfRanges|StartOfRange);
      } else if (list[list.size()-1] == me) {
        return Boundary(EndOfRange|EndOfRanges);
      }
    break;
    default:
    break;
  }
  if (list.size() == 1) {
    return StartAndEndOfRange;
  }
  if (list[0] == me) {
    return StartOfRange;
  }
  if (list[list.size()-1] == me) {
    return EndOfRange;
  }
  return Middle;
}
