
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
 * The traverse function is the one function that traverses the LDraw model
 * higherarchy seaching for pages to render.  It tracks the partial assembly
 * contents, parts list contents, step group contents, and callouts.
 *
 * It can count pages in the design, gather page contents for translation
 * into graphical representation of pages for the user.  In the future it
 * will gather Bill of Materials contents.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/
 
#include <QtGui>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QString>
#include "lpub.h"
#include "ranges.h"
#include "callout.h"
#include "pointer.h"
#include "range.h"
#include "reserve.h"
#include "step.h"

/*********************************************
 *
 * remove_group
 *
 * this removes members of a group from the
 * ldraw file held in the the ldr string
 *
 ********************************************/

static void remove_group(
  QStringList  in,
  QString      group,
  QStringList &out)
{
  QRegExp bgt("^\\s*0\\s+MLCAD\\s+BTG\\s+(.*)$");

  for (int i = 0; i < in.size(); i++) {
    QString line = in.at(i);

    if (line.contains(bgt)) {
      if (bgt.cap(bgt.numCaptures()) == group) {
        i++;
      } else {
        out << line;
      }
    } else {
      out << line;
    }
  }

  return;
}

/*********************************************
 *
 * remove_part
 *
 * this removes members of a part from the
 * ldraw file held in the the ldr string
 *
 ********************************************/

static void remove_parttype(
  QStringList  in,
  QString      model,
  QStringList &out)
{

  model = model.toLower();

  for (int i = 0; i < in.size(); i++) {
    QString line = in.at(i);
    QStringList tokens;
 
    split(line,tokens);

    if (tokens.size() == 15 && tokens[0] == "1") {
      QString type = tokens[14].toLower();
      if (type != model) {
        out << line;
      }
    } else {
      out << line;
    }
  }

  return;
}

/*********************************************
 *
 * remove_name
 *
 ********************************************/

static void remove_partname(
  QStringList  in,
  QString      name,
  QStringList &out)
{
  name = name.toLower();

  for (int i = 0; i < in.size(); i++) {
    QString line = in.at(i);
    QStringList tokens;
 
    split(line,tokens);

    if (tokens.size() == 4 && tokens[0] == "0" && 
                              tokens[1] == "LPUB" && 
                              tokens[2] == "NAME") {
      QString type = tokens[3].toLower();
      if (type == name) {
        for ( ; i < in.size(); i++) {
          line = in.at(i);
          split(line,tokens);
          if (tokens.size() == 15 && tokens[0] == "1") {
            break;
          } else {
            out << line;
          }
        }
      } else {
        out << line;
      }
    } else {
      out << line;
    }
  }

  return;
}

/*
 * This function, drawPage, is handed the parse state going into the page
 * that is to be displayed.  It gathers up a step group, or a single step,
 * including any called out models (think recursion), but ignores non-called
 * out submodels.  It stops parsing the LDraw files when it hits end of
 * page, at which point, it calls a function to convert the parsed and
 * retained results into Qt GraphicsItems for display to the user.
 *
 * This drawPage function is only called by the findPage function.  The findPage
 * function and this drawPage function used to be one function, but doing
 * this processing in one function was problematic.  The design issue is that
 * at start of step, or multistep, you do not know the page number, because
 * the step could contain submodels that are not called out, which produce at
 * least one page each.
 *
 * findPage (is below this drawPage function in this file), is lightweight
 * in that it is much smaller that the original combined function traverse.
 * Its design goal is to find the page the user wants displayed, and present
 * the parse state of the start of page to this function drawPage.
 *
 * depends on the current page number of the parse, and the page number the
 * user wants to see.  If the current page number is lower than the "display"
 * page number, the state of meta, the parts in the submodel, the filename
 * and linenumber of the first line of page is saved.  When findPage hits end
 * of page for the "display" page, it hands the saved start of page state to 
 * drawPage.  drawPage parses from start of page, creating a tree of data 
 * structures representing the content of the page.  At end of page, the 
 * tree is converted into Qt GraphicsItems for display.
 *
 * One thing to note is that findPage does the bulk of the LDraw file parsing
 * and is as lightweight (e.g. small) as I could make it.  Since callouts do
 * not have pages of their own (they are on the page of their parent step),
 * findPage ignores callouts.  Since findPage deals with non-callout submodels,
 * drawPage ignores non-called out submodels, and only deals with callout
 * submodels.
 *
 * After drawPage finishes gathering the page and converting the tree to
 * graphics items, it returns to findPage, which discards the parse state,
 * but continues parsing through to the last page, so we know how many pages
 * are in the building instuctions.
 *
 */
 
