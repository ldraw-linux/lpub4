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

#include <QSizePolicy>
#include <QPainter>
#include <QPrinter>
#include <QFileDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QCloseEvent>

#include "lpub.h"
#include "editwindow.h"
#include "commands.h"
#include "name.h"
#include "paths.h"
#include "globals.h"
#include "render.h"

Gui *gui;

LDGLite ldglite;
LDView  ldview;

void clearPliCache()
{
  gui->clearPLICache();
}

void clearCsiCache()
{
  gui->clearCSICache();
}

/****************************************************************************
 * 
 * The Gui constructor and destructor are at the bottom of the file with
 * the code that creates the basic GUI framekwork 
 *
 ***************************************************************************/

void Gui::open()
{
  if (maybeSave()) {
    QString fileName = QFileDialog::getOpenFileName(
      this,
      tr("Open LDraw File"),
      Paths::ldrawPath + "\\MODELS",
      tr("LDraw Files (*.DAT *.LDR *.MPD *.dat *.ldr *.mpd"));

    if (!fileName.isEmpty()) {
        openFile(fileName);
        displayPage();
        return;
      }
    }
  return;
}

void Gui::save()
{
  if (isMpd()) {
    watcher.removePath(curFile);
  } else {
    QStringList list = ldrawFile.subFileOrder();
    QString foo;
    foreach (foo,list) {
      QString bar = QDir::currentPath() + "/" + foo;
      watcher.removePath(bar);
    }
  }
  if (curFile.isEmpty()) {
    saveAs();
  } else {
    saveFile(curFile);
  }

  if (isMpd()) {
    watcher.addPath(curFile);
  } else {
    QStringList list = ldrawFile.subFileOrder();
    QString foo;
    foreach (foo,list) {
      QString bar = QDir::currentPath() + "/" + foo;
      watcher.addPath(bar);
    }
  }
}

void Gui::saveAs()
{
  if (curFile != "") {
    if (isMpd()) {
      watcher.removePath(curFile);
    } else {
      QStringList list = ldrawFile.subFileOrder();
      QString foo; 
      foreach (foo,list) {
        QString bar = QDir::currentPath() + "/" + foo;
        watcher.removePath(bar);
      }
    }
  }
  QString fileName = QFileDialog::getSaveFileName(this);
  if (fileName.isEmpty()) {
    return;
  }
  saveFile(fileName); 
  if (curFile != "") {
    if (isMpd()) {
      watcher.addPath(curFile);
    } else {
      QStringList list = ldrawFile.subFileOrder();
      QString foo;
      foreach (foo,list) {
        QString bar = QDir::currentPath() + "/" + foo;
        watcher.addPath(bar);
      }
    }
  }
} 

bool Gui::maybeSave()
{
  if ( ! undoStack->isClean() ) {
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr(LPUB),
            tr("The document has been modified.\n"
                "Do you want to save your changes?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save) {
      save();
      return true;
    } else if (ret == QMessageBox::Cancel) {
      return false;
    }
  }
  return true;
}

bool Gui::saveFile(const QString &fileName)
{
  bool rc;
  rc = ldrawFile.saveFile(fileName);
  setCurrentFile(fileName);
  undoStack->setClean();
  if (rc) {
    statusBar()->showMessage(tr("File saved"), 2000);
  }
  return rc;
}

void Gui::closeFile()
{
  ldrawFile.empty();
  undoStack->clear();
  editWindow->textEdit()->document()->clear();
  editWindow->textEdit()->document()->setModified(false);
  mpdCombo->setMaxCount(0);
  mpdCombo->setMaxCount(1000);
}

void Gui::openRecentFile()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    QString fileName = action->data().toString();
    clearPage();
    QFileInfo info(fileName);
    QDir::setCurrent(info.absolutePath());
    openFile(fileName);
    setCurrentFile(fileName);
    Paths::mkdirs();
    displayPage();
  }
}
   
void Gui::displayPage()
{
  if (macroNesting == 0) {
    clearPage();
    drawPage();
  }
}

void Gui::nextPage()
{
  countPages();
  if (displayPageNum < maxPages) {
    ++displayPageNum;
    displayPage();
  } else {
    statusBarMsg("You're on the last page");
  }
}

void Gui::prevPage()
{
  if (displayPageNum > 1) {
    displayPageNum--;
    displayPage();
  }
}

void Gui::firstPage()
{
  displayPageNum = 1;
  displayPage();
}

