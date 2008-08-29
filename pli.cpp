
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
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include "pli.h"
#include "step.h"
#include "ranges.h"
#include "callout.h"
#include "resolution.h"
#include "render.h"
#include "paths.h"
#include "partslist.h"
#include "ldrawfiles.h"
#include "placementdialog.h"
#include "metaitem.h"
#include "color.h"
#include "partslist.h"
#include "lpub.h"
#include "commonmenus.h"
#include "lpub_preferences.h"

QCache<QString,QString> Pli::orientation;
    
const Where &Pli::topOfStep()
{
  return step->topOfStep();
}
const Where &Pli::bottomOfStep()
{
  return step->bottomOfStep();
}
const Where &Pli::topOfSteps()
{
  return steps->topOfSteps();
}
const Where &Pli::bottomOfSteps()
{
  return steps->bottomOfSteps();
}
const Where &Pli::topOfCallout()
{
  return callout->topOfCallout();
}
const Where &Pli::bottomOfCallout()
{
   return callout->bottomOfCallout();
}

/****************************************************************************
 * Part List Images routines
 ***************************************************************************/

PliPart::~PliPart()
{
  instances.clear();
  leftEdge.clear();
  rightEdge.clear();
}

float PliPart::maxMargin()
{
  float margin1 = qMax(instanceMeta.margin.value(XX),
                       csiMargin.value(XX));
  if (annotWidth) {
    margin1 = qMax(margin1,annotateMeta.margin.value(XX));
  }
  return margin1;
}

void Pli::clear()
{
  parts.clear();
}

/* Append a new part instance to the parts of parts used so far */

void Pli::append(
  Meta          *meta,
  bool           bom,
  QString       &type,
  QString       &color,
  Where         &here,
  bool           includeSubs)
{
  QFileInfo info(type);

  QString key = info.baseName() + "_" + color;

  if ( ! parts.contains(key)) {
    PliPart *part = new PliPart(type,color,includeSubs);
    part->annotateMeta = bom ? meta->LPub.bom.annotate : meta->LPub.pli.annotate;
    part->instanceMeta = bom ? meta->LPub.bom.instance : meta->LPub.pli.instance;
    part->csiMargin    = meta->LPub.pli.part.margin;
    parts.insert(key,part);
  }
  
  

  parts[key]->instances.append(here);
}

void Pli::unite(Pli &pli)
{
  QString key;
  foreach(key,pli.parts.keys()) {
    if (parts.contains(key)) {
      parts[key]->instances += pli.parts[key]->instances;
    } else {
      PliPart *part = new PliPart(*pli.parts[key]);
      parts.insert(key,part);
    }
  }
}

QHash<int, QString>     annotationString;
QList<QString>          titles;

bool Pli::initAnnotationString()
{
  if (annotationString.empty()) {
    annotationString[1] = "B";  // blue
    annotationString[2] = "G";  // green
    annotationString[3] = "DC"; // dark cyan
    annotationString[4] = "R";  // red
    annotationString[5] = "M";  // magenta
    annotationString[6] = "Br"; // brown
    annotationString[9] = "LB"; // light blue
    annotationString[10]= "LG"; // light green
    annotationString[11]= "C";  // cyan
    annotationString[12]= "LR"; // cyan
    annotationString[13]= "P";  // pink
    annotationString[14]= "Y";  // yellow
    annotationString[22]= "Ppl";// purple
    annotationString[25]= "O";  // orange
  
    annotationString[32+1] = "TB";  // blue
    annotationString[32+2] = "TG";  // green
    annotationString[32+3] = "TDC"; // dark cyan
    annotationString[32+4] = "TR";  // red
    annotationString[32+5] = "TM";  // magenta
    annotationString[32+6] = "TBr"; // brown
    annotationString[32+9] = "TLB"; // light blue
    annotationString[32+10]= "TLG"; // light green
    annotationString[32+11]= "TC";  // cyan
    annotationString[32+12]= "TLR"; // cyan
    annotationString[32+13]= "TP";  // pink
    annotationString[32+14]= "TY";  // yellow
    annotationString[32+22]= "TPpl";// purple
    annotationString[32+25]= "TO";  // orange
    titles << "^Technic Axle\\s+(\\d+)\\s*$";
    titles << "^Technic Axle Flexible\\s+(\\d+)\\s*$";
    titles << "^Electric Cable NXT\\s+([0-9].*)$";
    titles << "^Electric Cable RCX\\s+([0-9].*)$";
  }
  return true;
}

void Pli::getAnnotate(
  QString &type,
  QString &annotateStr)
{
  annotateStr.clear();
  if (titles.size() == 0) {
    return;
  }

  annotateStr = PartsList::title(type.toLower());

  // pick up LSynth lengths

  QString title;
  for (int i = 0; i < titles.size(); i++) {
    title = titles[i];
    QRegExp rx(title);
    if (annotateStr.contains(rx)) {
      annotateStr = rx.cap(1);
      return;
    }
  }

  annotateStr.clear();
  return;
}

