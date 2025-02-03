/* ======================================================================
    This file is part of ffDiaporama
    ffDiaporama is a tool to make diaporama as video
    Copyright (C) 2011-2014 Dominique Levray <domledom@laposte.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
   ====================================================================== */

#ifndef _GLOBALDEFINES_H
#define _GLOBALDEFINES_H

#include "BasicDefines.h"
#include <QElapsedTimer>
#include <QAction>
#include <QWidget>
#include <QFont>

#if QT_VERSION < 0x050000
    #ifdef Q_OS_WIN
    void    SetLFHeap();
    #endif
#endif

#ifdef Q_OS_WIN
    extern bool IsWindowsXP;    // True if OS is Windows/XP
#endif

//====================================================================

// Note : Application version and revision are in BUILDVERSION.txt
// Syntax for BUILDVERSION.txt is : <Version MAJOR.MINOR[.SUB|_beta_VERSION|_devel]>
#define APPLICATION_NAME                    "ffDiaporama"
// File extension of configuration files
#define CONFIGFILEEXT                       ".xml"
// Name of root node in the config xml file
#define CONFIGFILE_ROOTNAME                 "Configuration"
// Name of root node in the project xml file
#define APPLICATION_ROOTNAME                "Project"
// Name of root node in the title model xml files
#define TITLEMODEL_ROOTNAME                 "Model"
// Name of element in the title model xml files
#define TITLEMODEL_ELEMENTNAME              "TitleModel"
// Name of root node in the thumbnail xml files
#define THUMBMODEL_ROOTNAME                 "Thumbnail"
// Name of element in the thumbnail xml files
#define THUMBMODEL_ELEMENTNAME              "ProjectThumbnail"

// Application version : url to file on internet

    // devel version
//#define BUILDVERSION_WEBURL                 "http://ffdiaporama.tuxfamily.org/Devel/BUILDVERSION.txt"
//#define DOWNLOADPAGE                        "http://ffdiaporama.tuxfamily.org/?page_id=3635&lang=%1"
//#define LOCAL_WEBURL                        "http://download.tuxfamily.org/ffdiaporama/Devel/"
#define BUILDVERSION_WEBURL                 "http://ffDiaporama.sbk2day.com/BUILDVERSION.txt"
#define DOWNLOADPAGE                        "http://ffDiaporama.sbk2day.com"
#define LOCAL_WEBURL                        "http://ffDiaporama.sbk2day.com/"

    // stable version
//#define BUILDVERSION_WEBURL                 "http://ffdiaporama.tuxfamily.org/Stable/BUILDVERSION.txt"
//#define DOWNLOADPAGE                        "http://ffdiaporama.tuxfamily.org/?page_id=178&lang=%1"
//#define LOCAL_WEBURL                        "http://download.tuxfamily.org/ffdiaporama/Stable/"

// Global values
extern QString  CurrentAppName;             // Application name (including devel, beta, ...)
extern QString  CurrentAppVersion;          // Application version read from BUILDVERSION.txt
extern double   ScreenFontAdjust;           // System Font adjustement
extern int      SCALINGTEXTFACTOR;          // 700 instead of 400 (ffD 1.0/1.1/1.2) to keep similar display from plaintext to richtext
extern double   ScaleFontAdjust;

// URL to link to help page
#define HELPFILE_CAT                        "http://ffdiaporama.tuxfamily.org/?cat=%1&lang=%2"
#define ALLOWEDWEBLANGUAGE                  "en;fr;it;es;el;de;nl;pt;ru"

//====================================================================
// Latency for QTimer::singleShot(LATENCY, ... actions
#define LATENCY 5

//====================================================================

#define THUMBWITH                           600
#define THUMBHEIGHT                         800
#define THUMBGEOMETRY                       (double(THUMBWITH)/double(THUMBHEIGHT))
#define THUMB_THUMBWITH                     600/10
#define THUMB_THUMBHEIGHT                   800/10

//====================================================================

enum    FilterFile          {ALLFILE,IMAGEFILE,IMAGEVECTORFILE,VIDEOFILE,MUSICFILE};
enum    LoadConfigFileType  {USERCONFIGFILE,GLOBALCONFIGFILE};