void Gui::redrawPage()
{
  displayPage();
}

void Gui::lastPage()
{
  countPages();
  displayPageNum = maxPages;
  displayPage();
}

void Gui::setPage()
{
  QString string = setPageLineEdit->displayText();
  QRegExp rx("^(\\d+)\\s+.*$");
  if (string.contains(rx)) {
    bool ok;
    int inputPage;
    inputPage = rx.cap(1).toInt(&ok);
    if (ok) {
      countPages();
      if (inputPage <= maxPages) {
        if (inputPage != displayPageNum) {
          displayPageNum = inputPage;
          displayPage();
          return;
        }
      }
    }
  }
  string = QString("%1 of %2") .arg(displayPageNum) .arg(maxPages);
  setPageLineEdit->setText(string);
}

void Gui::fitWidth()
{
  pageView->scale(1.0,1.0);

  QRectF rect(0,0,page.meta.LPub.page.size.value(0),page.meta.LPub.page.size.value(1));

  QRectF unity = pageView->matrix().mapRect(QRectF(0,0,1,1));
  pageView->scale(1/unity.width(), 1 / unity.height());

  int margin = 2;
  QRectF viewRect = pageView->viewport()->rect().adjusted(margin, margin, -margin, -margin);
  QRectF sceneRect = pageView->matrix().mapRect(rect);
  qreal xratio = viewRect.width() / sceneRect.width();

  pageView->scale(xratio,xratio);
  pageView->centerOn(rect.center());
  fitMode = FitWidth;
}

void Gui::fitVisible()
{
  pageView->scale(1.0,1.0);

  QRectF rect(0,0,page.meta.LPub.page.size.value(0),page.meta.LPub.page.size.value(1));

  QRectF unity = pageView->matrix().mapRect(QRectF(0,0,1,1));
  pageView->scale(1/unity.width(), 1 / unity.height());

  int margin = 2;
  QRectF viewRect = pageView->viewport()->rect().adjusted(margin, margin, -margin, -margin);
  QRectF sceneRect = pageView->matrix().mapRect(rect);
  qreal xratio = viewRect.width() / sceneRect.width();
  qreal yratio = viewRect.height() / sceneRect.height();

  xratio = yratio = qMin(xratio,yratio);
  pageView->scale(xratio,yratio);
  pageView->centerOn(rect.center());
  fitMode = FitVisible;
}

void Gui::actualSize()
{
  pageView->resetMatrix();
  fitMode = FitNone;
}

void Gui::zoomIn()
{
  fitMode = FitNone;
  pageView->scale(1.1,1.1);
}

void Gui::zoomOut()
{
  fitMode = FitNone;
  pageView->scale(1.0/1.1,1.0/1.1);
}

void Gui::printToFile()
{
  QPrinter printer(QPrinter::HighResolution);
  printer.setPageSize(QPrinter::A4);
  if (page.meta.LPub.page.size.value(0) > page.meta.LPub.page.size.value(1)) {
    printer.setOrientation(QPrinter::Landscape);
  } else {
    printer.setOrientation(QPrinter::Portrait);
  }

  QString fileName = QFileDialog::getSaveFileName(
    this,tr("Print File Name"),QDir::currentPath(),tr("PDF (*.pdf)"));

  printer.setOutputFileName(fileName);
  QPainter painter;
  painter.begin(&printer);

  int savePageNumber = displayPageNum;

  for (displayPageNum = 1; displayPageNum <= maxPages; displayPageNum++) {

    clearPage();
    qApp->processEvents();
    drawPage();

    pageView->render(&painter);

    if (maxPages - displayPageNum > 0) {
      printer.newPage();
    }

  }
  painter.end();
  displayPageNum = savePageNumber;
  clearPage();
  drawPage();
}

void Gui::fileChanged(const QString &path)
{
  QString msg = QString(tr("The file \"%1\" contents have changed.  Reload?"))
                        .arg(path);
  int ret = QMessageBox::warning(this,tr(LPUB),msg,
              QMessageBox::Apply | QMessageBox::No,
              QMessageBox::Apply);
  if (ret == QMessageBox::Apply) {
    QString fileName = path;
    openFile(fileName);
    drawPage();
  }
}

void Gui::statusBarMsg(QString msg)
{
  statusBar()->showMessage(msg);
}

