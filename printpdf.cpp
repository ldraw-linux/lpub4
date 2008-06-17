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
#include <QPainter>
#include <QPrinter>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QString>

#include "lpub.h"

struct PageSizes {
  QPrinter::PageSize pageSize;
  float              width;
  float              height;
} pageSizes[] = { 
  { QPrinter::A0,         841, 1189 },
  { QPrinter::A1,         594,  841 },
  { QPrinter::A2,         420,  594 },
  { QPrinter::A3,         297,  420 },
  { QPrinter::A4,         210,  297 },
  { QPrinter::A5,         148,  210 },
  { QPrinter::A6,         105,  148 },
  { QPrinter::A7,          74,  105 },
  { QPrinter::A8,          52,   74 },
  { QPrinter::A9,          37,   52 },
  { QPrinter::B0,        1030, 1456 },
  { QPrinter::B1,         728, 1030 },
  { QPrinter::B10,         32,   45 },
  { QPrinter::B2,         515,  728 },
  { QPrinter::B3,         364,  515 },
  { QPrinter::B4,         257,  364 },
  { QPrinter::B5,         182,  257 },
  { QPrinter::B6,         128,  182 },
  { QPrinter::B7,          91,  128 },
  { QPrinter::B8,          64,   91 },
  { QPrinter::B9,          45,   64 },
  { QPrinter::C5E,        163,  229 },
  { QPrinter::Comm10E,    105,  241 },
  { QPrinter::DLE,        110,  220 },
  { QPrinter::Executive,  191,  254 },
  { QPrinter::Folio,      210,  330 },
  { QPrinter::Legal,      216,  356 },
  { QPrinter::Letter,     216,  279 },
  { QPrinter::Tabloid,    279,  432 },
};

void Gui::bestPageSizeOrientation(
  float widthMm,
  float heightMm,
  QPrinter::PageSize &pageSize,
  QPrinter::Orientation &orient)
{
  int   numPageSizes = sizeof(pageSizes)/sizeof(pageSizes[0]);
  float diffWidth = 0;
  float diffHeight = 0;
  bool  fit = false;
  
  for (int i = 0; i < numPageSizes; i++) {
 
  float widthDiff = pageSizes[i].width - widthMm;
	float heightDiff = pageSizes[i].height - heightMm;
	
    if (widthDiff >= 0  && heightDiff >= 0) {
	    if (fit) {
		    if (widthDiff < diffWidth && heightDiff < diffHeight) {
		      pageSize = pageSizes[i].pageSize;
		      orient = QPrinter::Portrait;
		      diffWidth = widthDiff;
		      diffHeight = heightDiff;
	    	}
	    } else {
	      fit = true;
		    pageSize = pageSizes[i].pageSize;
		    orient = QPrinter::Portrait;
		    diffWidth  = widthDiff;
		    diffHeight = heightDiff;
	    }
	  }
 
    widthDiff = pageSizes[i].height - widthMm;
	  heightDiff  = pageSizes[i].width - heightMm;
	
    if (widthDiff >= 0  && heightDiff >= 0) {
	    if (fit) {
		  if (widthDiff < diffWidth && heightDiff < diffHeight) {
		    pageSize = pageSizes[i].pageSize;
		    orient = QPrinter::Landscape;
		    diffWidth = widthDiff;
		    diffHeight = heightDiff;
		  }
	  } else {
	    fit = true;
		  pageSize = pageSizes[i].pageSize;
		  orient = QPrinter::Portrait;
		  diffWidth  = widthDiff;
		  diffHeight = heightDiff;
	    }
	  }
  }
}

void Gui::printToFile()
{
  QPrinter printer(QPrinter::HighResolution);
  float pageWidth = page.meta.LPub.page.size.valueUnit(0);
  float pageHeight = page.meta.LPub.page.size.valueUnit(1);
  if (page.meta.LPub.resolution.type() == DPI) {
    // convert to MM
	  pageWidth = int(inches2centimeters(pageWidth)*10);
	  pageHeight = int(inches2centimeters(pageHeight)*10);
  }
  QPrinter::PageSize pageSize;
  QPrinter::Orientation orientation;
  bestPageSizeOrientation(pageWidth,pageHeight,pageSize,orientation);
	
  printer.setOrientation(orientation);
  printer.setPageSize(pageSize);
 
  printer.setFullPage(true);

  QFileInfo fileInfo(curFile);
  QString   baseName = fileInfo.baseName();

  QString fileName = QFileDialog::getSaveFileName(
    this,
	  tr("Print File Name"),
	  QDir::currentPath() + "/" + baseName,
	  tr("PDF (*.pdf)\nPDF (*.PDF)"));

  fileInfo.setFile(fileName);
  QString suffix = fileInfo.suffix();
  if (suffix == "") {
    fileName += ".pdf";
  } else if (suffix != ".pdf" && suffix != ".PDF") {
    fileName = fileInfo.baseName() + ".pdf";
  }
  
  printer.setOutputFileName(fileName);

  QPainter painter;
  painter.begin(&printer);

  int savePageNumber = displayPageNum;
  
  QGraphicsScene scene;
  LGraphicsView  view(&scene);
  QRectF boundingRect(0.0,0.0,page.meta.LPub.page.size.value(0),page.meta.LPub.page.size.value(1));
  QRect  bounding(0,0,page.meta.LPub.page.size.value(0),page.meta.LPub.page.size.value(1));
  view.setMinimumSize(page.meta.LPub.page.size.value(0),page.meta.LPub.page.size.value(1));
  view.setMaximumSize(page.meta.LPub.page.size.value(0),page.meta.LPub.page.size.value(1));
  view.setGeometry(bounding);
  view.setSceneRect(boundingRect);
  view.setRenderHints(QPainter::Antialiasing | 
					  QPainter::TextAntialiasing |
					  QPainter::SmoothPixmapTransform);

  view.scale(3.0,3.0);
  view.centerOn(boundingRect.center());
  clearPage(&view,&scene);
  
  for (displayPageNum = 1; displayPageNum <= maxPages; displayPageNum++) {

    qApp->processEvents();
	
    drawPage(&view,&scene);
    view.render(&painter);
    clearPage(&view,&scene);

    if (maxPages - displayPageNum > 0) {
      printer.newPage();
    }
  }
  painter.end();
  displayPageNum = savePageNumber;
  drawPage(KpageView,KpageScene);
}
