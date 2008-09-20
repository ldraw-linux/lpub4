
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
 * This class represents one step including a step number, and assembly
 * image, possibly a parts list image and zero or more callouts needed for
 * the step.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef stepH
#define stepH

#include <QGraphicsRectItem>
#include <QString>
#include <QStringList>
#include "range_element.h"
#include "pli.h"
#include "meta.h"
class Meta;
class Callout;
class Range;
enum PlacementType;

class Step : public AbstractRangeElement
{
  public: 
    bool              calledOut;
    QList<Callout *>  list;
    QList<InsertMeta> inserts;
    Pli               pli;
    PlacementPixmap   csiPixmap;
    PlacementNum      stepNumber;
    int               submodelLevel;
    bool              pliPerStep;
    PlacementMeta     placement;

    Step(
      Where                 &topOfStep,
      AbstractStepsElement *_parent,
      int                    num, 
      Meta                  &meta, 
      bool                   calledOut, 
      bool                   multiStep,
      Pli                   &pli);

    virtual ~Step();

    void append(
      Callout *re)
    {
      list.append(re);
    }
    
    Step  *nextStep();
    Range *range();

    int  createCsi(
		       QString const     &addLine,
           QStringList const &csiParts,
           QPixmap          *pixmap,
           Meta             &meta);

    void setInserts(QList<InsertMeta> _inserts) {
      inserts = _inserts;
    }
    
    int  sizeitVert(int  rows[],
                    int  cols[],
                    int  rowsMargin[][2],
                    int  colsMargin[][2]);

    void vertMargin(int &top, int &bot);

    void placeitVert(int rows[],
                     int rowsMargin[][2],
                     int y);

    int  sizeitHoriz(int  rows[],
                     int  cols[],
                     int  rowsMargin[][2],
                     int  colsMargin[][2]);

    void horizMargin(int &top, int &bot);

    void placeitHoriz(int rows[],
                      int rowsMargin[][2],
                      int x);

    void placeInside();

    void sizeitFreeform(
      int xx,
      int yy,
      int relativeBase,
      int relativeJustification,
      int &left,
      int &right);
      
    virtual void addGraphicsItems(int ox, int oy, Meta *, PlacementType, QGraphicsItem *);
};
#endif