void Gui::displayFile(
  LDrawFile     *ldrawFile, 
  const QString &modelName, 
  bool force)
{
  if (force || modelName != curSubFile) {
    for (int i = 0; i < mpdCombo->count(); i++) {
      if (mpdCombo->itemText(i) == modelName) {
        mpdCombo->setCurrentIndex(i);
        break;
      }
    }
    curSubFile = modelName;
    displayFileSig(ldrawFile, modelName);
  }
}


/*-----------------------------------------------------------------------------*/

void Gui::insertLine(const Where &here, const QString &line, QUndoCommand *parent)
{
  if (ldrawFile.contains(here.modelName)) {
    undoStack->push(new InsertLineCommand(&ldrawFile,here,line,parent));
  }
}

void Gui::appendLine(const Where &here, const QString &line, QUndoCommand *parent)
{
  if (ldrawFile.contains(here.modelName)) {
    undoStack->push(new AppendLineCommand(&ldrawFile,here,line,parent));
  }
}
      
void Gui::replaceLine(const Where &here, const QString &line, QUndoCommand *parent)
{
  if (ldrawFile.contains(here.modelName) && 
      here.lineNumber < ldrawFile.size(here.modelName)) {

    undoStack->push(new ReplaceLineCommand(&ldrawFile,here,line,parent));
  }
}

void Gui::deleteLine(const Where &here, QUndoCommand *parent)
{
  if (ldrawFile.contains(here.modelName) && 
      here.lineNumber < ldrawFile.size(here.modelName)) {
    undoStack->push(new DeleteLineCommand(&ldrawFile,here,parent));
  }
}
QString Gui::readLine(const Where &here)
{
  return ldrawFile.readLine(here.modelName,here.lineNumber);
}
bool Gui::isSubmodel(const QString &modelName)
{
  return ldrawFile.contains(modelName);
}

void Gui::beginMacro(QString name)
{
  ++macroNesting;
  undoStack->beginMacro(name);
}

void Gui::endMacro()
{
  undoStack->endMacro();
  --macroNesting;
  displayPage();
  displayFile(&ldrawFile,curSubFile,true);
}

void Gui::contentsChange(
  int      position,
  int      _charsRemoved,
  const QString &charsAdded)
{
  QString  charsRemoved;

  /* Calculate the characters removed from the LDrawFile */

  if (_charsRemoved && ldrawFile.contains(curSubFile)) {

    QString contents = ldrawFile.contents(curSubFile).join("\n");

    charsRemoved = contents.mid(position,_charsRemoved);
  }
  
  undoStack->push(new ContentsChangeCommand(&ldrawFile,
                                            curSubFile,
                                            position,
                                            charsRemoved,
                                            charsAdded));
}

void Gui::undo()
{
  macroNesting++;
  undoStack->undo();
  macroNesting--;
  displayPage();
  displayFile(&ldrawFile,curSubFile,true);
}

void Gui::redo()
{
  macroNesting++;
  undoStack->redo();
  macroNesting--;
  displayPage();
  displayFile(&ldrawFile,curSubFile,true);
}

void Gui::canRedoChanged(bool enabled)
{
  redoAct->setEnabled(enabled);
}

void Gui::canUndoChanged(bool enabled)
{
  undoAct->setEnabled(enabled);
}
void Gui::cleanChanged(bool cleanState)
{
  saveAct->setDisabled( cleanState);
}

/***************************************************************************
 * File opening closing stuff
 **************************************************************************/

void Gui::openFile(QString &fileName)
{
  if (curFile != "") {
    if (isMpd()) {
      watcher.removePath(curFile);
    } else { 
      QStringList list = ldrawFile.subFileOrder();
      QString foo;
      foreach (foo,list) {
        QString bar = QDir::currentPath() + "/" + foo;
        watcher.removePath(bar);
      }
    }
  }

  clearPage();
  closeFile();
  displayPageNum = 1;
  QFileInfo info(fileName);
  QDir::setCurrent(info.absolutePath());
  Paths::mkdirs();
  ldrawFile.loadFile(fileName);
  mpdCombo->setMaxCount(0);
  mpdCombo->setMaxCount(1000);
  mpdCombo->addItems(ldrawFile.subFileOrder());
  setCurrentFile(fileName);
  displayFile(&ldrawFile,ldrawFile.topLevelFile());
  undoStack->setClean();
  curFile = fileName;
  QStringList list = ldrawFile.subFileOrder();
  QString foo;

  if (isMpd()) {
    watcher.addPath(curFile);
  } else {
    QStringList list = ldrawFile.subFileOrder();
    QString foo;
    foreach (foo,list) {
      QString bar = QDir::currentPath() + "/" + foo;
      watcher.addPath(bar);
    }
  }
}

