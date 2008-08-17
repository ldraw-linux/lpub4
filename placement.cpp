
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
 * This class implements a fundamental class for placing things relative to
 * other things.  This concept is the cornerstone of LPub's meta commands
 * for describing what building instructions should look like without having
 * to specify inches, centimeters or pixels.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include "placement.h"
#include "ranges.h"
#include "callout.h"
#include "range.h"
#include "step.h"

void PlacementNum::sizeit()
{
  if (number < 1) {
    size[0] = 0;
    size[1] = 0;
  } else {
    QString string = QString("%1x") .arg(number);
    QFont   f;
    f.fromString(font);
    QFontMetrics fm(f);
    QSize fSize = fm.size(0,string);
    size[0] = fSize.width();
    size[1] = fSize.height();
  }
}

void PlacementNum::sizeit(QString format)
{
  if (number < 1) {
    size[0] = 0;
    size[1] = 0;
  } else {
    QString string = QString(format) .arg(number);
    QGraphicsTextItem gti(string);
    QFont   f;
    f.fromString(font);
    gti.setFont(f);
    size[0] = int(gti.document()->size().width());
    size[1] = int(gti.document()->size().height());
  }
}

void Placement::appendRelativeTo(Placement *element)
{
  if (element->relativeType != PageType) {
    for (int i = 0; i < relativeToList.size(); i++) {
      if (relativeToList[i] == element) {
        return;
      }
    }
    if (relativeToList.size() < 100) {
      relativeToList.append(element);
    }
  }
}

/*
 * we start with a page, and ranges, and
 * we walk through the ranges,range,steps,
 * looking for things that are relative to page.
 * We put these in the p_head list.
 *
 * foreach thing relative to page, we make a list
 * of things that are relative to them.
 */
int rc;
int Placement::relativeTo(
  Placement *pe)
{
  rc = 0;
  
  Step *step = dynamic_cast<Step *>(pe);
  if (step) {
    if (step->csiPixmap.placement.value().relativeTo == relativeType) {
      placeRelative(&step->csiPixmap);
      appendRelativeTo(&step->csiPixmap);
    }

    if (step->pli.placement.value().relativeTo == relativeType) {
      placeRelative(&step->pli);
      appendRelativeTo(&step->pli);
    }

    if (step->stepNumber.placement.value().relativeTo == relativeType) {
      placeRelative(&step->stepNumber);
      appendRelativeTo(&step->stepNumber);
    }

    /* callouts */

    for (int i = 0; i < step->list.size(); i++) {
      if (step->list[i]->relativeType == CalloutType) {
        Callout *callout = step->list[i];
        if (callout->placement.value().relativeTo == relativeType) {
          placeRelative(callout);
          appendRelativeTo(callout);
        }
      }
    } // callouts
    // Everything placed    
  } // if step

  /* try to find relation for things relative to us */
    
  int limit = relativeToList.size();
      
  if (limit < 100) {
    for (int i = 0; i < limit; i++) {
      rc = relativeToList[i]->relativeTo(pe);
      if (rc) {
        break;
      }
    }
  } else {
    rc = -1;
  }

  return rc;
}

int Placement::relativeToMs(
  Placement *placement)
{
  if (placement->relativeType == CalloutType ||
      placement->relativeType == StepGroupType) {
    Ranges *ranges = dynamic_cast<Ranges *>(placement);

    if (ranges) {
      if (ranges->pli.tsize() && 
          ranges->pli.placement.value().relativeTo == relativeType) {
        placeRelative(&ranges->pli);
        appendRelativeTo(&ranges->pli);
      }
      for (int i = 0; i < ranges->list.size(); i++) {
        if (ranges->list[i]->relativeType == RangeType) {
          Range *range = dynamic_cast<Range *>(ranges->list[i]);
          for (int i = 0; i < range->list.size(); i++) {
            if (range->list[i]->relativeType == StepType) {
              Step *step = dynamic_cast<Step *>(range->list[i]);

              /* callouts */

              for (int i = 0; i < step->list.size(); i++) {
                if (step->list[i]->relativeType == CalloutType) {
                  Callout *callout = dynamic_cast<Callout *>(step->list[i]);

                  PlacementData placementData = callout->placement.value();

                  if ((placementData.relativeTo == PageType ||
                       placementData.relativeTo == StepGroupType) &&
                       placementData.relativeTo == relativeType) {
                    placeRelative(callout);
                    placement->appendRelativeTo(callout);
                  }
                } // if callout
              } // callouts
            } // if step
          } // foreach step
        } // if range
      } // foreach range
    } // if ranges

    /* try to find relation for things relative to us */

    for (int i = 0; i < relativeToList.size(); i++) {
      if (relativeToList[i]->relativeToMs(ranges)) {
        return -1;
      }
    }

    // Everything placed

    return 0;
  } else {
    return -1;
  }
}