Range *newRange(
  Steps  *steps,
  bool    calledOut)
{
  Range *range;
  
  if (calledOut) {
    range = new Range(steps,
                      steps->meta.LPub.callout.alloc.value(),
                      steps->meta.LPub.callout.freeform);
  } else {
    range = new Range(steps,
                      steps->meta.LPub.multiStep.alloc.value(),
                      steps->meta.LPub.multiStep.freeform);
  }
  return range;
}

int Gui::drawPage(
  LGraphicsView  *view,
  QGraphicsScene *scene,
  Steps          *steps,
  int             stepNum,
  QString const  &addLine,
  Where          &current,
  QStringList     csiParts,
  Pli            &pli,
  QHash<QString, QStringList> &bfx,
  bool            calledOut)
{
  bool        global = true;
  QString     line;
  Callout    *callout     = NULL;
  Range      *range       = NULL;
  Step       *step        = NULL;
  bool        pliIgnore   = false;
  bool        partIgnore  = false;
  bool        synthBegin  = false;
  bool        multiStep   = false;
  bool        partsAdded  = false;
  int         numLines = ldrawFile.size(current.modelName);
  
  QList<InsertMeta> inserts;
  
  Where topOfStep = current;
  Rc gprc = OkRc;
  Rc rc;

  statusBar()->showMessage("Processing " + current.modelName);

  /*
   * do until end of file
   */
  for ( ; current <= numLines; current++) {

    QStringList tokens;
    
    // If we hit end of file we've got to note end of step

    if (current >= numLines) {
      line.clear();
      gprc = EndOfFileRc;
      tokens << "0";
      
      // not end of file, so get the next LDraw line 
     
    } else {
      line = ldrawFile.readLine(current.modelName,current.lineNumber);
      split(line,tokens);
    }
    
    if (tokens.size() == 15 && tokens[0] == "1") {
      
      QString color = tokens[1];
      QString type  = tokens[tokens.size()-1];

      partsAdded = true;

      /* since we have a part usage, we have a valid step */

      if (step == NULL) {
        if (range == NULL) {
          range = newRange(steps,calledOut);
          steps->append(range);
        }

        step = new Step(topOfStep,
                        range,
                        stepNum,
                        steps->meta,
                        calledOut,
                        multiStep,
                        pli);

        range->append(step);
      }

      /* addition of ldraw parts */

      if (steps->meta.LPub.pli.show.value() 
          && ! pliIgnore 
          && ! partIgnore 
          && ! synthBegin) {

        pli.append(&steps->meta,false,type,color,current,
                    steps->meta.LPub.pli.includeSubs.value());
      }
      csiParts << line;
  
      /* if it is a sub-model, then process it */

      if (ldrawFile.contains(type) && callout) {

        /* we are a callout, so gather all the steps within the callout */
        /* start with new meta, but no rotation step */

        if (callout->bottom.modelName != type) {

          Where current2(type,0);
          skipHeader(current2);          
          callout->meta.rotStep.clear();
          callout->meta.submodelStack << current;

          Meta saveMeta = callout->meta;

          step->append(callout);

          QStringList csiParts2;

          QHash<QString, QStringList> calloutBfx;

          int rc;

          rc = drawPage(
                 view,
                 scene,
                 callout,
                 1,
                 line,
                 current2,
                 csiParts2,
                 callout->pli,
                 calloutBfx,
                 true);

          callout->meta = saveMeta;

          if (rc != 0) {
            steps->placement = steps->meta.LPub.assem.placement;
            return rc;
          }
        } else {
          callout->instances++;
        }

        if (steps->meta.LPub.pli.show.value() && ! pliIgnore && ! partIgnore && ! synthBegin) {
          pli.unite(callout->pli);
        }

        /* remind user what file we're working on */

        statusBar()->showMessage("Processing " + current.modelName);
      }
    } else if (tokens.size() > 0 &&
              (tokens[0] == "2" ||
               tokens[0] == "3" ||
               tokens[0] == "4" ||
               tokens[0] == "5")) {
      
      partsAdded = true;

      /* we've got a line, triangle or polygon, so add it to the list */
      /* and make sure we know we have a step */

      if (step == NULL) {
        if (range == NULL) {            
          range = newRange(steps,calledOut);
          steps->append(range);
        }

        step = new Step(topOfStep,
                        range,
                        stepNum,
                        steps->meta,
                        calledOut,
                        multiStep,
                        pli);
        range->append(step);
      }
      csiParts << line;

    } else if (tokens.size() > 0 && tokens[0] == "0" || gprc == EndOfFileRc) {
      
      /* must be meta-command (or comment) */
      if (global && tokens.contains("!LPUB") && tokens.contains("GLOBAL")) {
        topOfStep = current;
      } else {
        global = false;
      }

      QString part;
      
      Meta   &curMeta = callout ? callout->meta : steps->meta;

      if (gprc == EndOfFileRc) {
        rc = gprc;
      } else {
        rc = curMeta.parse(line,current);
      }

      /* handle specific meta-commands */

      switch (rc) {

        /* substitute part/parts with this */

        case PliBeginSub1Rc:
          if (pliIgnore) {
            parseError("Nested PLI BEGIN/ENDS not allowed\n",current);
          } 
          if (steps->meta.LPub.pli.show.value() && 
              ! pliIgnore && 
              ! partIgnore && 
              ! synthBegin) {

            QString black("0");
            SubData subData = steps->meta.LPub.pli.begin.sub.value();
            pli.append(&steps->meta,false,subData.part,black,current,true);
          }

          if (step == NULL) {
            if (range == NULL) {
              range = newRange(steps,calledOut);
              steps->append(range);
            }
            step = new Step(topOfStep,
                            range,
                            stepNum,
                            steps->meta,
                            calledOut,
                            multiStep,
                            pli);
            range->append(step);
          }
          pliIgnore = true;
        break;

        /* substitute part/parts with this */
        case PliBeginSub2Rc:
          if (pliIgnore) {
            parseError("Nested BEGIN/ENDS not allowed\n",current);
          } 
          if (steps->meta.LPub.pli.show.value() && ! pliIgnore && ! partIgnore && ! synthBegin) {
            SubData subData = steps->meta.LPub.pli.begin.sub.value();
            pli.append(&steps->meta,false,subData.part,subData.color,current,true);
          }

          if (step == NULL) {
            if (range == NULL) {
              range = newRange(steps,calledOut);
              steps->append(range);
            }
            step = new Step(topOfStep,
                            range,
                            stepNum,
                            steps->meta,
                            calledOut,
                            multiStep,
                            pli);
            range->append(step);
          }
          pliIgnore = true;
        break;

        /* do not put subsequent parts into PLI */
        case PliBeginIgnRc:
          if (pliIgnore) {
            parseError("Nested BEGIN/ENDS not allowed\n",current);
          } 
          pliIgnore = true;
        break;
        case PliEndRc:
          if ( ! pliIgnore) {
            parseError("PLI END with no PLI BEGIN",current);
          }
          pliIgnore = false;
        break;

        /* discard subsequent parts, and don't create CSI's for them */
        case PartBeginIgnRc:
        case MLCadSkipBeginRc:
          if (partIgnore) {
            parseError("Nested BEGIN/ENDS not allowed\n",current);
          } 
          partIgnore = true;
        break;

        case PartEndRc:
        case MLCadSkipEndRc:
          if (partIgnore) {
            parseError("Ignore ending with no ignore begin",current);
          }
          partIgnore = false;
        break;

        case SynthBeginRc:
          if (synthBegin) {
            parseError("Nested BEGIN/ENDS not allowed\n",current);
          } 
          synthBegin = true;
        break;

        case SynthEndRc:
          if ( ! synthBegin) {
            parseError("Ignore ending with no ignore begin",current);
          }
          synthBegin = false;
        break;


        /* toss it all out the window, per James' original plan */
        case ClearRc:
          csiParts.empty();
          pli.clear();
          steps->freeSteps();
        break;

        /* Buffer exchange */
        case BufferStoreRc:
          {
            QString buffer = steps->meta.bfx.value();
            bfx[buffer].empty();
            bfx[buffer] = csiParts;
          }
        break;
        case BufferLoadRc:
          csiParts = bfx[steps->meta.bfx.value()];
        break;

        case MLCadGroupRc:
          csiParts << line;
        break;

        /* remove a group or all instances of a part type */
        case GroupRemoveRc:
        case RemoveGroupRc:
        case RemovePartRc:
        case RemoveNameRc:
          {
            QStringList newCSIParts;
            if (rc == RemoveGroupRc) {
              remove_group(csiParts,steps->meta.LPub.remove.group.value(),newCSIParts);
            } else if (rc == RemovePartRc) {
              remove_parttype(csiParts, steps->meta.LPub.remove.parttype.value(),newCSIParts);
            } else {
              remove_partname(csiParts, steps->meta.LPub.remove.partname.value(),newCSIParts);
            }
            csiParts = newCSIParts;
            newCSIParts.empty();

            if (step == NULL) {
              if (range == NULL) {
                range = newRange(steps,calledOut);
                steps->append(range);
              }
              step = new Step(topOfStep,
                              range,
                              stepNum,
                              steps->meta,
                              calledOut,
                              multiStep,
                              pli);
              range->append(step);
            }
          }
        break;
        
        case InsertRc:
          inserts.append(curMeta.LPub.insert);  // these are always placed before any parts in step
        break;

        case ReserveSpaceRc:
          /* since we have a part usage, we have a valid step */
          if (calledOut || multiStep) {
            step = NULL;
            Reserve *reserve = new Reserve(current,steps->meta.LPub);
            if (range == NULL) {
              range = newRange(steps,calledOut);
              steps->append(range);
            }
            range->append(reserve);
          }
        break;

        case CalloutBeginRc:
          if (callout) {
            parseError("Nested CALLOUT not allowed within the same file",current);
          } else {
            callout = new Callout(steps->meta,view);
            callout->setTopOfCallout(current);
          }
        break;

        case CalloutDividerRc:
          range->sepMeta = steps->meta.LPub.callout.sep;
          range = NULL;
          step = NULL;
        break;

        case CalloutPointerRc:
          if (callout) {
            callout->appendPointer(current,callout->meta.LPub.callout);
          }
        break;

        case CalloutEndRc:
          if ( ! callout) {
            parseError("CALLOUT END without a CALLOUT BEGIN",current);
          } else {
            callout->parent = step;
            callout->parentRelativeType = step->relativeType;
            callout->pli.clear();
            callout->placement = callout->meta.LPub.callout.placement;
            callout->setBottomOfCallout(current);
            callout = NULL;
          }
        break;

        case StepGroupBeginRc:
          if (calledOut) {
            parseError("MULTI_STEP not allowed inside callout models",current);
          } else {
            if (multiStep) {
              parseError("Nested MULTI_STEP not allowed",current);
            }
            multiStep = true;
          }
          steps->relativeType = StepGroupType;
        break;

        case StepGroupDividerRc:
          range->sepMeta = steps->meta.LPub.multiStep.sep;
          range = NULL;
          step = NULL;
        break;

        /* finished off a multiStep */
        case StepGroupEndRc:
          if (multiStep) {
            if (partsAdded) {
              parseError("Expected STEP before MULTI_STEP END", current);
            }
            multiStep = false;
  
            if (pli.tsize() != 0) {
              steps->pli = pli;
              steps->pli.sizePli(&steps->meta, StepGroupType);
            }
            pli.clear();

            /* this is a page we're supposed to process */

            steps->setBottomOfSteps(topOfStep);
            steps->placement = steps->meta.LPub.multiStep.placement;
            showLine(steps->bottomOfSteps());
            addGraphicsPageItems(steps, view, scene);
            return HitEndOfPage;
          }
        break;

        /* we're hit some kind of step, or implied step and end of file */
        case EndOfFileRc:
          steps->setBottomOfSteps(current);
        case RotStepRc:
        case StepRc:
          if (partsAdded) {
            if (pliIgnore) {
              parseError("PLI BEGIN then STEP. Expected PLI END",current);
              pliIgnore = false;
            }
            if (partIgnore) {
              parseError("PART BEGIN then STEP. Expected PART END",current);
              partIgnore = false;
            }
            if (synthBegin) {
              parseError("SYNTH BEGIN then STEP. Expected SYNTH_END",current);
              synthBegin = false;
            }

            bool pliPerStep;

            if (multiStep && steps->meta.LPub.multiStep.pli.perStep.value()) {
              pliPerStep = true;
            } else if (calledOut && steps->meta.LPub.callout.pli.perStep.value()) {
              pliPerStep = true;
            } else if ( ! multiStep && ! calledOut) {
              pliPerStep = true;
            } else {
              pliPerStep = false;
            }

            if (step) {
              step->setInserts(inserts);
              if (pliPerStep) {
                PlacementType relativeType;
                if (multiStep) {
                  relativeType = StepGroupType;
                } else if (calledOut) {
                  relativeType = CalloutType;
                } else {
                  relativeType = SingleStepType;
                }
                step->pli = pli;
                step->pli.sizePli(&steps->meta,relativeType);
                pli.clear();
              }

              step->csiPixmap.pixmap = new QPixmap;

              if (step->csiPixmap.pixmap == NULL) {
                // fatal
                exit(-1);
              }

              int rc = step->createCsi(
                curFile,
                addLine,
                csiParts,
                step->csiPixmap.pixmap,
                steps->meta);

              statusBar()->showMessage("Processing " + current.modelName);

              if (rc) {
                return rc;
              }
            } else {
              if (pliPerStep) {
                pli.clear();
              }
            }

            if ( ! multiStep && ! calledOut) {

              /*
               * Simple step
               */
              if (steps->list.size() == 0) {
                return 0;
              } else {
                steps->setBottomOfSteps(current);
                steps->placement = steps->meta.LPub.assem.placement;
                showLine(topOfStep);
                addGraphicsPageItems(steps,view,scene);
                return HitEndOfPage;
              }
            }
            steps->meta.pop();
            if (partsAdded) {
              stepNum++;
            }
            topOfStep = current;

            partsAdded = false;
            step = NULL;
          }
          inserts.clear();
        break;
        case RangeErrorRc:
          showLine(current);
          QMessageBox::critical(NULL,
                               QMessageBox::tr("LPub"),
                               QMessageBox::tr("Parameter(s) out of range: %1:%2\n%3")
                               .arg(current.modelName) 
                               .arg(current.lineNumber) 
                               .arg(line));
          return RangeErrorRc;
        break;
        default:
        break;
      }
    } else if (line != "") {
      if (tokens.size() > 0) {
        QString foo = tokens[0];
      }
      showLine(current);
      QMessageBox::critical(NULL,
                            QMessageBox::tr("LPub"),
                            QMessageBox::tr("Invalid LDraw Line Type: %1:%2\n  %3")
                            .arg(current.modelName) 
                            .arg(current.lineNumber) 
                            .arg(line));
      return InvalidLDrawLineRc;
    }
  }

  if (multiStep) {
    showLine(current);
    QMessageBox::critical(NULL,
                          QMessageBox::tr("LPub"),
                          QMessageBox::tr("End of %1 while multiStep pending")
                          .arg(current.modelName));
    multiStep = false;
  
    if (pli.tsize() != 0) {
      steps->pli = pli;
      steps->pli.sizePli(&steps->meta, StepGroupType);
    }
    pli.clear();

    /* this is a page we're supposed to process */

    steps->setBottomOfSteps(current);
    steps->placement = steps->meta.LPub.multiStep.placement;

    showLine(topOfStep);
    addGraphicsPageItems(steps, view, scene);
    return HitEndOfPage;
  }
  return 0;
}