void Gui::updateRecentFileActions()
{
  QSettings settings(LPUB,SETTINGS);
  QStringList files = settings.value("recentFileList").toStringList();

  int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

  for (int i = 0; i < numRecentFiles; i++) {
    QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
    recentFilesActs[i]->setText(text);
    recentFilesActs[i]->setData(files[i]);
    recentFilesActs[i]->setVisible(true);
  }
  for (int j = numRecentFiles; j < MaxRecentFiles; j++) {
    recentFilesActs[j]->setVisible(false);
  }
  separatorAct->setVisible(numRecentFiles > 0);
}

QString Gui::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

void Gui::setCurrentFile(const QString &fileName)
{
  QString shownName;
  if (fileName.size() == 0)
      shownName = "An LDraw Building Instruction Editor";
  else
      shownName = strippedName(fileName);

  setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr(LPUB)));

  if (fileName.size() > 0) {
    QSettings settings(LPUB, SETTINGS);
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll("");
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles) {
      files.removeLast();
    }
    settings.setValue("recentFileList", files);
  }
  updateRecentFileActions();
}

void Gui::mpdComboChanged(int index)
{
  index = index;
  QString newSubFile = mpdCombo->currentText();
  if (curSubFile != newSubFile) {
    curSubFile = newSubFile;
    displayFileSig(&ldrawFile, curSubFile);
  }
}

void Gui::clearPLICache()
{
  QString dirName = QDir::currentPath() + "/" + Paths::partsDir;
  QDir dir(dirName);

  dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);

  QFileInfoList list = dir.entryInfoList();
  for (int i = 0; i < list.size(); i++) {
    QFileInfo fileInfo = list.at(i);
    QFile     file(dirName + "/" + fileInfo.fileName());
    file.remove();
  }
}

void Gui::clearCSICache()
{
  QString dirName = QDir::currentPath() + "/" + Paths::assemDir;
  QDir dir(dirName);

  dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);

  QFileInfoList list = dir.entryInfoList();
  for (int i = 0; i < list.size(); i++) {
    QFileInfo fileInfo = list.at(i);
    QFile     file(dirName + "/" + fileInfo.fileName());
    file.remove();
  }
}

/***************************************************************************
 * These are infrequently used functions for basic environment 
 * configuration stuff
 **************************************************************************/

void Gui::pageSetup()
{
  GlobalPageDialog::getPageGlobals(ldrawFile.topLevelFile());
}

void Gui::assemSetup()
{
  GlobalAssemDialog::getAssemGlobals(ldrawFile.topLevelFile());
}

void Gui::pliSetup()
{
  GlobalPliDialog::getPliGlobals(ldrawFile.topLevelFile());
}

void Gui::calloutSetup()
{
  GlobalCalloutDialog::getCalloutGlobals(ldrawFile.topLevelFile());
}

void Gui::multiStepSetup()
{
  GlobalMultiStepDialog::getMultiStepGlobals(ldrawFile.topLevelFile());
}

void Gui::projectSetup()
{
  GlobalProjectDialog::getProjectGlobals(ldrawFile.topLevelFile());
}

void Gui::findLDraw()
{
  Paths::getLDrawPath(true,this);
}

void Gui::findLDGLite()
{
  bool ok;

  Paths::getLdglitePath(&ok, true, this);
}

void Gui::findLDView()
{
  bool ok;

  Paths::getLdviewPath(&ok, true, this);
}

void Gui::findPliFile()
{
  bool ok;

  Paths::getPliPath(&ok, true, this);
}

/*******************************************************************************
 *
 * This is all the initialization stuff.  It is used once when the program 
 * starts up
 *
 ******************************************************************************/

void Gui::setRenderer(
  QString const &pick)
{
  if (pick == "LDGLite") {
    renderer = &ldglite;
    clearPLICache();
    clearCSICache();
  } else if (pick == "LDView") {
    renderer = &ldview;
    clearPLICache();
    clearCSICache();
  }
}

