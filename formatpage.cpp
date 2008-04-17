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

/***************************************************************************
 *
 * This file invokes the traverse function to count pages, and to gather
 * the contents of a given page of your building instructions.  Once 
 * gathered, the contents of the page are translated to graphical representation
 * and presented to the user for editing.
 *
 **************************************************************************/
 
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QScrollBar>
#include <QPixmap>
#include <QColor>
#include "callout.h"
#include "lpub.h"
#include "ranges.h"
#include "range.h"
#include "step.h"
#include "meta.h"
#include "color.h"
#include "pagebackgrounditem.h"
#include "numberitem.h"
#include "csiitem.h"
#include "calloutbackgrounditem.h"

/*
 * We need to draw page every time there is change to the LDraw file.
 *   Changes can come from Menu->dialogs, people editing the file.
 *
 * Gui tracks modified, so whenever things go modified, we need to
 * delete all the GraphicsItems and do a freeRanges.
 */

void Gui::clearPage()
{
  page.freeRanges();
  page.pli.clear();

  if (pageView->pageBackgroundItem) {
    delete pageView->pageBackgroundItem;
    pageView->pageBackgroundItem = NULL;
  }
}

/*********************************************
 *
 * given a Ranges for a page, format the
 * entire page.
 *
 ********************************************/