QString Pli::orient(QString &color, QString type)
{
  type = type.toLower();

  float a = 1, b = 0, c = 0;
  float d = 0, e = 1, f = 0;
  float g = 0, h = 0, i = 1;

  QString *cached = orientation[type];

  if ( ! cached) {
    QString name(Preferences::pliFile);
    QFile file(name);
  
    if (file.open(QFile::ReadOnly | QFile::Text)) {      
      QTextStream in(&file);
  
      while ( ! in.atEnd()) {
        QString line = in.readLine(0);
        QStringList tokens;
 
        split(line,tokens);

        if (tokens.size() != 15) {
          continue;
        }

        QString token14 = tokens[14].toLower();

        if (tokens.size() == 15 && tokens[0] == "1" && token14 == type) {
          cached = new QString(line);
          orientation.insert(type,cached);
          break;
        }
      }
      file.close();
    }
  }

  if (cached) {
    QStringList tokens;

    split(*cached, tokens);

    if (tokens.size() == 15 && tokens[0] == "1") {
      a = tokens[5].toFloat();
      b = tokens[6].toFloat();
      c = tokens[7].toFloat();
      d = tokens[8].toFloat();
      e = tokens[9].toFloat();
      f = tokens[10].toFloat();
      g = tokens[11].toFloat();
      h = tokens[12].toFloat();
      i = tokens[13].toFloat();
    }
  }

  return QString ("1 %1 0 0 0 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11")
    .arg(color)
    .arg(a) .arg(b) .arg(c)
    .arg(d) .arg(e) .arg(f)
    .arg(g) .arg(h) .arg(i)
    .arg(type);
}

int Pli::createPartImage(
  QString  &partialKey,
  QString  &type,
  QString  &color,
  QPixmap  *pixmap)
{
  float modelScale = pliMeta.modelScale.value();
  ResolutionType resolutionType = meta->LPub.resolution.type();
  QString        unitsName = resolutionType ? "DPI" : "DPCM";
  float          resolution    = meta->LPub.resolution.value();

  QString key = QString("%1_%2_%3_%4_%5_%6_%7")
                    .arg(partialKey) 
                    .arg(meta->LPub.page.size.value(0)) 
                    .arg(resolution)
                    .arg(resolutionType == DPI ? "DPI" : "DPCM")
                    .arg(modelScale)
                    .arg(pliMeta.angle.value(0))
                    .arg(pliMeta.angle.value(1));
  QString imageName = QDir::currentPath() + "/" +
                      Paths::partsDir + "/" + key + ".png";
  QString ldrName = QDir::currentPath() + "/" + 
                    Paths::tmpDir + "/pli.ldr";
  QFile part(imageName);

  if ( ! part.exists()) {

    // create a temporary DAT to feed to LDGLite
  
    part.setFileName(ldrName);
  
    if ( ! part.open(QIODevice::WriteOnly)) {
      QMessageBox::critical(NULL,QMessageBox::tr(LPUB),
                         QMessageBox::tr("Cannot open file for writing %1:\n%2.")
                         .arg(ldrName)
                         .arg(part.errorString()));
      return -1;
    }
  
    QTextStream out(&part);
    out << orient(color, type);
    part.close();
      
    // feed DAT to LDGLite
  
    int rc = renderer->renderPli(ldrName,imageName,*meta);
  
    if (rc != 0) {
      QMessageBox::warning(NULL,QMessageBox::tr(LPUB),
                         QMessageBox::tr("Render failed for %1 %2\n")
                         .arg(imageName)
                         .arg(Paths::tmpDir+"/part.dat"));
      return -1;
    }
  } 
  pixmap->load(imageName);
  return 0;
}

void Pli::partClass(
  QString &type,
  QString &pclass)
{
  pclass = PartsList::title(type);

  if (pclass.length()) {
    QRegExp rx("^(\\w+)\\s+([a-zA-Z]+).*$");
    if (pclass.contains(rx)) {
      pclass = rx.cap(1);
      if (rx.numCaptures() == 2) {
        pclass += rx.cap(2);
      }
    } else {
      pclass = "ZZZ";
    }
  } else {
    pclass = "ZZZ";
  }
}