void Placement::placeRelative(
  Placement *them)
{
  int disp[2];

  disp[XX] = (int) (size[XX] * them->placement.value().offsets[XX]);
  disp[YY] = (int) (size[YY] * them->placement.value().offsets[YY]);

  float lmargin[2];
  lmargin[XX] = qMax(margin.value(XX),them->margin.value(XX));
  lmargin[YY] = qMax(margin.value(YY),them->margin.value(YY));

  PlacementData placementData = them->placement.value();

  if (placementData.preposition == Outside) {
    switch (placementData.placement) {
      case TopLeft:
      case Left:
      case BottomLeft:
        them->offset[XX] = offset[XX] - them->size[XX] - lmargin[XX];
      break;
      case TopRight:
      case Right:
      case BottomRight:
        them->offset[XX] = offset[XX] + size[XX] + lmargin[XX];
      break;
      case Top:
      case Bottom:
        them->offset[XX] = offset[XX];
        switch (placementData.justification) {
          case Center:
            them->offset[XX] += (size[XX]-them->size[XX])/2;
          break;
          case Right:
            them->offset[XX] += size[XX]-them->size[XX];
          break;
          default:
          break;
        }
      break;
      case Center:
        them->offset[XX] = offset[XX];
      break;
      default:
      break;
    }
    switch (placementData.placement) {
      case TopLeft:
      case Top:
      case TopRight:
        them->offset[YY] = offset[YY] - them->size[YY] - lmargin[YY];
      break;
      case BottomLeft:
      case Bottom:
      case BottomRight:
        them->offset[YY] = offset[YY] + size[YY] + lmargin[YY];
      break;
      case Left:
      case Right:
        them->offset[YY] = offset[YY];
        switch(placementData.justification) {
          case Center:
            them->offset[YY] += (size[YY]-them->size[YY])/2;
          break;
          case Bottom:
            them->offset[YY] += size[YY]-them->size[YY];
          break;
          default:
          break;
        }
      break;
      default:
      break;
    }
  } else {
    switch (placementData.placement) {
      case TopLeft:
      case Left:
      case BottomLeft:
        them->offset[XX] = offset[XX] + lmargin[XX];
      break;
      case Top:
      case Bottom:
        them->offset[XX] = offset[XX] + (size[XX]-them->size[XX])/2;
      break;
      case TopRight:
      case Right:
      case BottomRight:
        them->offset[XX] = offset[XX] + size[XX] - them->size[XX] - lmargin[XX];
      break;
      case Center:
        them->offset[XX] = offset[XX] + (size[XX]-them->size[XX])/2;
      break;
      default:
      break;
    }

    switch (placementData.placement) {
      case TopLeft:
      case Top:
      case TopRight:
        them->offset[YY] = offset[YY] + lmargin[YY];
      break;
      case Left:
      case Right:
        them->offset[YY] = offset[YY] + (size[YY] - them->size[YY])/2;
      break;
      case BottomLeft:
      case Bottom:
      case BottomRight:
        them->offset[YY] = offset[YY] + size[YY] - them->size[YY] - lmargin[YY];
      break;
      case Center:
        them->offset[YY] = offset[YY] + (size[YY] - them->size[YY])/2;
      break;
      default:
      break;
    }
  }
  them->offset[XX] += disp[XX];
  them->offset[YY] += disp[YY];
}

