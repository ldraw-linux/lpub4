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

#include <QFileDialog>

#include "ui_preferences.h"
#include "preferencesdialog.h"
#include "lpub_preferences.h"

PreferencesDialog::PreferencesDialog(QWidget     *_parent)
{
  ui.setupUi(this);
  
  ui.ldrawPath->setText(    Preferences::ldrawPath);
  ui.pliName->setText(      Preferences::pliFile);
  ui.pliBox->setChecked(    Preferences::pliFile != "");
  ui.ldglitePath->setText(  Preferences::ldgliteExe);
  ui.ldgliteBox->setChecked(Preferences::ldgliteExe != "");
  ui.ldviewPath->setText(   Preferences::ldviewExe);
  ui.ldviewBox->setChecked( Preferences::ldviewExe != "");
  parent = _parent;
  if (Preferences::preferredRenderer == "LDView") {
    ui.preferredRenderer->setCurrentIndex(1);
	ui.preferredRenderer->setEnabled(true);
  } else if (Preferences::preferredRenderer == "LDGLite") {
    ui.preferredRenderer->setCurrentIndex(0);
	ui.preferredRenderer->setEnabled(true);
  } else {
    ui.preferredRenderer->setEnabled(false);
  }
}

void PreferencesDialog::on_browseLDraw_clicked()
{
  Preferences::ldrawPreferences(true);
}

void PreferencesDialog::on_browsePli_clicked()
{
  QFileDialog dialog(parent);
  
  dialog.setWindowTitle(tr("Locate Parts List orientation/size file"));
  dialog.setFileMode(QFileDialog::ExistingFile);

  dialog.setFilter("LDraw (*.mpd,*.dat,*.ldr)");

  if (dialog.exec()) {
    QStringList selectedFiles = dialog.selectedFiles();
	
	if (selectedFiles.size() == 1) {
      ui.pliName->setText(selectedFiles[0]);
	  QFileInfo  fileInfo(ui.pliName->displayText());
	  ui.pliBox->setChecked(fileInfo.exists());
	}
  }
}

void PreferencesDialog::on_browseLDView_clicked()
{
  QFileDialog dialog(parent);
  
  dialog.setWindowTitle(tr("Locate LDView program"));
  dialog.setFileMode(QFileDialog::ExistingFile);

#ifdef __APPLE__
  dialog.setFilter("Program (*.app,*.App)");
#else
  dialog.setFilter("Program (*.exe)");
#endif
  if (dialog.exec()) {
    QStringList selectedFiles = dialog.selectedFiles();
	
	if (selectedFiles.size() == 1) {
      ui.ldviewPath->setText(selectedFiles[0]);
	  QFileInfo  fileInfo(selectedFiles[0]);
	  if (fileInfo.exists()) {
 	    if ( ! ui.preferredRenderer->isEnabled()) {
	      ui.preferredRenderer->setCurrentIndex(1);
          ui.preferredRenderer->setEnabled(true);
		}
	  } 
	  ui.ldgliteBox->setChecked(fileInfo.exists());
	}
  }
}

void PreferencesDialog::on_browseLDGLite_clicked()
{
  QFileDialog dialog(parent);
  
  dialog.setWindowTitle(tr("Locate LDGLite program"));
  dialog.setFileMode(QFileDialog::ExistingFile);

#ifdef __APPLE__
  dialog.setFilter("Program (*.app,*.App)");
#else
  dialog.setFilter("Program (*.exe)");
#endif
  if (dialog.exec()) {
    QStringList selectedFiles = dialog.selectedFiles();
	
	if (selectedFiles.size() == 1) {
      ui.ldglitePath->setText(selectedFiles[0]);
	  QFileInfo  fileInfo(selectedFiles[0]);
	  if (fileInfo.exists()) {
 	    if ( ! ui.preferredRenderer->isEnabled()) {
	      ui.preferredRenderer->setCurrentIndex(0);
          ui.preferredRenderer->setEnabled(true);
		}
	  } 
      ui.ldgliteBox->setChecked(fileInfo.exists());
	}
  }
}

QString const PreferencesDialog::ldrawPath()
{
  return ui.ldrawPath->displayText();
}

QString const PreferencesDialog::pliFile()
{
  if (ui.pliBox->isChecked()) {
    return ui.pliName->displayText();
  }
  return "";
}

QString const PreferencesDialog::ldviewExe()
{
  if (ui.ldviewBox->isChecked()) {
    return ui.ldviewPath->displayText();
  } 
  return "";
}

QString const PreferencesDialog::ldgliteExe()
{
  if (ui.ldgliteBox->isChecked()) {
    return ui.ldglitePath->displayText();
  }
  return "";
}

QString const PreferencesDialog::preferredRenderer()
{
  if (ui.preferredRenderer->isEnabled()) {
    return ui.preferredRenderer->currentText();
  }
  return "";
}