int Pli::placePli(
  QList<QString> &keys,
  int    xConstraint,
  int    yConstraint,
  bool   packSubs,
  bool   sortType,
  int   &cols,
  int   &width,
  int   &height)
{
  // Place the first row
  BorderData borderData;
  borderData = pliMeta.border.value();
  int left = 0;
  int nPlaced = 0;
  int tallest = 0;
  float topMargin = borderData.margin[1]+borderData.thickness;
  float botMargin = topMargin;

  cols = 0;

  QString key;

  for (int i = 0; i < keys.size(); i++) {
    parts[keys[i]]->placed = false;

    if (parts[keys[i]]->height > yConstraint) {
      return -2;
    }
  }

  QList< QPair<int, int> > margins;

  while (nPlaced < keys.size()) {

    int i;
    PliPart *part = NULL;

    for (i = 0; i < keys.size(); i++) {
      QString key = keys[i];
      part = parts[key];
      if ( ! part->placed && left + part->width < xConstraint) {
        break;
      }
    }

    if (i == keys.size()) {
      return -1;
    }

    /* Start new col */

    PliPart *prevPart = parts[keys[i]];

    cols++;

    int width = prevPart->width /* + partMarginX */;
    int widest = i;

    prevPart->left = left;
    prevPart->bot  = 0;
    prevPart->placed = true;
    prevPart->col = cols;
    nPlaced++;

    QPair<int, int> margin;

    margin.first = qMax(prevPart->instanceMeta.margin.value(XX),
                        prevPart->csiMargin.value(XX));

    tallest = qMax(tallest,prevPart->height);

    int right = left + prevPart->width;
    int bot = prevPart->height;

    float tmargin = prevPart->instanceMeta.margin.value(YY);
    botMargin = qMax(botMargin,tmargin);

    // allocate new row

    while (nPlaced < parts.size()) {

      int overlap = 0;

      int limit1 = prevPart->height;

      bool overlapped = false;

      // new possible upstairs neighbors

      for (i = 0; i < keys.size() && ! overlapped; i++) {
        PliPart *part = parts[keys[i]];

        if ( ! part->placed) {

          float tmargin = part->instanceMeta.margin.value(YY);
          int splitMargin = qMax(prevPart->topMargin,tmargin);

          bot += splitMargin;

          int limit2 = part->height;

          // dropping part down into prev part (top part is right edge, bottom left)

          for (overlap = 1; overlap < limit1 && ! overlapped; ) {

            if (overlap > limit2) {
              for (int right = 0, left = 0;
                       right < limit2;
                       right++,left++) {
                if (part->rightEdge[right] + splitMargin > 
                    prevPart->leftEdge[left+overlap-limit2]) {
                  overlapped = true;
                  break;
                }
              }
            } else {
              for (int right = limit2 - overlap - 1, left = 0;
                       right < limit2 && left < overlap;
                       right++,left++) {
                if (right >= 0 && part->rightEdge[right] + splitMargin > 
                    prevPart->leftEdge[left]) {
                  overlapped = true;
                  break;
                }
              }
            }
            overlap++;
          }

          // overlap = 0;

          if (bot + part->height + splitMargin - overlap <= yConstraint) {
            break;
          } else {
            overlapped = false;
          }
        }
      }

      if (i == keys.size()) {
        break; // we can't go more Vertical in this column
      }

      PliPart *part = parts[keys[i]];

      margin.first    = qMax(part->instanceMeta.margin.value(XX),
                        part->csiMargin.value(XX));
      float tmargin   = part->instanceMeta.margin.value(YY);
      int splitMargin = qMax(prevPart->topMargin,tmargin);

      prevPart = parts[keys[i]];

      bot -= overlap;

      prevPart->left = left;
      prevPart->bot  = bot;
      prevPart->placed = true;
      prevPart->col = cols;
      nPlaced++;

      if (sortType) {
        if (prevPart->width > width) {
          widest = i;
          width = prevPart->width;
        }
      }

      int height = prevPart->height + splitMargin;

      // try to do sub columns

      if (packSubs && overlap == 0) {
        int subLeft = left + prevPart->width;
        int top = bot + prevPart->height + prevPart->topMargin;

        // allocate new sub_col

        while (nPlaced < keys.size() && i < parts.size()) {

          PliPart *part = parts[keys[i]];
          for (i = 0; i < keys.size(); i++) {
            part = parts[keys[i]];
            if ( ! part->placed) {
              if (subLeft + part->width <= right &&
                  bot + part->height + part->topMargin <= top) {
                break;
              }
            }
          }

          if (i == parts.size()) {
            break;
          }

          int width = part->width;
          part->left = subLeft;
          part->bot  = bot;
          part->placed = true;
          nPlaced++;

          int subBot = bot + part->height + part->topMargin;

          // try to place sub_row

          while (nPlaced < parts.size()) {

            for (i = 0; i < parts.size(); i++) {
              part = parts[keys[i]];
              if ( ! part->placed &&
                  subBot + part->height + splitMargin <= top &&
                  subLeft + part->width <= right) {
                break;
              }
            }

            if (i == parts.size()) {
              break;
            }

            part->left = subLeft;
            part->bot  = subBot;
            part->placed = true;
            nPlaced++;

            subBot += part->height + splitMargin;
          }
          subLeft += width;
        }
      }

      // FIMXE:: try to pack something under bottom of the row.

      bot += height;
      if (bot > tallest) {
        tallest = bot;
      }
    }
    topMargin = qMax(topMargin,part->topMargin);

    left += width;

    part = parts[keys[widest]];
    if (part->annotWidth) {
      margin.second = qMax(part->annotateMeta.margin.value(XX),part->csiMargin.value(XX));
    } else {
      margin.second = part->csiMargin.value(XX);
    }
    margins.append(margin);
  }

  width = left;

  int margin;
  int totalCols = margins.size();
  int lastMargin = 0;
  for (int col = 0; col < totalCols; col++) {
    lastMargin = margins[col].second;
    if (col == 0) {
      int bmargin = borderData.thickness + borderData.margin[0];
      margin = qMax(bmargin,margins[col].first);
    } else {
      margin = qMax(margins[col].first,margins[col].second);
    }
    for (int i = 0; i < parts.size(); i++) {
      if (parts[keys[i]]->col >= col+1) {
        parts[keys[i]]->left += margin;
      }
    }
    width += margin;
  }
  if (lastMargin < borderData.margin[0]+borderData.thickness) {
    lastMargin = borderData.margin[0]+borderData.thickness;
  }
  width += lastMargin;

  height = tallest;

  for (int i = 0; i < parts.size(); i++) {
    parts[keys[i]]->bot += botMargin;
  }

  height += botMargin + topMargin;

  return 0;
}

