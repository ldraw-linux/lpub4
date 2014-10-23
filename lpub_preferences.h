/****************************************************************************
**
** Copyright (C) 2007-2009 Kevin Clague. All rights reserved.
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
#ifndef LPUB_PREFERENCES_H
#define LPUB_PREFERENCES_H

#ifdef __APPLE__
#define D_LDRAW_PATH_DEFAULT	"/usr/share/ldraw"
#define D_LDRAW_PATH_DEFAULT2	"/usr/local/share/ldraw"
#define D_LDGLITE_PATH_DEFAULT	"/usr/bin/ldglite"
#define D_LDVIEW_PATH_DEFAULT	"/usr/bin/ldview"
#define D_L3P_PATH_DEFAULT	"/usr/bin/l3p"
#define D_POVRAY_PATH_DEFAULT	"/usr/bin/povray"
#define D_LGEO_PATH_DEFAULT	"/usr/bin/lgeo"
#endif

#ifdef __linux__
#define D_LDRAW_PATH_DEFAULT	"/usr/share/ldraw"
#define D_LDRAW_PATH_DEFAULT2	"/usr/local/share/ldraw"
#define D_LDGLITE_PATH_DEFAULT	"/usr/bin/ldglite"
#define D_LDVIEW_PATH_DEFAULT	"/usr/bin/ldview"
#define D_L3P_PATH_DEFAULT	"/usr/bin/l3p"
#define D_POVRAY_PATH_DEFAULT	"/usr/bin/povray"
#define D_LGEO_PATH_DEFAULT	"/usr/bin/lgeo"
#endif

#ifdef _WIN32
#define D_LDRAW_PATH_DEFAULT	"c:/LDraw"
#define D_LDRAW_PATH_DEFAULT2	"c:/Program Files/LDraw"
#define D_LDGLITE_PATH_DEFAULT	"c:/Program Files/ldglite"
#define D_LDVIEW_PATH_DEFAULT	"c:/Program Files/ldview"
#define D_L3P_PATH_DEFAULT	"c:/Program Files/l3p"
#define D_POVRAY_PATH_DEFAULT	"c:/Program Files/povray"
#define D_LGEO_PATH_DEFAULT	"c:/Program Files/lgeo"
#endif

#if !defined(__APPLE__) && !defined(__linux__) && !defined(_WIN32)
#define D_LDRAW_PATH_DEFAULT	" "
#define D_LDRAW_PATH_DEFAULT2	" "
#define D_LDGLITE_PATH_DEFAULT	" "
#define D_LDVIEW_PATH_DEFAULT	" "
#define D_L3P_PATH_DEFAULT	" "
#define D_POVRAY_PATH_DEFAULT	" "
#define D_LGEO_PATH_DEFAULT	" "
#endif

#ifndef LDRAW_PATH_DEFAULT
#define LDRAW_PATH_DEFAULT	D_LDRAW_PATH_DEFAULT
#endif

#ifndef LDRAW_PATH_DEFAULT2
#define LDRAW_PATH_DEFAULT2	D_LDRAW_PATH_DEFAULT2
#endif

#ifndef LDGLITE_PATH_DEFAULT
#define LDGLITE_PATH_DEFAULT	D_LDGLITE_PATH_DEFAULT
#endif

#ifndef LDVIEW_PATH_DEFAULT
#define LDVIEW_PATH_DEFAULT	D_LDVIEW_PATH_DEFAULT
#endif

#ifndef L3P_PATH_DEFAULT
#define L3P_PATH_DEFAULT	D_L3P_PATH_DEFAULT
#endif

#ifndef POVRAY_PATH_DEFAULT
#define POVRAY_PATH_DEFAULT	D_POVRAY_PATH_DEFAULT
#endif

#ifndef LGEO_PATH_DEFAULT
#define LGEO_PATH_DEFAULT	D_LGEO_PATH_DEFAULT
#endif
class QString;

class Preferences
{

  public:
    Preferences();
    static void lpubPreferences();
    static void ldrawPreferences(bool);
	static void lgeoPreferences();
	  static void renderPreferences();
	  static void pliPreferences();
    static void unitsPreferences();
	  static void getRequireds();
	  static bool getPreferences();

    static QString ldrawPath;
    static QString lgeoPath;
    static QString ldgliteExe;
    static QString ldviewExe;
	static QString l3pExe;
	static QString povrayExe;
	  static QString preferredRenderer;
    static QString pliFile;
    static QString lpubPath;
    static bool    preferCentimeters;

    virtual ~Preferences() {}
};

#endif
