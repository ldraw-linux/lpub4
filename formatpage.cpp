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
 * delete all the GraphicsItems and do a freeranges.h.
 */

void Gui::clearPage(
  LGraphicsView  *view,
  QGraphicsScene * /* scene - unused */)
{
  page.freePage();
  page.pli.clear();

  if (view->pageBackgroundItem) {
    delete view->pageBackgroundItem;
    view->pageBackgroundItem = NULL;
  }
}

/*********************************************
 *
 * given a ranges.h for a page, format the
 * entire page.
 *
 ********************************************/
 
class SubmodelInstanceCount : public NumberPlacementItem
{
  Page *page;
  
  public:
  
    SubmodelInstanceCount(
      Page                *pageIn,
      Meta                *metaIn,
      NumberPlacementMeta &numberMetaIn,
      const char          *formatIn,
      int                  valueIn,
      QGraphicsItem       *parentIn)
    {
      page = pageIn;
      QString toolTip("Number of times to build this submodel");
      setAttributes(PageNumberType,
                    SingleStepType,
                    metaIn,
                    numberMetaIn,
                    formatIn,
                    valueIn,
                    toolTip,
                    parentIn);
    }
  protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};

void SubmodelInstanceCount::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;

  PlacementData placementData = meta->LPub.page.instanceCount.placement.value();

  QAction *fontAction       = menu.addAction("Change Submodel Count Font");
  QAction *colorAction      = menu.addAction("Change Submodel Count Color");
  QAction *marginAction     = menu.addAction("Change Submodel Count Margins");

  fontAction->setWhatsThis("You can change the font or the size of the page number");
  colorAction->setWhatsThis("You can change the color of the page number");
  marginAction->setWhatsThis("You can change how much empty space their is around the page number");

  QAction *selectedAction   = menu.exec(event->screenPos());

  if (selectedAction == fontAction) {

    changeFont(page->topOfSteps(),page->bottomOfSteps(),font);

  } else if (selectedAction == colorAction) {

    changeColor(page->topOfSteps(),page->bottomOfSteps(),color);

  } else if (selectedAction == marginAction) {

    changeMargins("Submodel Count Margins",
                  page->topOfSteps(),page->bottomOfSteps(),
                  margin);
  }
}