void Pli::placeCols(
  QList<QString> &keys)
{
  QList< QPair<int, int> > margins;

  // Place the first row
  BorderData borderData;
  borderData = pliMeta.border.value();

  float topMargin = qMax(borderData.margin[1]+borderData.thickness,parts[keys[0]]->topMargin);
  float botMargin = qMax(borderData.margin[1]+borderData.thickness,parts[keys[0]]->instanceMeta.margin.value(YY));

  int height = 0;
  int width;

  PliPart *part = parts[keys[0]];

  float borderMargin = borderData.thickness + borderData.margin[XX];

  width = qMax(borderMargin, part->maxMargin()); 

  for (int i = 0; i < keys.size(); i++) {
    part = parts[keys[i]];
    part->left = width;
    part->bot  = botMargin;
	  part->col = i;

	  width += part->width;

	  if (part->height > height) {
        height = part->height;
	  }

	  if (i < keys.size() - 1) {
	    PliPart *nextPart = parts[keys[i+1]];
      width += qMax(part->maxMargin(),nextPart->maxMargin());
	  }
  }
  part = parts[keys[keys.size()-1]];
  width += qMax(part->maxMargin(),borderMargin);
  
  Placement::size[0] = width;
  Placement::size[1] = topMargin + height + botMargin;
}

void Pli::getLeftEdge(
  QImage     &image,
  QList<int> &edge)
{
  QImage alpha = image.alphaChannel();

  for (int y = 0; y < alpha.height(); y++) {
    int x;
    for (x = 0; x < alpha.width(); x++) {
      QColor c = alpha.pixel(x,y);
      if (c.blue()) {
        edge << x;
        break;
      }
    }
    if (x == alpha.width()) {
      edge << x - 1;
    }
  }
}

void Pli::getRightEdge(
  QImage     &image,
  QList<int> &edge)
{
  QImage alpha = image.alphaChannel();

  for (int y = 0; y < alpha.height(); y++) {
    int x;
    for (x = alpha.width() - 1; x >= 0; x--) {
      QColor c = alpha.pixel(x,y);
      if (c.blue()) {
        edge << x;
        break;
      }
    }
    if (x < 0) {
      edge << 0;
    }
  }
}