int Gui::addGraphicsPageItems(
  Ranges         *ranges,
  QGraphicsScene *scene)
{
  statusBarMsg("Displaying Page");

  PageBackgroundItem *pageBg = new PageBackgroundItem(ranges);

  pageView->pageBackgroundItem = pageBg;

  Placement plPage;
  plPage.relativeType = PageType;
  plPage.size[XX] = ranges->meta.LPub.page.size.value(0);
  plPage.size[YY] = ranges->meta.LPub.page.size.value(1);
  plPage.margin   = ranges->meta.LPub.page.margin;
  plPage.offset[XX] = 0;
  plPage.offset[YY] = 0;

  if (ranges->meta.LPub.page.dpn.value()) {

    // allocate QGraphicsTextItem for page number

    PageNumberItem  *pageNumber = 
      new PageNumberItem(&ranges->meta, 
                     ranges->meta.LPub.page.number, 
                     "%d", 
                     displayPageNum,
                     pageBg);

    PlacementData placementData = ranges->meta.LPub.page.number.placement.value();

    Placement pn;

    pn.placement.setValue(placementData);
    pn.relativeType = PageNumberType;
    pn.size[XX]     = (int) pageNumber->document()->size().width();
    pn.size[YY]     = (int) pageNumber->document()->size().height();
    pn.margin       = ranges->meta.LPub.page.number.margin;

    if (ranges->meta.LPub.page.togglePnPlacement.value() && ! (displayPageNum & 1)) {
      switch (placementData.placement) {
        case TopLeft:
          placementData.placement = TopRight;
        break;
        case Top:
        case Bottom:
          switch (placementData.justification) {
            case Left:
              placementData.justification = Right;
            break;
            case Right:
              placementData.justification = Left;
            break;
            default:
            break;
          }
        break;
        case TopRight:
          placementData.placement = TopLeft;
        break;
        case Left:
          placementData.placement = Right;
        break;
        case Right:
          placementData.placement = Left;
        break;
        case BottomLeft:
          placementData.placement = BottomRight;
        break;
        case BottomRight:
          placementData.placement = BottomLeft;
        break;
        default:
        break;
      }

      pn.placement.setValue(placementData);
    }

    plPage.placeRelative(&pn);
    pageNumber->setPos(pn.offset[XX],pn.offset[YY]);
  }

  if (ranges->relativeType == SingleStepType) {
    if (ranges->list.size()) {
      Range *range = dynamic_cast<Range *>(ranges->list[0]);
      if (range->relativeType == RangeType) {
        Step *step= dynamic_cast<Step *>(range->list[0]);
        if (step && step->relativeType == StepType) {

          step->stepNumber.sizeit();

          step->pli.addPli(&ranges->meta, step->submodelLevel, pageBg);

          /* Size the callouts */
          for (int i = 0; i < step->list.size(); i++) {
            step->list[i]->sizeIt();
          }

          plPage.relativeTo(step);      // place everything

          step->pli.setPos(step->pli.offset[XX],
                           step->pli.offset[YY]);

          CsiItem *csiItem = NULL;
          if (step->csiPixmap.pixmap) {
            csiItem = new CsiItem(step,
                                 &ranges->meta, 
                                 *step->csiPixmap.pixmap,
                                  step->context,
                                  step->submodelLevel,
                                  pageBg, 
                                  ranges->relativeType);
            csiItem->setPos(step->csiPixmap.offset[XX],
                            step->csiPixmap.offset[YY]);
          } else {
            exit(-1);
          }

          // allocate QGraphicsTextItem for step number

          if (ldrawFile.numSteps(ranges->meta.context.topOfFile().modelName) > 1) {
            StepNumberItem *stepNumber = 
              new StepNumberItem(ranges->relativeType,
                                 step->topOfRanges(),
                                &ranges->meta, 
                                 ranges->meta.LPub.stepNumber, 
                                 "%d", 
                                 step->stepNumber.number,
                                 pageBg);

            stepNumber->setPos(step->stepNumber.offset[XX],
                               step->stepNumber.offset[YY]);
          }

          for (int i = 0; i < step->list.size(); i++) {
            Callout *callout = step->list[i];
            QRect    csiRect(step->csiPixmap.offset[XX]-callout->offset[XX],
                             step->csiPixmap.offset[YY]-callout->offset[YY],
                             step->csiPixmap.size[XX], 
                             step->csiPixmap.size[YY]);

            callout->addGraphicsItems(0,0,csiRect,pageBg);
            for (int i = 0; i < callout->pointerList.size(); i++) {
              Pointer *pointer = callout->pointerList[i];
              callout->addGraphicsPointerItem(callout->offset[XX],
                                              callout->offset[YY],
                                              0,
                                              0,
                                              step->csiPixmap.offset,
                                              step->csiPixmap.size,
                                              pointer,
                                              callout->background);
            }
          }
        } else {
          QMessageBox::warning(
            NULL,
            QMessageBox::tr("LPub"),
            QMessageBox::tr("drawPage(): not a valid step"));
        }
      } else {
        QMessageBox::warning(
          NULL,
          QMessageBox::tr("LPub"),
          QMessageBox::tr("drawPage(): not a valid range"));
      }
    } else {
      QMessageBox::warning(
        NULL,
        QMessageBox::tr("LPub"),
        QMessageBox::tr("drawPage(): not a valid ranges"));
    }
  } else {
    PlacementData data = ranges->meta.LPub.multiStep.placement.value();
    ranges->placement.setValue(data);
    ranges->sizeIt();
    plPage.placeRelative(ranges); // place multi-step relative to the page
    ranges->relativeToMs(ranges); // place callouts relative to MULTI_STEP
    plPage.relativeToMs(ranges);    // place callouts relative to PAGE
    ranges->addGraphicsItems(0,0,pageBg);
  }
  scene->addItem(pageBg);

  pageView->setSceneRect(0,0,ranges->meta.LPub.page.size.value(0),
                             ranges->meta.LPub.page.size.value(1));

  pageView->horizontalScrollBar()->setRange(0,ranges->meta.LPub.page.size.value(0));
  pageView->verticalScrollBar()->setRange(0,ranges->meta.LPub.page.size.value(1));

  if (fitMode == FitWidth) {
    fitWidth();
  } else if (fitMode == FitVisible) {
    fitVisible();
  }

#if 0


  // draw the inserted pictures

  for (unsigned i = 0; i < meta->page.inserts.picList.size(); i++) {
    pic_inserted *pic = meta->page.inserts.picList[i];

    QFileInfo pic(pic->fileName);
    if (pic.exists()) {

      TPNGObject *png = new TPNGObject();

      png->LoadFromFile(pic->filename);

      translucent->DrawAt(
        pic->pic_pos[0]*meta->page.size.x+meta->page.margin.size[0],
        pic->pic_pos[1]*meta->page.size.y+meta->page.margin.size[1],
        png);

      delete png;
    }
  }
#endif
  ranges->relativeType = SingleStepType;
  statusBarMsg("");
  return 0;
}