void Placement::placeRelative(
  Placement   *them,
  int          margin[2])
{
  int lmargin[2];
  lmargin[XX] = them->margin.value(XX);
  lmargin[YY] = them->margin.value(YY);
  lmargin[XX] = qMax(margin[XX],lmargin[XX]);
  lmargin[YY] = qMax(margin[YY],lmargin[YY]);

  PlacementData placementData = them->placement.value();

  if (placementData.preposition == Outside) {
    switch (placementData.placement) {
      case TopLeft:
      case Left:
      case BottomLeft:
        them->offset[XX] = offset[XX] - them->size[XX] - lmargin[XX];
      break;
      case TopRight:
      case Right:
      case BottomRight:
        them->offset[XX] = offset[XX] + size[XX] + lmargin[XX];
      break;
      case Top:
      case Bottom:
        them->offset[XX] = offset[XX];
        switch (placementData.justification) {
          case Center:
            them->offset[XX] += (size[XX]-them->size[XX])/2;
          break;
          case Right:
            them->offset[XX] += size[XX]-them->size[XX];
          break;
          default:
          break;
        }
      break;
      case Center:
        them->offset[XX] = offset[XX];
      break;
      default:
      break;
    }
    switch (placementData.placement) {
      case TopLeft:
      case Top:
      case TopRight:
        them->offset[YY] = offset[YY] - them->size[YY] - lmargin[YY];
      break;
      case BottomLeft:
      case Bottom:
      case BottomRight:
        them->offset[YY] = offset[YY] + size[YY] + lmargin[YY];
      break;
      case Left:
      case Right:
        them->offset[YY] = offset[YY];
        switch(placementData.justification) {
          case Center:
            them->offset[YY] += (size[YY]-them->size[YY])/2;
          break;
          case Bottom:
            them->offset[YY] += size[YY]-them->size[YY];
          break;
          default:
          break;
        }
      break;
      default:
      break;
    }
  } else {
    switch (placementData.placement) {
      case TopLeft:
      case Left:
      case BottomLeft:
        them->offset[XX] = offset[XX] + lmargin[XX];
      break;
      case Top:
      case Bottom:
        them->offset[XX] = offset[XX] + (size[XX]-them->size[XX])/2;
      break;
      case TopRight:
      case Right:
      case BottomRight:
        them->offset[XX] = offset[XX] + size[XX] - them->size[XX] - lmargin[XX];
      break;
      case Center:
        them->offset[XX] = offset[XX] + (size[XX]-them->size[XX])/2;
      break;
      default:
      break;
    }

    switch (placementData.placement) {
      case TopLeft:
      case Top:
      case TopRight:
        them->offset[YY] = offset[YY] + lmargin[YY];
      break;
      case Left:
      case Right:
        them->offset[YY] = offset[YY] + (size[YY] - them->size[YY])/2;
      break;
      case BottomLeft:
      case Bottom:
      case BottomRight:
        them->offset[YY] = offset[YY] + size[YY] - them->size[YY] - lmargin[YY];
      break;
      case Center:
        them->offset[YY] = offset[YY] + (size[YY]-them->size[YY])/2;
      break;
      default:
      break;
    }
  }
  int disp[2];

  disp[XX] = (int) (size[XX] * them->placement.value().offsets[XX]);
  disp[YY] = (int) (size[YY] * them->placement.value().offsets[YY]);
  them->offset[XX] += disp[XX];
  them->offset[YY] += disp[YY];
}

void Placement::justifyX(
  int          origin,
  int          height)
{
  switch (placement.value().placement) {
    case Top:
    case Bottom:
      switch (placement.value().justification) {
        case Left:
          offset[XX] = origin;
        break;
        case Center:
          offset[XX] = origin + (height - size[XX])/2;
        break;
        case Right:
          offset[XX] = origin + height - size[XX];
        break;
        default:
        break;
      }
    break;
    default:
    break;
  }
}

void Placement::justifyY(
  int          origin,
  int          height)
{
  switch (placement.value().placement) {
    case Left:
    case Right:
      switch (placement.value().justification) {
        case Top:
          offset[YY] = origin;
        break;
        case Center:
          offset[YY] = origin + (height - size[YY])/2;
        break;
        case Bottom:
          offset[YY] = origin + height - size[YY];
        break;
        default:
        break;
      }
    break;
    default:
    break;
  }
}