int Pli::sizePli(Meta *_meta, PlacementType _parentRelativeType)
{
  meta = _meta;
  switch (_parentRelativeType) {
    case StepGroupType:
      placement = meta->LPub.multiStep.pli.placement;
    break;
    case CalloutType:
      placement = meta->LPub.callout.pli.placement;
    break;
    default:
      placement = meta->LPub.pli.placement;
    break;
  }
  parentRelativeType = _parentRelativeType;

  margin.setValueUnits(0,0);

  if (bom) {
    pliMeta = meta->LPub.bom;
    
  } else {
    pliMeta = meta->LPub.pli;
  }

  QString key;

  foreach(key,parts.keys()) {
    PliPart *part;

    part = parts[key];

    if (PartsList::isKnownPart(part->type) || 
        part->includeSubmodel && !bom &&
        gui->ldrawFileContains(part->type)) {

      if (part->color == "16") {
        part->color = "0";
      }

      // Part Pixmap

      QFileInfo info(part->type);

      QString imageName = Paths::partsDir + "/" + key + ".png";

      QPixmap *pixmap = new QPixmap();

      if (createPartImage(key,part->type,part->color,pixmap)) {
        QMessageBox::warning(NULL,QMessageBox::tr("LPub"),
        QMessageBox::tr("Failed to load %1")
        .arg(imageName));
        return -1;
      }

      QImage image = pixmap->toImage();

      part->pixmap = new PGraphicsPixmapItem(this,part,*pixmap,parentRelativeType,part->type, part->color);

      delete pixmap;

      part->pixmapWidth  = image.width(); 
      part->pixmapHeight = image.height();
     
      part->width  = image.width();

      /* Add instance count area */

      QString descr;

      descr = QString("%1x") .arg(part->instances.size(),0,10);
      
      QString font = pliMeta.instance.font.value();
      QString color = pliMeta.instance.color.value();
      
      part->instanceText = 
        new InstanceTextItem(this,part,descr,font,color,parentRelativeType);
        
      part->instanceText->size(part->textWidth,part->textHeight);

      // if text width greater than image width
      // the bounding box is wider

      if (part->textWidth > part->width) {
        part->width = part->textWidth;
      }

      /* Add annotation area */

      getAnnotate(part->type,descr);

      if (descr.size()) {

        font = pliMeta.annotate.font.value();
        color = pliMeta.annotate.color.value();
         part->annotateText = 
          new AnnotateTextItem(this,part,descr,font,color,parentRelativeType);

        part->annotateText->size(part->annotWidth,part->annotHeight);

        if (part->annotWidth > part->width) {
          part->width = part->annotWidth;
        }

        if (part->annotateMeta.margin.value(YY) > part->csiMargin.value(YY)) {
          part->partTopMargin = part->annotateMeta.margin.value(YY);
        } else {
          part->partTopMargin = part->csiMargin.value(YY);
        }

        for (int h = 0; h < part->annotHeight + part->partTopMargin; h++) {
          part->leftEdge  << part->width - part->annotWidth;
          part->rightEdge << part->width;
        }

        part->topMargin = part->annotateMeta.margin.value(YY);
      } else {
        part->annotateText = NULL;
        part->annotWidth  = 0;
        part->annotHeight = 0;
        part->partTopMargin = 0;
        part->topMargin = part->csiMargin.value(YY);
      }
      getLeftEdge(image,part->leftEdge);
      getRightEdge(image,part->rightEdge);

      if (part->instanceMeta.margin.value(YY) > part->csiMargin.value(YY)) {
        part->partBotMargin = part->instanceMeta.margin.value(YY);
      } else {
        part->partBotMargin = part->csiMargin.value(YY);
      }

      for (int h = 0; h < part->textHeight + part->partBotMargin; h++) {
        part->leftEdge << 0;
        part->rightEdge << part->textWidth;
      }

      part->height = part->leftEdge.size();

      if (bom && pliMeta.sort.value()) {
        QString pclass;
        partClass(part->type,pclass);
        part->sort = QString("%1%2%3%%4")
                     .arg(pclass,80,' ')
                     .arg(part->width, 8,10,QChar('0'))
                     .arg(part->height,8,10,QChar('0'))
                     .arg(part->color);
      } else {
        part->sort = QString("%1%2%3%4")
                     .arg(part->width, 8,10,QChar('0'))
                     .arg(part->height,8,10,QChar('0'))
                     .arg(part->type)
                     .arg(part->color);
      }
    } else {
      delete parts[key];
      parts.remove(key);
    }
  }

  if (parts.size() == 0) {
    return 1;
  }

  sortedKeys = parts.keys();

  // We got all the sizes for parts for a given step so sort

  for (int i = 0; i < parts.size() - 1; i++) {
    for (int j = i+1; j < parts.size(); j++) {
      if (parts[sortedKeys[i]]->sort < parts[sortedKeys[j]]->sort) {
        QString t = sortedKeys[i];
        sortedKeys[i] = sortedKeys[j];
        sortedKeys[j] = t;
      }
    }
  }

  // Fill the part list image using constraint
  //   Constrain Height
  //   Constrain Width
  //   Constrain Columns
  //   Constrain Area
  //   Constrain Square

  int cols, height;
  bool packSubs = pliMeta.pack.value();
  bool sortType = pliMeta.sort.value();
  int pliWidth,pliHeight;

  ConstrainData constrainData = pliMeta.constrain.value();

  if (constrainData.type == PliConstrainHeight) {
    int cols;
    int rc;
    rc = placePli(sortedKeys,
                  10000000,
                  constrainData.constraint,
                  packSubs,
                  sortType,
                  cols,
                  pliWidth,
                  pliHeight);
    if (rc == -2) {
      Where here;
      here = pliMeta.constrain.here();
      gui->parseError("Error: Packing PLI failed. Part taller than constraint",here);
      clear();
      return -1;
    }
  } else if (constrainData.type == PliConstrainColumns) {
	  if (parts.size() <= constrainData.constraint) {
	    placeCols(sortedKeys);
	    pliWidth = Placement::size[0];
	    pliHeight = Placement::size[1];
	    cols = parts.size();
	  } else { 
      int bomCols = constrainData.constraint;

      int maxHeight = 0;
      for (int i = 0; i < parts.size(); i++) {
        maxHeight += parts[sortedKeys[i]]->height + parts[sortedKeys[i]]->csiMargin.value(1);
      }

      maxHeight += maxHeight;

      if (bomCols) {
        for (height = maxHeight/(4*bomCols); height <= maxHeight; height++) {
          int rc = placePli(sortedKeys,10000000,
                            height,
                            packSubs,
                            sortType,
                            cols,
                            pliWidth,
                            pliHeight);
          if (rc == 0 && cols == bomCols) {
            break;
          }
		    }
      }
    }
  } else if (constrainData.type == PliConstrainWidth) {

    int height = 0;
    for (int i = 0; i < parts.size(); i++) {
      height += parts[sortedKeys[i]]->height;
    }

    int cols;
    int good_height = height;

    for ( ; height > 0; height--) {

      int rc = placePli(sortedKeys,10000000,
                        height,
                        packSubs,
                        sortType,
                        cols,
                        pliWidth,
                        pliHeight);
      if (rc) {
        break;
      }

      int w = 0;

      for (int i = 0; i < parts.size(); i++) {
        int t;
        t = parts[sortedKeys[i]]->left + parts[sortedKeys[i]]->width;
        if (t > w) {
          w = t;
        }
      }
      if (w < constrainData.constraint) {
        good_height = height;
      }
    }
    placePli(sortedKeys,10000000,
             good_height,
             packSubs,
             sortType,
             cols,
             pliWidth,
             pliHeight);
  } else if (constrainData.type == PliConstrainArea) {

    int height = 0;
    for (int i = 0; i < parts.size(); i++) {
      height += parts[sortedKeys[i]]->height;
    }

    int cols;
    int min_area = height*height;
    int good_height = height;

    // step by 1/10 of inch or centimeter

    int step = toPixels(0.1,DPI);

    for ( ; height > 0; height -= step) {

      int rc = placePli(sortedKeys,10000000,
                        height,
                        packSubs,
                        sortType,
                        cols,
                        pliWidth,
                        pliHeight);

      if (rc) {
        break;
      }

      int h = 0;
      int w = 0;

      for (int i = 0; i < parts.size(); i++) {
        int t;
        t = parts[sortedKeys[i]]->bot + parts[sortedKeys[i]]->height;
        if (t > h) {
          h = t;
        }
        t = parts[sortedKeys[i]]->left + parts[sortedKeys[i]]->width;
        if (t > w) {
          w = t;
        }
      }
      if (w*h < min_area) {
        min_area = w*h;
        good_height = height;
      }
    }
    placePli(sortedKeys,10000000,
             good_height,
             packSubs,
             sortType,
             cols,
             pliWidth,
             pliHeight);
  } else if (constrainData.type == PliConstrainSquare) {

    int height = 0;
    for (int i = 0; i < parts.size(); i++) {
      height += parts[sortedKeys[i]]->height;
    }

    int cols;
    int min_delta = height;
    int good_height = height;
    int step = toPixels(0.1,DPI);

    for ( ; height > 0; height -= step) {

      int rc = placePli(sortedKeys,10000000,
                        height,
                        packSubs,
                        sortType,
                        cols,
                        pliWidth,
                        pliHeight);

      if (rc) {
        break;
      }



      int h = pliWidth;
      int w = pliHeight;

      int delta = 0;
      if (w < h) {
        delta = h - w;
      } else if (h < w) {
        delta = w - h;
      }
      if (delta < min_delta) {
        min_delta = delta;
        good_height = height;
      }
    }
    placePli(sortedKeys,10000000,
             good_height,
             packSubs,
             sortType,
             cols,
             pliWidth,
             pliHeight);
  }

  Placement::size[0] = pliWidth;
  Placement::size[1] = pliHeight;

  return 0;
}