void Gui::countPages()
{
  if (maxPages < 1) {
    statusBarMsg("Counting");
    Where       current(ldrawFile.topLevelFile(),0);
    int savedDpn   = displayPageNum;
    displayPageNum = 1 << 31;
    maxPages       = 1;
    Meta meta;
    QString empty;
    findPage(KpageView,KpageScene,maxPages,empty,current,meta);
    maxPages--;
    if (displayPageNum > maxPages) {
      displayPageNum = maxPages;
    } else {
      displayPageNum = savedDpn;
    }
    QString string = QString("%1 of %2") .arg(displayPageNum) .arg(maxPages);
    setPageLineEdit->setText(string);
    statusBarMsg("");
  }
}         

void Gui::drawPage(
  LGraphicsView  *view,
  QGraphicsScene *scene)
{

  QApplication::setOverrideCursor(Qt::WaitCursor);

  Where       current(ldrawFile.topLevelFile(),0);
  maxPages = 1;
  Meta meta;
  QString empty;
  findPage(view,scene,maxPages,empty,current,meta);
  maxPages--;  

  QString string = QString("%1 of %2") .arg(displayPageNum) .arg(maxPages);
  setPageLineEdit->setText(string);

  QApplication::restoreOverrideCursor();

}

void Gui::skipHeader(Where &current)
{
  int numLines = ldrawFile.size(current.modelName);
  for ( ; current.lineNumber < numLines; current.lineNumber++) {
    QString line = gui->readLine(current);
    int p;
    for (p = 0; p < line.size(); ++p) {
      if (line[p] != ' ') {
        break;
      }
    }
    if (line[p] >= '1' && line[p] <= '5') {
      if (current.lineNumber == 0) {
        QString empty = "0 ";
        gui->insertLine(current,empty,NULL);
      } else if (current > 0) {
        --current;
      }        
      break;
    } else if ( ! isHeader(line)) {
      if (current.lineNumber != 0) {
        --current;
        break;
      }
    }
  }
}

