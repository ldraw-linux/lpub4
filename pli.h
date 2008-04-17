
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
 * This class implements part list images.  It gathers and maintains a list
 * of part/colors that need to be displayed.  It provides mechanisms for
 * rendering the parts.  It provides methods for organizing the parts into
 * a reasonable looking box for display in your building instructions.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef pliH
#define pliH

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QStringList>
#include <QPair>
#include <QString>
#include <QList>
#include <QHash>
#include <QCache>
#include <QTextDocument>
#include "meta.h"
#include "placement.h"
#include "backgrounditem.h"
#include "where.h"
#include "name.h"


/****************************************************************************
 * Part List 
 ***************************************************************************/ 

class InstanceTextItem;
class AnnotateTextItem;
class PGraphicsPixmapItem;
class PliBackgroundItem;

class PliPart;
class Pli;

class PliPart {
  public:
    // where the exist in an LDraw file
    QList<Where>         instances; 
    QString              type;
    QString              color;
    QString              sort;
    NumberMeta           instanceMeta;
    NumberMeta           annotateMeta;
    MarginsMeta          csiMargin;
    InstanceTextItem    *instanceText;
    AnnotateTextItem    *annotateText;
    PGraphicsPixmapItem *pixmap;
    bool                 includeSubmodel;
  
    int           width;
    int           height;

    int           col;

    int           pixmapWidth;
    int           pixmapHeight;
    int           textWidth;
    int           textHeight;
    int           annotHeight;
    int           annotWidth;

    float         topMargin;
    float         partTopMargin;
    float         partBotMargin;

    QList<int>    leftEdge;
    QList<int>    rightEdge;
  
    // placement info
    bool          placed;
    int           left;
    int           bot;

    PliPart()
    {
      placed       = false;
      instanceText = NULL;
      annotateText = NULL;
      pixmap       = NULL;
    }

    PliPart(QString _type, QString _color, bool _includeSubmodel)
    {
      type         = _type;
      color        = _color;
      includeSubmodel = _includeSubmodel;
      placed       = false;
      instanceText = NULL;
      annotateText = NULL;
      pixmap       = NULL;
    }
	float maxMargin();

    virtual ~PliPart();
};

#define INSTANCE_SEP ":"

class Pli : public Placement {
  private:
    static QCache<QString, QString> orientation;

    QHash<QString, PliPart*> parts;
    QList<QString>           sortedKeys;

  public:
    Meta              *meta;
    PlacementType      parentRelativeType;
    PliBackgroundItem *background;
    bool               bom;
    ConstrainMeta      constraint;
    WhereRange         range;

    Pli(bool _bom = false)
    {
      relativeType = PartsListType;
      meta = NULL;
      bom = _bom;
      initAnnotationString();
    }
    ~Pli()
    {
      clear();
    }
      
    void setPos(float x, float y);

    /* Append a new part instance to the parts of parts used so far */

    void append(
      Meta    *meta,
      bool     bom,
      QString &type,
      QString &color,
      Where   &here,
      bool     includeSub);

    int tsize()
    {
      return parts.size();
    }

    void clear();

    void unite(Pli &pli)
    {
      QString key;
      foreach(key,pli.parts.keys()) {
        PliPart *part = pli.parts[key];
        pli.parts.remove(key);
        parts.insert(key,part);
      }
      pli.clear();
    }

	int  placeSort(QList<QString> &);
    int  placePli(QList<QString> &, int,int,bool,bool,int&,int&,int&);
	void placeCols(QList<QString> &);
    bool initAnnotationString();
    void getAnnotate(QString &, QString &);
    void partClass(QString &, QString &);
    int  createPartImage(QString &, QString &, QString &, QPixmap*, Meta *);
    QString orient(QString &color, QString part);

    int  addPli (Meta *, int, QGraphicsItem *);
    int  sizePli(Meta *, PlacementType);

    void operator= (Pli& from)
    {
      QString key;
      foreach(key,from.parts.keys()) {
        PliPart *part = from.parts[key];
        from.parts.remove(key);
        parts.insert(key,part);
      }
      from.parts.clear();
      placement = from.placement;
      margin    = from.margin;
      bom       = from.bom;
      range     = from.range;
    }

    void getLeftEdge(QImage &, QList<int> &);
    void getRightEdge(QImage &, QList<int> &);

    void setTopOfPLI(const Where &here)
    {
      range.setTop(here);
    }
    void setBottomOfPLI(const Where &here)
    {
      range.setBottom(here);
    }
    const Where &topOfPLI()            
    { 
      return range.top();
    }
    const Where &bottomOfPLI()
    {
      return range.bottom();
    }
};