int Pli::addPli(
  int       submodelLevel,
  QGraphicsItem *parent)
{
  int width = Placement::size[0];
  int height = Placement::size[1];

  background = 
    new PliBackgroundItem(
          this,
          width,
          height,
          parentRelativeType,
          submodelLevel,
          parent);
          
  if ( ! background) {
    return -1;
  }

  QString key;

  foreach (key, sortedKeys) {
    PliPart *part = parts[key];
    if (part == NULL) {
      continue;
    }
    float x,y;
    float center = (part->width - part->pixmapWidth)/2;

    x = part->left;
    y = height - part->bot;

    if (part->annotateText) {
      part->annotateText->setParentItem(background);
      part->annotateText->setPos(
        x + part->width - part->annotWidth,
        y - part->height /*+ part->annotHeight*/);
    }

    part->pixmap->setParentItem(background);
    part->pixmap->setPos(
      x + center,
      y - part->height + part->annotHeight + part->partTopMargin);
    part->pixmap->setTransformationMode(Qt::SmoothTransformation);

    part->instanceText->setParentItem(background);
    part->instanceText->setPos(
      x,
      y - part->textHeight);
  }
  size[0] = width;
  size[1] = height;

  return 0;
}

void Pli::setPos(float x, float y)
{
  background->setPos(x,y);
}