int Gui::findPage(
  LGraphicsView  *view,
  QGraphicsScene *scene,
  int            &pageNum,
  QString const  &addLine,
  Where           current,
  Meta           &meta)
{
  bool stepGroup = false;
  bool partIgnore = false;
  bool pageBegin = false;
  int  partsAdded = 0;
  int  stepNumber = 1;
  Rc   rc;
  
  skipHeader(current);

  QStringList csiParts;
  QStringList saveCsiParts;
  Where       saveCurrent = current;
  int         saveStepNumber = 1;

  Meta        tmpMeta;
  Meta        saveMeta = meta;
  Meta        stepGroupMeta;
  Pli         pli;

  QHash<QString, QStringList> bfx;
  QHash<QString, QStringList> saveBfx;

  int numLines = ldrawFile.size(current.modelName);
  
  Where topOfStep = current;

  for ( ;
       current.lineNumber < numLines;
       current.lineNumber++) {

    // scan through the rest of the model counting pages
    // if we've already hit the display page, then do as little as possible

    QString line = ldrawFile.readLine(current.modelName,current.lineNumber);

    while (line[0] == ' ') {
      line = line.mid(1);
    }

    if (strncmp(line.toAscii(),"0 GHOST ",8) == 0) {
      line = line.mid(8);
      while (line[0] == ' ') {
        line = line.mid(1);
      }
    }

    switch (line.toAscii()[0]) {
      case '1':
        if (pageNum < displayPageNum) {
          csiParts << line;
        }
        if ( ! partIgnore) {
          QStringList token = line.split(" ");
          if (ldrawFile.contains(token[token.size()-1])) {
            // can't be a callout
            tmpMeta = meta;
            tmpMeta.submodelStack << current;
            Where current2(token[token.size()-1],0);
            findPage(view,scene,pageNum,line,current2,tmpMeta);
          }
        }
        if (partsAdded++ == 0) {
          if (stepGroup) {
            pli.margin    = meta.LPub.multiStep.pli.margin;
            pli.placement = meta.LPub.multiStep.pli.placement;
          } else {
            pli.margin    = meta.LPub.pli.margin;
            pli.placement = meta.LPub.pli.placement;
          }
        }
      break;
      case '2':
      case '3':
      case '4':
      case '5':
        if (pageNum < displayPageNum) {
          csiParts << line;
        }
        if (partsAdded++ == 0) {
          if (stepGroup) {
            pli.margin    = meta.LPub.multiStep.pli.margin;
            pli.placement = meta.LPub.multiStep.pli.placement;
          } else {
            pli.margin    = meta.LPub.pli.margin;
            pli.placement = meta.LPub.pli.placement;
          }
        }
      break;
      case '0':
        rc = meta.parse(line,current);

        switch (rc) {
          case StepGroupBeginRc:
            stepGroup = true;
            saveCurrent = topOfStep;
            stepGroupMeta = meta;
          break;
          case StepGroupEndRc:
            if (stepGroup) {
              stepGroup = false;

              if (pageNum < displayPageNum) {
                saveCsiParts   = csiParts;
                saveStepNumber = stepNumber;
                saveMeta       = stepGroupMeta;
                saveBfx        = bfx;
              } else if (pageNum == displayPageNum) {
                csiParts.clear();
                ldrawFile.setNumSteps(current.modelName,stepNumber);
                page.meta      = saveMeta;
                (void) drawPage(view,scene,&page,saveStepNumber,
                                addLine,saveCurrent,saveCsiParts,pli,saveBfx);
                saveCurrent.modelName.clear();
                saveCsiParts.clear();
              }
              ++pageNum;
            }
          break;
          
          // STEP PAGE_BEGIN * PAGE_END PARTS STEP
          //                 +-- INSERT (PICTURE "name"|BOM|ARROW X Y X Y|TEXT "") TOP_LEFT PAGE INSIDE XX YY
          
          case PageBeginRc:
            if (partsAdded) {
            } else {
              pageBegin = true;
              saveCurrent = current;
            }
          break;
          case PageEndRc:
            if (pageBegin) {
              if (pageNum == displayPageNum) {
                //
                // So what we end up with is a list of inserts attached to page.
                //
                drawPage(view,scene,&page,saveCurrent);
              }
              pageBegin = false;
            }
          break;

          case StepRc:
          case RotStepRc:
            if (partsAdded) {
              ++stepNumber;
              meta.pop();
              if (pageNum < displayPageNum) {
                saveCsiParts   = csiParts;
                saveCurrent    = current;
                saveStepNumber = stepNumber;
                saveMeta       = meta;
                saveBfx        = bfx;
              }
              if ( ! stepGroup) {
                if (pageNum == displayPageNum) {
                  csiParts.clear();
                  ldrawFile.setNumSteps(current.modelName,stepNumber);
                  page.meta      = saveMeta;
                  (void) drawPage(view,scene,&page,saveStepNumber,
                                  addLine,saveCurrent,saveCsiParts,pli,saveBfx);
                  saveCurrent.modelName.clear();
                  saveCsiParts.clear();
                } 
                ++pageNum;
              }
              topOfStep = current;
              partsAdded = 0;
            } else if ( ! stepGroup) {
              saveCurrent = current;  // so that draw page doesn't have to
                                      // deal with steps that are not steps
            }
          break;  

          case CalloutBeginRc:
            do {
              line = ldrawFile.readLine(current.modelName,++current.lineNumber);
              while (line[0] == ' ') {
                line = line.mid(1);
              }
              rc = OkRc;
              if (line[0] == '0') {
                rc = tmpMeta.parse(line,current);
              } else if (line[0] >= '1' && line[0] <= '5') {
                partsAdded++;
                csiParts << line;
              }
            } while (rc != CalloutEndRc && current.lineNumber < numLines);
          break;
          case PartBeginIgnRc:
            partIgnore = true;
          break;
          case PartEndRc:
            partIgnore = false;
          break;

          // Any of the metas that can change csiParts needs
          // to be processed here

          case ClearRc:
            csiParts.empty();
          break;

          /* Buffer exchange */
          case BufferStoreRc:
            if (pageNum < displayPageNum) {              
              QString buffer = meta.bfx.value();
              bfx[buffer] = csiParts;
            }
          break;
          case BufferLoadRc:
            if (pageNum < displayPageNum) {
              csiParts = bfx[meta.bfx.value()];
            }
          break;

          case MLCadGroupRc:
            if (pageNum < displayPageNum) {
              csiParts << line;
              partsAdded++;
            }
          break;

          /* remove a group or all instances of a part type */
          case GroupRemoveRc:
          case RemoveGroupRc:
          case RemovePartRc:
          case RemoveNameRc:
            if (pageNum < displayPageNum) {
              QStringList newCSIParts;
              if (rc == RemoveGroupRc) {
                remove_group(csiParts,meta.LPub.remove.group.value(),newCSIParts);
              } else if (rc == RemovePartRc) {
                remove_parttype(csiParts, meta.LPub.remove.parttype.value(),newCSIParts);
              } else {
                remove_partname(csiParts, meta.LPub.remove.partname.value(),newCSIParts);
              }
              csiParts = newCSIParts;
              newCSIParts.empty();
            }
          break;
          
          default:
          break;
        } // switch
      break;
    }
  } // for every line
  csiParts.clear();
  if (partsAdded) {
    ldrawFile.setNumSteps(current.modelName,stepNumber);
    if (pageNum == displayPageNum) {
      page.meta = saveMeta;
      (void) drawPage(view, scene, &page,saveStepNumber,addLine,saveCurrent,saveCsiParts,pli,bfx);
    }
    ++pageNum;  
  } else {
    ldrawFile.setNumSteps(current.modelName,stepNumber-1);
  }
  saveCurrent.modelName.clear();
  saveCsiParts.clear();
  return 0;
}

int Gui::drawPage(
  LGraphicsView  * /* unused view */,
  QGraphicsScene * /* unused scene */,
  Steps          * /* unused page */,
  Where          & /* unused current */)
{
  return 0;
}