class PliBackgroundItem : public PlacementBackgroundItem
{
public:
  Pli *pli;
  PlacementType  parentRelativeType;
  ConstrainMeta  constraint;
  bool           bom;
  Meta           meta;

  PliBackgroundItem(
    Pli           *_pli,
    int            width,
    int            height,
    Meta          *_meta,
    PlacementType  _parentRelativeType,
    int            submodelLevel, 
    QGraphicsItem *parent)
  {
    meta      = *_meta;
    pli       = _pli;
    placement = _pli->placement;

    parentRelativeType = _parentRelativeType;

    QPixmap *pixmap = new QPixmap(width,height);

    constraint = pli->constraint;
    
    if (_pli->bom) {
      QString toolTip("Bill Of Materials");
      setBackground( pixmap,
                     PartsListType,
                    _parentRelativeType,
                    _meta->LPub.bom.placement,
                    _meta->LPub.bom.background,
                    _meta->LPub.bom.border,
                    _meta->LPub.bom.margin,
                    _meta->LPub.pli.subModelColor,
                    0,
                    toolTip);
    } else {
      QString toolTip("Part List");
      setBackground( pixmap,
                     PartsListType,
                     _parentRelativeType,
                    _meta->LPub.pli.placement,
                    _meta->LPub.pli.background,
                    _meta->LPub.pli.border,
                    _meta->LPub.pli.margin,
                    _meta->LPub.pli.subModelColor,
                    submodelLevel,
                    toolTip);
    }
    placement = meta.LPub.pli.placement;
    setPixmap(*pixmap);
    setParentItem(parent);
  }
  void setPos(float x, float y)
  {
    QGraphicsPixmapItem::setPos(x,y);
  }
  void setFlag(GraphicsItemFlag flag, bool value)
  {
    QGraphicsItem::setFlag(flag,value);
  }
protected:
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
private:
};

class PGraphicsTextItem : public QGraphicsTextItem, public MetaItem
{
public:
  PGraphicsTextItem()
  {
    pli = NULL;
    part = NULL;
  }
  PGraphicsTextItem(
    Pli     *_pli,
    PliPart *_part,
    QString &text, 
    QString &fontString,
    QString &toolTip)
  {
    setText(_pli,
            _part,
            text,
            fontString,
            toolTip);
  }
  void setText(
    Pli     *_pli,
    PliPart *_part,
    QString &text, 
    QString &fontString,
    QString &toolTip)
  {
    pli  = _pli;
    part = _part;
    setPlainText(text);
    QFont font;
    font.fromString(fontString);
    setFont(font);
    setToolTip(toolTip);
  }
  void size(int &x, int &y)
  {
    QSizeF size = document()->size();
    x = int(size.width());
    y = int(size.height());
  }
  PliPart *part;
  Pli     *pli;
  PlacementType  parentRelativeType;
};

class AnnotateTextItem : public PGraphicsTextItem
{
public:
  AnnotateTextItem(
    Pli     *_pli,
    PliPart *_part,
    QString &text, 
    QString &fontString,
    QString &colorString,
    PlacementType _parentRelativeType)
  {
    parentRelativeType = _parentRelativeType;
    QString toolTip("Part Length");
    setText(_pli,_part,text,fontString,toolTip);
    QColor color(colorString);
    setDefaultTextColor(color);
  }

protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};

class InstanceTextItem : public PGraphicsTextItem
{
public:
  InstanceTextItem(
    Pli            *_pli,
    PliPart        *_part,
    QString        &text, 
    QString        &fontString,
    QString        &colorString,
    PlacementType _parentRelativeType)
  {
    parentRelativeType = _parentRelativeType;
    QString toolTip(tr("Number of times this part is used"));
    setText(_pli,_part,text,fontString,toolTip);
    QColor color(colorString);
    setDefaultTextColor(color);
  }

protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};

class PGraphicsPixmapItem : public QGraphicsPixmapItem, public MetaItem
{
public:
  PGraphicsPixmapItem(
    Pli     *_pli,
    PliPart *_part,
    QPixmap &pixmap,
    PlacementType  _parentRelativeType,
    QString &type,
    QString &color)
  {
    parentRelativeType = _parentRelativeType;
    pli = _pli;
    part = _part;
    setPixmap(pixmap);
    setToolTip(pliToolTip(type,color));
  }
  QString pliToolTip(QString type, QString Color);
  PliPart *part;
  Pli     *pli;
  PlacementType  parentRelativeType;

protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};

#endif
