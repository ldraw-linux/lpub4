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
      NumberPlacementMeta &numberMetaIn,
      const char          *formatIn,
      int                  valueIn,
      QGraphicsItem       *parentIn)    {
      page = pageIn;
      QString toolTip("Number of times to build this submodel");
      setAttributes(PageNumberType,
                    SingleStepType,
                    numberMetaIn,
                    formatIn,
                    valueIn,
                    toolTip,
                    parentIn);
    }
  protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
};

void SubmodelInstanceCount::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;

  PlacementData placementData = placement.value();

  QAction *fontAction       = menu.addAction("Change Submodel Count Font");
  QAction *colorAction      = menu.addAction("Change Submodel Count Color");
  QAction *marginAction     = menu.addAction("Change Submodel Count Margins");
  QAction *placementAction  = menu.addAction("Move this number");

  fontAction->setWhatsThis("You can change the font or the size of the page number");
  colorAction->setWhatsThis("You can change the color of the page number");
  marginAction->setWhatsThis("You can change how much empty space their is around the page number");
  placementAction->setWhatsThis("You can move this submodel count around");

  QAction *selectedAction   = menu.exec(event->screenPos());

  if (selectedAction == fontAction) {

    changeFont(page->topOfSteps(),page->bottomOfSteps(),&font);

  } else if (selectedAction == colorAction) {

    changeColor(page->topOfSteps(),page->bottomOfSteps(),&color);

  } else if (selectedAction == marginAction) {

    changeMargins("Submodel Count Margins",
                  page->topOfSteps(),page->bottomOfSteps(),
                  &margin);
  } else if (selectedAction == placementAction) {
  
    changePlacement(PageType,
                    SubmodelInstanceCountType,
                    "Submodel Count Placement",
                    page->topOfSteps(),
                    page->bottomOfSteps(),
                  &placement);
  }
}