int Gui::addGraphicsPageItems(
  Steps          *steps,
  bool            coverPage,
  bool            endOfSubmodel,
  int             instances,
  LGraphicsView  *view,
  QGraphicsScene *scene)
{
  Page *page = dynamic_cast<Page *>(steps);

  PageBackgroundItem *pageBg = new PageBackgroundItem(page);

  view->pageBackgroundItem = pageBg;

  Placement plPage;
  plPage.relativeType = PageType;
  plPage.size[XX] = page->meta.LPub.page.size.value(0);
  plPage.size[YY] = page->meta.LPub.page.size.value(1);
  plPage.margin   = page->meta.LPub.page.margin;
  plPage.offset[XX] = 0;
  plPage.offset[YY] = 0;

  if (page->meta.LPub.page.dpn.value() && ! coverPage) {

    // allocate QGraphicsTextItem for page number

    PageNumberItem  *pageNumber = 
      new PageNumberItem(
                      page,
                     &page->meta, 
                      page->meta.LPub.page.number, 
                     "%d", 
                     stepPageNum,
                     pageBg);

    PlacementData placementData = page->meta.LPub.page.number.placement.value();

    Placement pn;

    pn.placement.setValue(placementData);
    pn.relativeType = PageNumberType;
    pn.size[XX]     = (int) pageNumber->document()->size().width();
    pn.size[YY]     = (int) pageNumber->document()->size().height();
    pn.margin       = page->meta.LPub.page.number.margin;            
    
    // if this page contains the last step of the page, 
    // and instance is > 1 then display instance

    // allocate QGraphicsTextItem for instance number 
    
    SubmodelInstanceCount *instanceCount;
    
    if (endOfSubmodel && instances > 1) {
      instanceCount = new SubmodelInstanceCount(
         page,
        &page->meta,
        page->meta.LPub.page.instanceCount,
        "x%d",
        instances,
        pageBg);
    } else {
      instanceCount = NULL;
    }

    Placement ic;
        
    PlacementData icPlacementData = page->meta.LPub.page.instanceCount.placement.value();

    icPlacementData.placement = BottomRight;
    icPlacementData.relativeTo = PageType;
    icPlacementData.preposition = Inside;
    
    if (page->meta.LPub.page.togglePnPlacement.value() && ! (stepPageNum & 1)) {
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
          
    if (placementData.placement == BottomRight) {
      icPlacementData.placement = Top;
      icPlacementData.justification = Right;
      icPlacementData.relativeTo = PageNumberType;
      icPlacementData.preposition = Outside;
    }
    
    plPage.appendRelativeTo(&pn);
    plPage.placeRelative(&pn);
    pageNumber->setPos(pn.offset[XX],pn.offset[YY]);
    
    if (instanceCount) {
      ic.placement.setValue(icPlacementData);
      ic.relativeType = SubmodelInstanceCountType;
      ic.size[XX] = (int) instanceCount->document()->size().width();
      ic.size[YY] = (int) instanceCount->document()->size().height();
      ic.margin = page->meta.LPub.page.instanceCount.margin;

      if (icPlacementData.relativeTo == PageType) {
        plPage.placeRelative(&ic);
      } else {
        pn.placeRelative(&ic);
      }
      instanceCount->setPos(ic.offset[XX],ic.offset[YY]);
    }
  }
  
  /* Create any graphics items in the insert list */
  
  int nInserts = page->inserts.size();
  
  if (nInserts) {
    QFileInfo fileInfo;    
    for (int i = 0; i < nInserts; i++) {
      InsertData insert = page->inserts[i].value();  
      switch (insert.type) {
        case InsertPicture:
          fileInfo.setFile(insert.picName);
          if (fileInfo.exists()) {
            PlacementPixmap *pixmap = new PlacementPixmap;
            pixmap->pixmap = new QPixmap;
            pixmap->pixmap->load(insert.picName);
            pixmap->size[0] = pixmap->pixmap->width()*insert.picScale;
            pixmap->size[1] = pixmap->pixmap->height()*insert.picScale;
            page->addInsertPixmap(pixmap);
            QGraphicsPixmapItem *gpi = new QGraphicsPixmapItem(*pixmap->pixmap,pageBg);
            gpi->setTransformationMode(Qt::SmoothTransformation);
            gpi->scale(insert.picScale,insert.picScale);
          }
        break;
        case InsertText:
        break;
        case InsertArrow:
        break;
        case InsertBom:
        break;
      }
    }
  }

  if (page->relativeType == SingleStepType) {
    if (page->list.size()) {
      Range *range = dynamic_cast<Range *>(page->list[0]);
      if (range->relativeType == RangeType) {
        Step *step= dynamic_cast<Step *>(range->list[0]);
        if (step && step->relativeType == StepType) {

          step->stepNumber.sizeit();

          step->pli.addPli(step->submodelLevel, pageBg);

          /* Size the callouts */
          for (int i = 0; i < step->list.size(); i++) {
            step->list[i]->sizeIt();
          }
          
          /* Size the inserts : FIXME */

          plPage.relativeTo(step);      // place everything

          step->pli.setPos(step->pli.offset[XX],
                           step->pli.offset[YY]);

          CsiItem *csiItem = NULL;
          if (step->csiPixmap.pixmap) {
            csiItem = new CsiItem(step,
                                 &page->meta, 
                                 *step->csiPixmap.pixmap,
                                  step->submodelLevel,
                                  pageBg, 
                                  page->relativeType);
            csiItem->setPos(step->csiPixmap.offset[XX],
                            step->csiPixmap.offset[YY]);
          } else {
            exit(-1);
          }

          // allocate QGraphicsTextItem for step number

          if (ldrawFile.numSteps(page->bottom.modelName) > 1) {
            StepNumberItem *stepNumber = 
              new StepNumberItem(step,
                                 page->relativeType,
                                &page->meta, 
                                 page->meta.LPub.stepNumber, 
                                 "%d", 
                                 step->stepNumber.number,
                                 pageBg);

            stepNumber->setPos(step->stepNumber.offset[XX],
                               step->stepNumber.offset[YY]);
          }
          
          // foreach callout

          for (int i = 0; i < step->list.size(); i++) {
            Callout *callout = step->list[i];
            QRect    csiRect(step->csiPixmap.offset[XX]-callout->offset[XX],
                             step->csiPixmap.offset[YY]-callout->offset[YY],
                             step->csiPixmap.size[XX], 
                             step->csiPixmap.size[YY]);
                             
            // foreach pointer

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
          
          // foreach insert
          
          for (int i = 0; i < step->inserts.size(); i++) {
            // picture, text, arrows, BOM
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
  } else if (page->relativeType == StepGroupType) {
    PlacementData data = page->meta.LPub.multiStep.placement.value();
    page->placement.setValue(data);
    page->sizeIt();
    plPage.placeRelative(page); // place multi-step relative to the page
    page->relativeToMs(page); // place callouts relative to MULTI_STEP
    plPage.relativeToMs(page);    // place callouts relative to PAGE
    page->addGraphicsItems(0,0,pageBg);
  } else {
  }
  
  scene->addItem(pageBg);
  
  int width = page->meta.LPub.page.size.value(0);
  int height = page->meta.LPub.page.size.value(1);

  view->setSceneRect(0,0,width,height);

  view->horizontalScrollBar()->setRange(0,page->meta.LPub.page.size.value(0));
  view->verticalScrollBar()->setRange(0,page->meta.LPub.page.size.value(1));

  if (fitMode == FitWidth) {
    fitWidth(view);
  } else if (fitMode == FitVisible) {
    fitVisible(view);
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
  page->relativeType = SingleStepType;
  statusBarMsg("");
  return 0;
}