Gui::Gui()
{
    displayPageNum = 1;

    //renderer = &ldview;
    renderer = &ldglite;

    editWindow    = new EditWindow();
    pageScene     = new QGraphicsScene(this);
    pageScene->setBackgroundBrush(Qt::lightGray);
    pageView      = new LGraphicsView(pageScene);
    pageView->pageBackgroundItem = NULL;
    pageView->setRenderHints(QPainter::Antialiasing | 
                             QPainter::TextAntialiasing |
                             QPainter::SmoothPixmapTransform);
    setCentralWidget(pageView);

    mpdCombo = new QComboBox;
    mpdCombo->setEditable(false);
    mpdCombo->setMinimumContentsLength(20);
    mpdCombo->setInsertPolicy(QComboBox::InsertAtBottom);
    mpdCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

    connect(mpdCombo,SIGNAL(activated(int)),
            this,    SLOT(mpdComboChanged(int)));

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    createDockWindows();

    readSettings();

    undoStack = new QUndoStack();
    macroNesting = 0;

    connect(this,       SIGNAL(displayFileSig(LDrawFile *, const QString &)),
            editWindow, SLOT(  displayFile   (LDrawFile *, const QString &)));
    connect(this,       SIGNAL(showLineSig(int)),
            editWindow, SLOT(  showLine(   int)));

    connect(editWindow, SIGNAL(contentsChange(int,int,const QString &)),
            this,       SLOT(  contentsChange(int,int,const QString &)));

    connect(editWindow, SIGNAL(redrawSig()),
            this,       SLOT(  redrawPage()));

    connect(undoStack,  SIGNAL(canRedoChanged(bool)),
            this,       SLOT(  canRedoChanged(bool)));
    connect(undoStack,  SIGNAL(canUndoChanged(bool)),
            this,       SLOT(  canUndoChanged(bool)));
    connect(undoStack,  SIGNAL(cleanChanged(bool)),
            this,       SLOT(  cleanChanged(bool)));

    connect(&watcher,   SIGNAL(fileChanged(const QString &)),
             this,      SLOT(  fileChanged(const QString &)));

    setCurrentFile("");

    resize(800,600);

    gui = this;

    fitMode = FitVisible;

#ifdef __APPLE__
    extern void qt_mac_set_native_menubar(bool);
    qt_mac_set_native_menubar(true);
#endif

}

Gui::~Gui()
{
    delete pageScene;
    delete pageView;
    delete editWindow;
}


void Gui::closeEvent(QCloseEvent *event)
{
  writeSettings();

  if (maybeSave()) {
    event->accept();
  } else {
    event->ignore();
  }
}

void Gui::about()
{
   QMessageBox::about(this, tr("About LPub"),
            tr("<b>LPub</b> is a WYSIWYG tool for creating LEGO building\n"
               "instructions Copyright 2000-2008 Kevin Clague"
               "kevin_clague@yahoo.com"));
}