//====================================================================
// Various functions
//====================================================================
void getMemInfo();
QString ito2a(int val);
QString ito3a(int val);
QString UpInitials(QString Source);
QString FormatLongDate(QDate EventDate);
QString GetInformationValue(QString ValueToSearch,QStringList const *InformationList);    // Get a value from a list of value (value as store in pair name##value)
QString GetInformationValue(QString ValueToSearch,const QStringList &InformationList);    // Get a value from a list of value (value as store in pair name##value)
QString GetCumulInfoStr(QStringList *InformationList,QString Key1,QString Key2);          // Return a string concataining each value of a key containing key1 and key2
int     getCpuCount();                                                                    // Retrieve number of processor
QString GetTextSize(int64_t Size);                                                        // transform a size (_int64) in a string with apropriate unit (Gb/Tb...)
QAction *CreateMenuAction(QImage *Icon,QString Text,int Data,bool Checkable,bool IsCheck,QWidget *Parent);
QAction *CreateMenuAction(QIcon Icon,QString Text,int Data,bool Checkable,bool IsCheck,QWidget *Parent);

//====================================================================
// VARIOUS
//====================================================================

// some helper-function for XML reading and writing
class xmlReadWrite
{
   QDomDocument domDocument;
   public:
      void readBool(bool *dest, const QDomElement &Element, const QString &attribute);
      QDomNode node(const QString &name, const QString& value);
      QDomNode node(const QString &name, int value);
      QDomNode node(const QString &name, qlonglong value);
      QDomNode node(const QString &name, bool value);
      QDomNode node(const QString &name, double value);
      QDomNode node(const QString &name, const QRect r);
      QDomNode node(const QString &name, const QRectF rf);
      QXmlStreamAttribute attribute(const QString &name, const QString& value);
      QXmlStreamAttribute attribute(const QString &name, int value);
      QXmlStreamAttribute attribute(const QString &name, qlonglong value);
      QXmlStreamAttribute attribute(const QString &name, bool value);
      QXmlStreamAttribute attribute(const QString &name, double value);
      QXmlStreamAttributes attributes(const QString &name, const QRect r);
      QXmlStreamAttributes attributes(const QString &name, const QRectF rf);

      void getAttrib(bool& value, const QDomElement& Element, const QString& attribute);
      void getAttrib(int& value, const QDomElement& Element, const QString& attribute);
      void getAttrib(double& value, const QDomElement& Element, const QString& attribute);
};

class ExXmlStreamWriter: public QXmlStreamWriter, public xmlReadWrite
{ 
public:
   ExXmlStreamWriter(QIODevice * device) : QXmlStreamWriter(device) {}
   ExXmlStreamWriter(QString *string) : QXmlStreamWriter(string) {}
   void writeAttribute(const QString name, const QString &value)
   {
      QXmlStreamWriter::writeAttribute(name, value);
   }
   void writeAttribute(const QString name, int value)
   {
      QXmlStreamWriter::writeAttribute(attribute(name, value));
   }
   void writeAttribute(const QString name, qlonglong value)
   {
      QXmlStreamWriter::writeAttribute(attribute(name, value));
   }
   void writeAttribute(const QString name, bool value)
   {
      QXmlStreamWriter::writeAttribute(attribute(name, value));
   }
   void writeAttribute(const QString name, double value)
   {
      QXmlStreamWriter::writeAttribute(attribute(name, value));
   }
   void writeAttribute(const QXmlStreamAttribute &att)
   {
      QXmlStreamWriter::writeAttribute(att);
   }
};

enum SELECTMODE {SELECTMODE_NONE, SELECTMODE_ONE, SELECTMODE_MULTIPLE};

void repairFonts(QString& ffdPart);
void repairFontsWindows(QString& ffdPart);
void repairFontsLinux(QString& ffdPart);
enum appFontNums
{
   Sans8, Sans9, Sans8Bold, Sans9Bold
};
QFont getAppFont(appFontNums which);
#endif // _GLOBALDEFINES_H