void SubmodelInstanceCount::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseReleaseEvent(event);

  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {

    QPointF newPosition;

    // back annotate the movement of the PLI into the LDraw file.
    newPosition = pos() - position;
    
    if (newPosition.x() || newPosition.y()) {
      positionChanged = true;

      PlacementData placementData = placement.value();

      placementData.offsets[0] += newPosition.x()/relativeToWidth;
      placementData.offsets[1] += newPosition.y()/relativeToHeight;

      placement.setValue(placementData);

      changePlacementOffset(page->topOfSteps(),&placement);
    }
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
  plPage.loc[XX] = 0;
  plPage.loc[YY] = 0;

  if (page->meta.LPub.page.dpn.value() && ! coverPage) {

    // allocate QGraphicsTextItem for page number

    PageNumberItem  *pageNumber = 
      new PageNumberItem(
                      page,
                      page->meta.LPub.page.number, 
                     "%d", 
                     stepPageNum,
                     pageBg);
    
    pageNumber->relativeType = PageNumberType;
    pageNumber->size[XX]     = (int) pageNumber->document()->size().width();
    pageNumber->size[YY]     = (int) pageNumber->document()->size().height();
    
    PlacementData placementData = pageNumber->placement.value();
    
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

      pageNumber->placement.setValue(placementData);
    }
    
    plPage.appendRelativeTo(pageNumber);
    plPage.placeRelative(pageNumber);
    pageNumber->setPos(pageNumber->loc[XX],pageNumber->loc[YY]);
    
    // if this page contains the last step of the page, 
    // and instance is > 1 then display instance

    // allocate QGraphicsTextItem for instance number 
    
    SubmodelInstanceCount *instanceCount;
    
    if (endOfSubmodel && instances > 1) {
      instanceCount = new SubmodelInstanceCount(
        page,
        page->meta.LPub.page.instanceCount,
        "x%d",
        instances,
        pageBg);
        
      /*
       * To make mousemove always know how to calculate offset, I modified
       * SubmodelInstanceClass to be derived from Placement.  The relativeToWidth
       * and relativeToHeight for offset calculation are in Placement.
       *
       * The offset calculation works great, but we end up with a problem
       * SubmodelInstanceCount gets placement from NumberPlacementItem, and
       * placement from Placement.  To work around this, I had to hack (and I mean
       * ugly) SubmodelInstanceCount to Placement.
       */
        
      if (instanceCount) {
        instanceCount->size[XX] = (int) instanceCount->document()->size().width();
        instanceCount->size[YY] = (int) instanceCount->document()->size().height();
        instanceCount->loc[XX] = 0;
        instanceCount->loc[YY] = 0;
        instanceCount->tbl[0] = 0;
        instanceCount->tbl[1] = 0;
        
        instanceCount->placement = page->meta.LPub.page.instanceCount.placement;
                        
        PlacementData placementData = instanceCount->placement.value();
        
        if (placementData.relativeTo == PageType) {
          plPage.placeRelative(instanceCount);
        } else {
          pageNumber->placeRelative(instanceCount);
        }
        instanceCount->setPos(instanceCount->loc[XX],instanceCount->loc[YY]);
      }
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
          {
            fileInfo.setFile(insert.picName);
            if (fileInfo.exists()) {

              QPixmap qpixmap;
              qpixmap.load(insert.picName);
              InsertPixmapItem *pixmap = new InsertPixmapItem(qpixmap,page->inserts[i],pageBg);
              
              page->addInsertPixmap(pixmap);
              pixmap->setTransformationMode(Qt::SmoothTransformation);
              pixmap->scale(insert.picScale,insert.picScale);
              
              PlacementData pld;
              
              pld.placement     = insert.placement;
              pld.justification = insert.justification;
              pld.relativeTo    = insert.relativeTo;
              pld.preposition   = insert.preposition;
              pld.offsets[0]    = insert.offsets[0];
              pld.offsets[1]    = insert.offsets[1];
              
              pixmap->placement.setValue(pld);

              plPage.placeRelative(pixmap);
              pixmap->setPos(pixmap->loc[XX],pixmap->loc[YY]);
              pixmap->relativeToWidth  = plPage.size[XX];
              pixmap->relativeToHeight = plPage.size[YY];
            }
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

          step->pli.setPos(step->pli.loc[XX],
                           step->pli.loc[YY]);

          CsiItem *csiItem = NULL;
          if (step->csiPixmap.pixmap) {
            csiItem = new CsiItem(step,
                                 &page->meta, 
                                 *step->csiPixmap.pixmap,
                                  step->submodelLevel,
                                  pageBg,
                                  page->relativeType);
            csiItem->setPos(step->csiPixmap.loc[XX],
                            step->csiPixmap.loc[YY]);
          } else {
            exit(-1);
          }
          
          /* Size the inserts : FIXME */

          // allocate QGraphicsTextItem for step number

          if (ldrawFile.numSteps(page->bottom.modelName) > 1) {
            StepNumberItem *stepNumber = 
              new StepNumberItem(step,
                                 page->relativeType,
                                 page->meta.LPub.stepNumber, 
                                 "%d", 
                                 step->stepNumber.number,
                                 pageBg);

            stepNumber->setPos(step->stepNumber.loc[XX],
                               step->stepNumber.loc[YY]);
            stepNumber->relativeToWidth  = step->stepNumber.relativeToWidth;
            stepNumber->relativeToHeight = step->stepNumber.relativeToHeight;
          }
          
          // foreach callout

          for (int i = 0; i < step->list.size(); i++) {
            Callout *callout = step->list[i];
            QRect    csiRect(step->csiPixmap.loc[XX]-callout->loc[XX],
                             step->csiPixmap.loc[YY]-callout->loc[YY],
                             step->csiPixmap.size[XX], 
                             step->csiPixmap.size[YY]);
                             
            // foreach pointer

            callout->addGraphicsItems(0,0,csiRect,pageBg);
            for (int i = 0; i < callout->pointerList.size(); i++) {
              Pointer *pointer = callout->pointerList[i];
              callout->addGraphicsPointerItem(callout->loc[XX],
                                              callout->loc[YY],
                                              0,
                                              0,
                                              step->csiPixmap.loc,
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