void Gui::createActions()
{
    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    printToFileAct = new QAction(QIcon(":/images/pdf_logo.png"), tr("Print to &File"), this);
    printToFileAct->setShortcut(tr("Ctrl+F"));
    printToFileAct->setStatusTip(tr("Print your document to a file"));
    connect(printToFileAct, SIGNAL(triggered()), this, SLOT(printToFile()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    for (int i = 0; i < MaxRecentFiles; i++) {
      recentFilesActs[i] = new QAction(this);
      recentFilesActs[i]->setVisible(false);
      connect(recentFilesActs[i], SIGNAL(triggered()), this, 
                                 SLOT(openRecentFile()));
    }

    // undo/redo

    undoAct = new QAction(QIcon(":/images/editundo.png"), tr("Undo"), this);
    undoAct->setShortcut(tr("Ctrl+Z"));
    undoAct->setStatusTip(tr("Undo last change"));
    connect(undoAct, SIGNAL(triggered()), this, SLOT(undo()));
    redoAct = new QAction(QIcon(":/images/editredo.png"), tr("Redo"), this);
#ifdef __APPLE__
    redoAct->setShortcut(tr("Ctrl+Shift+Z"));
#else
    redoAct->setShortcut(tr("Ctrl+Y"));
#endif
    redoAct->setStatusTip(tr("Redo last change"));
    connect(redoAct, SIGNAL(triggered()), this, SLOT(redo()));

    // fitWidth,fitVisible,actualSize

    fitWidthAct = new QAction(QIcon(":/images/fitWidth.png"), tr("Fit Width"), this);
    fitWidthAct->setShortcut(tr("Ctrl+W"));
    fitWidthAct->setStatusTip(tr("Fit document to width"));
    connect(fitWidthAct, SIGNAL(triggered()), this, SLOT(fitWidth()));

    fitVisibleAct = new QAction(QIcon(":/images/fitVisible.png"), tr("Fit Visible"), this);
    fitVisibleAct->setShortcut(tr("Ctrl+I"));
    fitVisibleAct->setStatusTip(tr("Fit document so whole page is visible"));
    connect(fitVisibleAct, SIGNAL(triggered()), this, SLOT(fitVisible()));

    actualSizeAct = new QAction(QIcon(":/images/actual.png"),tr("Actual Size"), this);
    actualSizeAct->setShortcut(tr("Ctrl+A"));
    actualSizeAct->setStatusTip(tr("Show document actual size"));
    connect(actualSizeAct, SIGNAL(triggered()), this, SLOT(actualSize()));
    
    // zoomIn,zoomOut

    zoomInAct = new QAction(QIcon(":/images/zoomin.png"), tr("&Zoom In"), this);
    zoomInAct->setShortcut(tr("Ctrl++"));
    zoomInAct->setStatusTip(tr("Zoom in"));
    connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));

    zoomOutAct = new QAction(QIcon(":/images/zoomout.png"),tr("Zoom &Out"),this);
    zoomOutAct->setShortcut(tr("Ctrl+-"));
    zoomOutAct->setStatusTip(tr("Zoom out"));
    connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));

    // firstPage,lastPage,nextPage,previousPage

    firstPageAct = new QAction(QIcon(":/images/first.png"),tr("First Page"), this);
    firstPageAct->setShortcut(tr("Ctrl+F"));
    firstPageAct->setStatusTip(tr("Go to first page of document"));
    connect(firstPageAct, SIGNAL(triggered()), this, SLOT(firstPage()));

    lastPageAct = new QAction(QIcon(":/images/last.png"),tr("Last Page"), this);
    lastPageAct->setShortcut(tr("Ctrl+L"));
    lastPageAct->setStatusTip(tr("Go to last page of document"));
    connect(lastPageAct, SIGNAL(triggered()), this, SLOT(lastPage()));

    nextPageAct = new QAction(QIcon(":/images/next.png"),tr("&Next Page"),this);
    nextPageAct->setShortcut(tr("Ctrl+N"));
    nextPageAct->setStatusTip(tr("Go to next page of document"));
    connect(nextPageAct, SIGNAL(triggered()), this, SLOT(nextPage()));

    previousPageAct = new QAction(QIcon(":/images/prev.png"),tr("&Previous Page"),this);
    previousPageAct->setShortcut(tr("Ctrl+P"));
    previousPageAct->setStatusTip(tr("Go to previous page of document"));
    connect(previousPageAct, SIGNAL(triggered()), this, SLOT(prevPage()));

    QString pageString = "";
    setPageLineEdit = new QLineEdit(pageString,this);
    connect(setPageLineEdit, SIGNAL(returnPressed()), this, SLOT(setPage()));

    clearPLICacheAct = new QAction(tr("Clear Parts List Cache"), this);
    clearPLICacheAct->setStatusTip(tr("Erase the parts list image cache"));
    connect(clearPLICacheAct, SIGNAL(triggered()), this, SLOT(clearPLICache()));

    clearCSICacheAct = new QAction(tr("Clear Assembly Image Cache"), this);
    clearCSICacheAct->setStatusTip(tr("Erase the assembly image cache"));
    connect(clearCSICacheAct, SIGNAL(triggered()), this, SLOT(clearCSICache()));

    // Config menu

    pageSetupAct = new QAction(tr("Page Setup"), this);
    pageSetupAct->setStatusTip(tr("Default values for your project's pages"));
    connect(pageSetupAct, SIGNAL(triggered()), this, SLOT(pageSetup()));

    assemSetupAct = new QAction(tr("Assembly Setup"), this);
    assemSetupAct->setStatusTip(tr("Default values for your project's assembly images"));
    connect(assemSetupAct, SIGNAL(triggered()), this, SLOT(assemSetup()));

    pliSetupAct = new QAction(tr("Parts List Setup"), this);
    pliSetupAct->setStatusTip(tr("Default values for your project's parts lists"));
    connect(pliSetupAct, SIGNAL(triggered()), this, SLOT(pliSetup()));

    calloutSetupAct = new QAction(tr("Callout Setup"), this);
    calloutSetupAct->setStatusTip(tr("Default values for your project's callouts"));
    connect(calloutSetupAct, SIGNAL(triggered()), this, SLOT(calloutSetup()));

    multiStepSetupAct = new QAction(tr("Step Group Setup"), this);
    multiStepSetupAct->setStatusTip(tr("Default values for your project's step groups"));
    connect(multiStepSetupAct, SIGNAL(triggered()), this, SLOT(multiStepSetup()));

    projectSetupAct = new QAction(tr("Project Setup"), this);
    projectSetupAct->setStatusTip(tr("Default values for your project"));
    connect(projectSetupAct, SIGNAL(triggered()), this, SLOT(projectSetup()));

    ldrawPathAct = new QAction(tr("Find LDraw"), this);
    ldrawPathAct->setStatusTip(tr("Find the LDraw directory on this computer"));
    connect(ldrawPathAct, SIGNAL(triggered()), this, SLOT(findLDraw()));

    ldgliteAct = new QAction(tr("Find LDGLite"), this);
    ldgliteAct->setStatusTip(tr("Find LDGLite program on this computer"));
    connect(ldgliteAct, SIGNAL(triggered()), this, SLOT(findLDGLite()));

    ldviewAct = new QAction(tr("Find LDView"), this);
    ldviewAct->setStatusTip(tr("Find LDView program on this computer"));
    connect(ldviewAct, SIGNAL(triggered()), this, SLOT(findLDView()));

    pliAct = new QAction(tr("Find Parts List LDraw file\n"), this);
    pliAct->setStatusTip(tr("This defines orientation and soze of parts in Parts List Image"));
    connect(pliAct,SIGNAL(triggered()), this, SLOT(findPliFile()));

    // Help

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