QString PGraphicsPixmapItem::pliToolTip(
  QString type,
  QString color)
{
  QString title = PartsList::title(type);
  if (title == "") {
    Where here(type,0);
    title = gui->readLine(here);
    title = title.right(title.length() - 2);
  }
  QString toolTip;

  toolTip = LDrawColor::name(color) + " " + type + " \"" + title + "\"";
  return toolTip;
}

PliBackgroundItem::PliBackgroundItem(
  Pli           *_pli,
  int            width,
  int            height,
  PlacementType  _parentRelativeType,
  int            submodelLevel, 
  QGraphicsItem *parent)
{
  pli       = _pli;
  placement = _pli->placement;

  parentRelativeType = _parentRelativeType;

  QPixmap *pixmap = new QPixmap(width,height);
    
  QString toolTip;

  if (_pli->bom) {
    toolTip = "Bill Of Materials";
  } else {
    toolTip = "Part List";
  }

  placement = pli->pliMeta.placement;

  setBackground( pixmap,
                 PartsListType,
                 _parentRelativeType,
                 pli->placement,
                 pli->pliMeta.background,
                 pli->pliMeta.border,
                 pli->pliMeta.margin,
                 pli->pliMeta.subModelColor,
                 submodelLevel,
                 toolTip);

  setPixmap(*pixmap);
  setParentItem(parent);
  if (parentRelativeType != SingleStepType) {
    setFlag(QGraphicsItem::ItemIsMovable,false);
  }
}

void PliBackgroundItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseReleaseEvent(event);

  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {

    QPointF newPosition;

    // back annotate the movement of the PLI into the LDraw file.
    newPosition = pos() - position;
    if (newPosition.x() || newPosition.y()) {
      positionChanged = true;
      PlacementData placementData = placement.value();
      placementData.offsets[0] += newPosition.x()/pli->meta->LPub.page.size.value(0);
      placementData.offsets[1] += newPosition.y()/pli->meta->LPub.page.size.value(1);
      placement.setValue(placementData);

      changePlacementOffset(pli->topOfStep(),&placement);
    }
  }
}

void PliBackgroundItem::contextMenuEvent(
  QGraphicsSceneContextMenuEvent *event)
{
  if (pli) {
    QMenu menu;
      
    QAction *constrainAction  = menu.addAction("Change Shape");
    constrainAction->setWhatsThis(             "Change Shape:\n"
      "  You can change the shape of this parts list.  One way, is\n"
      "  is to ask the computer to make the parts list as small as\n"
      "  possible (area). Another way is to ask the computer to\n"
      "  make it as close to square as possible.  You can also pick\n"
      "  how wide you want it, and the computer will make it as\n"
      "  tall as is needed.  Another way is to pick how tall you\n"
      "  and it, and the computer will make it as wide as it needs.\n"
      "  The last way is to tell the computer how many columns it\n"
      "  can have, and then it will try to make all the columns the\n"
                                               "  same height\n");
                                               
    PlacementData placementData = pli->placement.value();

    QString name = "Move Parts List";
    QAction *placementAction  = menu.addAction(name);
    placementAction->setWhatsThis(
      commonMenus.naturalLanguagePlacementWhatsThis(PartsListType,placementData,name));

    QString pl = "Parts List ";
    QAction *backgroundAction = commonMenus.backgroundMenu(menu,pl);
    QAction *borderAction     = commonMenus.borderMenu(menu,pl);
    QAction *marginAction     = commonMenus.marginMenu(menu,pl);
     
    QAction *selectedAction   = menu.exec(event->screenPos());

    if (selectedAction == NULL) {
      return;
    }
  
    Where top;
    Where bottom;
    bool  local;
      
    Where topOfStep = pli->topOfStep();
    Where bottomOfStep = pli->bottomOfStep();
  
    switch (parentRelativeType) {
      case StepGroupType:
        top    = pli->topOfSteps();
        MetaItem mi;
        mi.scanForward(top,StepGroupMask);
        bottom = pli->bottomOfSteps();
        local = false;
      break;
      case CalloutType:
        top    = pli->topOfCallout();
        bottom = pli->bottomOfCallout();
        local = false;
      break;
      default:
        top    = topOfStep;
        bottom = bottomOfStep;
        local = true;
      break;
    }

    QString me = pli->bom ? "BOM" : "PLI";
    if (selectedAction == constrainAction) {
      changeConstraint(me+" Constraint",
                       topOfStep,
                       bottomOfStep,
                       &pli->pliMeta.constrain);
    } else if (selectedAction == placementAction) {
      changePlacement(parentRelativeType,
                      PartsListType,
                      me+" Placement",
                      topOfStep,
                      bottomOfStep,
                    &pli->placement);
    } else if (selectedAction == marginAction) {
      changeMargins(me+" Margins",
                    topOfStep,
                    bottomOfStep,
                    &pli->pliMeta.margin);
    } else if (selectedAction == backgroundAction) {
      changeBackground(me+" Background",
                       top,
                       bottom,
                       &pli->pliMeta.background);
    } else if (selectedAction == borderAction) {
      changeBorder(me+" Border",
                   top,
                   bottom,
                   &pli->pliMeta.border);
  	}
  }
}

