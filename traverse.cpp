
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
#include "rx.h"
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

void endOfPage(
  Where   current,
  Ranges *ranges,
  Range  *range,
  Pli    &pli)
{
  ranges->placement = ranges->meta.LPub.multiStep.placement;
  pli.setBottomOfPLI(current);
  if (range) {
    range->context = ranges->meta.context;
    if (range->list.size()) {
      range->list[range->list.size()-1]->context = ranges->meta.context;
    }
  }
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
 * findPage only looks for page boundaries.  Its behavior at page boundaries
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

int Gui::drawPage(
  LGraphicsView  *view,
  QGraphicsScene *scene,
  Ranges      *ranges,
  int          stepNum,
  Where       &current,
  QStringList  csiParts,
  Pli         &pli,
  QHash<QString, QStringList> &bfx,
  bool         calledOut)
{
  int         numLines = ldrawFile.size(current.modelName);

  QString     line;

  Callout    *callout     = NULL;
  Range      *range       = NULL;
  Step       *step        = NULL;

  bool        pliIgnore   = false;
  bool        partIgnore  = false;
  bool        synthBegin  = false;

  bool        multiStep   = false;
  bool        partsAdded  = false;

  statusBar()->showMessage("Processing " + current.modelName);

  /*
   * Let Meta know our context for top of file and
   * top of callout
   */
  pli.setTopOfPLI(current);

  Rc gprc = OkRc;
  Rc rc;

  ++current;

  /*
   * do until end of file
   */
  for ( ; current <= numLines; current++) {

    QStringList tokens;

    if (current >= numLines) {
      line.clear();
      current--;
      ranges->meta.context.setBottomOfStep(current);
      current++;
      gprc = EndOfFileRc;
      tokens << "0";
     
    } else {
      line = ldrawFile.readLine(current.modelName,current.lineNumber);

      split(line,tokens);
    }

    /* is it a part usage? */

    if (tokens.size() == 15 && tokens[0] == "1") {
      QString color = tokens[1];
      QString type  = tokens[tokens.size()-1];

      partsAdded = true;

      /* since we have a part usage, we have a valid step */

      if (step == NULL) {
        if (range == NULL) {
          if (calledOut) {
            range = new Range(ranges,
                              ranges->meta.LPub.callout.alloc.value(),
                              ranges->meta.LPub.callout.sep.value(),
                              ranges->meta.LPub.callout.freeform);
          } else {
            range = new Range(ranges,
                              ranges->meta.LPub.multiStep.alloc.value(),
                              ranges->meta.LPub.multiStep.sep.value(),
                              ranges->meta.LPub.multiStep.freeform);
          }
          ranges->append(range);
        }

        step = new Step(range,
                        stepNum,
                        ranges->meta,
                        calledOut,
                        multiStep,
                        pli);

        range->append(step);
      }

      /* addition of ldraw parts */

      if (ranges->meta.LPub.pli.show.value() && ! pliIgnore && ! partIgnore && ! synthBegin) {
        pli.append(&ranges->meta,false,type,color,current,ranges->meta.LPub.pli.includeSubs.value());
      }
      csiParts << line;
  
      /* if it is a sub-model, then process it */

      if (ldrawFile.contains(type) && callout) {

        /* we are a callout, so gather all the steps within the callout */
        /* start with new meta, but no rotation step */

        if (callout->meta.context.topOfFile().modelName != type) {

          Where current2(type,0);
          callout->meta.context.setTopOfFile(type);
          callout->meta.context.setBottomOfStep(--current);
          callout->meta.context.setBottomOfStep(++current);
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
                 current2,
                 csiParts2,
                 callout->pli,
                 calloutBfx,
                 true);

          callout->meta = saveMeta;

          if (rc != 0) {
            endOfPage(current,ranges,range,pli);
            return rc;
          }
        } else {
          callout->instances++;
        }

        if (ranges->meta.LPub.pli.show.value() && ! pliIgnore && ! partIgnore && ! synthBegin) {
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
          if (calledOut) {
            range = new Range(ranges,
                              ranges->meta.LPub.callout.alloc.value(),
                              ranges->meta.LPub.callout.sep.value(),
                              ranges->meta.LPub.callout.freeform);
          } else {
            range = new Range(ranges,
                              ranges->meta.LPub.multiStep.alloc.value(),
                              ranges->meta.LPub.multiStep.sep.value(),
                              ranges->meta.LPub.callout.freeform);
          }
          ranges->append(range);
        }

        step = new Step(range,
                        stepNum,
                        ranges->meta,
                        calledOut,
                        multiStep,
                        pli);
        range->append(step);
      }
      csiParts << line;

    } else if (tokens.size() > 0 && tokens[0] == "0" || gprc == EndOfFileRc) {

      /* must be meta-command (or comment) */

      QString part;

      if (gprc == EndOfFileRc) {
        rc = gprc;
      } else {

        if (callout) {
          rc = callout->meta.parse(line,current,csiParts.size() != 0);
        } else {
          rc = ranges->meta.parse(line,current,csiParts.size() != 0);
        }
      }

      /* handle specific meta-commands */

      switch (rc) {

        /* substitute part/parts with this */

        case PliBeginSub1Rc:
          if (pliIgnore) {
            parseError("Nested PLI BEGIN/ENDS not allowed\n",current);
          } 
          if (ranges->meta.LPub.pli.show.value() && 
              ! pliIgnore && 
              ! partIgnore && 
              ! synthBegin) {

            QString black("0");
            SubData subData = ranges->meta.LPub.pli.begin.sub.value();
            pli.append(&ranges->meta,false,subData.part,black,current,true);
          }

          if (step == NULL) {
            if (range == NULL) {
              if (calledOut) {
                range = new Range(ranges,
                                  ranges->meta.LPub.callout.alloc.value(),
                                  ranges->meta.LPub.callout.sep.value(),
                                  ranges->meta.LPub.callout.freeform);
              } else {
                range = new Range(ranges,
                                  ranges->meta.LPub.multiStep.alloc.value(),
                                  ranges->meta.LPub.multiStep.sep.value(),
                                  ranges->meta.LPub.callout.freeform);
              }
              ranges->append(range);
            }
            step = new Step(range,
                            stepNum,
                            ranges->meta,
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
          if (ranges->meta.LPub.pli.show.value() && ! pliIgnore && ! partIgnore && ! synthBegin) {
            SubData subData = ranges->meta.LPub.pli.begin.sub.value();
            pli.append(&ranges->meta,false,subData.part,subData.color,current,true);
          }

          if (step == NULL) {
            if (range == NULL) {
              if (calledOut) {
                range = new Range(ranges,
                                  ranges->meta.LPub.callout.alloc.value(),
                                  ranges->meta.LPub.callout.sep.value(),
                                  ranges->meta.LPub.callout.freeform);
              } else {
                range = new Range(ranges,
                                  ranges->meta.LPub.multiStep.alloc.value(),
                                  ranges->meta.LPub.multiStep.sep.value(),
                                  ranges->meta.LPub.callout.freeform);
              }
              ranges->append(range);
            }
            step = new Step(range,
                            stepNum,
                            ranges->meta,
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
          pli.setTopOfPLI(current);
          ranges->freeRanges();
        break;

        /* Buffer exchange */
        case BufferStoreRc:
          {
            QString buffer = ranges->meta.bfx.value();
            bfx[buffer].empty();
            bfx[buffer] = csiParts;
          }
        break;
        case BufferLoadRc:
          csiParts = bfx[ranges->meta.bfx.value()];
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
              remove_group(csiParts,ranges->meta.LPub.remove.group.value(),newCSIParts);
            } else if (rc == RemovePartRc) {
              remove_parttype(csiParts, ranges->meta.LPub.remove.parttype.value(),newCSIParts);
            } else {
              remove_partname(csiParts, ranges->meta.LPub.remove.partname.value(),newCSIParts);
            }
            csiParts = newCSIParts;
            newCSIParts.empty();

            if (step == NULL) {
              if (range == NULL) {
                if (calledOut) {
                  range = new Range(ranges,
                                    ranges->meta.LPub.callout.alloc.value(),
                                    ranges->meta.LPub.callout.sep.value(),
                                    ranges->meta.LPub.callout.freeform);
                } else {
                  range = new Range(ranges,
                                    ranges->meta.LPub.multiStep.alloc.value(),
                                    ranges->meta.LPub.multiStep.sep.value(),
                                    ranges->meta.LPub.callout.freeform);
                }
                ranges->append(range);
              }
              step = new Step(range,
                              stepNum,
                              ranges->meta,
                              calledOut,
                              multiStep,
                              pli);
              range->append(step);
            }
          }
        break;

        case ReserveSpaceRc:
          /* since we have a part usage, we have a valid step */
          if (calledOut || multiStep) {
            step = NULL;
            Reserve *reserve = new Reserve(current,ranges->meta.LPub);
            if (range == NULL) {
              if (calledOut) {
                range = new Range(ranges,
                                  ranges->meta.LPub.callout.alloc.value(),
                                  ranges->meta.LPub.callout.sep.value(),
                                  ranges->meta.LPub.callout.freeform);
              } else {
                range = new Range(ranges,
                                  ranges->meta.LPub.multiStep.alloc.value(),
                                  ranges->meta.LPub.multiStep.sep.value(),
                                  ranges->meta.LPub.callout.freeform);
              }
              ranges->append(range);
            }
            range->append(reserve);
          }
        break;

        case CalloutBeginRc:
          if (callout) {
            parseError("Nested CALLOUT not allowed within the same file",current);
          } else {
            callout = new Callout(step,
                                  calledOut ? CalloutType : ranges->relativeType,
                                  ranges->meta,view);
            callout->meta.context.setTopOfRanges(current);
          }
        break;

        case CalloutDividerRc:
          if (range && range->list.size()) {
            range->context = ranges->meta.context;
            range->list[range->list.size()-1]->context = ranges->meta.context;
          }
          range = NULL;
          step = NULL;
        break;

        case CalloutPointerRc:
          if (callout) {
            callout->appendPointer(current,callout->meta.LPub.callout);
          }
        break;

        /*
         * Callout->meta.context.topOfRanges    == CALLOUT_BEGIN
         * Callout->meta.context.topOfStep      == Type 1 - 1
         * Callout->meta.context.bottomOfStep   == Type 1
         * Callout->meta.context.bottomOfRanges == CALLOUT_END
         */

        case CalloutEndRc:
          if ( ! callout) {
            parseError("CALLOUT END without a CALLOUT BEGIN",current);
          } else {
            callout->pli.clear();
            callout->placement = callout->meta.LPub.callout.placement;
            callout->meta.context.setBottomOfRanges(current);
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
          ranges->relativeType = StepGroupType;
        break;

        case StepGroupDividerRc:
          if (multiStep && range) {
            range->context = ranges->meta.context;
            if (range->list.size()) {
              range->list[range->list.size()-1]->context = ranges->meta.context;
            }
          }
          range = NULL;
          step = NULL;
        break;

        /* finished off a multiStep */
        case StepGroupEndRc:
          if (! multiStep) {
            parseError("MULTI_STEP END without MULTI_STEP begin", current);
          } else {
            if (partsAdded) {
              parseError("Expected STEP before MULTI_STEP END", current);
            }
            if (range) {
              range->context = ranges->meta.context;
            }
            multiStep = false;

            if (pli.tsize() != 0) {
              ranges->pli = pli;
              ranges->pli.sizePli(&ranges->meta, StepGroupType);
              ranges->pli.setBottomOfPLI(current);
            }
            pli.clear();

            /* this is a page we're supposed to process */

            endOfPage(current,ranges,range,pli);
            addGraphicsPageItems(ranges, view, scene);
            return HitEndOfPage;
          }
        break;

        /* we're hit some kind of step, or implied step and end of file */
        case EndOfFileRc:
          ranges->meta.context.setBottomOfRanges(current);
          ranges->meta.context.setBottomOfRange(current);
        case RotStepRc:
        case StepRc:
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
          if (step) {
            step->context = ranges->meta.context;
          }

          bool pliPerStep;

          if (multiStep && ranges->meta.LPub.multiStep.pli.perStep.value()) {
            pliPerStep = true;
          } else if (calledOut && ranges->meta.LPub.callout.pli.perStep.value()) {
            pliPerStep = true;
          } else if ( ! multiStep && ! calledOut) {
            pliPerStep = true;
          } else {
            pliPerStep = false;
          }

          if (step) {
            if (pliPerStep) {
              PlacementType relativeType;
              if (multiStep) {
                relativeType = StepGroupType;
              } else if (calledOut) {
                relativeType = CalloutType;
              } else {
                relativeType = SingleStepType;
              }
              pli.setBottomOfPLI(current);
              step->pli = pli;
              step->pli.sizePli(&ranges->meta,relativeType);
              pli.clear();
            }

            step->csiPixmap.pixmap = new QPixmap;

            if (step->csiPixmap.pixmap == NULL) {
              // fatal
              exit(-1);
            }

            int rc = step->createCsi(
              curFile,
              csiParts,
              step->csiPixmap.pixmap,
              ranges->meta);

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
            if (ranges->list.size() == 0) {
              return 0;
            } else {
              endOfPage(current,ranges,range,pli);
              addGraphicsPageItems(ranges,view,scene);
              return HitEndOfPage;
            }
          }
          ranges->meta.pop();
          if (partsAdded) {
            stepNum++;
          }
          partsAdded = false;
          step = NULL;
        break;
        case RangeErrorRc:
          displayFileSig(&ldrawFile,current.modelName);
          showLineSig(current.lineNumber);
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
      displayFileSig(&ldrawFile,current.modelName);
      showLineSig(current.lineNumber);
      QMessageBox::critical(NULL,
                            QMessageBox::tr("LPub"),
                            QMessageBox::tr("Invalid LDraw Line Type: %1:%2\n  %3")
                            .arg(current.modelName) 
                            .arg(current.lineNumber) 
                            .arg(line));
      return InvalidLDrawLineRc;
    }

  } while (gprc != EndOfFileRc);

  if (multiStep) {
    displayFileSig(&ldrawFile,current.modelName);
    showLineSig(current.lineNumber);
    QMessageBox::critical(NULL,
                          QMessageBox::tr("LPub"),
                          QMessageBox::tr("End of %1 while multiStep pending")
                          .arg(current.modelName));
    if (range) {
      range->context = ranges->meta.context;
    }
    multiStep = false;

    if (pli.tsize() != 0) {
      ranges->pli = pli;
      ranges->pli.sizePli(&ranges->meta, StepGroupType);
      ranges->pli.setBottomOfPLI(current);
    }
    pli.clear();

    /* this is a page we're supposed to process */

    endOfPage(current,ranges,range,pli);
    addGraphicsPageItems(ranges, view, scene);
    return HitEndOfPage;
  }

  if ( ! calledOut) {
    ranges->freeRanges();
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
    findPage(KpageView,KpageScene,maxPages,current,meta);
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
  findPage(view,scene,maxPages,current,meta);
  maxPages--;  

  QString string = QString("%1 of %2") .arg(displayPageNum) .arg(maxPages);
  setPageLineEdit->setText(string);

  QApplication::restoreOverrideCursor();

}         

int Gui::findPage(
  LGraphicsView  *view,
  QGraphicsScene *scene,
  int         &pageNum,
  Where        current,
  Meta        &meta)
{
  meta.context.setTopOfFile(current);

  bool stepGroup = false;
  bool partIgnore = false;
  int  partsAdded = 0;
  int  stepNumber = 1;
  Rc   rc;

  QStringList csiParts;
  QStringList saveCsiParts;
  Where       saveCurrent = current;
  int         saveStepNumber = 1;

  Meta        tmpMeta;
  Meta        saveMeta = meta;
  Pli         pli;

  QHash<QString, QStringList> bfx;
  QHash<QString, QStringList> saveBfx;

  int numLines = ldrawFile.size(current.modelName);

  for (current.lineNumber = 0;
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
        partsAdded++;
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
            findPage(view,scene,pageNum,current2,tmpMeta);
          }
        }
      break;
      case '2':
      case '3':
      case '4':
      case '5':
        partsAdded++;
        if (pageNum < displayPageNum) {
          csiParts << line;
        }
      break;
      case '0':
        rc = meta.parse(line,current,partsAdded);

        switch (rc) {
          case StepGroupBeginRc:
            stepGroup = true;
          break;
          case StepGroupEndRc:
            stepGroup = false;

            if (pageNum < displayPageNum) {
              saveCsiParts   = csiParts;
              saveCurrent    = current;
              saveStepNumber = stepNumber;
              saveMeta       = meta;
              saveBfx        = bfx;
            } else if (pageNum == displayPageNum) {
              csiParts.clear();
              ldrawFile.setNumSteps(current.modelName,stepNumber);
              page.meta      = saveMeta;
              (void) drawPage(view,scene,&page,saveStepNumber,saveCurrent,saveCsiParts,pli,saveBfx);
              saveCurrent.modelName.clear();
              saveCsiParts.clear();
            } 
            pageNum++;
          break;

          case StepRc:
          case RotStepRc:
            if (partsAdded) {
              ++stepNumber;
              if ( ! stepGroup) {
                if (pageNum < displayPageNum) {
                  saveCsiParts   = csiParts;
                  saveCurrent    = current;
                  saveStepNumber = stepNumber;
                  saveMeta       = meta;
                  saveBfx        = bfx;
                } else if (pageNum == displayPageNum) {
                  csiParts.clear();
                  ldrawFile.setNumSteps(current.modelName,stepNumber);
                  page.meta      = saveMeta;
                  (void) drawPage(view,scene,&page,saveStepNumber,saveCurrent,saveCsiParts,pli,saveBfx);
                  saveCurrent.modelName.clear();
                  saveCsiParts.clear();
                } 
                pageNum++;
              }
              partsAdded = 0;
              meta.pop();
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
                rc = tmpMeta.parse(line,current,partsAdded);
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
    } // meta
  } // for every line
  csiParts.clear();
  if (partsAdded) {
    ldrawFile.setNumSteps(current.modelName,stepNumber);
    if (pageNum == displayPageNum) {
      page.meta = saveMeta;
      (void) drawPage(view, scene, &page,saveStepNumber,saveCurrent,saveCsiParts,pli,bfx);
    }
    pageNum++;  
  } else {
    ldrawFile.setNumSteps(current.modelName,stepNumber-1);
  }
  saveCurrent.modelName.clear();
  saveCsiParts.clear();
  return 0;
}