void Gui::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addAction(printToFileAct);
    separatorAct = fileMenu->addSeparator();
    for (int i = 0; i < MaxRecentFiles; i++) {
      fileMenu->addAction(recentFilesActs[i]);
    }
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    menuBar()->addSeparator();

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(fitWidthAct);
    viewMenu->addAction(fitVisibleAct);
    viewMenu->addAction(actualSizeAct);
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);
    viewMenu->addSeparator();

    toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(firstPageAct);
    toolsMenu->addAction(previousPageAct);
    toolsMenu->addAction(nextPageAct);
    toolsMenu->addAction(lastPageAct);
    toolsMenu->addSeparator();
    toolsMenu->addAction(clearPLICacheAct);
    toolsMenu->addAction(clearCSICacheAct);

    configMenu = menuBar()->addMenu(tr("&Configuration"));
    configMenu->addAction(pageSetupAct);
    configMenu->addAction(assemSetupAct);
    configMenu->addAction(pliSetupAct);
    configMenu->addAction(calloutSetupAct);
    configMenu->addAction(multiStepSetupAct);
    configMenu->addAction(projectSetupAct);

    configMenu->addSeparator();

    configMenu->addAction(ldrawPathAct);
    configMenu->addAction(ldgliteAct);
    configMenu->addAction(ldviewAct);
    configMenu->addAction(pliAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
}

void Gui::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);
    fileToolBar->addAction(printToFileAct);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(undoAct);
    editToolBar->addAction(redoAct);

    zoomToolBar = addToolBar(tr("Zoom"));
    zoomToolBar->addAction(fitVisibleAct);
    zoomToolBar->addAction(fitWidthAct);
    zoomToolBar->addAction(zoomInAct);
    zoomToolBar->addAction(zoomOutAct);

    navigationToolBar = addToolBar(tr("Navigation"));
    navigationToolBar->addAction(firstPageAct);
    navigationToolBar->addAction(previousPageAct);
    setPageAct = navigationToolBar->addWidget(setPageLineEdit);
    navigationToolBar->addAction(nextPageAct);
    navigationToolBar->addAction(lastPageAct);
    mpdToolBar = addToolBar(tr("MPD"));
    mpdToolBar->addWidget(mpdCombo);
}

void Gui::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void Gui::createDockWindows()
{
    QDockWidget *dock = new QDockWidget(tr("LDraw File"), this);
    dock->setAllowedAreas(
      Qt::TopDockWidgetArea  | Qt::BottomDockWidgetArea |
      Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setWidget(editWindow);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());
}

void Gui::readSettings()
{
    QSettings settings(LPUB, "LEGO Building Instruction Tool");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    resize(size);
    move(pos);
}

void Gui::writeSettings()
{
    QSettings settings(LPUB, SETTINGS);
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}
