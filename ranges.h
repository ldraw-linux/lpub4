
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
 * This class contains one or more individual range's.
 * By itself, this class represents step groups.  Callouts are derived
 * from ranges.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef rangesH
#define rangesH

#include "pli.h"
#include "meta.h"
#include "placement.h"

/*
 * This is a base class for multi-step and
 * callouts.
 */
class Step;
class AbstractRangesElement;
class Where;

class Ranges : public Placement {
  public:
    Meta           meta;

    QList<AbstractRangesElement *> list;  // of range and/or PLI
    Step          *parent;
    QGraphicsView *view;

    Pli            pli;
    
    Where          bottom;

    Ranges();
    Ranges(Step *_parent,Meta &_meta,QGraphicsView *_view);

    ~Ranges();

    QString modelName();

    QString path();
	  QString csiName();

    QStringList submodelStack();

    void freeRanges();

    void append(AbstractRangesElement *re);

    virtual AllocEnc allocType();

    virtual AllocMeta &allocMeta();
      
    /* size ranges and place each range */

    void sizeIt();
    void addGraphicsItems(int ox, int oy, QGraphicsItem *parent);

    void sizeitVert();
    virtual void addGraphicsItemsVert(int ox, int oy, QGraphicsItem *parent);

    void sizeitHoriz();
    virtual void addGraphicsItemsHoriz(int ox, int oy, QGraphicsItem *parent);

    void sizeitFreeform(int xx, int yy);

    Boundary boundary(AbstractRangesElement *);

    const Where &bottomOfStep(AbstractRangesElement *me);
    const Where &topOfRanges();
    const Where &bottomOfRanges();
    void setBottomOfRanges(const Where &bos);
};
#endif