void AnnotateTextItem::contextMenuEvent(
  QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
     
  QString pl = "Part Length ";

  QAction *fontAction   = commonMenus.fontMenu(menu,pl);
  QAction *colorAction  = commonMenus.colorMenu(menu,pl);
  QAction *marginAction = commonMenus.marginMenu(menu,pl);

  QAction *selectedAction   = menu.exec(event->screenPos());

  if (selectedAction == NULL) {
    return;
  }
  
  Where top;
  Where bottom;
  
  switch (parentRelativeType) {
    case StepGroupType:
      top    = pli->topOfSteps();
      MetaItem mi;
      mi.scanForward(top,StepGroupMask);
      bottom = pli->bottomOfSteps();
    break;
    case CalloutType:
      top    = pli->topOfCallout();
      bottom = pli->bottomOfCallout();
    break;
    default:
      top    = pli->topOfStep();
      bottom = pli->bottomOfStep();
    break;
  }

  if (selectedAction == fontAction) {
    changeFont(top,bottom,&pli->pliMeta.annotate.font);


  } else if (selectedAction == colorAction) {
    changeColor(top,bottom,&pli->pliMeta.annotate.color);
  } else if (selectedAction == marginAction) {
    changeMargins("Part Length Margins",
                  top,
                  bottom,
                  &pli->pliMeta.annotate.margin);
  }
}

void InstanceTextItem::contextMenuEvent(
  QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
     
  QString pl = "Parts Count ";

  QAction *fontAction   = commonMenus.fontMenu(menu,pl);
  QAction *colorAction  = commonMenus.colorMenu(menu,pl);
  QAction *marginAction = commonMenus.marginMenu(menu,pl);

  QAction *selectedAction   = menu.exec(event->screenPos());

  if (selectedAction == NULL) {
    return;
  }
  
  Where top;
  Where bottom;
  
  switch (parentRelativeType) {
    case StepGroupType:
      top    = pli->topOfSteps() + 1;
      MetaItem mi;
      mi.scanForward(top,StepGroupMask);
      bottom = pli->bottomOfSteps();
    break;
    case CalloutType:
      top    = pli->topOfCallout();
      bottom = pli->bottomOfCallout();
    break;
    default:
      top    = pli->topOfStep();
      bottom = pli->bottomOfStep();
    break;
  }

  if (selectedAction == fontAction) {
    changeFont(top,bottom,&pli->pliMeta.instance.font,1,false);
  } else if (selectedAction == colorAction) {
    changeColor(top,bottom,&pli->pliMeta.instance.color,1,false);
  } else if (selectedAction == marginAction) {
    changeMargins("Margins",top,bottom,&pli->pliMeta.instance.margin,1,false);
  }
}

void PGraphicsPixmapItem::contextMenuEvent(
  QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
  QString part = "Part ";
  QAction *marginAction = commonMenus.marginMenu(menu,part);

#if 0
  QAction *scaleAction  = commonMenus.scaleMenu(menu,part);

  QAction *orientationAction= menu.addAction("Change Part Orientation");
  orientationAction->setDisabled(true);
  orientationAction->setWhatsThis("This doesn't work right now");
#endif

  QAction *selectedAction   = menu.exec(event->screenPos());

  if (selectedAction == NULL) {
    return;
  }

  if (selectedAction == marginAction) {
    changeMargins("Parts List Part Margins",
                  pli->topOfStep(),
                  pli->bottomOfStep(),
                  &pli->pliMeta.part.margin);
#if 0
  } else if (selectedAction == scaleAction) {
    changeFloatSpinTop(
      "Parts List",
      "Model Size",
      pli->topOfStep(),
      pli->bottomOfStep(),
      &meta->LPub.pli.modelScale,0);
    gui->clearPLICache();
#endif
  }
}
