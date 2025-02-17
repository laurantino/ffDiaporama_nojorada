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

// Include some common various class
#include "cApplicationConfig.h"

// Include some additional standard class
#include "CustomCtrl/_QCustomDialog.h"
#include <QFileDialog>
#include <QPainter>

// Include some additional standard class
#include "cBaseMediaFile.h"
#include "_Diaporama.h"
#include "cLocation.h"
#include "cLuLoImageCache.h"
#ifndef _MSC_VER
#include <x86intrin.h>
#endif

#define USE_YUVCACHE_MAP
#define NO_SEEK_DEBUG
//****************************************************************************************************************************************************************
// EXIV2 PART
//****************************************************************************************************************************************************************
#ifdef Q_OS_WIN
    #include <exiv2/exif.hpp>
    #include "exiv2/version.hpp"
    #if (EXIV2_MAJOR_VERSION >= 0) || ((EXIV2_MAJOR_VERSION == 0) && (EXIV2_MINOR_VERSION > 20))
        #include <exiv2/exiv2.hpp>
        bool Exiv2WithPreview = true;
    #else
        bool Exiv2WithPreview = false;
        #include <image.hpp>
    #endif
#else
    #include <exiv2/exif.hpp>
    #if (EXIV2_MAJOR_VERSION>=0)||((EXIV2_MAJOR_VERSION==0)&&(EXIV2_MINOR_VERSION>20))
        #include <exiv2/exiv2.hpp>
        bool Exiv2WithPreview = true;
    #else
        bool Exiv2WithPreview = false;
        #include <exiv2/image.hpp>
    #endif
#endif

int  Exiv2MajorVersion = EXIV2_MAJOR_VERSION;
int  Eviv2MinorVersion = EXIV2_MINOR_VERSION;
int  Exiv2PatchVersion = EXIV2_PATCH_VERSION;

#ifdef _MSC_VER
#pragma warning(default: 4996)      /* enable deprecation */
#endif
//****************************************************************************************************************************************************************

#define FFD_APPLICATION_ROOTNAME    "Project"           // Name of root node in the project xml file
#define MaxAudioLenDecoded          AVCODEC_MAX_AUDIO_FRAME_SIZE*4

#ifndef INT64_MAX
    #define 	INT64_MAX   0x7fffffffffffffffLL
    #define 	INT64_MIN   (-INT64_MAX - 1LL)
#endif

//#define PIXFMT          PIX_FMT_RGB24
//#define QTPIXFMT        QImage::Format_RGB888
#define PIXFMT      AV_PIX_FMT_BGRA
#define QTPIXFMT    QImage::Format_ARGB32

//====================================================================================================================

cReplaceObjectList::cReplaceObjectList() 
{
}

void cReplaceObjectList::SearchAppendObject(QString SourceFileName) 
{
   int i = 0;
   while ((i < replaceObjects.count()) && (replaceObjects[i].SourceFileName != SourceFileName))
      i++;
   if (( i < replaceObjects.count()) && (replaceObjects[i].SourceFileName == SourceFileName))
   {
      // Object already define
   } 
   else 
   {
      // Object not yet defined
      QString DestFileName = QFileInfo(SourceFileName).baseName() + "." + QFileInfo(SourceFileName).completeSuffix();
      // Search if DestFileName already exist
      bool Cont = true;
      int  Num = 0;
      while (Cont) 
      {
         Cont = false;
         int j = 0;
         while (( j < replaceObjects.count()) && (replaceObjects[j].DestFileName != DestFileName))
            j++;
         if (( j < replaceObjects.count()) && (replaceObjects[j].DestFileName == DestFileName))
         {
            // DestFileName already defined
            Cont = true;
            Num++;
            DestFileName = QFileInfo(SourceFileName).baseName() + QString("-%1").arg(Num) + "." + QFileInfo(SourceFileName).completeSuffix();
         }
      }
      replaceObjects.append(cReplaceObject(SourceFileName,DestFileName));
   }
}

QString cReplaceObjectList::GetDestinationFileName(QString SourceFileName) 
{
   for (int i = 0; i < replaceObjects.count(); i++)
      if (replaceObjects[i].SourceFileName == SourceFileName)
         return replaceObjects[i].DestFileName;
   return SourceFileName;
}

int geometryFromSize(double width, double height)
{
   int geometry = IMAGE_GEOMETRY_UNKNOWN;
   double RatioHW = width/height;
   if      (RatioHW >= 1.45 && RatioHW <= 1.55) geometry = IMAGE_GEOMETRY_3_2;
   else if (RatioHW >= 0.65 && RatioHW <= 0.67) geometry = IMAGE_GEOMETRY_2_3;
   else if (RatioHW >= 1.32 && RatioHW <= 1.34) geometry = IMAGE_GEOMETRY_4_3;
   else if (RatioHW >= 0.74 && RatioHW <= 0.76) geometry = IMAGE_GEOMETRY_3_4;
   else if (RatioHW >= 1.77 && RatioHW <= 1.79) geometry = IMAGE_GEOMETRY_16_9;
   else if (RatioHW >= 0.56 && RatioHW <= 0.58) geometry = IMAGE_GEOMETRY_9_16;
   else if (RatioHW >= 2.34 && RatioHW <= 2.36) geometry = IMAGE_GEOMETRY_40_17;
   else if (RatioHW >= 0.42 && RatioHW <= 0.44) geometry = IMAGE_GEOMETRY_17_40;
   return geometry;
}

cBaseMediaFile* getMediaObject(enum OBJECTTYPE objectType, int AllowedFilter, cApplicationConfig* ApplicationConfig)
{
   cBaseMediaFile* MediaObject = NULL;
   switch (objectType)
   {
      case OBJECTTYPE_FOLDER:       if (AllowedFilter & FILTERALLOW_OBJECTTYPE_FOLDER)       MediaObject = new cFolder(ApplicationConfig);             break;
      case OBJECTTYPE_UNMANAGED:                                                             MediaObject = new cUnmanagedFile(ApplicationConfig);      break;
      case OBJECTTYPE_FFDFILE:      if (AllowedFilter & FILTERALLOW_OBJECTTYPE_FFDFILE)      MediaObject = new cffDProjectFile(ApplicationConfig);     break;
      case OBJECTTYPE_IMAGEFILE:    if (AllowedFilter & FILTERALLOW_OBJECTTYPE_IMAGEFILE)    MediaObject = new cImageFile(ApplicationConfig);          break;
      case OBJECTTYPE_IMAGEVECTOR:  if (AllowedFilter & FILTERALLOW_OBJECTTYPE_IMAGEVECTOR)  MediaObject = new cImageFile(ApplicationConfig);          break;
      case OBJECTTYPE_VIDEOFILE:    if (AllowedFilter & FILTERALLOW_OBJECTTYPE_VIDEOFILE)    MediaObject = new cVideoFile(ApplicationConfig);          break;
      case OBJECTTYPE_MUSICFILE:    if (AllowedFilter & FILTERALLOW_OBJECTTYPE_MUSICFILE)    MediaObject = new cMusicObject(ApplicationConfig);        break;
      case OBJECTTYPE_THUMBNAIL:    if (AllowedFilter & FILTERALLOW_OBJECTTYPE_THUMBNAIL)    MediaObject = new cImageFile(ApplicationConfig);          break;
         // to avoid warning
      case OBJECTTYPE_IMAGECLIPBOARD: if (AllowedFilter & FILTERALLOW_OBJECTTYPE_IMAGECLIPBOARD)    MediaObject = new cImageClipboard(ApplicationConfig);          break;
      case OBJECTTYPE_MANAGED:
      case OBJECTTYPE_GMAPSMAP:       break;
   }
   return MediaObject;
}
//*********************************************************************************************************************************************
// Base class object
//*********************************************************************************************************************************************

cBaseMediaFile::cBaseMediaFile(cApplicationConfig *TheApplicationConfig) 
{
    ApplicationConfig   = TheApplicationConfig;
    ObjectType          = OBJECTTYPE_UNMANAGED;
    ObjectName          = "NoName";
    Reset();
}

void cBaseMediaFile::Reset() 
{
    fileKey             = -1;
    folderKey           = -1;
    ressourceKey        = -1;
    IsValide            = false;                                    // if true then object if initialise
    IsInformationValide = false;                                    // if true then information list if fuly initialise
    geometry            = IMAGE_GEOMETRY_UNKNOWN;                   // Image geometry
    fileSize            = 0;
    imageWidth          = 0;                                        // Widht of normal image
    imageHeight         = 0;                                        // Height of normal image
    creatDateTime       = QDateTime(QDate(0,0,0),QTime(0,0,0));     // Original date/time
    modifDateTime       = QDateTime(QDate(0,0,0),QTime(0,0,0));     // Last modified date/time
    aspectRatio         = 1;
    imageOrientation    = -1;

    // Analyse
    GivenDuration           = QTime(0,0,0);
    RealAudioDuration       = QTime(0,0,0);
    RealVideoDuration       = QTime(0,0,0);
    IsComputedAudioDuration = false;
    IsComputedVideoDuration = false;
    SoundLevel              = -1;
}

//====================================================================================================================

cBaseMediaFile::~cBaseMediaFile() 
{
}

//====================================================================================================================

QTime cBaseMediaFile::GetRealDuration() 
{
   //to check
   if (IsComputedAudioDuration && IsComputedVideoDuration) 
      return RealAudioDuration < RealVideoDuration ? RealAudioDuration : RealVideoDuration;
   else if (IsComputedAudioDuration) 
      return RealAudioDuration;
   else if (IsComputedVideoDuration) 
      return RealVideoDuration;
   else
      return GivenDuration;
}

QTime cBaseMediaFile::GetRealAudioDuration() 
{
   if (IsComputedAudioDuration) 
      return RealAudioDuration; 
   return GivenDuration;
}

QTime cBaseMediaFile::GetRealVideoDuration() 
{
   if (IsComputedVideoDuration) 
      return RealVideoDuration; 
   else 
      return GivenDuration;
}

QTime cBaseMediaFile::GetGivenDuration() 
{
   return GivenDuration;
}

void cBaseMediaFile::SetGivenDuration(QTime GivenDuration) 
{
   this->GivenDuration = GivenDuration;
}

void cBaseMediaFile::SetRealAudioDuration(QTime RealDuration) 
{
   IsComputedAudioDuration = true;
   RealAudioDuration       = RealDuration;
}

void cBaseMediaFile::SetRealVideoDuration(QTime RealDuration) 
{
   IsComputedVideoDuration = true;
   RealVideoDuration       = RealDuration;
}

bool cBaseMediaFile::setSlideThumb(QImage Thumb)
{
   return ApplicationConfig->SlideThumbsTable->SetThumb(&ressourceKey, Thumb);
}

bool cBaseMediaFile::getSlideThumb(QImage *Thumb)
{
   return ApplicationConfig->SlideThumbsTable->GetThumb(&ressourceKey, Thumb);
}

//====================================================================================================================

QString cBaseMediaFile::FileName() 
{
   if (cachedFileName.isEmpty()) 
      cachedFileName = ApplicationConfig->FoldersTable->GetFolderPath(folderKey) + ApplicationConfig->FilesTable->GetShortName(fileKey);
   return cachedFileName;
}

//====================================================================================================================

QString cBaseMediaFile::ShortName() 
{
   if (cachedFileName.isEmpty()) 
      cachedFileName = ApplicationConfig->FoldersTable->GetFolderPath(folderKey) + ApplicationConfig->FilesTable->GetShortName(fileKey);
   return QFileInfo(cachedFileName).fileName();
}

//====================================================================================================================

QImage cBaseMediaFile::GetIcon(cCustomIcon::IconSize Size,bool useDelayed) 
{
   QImage Icon16, Icon100;
   ApplicationConfig->FilesTable->GetThumbs(fileKey, &Icon16, &Icon100);
   if (Size == cCustomIcon::ICON16) 
   {
      if (Icon16.isNull()) 
      {
         if (useDelayed) 
            Icon16 = ApplicationConfig->DefaultDelayedIcon.GetIcon(cCustomIcon::ICON16)->copy();
         else 
            Icon16 = GetDefaultTypeIcon(cCustomIcon::ICON16)->copy();
      }
      return Icon16;
   } 
   else 
   {
      if (Icon100.isNull()) 
      {
         if (useDelayed) 
            Icon100 = ApplicationConfig->DefaultDelayedIcon.GetIcon(cCustomIcon::ICON100)->copy();
         else 
            Icon100 = GetDefaultTypeIcon(cCustomIcon::ICON100)->copy();
      }
      return Icon100;
   }
}

//====================================================================================================================

bool cBaseMediaFile::GetFullInformationFromFile(bool IsPartial) 
{
   cCustomIcon Icon;
   QStringList ExtendedProperties;
   //bool bExtPropertiesOk = ApplicationConfig->FilesTable->GetExtendedProperties(fileKey, &ExtendedProperties);
   bool bExtPropertiesOk = ApplicationConfig->FilesTable->HasExtendedProperties(fileKey);
   bool bThumbsOk = ApplicationConfig->FilesTable->GetThumbs(fileKey, &Icon.Icon16, &Icon.Icon100);
   IsInformationValide = bExtPropertiesOk && bThumbsOk;
   //IsInformationValide = ApplicationConfig->FilesTable->GetExtendedProperties(FileKey, &ExtendedProperties) 
   //                      && ApplicationConfig->FilesTable->GetThumbs(FileKey, &Icon.Icon16, &Icon.Icon100);
   if (!IsInformationValide) 
   {
      //qDebug() << "IsInformationValide failed for " << FileName() << " (" << fileKey << ") " << bExtPropertiesOk << " " << bThumbsOk;
      IsInformationValide = GetChildFullInformationFromFile(IsPartial, &Icon, &ExtendedProperties);
      if (IsInformationValide) 
      {
         QDomDocument domDocument;
         QDomElement  root = domDocument.createElement("BasicProperties");
         domDocument.appendChild(root);
         SaveBasicInformationToDatabase(&root, "", "", false, NULL, NULL, false);
         IsInformationValide = ApplicationConfig->FilesTable->SetBasicProperties(fileKey, domDocument.toString())
            && ApplicationConfig->FilesTable->SetExtendedProperties(fileKey, &ExtendedProperties)
            && ApplicationConfig->FilesTable->SetThumbs(fileKey, &Icon.Icon16, &Icon.Icon100);
      }
   }
   return IsInformationValide;
}

//====================================================================================================================

bool cBaseMediaFile::GetInformationFromFile(QString& FileName, QStringList *AliasList, bool *ModifyFlag, qlonglong GivenFolderKey) 
{
   if (ModifyFlag) 
      *ModifyFlag = false;
   if (!cachedFileName.isEmpty() && cachedFileName != FileName) 
      cachedFileName = "";

   // Use aliaslist
   if (AliasList && !QFileInfo(FileName).exists()) 
   {
      // First test : seach for a new path+filename for this filename
      int i;
      for (i = 0; (i < AliasList->count()) && (!AliasList->at(i).startsWith(FileName)); i++);
      if ((i < AliasList->count()) && (AliasList->at(i).startsWith(FileName))) 
      {
         FileName = AliasList->at(i);
         if (FileName.indexOf("####") > 0) 
            FileName = FileName.mid(FileName.indexOf("####") + QString("####").length());
      } 
      else 
      {
         // Second test : use each replacement folder to try to find 
         i = 0;
         QString NewFileName = QFileInfo(FileName).absoluteFilePath();
         while ((i < AliasList->count()) && (!QFileInfo(NewFileName).exists())) 
         {
            QString OldName = AliasList->at(i);
            QString NewName = OldName.mid(OldName.indexOf("####") + QString("####").length());
            OldName = OldName.left(OldName.indexOf("####"));
            OldName = OldName.left(OldName.lastIndexOf(QDir::separator()));
            NewName = NewName.left(NewName.lastIndexOf(QDir::separator()));
            NewFileName = NewName+QDir::separator()+QFileInfo(FileName).fileName();
            i++;
         }
         if (QFileInfo(NewFileName).exists()) 
         {
            FileName = NewFileName;
            if (AliasList) 
               AliasList->append(FileName+"####" + NewFileName);
            if (ModifyFlag) 
               *ModifyFlag = true;
         }
      }
   }

   bool Continue = true;
   while (Continue && !QFileInfo(FileName).exists()) 
   {
      QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
      if (CustomMessageBox(ApplicationConfig->TopLevelWindow,QMessageBox::Question,QApplication::translate("cBaseMediaFile","Open file"),
         QApplication::translate("cBaseMediaFile","Impossible to open file ")+FileName+"\n"+QApplication::translate("cBaseMediaFile","Do you want to select another file ?"),
         QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)!=QMessageBox::Yes)
         Continue = false;
      else 
      {
         QString NewFileName = QFileDialog::getOpenFileName(ApplicationConfig->TopLevelWindow,QApplication::translate("cBaseMediaFile","Select another file for ") + QFileInfo(FileName).fileName(),
            ApplicationConfig->RememberLastDirectories ? QDir::toNativeSeparators(ApplicationConfig->SettingsTable->GetTextValue(QString("%1_path").arg(BrowserTypeDef[ObjectType==OBJECTTYPE_IMAGEFILE?BROWSER_TYPE_IMAGEONLY:ObjectType==OBJECTTYPE_VIDEOFILE?BROWSER_TYPE_VIDEOONLY:BROWSER_TYPE_SOUNDONLY].BROWSERString),DefaultMediaPath)):DefaultMediaPath,
            ApplicationConfig->GetFilterForMediaFile(ObjectType == OBJECTTYPE_IMAGEFILE ? IMAGEFILE : 
               ObjectType == OBJECTTYPE_IMAGEVECTOR ? IMAGEVECTORFILE :
               ObjectType == OBJECTTYPE_VIDEOFILE ? VIDEOFILE:
               MUSICFILE));
         if (NewFileName != "") 
         {
            if (AliasList) 
               AliasList->append(FileName+"####"+NewFileName);
            FileName = NewFileName;
            if (ApplicationConfig->RememberLastDirectories) 
               ApplicationConfig->SettingsTable->SetTextValue(QString("%1_path").arg(BrowserTypeDef[ObjectType==OBJECTTYPE_IMAGEFILE?BROWSER_TYPE_IMAGEONLY:ObjectType==OBJECTTYPE_VIDEOFILE?BROWSER_TYPE_VIDEOONLY:BROWSER_TYPE_SOUNDONLY].BROWSERString),QFileInfo(FileName).absolutePath());     // Keep folder for next use
            if (ModifyFlag) 
               *ModifyFlag = true;
         } 
         else 
            Continue = false;
      }
      QApplication::restoreOverrideCursor();
   }
   if (!Continue) 
   {
      ToLog(LOGMSG_CRITICAL,QApplication::translate("cBaseMediaFile","Impossible to open file %1").arg(FileName));
      return false;
   }

   QFileInfo fInfo(FileName);
   FileName = fInfo.absoluteFilePath();
   if (folderKey == -1)  
      folderKey = GivenFolderKey >= 0 ? GivenFolderKey : ApplicationConfig->FoldersTable->GetFolderKey(fInfo.absolutePath());
   if (fileKey == -1)    
      fileKey   = ApplicationConfig->FilesTable->GetFileKey(folderKey, fInfo.fileName(),ObjectType, true);

   QString BasicInfo;
   if (ApplicationConfig->FilesTable->GetBasicProperties(fileKey, &BasicInfo, FileName, &fileSize, &creatDateTime, &modifDateTime)) 
   {
      QDomDocument    domDocument;

      QString         errorStr;
      int             errorLine,errorColumn;
      if (domDocument.setContent(BasicInfo,true,&errorStr,&errorLine,&errorColumn)
         && domDocument.elementsByTagName("BasicProperties").length() > 0
         && domDocument.elementsByTagName("BasicProperties").item(0).isElement()==true
         ) 
      {
         QDomElement Element = domDocument.elementsByTagName("BasicProperties").item(0).toElement();
         IsValide = LoadBasicInformationFromDatabase(&Element,"","",AliasList,ModifyFlag,NULL,false);
         return IsValide;
      }
   }
   //QFileInfo fi(FileName);
   fileSize      = fInfo.size();
   modifDateTime = fInfo.lastModified();
   creatDateTime = fInfo.FILECREATIONDATE();

   IsValide = true;
   return true;
}

//====================================================================================================================

QString cBaseMediaFile::GetImageGeometryStr() 
{
   switch (geometry) 
   {
      case IMAGE_GEOMETRY_3_2  : return "3:2";
      case IMAGE_GEOMETRY_2_3  : return "2:3";
      case IMAGE_GEOMETRY_4_3  : return "4:3";
      case IMAGE_GEOMETRY_3_4  : return "3:4";
      case IMAGE_GEOMETRY_16_9 : return "16:9";
      case IMAGE_GEOMETRY_9_16 : return "9:16";
      case IMAGE_GEOMETRY_40_17: return "40:17";
      case IMAGE_GEOMETRY_17_40: return "17:40";
      default                  : return "";        //QApplication::translate("cBaseMediaFile","ns","Non standard image geometry");
   }
}

//====================================================================================================================

QString cBaseMediaFile::GetFileSizeStr() 
{
   return GetTextSize(fileSize);
}

//====================================================================================================================

QString cBaseMediaFile::GetFileDateTimeStr(bool Created) 
{
   QLocale loc;
   if (Created) 
      return loc.toString(creatDateTime, QLocale::ShortFormat);
   return loc.toString( modifDateTime, QLocale::ShortFormat);
}

//====================================================================================================================

QString cBaseMediaFile::GetImageSizeStr(ImageSizeFmt Fmt) 
{
   QString SizeInfo = "";
   QString FmtInfo  = "";
   QString GeoInfo  = "";

   if (imageWidth > 0 && imageHeight > 0) 
   {
      // Compute MPix
      double MPix = double(double(imageWidth)*double(imageHeight))/double(1000000);
      SizeInfo = QString("%1x%2").arg(imageWidth).arg(imageHeight);

      // now search if size is referenced in DefImageFormat
      for (int i = 0; i < 2; i++) 
         for (int j = 0; j < 3; j++) 
            for (int k = 0; k < NBR_SIZEDEF; k++) 
               if (DefImageFormat[i][j][k].Width == imageWidth && DefImageFormat[i][j][k].Height == imageHeight)
               {
                  FmtInfo=QString(DefImageFormat[i][j][k].Name).left(QString(DefImageFormat[i][j][k].Name).indexOf(" -"));
                  break;
               }
      if ((FmtInfo=="") && (MPix >= 1)) 
         FmtInfo = QString("%1").arg(MPix,8,'f',1).trimmed()+QApplication::translate("cBaseMediaFile","MPix");
      else 
         switch (imageHeight) 
         {
            case 240:   FmtInfo = "QVGA";     break;
            case 320:   FmtInfo = "HVGA";     break;
            case 480:   FmtInfo = "WVGA";     break;
            case 576:   FmtInfo = "DVD";      break;
            case 600:   FmtInfo = "SVGA";     break;
            case 720:   FmtInfo = "720p";     break;
            case 768:   FmtInfo = "XGA";      break;
            case 1080:  FmtInfo = "1080p";    break;
            default:    FmtInfo = "ns";       break;
         }
   }
   GeoInfo = GetImageGeometryStr();
   switch (Fmt) 
   {
      case FULLWEB  : return SizeInfo + ((FmtInfo+GeoInfo) != "" ? "(" + FmtInfo + (FmtInfo != "" ? "-" : "") + GeoInfo + ")" : "");
      case SIZEONLY : return SizeInfo;
      case FMTONLY  : return FmtInfo;
      case GEOONLY  : return GeoInfo;
      default       : return "";
   }
}

//====================================================================================================================
// return 3 lines to display Summary of media file in dialog box which need them
QStringList cBaseMediaFile::GetSummaryText(QStringList *ExtendedProperties) 
{
   QStringList SummaryText;
   SummaryText.append(ShortName()+"("+GetFileSizeStr()+")");
   SummaryText.append(GetImageSizeStr(cBaseMediaFile::FULLWEB));
   if (ObjectType == OBJECTTYPE_IMAGEFILE) 
   {
      SummaryText.append(GetInformationValue("composer",ExtendedProperties));
      if (GetInformationValue("Photo.ExposureTime",   ExtendedProperties) != "") 
         SummaryText[2] = SummaryText[2] + (SummaryText[2] != "" ? "-" : "") + GetInformationValue("Photo.ExposureTime", ExtendedProperties);
      if (GetInformationValue("Photo.ApertureValue",  ExtendedProperties) != "") 
         SummaryText[2] = SummaryText[2] + (SummaryText[2] != "" ? "-" : "") + GetInformationValue("Photo.ApertureValue",  ExtendedProperties);
      if (GetInformationValue("Photo.ISOSpeedRatings",ExtendedProperties) != "") 
         SummaryText[2] = SummaryText[2] + (SummaryText[2] != "" ? "-" : "") + GetInformationValue("Photo.ISOSpeedRatings",ExtendedProperties) + " ISO";
   } 
   else 
      SummaryText.append(QApplication::translate("DlgSlideProperties","Duration:")+GetRealDuration().toString("HH:mm:ss.zzz"));
   return SummaryText;
}

//*********************************************************************************************************************************************
// Unmanaged File
//*********************************************************************************************************************************************
cUnmanagedFile::cUnmanagedFile(cApplicationConfig *ApplicationConfig):cBaseMediaFile(ApplicationConfig) 
{
    ObjectType = OBJECTTYPE_UNMANAGED;
    IsInformationValide = true;
}

//====================================================================================================================

QString cUnmanagedFile::GetFileTypeStr() 
{
    return QApplication::translate("cBaseMediaFile","Unmanaged","File type");
}

//*********************************************************************************************************************************************
// Folder
//*********************************************************************************************************************************************

cFolder::cFolder(cApplicationConfig *ApplicationConfig):cBaseMediaFile(ApplicationConfig) 
{
   ObjectType = OBJECTTYPE_FOLDER;
}

//====================================================================================================================

bool cFolder::GetChildFullInformationFromFile(bool, cCustomIcon *Icon,QStringList *) 
{
   QString AdjustedFileName = FileName();
   if (!AdjustedFileName.endsWith(QDir::separator())) 
      AdjustedFileName = AdjustedFileName + QDir::separator();

   // Check if a folder.jpg file exist
   if (Icon->Icon16.isNull() || Icon->Icon100.isNull()) 
   {
      QFileInfoList Directorys = QDir(FileName()).entryInfoList(QDir::Files);
      for (int j = 0; j < Directorys.count(); j++) 
      {
         if (Directorys[j].fileName().toLower() == "folder.jpg") 
         {
            QString FileName = AdjustedFileName + Directorys[j].fileName();
            QImage Final(":img/FolderMask_200.png");
            QImage Img(FileName);
            QImage ImgF;
            if (double(Img.height())/double(Img.width())*double(Img.width()) <= 162) 
               ImgF = Img.scaledToWidth(180,Qt::SmoothTransformation);
            else 
               ImgF = Img.scaledToHeight(162,Qt::SmoothTransformation);
            QPainter Painter;
            Painter.begin(&Final);
            Painter.drawImage(QRect((Final.width()-ImgF.width())/2,195-ImgF.height(),ImgF.width(),ImgF.height()),ImgF);
            Painter.end();
            Icon->LoadIcons(&Final);
         }
      }
   }

   // Check if there is an desktop.ini ==========> WINDOWS EXTENSION
   if (Icon->Icon16.isNull() || Icon->Icon100.isNull()) 
   {
      QFileInfoList Directorys = QDir(FileName()).entryInfoList(QDir::Files|QDir::Hidden);
      for (int j = 0; j < Directorys.count(); j++) 
      {
         if (Directorys[j].fileName().toLower() == "desktop.ini") 
         {
            QFile FileIO(AdjustedFileName + Directorys[j].fileName());
            QString IconFile = "";
#ifdef Q_OS_WIN
            int IconIndex = 0;
#endif
            if (FileIO.open(QIODevice::ReadOnly/*|QIODevice::Text*/)) 
            {
               // Sometimes this kind of files have incorrect line terminator : nor \r\n nor \n
               QTextStream FileST(&FileIO);
               QString     AllInfo = FileST.readAll();
               QString     Line = "";
               while (AllInfo != "") 
               {
                  int j = 0;
                  while ( j < AllInfo.length() && (AllInfo[j] >= char(32) || AllInfo[j] == QChar(QChar::Tabulation)))
                     j++;
                  if (j < AllInfo.length()) 
                  {
                     Line = AllInfo.left(j);
                     while (j < AllInfo.length() && AllInfo[j] <= char(32)) 
                        j++;
                     if ( j < AllInfo.length()) 
                        AllInfo = AllInfo.mid(j); 
                     else 
                        AllInfo = "";
                  } 
                  else 
                  {
                     Line = AllInfo;
                     AllInfo = "";
                  }
#ifdef Q_OS_WIN
                  if (Line.toUpper().startsWith("ICONINDEX") && Line.indexOf("=") != -1) 
                  {
                     IconIndex = Line.mid(Line.indexOf("=")+1).toInt();
                  } 
                  else
#endif
                     if (Line.toUpper().startsWith("ICONFILE") && Line.indexOf("=") != -1) 
                     {
                        Line = Line.mid(Line.indexOf("=")+1).trimmed();
                        // Replace all variables like %systemroot%
                        while (Line.indexOf("%") != -1) 
                        {
                           QString Var = Line.mid(Line.indexOf("%")+1);  
                           Var = Var.left(Var.indexOf("%"));
                           QString Value = getenv(Var.toLocal8Bit());
                           Line.replace("%" + Var + "%", Value,Qt::CaseInsensitive);
                        }
                        if (QFileInfo(Line).isRelative()) 
                           IconFile = QDir::toNativeSeparators(AdjustedFileName + Line);
                        else 
                           IconFile = QDir::toNativeSeparators(QFileInfo(Line).absoluteFilePath());
                     }
               }
               FileIO.close();
            }
            if (IconFile.toLower().endsWith(".jpg") || IconFile.toLower().endsWith(".png") || IconFile.toLower().endsWith(".ico")) 
               Icon->LoadIcons(IconFile);
#ifdef Q_OS_WIN
            else 
               Icon->LoadIcons(GetIconForFileOrDir(IconFile,IconIndex));
#endif
         }
      }
   }

   // if no icon then load default for type
   if (Icon->Icon16.isNull() || Icon->Icon100.isNull()) 
      Icon->LoadIcons(&ApplicationConfig->DefaultFOLDERIcon);
   return true;
}

//====================================================================================================================

QString cFolder::GetFileTypeStr() 
{
    return QApplication::translate("cBaseMediaFile","Folder","File type");
}

//*********************************************************************************************************************************************
// ffDiaporama project file
//*********************************************************************************************************************************************

cffDProjectFile::cffDProjectFile(cApplicationConfig *ApplicationConfig):cBaseMediaFile(ApplicationConfig) 
{
   ObjectType  = OBJECTTYPE_FFDFILE;
   nbrSlide    = 0;
   nbrChapters = 0;

   InitDefaultValues();
}

cffDProjectFile::~cffDProjectFile() 
{
   if (location) 
      delete location;
   location = NULL;
}

//====================================================================================================================

void cffDProjectFile::InitDefaultValues() 
{
   title           = QApplication::translate("cModelList","Project title");
   author          = QApplication::translate("cModelList","Project author");
   album           = QApplication::translate("cModelList","Project album");
   overrideDate    = false;
   eventDate       = QDate::currentDate();
   longDate        = (overrideDate ? longDate : FormatLongDate(eventDate));
   comment         = QApplication::translate("cModelList","Project comment");
   composer        = "";
   defaultLanguage = ApplicationConfig->DefaultLanguage;
   ffDRevision     = CurrentAppVersion;
   location        = NULL;
}

//====================================================================================================================

bool cffDProjectFile::LoadBasicInformationFromDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes) 
{
   return LoadFromXML(ParentElement,ElementName,PathForRelativPath,AliasList,ModifyFlag,ResKeyList,DuplicateRes,true);
}

//====================================================================================================================

void cffDProjectFile::SaveBasicInformationToDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool) 
{
   SaveToXML(ParentElement,ElementName,PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,false);

   QDomDocument DomDocument;
   QDomElement  Element = DomDocument.createElement("Project");
   Element.setAttribute("ImageGeometry",geometry == IMAGE_GEOMETRY_16_9 ? GEOMETRY_16_9 : geometry == IMAGE_GEOMETRY_40_17 ? GEOMETRY_40_17 : GEOMETRY_4_3);
   Element.setAttribute("ObjectNumber",nbrSlide);
   ParentElement->appendChild(Element);
}
//====================================================================================================================

void cffDProjectFile::SaveToXML(QDomElement *ParentElement,QString,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel) 
{
   QDomDocument DomDocument;
   QString xmlFragment;
   ExXmlStreamWriter xlmStream(&xmlFragment);
   SaveToXMLex(xlmStream, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
   DomDocument.setContent(xmlFragment);
   ParentElement->appendChild(DomDocument.firstChildElement());
   return;
   /*
   QDomElement  Element=DomDocument.createElement("ffDiaporamaProjectProperties");
   Element.setAttribute("Title",title);
   Element.setAttribute("Author",author);
   Element.setAttribute("Album",album);
   Element.setAttribute("LongDate",longDate);
   Element.setAttribute("EventDate",eventDate.toString(Qt::ISODate));
   Element.setAttribute("OverrideDate",overrideDate?1:0);
   Element.setAttribute("Comment",comment);
   Element.setAttribute("Composer",composer);
   Element.setAttribute("Duration",qlonglong(QTime(0,0,0,0).msecsTo(GetRealDuration())));
   Element.setAttribute("ffDRevision",ffDRevision);
   Element.setAttribute("DefaultLanguage",defaultLanguage);
   Element.setAttribute("ChaptersNumber",nbrChapters);
   for (int i = 0; i < nbrChapters; i++) 
   {
      QString ChapterNum = QString("%1").arg(i); 
      while (ChapterNum.length() < 3) 
         ChapterNum = "0" + ChapterNum;
      QDomElement SubElement = DomDocument.createElement("Chapter_"+ChapterNum);
      SubElement.setAttribute("Start",    GetInformationValue("Chapter_"+ChapterNum+":Start",&chaptersProperties));
      SubElement.setAttribute("End",      GetInformationValue("Chapter_"+ChapterNum+":End",&chaptersProperties));
      SubElement.setAttribute("Duration", GetInformationValue("Chapter_"+ChapterNum+":Duration",&chaptersProperties));
      SubElement.setAttribute("title",    GetInformationValue("Chapter_"+ChapterNum+":title",&chaptersProperties));
      SubElement.setAttribute("InSlide",  GetInformationValue("Chapter_"+ChapterNum+":InSlide",&chaptersProperties));
      Element.appendChild(SubElement);
   }
   if (location) 
   {
      QDomElement SubElement = DomDocument.createElement("PrjLocation");
      location->SaveToXML(&SubElement,"",PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,IsModel);
      Element.appendChild(SubElement);
   }
   ParentElement->appendChild(Element);
   */
}
void cffDProjectFile::SaveToXMLex(ExXmlStreamWriter &xmlStream, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel)
{
   xmlStream.writeStartElement("ffDiaporamaProjectProperties");
   xmlStream.writeAttribute("Title", title);
   xmlStream.writeAttribute("Author", author);
   xmlStream.writeAttribute("Album", album);
   xmlStream.writeAttribute("LongDate", longDate);
   xmlStream.writeAttribute("EventDate", eventDate.toString(Qt::ISODate));
   xmlStream.writeAttribute("OverrideDate", overrideDate );
   xmlStream.writeAttribute("Comment", comment);
   xmlStream.writeAttribute("Composer", composer);
   xmlStream.writeAttribute("Duration", QString("%1").arg(qlonglong(QTime(0, 0, 0, 0).msecsTo(GetRealDuration()))));
   xmlStream.writeAttribute("ffDRevision", ffDRevision);
   xmlStream.writeAttribute("DefaultLanguage", defaultLanguage);
   xmlStream.writeAttribute("ChaptersNumber", QString("%1").arg(nbrChapters));
   for (int i = 0; i < nbrChapters; i++)
   {
      QString ChapterNum = QString("%1").arg(i);
      while (ChapterNum.length() < 3)
         ChapterNum = "0" + ChapterNum;
      xmlStream.writeStartElement("Chapter_" + ChapterNum);
      xmlStream.writeAttribute("Start", GetInformationValue("Chapter_" + ChapterNum + ":Start", &chaptersProperties));
      xmlStream.writeAttribute("End", GetInformationValue("Chapter_" + ChapterNum + ":End", &chaptersProperties));
      xmlStream.writeAttribute("Duration", GetInformationValue("Chapter_" + ChapterNum + ":Duration", &chaptersProperties));
      xmlStream.writeAttribute("title", GetInformationValue("Chapter_" + ChapterNum + ":title", &chaptersProperties));
      xmlStream.writeAttribute("InSlide", GetInformationValue("Chapter_" + ChapterNum + ":InSlide", &chaptersProperties));
      xmlStream.writeEndElement();
   }
   if (location)
   {
      xmlStream.writeStartElement("PrjLocation");
      location->SaveToXMLex(xmlStream, "", PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
      xmlStream.writeEndElement();
   }
   xmlStream.writeEndElement();
}


//====================================================================================================================

bool cffDProjectFile::LoadFromXML(QDomElement *ParentElement,QString,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes,bool IsPartial) 
{
   InitDefaultValues();
   bool IsOk=false;
   if ((ParentElement->elementsByTagName("ffDiaporamaProjectProperties").length()>0)&&(ParentElement->elementsByTagName("ffDiaporamaProjectProperties").item(0).isElement()==true)) 
   {
      QDomElement Element=ParentElement->elementsByTagName("ffDiaporamaProjectProperties").item(0).toElement();
      if (Element.hasAttribute("Title"))              title=Element.attribute("Title");
      if (Element.hasAttribute("Author"))             author=Element.attribute("Author");
      if (Element.hasAttribute("Album"))              album=Element.attribute("Album");
      if (Element.hasAttribute("EventDate"))          eventDate=eventDate.fromString(Element.attribute("EventDate"),Qt::ISODate);
      else if (Element.hasAttribute("Year"))      eventDate.setDate(Element.attribute("Year").toInt(),1,1);
      if (Element.hasAttribute("OverrideDate"))       overrideDate=Element.attribute("OverrideDate")=="1";
      if (!overrideDate)                              longDate=FormatLongDate(eventDate);
      else if (Element.hasAttribute("LongDate"))  longDate=Element.attribute("LongDate");
      if (Element.hasAttribute("Comment"))            comment=Element.attribute("Comment");
      if (Element.hasAttribute("ffDRevision"))        ffDRevision=Element.attribute("ffDRevision");
      if (Element.hasAttribute("Composer"))           composer=Element.attribute("Composer");
      if (Element.hasAttribute("DefaultLanguage"))    defaultLanguage=Element.attribute("DefaultLanguage");
      if (Element.hasAttribute("Duration"))           SetRealDuration(QTime(0,0,0,0).addMSecs(Element.attribute("Duration").toLongLong()));

      if (Element.hasAttribute("ChaptersNumber")) {
         nbrChapters=Element.attribute("ChaptersNumber").toInt();
         for (int i=0;i<nbrChapters;i++) {
            QString     ChapterNum=QString("%1").arg(i); while (ChapterNum.length()<3) ChapterNum="0"+ChapterNum;
            if ((ParentElement->elementsByTagName("Chapter_"+ChapterNum).length()>0)&&(ParentElement->elementsByTagName("Chapter_"+ChapterNum).item(0).isElement()==true)) {
               QDomElement SubElement=ParentElement->elementsByTagName("Chapter_"+ChapterNum).item(0).toElement();
               QString Start = "";
               QString End = "";
               QString Duration = "";
               QString Title = "";
               QString InSlide = "";
               if (SubElement.hasAttribute("Start"))       Start=SubElement.attribute("Start");
               if (SubElement.hasAttribute("End"))         End=SubElement.attribute("End");
               if (SubElement.hasAttribute("Duration"))    Duration=SubElement.attribute("Duration");
               if (SubElement.hasAttribute("title"))       Title=SubElement.attribute("title");
               if (SubElement.hasAttribute("InSlide"))     InSlide=SubElement.attribute("InSlide");

               chaptersProperties.append("Chapter_"+ChapterNum+":Start"   +QString("##")+Start);
               chaptersProperties.append("Chapter_"+ChapterNum+":End"     +QString("##")+End);
               chaptersProperties.append("Chapter_"+ChapterNum+":Duration"+QString("##")+Duration);
               chaptersProperties.append("Chapter_"+ChapterNum+":title"   +QString("##")+Title);
               chaptersProperties.append("Chapter_"+ChapterNum+":InSlide" +QString("##")+InSlide);
            }
         }
      }
      if ((!IsPartial)&&(Element.elementsByTagName("PrjLocation").length()>0)&&(Element.elementsByTagName("PrjLocation").item(0).isElement()==true)) {
         QDomElement SubElement=Element.elementsByTagName("PrjLocation").item(0).toElement();
         if (location) 
            delete location;
         location = new cLocation(ApplicationConfig);
         location->LoadFromXML(&SubElement,"",PathForRelativPath,AliasList,ModifyFlag,ResKeyList,DuplicateRes);
      }
      IsOk=true;
   }
   if ((ParentElement->elementsByTagName("Project").length()>0)&&(ParentElement->elementsByTagName("Project").item(0).isElement()==true)) {
      QDomElement Element=ParentElement->elementsByTagName("Project").item(0).toElement();
      if (Element.hasAttribute("ImageGeometry")) {
         switch (Element.attribute("ImageGeometry").toInt()) 
         {
         case GEOMETRY_16_9:  geometry = IMAGE_GEOMETRY_16_9;   break;
         case GEOMETRY_40_17: geometry = IMAGE_GEOMETRY_40_17;  break;
         case GEOMETRY_4_3:              
         default:             geometry = IMAGE_GEOMETRY_4_3;    break;
         }
      }
      if (Element.hasAttribute("ObjectNumber"))
         nbrSlide=Element.attribute("ObjectNumber").toInt();
   }
   return IsOk;
}

void cffDProjectFile::clearLocation()
{ 
   if( location) 
      delete location; 
   location = NULL; 
}

cLocation *cffDProjectFile::createLocation()
{ 
   if( !location) 
      location = new cLocation(ApplicationConfig); 
   return location; 
}

//====================================================================================================================

bool cffDProjectFile::GetChildFullInformationFromFile(bool IsPartial,cCustomIcon *Icon,QStringList *) 
{
   Icon->LoadIcons(&ApplicationConfig->DefaultFFDIcon);

   QFile           file(FileName());
   QDomDocument    domDocument;
   QDomElement     root;
   QString         errorStr;
   int             errorLine, errorColumn;
   if (file.open(QFile::ReadOnly | QFile::Text))
   {
      QTextStream InStream(&file);
      QString     ffDPart;
      bool        EndffDPart = false;
#if QT_VERSION < 0x060000
      InStream.setCodec("UTF-8");
#endif
      while (!InStream.atEnd())
      {
         QString Line = InStream.readLine();
         if (!EndffDPart)
         {
            ffDPart.append(Line);
            if (Line == "</Project>")
               EndffDPart = true;
         }
      }
      file.close();
      // Now import ffDPart
      if (domDocument.setContent(ffDPart, true, &errorStr, &errorLine, &errorColumn))
      {
         root = domDocument.documentElement();
         // Load project properties
         if (root.tagName() == FFD_APPLICATION_ROOTNAME)
            LoadFromXML(&root, "", QFileInfo(file.fileName()).absolutePath(), NULL, NULL, NULL, false, IsPartial);
      }
      file.close();
   }
   return true;
}

//====================================================================================================================

QString cffDProjectFile::GetTechInfo(QStringList *) 
{
   QString Info = "";
   if (composer != "")   
      Info = Info+(Info!=""?" - ":"")+composer+" ("+ffDRevision+")";
   if (GetImageSizeStr(cBaseMediaFile::GEOONLY) != "")   
      Info=Info+(Info!=""?" - ":"")+GetImageSizeStr(cBaseMediaFile::GEOONLY);
   if (nbrSlide > 0)                                     
      Info = Info+(Info!=""?" - ":"")+QString("%1").arg(nbrSlide)   +" "+QApplication::translate("cBaseMediaFile","Slides");
   if (nbrChapters > 0)                                  
      Info = Info+(Info!=""?" - ":"")+QString("%1").arg(nbrChapters)+" "+QApplication::translate("cBaseMediaFile","Chapters");
   return Info;
}

//====================================================================================================================

QString cffDProjectFile::GetTAGInfo(QStringList *) 
{
   QString Info=title;
   if (longDate!="")       
      Info = Info + (Info!=""?" - ":"")+longDate; 
   else 
      Info = Info+(Info!=""?" - ":"")+eventDate.toString(ApplicationConfig->ShortDateFormat);
   if (album != "")          
      Info = Info+(Info!=""?" - ":"")+album;
   if (author != "")
      Info = Info+(Info!=""?" - ":"")+author;
   return Info;
}

//====================================================================================================================

QString cffDProjectFile::GetFileTypeStr() 
{
    return QApplication::translate("cBaseMediaFile","ffDiaporama","File type");
}

//*********************************************************************************************************************************************
// Image file
//*********************************************************************************************************************************************

cImageFile::cImageFile(cApplicationConfig *ApplicationConfig):cBaseMediaFile(ApplicationConfig) 
{
   ObjectType = OBJECTTYPE_IMAGEFILE;  // coud be turn later to OBJECTTYPE_THUMBNAIL
   NoExifData = false;
}

//====================================================================================================================

cImageFile::~cImageFile() 
{
}

//====================================================================================================================

QString cImageFile::GetFileTypeStr() 
{
   if (ObjectType == OBJECTTYPE_IMAGEFILE)         return QApplication::translate("cBaseMediaFile", "Image", "File type");
   else if (ObjectType == OBJECTTYPE_IMAGEVECTOR)  return QApplication::translate("cBaseMediaFile", "Vector image", "File type");
   else                                            return QApplication::translate("cBaseMediaFile", "Thumbnail", "File type");
}

//====================================================================================================================

bool cImageFile::LoadBasicInformationFromDatabase(QDomElement *ParentElement,QString,QString,QStringList *,bool *,TResKeyList *,bool) 
{
   imageWidth = ParentElement->attribute("ImageWidth").toInt();
   imageHeight = ParentElement->attribute("ImageHeight").toInt();
   imageOrientation = ParentElement->attribute("ImageOrientation").toInt();
   geometry = ParentElement->attribute("ObjectGeometry").toInt();
   aspectRatio = GetDoubleValue(*ParentElement, "AspectRatio");
   return true;
}

//====================================================================================================================

void cImageFile::SaveBasicInformationToDatabase(QDomElement *ParentElement,QString,QString,bool,cReplaceObjectList *,QList<qlonglong> *,bool) 
{
   ParentElement->setAttribute("ImageWidth", imageWidth);
   ParentElement->setAttribute("ImageHeight", imageHeight);
   ParentElement->setAttribute("ImageOrientation", imageOrientation);
   ParentElement->setAttribute("ObjectGeometry", geometry);
   ParentElement->setAttribute("AspectRatio", QString("%1").arg(aspectRatio, 0, 'f'));
}

//====================================================================================================================

bool cImageFile::CheckFormatValide(QWidget *Window) 
{
   bool IsOk = GetFullInformationFromFile();

   // Try to load an image to ensure all is ok
   if (IsOk)
   {
      QImage *Image = ImageAt(true);
      if (Image)
      {
         delete Image;
      }
      else
      {
         QString ErrorMessage = ErrorMessage + "\n" + QApplication::translate("MainWindow", "Impossible to read an image from the file", "Error message");
         CustomMessageBox(Window, QMessageBox::Critical, QApplication::translate("MainWindow", "Error", "Error message"), ShortName() + "\n\n" + ErrorMessage, QMessageBox::Close);
         IsOk = false;
      }
   }
   return IsOk;
}

//====================================================================================================================

bool cImageFile::GetInformationFromFile(QString& FileName,QStringList *AliasList,bool *ModifyFlag,qlonglong GivenFolderKey) 
{
   if (QFileInfo(FileName).suffix().toLower() == "svg")
      ObjectType = OBJECTTYPE_IMAGEVECTOR;
   return cBaseMediaFile::GetInformationFromFile(FileName, AliasList, ModifyFlag, GivenFolderKey);
}

//====================================================================================================================

bool cImageFile::GetChildFullInformationFromFile(bool,cCustomIcon *Icon,QStringList *ExtendedProperties) 
{
   imageOrientation    = -1;
   bool ExifOk = false;

   if (ObjectType == OBJECTTYPE_IMAGEVECTOR) 
   {
      // Vector image file
      QSvgRenderer SVGImg(FileName());
      if (SVGImg.isValid()) 
      {
         imageOrientation=0;
         imageWidth      =SVGImg.viewBox().width();
         imageHeight     =SVGImg.viewBox().height();

         QPainter Painter;
         QImage   Img;
         qreal    RatioX=(imageWidth > imageHeight ? 1 : qreal(imageWidth)/qreal(imageHeight));
         qreal    RatioY=(imageWidth < imageHeight ? 1 : qreal(imageHeight)/qreal(imageWidth));

         // 16x16 icon
         Img = QImage(qreal(16)*RatioX,qreal(16)*RatioY,QImage::Format_ARGB32);
         Img.fill(0);
         Painter.begin(&Img);
         SVGImg.render(&Painter);
         Painter.end();
         Icon->Icon16 = QImage(16,16,QImage::Format_ARGB32_Premultiplied);
         Icon->Icon16.fill(0);
         Painter.begin(&Icon->Icon16);
         Painter.drawImage(QPoint((16-Img.width())/2,(16-Img.height())/2),Img);
         Painter.end();

         // 100x100 icon
         Img = QImage(qreal(100)*RatioX,qreal(100)*RatioY,QImage::Format_ARGB32);
         Img.fill(0);
         Painter.begin(&Img);
         SVGImg.render(&Painter);
         Painter.end();
         Icon->Icon100 = QImage(100,100,QImage::Format_ARGB32_Premultiplied);
         Icon->Icon100.fill(0);
         Painter.begin(&Icon->Icon100);
         Painter.drawImage(QPoint((100-Img.width())/2,(100-Img.height())/2),Img);
         Painter.end();

         ExtendedProperties->append(QString("Photo.PixelXDimension")+QString("##")+QString("%1").arg(imageWidth));
         ExtendedProperties->append(QString("Photo.PixelYDimension")+QString("##")+QString("%1").arg(imageHeight));
      }

   } 
   else if (NoExifData) 
   {
   } 
   else if (!NoExifData) 
   {
      // ******************************************************************************************************
      // Try to load EXIF information using library exiv2
      // ******************************************************************************************************
      Exiv2::Image::UniquePtr ImageFile;
      try 
      {
#ifdef Q_OS_WIN
         ImageFile = Exiv2::ImageFactory::open(FileName().toLocal8Bit().data());
#else
         ImageFile = Exiv2::ImageFactory::open(FileName().toUtf8().data());
#endif
         ExifOk = true;
      }
      catch( Exiv2::Error& ) 
      {
         //ToLog(LOGMSG_INFORMATION,QApplication::translate("cBaseMediaFile","Image don't have EXIF metadata %1").arg(FileName));
         NoExifData = true;
      }
      catch (...)
      {
         //ToLog(LOGMSG_INFORMATION,QApplication::translate("cBaseMediaFile","Image don't have EXIF metadata %1").arg(FileName));
         NoExifData = true;
      }

      if ( ExifOk && ImageFile->good()) 
      {
         ImageFile->readMetadata();
         // Read data
         Exiv2::ExifData &exifData = ImageFile->exifData();
         if (!exifData.empty()) 
         {
            Exiv2::ExifData::const_iterator end = exifData.end();
            for (Exiv2::ExifData::const_iterator CurrentData = exifData.begin(); CurrentData != end; ++CurrentData) 
            {
               if (QString().fromStdString(CurrentData->key()) == "Exif.Image.Orientation" && CurrentData->tag() == 274)
                  imageOrientation = QString().fromStdString(CurrentData->value().toString()).toInt();

               if (CurrentData->typeId() != Exiv2::undefined &&
                  !( (CurrentData->typeId()==Exiv2::unsignedByte || CurrentData->typeId() == Exiv2::signedByte) && CurrentData->size()>64)) 
               {
                  QString Key = QString().fromStdString(CurrentData->key());
#ifdef Q_OS_WIN
                  QString Value = QString().fromStdString(CurrentData->print(&exifData).c_str());
#else
                  QString Value = QString().fromUtf8(CurrentData->print(&exifData).c_str());
#endif
                  if (Key.startsWith("Exif.")) 
                     Key = Key.mid(QString("Exif.").length());
                  ExtendedProperties->append(Key+QString("##")+Value);
               }
            }
         }

         // Append ExtendedProperties
         if (GetInformationValue("Image.Artist",ExtendedProperties) != "") 
            ExtendedProperties->append(QString("artist")+QString("##")+GetInformationValue("Image.Artist",ExtendedProperties));
         if (GetInformationValue("Image.Model",ExtendedProperties)!="")  
         {
            if (GetInformationValue("Image.Model",ExtendedProperties).contains(GetInformationValue("Image.Make",ExtendedProperties),Qt::CaseInsensitive)) 
               ExtendedProperties->append(QString("composer")+QString("##")+GetInformationValue("Image.Model",ExtendedProperties));
            else 
               ExtendedProperties->append(QString("composer")+QString("##")+GetInformationValue("Image.Make",ExtendedProperties)+" "+GetInformationValue("Image.Model",ExtendedProperties));
         }
         // Get size information
         imageWidth = ImageFile->pixelWidth();
         imageHeight = ImageFile->pixelHeight();

         //if (GetInformationValue("Photo.PixelXDimension")!="")       ImageWidth =GetInformationValue("Photo.PixelXDimension").toInt();
         //    else if (GetInformationValue("Image.ImageWidth")!="")   ImageWidth =GetInformationValue("Image.ImageWidth").toInt();            // TIFF Version
         //if (GetInformationValue("Photo.PixelYDimension")!="")       ImageHeight=GetInformationValue("Photo.PixelYDimension").toInt();
         //    else if (GetInformationValue("Image.ImageLength")!="")  ImageHeight=GetInformationValue("Image.ImageLength").toInt();           // TIFF Version

         // switch ImageWidth and ImageHeight if image was rotated
         #if QT_VERSION < 0x050400 || QT_VERSION > 0x050401
         if (imageOrientation == 6 || imageOrientation == 8) 
         {
            int IW = imageWidth;
            imageWidth = imageHeight;
            imageHeight = IW;
         }
         #endif

         // Read preview image
         if (Exiv2WithPreview && (Icon->Icon16.isNull() || Icon->Icon100.isNull())) 
         {
            Exiv2::PreviewManager *Manager = new Exiv2::PreviewManager(*ImageFile);
            if (Manager) 
            {
               Exiv2::PreviewPropertiesList Properties=Manager->getPreviewProperties();
               if (!Properties.empty()) 
               {
                  Exiv2::PreviewImage Image = Manager->getPreviewImage(Properties[Properties.size()-1]);      // Get the latest image (biggest)
                  QImage *IconImage = new QImage();
                  if (IconImage->loadFromData(QByteArray((const char*)Image.pData(),Image.size()))) 
                  {
                     #if QT_VERSION < 0x050400 || QT_VERSION > 0x050401
                     int rotation = 0;
                     switch (imageOrientation)
                     {
                        case 8: rotation = -90; break;
                        case 3: rotation = 180; break;
                        case 6: rotation = 90; break;
                     }
                     if (rotation != 0)
                     {
                        QTransform transform;
                        transform.rotate(rotation);
                        QImage* NewImage = new QImage(IconImage->transformed(transform, Qt::SmoothTransformation));
                        delete IconImage;
                        IconImage = NewImage;
                     }
                     #endif
                     // Sometimes, Icon have black bar : try to remove them
                     if ((double(IconImage->width())/double(IconImage->height())) != (double(imageWidth)/double(imageHeight))) 
                     {
                        if (imageWidth > imageHeight) 
                        {
                           int RealHeight = int((double(IconImage->width())*double(imageHeight))/double(imageWidth));
                           int Delta      = IconImage->height()-RealHeight;
                           QImage *NewImage = new QImage(IconImage->copy(0,Delta/2,IconImage->width(),IconImage->height()-Delta));
                           delete IconImage;
                           IconImage = NewImage;
                           // if preview Icon have a really small size, then don't use it
                           if (IconImage->width() >= MinimumEXIFHeight) 
                              Icon->LoadIcons(IconImage);
                        } 
                        else 
                        {
                           int RealWidth = int((double(IconImage->height())*double(imageWidth))/double(imageHeight));
                           int Delta     = IconImage->width()-RealWidth;
                           QImage *NewImage = new QImage(IconImage->copy(Delta/2,0,IconImage->width()-Delta,IconImage->height()));
                           delete IconImage;
                           IconImage = NewImage;
                           // if preview Icon have a really small size, then don't use it
                           if (IconImage->height() >= MinimumEXIFHeight) 
                              Icon->LoadIcons(IconImage);
                        }
                     }
                  }
                  delete IconImage;
               }
               delete Manager;
            }
         }
      }

      //************************************************************************************
      // If no exif preview image (or image too small) then load/create thumbnail
      //************************************************************************************
      if (Icon->Icon16.isNull() || Icon->Icon100.isNull()) 
      {
         cLuLoImageCacheObject *ImageObject = ApplicationConfig->ImagesCache.FindObject(ressourceKey,fileKey,modifDateTime,imageOrientation,ApplicationConfig->Smoothing,true);
         if (ImageObject == NULL) 
         {
            ToLog(LOGMSG_CRITICAL,"Error in cImageFile::GetFullInformationFromFile : FindObject return NULL for thumbnail creation !");
         } 
         else 
         {
            QImageReader ImgReader(FileName());
            if (ImgReader.canRead()) 
            {
               QSize Size = ImgReader.size();
               if (Size.width() >= 100 || Size.height() >= 100) 
               {
                  if ((qreal(Size.height())/qreal(Size.width()))*100 <= 100) 
                  {
                     Size.setHeight((qreal(Size.height())/qreal(Size.width()))*100);
                     Size.setWidth(100);
                  } 
                  else 
                  {
                     Size.setWidth((qreal(Size.width())/qreal(Size.height()))*100);
                     Size.setHeight(100);
                  }
                  ImgReader.setScaledSize(Size);
               }
               QImage Image = ImgReader.read();
               int rotation = 0;
               switch (imageOrientation)
               {
                  case 8: rotation = -90; break;   // Rotating image anti-clockwise by 90 degrees...'
                  case 3: rotation = 180; break;   // Rotating image clockwise by 180 degrees...'
                  case 6: rotation = 90; break;    // Rotating image clockwise by 90 degrees...'
               }
               if (rotation != 0)
               {
                  QTransform transform;
                  transform.rotate(rotation);
                  Image = Image.transformed(transform, Qt::SmoothTransformation);
               }
               if (Image.isNull()) 
                  ToLog(LOGMSG_CRITICAL,"QImageReader.read return error in GetFullInformationFromFile");
               else 
                  Icon->LoadIcons(&Image);
            }
         }
      }

      //************************************************************************************
      // if no information about size then load image
      //************************************************************************************
      if (imageWidth == 0 || imageHeight == 0) 
      {
         cLuLoImageCacheObject *ImageObject=ApplicationConfig->ImagesCache.FindObject(ressourceKey,fileKey,modifDateTime,imageOrientation,ApplicationConfig->Smoothing,true);
         if (ImageObject==NULL) 
         {
            ToLog(LOGMSG_CRITICAL,"Error in cImageFile::GetFullInformationFromFile : FindObject return NULL for size computation !");
         } 
         else 
         {
            QImageReader Img(FileName());
            if (Img.canRead()) 
            {
               QSize Size = Img.size();
               imageWidth = Size.width();
               imageHeight= Size.height();
               ExtendedProperties->append(QString("Photo.PixelXDimension")+QString("##")+QString("%1").arg(imageWidth));
               ExtendedProperties->append(QString("Photo.PixelYDimension")+QString("##")+QString("%1").arg(imageHeight));
            }
         }
      }
   }

   //************************************************************************************
   // End process by computing some values ....
   //************************************************************************************

   // Sort ExtendedProperties
   ExtendedProperties->sort();

   // Now we have image size then compute image geometry
   geometry = geometryFromSize(imageWidth,imageHeight);

   // if Icon16 stil null then load default icon
   if (Icon->Icon16.isNull() || Icon->Icon100.isNull()) 
      Icon->LoadIcons(&ApplicationConfig->DefaultIMAGEIcon);
   return true;
}

//====================================================================================================================

QString cImageFile::GetTechInfo(QStringList *ExtendedProperties) 
{
    QString Info=GetImageSizeStr(FULLWEB);
    if (GetInformationValue("artist",ExtendedProperties)!="")              Info=Info+(Info!=""?"-":"")+GetInformationValue("artist",ExtendedProperties);
    if (GetInformationValue("composer",ExtendedProperties)!="")            Info=Info+(Info!=""?"-":"")+GetInformationValue("composer",ExtendedProperties);
    if (GetInformationValue("Image.Orientation",ExtendedProperties)!="")   Info=Info+(Info!=""?"-":"")+GetInformationValue("Image.Orientation",ExtendedProperties);
    return Info;
}

//====================================================================================================================

QString cImageFile::GetTAGInfo(QStringList *ExtendedProperties) 
{
    QString Info=GetInformationValue("Photo.ExposureTime",ExtendedProperties);
    if (GetInformationValue("Photo.ApertureValue",ExtendedProperties)!="")    Info=Info+(Info!=""?"-":"")+GetInformationValue("Photo.ApertureValue",ExtendedProperties);
    if (GetInformationValue("Photo.ISOSpeedRatings",ExtendedProperties)!="")  Info=Info+(Info!=""?"-":"")+GetInformationValue("Photo.ISOSpeedRatings",ExtendedProperties)+" ISO";
    if (GetInformationValue("CanonCs.LensType",ExtendedProperties)!="")       Info=Info+(Info!=""?"-":"")+GetInformationValue("CanonCs.LensType",ExtendedProperties);                // Canon version
    if (GetInformationValue("NikonLd3.LensIDNumber",ExtendedProperties)!="")  Info=Info+(Info!=""?"-":"")+GetInformationValue("NikonLd3.LensIDNumber",ExtendedProperties);           // Nikon version
    if (GetInformationValue("Photo.Flash",ExtendedProperties)!="")            Info=Info+(Info!=""?"-":"")+GetInformationValue("Photo.Flash",ExtendedProperties);
    if (GetInformationValue("CanonCs.FlashMode",ExtendedProperties)!="")      Info=Info+(Info!=""?"-":"")+GetInformationValue("CanonCs.FlashMode",ExtendedProperties);               // Canon version
    if (GetInformationValue("Nikon3.FlashMode",ExtendedProperties)!="")       Info=Info+(Info!=""?"-":"")+GetInformationValue("Nikon3.FlashMode",ExtendedProperties);                // Nikon version
    return Info;
}

//====================================================================================================================

QImage *cImageFile::ImageAt(bool PreviewMode) 
{
   if (!IsValide)            
      return NULL;

   QMutexLocker locker(&accessMutex);
   QImage *RetImage=NULL;
   if (ObjectType == OBJECTTYPE_IMAGEVECTOR) 
   {
      // Vector image file
      QSvgRenderer SVGImg(FileName());
      if (SVGImg.isValid()) 
      {
         if (imageWidth == 0 || imageHeight == 0) 
         {
            imageWidth  = SVGImg.defaultSize().width();
            imageHeight = SVGImg.defaultSize().height();
         }
         RetImage = new QImage(imageWidth,imageHeight,QImage::Format_ARGB32_Premultiplied);
         RetImage->fill(0);
         QPainter Painter;
         Painter.begin(RetImage);
         Painter.setClipping(true);
         Painter.setClipRect(QRect(0,0,RetImage->width(),RetImage->height()));
         SVGImg.render(&Painter,QRectF(0,0,RetImage->width(),RetImage->height()));
         Painter.end();
      }
   } 
   else 
   {
      cLuLoImageCacheObject *ImageObject = ApplicationConfig->ImagesCache.FindObject(ressourceKey,fileKey,modifDateTime,imageOrientation,(!PreviewMode || ApplicationConfig->Smoothing),true);

      if (!ImageObject) 
      {
         ToLog(LOGMSG_CRITICAL,"Error in cImageFile::ImageAt : FindObject return NULL !");
         return NULL;  // There is an error !!!!!
      }
      if (PreviewMode) 
         RetImage = ImageObject->ValidateCachePreviewImage();
      else         
         RetImage = ImageObject->ValidateCacheRenderImage();
      if (RetImage == NULL) 
         ToLog(LOGMSG_CRITICAL,"Error in cImageFile::ImageAt : ValidateCacheImage return NULL !");
   }
   // return wanted image
   return RetImage;
}

QImage  *cImageFile::ImageAt(bool PreviewMode, QSizeF Size) 
{
   Q_UNUSED(PreviewMode)
   //qDebug() << "ImageAt " << Size << " from " << imageWidth << " " << imageHeight << " scaleX is " << Size.width()/ imageWidth << " scaleY is " << Size.height()/imageHeight;
   QMutexLocker locker(&accessMutex);
   QImage *RetImage = NULL;
   if (ObjectType == OBJECTTYPE_IMAGEVECTOR)
   {
      // Vector image file
      QSvgRenderer SVGImg(FileName());
      if (SVGImg.isValid())
      {
         if (imageWidth == 0 || imageHeight == 0)
         {
            imageWidth = SVGImg.defaultSize().width();
            imageHeight = SVGImg.defaultSize().height();
         }
         RetImage = new QImage(Size.toSize() /*imageWidth, imageHeight*/, QImage::Format_ARGB32_Premultiplied);
         RetImage->fill(0);
         QPainter Painter;
         Painter.begin(RetImage);
         Painter.setClipping(true);
         Painter.setClipRect(QRect(0, 0, RetImage->width(), RetImage->height()));
         SVGImg.render(&Painter, QRectF(0, 0, RetImage->width(), RetImage->height()));
         Painter.end();
      }
   }
   // return wanted image
   return RetImage;
}

//*********************************************************************************************************************************************
// Image from clipboard
//*********************************************************************************************************************************************

cImageClipboard::cImageClipboard(cApplicationConfig *ApplicationConfig):cImageFile(ApplicationConfig) 
{
   ObjectType       = OBJECTTYPE_IMAGECLIPBOARD;
   ObjectName       = "ImageClipboard";
   NoExifData       = true;
   imageOrientation = 0;
   creatDateTime = QDateTime().currentDateTime();
}

//====================================================================================================================

cImageClipboard::~cImageClipboard() 
{
}

//====================================================================================================================

bool cImageClipboard::LoadBasicInformationFromDatabase(QDomElement *ParentElement,QString,QString,QStringList *,bool *,TResKeyList *,bool) 
{
    imageWidth      =ParentElement->attribute("ImageWidth").toInt();
    imageHeight     =ParentElement->attribute("ImageHeight").toInt();
    imageOrientation=ParentElement->attribute("ImageOrientation").toInt();
    geometry  =ParentElement->attribute("ObjectGeometry").toInt();
    aspectRatio     =GetDoubleValue(*ParentElement,"AspectRatio");
    creatDateTime.fromString(ParentElement->attribute("CreatDateTime"),Qt::ISODate);
    return true;
}

//====================================================================================================================

void cImageClipboard::SaveBasicInformationToDatabase(QDomElement *ParentElement,QString,QString,bool,cReplaceObjectList *,QList<qlonglong> *,bool) 
{
    ParentElement->setAttribute("ImageWidth",        imageWidth);
    ParentElement->setAttribute("ImageHeight",       imageHeight);
    ParentElement->setAttribute("ImageOrientation",  imageOrientation);
    ParentElement->setAttribute("ObjectGeometry",    geometry);
    ParentElement->setAttribute("AspectRatio",       QString("%1").arg(aspectRatio,0,'f'));
    ParentElement->setAttribute("CreatDateTime",     creatDateTime.toString(Qt::ISODate));
}

void cImageClipboard::SaveBasicInformationToDatabase(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel)
{
   Q_UNUSED(IsModel)
   Q_UNUSED(ResKeyList)
   Q_UNUSED(ReplaceList)
   xmlStream.writeAttribute("ImageWidth", imageWidth);
   xmlStream.writeAttribute("ImageHeight", imageHeight);
   xmlStream.writeAttribute("ImageOrientation", imageOrientation);
   xmlStream.writeAttribute("ObjectGeometry", geometry);
   xmlStream.writeAttribute("AspectRatio", QString("%1").arg(aspectRatio, 0, 'f'));
   xmlStream.writeAttribute("CreatDateTime", creatDateTime.toString(Qt::ISODate));
}


//====================================================================================================================

bool cImageClipboard::GetInformationFromFile(QString&,QStringList *,bool *,qlonglong) 
{
    QImage ImageClipboard;
    ApplicationConfig->SlideThumbsTable->GetThumb(&ressourceKey,&ImageClipboard);
    if (!ImageClipboard.isNull()) 
    {
        imageWidth=ImageClipboard.width();
        imageHeight=ImageClipboard.height();
        imageOrientation=0;
        aspectRatio = double(imageHeight)/double(imageWidth);
        // Now we have image size then compute image geometry
        geometry = geometryFromSize(imageWidth,imageHeight);
        IsValide=true;
    } 
    else 
       IsValide=false;
    return IsValide;
}

//====================================================================================================================

bool cImageClipboard::GetChildFullInformationFromFile(bool,cCustomIcon *Icon,QStringList *ExtendedProperties) 
{
    if (Icon) {
        if (Icon->Icon16.isNull() || Icon->Icon100.isNull()) Icon->LoadIcons(&ApplicationConfig->DefaultIMAGEIcon);
    }
    if (ExtendedProperties) {
        ExtendedProperties->append(QString("Photo.PixelXDimension")+QString("##")+QString("%1").arg(imageWidth));
        ExtendedProperties->append(QString("Photo.PixelYDimension")+QString("##")+QString("%1").arg(imageHeight));
    }
    return true;
}

//====================================================================================================================

QStringList cImageClipboard::GetSummaryText(QStringList *) 
{
    QStringList SummaryText;
    SummaryText.append(GetFileTypeStr());
    SummaryText.append(GetImageSizeStr(cBaseMediaFile::FULLWEB));
    SummaryText.append("");
    return SummaryText;
}

//====================================================================================================================

bool cImageClipboard::LoadFromXML(QDomElement *ParentElement, QString ElementName, QString PathForRelativPath, QStringList *AliasList,
                                 bool *ModifyFlag, TResKeyList *ResKeyList, bool DuplicateRes) 
{
   if (DuplicateRes && ObjectType == OBJECTTYPE_IMAGECLIPBOARD) 
      DuplicateRes = false;    // Never duplicate an image clipboard (but allow it for child)
   if (ParentElement->elementsByTagName(ObjectName).length() > 0 && ParentElement->elementsByTagName(ObjectName).item(0).isElement() == true) 
   {
      QDomElement SubElement = ParentElement->elementsByTagName(ObjectName).item(0).toElement();
      if (LoadBasicInformationFromDatabase(&SubElement,ElementName,PathForRelativPath,AliasList,ModifyFlag,ResKeyList,DuplicateRes)) 
      {
         if (ResKeyList) 
         {
            ressourceKey = SubElement.attribute("RessourceKey").toLongLong();
            for (int ResNum = 0; ResNum < ResKeyList->count(); ResNum++) 
               if (ressourceKey == ResKeyList->at(ResNum).OrigKey) 
               {
                  ressourceKey = ResKeyList->at(ResNum).NewKey;
                  break;
               }
         } 
         else 
            ressourceKey = SubElement.attribute("RessourceKey").toLongLong();
         // if DuplicateRes (for exemple during a paste operation)
         if (DuplicateRes && ressourceKey != -1) 
         {
            QImage Image;
            ApplicationConfig->SlideThumbsTable->GetThumb(&ressourceKey,&Image);
            ressourceKey = -1;
            ApplicationConfig->SlideThumbsTable->SetThumb(&ressourceKey,Image);
         }
         return true;
      } 
      else 
         return false;
   } 
   return false;
}

//====================================================================================================================

void cImageClipboard::SaveToXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel) 
{
   QDomDocument DomDocument;
   QString xmlFragment;
   ExXmlStreamWriter xlmStream(&xmlFragment);
   SaveToXMLex(xlmStream, ElementName, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
   DomDocument.setContent(xmlFragment);
   ParentElement->appendChild(DomDocument.firstChildElement());
   return;
   /*
   QDomDocument DomDocument;
   QDomElement  SubElement=DomDocument.createElement(ObjectName);
   SaveBasicInformationToDatabase(&SubElement,ElementName,PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,IsModel);
   SubElement.setAttribute("RessourceKey",ressourceKey);
   if (ResKeyList) 
   {
      if( !ResKeyList->contains(ressourceKey)) 
         ResKeyList->append(ressourceKey);

   }
   ParentElement->appendChild(SubElement);
   */
}

void cImageClipboard::SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel)
{
   //QDomDocument DomDocument;
   //QDomElement  SubElement = DomDocument.createElement(ObjectName);
   xmlStream.writeStartElement(ObjectName);
   xmlStream.writeAttribute("RessourceKey", ressourceKey);
   SaveBasicInformationToDatabase(xmlStream, ElementName, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
   if (ResKeyList)
   {
      if (!ResKeyList->contains(ressourceKey))
         ResKeyList->append(ressourceKey);

   }
   xmlStream.writeEndElement();
   //ParentElement->appendChild(SubElement);
}
//*********************************************************************************************************************************************
// Google maps map
//*********************************************************************************************************************************************

cGMapsMap::cGMapsMap(cApplicationConfig *ApplicationConfig):cImageClipboard(ApplicationConfig) 
{
   ObjectType  = OBJECTTYPE_GMAPSMAP;
   ObjectName  = "GoogleMapsMap";
   NoExifData  = true;
   MapType     = Hybrid;
   ImageSize   = Small;
   ZoomLevel   = 13;
   Scale       = 1;
   IsMapValide = false;
   IsValide    = true;
   creatDateTime = QDateTime().currentDateTime();
}

//====================================================================================================================

cGMapsMap::~cGMapsMap() 
{
   while (locations.count()) 
      delete locations.takeLast();
}

//====================================================================================================================

void cGMapsMap::SaveToXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel) 
{
   QDomDocument DomDocument;
   QString xmlFragment;
   ExXmlStreamWriter xlmStream(&xmlFragment);
   SaveToXMLex(xlmStream, ElementName, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
   DomDocument.setContent(xmlFragment);
   ParentElement->appendChild(DomDocument.firstChildElement());
   return;
   /*
   QDomDocument DomDocument;
   QDomElement  SubElement = DomDocument.createElement(ObjectName);
   bool         HaveVarLoc = false;

   // if model, then check if map depends on projet or chapter location
   if (IsModel)
   {
      for (int i = 0; i < locations.count(); i++)
         if (locations[i]->LocationType != cLocation::FREE)
            HaveVarLoc = true;
   }
   SaveBasicInformationToDatabase(&SubElement, ElementName, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);

   if (!IsModel || (IsModel && !HaveVarLoc))
   {
      SubElement.setAttribute("RessourceKey", ressourceKey);
      if (ResKeyList)
      {
         // Check if RessourceKey is already in the ResKeyList
         bool ToAppend = true;
         for (int i = 0; i < ResKeyList->count(); i++)
            if (ResKeyList->at(i) == ressourceKey)
               ToAppend = false;
         // If not found, then add it to the list
         if (ToAppend)
            ResKeyList->append(ressourceKey);
      }
   }
   else
      SubElement.setAttribute("RessourceKey", -1); // Don't save ressource if model and map depends on projet or chapter location
   ParentElement->appendChild(SubElement);
   */
}

void cGMapsMap::SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel)
{
//   QDomDocument DomDocument;
//   QDomElement  SubElement = DomDocument.createElement(ObjectName);
   xmlStream.writeStartElement(ObjectName);
   bool         HaveVarLoc = false;


   // if model, then check if map depends on projet or chapter location
   if (IsModel)
   {
      for (int i = 0; i < locations.count(); i++)
         if (locations[i]->LocationType != cLocation::FREE)
            HaveVarLoc = true;
   }

   if (!IsModel || (IsModel && !HaveVarLoc))
   {
      xmlStream.writeAttribute("RessourceKey", ressourceKey);
      if (ResKeyList)
      {
         // Check if RessourceKey is already in the ResKeyList
         bool ToAppend = true;
         for (int i = 0; i < ResKeyList->count(); i++)
            if (ResKeyList->at(i) == ressourceKey)
               ToAppend = false;
         // If not found, then add it to the list
         if (ToAppend)
            ResKeyList->append(ressourceKey);
      }
   }
   else
      xmlStream.writeAttribute("RessourceKey", -1); // Don't save ressource if model and map depends on projet or chapter location
   SaveBasicInformationToDatabase(xmlStream, ElementName, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
   xmlStream.writeEndElement();
}

//====================================================================================================================

bool cGMapsMap::LoadBasicInformationFromDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes) {
    int i;
    if (cImageClipboard::LoadBasicInformationFromDatabase(ParentElement,ElementName,PathForRelativPath,AliasList,ModifyFlag,ResKeyList,DuplicateRes)) {
        if (ParentElement->hasAttribute("MapType"))   MapType    =(GMapsMapType)ParentElement->attribute("MapType").toInt();
        if (ParentElement->hasAttribute("ImageSize")) ImageSize  =(GMapsImageSize)ParentElement->attribute("ImageSize").toInt();
        if (ParentElement->hasAttribute("ZoomLevel")) ZoomLevel  =ParentElement->attribute("ZoomLevel").toInt();
        if (ParentElement->hasAttribute("Scale"))     Scale      =ParentElement->attribute("Scale").toInt();
        if (ParentElement->hasAttribute("MapCx"))     MapCx      =GetDoubleValue(*ParentElement,"MapCx");
        if (ParentElement->hasAttribute("MapCy"))     MapCy      =GetDoubleValue(*ParentElement,"MapCy");
        if (ParentElement->hasAttribute("MapValide")) IsMapValide=ParentElement->attribute("MapValide")=="1";

        // Loading of locations list
        while (locations.count()) 
         delete locations.takeLast();
        i=0;
        while ((ParentElement->elementsByTagName(QString("Location_%1").arg(i)).length()>0)&&(ParentElement->elementsByTagName(QString("Location_%1").arg(i)).item(0).isElement()==true)) {
            QDomElement SubElement=ParentElement->elementsByTagName(QString("Location_%1").arg(i)).item(0).toElement();
            cLocation *Location=new cLocation(ApplicationConfig);
            Location->LoadFromXML(&SubElement,"",PathForRelativPath,AliasList,ModifyFlag,ResKeyList,DuplicateRes);
            locations.append(Location);
            i++;
        }
        // Loading of pending request sections list
        RequestList.clear();
        i=0;
        while ((ParentElement->elementsByTagName(QString("PendingSection_%1").arg(i)).length()>0)&&(ParentElement->elementsByTagName(QString("PendingSection_%1").arg(i)).item(0).isElement()==true)) {
            QDomElement     SubElement=ParentElement->elementsByTagName(QString("PendingSection_%1").arg(i)).item(0).toElement();
            RequestSection  Item;
            double          X=1,Y=1;
            QString         R;
            if (SubElement.hasAttribute("X")) X=GetDoubleValue(SubElement,"X");
            if (SubElement.hasAttribute("Y")) Y=GetDoubleValue(SubElement,"Y");
            if (SubElement.hasAttribute("R")) R=SubElement.attribute("R");
            Item.Rect=QRectF(X,Y,SectionWith,SectionHeight);
            Item.GoogleRequest=R;
            RequestList.append(Item);
            i++;
        }
        return true;
    } else return false;
}

//====================================================================================================================

void cGMapsMap::SaveBasicInformationToDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel) 
{
    QDomDocument Document;
    cImageClipboard::SaveBasicInformationToDatabase(ParentElement,ElementName,PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,IsModel);

    bool HaveVarLoc=false;

    // if model, then check if map depends on projet or chapter location
    if (IsModel) for (int i=0;i<locations.count();i++) if (locations[i]->LocationType!=cLocation::FREE) HaveVarLoc=true;

    ParentElement->setAttribute("MapValide",(IsMapValide && !HaveVarLoc)?"1":"0");
    ParentElement->setAttribute("MapType",MapType);
    ParentElement->setAttribute("ImageSize",ImageSize);
    ParentElement->setAttribute("ZoomLevel",ZoomLevel);
    ParentElement->setAttribute("Scale",Scale);
    ParentElement->setAttribute("MapCx",MapCx);
    ParentElement->setAttribute("MapCy",MapCy);
    // Saving of locations list
    for (int i=0;i<locations.count();i++) {
        QDomElement SubElement=Document.createElement(QString("Location_%1").arg(i));
        locations[i]->SaveToXML(&SubElement,"",PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,IsModel);
        ParentElement->appendChild(SubElement);
    }
    // Saving of pending request sections list
    for (int i=0;i<RequestList.count();i++) {
        QDomElement SubElement=Document.createElement(QString("PendingSection_%1").arg(i));
        SubElement.setAttribute("X",RequestList[i].Rect.left());
        SubElement.setAttribute("Y",RequestList[i].Rect.top());
        SubElement.setAttribute("R",RequestList[i].GoogleRequest);
        ParentElement->appendChild(SubElement);
    }
}

void cGMapsMap::SaveBasicInformationToDatabase(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel)
{
   cImageClipboard::SaveBasicInformationToDatabase(xmlStream, ElementName, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
   bool HaveVarLoc = false;

   // if model, then check if map depends on projet or chapter location
   if (IsModel)
      for (int i = 0; i < locations.count(); i++)
         if (locations[i]->LocationType != cLocation::FREE)
            HaveVarLoc = true;

   xmlStream.writeAttribute("MapValide", (IsMapValide && !HaveVarLoc) ? "1" : "0");
   xmlStream.writeAttribute("MapType", MapType);
   xmlStream.writeAttribute("ImageSize", ImageSize);
   xmlStream.writeAttribute("ZoomLevel", ZoomLevel);
   xmlStream.writeAttribute("Scale", Scale);
   xmlStream.writeAttribute("MapCx", MapCx);
   xmlStream.writeAttribute("MapCy", MapCy);
   // Saving of locations list
   for (int i = 0; i<locations.count(); i++)
   {
      xmlStream.writeStartElement(QString("Location_%1").arg(i));
      locations[i]->SaveToXMLex(xmlStream, "", PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
      xmlStream.writeEndElement();
   }
   // Saving of pending request sections list
   for (int i = 0; i<RequestList.count(); i++)
   {
      xmlStream.writeStartElement(QString("PendingSection_%1").arg(i));
      xmlStream.writeAttribute("X", RequestList[i].Rect.left());
      xmlStream.writeAttribute("Y", RequestList[i].Rect.top());
      xmlStream.writeAttribute("R", RequestList[i].GoogleRequest);
      xmlStream.writeEndElement();
   }
}

//====================================================================================================================

bool cGMapsMap::GetInformationFromFile(QString&,QStringList *,bool *,qlonglong) 
{
    QSize Size=GetCurrentImageSize();
    imageWidth=Size.width();
    imageHeight=Size.height();
    imageOrientation=0;
    aspectRatio=double(imageHeight)/double(imageWidth);
    geometry = IMAGE_GEOMETRY_16_9;
    IsValide=true;
    return true;
}

//====================================================================================================================

bool cGMapsMap::GetChildFullInformationFromFile(bool IsPartial,cCustomIcon *Icon,QStringList *ExtendedProperties) 
{
    if ((cImageClipboard::GetChildFullInformationFromFile(IsPartial,Icon,ExtendedProperties))&&(ExtendedProperties)) 
    {
        ExtendedProperties->append(QApplication::translate("cBaseMediaFile","Map type")+QString("##")+QString("%1").arg(GetCurrentMapTypeName()));
        ExtendedProperties->append(QApplication::translate("cBaseMediaFile","Image size")+QString("##")+QString("%1").arg(GetCurrentImageSizeName()));
        ExtendedProperties->append(QApplication::translate("cBaseMediaFile","Map zoom and size")+QString("##")+QString("%1").arg(GetMapSizesPerZoomLevel()[ZoomLevel]));
        ExtendedProperties->append(QApplication::translate("cBaseMediaFile","Map latitude")+QString("##")+QString("%1").arg(PIXEL2GPS_Y(MapCy,ZoomLevel,Scale)));
        ExtendedProperties->append(QApplication::translate("cBaseMediaFile","Map longitude")+QString("##")+QString("%1").arg(PIXEL2GPS_X(MapCx,ZoomLevel,Scale)));
    }
    return true;
}

//====================================================================================================================

QString cGMapsMap::GetTechInfo(QStringList *) 
{
    return QString(QApplication::translate("cBaseMediaFile","%1 location(s)")).arg(locations.count());
}

//====================================================================================================================

QString cGMapsMap::GetTAGInfo(QStringList *) 
{
    return QString("%1-%2").arg(GetCurrentMapTypeName()).arg(GetCurrentImageSizeName());
}

//====================================================================================================================

QStringList cGMapsMap::GetSummaryText(QStringList *ExtendedProperties) 
{
    QStringList SummaryText;
    SummaryText.append(GetFileTypeStr());
    SummaryText.append(GetTAGInfo(ExtendedProperties));
    SummaryText.append(QApplication::translate("cBaseMediaFile","GPS Coordinates %1 / %2").arg(PIXEL2GPS_Y(MapCy,ZoomLevel,Scale)).arg(PIXEL2GPS_X(MapCx,ZoomLevel,Scale)));
    return SummaryText;
}

//====================================================================================================================

QImage *cGMapsMap::ImageAt(bool PreviewMode) 
{
    if (ressourceKey == -1) 
    {
        QImage   *Img=new QImage(":/img/defaultmap.png");  // If no ressource at this point then we are displaying a model !
        QString  Text=QApplication::translate("cBaseMediaFile","%1\nZoom %2").arg(GetCurrentImageSizeName(false)).arg(ZoomLevel);
        QPainter P;
        P.begin(Img);
        QFont font= QApplication::font();
        font.setPixelSize(double(Img->height())/double(6));
        P.setFont(font);
        P.setPen(QPen(Qt::black));
        P.drawText(QRectF(0,0,Img->width()-2,Img->height()-2),Text,QTextOption(Qt::AlignHCenter|Qt::AlignVCenter));
        P.drawText(QRectF(1,0,Img->width()-2,Img->height()-2),Text,QTextOption(Qt::AlignHCenter|Qt::AlignVCenter));
        P.drawText(QRectF(2,0,Img->width()-2,Img->height()-2),Text,QTextOption(Qt::AlignHCenter|Qt::AlignVCenter));
        P.drawText(QRectF(0,2,Img->width()-2,Img->height()-2),Text,QTextOption(Qt::AlignHCenter|Qt::AlignVCenter));
        P.drawText(QRectF(1,2,Img->width()-2,Img->height()-2),Text,QTextOption(Qt::AlignHCenter|Qt::AlignVCenter));
        P.drawText(QRectF(2,2,Img->width()-2,Img->height()-2),Text,QTextOption(Qt::AlignHCenter|Qt::AlignVCenter));
        P.drawText(QRectF(0,1,Img->width()-2,Img->height()-2),Text,QTextOption(Qt::AlignHCenter|Qt::AlignVCenter));
        P.drawText(QRectF(2,1,Img->width()-2,Img->height()-2),Text,QTextOption(Qt::AlignHCenter|Qt::AlignVCenter));
        P.setPen(QPen(Qt::white));
        P.drawText(QRectF(1,1,Img->width()-2,Img->height()-2),Text,QTextOption(Qt::AlignHCenter|Qt::AlignVCenter));
        P.end();
        return Img;
    }   else if (IsMapValide)   return cImageClipboard::ImageAt(PreviewMode);
        else                    return new QImage(CreateDefaultImage(NULL));
}

//====================================================================================================================

QStringList cGMapsMap::GetGoogleMapTypeNames() {
    QStringList GoogleMapTypeName;
    GoogleMapTypeName.append("roadmap");
    GoogleMapTypeName.append("satellite");
    GoogleMapTypeName.append("terrain");
    GoogleMapTypeName.append("hybrid");
    return GoogleMapTypeName;
}

QStringList cGMapsMap::GetMapTypeNames() 
{
    QStringList List;
    List.append(QApplication::translate("cBaseMediaFile","Roadmap"));
    List.append(QApplication::translate("cBaseMediaFile","Satellite"));
    List.append(QApplication::translate("cBaseMediaFile","Terrain"));
    List.append(QApplication::translate("cBaseMediaFile","Hybrid"));
    return List;
}

QStringList cGMapsMap::GetShortImageSizeNames() 
{
    QStringList List;
    List.append(QApplication::translate("cBaseMediaFile","Small"));
    List.append("720p");
    List.append("720px4");
    List.append("720px9");
    List.append("1080p");
    List.append("1080px4");
    List.append("1080px9");
    return List;
}

QStringList cGMapsMap::GetImageSizeNames() 
{
    QStringList List;
    List.append(QApplication::translate("cBaseMediaFile","Small (640x360)"));
    List.append(QApplication::translate("cBaseMediaFile","720p-16:9 (1280x720)"));
    List.append(QApplication::translate("cBaseMediaFile","720px4-16:9 (2560x1440)"));
    List.append(QApplication::translate("cBaseMediaFile","720px9-16:9 (3840x2160)"));
    List.append(QApplication::translate("cBaseMediaFile","1080p-16:9 (1920x1080)"));
    List.append(QApplication::translate("cBaseMediaFile","1080px4-16:9 (3840x2160)"));
    List.append(QApplication::translate("cBaseMediaFile","1080px9-16:9 (5760x3240)"));
    return List;
}

QString cGMapsMap::GetCurrentMapTypeName() 
{
    return GetMapTypeNames().at(MapType);
}

QString cGMapsMap::GetCurrentGoogleMapTypeName() 
{
    return GetGoogleMapTypeNames().at(MapType);
}

QString cGMapsMap::GetCurrentImageSizeName(bool Full) 
{
    if (Full) return GetImageSizeNames().at(ImageSize);
        else  return GetShortImageSizeNames().at(ImageSize);
}

QSize cGMapsMap::GetCurrentImageSize() 
{
    switch (ImageSize) 
    {
        case GMapsImageSize_NBR:
        case Small:             return QSize(640,360);
        case FS720P:            return QSize(1280,720);
        case FS720X4:           return QSize(2560,1440);
        case FS720X9:           return QSize(3840,2160);
        case FS1080P:           return QSize(1920,1080);
        case FS1080X4:          return QSize(3840,2160);
        case FS1080X9:          return QSize(5760,3240);
    }
    return QSize(600,600);
}

//====================================================================================================================
// return minimum zoom level depending on current image size

int cGMapsMap::GetMinZoomLevelForSize() 
{
    switch (ImageSize) 
    {
        case GMapsImageSize_NBR:
        case Small:             return 2;
        case FS720P:            return 3;
        case FS720X4:           return 4;
        case FS720X9:           return 5;
        case FS1080P:           return 4;
        case FS1080X4:          return 5;
        case FS1080X9:          return 6;
    }
    return 5;
}

//====================================================================================================================

int cGMapsMap::ComputeNbrSection(int Size,int Divisor) 
{
    int Ret=Size/Divisor;
    if (Ret*Divisor<Size) Ret++;
    return Ret;
}

QRectF cGMapsMap::GetGPSRectF() 
{
    if (locations.isEmpty()) return QRectF(0,0,0,0);
    double  GPS_x1=((cLocation *)locations.at(0))->GPS_cx;
    double  GPS_x2=GPS_x1;
    double  GPS_y1=((cLocation *)locations.at(0))->GPS_cy;
    double  GPS_y2=GPS_y1;

    for (int i=1;i<locations.count();i++) 
    {
        if (locations.at(i)->GPS_cx<GPS_x1) GPS_x1 = locations.at(i)->GPS_cx;
        if (locations.at(i)->GPS_cx>GPS_x2) GPS_x2 = locations.at(i)->GPS_cx;
        if (locations.at(i)->GPS_cy<GPS_y1) GPS_y1 = locations.at(i)->GPS_cy;
        if (locations.at(i)->GPS_cy>GPS_y2) GPS_y2 = locations.at(i)->GPS_cy;
    }
    return QRectF(GPS_x1,GPS_y1,GPS_x2-GPS_x1,GPS_y2-GPS_y1);
}

QStringList cGMapsMap::GetMapSizesPerZoomLevel() 
{
    QStringList DistanceList;
    QSize       MapS   =GetCurrentImageSize();
    QRectF      Wanted =GetGPSRectF();
    double      WWidth =DISTANCE(Wanted.left(),Wanted.top(),Wanted.right(),Wanted.top());
    double      WHeight=DISTANCE(Wanted.left(),Wanted.top(),Wanted.left(), Wanted.bottom());
    double      W      =MapS.width();
    double      H      =MapS.height();
    double      GPS0x  =0;
    double      GPS0y  =0;
    int         Start  =GetMinZoomLevelForSize();

    for (int i=0;i<Start;i++) DistanceList.append(QString());

    for (int i=Start;i<=21;i++) {
        double  PIX0x =GPS2PIXEL_X(GPS0x,i,Scale);
        double  PIX0y =GPS2PIXEL_Y(GPS0y,i,Scale);
        double  GPS1x =PIXEL2GPS_X(PIX0x+W,i,Scale);
        double  GPS1y =PIXEL2GPS_Y(PIX0y+H,i,Scale);
        double  Width =DISTANCE(GPS0x,GPS0y,GPS1x,GPS0y);
        double  Height=DISTANCE(GPS0x,GPS0y,GPS0x,GPS1y);
        if ((locations.count()==1 || (Width >= WWidth && Height >= WHeight)) && Width >= 0.4 && Height >= 0.4) 
        {
            if (ApplicationConfig->DistanceUnit==cApplicationConfig::MILES) DistanceList.append(QString("Zoom %1: %2 miles x %3 miles").arg(i).arg(KMTOMILES(Width),0,'f',3).arg(KMTOMILES(Height),0,'f',3));
                else                                                            DistanceList.append(QString("Zoom %1: %2 km x %3 km").arg(i).arg(Width,0,'f',3).arg(Height,0,'f',3));
        } else DistanceList.append(QString());
    }
    return DistanceList;
}

QRectF cGMapsMap::GetPixRectF() 
{
    QRectF  GPSRect=GetGPSRectF();
    QRectF  PixRect;
    PixRect.setLeft  (GPS2PIXEL_X(GPSRect.left(),  ZoomLevel,Scale));
    PixRect.setRight (GPS2PIXEL_X(GPSRect.right(), ZoomLevel,Scale));
    PixRect.setTop   (GPS2PIXEL_Y(GPSRect.top(),   ZoomLevel,Scale));
    PixRect.setBottom(GPS2PIXEL_Y(GPSRect.bottom(),ZoomLevel,Scale));
    return PixRect;
}

void cGMapsMap::ComputeSectionList() 
{
    QSize   IMSize  =GetCurrentImageSize();
    double  Map_Cx  =IMSize.width()/2;
    double  Map_Cy  =IMSize.height()/2;
    QRectF  PixRect =GetPixRectF();

    MapCx=PixRect.center().x();
    MapCy=PixRect.center().y();

    RequestList.clear();

    RequestSection Item;
    for (int i=0;i<ComputeNbrSection(IMSize.height(),SectionHeight);i++) for (int j=0;j<ComputeNbrSection(IMSize.width(),SectionWith);j++) {
        if (IMSize.height()<=SectionHeight) Item.Rect=QRectF(0,0,IMSize.width(),IMSize.height());
            else                            Item.Rect=QRectF(SectionWith*j,SectionHeight*i,SectionWith,SectionHeight);
        Item.GoogleRequest=QString("http://maps.googleapis.com/maps/api/staticmap?center=%1,%2&zoom=%3&size=640x640&scale=%4&maptype=%5&sensor=false")
                .arg(PIXEL2GPS_Y((Item.Rect.center().y()-Map_Cy+MapCy),ZoomLevel,Scale))
                .arg(PIXEL2GPS_X((Item.Rect.center().x()-Map_Cx+MapCx),ZoomLevel,Scale))
                .arg(ZoomLevel).arg(Scale).arg(GetCurrentGoogleMapTypeName());
        RequestList.append(Item);
    }
}

//====================================================================================================================

QPoint cGMapsMap::GetLocationPoint(int Index) 
{
   QSize   IMSize  = GetCurrentImageSize();
   double  Map_Cx  = IMSize.width()/2;
   double  Map_Cy  = IMSize.height()/2;
   QPoint  Ret(Map_Cx,Map_Cy);
   if (Index < 0 || Index > locations.count()) 
      return Ret;
   Ret.setX(GPS2PIXEL_X(locations[Index]->GPS_cx,ZoomLevel,Scale)-MapCx+Map_Cx);
   Ret.setY(GPS2PIXEL_Y(locations[Index]->GPS_cy,ZoomLevel,Scale)-MapCy+Map_Cy);
   return Ret;
}

//====================================================================================================================

QImage cGMapsMap::CreateDefaultImage(cDiaporama *Diaporama) 
{
    // clear request list (delete any pending section)
    RequestList.clear();

    // Create a new ressource key
    ressourceKey = -1;
    //ApplicationConfig->ImagesCache.RemoveImageObject(RessourceKey,FileKey);   // remove object from Lulo if it exist

    // create new empty image
    QImage Image(GetCurrentImageSize(),QImage::Format_ARGB32_Premultiplied);
    imageWidth   =Image.width();
    imageHeight  =Image.height();
    Image.fill(Qt::white);

    // search if list contains Project's or Chapter's location
    bool HaveLinkLoc=false;
    for (int i=0;i<locations.count();i++) if (locations[i]->LocationType != cLocation::FREE) HaveLinkLoc=true;

    if ((HaveLinkLoc)&&((!Diaporama)||(Diaporama->ProjectInfo()->Location() == NULL))) {
        // add a message on the image if project location is empty
        QPainter Painter;
        Painter.begin(&Image);
        QFont font= QApplication::font();
        font.setPixelSize(double(Image.height())/double(20));
        Painter.setFont(font);
        if (!Diaporama) Painter.drawText(QRectF(0,0,Image.width(),Image.height()),
                             Qt::AlignHCenter|Qt::AlignCenter|Qt::TextWordWrap,
                             QApplication::translate("cBaseMediaFile","The map must be regenerated because the locations have changed or project's location no set"));
            else        Painter.drawText(QRectF(0,0,Image.width(),Image.height()),
                             Qt::AlignHCenter|Qt::AlignCenter|Qt::TextWordWrap,
                             QApplication::translate("cBaseMediaFile","Project's location must be defined to produce this Google Maps map"));

        Painter.end();
    } 
    else if (locations.isEmpty()) 
    {
        // add a message on the image if location list is empty
        QPainter Painter;
        Painter.begin(&Image);
        QFont font= QApplication::font();
        font.setPixelSize(double(Image.height())/double(20));
        Painter.setFont(font);
        Painter.drawText(QRectF(0,0,Image.width(),Image.height()),
                         Qt::AlignHCenter|Qt::AlignCenter|Qt::TextWordWrap,
                         QApplication::translate("cBaseMediaFile","Select at least one location to produce Google Maps map"));
        Painter.end();
    }

    // update ressource image in database
    ApplicationConfig->SlideThumbsTable->SetThumb(&ressourceKey,Image);

    // return image
    return Image;
}

#ifndef IS_FFMPEG_414
/*************************************************************************************************************************************
    CLASS cVideoFile
*************************************************************************************************************************************/

cImageInCache::cImageInCache(int64_t Position,AVFrame *FiltFrame,AVFrame *FrameBufferYUV) 
{
   this->Position       = Position;
   this->FiltFrame      = FiltFrame;
   this->FrameBufferYUV = FrameBufferYUV;
}

cImageInCache::~cImageInCache() 
{
    if (FiltFrame) 
    {
        av_frame_unref(FiltFrame);
        av_frame_free(&FiltFrame);
    }
    if (FrameBufferYUV)
    {
       av_frame_unref(FrameBufferYUV);
       av_frame_free(&FrameBufferYUV);
    }
    //FREEFRAME(&FrameBufferYUV);
}

cVideoFile::cVideoFile(cApplicationConfig *ApplicationConfig) : cBaseMediaFile(ApplicationConfig) 
{
   Reset(OBJECTTYPE_VIDEOFILE);
   yuv2rgbImage = NULL;
   FrameBufferRGB = 0;
   img_convert_ctx = 0;
}

void cVideoFile::Reset(OBJECTTYPE TheWantedObjectType) 
{
   cBaseMediaFile::Reset();

   MusicOnly      = (TheWantedObjectType==OBJECTTYPE_MUSICFILE);
   ObjectType     = TheWantedObjectType;
   IsOpen         = false;
   StartTime      = QTime(0,0,0,0);   // Start position
   EndTime        = QTime(0,0,0,0);   // End position
   AVStartTime    = QTime(0,0,0,0);
   LibavStartTime = 0;

   // Video part
   IsMTS                   = false;
   LibavVideoFile          = NULL;
   VideoDecoderCodec       = NULL;
   VideoStreamNumber       = 0;
   FrameBufferYUV          = NULL;
   FrameBufferYUVReady     = false;
   FrameBufferYUVPosition  = 0;
   VideoCodecInfo          = "";
   VideoTrackNbr           = 0;
   VideoStreamNumber       = -1;
   NbrChapters             = 0;

   // Audio part
   LibavAudioFile          = NULL;
   AudioDecoderCodec       = NULL;
   LastAudioReadPosition   = -1;
   IsVorbis                = false;
   AudioCodecInfo          = "";
   AudioTrackNbr           = 0;
   AudioStreamNumber       = -1;

   // Audio resampling
   RSC                     = NULL;
   RSC_InChannels          = 2;
   RSC_OutChannels         = 2;
   RSC_InSampleRate        = 48000;
   RSC_OutSampleRate       = 48000;
   RSC_InChannelLayout     = av_get_default_channel_layout(2);
   RSC_OutChannelLayout    = av_get_default_channel_layout(2);
   RSC_InSampleFmt         = AV_SAMPLE_FMT_S16;
   RSC_OutSampleFmt        = AV_SAMPLE_FMT_S16;

   // Filter part
   VideoFilterGraph        = NULL;
   VideoFilterIn           = NULL;
   VideoFilterOut          = NULL;
}                             

//====================================================================================================================

cVideoFile::~cVideoFile() 
{
   // Close LibAVFormat and LibAVCodec contexte for the file
   CloseCodecAndFile();
   if( yuv2rgbImage )
      delete yuv2rgbImage;
}

//====================================================================================================================

bool cVideoFile::DoAnalyseSound(QList<qreal> *Peak,QList<qreal> *Moyenne,bool *CancelFlag,qreal *Analysed) 
{
   bool IsAnalysed = LoadAnalyseSound(Peak,Moyenne);
   if (IsAnalysed) 
      return IsAnalysed;

   qint64          NewPosition = 0, Position = -1;
   qint64          Duration = QTime(0,0,0,0).msecsTo(GetRealDuration());
   int             WantedValues;
   QList<qreal>    Values;
   int16_t         *Block = NULL, *CurData = NULL;
   cSoundBlockList AnalyseMusic;

   AnalyseMusic.SetFPS(2000,2,1000,AV_SAMPLE_FMT_S16);
   WantedValues = (Duration/2000);
   Peak->clear();
   Moyenne->clear();

   //*******************************************************************************************
   // Load music and compute music count, max value, 2000 peak and 2000 moyenne values
   // decibels=decibels>0?0.02*log10(decibels/32768.0):0;    // PCM S16 is from -48db to +48db
   //*******************************************************************************************
   while ((!*CancelFlag) && (Position != NewPosition)) 
   {
      *Analysed = qreal(NewPosition)/qreal(Duration);
      QApplication::processEvents();
      Position = NewPosition;
      ReadFrame(true,Position*1000,true,false,&AnalyseMusic,1,true);
      NewPosition += qreal(AnalyseMusic.ListCount())*AnalyseMusic.dDuration*qreal(1000);
      while (AnalyseMusic.ListCount() > 0) 
      {
         Block=AnalyseMusic.DetachFirstPacket();
         if( Block )
         {
            CurData = Block;
            Values.reserve(WantedValues);
            for (int j = 0; j < AnalyseMusic.SoundPacketSize/4; j++) 
            {
               //int16_t sample16Bit = *CurData++;
               double  decibels1 = abs(*CurData++);
               //sample16Bit = *CurData++;
               double  decibels2 = abs(*CurData++);
               //double  decibels = (decibels1 + decibels2)/2;
               Values.append((decibels1 + decibels2)/2);
               if (Values.count() == WantedValues) 
               {
                  qreal vPeak = 0, vMoyenne = 0;
                  foreach (qreal V,Values) 
                  {
                     if (vPeak < V) 
                        vPeak = V;
                     vMoyenne = vMoyenne + V;
                  }
                  vMoyenne = vMoyenne/Values.count();
                  Peak->append(vPeak);
                  Moyenne->append(vMoyenne);
                  Values.clear();
                  Values.reserve(WantedValues);
               }
            }
            av_free(Block);
         }
      }
   }
   // tempdata
   CurData = (int16_t *)AnalyseMusic.TempData;
   for (int j = 0; j < AnalyseMusic.CurrentTempSize/4; j++) 
   {
      //int16_t sample16Bit = *CurData++;
      double  decibels1 = abs(*CurData++);
      //sample16Bit = *CurData++;
      double  decibels2 = abs(*CurData++);
      //double  decibels = (decibels1 + decibels2)/2;
      Values.append((decibels1 + decibels2)/2);
      if (Values.count() == WantedValues) 
      {
         qreal vPeak = 0, vMoyenne = 0;
         foreach (qreal V,Values) 
         {
            if (vPeak < V) 
               vPeak = V;
            vMoyenne = vMoyenne + V;
         }
         vMoyenne = vMoyenne/Values.count();
         Peak->append(vPeak);
         Moyenne->append(vMoyenne);
         Values.clear();
         Values.reserve(WantedValues);
      }
   }
   if (Values.count() > 0) 
   {
      qreal vPeak = 0, vMoyenne = 0;
      foreach (qreal V,Values) 
      {
         if (vPeak < V) 
            vPeak = V;
         vMoyenne = vMoyenne + V;
      }
      vMoyenne = vMoyenne/Values.count();
      Peak->append(vPeak);
      Moyenne->append(vMoyenne);
      Values.clear();
   }

   // Compute MaxSoundValue as 90% of the max peak value
   QList<qreal> MaxVal;
   MaxVal = *Peak;
   //foreach (qreal Value,*Peak) 
   //   MaxVal.append(Value);
   QT_QSORT(MaxVal.begin(),MaxVal.end());
   qreal MaxSoundValue = MaxVal.count()> 0 ? MaxVal[MaxVal.count()*0.9] : 1;
   if( MaxSoundValue == 0 )
      MaxSoundValue = 1;

   // Adjust Peak and Moyenne values by transforming them as % of the max value
   for (int i = 0; i < Peak->count(); i++) 
   {
      (*Peak)[i]    = (*Peak)[i]/MaxSoundValue;
      (*Moyenne)[i] = (*Moyenne)[i]/MaxSoundValue;
   }
   MaxVal = *Moyenne;
   //MaxVal.clear();
   //foreach (qreal Value,*Moyenne) 
   //   MaxVal.append(Value);
   QT_QSORT(MaxVal.begin(),MaxVal.end());
   MaxSoundValue = MaxVal.count() > 0 ? MaxVal[MaxVal.count()*0.9] : 1;

   //**************************
   // End analyse
   //**************************
   IsAnalysed = true;
   SaveAnalyseSound(Peak,Moyenne,MaxSoundValue);
   if (EndTime > GetRealDuration()) 
      EndTime = GetRealDuration();
   return IsAnalysed;
}

//====================================================================================================================

bool cVideoFile::LoadBasicInformationFromDatabase(QDomElement *ParentElement,QString,QString,QStringList *,bool *,TResKeyList *,bool) 
{
    imageWidth       =ParentElement->attribute("ImageWidth").toInt();
    imageHeight      =ParentElement->attribute("ImageHeight").toInt();
    imageOrientation =ParentElement->attribute("ImageOrientation").toInt();
    geometry   =ParentElement->attribute("ObjectGeometry").toInt();
    aspectRatio      =GetDoubleValue(*ParentElement,"AspectRatio");
    NbrChapters      =ParentElement->attribute("NbrChapters").toInt();
    VideoStreamNumber=ParentElement->attribute("VideoStreamNumber").toInt();
    VideoTrackNbr    =ParentElement->attribute("VideoTrackNbr").toInt();
    AudioStreamNumber=ParentElement->attribute("AudioStreamNumber").toInt();
    AudioTrackNbr    =ParentElement->attribute("AudioTrackNbr").toInt();
    if (ParentElement->hasAttribute("Duration"))            SetGivenDuration(QTime(0,0,0,0).addMSecs(ParentElement->attribute("Duration").toLongLong()));
    if (ParentElement->hasAttribute("RealDuration"))        SetRealAudioDuration(QTime(0,0,0,0).addMSecs(ParentElement->attribute("RealDuration").toLongLong()));
    if (ParentElement->hasAttribute("RealAudioDuration"))   SetRealAudioDuration(QTime(0,0,0,0).addMSecs(ParentElement->attribute("RealAudioDuration").toLongLong()));
    if (ParentElement->hasAttribute("RealVideoDuration"))   SetRealVideoDuration(QTime(0,0,0,0).addMSecs(ParentElement->attribute("RealVideoDuration").toLongLong()));
    if (ParentElement->hasAttribute("SoundLevel"))          SetSoundLevel(GetDoubleValue(*ParentElement,"SoundLevel"));
    if (ParentElement->hasAttribute("IsComputedAudioDuration"))  IsComputedAudioDuration=ParentElement->attribute("IsComputedAudioDuration")=="1";
    if (EndTime == QTime(0,0,0,0)) EndTime = GetRealDuration();
    return true;
}

//====================================================================================================================

void cVideoFile::SaveBasicInformationToDatabase(QDomElement *ParentElement,QString,QString,bool,cReplaceObjectList *,QList<qlonglong> *,bool) 
{
    ParentElement->setAttribute("ImageWidth",        imageWidth);
    ParentElement->setAttribute("ImageHeight",       imageHeight);
    ParentElement->setAttribute("ImageOrientation",  imageOrientation);
    ParentElement->setAttribute("ObjectGeometry",    geometry);
    ParentElement->setAttribute("AspectRatio",       QString("%1").arg(aspectRatio,0,'f'));
    ParentElement->setAttribute("Duration",          QTime(0,0,0,0).msecsTo(GetGivenDuration()));
    ParentElement->setAttribute("RealAudioDuration", QTime(0,0,0,0).msecsTo(GetRealAudioDuration()));
    if (ObjectType==OBJECTTYPE_VIDEOFILE) 
      ParentElement->setAttribute("RealVideoDuration", QTime(0,0,0,0).msecsTo(GetRealVideoDuration()));
    ParentElement->setAttribute("SoundLevel",        GetSoundLevel());
    ParentElement->setAttribute("IsComputedAudioDuration",IsComputedAudioDuration?"1":"0");
    ParentElement->setAttribute("NbrChapters",       NbrChapters);
    ParentElement->setAttribute("VideoStreamNumber", VideoStreamNumber);
    ParentElement->setAttribute("VideoTrackNbr",     VideoTrackNbr);
    ParentElement->setAttribute("AudioStreamNumber", AudioStreamNumber);
    ParentElement->setAttribute("AudioTrackNbr",     AudioTrackNbr);
}

//====================================================================================================================
#include "ffTools.h"
// Overloaded function use to dertermine if format of media file is correct

bool cVideoFile::CheckFormatValide(QWidget *Window) 
{
   bool IsOk = IsValide;

   // try to open file
   if (!OpenCodecAndFile()) 
   {
      QString ErrorMessage = QApplication::translate("MainWindow","Format not supported","Error message");
      CustomMessageBox(Window,QMessageBox::Critical,QApplication::translate("MainWindow","Error","Error message"),ShortName()+"\n\n"+ErrorMessage,QMessageBox::Close);
      IsOk = false;
   }

   // check if file have at least one sound track compatible
   if (IsOk && (AudioStreamNumber != -1)) 
   {
      if (!((LibavAudioFile->streams[AudioStreamNumber]->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_S16) || (LibavAudioFile->streams[AudioStreamNumber]->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_U8)))
      {
         QString ErrorMessage = "\n"+QApplication::translate("MainWindow","This application support only audio track with unsigned 8 bits or signed 16 bits sample format","Error message");
         CustomMessageBox(Window,QMessageBox::Critical,QApplication::translate("MainWindow","Error","Error message"),ShortName()+"\n\n"+ErrorMessage,QMessageBox::Close);
         IsOk = false;
      }

   }

   // Try to load an image to ensure all is ok
   if (IsOk) 
   {
      QImage *Image = ImageAt(true,0,NULL,true,1,false,false);
      if (Image) 
      {
         delete Image;
      } 
      else 
      {
         QString ErrorMessage = "\n"+QApplication::translate("MainWindow","Impossible to read one image from the file","Error message");
         CustomMessageBox(Window,QMessageBox::Critical,QApplication::translate("MainWindow","Error","Error message"),ShortName()+"\n\n"+ErrorMessage,QMessageBox::Close);
         IsOk = false;
      }
   }
   // close file if it was opened
   CloseCodecAndFile();

   return IsOk;
}

//====================================================================================================================

// Overloaded function use to dertermine if media file correspond to WantedObjectType
//      WantedObjectType could be OBJECTTYPE_VIDEOFILE or OBJECTTYPE_MUSICFILE
//      if AudioOnly was set to true in constructor then ignore all video track and set WantedObjectType to OBJECTTYPE_MUSICFILE else set it to OBJECTTYPE_VIDEOFILE
//      return true if WantedObjectType=OBJECTTYPE_VIDEOFILE and at least one video track is present
//      return true if WantedObjectType=OBJECTTYPE_MUSICFILE and at least one audio track is present

bool cVideoFile::GetChildFullInformationFromFile(bool,cCustomIcon *Icon,QStringList *ExtendedProperties) 
{
    //Mutex.lock();
    bool            Continu=true;
    AVFormatContext *LibavFile=NULL;
    QString         sFileName=FileName();

    //*********************************************************************************************************
    // Open file and get a LibAVFormat context and an associated LibAVCodec decoder
    //*********************************************************************************************************
    char filename[512];
    strcpy(filename,sFileName.toLocal8Bit());
    if (avformat_open_input(&LibavFile,filename,NULL,NULL)!=0) {
        LibavFile=NULL;
        //Mutex.unlock();
        return false;
    }
    ExtendedProperties->append(QString("Short Format##")+QString(LibavFile->iformat->name));
    ExtendedProperties->append(QString("Long Format##")+QString(LibavFile->iformat->long_name));
    LibavFile->flags|=AVFMT_FLAG_GENPTS;       // Generate missing pts even if it requires parsing future NbrFrames.

    //*********************************************************************************************************
    // Search stream in file
    //*********************************************************************************************************
    if (avformat_find_stream_info(LibavFile,NULL)<0) 
    {
        avformat_close_input(&LibavFile);
        LibavFile=NULL;
        Continu=false;
    }

    if (Continu) 
    {
        //*********************************************************************************************************
        // Get metadata
        //*********************************************************************************************************
        AVDictionaryEntry *tag=NULL;
        while ((tag=av_dict_get(LibavFile->metadata,"",tag,AV_DICT_IGNORE_SUFFIX))) {
            QString Value=QString().fromUtf8(tag->value);
            #ifdef Q_OS_WIN
            Value.replace(char(13),"\n");
            #endif
            if (Value.endsWith("\n")) Value=Value.left(Value.lastIndexOf("\n"));
            ExtendedProperties->append(QString().fromUtf8(tag->key).toLower()+QString("##")+Value);
        }

        //*********************************************************************************************************
        // Get chapters
        //*********************************************************************************************************
        NbrChapters=0;
        for (uint i=0;i<LibavFile->nb_chapters;i++) {
            AVChapter   *ch=LibavFile->chapters[i];
            QString     ChapterNum=QString("%1").arg(NbrChapters);
            while (ChapterNum.length()<3) ChapterNum="0"+ChapterNum;
            int64_t Start=double(ch->start)*(double(av_q2d(ch->time_base))*1000);     // Lib AV use 1/1 000 000 000 sec and we want msec !
            int64_t End  =double(ch->end)*(double(av_q2d(ch->time_base))*1000);       // Lib AV use 1/1 000 000 000 sec and we want msec !

            // Special case if it's first chapter and start!=0 => add a chapter 0
            if ((NbrChapters==0)&&(LibavFile->chapters[i]->start>0)) {
                ExtendedProperties->append("Chapter_"+ChapterNum+":Start"   +QString("##")+QTime(0,0,0,0).toString("hh:mm:ss.zzz"));
                ExtendedProperties->append("Chapter_"+ChapterNum+":End"     +QString("##")+QTime(0,0,0,0).addMSecs(Start).toString("hh:mm:ss.zzz"));
                ExtendedProperties->append("Chapter_"+ChapterNum+":Duration"+QString("##")+QTime(0,0,0,0).addMSecs(Start).toString("hh:mm:ss.zzz"));
                if (GetInformationValue("title",ExtendedProperties)!="") ExtendedProperties->append("Chapter_"+ChapterNum+":title##"+GetInformationValue("title",ExtendedProperties));
                    else ExtendedProperties->append("Chapter_"+ChapterNum+":title##"+QFileInfo(sFileName).baseName());
                NbrChapters++;
                ChapterNum=QString("%1").arg(NbrChapters);
                while (ChapterNum.length()<3) ChapterNum="0"+ChapterNum;
            }

            ExtendedProperties->append("Chapter_"+ChapterNum+":Start"   +QString("##")+QTime(0,0,0,0).addMSecs(Start).toString("hh:mm:ss.zzz"));
            ExtendedProperties->append("Chapter_"+ChapterNum+":End"     +QString("##")+QTime(0,0,0,0).addMSecs(End).toString("hh:mm:ss.zzz"));
            ExtendedProperties->append("Chapter_"+ChapterNum+":Duration"+QString("##")+QTime(0,0,0,0).addMSecs(End-Start).toString("hh:mm:ss.zzz"));
            // Chapter metadata
            while ((tag=av_dict_get(ch->metadata,"",tag,AV_DICT_IGNORE_SUFFIX)))
                ExtendedProperties->append("Chapter_"+ChapterNum+":"+QString().fromUtf8(tag->key).toLower()+QString("##")+QString().fromUtf8(tag->value));

            NbrChapters++;
        }

        //*********************************************************************************************************
        // Get information about duration
        //*********************************************************************************************************
        int64_t duration = (LibavFile->duration / AV_TIME_BASE);
        int hours = (int)(duration / 3600);
        int min = (int)(duration / 60 % 60);
        int sec = (int)(duration % 60);
        int ms = (int)(LibavFile->duration / (AV_TIME_BASE / 1000) % 1000);
        SetGivenDuration(QTime(hours, min, sec, ms));

        EndTime = GetRealDuration();

        //*********************************************************************************************************
        // Get information from track
        //*********************************************************************************************************
        for (int Track=0;Track<(int)LibavFile->nb_streams;Track++) 
        {

            // Find codec
            AVCodec *Codec=avcodec_find_decoder(LibavFile->streams[Track]->CODEC_OR_PAR->codec_id);

            //*********************************************************************************************************
            // Audio track
            //*********************************************************************************************************
            if (LibavFile->streams[Track]->CODEC_OR_PAR->codec_type==AVMEDIA_TYPE_AUDIO)
            {
                // Keep this as default track
                if (AudioStreamNumber==-1) AudioStreamNumber=Track;

                // Compute TrackNum
                QString TrackNum=QString("%1").arg(AudioTrackNbr);
                while (TrackNum.length()<3) TrackNum="0"+TrackNum;
                TrackNum="Audio_"+TrackNum+":";

                // General
                ExtendedProperties->append(TrackNum+QString("Track")+QString("##")+QString("%1").arg(Track));
                if (Codec) ExtendedProperties->append(TrackNum+QString("Codec")+QString("##")+QString(Codec->name));

                // Channels and Sample format
                QString SampleFMT="";
                switch (LibavFile->streams[Track]->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT) {
                    case AV_SAMPLE_FMT_U8  : SampleFMT="-U8";   ExtendedProperties->append(TrackNum+QString("Sample format")+QString("##")+"unsigned 8 bits");          break;
                    case AV_SAMPLE_FMT_S16 : SampleFMT="-S16";  ExtendedProperties->append(TrackNum+QString("Sample format")+QString("##")+"signed 16 bits");           break;
                    case AV_SAMPLE_FMT_S32 : SampleFMT="-S32";  ExtendedProperties->append(TrackNum+QString("Sample format")+QString("##")+"signed 32 bits");           break;
                    case AV_SAMPLE_FMT_FLT : SampleFMT="-FLT";  ExtendedProperties->append(TrackNum+QString("Sample format")+QString("##")+"float");                    break;
                    case AV_SAMPLE_FMT_DBL : SampleFMT="-DBL";  ExtendedProperties->append(TrackNum+QString("Sample format")+QString("##")+"double");                   break;
                    case AV_SAMPLE_FMT_U8P : SampleFMT="-U8P";  ExtendedProperties->append(TrackNum+QString("Sample format")+QString("##")+"unsigned 8 bits, planar");  break;
                    case AV_SAMPLE_FMT_S16P: SampleFMT="-S16P"; ExtendedProperties->append(TrackNum+QString("Sample format")+QString("##")+"signed 16 bits, planar");   break;
                    case AV_SAMPLE_FMT_S32P: SampleFMT="-S32P"; ExtendedProperties->append(TrackNum+QString("Sample format")+QString("##")+"signed 32 bits, planar");   break;
                    case AV_SAMPLE_FMT_FLTP: SampleFMT="-FLTP"; ExtendedProperties->append(TrackNum+QString("Sample format")+QString("##")+"float, planar");            break;
                    case AV_SAMPLE_FMT_DBLP: SampleFMT="-DBLP"; ExtendedProperties->append(TrackNum+QString("Sample format")+QString("##")+"double, planar");           break;
                    default                : SampleFMT="-?";    ExtendedProperties->append(TrackNum+QString("Sample format")+QString("##")+"Unknown");                  break;
                }
                if (LibavFile->streams[Track]->CODEC_OR_PAR->channels==1)      ExtendedProperties->append(TrackNum+QString("Channels")+QString("##")+QApplication::translate("cBaseMediaFile","Mono","Audio channels mode")+SampleFMT);
                else if (LibavFile->streams[Track]->CODEC_OR_PAR->channels==2) ExtendedProperties->append(TrackNum+QString("Channels")+QString("##")+QApplication::translate("cBaseMediaFile","Stereo","Audio channels mode")+SampleFMT);
                else                                                    ExtendedProperties->append(TrackNum+QString("Channels")+QString("##")+QString("%1").arg(LibavFile->streams[Track]->CODEC_OR_PAR->channels)+SampleFMT);

                // Frequency
                if (int(LibavFile->streams[Track]->CODEC_OR_PAR->sample_rate/1000)*1000>0) {
                    if (int(LibavFile->streams[Track]->CODEC_OR_PAR->sample_rate/1000)*1000==LibavFile->streams[Track]->CODEC_OR_PAR->sample_rate)
                         ExtendedProperties->append(TrackNum+QString("Frequency")+QString("##")+QString("%1").arg(int(LibavFile->streams[Track]->CODEC_OR_PAR->sample_rate/1000))+"Khz");
                    else ExtendedProperties->append(TrackNum+QString("Frequency")+QString("##")+QString("%1").arg(double(LibavFile->streams[Track]->CODEC_OR_PAR->sample_rate)/1000,8,'f',1).trimmed()+"Khz");
                }

                // Bitrate
                if (int(LibavFile->streams[Track]->CODEC_OR_PAR->bit_rate/1000)>0) ExtendedProperties->append(TrackNum+QString("Bitrate")+QString("##")+QString("%1").arg(int(LibavFile->streams[Track]->CODEC_OR_PAR->bit_rate/1000))+"Kb/s");

                // Stream metadata
                while ((tag=av_dict_get(LibavFile->streams[Track]->metadata,"",tag,AV_DICT_IGNORE_SUFFIX))) {
                    // OGV container affect TAG to audio stream !
                    QString Key=QString().fromUtf8(tag->key).toLower();
                    if ((sFileName.toLower().endsWith(".ogv"))&&((Key=="title")||(Key=="artist")||(Key=="album")||(Key=="comment")||(Key=="date")||(Key=="composer")||(Key=="encoder")))
                             ExtendedProperties->append(Key+QString("##")+QString().fromUtf8(tag->value));
                        else ExtendedProperties->append(TrackNum+Key+QString("##")+QString().fromUtf8(tag->value));
                }

                // Ensure language exist (Note : AVI and FLV container own language at container level instead of track level)
                if (GetInformationValue(TrackNum+"language",ExtendedProperties)=="") {
                    QString Lng=GetInformationValue("language",ExtendedProperties);
                    ExtendedProperties->append(TrackNum+QString("language##")+(Lng==""?"und":Lng));
                }

                // Next
                AudioTrackNbr++;

            //*********************************************************************************************************
            // Video track
            //*********************************************************************************************************
            } 
            else if (!MusicOnly && (LibavFile->streams[Track]->CODEC_OR_PAR->codec_type == AVMEDIA_TYPE_VIDEO))
            {
                // Compute TrackNum
                QString TrackNum=QString("%1").arg(VideoTrackNbr);
                while (TrackNum.length()<3) TrackNum="0"+TrackNum;
                TrackNum="Video_"+TrackNum+":";

                // General
                ExtendedProperties->append(TrackNum+QString("Track")+QString("##")+QString("%1").arg(Track));
                if (Codec) 
                   ExtendedProperties->append(TrackNum+QString("Codec")+QString("##")+QString(Codec->name));

                // Bitrate
                if (LibavFile->streams[Track]->CODEC_OR_PAR->bit_rate>0) 
                   ExtendedProperties->append(TrackNum+QString("Bitrate")+QString("##")+QString("%1").arg(int(LibavFile->streams[Track]->CODEC_OR_PAR->bit_rate/1000))+"Kb/s");

                // Frame rate
                if (int(double(LibavFile->streams[Track]->avg_frame_rate.num) / double(LibavFile->streams[Track]->avg_frame_rate.den)) > 0)
                {
                   if (int(double(LibavFile->streams[Track]->avg_frame_rate.num) / double(LibavFile->streams[Track]->avg_frame_rate.den)) == double(LibavFile->streams[Track]->avg_frame_rate.num) / double(LibavFile->streams[Track]->avg_frame_rate.den))
                      ExtendedProperties->append(TrackNum + QString("Frame rate") + QString("##") + QString("%1").arg(int(double(LibavFile->streams[Track]->avg_frame_rate.num) / double(LibavFile->streams[Track]->avg_frame_rate.den))) + " FPS");
                   else 
                      ExtendedProperties->append(TrackNum + QString("Frame rate") + QString("##") + QString("%1").arg(double(double(LibavFile->streams[Track]->avg_frame_rate.num) / double(LibavFile->streams[Track]->avg_frame_rate.den)), 8, 'f', 3).trimmed() + " FPS");
                }

                // Stream metadata
                while ((tag=av_dict_get(LibavFile->streams[Track]->metadata,"",tag,AV_DICT_IGNORE_SUFFIX)))
                    ExtendedProperties->append(TrackNum+QString(tag->key)+QString("##")+QString().fromUtf8(tag->value));

                // Ensure language exist (Note : AVI ‘AttachedPictureFrame’and FLV container own language at container level instead of track level)
                if (GetInformationValue(TrackNum+"language",ExtendedProperties)=="") {
                    QString Lng=GetInformationValue("language",ExtendedProperties);
                    ExtendedProperties->append(TrackNum+QString("language##")+(Lng==""?"und":Lng));
                }

                // Keep this as default track
                if (VideoStreamNumber==-1) {
                    QImage  *Img=NULL;
                    AVFrame *FrameBufYUV=NULL;


                    // Search if a jukebox mode thumbnail (jpg file with same name as video) exist
                    QFileInfo   File(sFileName);
                    QString     JPegFile=File.absolutePath()+(File.absolutePath().endsWith(QDir::separator())?"":QString(QDir::separator()))+File.completeBaseName()+".jpg";
                    if (QFileInfo(JPegFile).exists()) Icon->LoadIcons(JPegFile);

                    VideoStreamNumber=Track;
                    IsMTS=(sFileName.toLower().endsWith(".mts",Qt::CaseInsensitive) || sFileName.toLower().endsWith(".m2ts",Qt::CaseInsensitive) || sFileName.toLower().endsWith(".mod",Qt::CaseInsensitive));
                    LibavFile->flags|=AVFMT_FLAG_GENPTS;       // Generate missing pts even if it requires parsing future NbrFrames.
                    LibavFile->streams[VideoStreamNumber]->discard=AVDISCARD_DEFAULT;  // Setup STREAM options

                    // Setup decoder options
                    AVDictionary * av_opts = NULL;
                    #if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
                    LibavFile->streams[VideoStreamNumber]->codec->debug_mv         =0;                    // Debug level (0=nothing)
                    LibavFile->streams[VideoStreamNumber]->codec->debug            =0;                    // Debug level (0=nothing)
                    LibavFile->streams[VideoStreamNumber]->codec->workaround_bugs  = FF_BUG_AUTODETECT;                    // Work around bugs in encoders which sometimes cannot be detected automatically : 1=autodetection
                    LibavFile->streams[VideoStreamNumber]->codec->idct_algo        =FF_IDCT_AUTO;         // IDCT algorithm, 0=auto
                    LibavFile->streams[VideoStreamNumber]->codec->skip_frame       =AVDISCARD_DEFAULT;    // ???????
                    LibavFile->streams[VideoStreamNumber]->codec->skip_idct        =AVDISCARD_DEFAULT;    // ???????
                    LibavFile->streams[VideoStreamNumber]->codec->skip_loop_filter =AVDISCARD_DEFAULT;    // ???????
                    LibavFile->streams[VideoStreamNumber]->codec->error_concealment=3;
                    LibavFile->streams[VideoStreamNumber]->codec->thread_count     =getCpuCount();
                    LibavFile->streams[VideoStreamNumber]->codec->thread_type      =getThreadFlags(LibavFile->streams[VideoStreamNumber]->codec->codec_id);
                    #else
                    av_dict_set(&av_opts, "thread_count", QString("%1").arg(getCpuCount()).toLocal8Bit().constData(), 0);
                    #endif

                    // Hack to correct wrong frame rates that seem to be generated by some codecs
                    if (LibavFile->streams[VideoStreamNumber]->codec->time_base.num>1000 && LibavFile->streams[VideoStreamNumber]->codec->time_base.den==1) 
                       LibavFile->streams[VideoStreamNumber]->codec->time_base.den=1000;

                    if (avcodec_open2(LibavFile->streams[VideoStreamNumber]->codec,Codec,&av_opts)>=0)
                    {
                        // Get Aspect Ratio

                        aspectRatio = double(LibavFile->streams[VideoStreamNumber]->CODEC_OR_PAR->sample_aspect_ratio.num)/double(LibavFile->streams[VideoStreamNumber]->CODEC_OR_PAR->sample_aspect_ratio.den);

                        if (LibavFile->streams[VideoStreamNumber]->sample_aspect_ratio.num!=0)
                            aspectRatio = double(LibavFile->streams[VideoStreamNumber]->sample_aspect_ratio.num)/double(LibavFile->streams[VideoStreamNumber]->sample_aspect_ratio.den);

                        if (aspectRatio == 0) 
                           aspectRatio = 1;

                        // Special case for DVD mode video without PAR
                        if ((aspectRatio==1)&&(LibavFile->streams[VideoStreamNumber]->codec->coded_width==720)&&((LibavFile->streams[VideoStreamNumber]->codec->coded_height==576)||(LibavFile->streams[VideoStreamNumber]->codec->coded_height==480)))
                            aspectRatio=double((LibavFile->streams[VideoStreamNumber]->codec->coded_height/3)*4)/720;

                        // Try to load one image to be sure we can make something with this file
                        // and use this first image as thumbnail (if no jukebox thumbnail)
                        int64_t   Position =0;
                        double    dEndFile =double(QTime(0,0,0,0).msecsTo(GetRealDuration()))/1000;    // End File Position in double format
                        if (dEndFile!=0) {
                            // Allocate structure for YUV image

                            FrameBufYUV=ALLOCFRAME();

                            if (FrameBufYUV!=NULL) {

                                AVStream    *VideoStream    =LibavFile->streams[VideoStreamNumber];
                                AVPacket    *StreamPacket   =NULL;
                                bool        Continue        =true;
                                bool        IsVideoFind     =false;
                                double      FrameTimeBase   =av_q2d(VideoStream->time_base);
                                double      FramePosition   =0;

                                while (Continue) {
                                    StreamPacket=new AVPacket();
                                    av_init_packet(StreamPacket);
                                    StreamPacket->flags|=AV_PKT_FLAG_KEY;  // HACK for CorePNG to decode as normal PNG by default
                                    if (av_read_frame(LibavFile,StreamPacket)==0) {
                                        if (StreamPacket->stream_index==VideoStreamNumber) {
                                            int FrameDecoded=0;
                                            if (avcodec_decode_video2(VideoStream->codec,FrameBufYUV,&FrameDecoded,StreamPacket)<0)
                                                ToLog(LOGMSG_INFORMATION,"IN:cVideoFile::OpenCodecAndFile : avcodec_decode_video2 return an error");
                                            if (FrameDecoded>0) {
                                                int64_t pts=AV_NOPTS_VALUE;
                                                if ((FrameBufYUV->pkt_dts==(int64_t)AV_NOPTS_VALUE)&&(FrameBufYUV->pkt_pts!=(int64_t)AV_NOPTS_VALUE)) pts=FrameBufYUV->pkt_pts; else pts=FrameBufYUV->pkt_dts;
                                                if (pts==(int64_t)AV_NOPTS_VALUE) 
                                                   pts=0;
                                                else 
                                                   pts = pts - av_rescale_q(LibavFile->start_time,AV_TIME_BASE_Q,LibavFile->streams[VideoStreamNumber]->time_base);
                                                FramePosition         =double(pts)*FrameTimeBase;
                                                Img                   =ConvertYUVToRGB(false,FrameBufYUV);      // Create Img from YUV Buffer
                                                IsVideoFind           =(Img!=NULL)&&(!Img->isNull());
                                                geometry        =IMAGE_GEOMETRY_UNKNOWN;
                                            }
                                        }
                                        // Check if we need to continue loop
                                        Continue=(IsVideoFind==false)&&(FramePosition<dEndFile);
                                    } else {
                                        // if error in av_read_frame(...) then may be we have reach the end of file !
                                        Continue=false;
                                    }
                                    // Continue with a new one
                                    if (StreamPacket!=NULL) {
                                       AV_FREE_PACKET(StreamPacket); // Free the StreamPacket that was allocated by previous call to av_read_frame
                                        delete StreamPacket;
                                        StreamPacket=NULL;
                                    }
                                }
                                if ((!IsVideoFind)&&(!Img)) {
                                    ToLog(LOGMSG_CRITICAL,QString("No video image return for position %1 => return black frame").arg(Position));
                                    Img=new QImage(LibavFile->streams[VideoStreamNumber]->CODEC_OR_PAR->width,LibavFile->streams[VideoStreamNumber]->CODEC_OR_PAR->height,QImage::Format_ARGB32_Premultiplied);
                                    Img->fill(0);
                                }
                                FREEFRAME(&FrameBufYUV);

                            } else ToLog(LOGMSG_CRITICAL,"Error in cVideoFile::OpenCodecAndFile : Impossible to allocate FrameBufYUV");
                        } else ToLog(LOGMSG_CRITICAL,"Error in cVideoFile::OpenCodecAndFile : dEndFile=0 ?????");
                    }
                    av_dict_free(&av_opts);
                    if (Img) {
                        // Get information about size image
                        imageWidth = Img->width();
                        imageHeight = Img->height();
                        // Compute image geometry
                        geometry = geometryFromSize(imageWidth,imageHeight);
                        // Icon
                        if (Icon->Icon16.isNull()) {
                            QImage Final=(Video_ThumbWidth==162?ApplicationConfig->VideoMask_162:Video_ThumbWidth==150?ApplicationConfig->VideoMask_150:ApplicationConfig->VideoMask_120).copy();
                            QImage ImgF;
                            if (Img->width()>Img->height()) ImgF=Img->scaledToWidth(Video_ThumbWidth-2,Qt::SmoothTransformation);
                                else                        ImgF=Img->scaledToHeight(Video_ThumbHeight*0.7,Qt::SmoothTransformation);
                            QPainter Painter;
                            Painter.begin(&Final);
                            Painter.drawImage(QRect((Final.width()-ImgF.width())/2,(Final.height()-ImgF.height())/2,ImgF.width(),ImgF.height()),ImgF);
                            Painter.end();
                            Icon->LoadIcons(&Final);
                        }
                        delete Img;
                    }

                }

                // Next
                VideoTrackNbr++;

            }

            // Close the video codec
            if (Codec!=NULL) {
                avcodec_close(LibavFile->streams[Track]->codec);
                Codec=NULL;
            }

            //*********************************************************************************************************
            // Thumbnails (since lavf 54.2.0 - avformat.h)
            //*********************************************************************************************************
            if (LibavFile->streams[Track]->disposition & AV_DISPOSITION_ATTACHED_PIC) 
            {
                AVStream *ThumbStream=LibavFile->streams[Track];
                AVPacket pkt         =ThumbStream->attached_pic;
                int      FrameDecoded=0;
                AVFrame  *FrameYUV=ALLOCFRAME();
                if (FrameYUV) {

                    AVCodec *ThumbDecoderCodec=avcodec_find_decoder(ThumbStream->CODEC_OR_PAR->codec_id);

                    // Setup decoder options
                    AVDictionary * av_opts = NULL;
                    #if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
                    ThumbStream->codec->debug_mv         =0;                    // Debug level (0=nothing)
                    ThumbStream->codec->debug            =0;                    // Debug level (0=nothing)
                    ThumbStream->codec->workaround_bugs  =1;                    // Work around bugs in encoders which sometimes cannot be detected automatically : 1=autodetection
                    ThumbStream->codec->idct_algo        =FF_IDCT_AUTO;         // IDCT algorithm, 0=auto
                    ThumbStream->codec->skip_frame       =AVDISCARD_DEFAULT;    // ???????
                    ThumbStream->codec->skip_idct        =AVDISCARD_DEFAULT;    // ???????
                    ThumbStream->codec->skip_loop_filter =AVDISCARD_DEFAULT;    // ???????
                    ThumbStream->codec->error_concealment=3;
                    ThumbStream->codec->thread_count     =getCpuCount();
                    ThumbStream->codec->thread_type      =getThreadFlags(ThumbStream->codec->codec_id);
                    #else
                    av_dict_set(&av_opts, "thread_count", QString("%1").arg(getCpuCount()).toLocal8Bit().constData(), 0);
                    #endif
                    if (avcodec_open2(ThumbStream->codec,ThumbDecoderCodec, &av_opts)>=0) {
                        if ((avcodec_decode_video2(ThumbStream->codec,FrameYUV,&FrameDecoded,&pkt)>=0)&&(FrameDecoded>0)) {
                            int     W=FrameYUV->width, RealW=(W/8)*8;    if (RealW<W) RealW+=8;
                            int     H=FrameYUV->height,RealH=(H/8)*8;    if (RealH<H) RealH+=8;;
                            QImage  Thumbnail(RealW,RealH,QTPIXFMT);
                            AVFrame *FrameRGB=ALLOCFRAME();
                            if ((FrameRGB)&&(!Thumbnail.isNull())) {
                                avpicture_fill((AVPicture *)FrameRGB,Thumbnail.bits(),PIXFMT,RealW,RealH);
                                struct SwsContext *img_convert_ctx=sws_getContext(FrameYUV->width,FrameYUV->height,(AVPixelFormat)FrameYUV->format,RealW,RealH,PIXFMT,SWS_FAST_BILINEAR,NULL,NULL,NULL);
                                if (img_convert_ctx!=NULL) {
                                    int ret = sws_scale(img_convert_ctx,FrameYUV->data,FrameYUV->linesize,0,FrameYUV->height,FrameRGB->data,FrameRGB->linesize);
                                    if (ret>0) {
                                        // sws_scaler truncate the width of the images to a multiple of 8. So cut resulting image to comply a multiple of 8
                                        Thumbnail=Thumbnail.copy(0,0,W,H);
                                        Icon->LoadIcons(&Thumbnail);
                                    }
                                    sws_freeContext(img_convert_ctx);
                                }
                            }
                            if (FrameRGB) FREEFRAME(&FrameRGB);
                        }
                        avcodec_close(ThumbStream->codec);
                    }
                    av_dict_free(&av_opts);
                }
                if (FrameYUV) FREEFRAME(&FrameYUV);
            }
        }

        // if no icon then load default for type
        if (Icon->Icon16.isNull() || Icon->Icon100.isNull())
            Icon->LoadIcons(ObjectType==OBJECTTYPE_VIDEOFILE?&ApplicationConfig->DefaultVIDEOIcon:&ApplicationConfig->DefaultMUSICIcon);
    }

    // Close the libav file
    if (LibavFile!=NULL) 
    {
        avformat_close_input(&LibavFile);
        LibavFile=NULL;
    }

    //Mutex.unlock();
    return Continu;
}

//====================================================================================================================

QString cVideoFile::GetFileTypeStr() 
{
    if (MusicOnly || (ObjectType==OBJECTTYPE_MUSICFILE)) return QApplication::translate("cBaseMediaFile","Music","File type");
        else return QApplication::translate("cBaseMediaFile","Video","File type");
}

//====================================================================================================================

QImage *cVideoFile::GetDefaultTypeIcon(cCustomIcon::IconSize Size) 
{
    if (MusicOnly || (ObjectType==OBJECTTYPE_MUSICFILE)) return ApplicationConfig->DefaultMUSICIcon.GetIcon(Size);
        else return ApplicationConfig->DefaultVIDEOIcon.GetIcon(Size);
}

bool cVideoFile::LoadAnalyseSound(QList<qreal> *Peak, QList<qreal> *Moyenne) 
{
   int64_t RealAudioDuration, RealVideoDuration;
   bool IsOk = ApplicationConfig->FilesTable->GetAnalyseSound(fileKey, Peak, Moyenne, &RealAudioDuration, ObjectType == OBJECTTYPE_VIDEOFILE ? &RealVideoDuration : NULL, &SoundLevel);
   if (IsOk) 
   {
      SetRealAudioDuration(QTime(0,0,0,0).addMSecs(RealAudioDuration));
      if (ObjectType == OBJECTTYPE_VIDEOFILE) 
         SetRealVideoDuration(QTime(0,0,0,0).addMSecs(RealVideoDuration));
   }
   return IsOk;
}

//====================================================================================================================

void cVideoFile::SaveAnalyseSound(QList<qreal> *Peak,QList<qreal> *Moyenne,qreal MaxMoyenneValue) 
{
   int64_t RealVDuration = (ObjectType == OBJECTTYPE_VIDEOFILE) ? QTime(0,0,0,0).msecsTo(GetRealVideoDuration()) : 0;
   SoundLevel = MaxMoyenneValue;
   ApplicationConfig->FilesTable->SetAnalyseSound(fileKey,Peak,Moyenne,QTime(0,0,0,0).msecsTo(GetRealAudioDuration()),(ObjectType == OBJECTTYPE_VIDEOFILE ? &RealVDuration : NULL),SoundLevel);
}

//====================================================================================================================


//====================================================================================================================

QString cVideoFile::GetTechInfo(QStringList *ExtendedProperties) 
{
    QString Info="";
    if (ObjectType==OBJECTTYPE_MUSICFILE) {
        Info=GetCumulInfoStr(ExtendedProperties,"Audio","Codec");
        if (GetCumulInfoStr(ExtendedProperties,"Audio","Channels")!="")       Info=Info+(Info!=""?"-":"")+GetCumulInfoStr(ExtendedProperties,"Audio","Channels");
        if (GetCumulInfoStr(ExtendedProperties,"Audio","Bitrate")!="")        Info=Info+(Info!=""?"-":"")+GetCumulInfoStr(ExtendedProperties,"Audio","Bitrate");
        if (GetCumulInfoStr(ExtendedProperties,"Audio","Frequency")!="")      Info=Info+(Info!=""?"-":"")+GetCumulInfoStr(ExtendedProperties,"Audio","Frequency");
    } else {
        Info=GetImageSizeStr();
        if (GetCumulInfoStr(ExtendedProperties,"Video","Codec")!="")          Info=Info+(Info!=""?"-":"")+GetCumulInfoStr(ExtendedProperties,"Video","Codec");
        if (GetCumulInfoStr(ExtendedProperties,"Video","Frame rate")!="")     Info=Info+(Info!=""?"-":"")+GetCumulInfoStr(ExtendedProperties,"Video","Frame rate");
        if (GetCumulInfoStr(ExtendedProperties,"Video","Bitrate")!="")        Info=Info+(Info!=""?"-":"")+GetCumulInfoStr(ExtendedProperties,"Video","Bitrate");

        int     Num     =0;
        QString TrackNum="";
        QString Value   ="";
        QString SubInfo ="";
        do {
            TrackNum=QString("%1").arg(Num);
            while (TrackNum.length()<3) TrackNum="0"+TrackNum;
            TrackNum="Audio_"+TrackNum+":";
            Value=GetInformationValue(TrackNum+"language",ExtendedProperties);
            if (Value!="") {
                if (Num==0) Info=Info+"-"; else Info=Info+"/";
                SubInfo=GetInformationValue(TrackNum+"Codec",ExtendedProperties);
                if (GetInformationValue(TrackNum+"Channels",ExtendedProperties)!="")  SubInfo=SubInfo+(Info!=""?"-":"")+GetInformationValue(TrackNum+"Channels",ExtendedProperties);
                if (GetInformationValue(TrackNum+"Bitrate",ExtendedProperties)!="")   SubInfo=SubInfo+(Info!=""?"-":"")+GetInformationValue(TrackNum+"Bitrate",ExtendedProperties);
                if (GetInformationValue(TrackNum+"Frequency",ExtendedProperties)!="") SubInfo=SubInfo+(Info!=""?"-":"")+GetInformationValue(TrackNum+"Frequency",ExtendedProperties);
                Info=Info+Value+"("+SubInfo+")";
            }
            // Next
            Num++;
        } while (Value!="");
    }
    return Info;
}

//====================================================================================================================

QString cVideoFile::GetTAGInfo(QStringList *ExtendedProperties) 
{
    QString Info=GetInformationValue("track",ExtendedProperties);
    if (GetInformationValue("title",ExtendedProperties)!="")          Info=Info+(Info!=""?"-":"")+GetInformationValue("title",ExtendedProperties);
    if (GetInformationValue("artist",ExtendedProperties)!="")         Info=Info+(Info!=""?"-":"")+GetInformationValue("artist",ExtendedProperties);
    if (GetInformationValue("album",ExtendedProperties)!="")          Info=Info+(Info!=""?"-":"")+GetInformationValue("album",ExtendedProperties);
    if (GetInformationValue("date",ExtendedProperties)!="")           Info=Info+(Info!=""?"-":"")+GetInformationValue("date",ExtendedProperties);
    if (GetInformationValue("genre",ExtendedProperties)!="")          Info=Info+(Info!=""?"-":"")+GetInformationValue("genre",ExtendedProperties);
    return Info;
}

//====================================================================================================================
// Close LibAVFormat and LibAVCodec contexte for the file
//====================================================================================================================

void cVideoFile::CloseCodecAndFile()
{
   QMutexLocker locker(&accessMutex);
   //Mutex.lock();

#ifdef USE_YUVCACHE_MAP
    foreach (int64_t key, YUVCache.keys())
      delete YUVCache.value(key);
    YUVCache.clear();
#else
    while (CacheImage.count()>0) 
      delete(CacheImage.takeLast());
#endif

    // Close the resampling context
    CloseResampler();

    // Close the filter context
    if (VideoFilterGraph)
        VideoFilter_Close();

    // Close the video codec
    if (VideoDecoderCodec!=NULL) 
    {
        avcodec_close(LibavVideoFile->streams[VideoStreamNumber]->codec);
        VideoDecoderCodec=NULL;
    }

    // Close the audio codec
    if (AudioDecoderCodec!=NULL) {
        avcodec_close(LibavAudioFile->streams[AudioStreamNumber]->codec);
        AudioDecoderCodec=NULL;
    }

    // Close the libav files
    if (LibavAudioFile!=NULL) 
    {
        avformat_close_input(&LibavAudioFile);
        LibavAudioFile=NULL;
    }
    if (LibavVideoFile!=NULL) 
    {
        avformat_close_input(&LibavVideoFile);
        LibavVideoFile=NULL;
    }

    if (FrameBufferYUV!=NULL) 
    {
        FREEFRAME(&FrameBufferYUV);
    }
    FrameBufferYUVReady=false;

    IsOpen=false;
    //Mutex.unlock();
    if(FrameBufferRGB)
      FREEFRAME(&FrameBufferRGB);
    if(img_convert_ctx)
      sws_freeContext(img_convert_ctx);
     img_convert_ctx = 0;
}

//*********************************************************************************************************************

void cVideoFile::CloseResampler() 
{
   if (RSC)
   {
      swr_free(&RSC);
      RSC = NULL;
   }
}

//*********************************************************************************************************************

void cVideoFile::CheckResampler(int RSC_InChannels,int RSC_OutChannels,AVSampleFormat RSC_InSampleFmt,AVSampleFormat RSC_OutSampleFmt,int RSC_InSampleRate,int RSC_OutSampleRate
                                               ,uint64_t RSC_InChannelLayout,uint64_t RSC_OutChannelLayout) 
{
   if (RSC_InChannelLayout == 0)  RSC_InChannelLayout = av_get_default_channel_layout(RSC_InChannels);
   if (RSC_OutChannelLayout == 0) RSC_OutChannelLayout = av_get_default_channel_layout(RSC_OutChannels);
   if ((RSC != NULL) &&
      ((RSC_InChannels != this->RSC_InChannels) || (RSC_OutChannels != this->RSC_OutChannels)
      || (RSC_InSampleFmt != this->RSC_InSampleFmt) || (RSC_OutSampleFmt != this->RSC_OutSampleFmt)
      || (RSC_InSampleRate != this->RSC_InSampleRate) || (RSC_OutSampleRate != this->RSC_OutSampleRate)
      || (RSC_InChannelLayout != this->RSC_InChannelLayout) || (RSC_OutChannelLayout != this->RSC_OutChannelLayout)
      )) CloseResampler();
   if (!RSC)
   {
      this->RSC_InChannels = RSC_InChannels;
      this->RSC_OutChannels = RSC_OutChannels;
      this->RSC_InSampleFmt = RSC_InSampleFmt;
      this->RSC_OutSampleFmt = RSC_OutSampleFmt;
      this->RSC_InSampleRate = RSC_InSampleRate;
      this->RSC_OutSampleRate = RSC_OutSampleRate;

      this->RSC_InChannelLayout = RSC_InChannelLayout;
      this->RSC_OutChannelLayout = RSC_OutChannelLayout;
      /*RSC=swr_alloc_set_opts(NULL,RSC_OutChannelLayout,RSC_OutSampleFmt,RSC_OutSampleRate,
                                  RSC_InChannelLayout, RSC_InSampleFmt, RSC_InSampleRate,
                                  0, NULL);*/
      RSC = swr_alloc();
      av_opt_set_int(RSC, "in_channel_layout", RSC_InChannelLayout, 0);
      av_opt_set_int(RSC, "in_sample_rate", RSC_InSampleRate, 0);
      av_opt_set_int(RSC, "out_channel_layout", RSC_OutChannelLayout, 0);
      av_opt_set_int(RSC, "out_sample_rate", RSC_OutSampleRate, 0);
      av_opt_set_int(RSC, "in_channel_count", RSC_InChannels, 0);
      av_opt_set_int(RSC, "out_channel_count", RSC_OutChannels, 0);
      av_opt_set_sample_fmt(RSC, "in_sample_fmt", RSC_InSampleFmt, 0);
      av_opt_set_sample_fmt(RSC, "out_sample_fmt", RSC_OutSampleFmt, 0);
      if ((RSC) && (swr_init(RSC) < 0))
      {
         ToLog(LOGMSG_CRITICAL, QString("CheckResampler: swr_init failed"));
         swr_free(&RSC);
         RSC = NULL;
      }
      if (!RSC) ToLog(LOGMSG_CRITICAL, QString("CheckResampler: swr_alloc_set_opts failed"));
   }
}

//*********************************************************************************************************************
// VIDEO FILTER PART : This code was adapt from xbmc sources files
//*********************************************************************************************************************

int cVideoFile::VideoFilter_Open() 
{
   int result;

   if (VideoFilterGraph)
      VideoFilter_Close();

   if (!(VideoFilterGraph = avfilter_graph_alloc()))
   {
      ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : unable to alloc filter graph"));
      return -1;
   }

   VideoFilterGraph->scale_sws_opts = av_strdup("flags=4");

   char args[512];
   snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
      LibavVideoFile->streams[VideoStreamNumber]->CODEC_OR_PAR->width, LibavVideoFile->streams[VideoStreamNumber]->CODEC_OR_PAR->height,
      LibavVideoFile->streams[VideoStreamNumber]->CODEC_OR_PAR->CODEC_PIX_FORMAT,
      LibavVideoFile->streams[VideoStreamNumber]->codec->time_base.num, LibavVideoFile->streams[VideoStreamNumber]->codec->time_base.den,
      LibavVideoFile->streams[VideoStreamNumber]->CODEC_OR_PAR->sample_aspect_ratio.num, LibavVideoFile->streams[VideoStreamNumber]->CODEC_OR_PAR->sample_aspect_ratio.den
   );

    const AVFilter *srcFilter = avfilter_get_by_name("buffer");
    const AVFilter *outFilter = avfilter_get_by_name("buffersink");

    if ((result = avfilter_graph_create_filter(&VideoFilterIn, srcFilter, "in", args, NULL, VideoFilterGraph)) < 0)
    {
       ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_create_filter: src"));
       return result;
    }
    if ((result = avfilter_graph_create_filter(&VideoFilterOut, outFilter, "out", NULL, NULL, VideoFilterGraph)) < 0)
    {
       ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_create_filter: out"));
       return result;
    }
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();

    outputs->name = av_strdup("in");
    outputs->filter_ctx = VideoFilterIn;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = VideoFilterOut;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if ((result = avfilter_graph_parse_ptr(VideoFilterGraph, QString("yadif=deint=interlaced:mode=send_frame:parity=auto").toLocal8Bit().constData(), &inputs, &outputs, NULL)) < 0)
    {
       ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_parse"));
       return result;
    }

    if ((result=avfilter_graph_config(VideoFilterGraph,NULL))<0) {
        ToLog(LOGMSG_CRITICAL,QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_config"));
        return result;
    }
    return result;
}

//====================================================================================================================

void cVideoFile::VideoFilter_Close() 
{
   if (VideoFilterGraph) 
      avfilter_graph_free(&VideoFilterGraph);
   VideoFilterGraph = NULL;
   VideoFilterIn = NULL;
   VideoFilterOut = NULL;
}

//====================================================================================================================

bool cVideoFile::SeekFile(AVStream *VideoStream, AVStream *AudioStream, int64_t Position) 
{
   #ifdef SEEK_DEBUG
      qDebug() << "SeekFile " << (VideoStream != 0 ? "Video " : "Audio ") << " to Position " << Position; 
   #endif
   bool ret = true;
   AVFormatContext *LibavFile = NULL;
   int StreamNumber = 0;

   // Reset context variables and buffers
   if (AudioStream) 
   {
      CloseResampler();
      LibavFile = LibavAudioFile;
      StreamNumber = AudioStreamNumber;
   } 
   else if (VideoStream) 
   {
      if (VideoFilterGraph) 
         VideoFilter_Close();
#ifdef USE_YUVCACHE_MAP
      foreach (int64_t key, YUVCache.keys())
         delete YUVCache.value(key);
      YUVCache.clear();
#else
      while (!CacheImage.isEmpty()) 
         delete(CacheImage.takeLast());
#endif
      FrameBufferYUVReady = false;
      FrameBufferYUVPosition = 0;
      LibavFile = LibavVideoFile;
      StreamNumber = VideoStreamNumber;
   }

   if (Position < 0) 
      Position = 0;

   // Flush LibAV buffers
   for (unsigned int i = 0; i < LibavFile->nb_streams; i++)  
   {
      AVCodecContext *codec_context = LibavFile->streams[i]->codec;
      if (codec_context && codec_context->codec) 
         avcodec_flush_buffers(codec_context);
   }
   int64_t seek_target = av_rescale_q(Position,AV_TIME_BASE_Q,LibavFile->streams[StreamNumber]->time_base);
   #ifdef SEEK_DEBUG
   qDebug() << "seek position " << Position << " seek Target " << seek_target;
   #endif
   //seek_target -= LibavStartTime;
   if (seek_target < 0) 
      seek_target = 0;
   int errcode = 0;
   if (SeekErrorCount > 0 || ((errcode = avformat_seek_file(LibavFile,StreamNumber,INT64_MIN,seek_target,INT64_MAX,AVSEEK_FLAG_BACKWARD)) < 0)) 
   {
      if (SeekErrorCount == 0) 
         ToLog(LOGMSG_DEBUGTRACE,GetAvErrorMessage(errcode));
      // Try in AVSEEK_FLAG_ANY mode
      if ((errcode = av_seek_frame(LibavFile,StreamNumber,seek_target,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_ANY)) < 0) 
      {
         ToLog(LOGMSG_DEBUGTRACE,GetAvErrorMessage(errcode));
         // Try with default stream if exist
         int DefaultStream = av_find_default_stream_index(LibavFile);
         if ((DefaultStream == StreamNumber) || (Position > 0) || (DefaultStream < 0) || ((errcode = av_seek_frame(LibavFile,DefaultStream,0,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_BYTE) < 0))) 
         {
            ToLog(LOGMSG_DEBUGTRACE,GetAvErrorMessage(errcode));
            ToLog(LOGMSG_CRITICAL,"Error in cVideoFile::ReadFrame : Seek error");
            ret = false;
         }
      }
   }
   // read first packet to ensure we have correct position !
   // elsewhere, redo seek 5 times until exit with error
   if (AudioStream) 
   {
      AVPacket *StreamPacket = new AVPacket(); 
      av_init_packet(StreamPacket);
      StreamPacket->flags |= AV_PKT_FLAG_KEY;
      int read_error, read_error_count = 0;
      while ( (read_error = av_read_frame(LibavFile,StreamPacket)) != 0 && read_error_count++ < 10 );
      int64_t FramePts = StreamPacket->pts != (int64_t)AV_NOPTS_VALUE ? StreamPacket->pts : -1;
      //if ((FramePts < (Position/1000)-500) || (FramePts > (Position/1000)+500)) 
      if (FramePts < (seek_target-500*1000) || FramePts > (seek_target+500*1000))
      {
         SeekErrorCount++;
         if (SeekErrorCount < 5) 
            ret = SeekFile(VideoStream,AudioStream,Position);
      }
      AV_FREE_PACKET(StreamPacket); // Free the StreamPacket that was allocated here local
      delete StreamPacket;

   }
   return ret;
}

//====================================================================================================================

u_int8_t *cVideoFile::Resample(AVFrame *Frame,int64_t *SizeDecoded,int DstSampleSize) 
{
   u_int8_t *Data = NULL;
   Data = (u_int8_t *)av_malloc(MaxAudioLenDecoded);
   u_int8_t *out[] = { Data };
   if (Data) *SizeDecoded = swr_convert(RSC, out, MaxAudioLenDecoded / DstSampleSize, (const u_int8_t **)Frame->data, Frame->nb_samples)*DstSampleSize;
   //qDebug() << "resample gives " << *SizeDecoded << " bytes";
   return Data;
}

//====================================================================================================================
// return duration of one frame
//====================================================================================================================

qreal cVideoFile::GetFPSDuration() 
{
   qreal FPSDuration;
   if ((VideoStreamNumber >= 0) && (LibavVideoFile->streams[VideoStreamNumber]))
      FPSDuration = qreal(LibavVideoFile->streams[VideoStreamNumber]->r_frame_rate.den*(AV_TIME_BASE/1000))/qreal(LibavVideoFile->streams[VideoStreamNumber]->r_frame_rate.num);
   else 
      FPSDuration = 1;
   return FPSDuration;
}

//====================================================================================================================
// Read a frame from current stream
//====================================================================================================================
// maximum diff between asked image position and founded image position
#define ALLOWEDDELTA    250000
// diff between asked image position and current image position before exit loop and return black frame
#define MAXDELTA        2500000

// Remark: Position must use AV_TIMEBASE Unit
QImage *cVideoFile::ReadFrame(bool PreviewMode, int64_t Position, bool DontUseEndPos, bool Deinterlace, cSoundBlockList *SoundTrackBloc, double Volume, bool ForceSoundOnly, int NbrDuration) 
{
   //qDebug() << "cVideoFile::ReadFrame PreviewMode " << PreviewMode << " Position " << Position << " DontUseEndPos " << DontUseEndPos << " Deinterlace " << Deinterlace << " Volume " << Volume << " NbrDuration " <<NbrDuration;
   //qDebug() << "ReadFrame from " << FileName() << " for Position " << Position;
   // Ensure file was previously open
   if (!IsOpen && !OpenCodecAndFile()) 
      return NULL;

//AUTOTIMER(at,"cVideoFile::ReadFrame");
   //autoTimer at("cVideoFile::ReadFrame");

   //Position += LibavStartTime;//AV_TIME_BASE;
   //LONGLONG pc = curPCounter();
   //LONGLONG pcpart = pc;
   // Ensure file have an end file Position
   double dEndFile = double(QTime(0,0,0,0).msecsTo(DontUseEndPos ? GetRealDuration() : EndTime))/1000.0;
   if (Position < 0) 
      Position = 0;
   Position += LibavStartTime;//AV_TIME_BASE;
   dEndFile += LibavStartTime;
   if (dEndFile == 0) 
   {
      ToLog(LOGMSG_CRITICAL,"Error in cVideoFile::ReadFrame : dEndFile=0 ?????");
      return NULL;
   }

   AVStream *VideoStream = (/*(!MusicOnly)&&(!ForceSoundOnly)&&*/VideoStreamNumber >= 0 ? LibavVideoFile->streams[VideoStreamNumber] : NULL);

   cVideoFile::sAudioContext AudioContext;
   AudioContext.SoundTrackBloc = SoundTrackBloc;
   AudioContext.AudioStream    = (AudioStreamNumber >= 0 && SoundTrackBloc) ? LibavAudioFile->streams[AudioStreamNumber] : NULL;
   AudioContext.FPSSize        = SoundTrackBloc ? SoundTrackBloc->SoundPacketSize*SoundTrackBloc->NbrPacketForFPS : 0;
   AudioContext.FPSDuration    = AudioContext.FPSSize ? (double(AudioContext.FPSSize)/(SoundTrackBloc->Channels*SoundTrackBloc->SampleBytes*SoundTrackBloc->SamplingRate))*AV_TIME_BASE : 0;
   AudioContext.TimeBase       = AudioContext.AudioStream ? double(AudioContext.AudioStream->time_base.den)/double(AudioContext.AudioStream->time_base.num) : 0;
   AudioContext.DstSampleSize  = SoundTrackBloc ? (SoundTrackBloc->SampleBytes*SoundTrackBloc->Channels) : 0;
   AudioContext.NeedResampling = false;
   AudioContext.AudioLenDecoded= 0;
   AudioContext.Counter        = 20; // Retry counter (when len>0 and avcodec_decode_audio4 fail to retreave frame, we retry counter time before to discard the packet)
   AudioContext.Volume         = Volume;
   AudioContext.dEndFile       = &dEndFile;
   AudioContext.NbrDuration    = NbrDuration;
   AudioContext.DontUseEndPos  = DontUseEndPos;

   if (!AudioContext.FPSDuration) 
   {
      if (PreviewMode)            
         AudioContext.FPSDuration = double(AV_TIME_BASE) / ((cApplicationConfig *)ApplicationConfig)->PreviewFPS;
      else if (VideoStream)   
         AudioContext.FPSDuration = double(VideoStream->r_frame_rate.den * AV_TIME_BASE) / double(VideoStream->r_frame_rate.num);
      else                    
         AudioContext.FPSDuration = double(AV_TIME_BASE) / double(SoundTrackBloc->SamplingRate);
   }

   if (!AudioContext.AudioStream && !VideoStream)
      return NULL;

   //autoTimer atAV("cVideoFile::ReadFrame all");
   //QMutexLocker locker(&Mutex);
   QMutexLocker locker(&accessMutex);
   //Mutex.lock();

   // If position >= end of file : disable audio (only if IsComputedAudioDuration)
   double dPosition = double(Position) / AV_TIME_BASE;
   //if ((dPosition > 0) && (dPosition >= dEndFile) && (IsComputedAudioDuration)) 
   if (dPosition > 0 && dPosition >= dEndFile+1000 && IsComputedAudioDuration) 
   {
      AudioContext.AudioStream = NULL; // Disable audio
      // Check if last image is ready and correspond to end of file
      if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= dEndFile*AV_TIME_BASE-AudioContext.FPSDuration)
      {
         //Mutex.unlock();
         return new QImage(LastImage.copy());
      }
      // If not then change Position to end file - a FPS to prepare a last image
      Position = dEndFile * AV_TIME_BASE-AudioContext.FPSDuration;
      dPosition = double(Position) / AV_TIME_BASE;
      if (SoundTrackBloc) 
         SoundTrackBloc->UseLatestData();
   }

   //================================================
   bool ContinueVideo = true;
   AudioContext.ContinueAudio = (AudioContext.AudioStream) && (SoundTrackBloc);
   bool ResamplingContinue = (Position != 0);
   AudioContext.AudioFramePosition = dPosition;
   //================================================

   if (AudioContext.ContinueAudio) 
   {
      AudioContext.NeedResampling = ((AudioContext.AudioStream->codec->sample_fmt != AV_SAMPLE_FMT_S16)
         || (AudioContext.AudioStream->CODEC_OR_PAR->channels != SoundTrackBloc->Channels)
         || (AudioContext.AudioStream->CODEC_OR_PAR->sample_rate != SoundTrackBloc->SamplingRate));

      //qDebug() << "audio needs resampling " << AudioContext.NeedResampling;
      // Calc if we need to seek to a position
      int64_t Start = /*SoundTrackBloc->LastReadPosition;*/SoundTrackBloc->CurrentPosition;
      int64_t End   = Start+SoundTrackBloc->GetDuration();
      int64_t Wanted = AudioContext.FPSDuration * AudioContext.NbrDuration;
      //qDebug() << "Start " << Start << " End " << End << " Position " << Position << " Wanted " << Wanted;
      if ( (Position >= Start && Position + Wanted <= End) /*|| Start < 0 */) 
         AudioContext.ContinueAudio = false;
      if ( AudioContext.ContinueAudio && ( Position == 0 || Start < 0 || LastAudioReadPosition < 0 /*|| Position < Start*/ || Position > End + 1500000 ) ) 
      {
         if (Position < 0) 
            Position = 0;
         SoundTrackBloc->ClearList();                // Clear soundtrack list
         ResamplingContinue = false;
         LastAudioReadPosition = 0;
         SeekErrorCount = 0;
         int64_t seekPos = Position - SoundTrackBloc->WantedDuration * 1000.0;
         /*bool bSeekRet = */SeekFile(NULL,AudioContext.AudioStream,seekPos);        // Always seek one FPS before to ensure eventual filter have time to init
         //qDebug() << "seek to " << seekPos << " is " << (bSeekRet?"true":"false");
         AudioContext.AudioFramePosition = Position / AV_TIME_BASE;
      }

      // Prepare resampler
      if (AudioContext.ContinueAudio && AudioContext.NeedResampling)
      {
         if (!ResamplingContinue) 
            CloseResampler();
         CheckResampler(AudioContext.AudioStream->CODEC_OR_PAR->channels,SoundTrackBloc->Channels,
            AVSampleFormat(AudioContext.AudioStream->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT),SoundTrackBloc->SampleFormat,
            AudioContext.AudioStream->CODEC_OR_PAR->sample_rate,SoundTrackBloc->SamplingRate
            ,AudioContext.AudioStream->CODEC_OR_PAR->channel_layout
            ,av_get_default_channel_layout(SoundTrackBloc->Channels)
            );
      }
   }

   QImage *RetImage = NULL;
   int64_t RetImagePosition = 0;
   double VideoFramePosition = dPosition;

   // Count number of image > position
   bool IsVideoFind = false;
#ifdef USE_YUVCACHE_MAP
   if( !YUVCache.isEmpty() /*&& YUVCache.lastKey() >= Position*/ )
   {
      QMap<int64_t, cImageInCache *>::const_iterator i = YUVCache.constBegin();
      while (i != YUVCache.constEnd() && !IsVideoFind) 
      {
         if( i.key() >= Position && i.key() - Position < ALLOWEDDELTA ) 
            IsVideoFind = true;
          ++i;
      }      
   }
#else
   for (int CNbr = 0; !IsVideoFind && CNbr < CacheImage.count(); CNbr++) 
      if (CacheImage[CNbr]->Position >= Position && CacheImage[CNbr]->Position - Position < ALLOWEDDELTA) 
         IsVideoFind = true;
#endif
   ContinueVideo = (VideoStream && !IsVideoFind && !ForceSoundOnly);
   if (ContinueVideo) 
   {
      int64_t DiffTimePosition = -1000000;  // Compute difftime between asked position and previous end decoded position

      if (FrameBufferYUVReady) 
      {
         DiffTimePosition = Position - FrameBufferYUVPosition;
         //if ((Position==0)||(DiffTimePosition<0)||(DiffTimePosition>1500000))
         //    ToLog(LOGMSG_INFORMATION,QString("VIDEO-SEEK %1 TO %2").arg(ShortName).arg(Position));
      }

      // Calc if we need to seek to a position
      if (Position == 0 || DiffTimePosition < 0 || DiffTimePosition > 1500000) // Allow 1,5 sec diff (rounded double !)
      {
         if (Position < 0) 
            Position = 0;
         SeekErrorCount = 0;
         SeekFile(VideoStream,NULL,Position);        // Always seek one FPS before to ensure eventual filter have time to init
         VideoFramePosition = Position / AV_TIME_BASE;
      }
   }

   //*************************************************************************************************************************************
   // Decoding process : Get StreamPacket until endposition is reach (if sound is wanted) or until image is ok (if image only is wanted)
   //*************************************************************************************************************************************
   //qDebug() << "readFrame prep " << PC2time(curPCounter()-pcpart,true);
   //pcpart = curPCounter();

   // AUDIO PART
   //QFutureWatcher<void> ThreadAudio;
   {
   //AUTOTIMER(atAudio,"cVideoFile::ReadFrame audio Part");
   //autoTimer atAudio("cVideoFile::ReadFrame audio Part");
   //if( !AudioContext.ContinueAudio )
   //   qDebug() << "cVideoFile::ReadFrame audio, AudioContext.ContinueAudio is false!!!!!";

   while (AudioContext.ContinueAudio) 
   {
      AVPacket *StreamPacket = new AVPacket();
      if (!StreamPacket) 
      {
         AudioContext.ContinueAudio = false;
      } 
      else 
      {
         av_init_packet(StreamPacket);
         StreamPacket->flags |= AV_PKT_FLAG_KEY;
         int err;
         if ( (err = av_read_frame(LibavAudioFile,StreamPacket)) < 0) 
         {
            // If error reading frame then we considere we have reach the end of file
            if (!IsComputedAudioDuration) 
            {
               dEndFile = qreal(SoundTrackBloc->/*LastReadPosition*/CurrentPosition) / AV_TIME_BASE;
               dEndFile += qreal(SoundTrackBloc->GetDuration()) / 1000;
               if (dEndFile-LibavStartTime/ AV_TIME_BASE == double(QTime(0,0,0,0).msecsTo(EndTime)) / 1000) 
                  EndTime = QTime(0,0,0).addMSecs((dEndFile-LibavStartTime)*1000);
               SetRealAudioDuration(QTime(0,0,0,0).addMSecs(qlonglong((dEndFile-LibavStartTime/ AV_TIME_BASE)*1000)));
            }
            AudioContext.ContinueAudio = false;
            //qDebug() << "cVideoFile::ReadFrame audio, set AudioContext.ContinueAudio to false!!!!! err is "<<err;

            // Use data in TempData to create a latest block
            SoundTrackBloc->UseLatestData();
         } 
         else 
         {
            DecodeAudio(&AudioContext,StreamPacket,Position);
            StreamPacket = NULL;
         }
      }
      // Continue with a new one
      if (StreamPacket != NULL) 
      {
         AV_FREE_PACKET(StreamPacket); // Free the StreamPacket that was allocated by previous call to av_read_frame
         delete StreamPacket;
         StreamPacket = NULL;
      }
   }
   }
   //qDebug() << "readFrame audio " << PC2time(curPCounter()-pcpart,true);
   //pcpart = curPCounter();

   // VIDEO PART
   if (VideoStream && !ForceSoundOnly)
   {
      //AUTOTIMER(atAudio,"cVideoFile::ReadFrame video Part");
      //autoTimer atVideo("cVideoFile::ReadFrame video Part");
      //LONGLONG pcpartv = curPCounter();
      if (!ContinueVideo) 
      {
         ToLog(LOGMSG_DEBUGTRACE,QString("Video image for position %1 => use image in cache").arg(Position));
      } 
      else if (Position < LibavStartTime) 
      {
         ToLog(LOGMSG_CRITICAL,QString("Image position %1 is before video stream start => return black frame").arg(Position));
         RetImage = new QImage(LibavVideoFile->streams[VideoStreamNumber]->codec->width,LibavVideoFile->streams[VideoStreamNumber]->codec->height,QImage::Format_ARGB32_Premultiplied);
         RetImage->fill(0);
         RetImagePosition = Position;
      } 
      else 
      {
#ifdef USE_YUVCACHE_MAP
         bool ByPassFirstImage = (Deinterlace) && (YUVCache.count() == 0);
#else
         bool ByPassFirstImage = (Deinterlace) && (CacheImage.count() == 0);
#endif
         int MaxErrorCount = 20;
         bool FreeFrames = false;

         while (ContinueVideo) 
         {
            AVPacket *StreamPacket = new AVPacket();
            if (!StreamPacket) 
            {
               ContinueVideo = false;
            } 
            else 
            {
               av_init_packet(StreamPacket);
               StreamPacket->flags |= AV_PKT_FLAG_KEY;  // HACK for CorePNG to decode as normal PNG by default

               int errcode = 0;
               if ((errcode = av_read_frame(LibavVideoFile,StreamPacket)) < 0) 
               {
                  if (errcode == AVERROR_EOF) 
                  {
                     // We have reach the end of file
                     if (!IsComputedAudioDuration) 
                     {
                        dEndFile = VideoFramePosition;
                        if (dEndFile-LibavStartTime == double(QTime(0,0,0,0).msecsTo(EndTime))/1000) 
                           EndTime = QTime(0,0,0).addMSecs((dEndFile-LibavStartTime)*1000.0);
                        SetRealVideoDuration(QTime(0,0,0,0).addMSecs(qlonglong((dEndFile-LibavStartTime)*1000)));
                     }
                     ContinueVideo = false;

                     if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= (dEndFile-1.5)*AV_TIME_BASE) 
                     {
                        if (!RetImage) 
                        {
                           RetImage=new QImage(LastImage);
                           RetImagePosition = FrameBufferYUVPosition;
                        }
                        IsVideoFind = true;
                        ContinueVideo = false;
                     }
                  } 
                  else 
                  {
                     ToLog(LOGMSG_CRITICAL,GetAvErrorMessage(errcode));
                     // If error reading frame
                     if (MaxErrorCount > 0) 
                     {
                        // Files with stream could provoque this, so we ignore the first MaxErrorCount errors
                        MaxErrorCount--;
                     } 
                     else 
                     {
                        if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= (dEndFile-1.5)*AV_TIME_BASE) 
                        {
                           if (!RetImage) 
                           {
                              RetImage = new QImage(LastImage);
                              RetImagePosition = FrameBufferYUVPosition;
                           }
                           IsVideoFind = true;
                           ContinueVideo = false;
                        } 
                        else 
                        {
                           SeekErrorCount = 0;
                           ContinueVideo = SeekFile(VideoStream,NULL,Position-2*AudioContext.FPSDuration);
                        }
                     }
                  }
               } 
               else 
               {
                  int64_t FramePts = StreamPacket->pts != (int64_t)AV_NOPTS_VALUE ? StreamPacket->pts : -1;
                  double TimeBase = double(LibavVideoFile->streams[StreamPacket->stream_index]->time_base.den)/double(LibavVideoFile->streams[StreamPacket->stream_index]->time_base.num);
                  if (FramePts >= 0) 
                     VideoFramePosition = (double(FramePts)/TimeBase);

                  if (StreamPacket->stream_index == VideoStreamNumber) 
                  {
                     // Allocate structures
                     if (FrameBufferYUV == NULL) 
                        FrameBufferYUV = ALLOCFRAME();
                     if (FrameBufferYUV) 
                     {
                        int FrameDecoded = 0;
                        LastLibAvMessageLevel = 0;    // Clear LastLibAvMessageLevel : some decoder dont return error but display errors messages !
                        int Error = avcodec_decode_video2(VideoStream->codec,FrameBufferYUV,&FrameDecoded,StreamPacket);
                        if (Error < 0 || LastLibAvMessageLevel == LOGMSG_CRITICAL) 
                        {
                           if (MaxErrorCount > 0) 
                           {
                              if (VideoFramePosition*1000000.0 < Position) 
                              {
                                 ToLog(LOGMSG_INFORMATION,QString("IN:cVideoFile::ReadFrame - Error decoding packet: try left %1").arg(MaxErrorCount));
                              } 
                              else 
                              {
                                 ToLog(LOGMSG_INFORMATION,QString("IN:cVideoFile::ReadFrame - Error decoding packet: seek to backward and restart reading"));
                                 if (Position > 1000000) 
                                 {
                                    SeekErrorCount = 0;
                                    SeekFile(VideoStream,NULL/*AudioStream*/,Position-1000000); // 1 sec before
                                 } 
                                 else 
                                 {
                                    SeekErrorCount = 0;
                                    SeekFile(VideoStream,NULL,0);
                                 }
                              }
                              MaxErrorCount--;
                           } 
                           else 
                           {
                              ToLog(LOGMSG_CRITICAL,QString("IN:cVideoFile::ReadFrame - Error decoding packet: and no try left"));
                              ContinueVideo = false;
                           }
                        } 
                        else 
                        if (FrameDecoded > 0) 
                        {

                           FrameBufferYUV->pkt_pts = av_frame_get_best_effort_timestamp(FrameBufferYUV);
                           // Video filter part
                           if (Deinterlace && !VideoFilterGraph)
                              VideoFilter_Open();
                           else if (!Deinterlace && VideoFilterGraph)
                              VideoFilter_Close();

                           AVFrame *FiltFrame = NULL;
                           if (VideoFilterGraph) 
                           {
                              // FFMPEG 2.0
                              // push the decoded frame into the filtergraph
                              if (av_buffersrc_add_frame_flags(VideoFilterIn,FrameBufferYUV,AV_BUFFERSRC_FLAG_KEEP_REF) < 0) 
                              {
                                 ToLog(LOGMSG_INFORMATION,"IN:cVideoFile::ReadFrame : Error while feeding the filtergraph");
                              } 
                              else 
                              {
                                 FiltFrame = av_frame_alloc();
                                 // pull filtered frames from the filtergraph
                                 int ret = av_buffersink_get_frame(VideoFilterOut,FiltFrame);
                                 if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                                 {
                                    ToLog(LOGMSG_INFORMATION,"IN:cVideoFile::ReadFrame : No image return by filter process");
                                    av_frame_unref(FiltFrame);
                                    av_frame_free(&FiltFrame);
                                    FiltFrame = NULL;
                                 }
                              }
                           }
                           if (ByPassFirstImage) 
                           {
                              ByPassFirstImage = false;
                              FreeFrames       = true;
                           } 
                           else 
                           {
                              int64_t pts = FrameBufferYUV->pkt_pts;
                              if (pts == (int64_t)AV_NOPTS_VALUE) 
                              {
                                 if (FrameBufferYUV->pkt_dts != (int64_t)AV_NOPTS_VALUE) 
                                 {
                                    pts = FrameBufferYUV->pkt_dts;
                                    ToLog(LOGMSG_DEBUGTRACE,QString("IN:cVideoFile::ReadFrame : No PTS so use DTS %1 for position %2").arg(pts).arg(Position));
                                 } 
                                 else 
                                 {
                                    pts = 0;
                                    ToLog(LOGMSG_DEBUGTRACE,QString("IN:cVideoFile::ReadFrame : No PTS and no DTS for position %1").arg(Position));
                                 }
                              }
                              FrameBufferYUVReady = true;                                            // Keep actual value for FrameBufferYUV
                              FrameBufferYUVPosition = int64_t((qreal(pts)*av_q2d(VideoStream->time_base))*AV_TIME_BASE);    // Keep actual value for FrameBufferYUV
                              // Append this frame
                              cImageInCache *ObjImage=
                                 new cImageInCache(FrameBufferYUVPosition,FiltFrame,FrameBufferYUV);
                              FreeFrames = false;
                              IsVideoFind = false;
#ifdef USE_YUVCACHE_MAP
                              YUVCache[FrameBufferYUVPosition] = ObjImage;
                              if( YUVCache.lastKey() >= Position )
                                 IsVideoFind = true;
#else
                              int ToIns = 0;
                              while (ToIns < CacheImage.count() && CacheImage.at(ToIns)->Position < ObjImage->Position)
                                 ToIns++;
                              if (ToIns < CacheImage.count()) 
                              {
                                 CacheImage.insert(ToIns,ObjImage);
                                 //ToLog(LOGMSG_DEBUGTRACE,QString("IN:cVideoFile::ReadFrame : Insert image %1 for position %2 (FramePosition=%3) - Key:%4 PTS:%5 Num:%6").arg(FrameBufferYUVPosition).arg(Position).arg(VideoFramePosition).arg(FrameBufferYUV->key_frame).arg(FrameBufferYUV->pkt_pts).arg(FrameBufferYUV->coded_picture_number));
                              } 
                              else 
                              {
                                 CacheImage.append(ObjImage);
                                 //ToLog(LOGMSG_DEBUGTRACE,QString("IN:cVideoFile::ReadFrame : Append image %1 for position %2 (FramePosition=%3) - Key:%4 PTS:%5 Num:%6").arg(FrameBufferYUVPosition).arg(Position).arg(VideoFramePosition).arg(FrameBufferYUV->key_frame).arg(FrameBufferYUV->pkt_pts).arg(FrameBufferYUV->coded_picture_number));
                              }
                              // Count number of image > position
                              int Nbr = 0;
                              for (int CNbr = 0; CNbr < CacheImage.count(); CNbr++) 
                                 if (CacheImage.at(CNbr)->Position >= Position && CacheImage.at(CNbr)->Position-Position < ALLOWEDDELTA)
                                    Nbr++;
                              IsVideoFind = Nbr > 0;
#endif
                           }
                           if (FreeFrames) 
                           {
                              if (FiltFrame) 
                              {
                                 av_frame_unref(FiltFrame);
                                 av_frame_free(&FiltFrame);
                                 //FiltFrame = NULL; // added
                                 av_frame_unref(FrameBufferYUV);
                              }
                              FREEFRAME(&FrameBufferYUV);
                           } 
                           else 
                           {
                              FrameBufferYUV = NULL;
                              FiltFrame     = NULL;
                           }
                        }
                     }
                  }
               }
               // Check if we need to continue loop
               // Note: FPSDuration*(!VideoStream?2:1) is to enhance preview speed
               ContinueVideo = ContinueVideo && (VideoStream && !IsVideoFind && (VideoFramePosition*1000000 < Position
                  || VideoFramePosition*1000000-Position < MAXDELTA));
            }

            // Continue with a new one
            if (StreamPacket != NULL) 
            {
               AV_FREE_PACKET(StreamPacket); // Free the StreamPacket that was allocated by previous call to av_read_frame
               delete StreamPacket;
               StreamPacket = NULL;
            }
         }
      }
      //qDebug() << "readFrame video stage1 " << PC2time(curPCounter()-pcpartv,true);
      //pcpartv = curPCounter();

#ifdef USE_YUVCACHE_MAP
      if( !RetImage && YUVCache.count() > 0 )
      {
         QMap<int64_t, cImageInCache *>::const_iterator i = YUVCache.lowerBound(Position - MAXDELTA);
         QMap<int64_t, cImageInCache *>::const_iterator upperBound = YUVCache.upperBound(Position + MAXDELTA);
         QMap<int64_t, cImageInCache *>::const_iterator nearest = YUVCache.end();
         int64_t Nearest = MAXDELTA;
         while (i != upperBound) 
         {
            if( abs(Position - i.key()) < Nearest )
            {
               nearest = i;
               Nearest = abs(Position - i.key());
            }
            ++i;
         }
         if ( nearest != YUVCache.end()) 
         {
            //autoTimer at("cVideoFile::ConvertYUVToRGB");
            RetImage = ConvertYUVToRGB(PreviewMode, nearest.value()->FiltFrame ? nearest.value()->FiltFrame : nearest.value()->FrameBufferYUV);
            RetImagePosition = nearest.key();
            //ToLog(LOGMSG_DEBUGTRACE,QString("Video image for position %1 => return image at %2").arg(Position).arg(CacheImage[i]->Position));
            //qDebug() << QString("Video image for position %1 => return image at %2").arg(Position).arg(RetImagePosition);
         } 
         else 
         {
            ToLog(LOGMSG_CRITICAL,QString("No video image return for position %1 => return image at %2").arg(Position).arg(YUVCache.first()->Position));
            RetImage = ConvertYUVToRGB(PreviewMode,YUVCache.first()->FiltFrame ? YUVCache.first()->FiltFrame : YUVCache.first()->FrameBufferYUV);
            RetImagePosition = YUVCache.first()->Position;
         }
      }
#else
      if (!RetImage && CacheImage.count() > 0) 
      {
         //AUTOTIMER(fc,"Image from YUV-Cache");
         // search nearest image (allowed up to MAXDELTA, after return black frame)
         int i = -1, Nearest = MAXDELTA;
         for (int jj = 0; jj < CacheImage.count(); jj++) 
         {
            if (CacheImage.at(jj)->Position >= Position && CacheImage.at(jj)->Position-Position < MAXDELTA)
            {
               if (i == -1 || CacheImage.at(jj)->Position-Position < Nearest)
               {
                  i = jj;
                  Nearest = CacheImage.at(jj)->Position-Position;
               }
            }
         }
         if ( i >= 0 && i < CacheImage.count()/*&&(CacheImage[i]->Position>=Position)&&(CacheImage[i]->Position-Position<100000)*/) 
         {
            //AUTOTIMER(cyuv,"ConvertYUVToRGB");
            RetImage = ConvertYUVToRGB(PreviewMode,CacheImage.at(i)->FiltFrame ? CacheImage.at(i)->FiltFrame : CacheImage.at(i)->FrameBufferYUV);
            RetImagePosition = CacheImage.at(i)->Position;
            //ToLog(LOGMSG_DEBUGTRACE,QString("Video image for position %1 => return image at %2").arg(Position).arg(CacheImage[i]->Position));
         } 
         else 
         {
            ToLog(LOGMSG_CRITICAL,QString("No video image return for position %1 => return image at %2").arg(Position).arg(CacheImage[0]->Position));
            RetImage = ConvertYUVToRGB(PreviewMode,CacheImage[0]->FiltFrame?CacheImage[0]->FiltFrame:CacheImage[0]->FrameBufferYUV);
            RetImagePosition = CacheImage.at(0)->Position;
         }
      }
#endif
      //qDebug() << "readFrame video stage2 " << PC2time(curPCounter()-pcpartv,true);
      //pcpartv = curPCounter();

      if (!RetImage) 
      {
         ToLog(LOGMSG_CRITICAL,QString("No video image return for position %1 => return black frame").arg(Position));
         RetImage = new QImage(LibavVideoFile->streams[VideoStreamNumber]->CODEC_OR_PAR->width,LibavVideoFile->streams[VideoStreamNumber]->CODEC_OR_PAR->height,QImage::Format_ARGB32_Premultiplied);
         RetImage->fill(0);
         RetImagePosition = Position;
      }
      int64_t cutoff = Position-50000;
#ifdef USE_YUVCACHE_MAP
      QMutableMapIterator<int64_t, cImageInCache *> mutmapiter(YUVCache);
      while( mutmapiter.hasNext() )
      {
         mutmapiter.next();
         if( mutmapiter.key() < cutoff )
         {
            delete mutmapiter.value();
            mutmapiter.remove();
         }
         else
            break;
      }
#else
      int i = 0;
      while (i < CacheImage.count()) 
      {
         if (CacheImage.at(i)->Position < cutoff) 
            delete CacheImage.takeAt(i);
         else 
            i++;
      }
#endif
   }
   //qDebug() << "readFrame video " << PC2time(curPCounter()-pcpart,true);
   //pcpart = curPCounter();

#ifdef USE_YUVCACHE_MAP
   if (AudioContext.AudioStream && SoundTrackBloc && YUVCache.count() > 0)
      SoundTrackBloc->AdjustSoundPosition(RetImagePosition);
#else
   if (AudioContext.AudioStream && SoundTrackBloc && CacheImage.count() > 0)
      SoundTrackBloc->AdjustSoundPosition(RetImagePosition);
#endif

   //Mutex.unlock();
   //qDebug() << "readFrame " << PC2time(curPCounter()-pc,true) ;
   return RetImage;
}

AVFrame *cVideoFile::ReadYUVFrame(bool PreviewMode, int64_t Position, bool DontUseEndPos, bool Deinterlace, cSoundBlockList *SoundTrackBloc, double Volume, bool ForceSoundOnly, int NbrDuration)
{
   //qDebug() << "cVideoFile::ReadFrame PreviewMode " << PreviewMode << " Position " << Position << " DontUseEndPos " << DontUseEndPos << " Deinterlace " << Deinterlace << " Volume " << Volume << " NbrDuration " <<NbrDuration;
   //qDebug() << "ReadYUVFrame from " << FileName() << " for Position " << Position;
   // Ensure file was previously open
   if (!IsOpen && !OpenCodecAndFile())
      return NULL;

   //AUTOTIMER(at,"cVideoFile::ReadFrame");
   //autoTimer at("cVideoFile::ReadFrame");

   //Position += LibavStartTime;//AV_TIME_BASE;
   //LONGLONG pc = curPCounter();
   //LONGLONG pcpart = pc;
   // Ensure file have an end file Position
   double dEndFile = double(QTime(0, 0, 0, 0).msecsTo(DontUseEndPos ? GetRealDuration() : EndTime)) / 1000.0;
   if (Position < 0)
      Position = 0;
   Position += LibavStartTime;//AV_TIME_BASE;
   dEndFile += LibavStartTime;
   if (dEndFile == 0)
   {
      ToLog(LOGMSG_CRITICAL, "Error in cVideoFile::ReadFrame : dEndFile=0 ?????");
      return NULL;
   }

   AVStream *VideoStream = (/*(!MusicOnly)&&(!ForceSoundOnly)&&*/VideoStreamNumber >= 0 ? LibavVideoFile->streams[VideoStreamNumber] : NULL);

   cVideoFile::sAudioContext AudioContext;
   AudioContext.SoundTrackBloc = SoundTrackBloc;
   AudioContext.AudioStream = (AudioStreamNumber >= 0 && SoundTrackBloc) ? LibavAudioFile->streams[AudioStreamNumber] : NULL;
   AudioContext.FPSSize = SoundTrackBloc ? SoundTrackBloc->SoundPacketSize*SoundTrackBloc->NbrPacketForFPS : 0;
   AudioContext.FPSDuration = AudioContext.FPSSize ? (double(AudioContext.FPSSize) / (SoundTrackBloc->Channels*SoundTrackBloc->SampleBytes*SoundTrackBloc->SamplingRate))*AV_TIME_BASE : 0;
   AudioContext.TimeBase = AudioContext.AudioStream ? double(AudioContext.AudioStream->time_base.den) / double(AudioContext.AudioStream->time_base.num) : 0;
   AudioContext.DstSampleSize = SoundTrackBloc ? (SoundTrackBloc->SampleBytes*SoundTrackBloc->Channels) : 0;
   AudioContext.NeedResampling = false;
   AudioContext.AudioLenDecoded = 0;
   AudioContext.Counter = 20; // Retry counter (when len>0 and avcodec_decode_audio4 fail to retreave frame, we retry counter time before to discard the packet)
   AudioContext.Volume = Volume;
   AudioContext.dEndFile = &dEndFile;
   AudioContext.NbrDuration = NbrDuration;
   AudioContext.DontUseEndPos = DontUseEndPos;

   if (!AudioContext.FPSDuration)
   {
      if (PreviewMode)
         AudioContext.FPSDuration = double(AV_TIME_BASE) / ((cApplicationConfig *)ApplicationConfig)->PreviewFPS;
      else if (VideoStream)
         AudioContext.FPSDuration = double(VideoStream->r_frame_rate.den * AV_TIME_BASE) / double(VideoStream->r_frame_rate.num);
      else
         AudioContext.FPSDuration = double(AV_TIME_BASE) / double(SoundTrackBloc->SamplingRate);
   }

   if (!AudioContext.AudioStream && !VideoStream)
      return NULL;

   //autoTimer atAV("cVideoFile::ReadFrame all");
   QMutexLocker locker(&accessMutex);
   //Mutex.lock();

   // If position >= end of file : disable audio (only if IsComputedAudioDuration)
   double dPosition = double(Position) / AV_TIME_BASE;
   //if ((dPosition > 0) && (dPosition >= dEndFile) && (IsComputedAudioDuration)) 
   if (dPosition > 0 && dPosition >= dEndFile + 1000 && IsComputedAudioDuration)
   {
      AudioContext.AudioStream = NULL; // Disable audio
                                       // Check if last image is ready and correspond to end of file
      if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= dEndFile*AV_TIME_BASE - AudioContext.FPSDuration)
      {
         //Mutex.unlock();
         return NULL;//QImage(LastImage.copy());
      }
      // If not then change Position to end file - a FPS to prepare a last image
      Position = dEndFile * AV_TIME_BASE - AudioContext.FPSDuration;
      dPosition = double(Position) / AV_TIME_BASE;
      if (SoundTrackBloc)
         SoundTrackBloc->UseLatestData();
   }

   //================================================
   bool ContinueVideo = true;
   AudioContext.ContinueAudio = (AudioContext.AudioStream) && (SoundTrackBloc);
   bool ResamplingContinue = (Position != 0);
   AudioContext.AudioFramePosition = dPosition;
   //================================================

   if (AudioContext.ContinueAudio)
   {
      AudioContext.NeedResampling = ((AudioContext.AudioStream->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_S16)
         || (AudioContext.AudioStream->CODEC_OR_PAR->channels != SoundTrackBloc->Channels)
         || (AudioContext.AudioStream->CODEC_OR_PAR->sample_rate != SoundTrackBloc->SamplingRate));

      //qDebug() << "audio needs resampling " << AudioContext.NeedResampling;
      // Calc if we need to seek to a position
      int64_t Start = /*SoundTrackBloc->LastReadPosition;*/SoundTrackBloc->CurrentPosition;
      int64_t End = Start + SoundTrackBloc->GetDuration();
      int64_t Wanted = AudioContext.FPSDuration * AudioContext.NbrDuration;
      //qDebug() << "Start " << Start << " End " << End << " Position " << Position << " Wanted " << Wanted;
      if ((Position >= Start && Position + Wanted <= End) /*|| Start < 0 */)
         AudioContext.ContinueAudio = false;
      if (AudioContext.ContinueAudio && (Position == 0 || Start < 0 || LastAudioReadPosition < 0 /*|| Position < Start*/ || Position > End + 1500000))
      {
         if (Position < 0)
            Position = 0;
         SoundTrackBloc->ClearList();                // Clear soundtrack list
         ResamplingContinue = false;
         LastAudioReadPosition = 0;
         SeekErrorCount = 0;
         int64_t seekPos = Position - SoundTrackBloc->WantedDuration * 1000.0;
         /*bool bSeekRet = */SeekFile(NULL, AudioContext.AudioStream, seekPos);        // Always seek one FPS before to ensure eventual filter have time to init
                                                                                       //qDebug() << "seek to " << seekPos << " is " << (bSeekRet?"true":"false");
         AudioContext.AudioFramePosition = Position / AV_TIME_BASE;
      }

      // Prepare resampler
      if (AudioContext.ContinueAudio && AudioContext.NeedResampling)
      {
         if (!ResamplingContinue)
            CloseResampler();
         CheckResampler(AudioContext.AudioStream->codec->channels, SoundTrackBloc->Channels,
            AudioContext.AudioStream->codec->sample_fmt, SoundTrackBloc->SampleFormat,
            AudioContext.AudioStream->codec->sample_rate, SoundTrackBloc->SamplingRate
            , AudioContext.AudioStream->codec->channel_layout
            , av_get_default_channel_layout(SoundTrackBloc->Channels)
         );
      }
   }

   AVFrame *RetFrame = NULL;
   int64_t RetImagePosition = 0;
   double VideoFramePosition = dPosition;

   // Count number of image > position
   bool IsVideoFind = false;
   #ifdef USE_YUVCACHE_MAP
   if (!YUVCache.isEmpty() /*&& YUVCache.lastKey() >= Position*/)
   {
      QMap<int64_t, cImageInCache *>::const_iterator i = YUVCache.constBegin();
      while (i != YUVCache.constEnd() && !IsVideoFind)
      {
         if (i.key() >= Position && i.key() - Position < ALLOWEDDELTA)
            IsVideoFind = true;
         ++i;
      }
   }
   #else
   for (int CNbr = 0; !IsVideoFind && CNbr < CacheImage.count(); CNbr++)
      if (CacheImage[CNbr]->Position >= Position && CacheImage[CNbr]->Position - Position < ALLOWEDDELTA)
         IsVideoFind = true;
   #endif
   ContinueVideo = (VideoStream && !IsVideoFind && !ForceSoundOnly);
   if (ContinueVideo)
   {
      int64_t DiffTimePosition = -1000000;  // Compute difftime between asked position and previous end decoded position

      if (FrameBufferYUVReady)
      {
         DiffTimePosition = Position - FrameBufferYUVPosition;
         //if ((Position==0)||(DiffTimePosition<0)||(DiffTimePosition>1500000))
         //    ToLog(LOGMSG_INFORMATION,QString("VIDEO-SEEK %1 TO %2").arg(ShortName).arg(Position));
      }

      // Calc if we need to seek to a position
      if (Position == 0 || DiffTimePosition < 0 || DiffTimePosition > 1500000) // Allow 1,5 sec diff (rounded double !)
      {
         if (Position < 0)
            Position = 0;
         SeekErrorCount = 0;
         SeekFile(VideoStream, NULL, Position);        // Always seek one FPS before to ensure eventual filter have time to init
         VideoFramePosition = Position / AV_TIME_BASE;
      }
   }

   //*************************************************************************************************************************************
   // Decoding process : Get StreamPacket until endposition is reach (if sound is wanted) or until image is ok (if image only is wanted)
   //*************************************************************************************************************************************
   //qDebug() << "readFrame prep " << PC2time(curPCounter()-pcpart,true);
   //pcpart = curPCounter();

   // AUDIO PART
   //QFutureWatcher<void> ThreadAudio;
   {
      //AUTOTIMER(atAudio,"cVideoFile::ReadFrame audio Part");
      //autoTimer atAudio("cVideoFile::ReadFrame audio Part");
      //if( !AudioContext.ContinueAudio )
      //   qDebug() << "cVideoFile::ReadFrame audio, AudioContext.ContinueAudio is false!!!!!";

      while (AudioContext.ContinueAudio)
      {
         AVPacket *StreamPacket = new AVPacket();
         if (!StreamPacket)
         {
            AudioContext.ContinueAudio = false;
         }
         else
         {
            av_init_packet(StreamPacket);
            StreamPacket->flags |= AV_PKT_FLAG_KEY;
            int err;
            if ((err = av_read_frame(LibavAudioFile, StreamPacket)) < 0)
            {
               // If error reading frame then we considere we have reach the end of file
               if (!IsComputedAudioDuration)
               {
                  dEndFile = qreal(SoundTrackBloc->/*LastReadPosition*/CurrentPosition) / AV_TIME_BASE;
                  dEndFile += qreal(SoundTrackBloc->GetDuration()) / 1000;
                  if (dEndFile - LibavStartTime / AV_TIME_BASE == double(QTime(0, 0, 0, 0).msecsTo(EndTime)) / 1000)
                     EndTime = QTime(0, 0, 0).addMSecs((dEndFile - LibavStartTime) * 1000);
                  SetRealAudioDuration(QTime(0, 0, 0, 0).addMSecs(qlonglong((dEndFile - LibavStartTime / AV_TIME_BASE) * 1000)));
               }
               AudioContext.ContinueAudio = false;
               //qDebug() << "cVideoFile::ReadFrame audio, set AudioContext.ContinueAudio to false!!!!! err is "<<err;

               // Use data in TempData to create a latest block
               SoundTrackBloc->UseLatestData();
            }
            else
            {
               DecodeAudio(&AudioContext, StreamPacket, Position);
               StreamPacket = NULL;
            }
         }
         // Continue with a new one
         if (StreamPacket != NULL)
         {
            AV_FREE_PACKET(StreamPacket); // Free the StreamPacket that was allocated by previous call to av_read_frame
            delete StreamPacket;
            StreamPacket = NULL;
         }
      }
   }
   //qDebug() << "readFrame audio " << PC2time(curPCounter()-pcpart,true);
   //pcpart = curPCounter();

   // VIDEO PART
   if (VideoStream && !ForceSoundOnly)
   {
      //AUTOTIMER(atAudio,"cVideoFile::ReadFrame video Part");
      //autoTimer atVideo("cVideoFile::ReadFrame video Part");
      //LONGLONG pcpartv = curPCounter();
      if (!ContinueVideo)
      {
         ToLog(LOGMSG_DEBUGTRACE, QString("Video image for position %1 => use image in cache").arg(Position));
      }
      else if (Position < LibavStartTime)
      {
         ToLog(LOGMSG_CRITICAL, QString("Image position %1 is before video stream start => return black frame").arg(Position));
         //RetImage = new QImage(LibavVideoFile->streams[VideoStreamNumber]->codec->width, LibavVideoFile->streams[VideoStreamNumber]->codec->height, QImage::Format_ARGB32_Premultiplied);
         //RetImage->fill(0);
         RetImagePosition = Position;
      }
      else
      {
         #ifdef USE_YUVCACHE_MAP
         bool ByPassFirstImage = (Deinterlace) && (YUVCache.count() == 0);
         #else
         bool ByPassFirstImage = (Deinterlace) && (CacheImage.count() == 0);
         #endif
         int MaxErrorCount = 20;
         bool FreeFrames = false;

         while (ContinueVideo)
         {
            AVPacket *StreamPacket = new AVPacket();
            if (!StreamPacket)
            {
               ContinueVideo = false;
            }
            else
            {
               av_init_packet(StreamPacket);
               StreamPacket->flags |= AV_PKT_FLAG_KEY;  // HACK for CorePNG to decode as normal PNG by default

               int errcode = 0;
               if ((errcode = av_read_frame(LibavVideoFile, StreamPacket)) < 0)
               {
                  if (errcode == AVERROR_EOF)
                  {
                     // We have reach the end of file
                     if (!IsComputedAudioDuration)
                     {
                        dEndFile = VideoFramePosition;
                        if (dEndFile - LibavStartTime == double(QTime(0, 0, 0, 0).msecsTo(EndTime)) / 1000)
                           EndTime = QTime(0, 0, 0).addMSecs((dEndFile - LibavStartTime)*1000.0);
                        SetRealVideoDuration(QTime(0, 0, 0, 0).addMSecs(qlonglong((dEndFile - LibavStartTime) * 1000)));
                     }
                     ContinueVideo = false;

                     if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= (dEndFile - 1.5)*AV_TIME_BASE)
                     {
                        //if (!RetImage)
                        //{
                        //   RetImage = new QImage(LastImage);
                        //   RetImagePosition = FrameBufferYUVPosition;
                        //}
                        IsVideoFind = true;
                        ContinueVideo = false;
                     }
                  }
                  else
                  {
                     ToLog(LOGMSG_CRITICAL, GetAvErrorMessage(errcode));
                     // If error reading frame
                     if (MaxErrorCount > 0)
                     {
                        // Files with stream could provoque this, so we ignore the first MaxErrorCount errors
                        MaxErrorCount--;
                     }
                     else
                     {
                        if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= (dEndFile - 1.5)*AV_TIME_BASE)
                        {
                           //if (!RetImage)
                           //{
                           //   RetImage = new QImage(LastImage);
                           //   RetImagePosition = FrameBufferYUVPosition;
                           //}
                           IsVideoFind = true;
                           ContinueVideo = false;
                        }
                        else
                        {
                           SeekErrorCount = 0;
                           ContinueVideo = SeekFile(VideoStream, NULL, Position - 2 * AudioContext.FPSDuration);
                        }
                     }
                  }
               }
               else
               {
                  int64_t FramePts = StreamPacket->pts != (int64_t)AV_NOPTS_VALUE ? StreamPacket->pts : -1;
                  double TimeBase = double(LibavVideoFile->streams[StreamPacket->stream_index]->time_base.den) / double(LibavVideoFile->streams[StreamPacket->stream_index]->time_base.num);
                  if (FramePts >= 0)
                     VideoFramePosition = (double(FramePts) / TimeBase);

                  if (StreamPacket->stream_index == VideoStreamNumber)
                  {
                     // Allocate structures
                     if (FrameBufferYUV == NULL)
                        FrameBufferYUV = ALLOCFRAME();
                     if (FrameBufferYUV)
                     {
                        int FrameDecoded = 0;
                        LastLibAvMessageLevel = 0;    // Clear LastLibAvMessageLevel : some decoder dont return error but display errors messages !
                        int Error = avcodec_decode_video2(VideoStream->codec, FrameBufferYUV, &FrameDecoded, StreamPacket);
                        if (Error < 0 || LastLibAvMessageLevel == LOGMSG_CRITICAL)
                        {
                           if (MaxErrorCount > 0)
                           {
                              if (VideoFramePosition*1000000.0 < Position)
                              {
                                 ToLog(LOGMSG_INFORMATION, QString("IN:cVideoFile::ReadFrame - Error decoding packet: try left %1").arg(MaxErrorCount));
                              }
                              else
                              {
                                 ToLog(LOGMSG_INFORMATION, QString("IN:cVideoFile::ReadFrame - Error decoding packet: seek to backward and restart reading"));
                                 if (Position > 1000000)
                                 {
                                    SeekErrorCount = 0;
                                    SeekFile(VideoStream, NULL/*AudioStream*/, Position - 1000000); // 1 sec before
                                 }
                                 else
                                 {
                                    SeekErrorCount = 0;
                                    SeekFile(VideoStream, NULL, 0);
                                 }
                              }
                              MaxErrorCount--;
                           }
                           else
                           {
                              ToLog(LOGMSG_CRITICAL, QString("IN:cVideoFile::ReadFrame - Error decoding packet: and no try left"));
                              ContinueVideo = false;
                           }
                        }
                        else
                        {
                           if (FrameDecoded > 0)
                           {

                              FrameBufferYUV->pkt_pts = av_frame_get_best_effort_timestamp(FrameBufferYUV);
                              // Video filter part
                              if (Deinterlace && !VideoFilterGraph)
                                 VideoFilter_Open();
                              else if (!Deinterlace && VideoFilterGraph)
                                 VideoFilter_Close();

                              AVFrame *FiltFrame = NULL;
                              if (VideoFilterGraph)
                              {
                                 // FFMPEG 2.0
                                 // push the decoded frame into the filtergraph
                                 if (av_buffersrc_add_frame_flags(VideoFilterIn, FrameBufferYUV, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                                 {
                                    ToLog(LOGMSG_INFORMATION, "IN:cVideoFile::ReadFrame : Error while feeding the filtergraph");
                                 }
                                 else
                                 {
                                    FiltFrame = av_frame_alloc();
                                    // pull filtered frames from the filtergraph
                                    int ret = av_buffersink_get_frame(VideoFilterOut, FiltFrame);
                                    if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                                    {
                                       ToLog(LOGMSG_INFORMATION, "IN:cVideoFile::ReadFrame : No image return by filter process");
                                       av_frame_unref(FiltFrame);
                                       av_frame_free(&FiltFrame);
                                       FiltFrame = NULL;
                                    }
                                 }
                              }
                              if (ByPassFirstImage)
                              {
                                 ByPassFirstImage = false;
                                 FreeFrames = true;
                              }
                              else
                              {
                                 int64_t pts = FrameBufferYUV->pkt_pts;
                                 if (pts == (int64_t)AV_NOPTS_VALUE)
                                 {
                                    if (FrameBufferYUV->pkt_dts != (int64_t)AV_NOPTS_VALUE)
                                    {
                                       pts = FrameBufferYUV->pkt_dts;
                                       ToLog(LOGMSG_DEBUGTRACE, QString("IN:cVideoFile::ReadFrame : No PTS so use DTS %1 for position %2").arg(pts).arg(Position));
                                    }
                                    else
                                    {
                                       pts = 0;
                                       ToLog(LOGMSG_DEBUGTRACE, QString("IN:cVideoFile::ReadFrame : No PTS and no DTS for position %1").arg(Position));
                                    }
                                 }
                                 FrameBufferYUVReady = true;                                            // Keep actual value for FrameBufferYUV
                                 FrameBufferYUVPosition = int64_t((qreal(pts)*av_q2d(VideoStream->time_base))*AV_TIME_BASE);    // Keep actual value for FrameBufferYUV
                                                                                                                                // Append this frame
                                 cImageInCache *ObjImage =
                                    new cImageInCache(FrameBufferYUVPosition, FiltFrame, FrameBufferYUV);
                                 FreeFrames = false;
                                 IsVideoFind = false;
                                 #ifdef USE_YUVCACHE_MAP
                                 YUVCache[FrameBufferYUVPosition] = ObjImage;
                                 if (YUVCache.lastKey() >= Position)
                                    IsVideoFind = true;
                                 #else
                                 int ToIns = 0;
                                 while (ToIns < CacheImage.count() && CacheImage.at(ToIns)->Position < ObjImage->Position)
                                    ToIns++;
                                 if (ToIns < CacheImage.count())
                                 {
                                    CacheImage.insert(ToIns, ObjImage);
                                    //ToLog(LOGMSG_DEBUGTRACE,QString("IN:cVideoFile::ReadFrame : Insert image %1 for position %2 (FramePosition=%3) - Key:%4 PTS:%5 Num:%6").arg(FrameBufferYUVPosition).arg(Position).arg(VideoFramePosition).arg(FrameBufferYUV->key_frame).arg(FrameBufferYUV->pkt_pts).arg(FrameBufferYUV->coded_picture_number));
                                 }
                                 else
                                 {
                                    CacheImage.append(ObjImage);
                                    //ToLog(LOGMSG_DEBUGTRACE,QString("IN:cVideoFile::ReadFrame : Append image %1 for position %2 (FramePosition=%3) - Key:%4 PTS:%5 Num:%6").arg(FrameBufferYUVPosition).arg(Position).arg(VideoFramePosition).arg(FrameBufferYUV->key_frame).arg(FrameBufferYUV->pkt_pts).arg(FrameBufferYUV->coded_picture_number));
                                 }
                                 // Count number of image > position
                                 int Nbr = 0;
                                 for (int CNbr = 0; CNbr < CacheImage.count(); CNbr++)
                                    if (CacheImage.at(CNbr)->Position >= Position && CacheImage.at(CNbr)->Position - Position < ALLOWEDDELTA)
                                       Nbr++;
                                 IsVideoFind = Nbr > 0;
                                 #endif
                              }
                              if (FreeFrames)
                              {
                                 if (FiltFrame)
                                 {
                                    av_frame_unref(FiltFrame);
                                    av_frame_free(&FiltFrame);
                                    //FiltFrame = NULL; // added
                                    av_frame_unref(FrameBufferYUV);
                                 }
                                 FREEFRAME(&FrameBufferYUV);
                              }
                              else
                              {
                                 FrameBufferYUV = NULL;
                                 FiltFrame = NULL;
                              }
                           }
                        }
                     }
                  }
               }
               // Check if we need to continue loop
               // Note: FPSDuration*(!VideoStream?2:1) is to enhance preview speed
               ContinueVideo = ContinueVideo && (VideoStream && !IsVideoFind && (VideoFramePosition * 1000000 < Position
                  || VideoFramePosition * 1000000 - Position < MAXDELTA));
            }

            // Continue with a new one
            if (StreamPacket != NULL)
            {
               AV_FREE_PACKET(StreamPacket); // Free the StreamPacket that was allocated by previous call to av_read_frame
               delete StreamPacket;
               StreamPacket = NULL;
            }
         }
      }
      //qDebug() << "readFrame video stage1 " << PC2time(curPCounter()-pcpartv,true);
      //pcpartv = curPCounter();

      #ifdef USE_YUVCACHE_MAP
      if (/*!RetImage && */YUVCache.count() > 0)
      {
         QMap<int64_t, cImageInCache *>::const_iterator i = YUVCache.lowerBound(Position - MAXDELTA);
         QMap<int64_t, cImageInCache *>::const_iterator upperBound = YUVCache.upperBound(Position + MAXDELTA);
         QMap<int64_t, cImageInCache *>::const_iterator nearest = YUVCache.end();
         int64_t Nearest = MAXDELTA;
         while (i != upperBound)
         {
            if (abs(Position - i.key()) < Nearest)
            {
               nearest = i;
               Nearest = abs(Position - i.key());
            }
            ++i;
         }
         if (nearest != YUVCache.end())
         {
            //autoTimer at("cVideoFile::ConvertYUVToRGB");
            //RetImage = ConvertYUVToRGB(PreviewMode, nearest.value()->FiltFrame ? nearest.value()->FiltFrame : nearest.value()->FrameBufferYUV);
            RetFrame = nearest.value()->FiltFrame ? nearest.value()->FiltFrame : nearest.value()->FrameBufferYUV;
            RetImagePosition = nearest.key();
            //ToLog(LOGMSG_DEBUGTRACE,QString("Video image for position %1 => return image at %2").arg(Position).arg(CacheImage[i]->Position));
         }
         else
         {
            ToLog(LOGMSG_CRITICAL, QString("No video YUV return for position %1 => return image at %2").arg(Position).arg(YUVCache.first()->Position));
            qDebug() << "no yuv for " << FileName();
            //RetImage = ConvertYUVToRGB(PreviewMode, YUVCache.first()->FiltFrame ? YUVCache.first()->FiltFrame : YUVCache.first()->FrameBufferYUV);
            RetFrame = YUVCache.first()->FiltFrame ? YUVCache.first()->FiltFrame : YUVCache.first()->FrameBufferYUV;
            RetImagePosition = YUVCache.first()->Position;
         }
      }
      #else
      if (!RetImage && CacheImage.count() > 0)
      {
         //AUTOTIMER(fc,"Image from YUV-Cache");
         // search nearest image (allowed up to MAXDELTA, after return black frame)
         int i = -1, Nearest = MAXDELTA;
         for (int jj = 0; jj < CacheImage.count(); jj++)
         {
            if (CacheImage.at(jj)->Position >= Position && CacheImage.at(jj)->Position - Position < MAXDELTA)
            {
               if (i == -1 || CacheImage.at(jj)->Position - Position < Nearest)
               {
                  i = jj;
                  Nearest = CacheImage.at(jj)->Position - Position;
               }
            }
         }
         if (i >= 0 && i < CacheImage.count()/*&&(CacheImage[i]->Position>=Position)&&(CacheImage[i]->Position-Position<100000)*/)
         {
            //AUTOTIMER(cyuv,"ConvertYUVToRGB");
            RetImage = ConvertYUVToRGB(PreviewMode, CacheImage.at(i)->FiltFrame ? CacheImage.at(i)->FiltFrame : CacheImage.at(i)->FrameBufferYUV);
            RetImagePosition = CacheImage.at(i)->Position;
            //ToLog(LOGMSG_DEBUGTRACE,QString("Video image for position %1 => return image at %2").arg(Position).arg(CacheImage[i]->Position));
         }
         else
         {
            ToLog(LOGMSG_CRITICAL, QString("No video image return for position %1 => return image at %2").arg(Position).arg(CacheImage[0]->Position));
            RetImage = ConvertYUVToRGB(PreviewMode, CacheImage[0]->FiltFrame ? CacheImage[0]->FiltFrame : CacheImage[0]->FrameBufferYUV);
            RetImagePosition = CacheImage.at(0)->Position;
         }
      }
      #endif
      //qDebug() << "readFrame video stage2 " << PC2time(curPCounter()-pcpartv,true);
      //pcpartv = curPCounter();

      //if (!RetImage)
      //{
      //   ToLog(LOGMSG_CRITICAL, QString("No video image return for position %1 => return black frame").arg(Position));
      //   RetImage = new QImage(LibavVideoFile->streams[VideoStreamNumber]->codec->width, LibavVideoFile->streams[VideoStreamNumber]->codec->height, QImage::Format_ARGB32_Premultiplied);
      //   RetImage->fill(0);
      //   RetImagePosition = Position;
      //}
      if(RetFrame)
      {
         RetFrame = av_frame_clone(RetFrame);
         //qDebug() << "cloned Frame is " << RetFrame;
      }
      int64_t cutoff = Position - 50000;
      #ifdef USE_YUVCACHE_MAP
      QMutableMapIterator<int64_t, cImageInCache *> mutmapiter(YUVCache);
      while (mutmapiter.hasNext())
      {
         mutmapiter.next();
         if (mutmapiter.key() < cutoff)
         {
            //bool noDel = false;
            //if(RetFrame == mutmapiter.value()->FiltFrame || RetFrame == mutmapiter.value()->FrameBufferYUV)
            //{ 
            //   qDebug() << "skip delete the current returning frame";
            //   noDel = true;
            //}
            //if( !noDel)
            {
               delete mutmapiter.value();
               mutmapiter.remove();
            }
         }
         else
            break;
      }
      #else
      int i = 0;
      while (i < CacheImage.count())
      {
         if (CacheImage.at(i)->Position < cutoff)
            delete CacheImage.takeAt(i);
         else
            i++;
      }
      #endif
   }
   //qDebug() << "readFrame video " << PC2time(curPCounter()-pcpart,true);
   //pcpart = curPCounter();

   #ifdef USE_YUVCACHE_MAP
   if (AudioContext.AudioStream && SoundTrackBloc && YUVCache.count() > 0)
      SoundTrackBloc->AdjustSoundPosition(RetImagePosition);
   #else
   if (AudioContext.AudioStream && SoundTrackBloc && CacheImage.count() > 0)
      SoundTrackBloc->AdjustSoundPosition(RetImagePosition);
   #endif

   //Mutex.unlock();
   //qDebug() << "readFrame " << PC2time(curPCounter()-pc,true) ;
   return RetFrame;
}

//====================================================================================================================

void cVideoFile::DecodeAudio(sAudioContext *AudioContext,AVPacket *StreamPacket,int64_t Position) 
{
   //qDebug() << "decode Audio";
//autoTimer at("deceodeAudio");
   qreal FramePts = StreamPacket->pts != (int64_t)AV_NOPTS_VALUE ? StreamPacket->pts*av_q2d(AudioContext->AudioStream->time_base) : -1;
   if (StreamPacket->stream_index == AudioStreamNumber && StreamPacket->size > 0) 
   {
      //qDebug() << "decode Audio, pts = " << StreamPacket->pts << " dts = " << StreamPacket->dts;
      AVPacket PacketTemp;
      av_init_packet(&PacketTemp);
      PacketTemp.data = StreamPacket->data;
      PacketTemp.size = StreamPacket->size;
      //qDebug() << "decode Audio, PacketTemp.size is " << PacketTemp.size;
      // NOTE: the audio packet can contain several NbrFrames
      while (AudioContext->Counter > 0 && AudioContext->ContinueAudio && PacketTemp.size > 0) 
      {
         AVFrame *Frame = ALLOCFRAME();
         int got_frame;
         int Len = avcodec_decode_audio4(AudioContext->AudioStream->codec,Frame,&got_frame,&PacketTemp);
         //qDebug() << "avcodec_decode_audio4 gives " << Len;
         if (Len < 0)
         {
            // if error, we skip the frame and exit the while loop
            PacketTemp.size = 0;
         } 
         else if (got_frame > 0) 
         {
            DecodeAudioFrame(AudioContext, &FramePts, Frame, Position);
            Frame = NULL;
            PacketTemp.data += Len;
            PacketTemp.size -= Len;
         } 
         else 
         {
            AudioContext->Counter--;
            if (AudioContext->Counter == 0) 
            {
               Len = 0;
               ToLog(LOGMSG_CRITICAL,QString("Impossible to decode audio frame: Discard it"));
            }
         }
         if (Frame != NULL) 
            FREEFRAME(&Frame);
      }
   }
   // Continue with a new one
   if (StreamPacket != NULL) 
   {
      AV_FREE_PACKET(StreamPacket); // Free the StreamPacket that was allocated by previous call to av_read_frame
      delete StreamPacket;
      StreamPacket = NULL;
   }
   // Check if we need to continue loop
   // Note: FPSDuration*(!VideoStream?2:1) is to enhance preview speed
   AudioContext->ContinueAudio = (AudioContext->ContinueAudio && AudioContext->Counter > 0 && AudioContext->AudioStream && AudioContext->SoundTrackBloc
      && ( (AudioContext->SoundTrackBloc->ListCount() < AudioContext->SoundTrackBloc->NbrPacketForFPS)
           ||( !(LastAudioReadPosition >= Position + AudioContext->FPSDuration*AudioContext->NbrDuration)) ));
}

//============================

void cVideoFile::DecodeAudioFrame(sAudioContext *AudioContext, qreal *FramePts, AVFrame *Frame, int64_t Position) 
{
   //qDebug() << "DecodeAudioFrame";
   int64_t SizeDecoded = 0;
   u_int8_t *Data = NULL;
   if (AudioContext->NeedResampling && RSC != NULL) 
   {
      Data = Resample(Frame, &SizeDecoded, AudioContext->DstSampleSize);
   } 
   else 
   {
      Data = Frame->data[0];
      SizeDecoded = Frame->nb_samples*av_get_bytes_per_sample(AudioContext->AudioStream->codec->sample_fmt)*AudioContext->AudioStream->codec->channels;
   }
   AudioContext->ContinueAudio = (Data != NULL);
   if (AudioContext->ContinueAudio) 
   {
      // Adjust FrameDuration with real Nbr Sample
      double FrameDuration = double(SizeDecoded)/(AudioContext->SoundTrackBloc->SamplingRate*AudioContext->DstSampleSize);
      // Adjust pts and inc FramePts int the case there are multiple blocks
      qreal pts = (*FramePts)/av_q2d(AudioContext->AudioStream->time_base);
      if (pts < 0) 
         pts = qreal(Position + AudioContext->FPSDuration);
      (*FramePts) += FrameDuration;
      AudioContext->AudioFramePosition = *FramePts;
      // Adjust volume if master volume <>1
      double Volume = AudioContext->Volume;
      if (Volume == -1) 
         Volume = GetSoundLevel() != -1 ? double(ApplicationConfig->DefaultSoundLevel)/double(GetSoundLevel()*100) : 1;
      if (Volume != 1) 
      {
         //qDebug() << "apply Volume " << Volume;
         int16_t *Buf1 = (int16_t*)Data;
         for (int j = 0; j < SizeDecoded/4; j++) 
         {
            // Left channel : Adjust if necessary (16 bits)
            *Buf1 = (int16_t) qBound(-32768.0, double(*Buf1)*Volume, 32767.0); Buf1++;
            // Right channel : Adjust if necessary (16 bits)
            *Buf1 = (int16_t) qBound(-32768.0, double(*Buf1)*Volume, 32767.0); Buf1++;
         }
      }
      // Append decoded data to SoundTrackBloc
      if (AudioContext->DontUseEndPos || AudioContext->AudioFramePosition < *AudioContext->dEndFile) 
      {
         //qDebug() << "add decoded data to SoundTrackBloc AudioContext->DontUseEndPos:" << AudioContext->DontUseEndPos 
         //   << " AudioContext->AudioFramePosition:" << AudioContext->AudioFramePosition 
         //   << " AudioContext->dEndFile:" <<*AudioContext->dEndFile
         //   << " SizeDecoded " << SizeDecoded;
         AudioContext->SoundTrackBloc->AppendData(AudioContext->AudioFramePosition*AV_TIME_BASE,(int16_t*)Data, SizeDecoded);
         //AudioContext->SoundTrackBloc->LastReadPosition = AudioContext->AudioFramePosition*AV_TIME_BASE;
         AudioContext->AudioLenDecoded += SizeDecoded;
         AudioContext->AudioFramePosition = AudioContext->AudioFramePosition + FrameDuration;
      }
      else
      {
         AudioContext->ContinueAudio = false;
         // qDebug() << "do not add decoded data to SoundTrackBloc!!";
      }
   }
   LastAudioReadPosition = int64_t(AudioContext->AudioFramePosition*AV_TIME_BASE);    // Keep NextPacketPosition for determine next time if we need to seek
   if (Data != Frame->data[0]) 
      av_free(Data);
   FREEFRAME(&Frame);
}

//====================================================================================================================


//int QMyImage::offset = -1;
//bool QMyImage::offsetDetected = false;

//void QMyImage::detectOffset()
//{
//   if(offsetDetected)
//      return;
//   QImage testImg(1,1,QImage::Format_RGB32);
//   int *data = (int*)testImg.data_ptr();
//   int datasize = 50;
//   QList<int> firstRun;
//   QList<int> secondRun;
//   QList<int> thirdRun;
//   for(int i = 0; i < datasize; i++ )
//   {
//      if( *data == QImage::Format_RGB32 )
//         firstRun.append(i);
//      data++;
//   }
//   testImg = testImg.convertToFormat(QImage::Format_ARGB32);
//   data = (int*)testImg.data_ptr();
//   for(int i = 0; i < datasize; i++ )
//   {
//      if( *data == QImage::Format_ARGB32 )
//      {
//         if( firstRun.contains(i) )
//            secondRun.append(i);
//      }
//      data++;
//   }

//   testImg = testImg.convertToFormat(QImage::Format_ARGB32_Premultiplied);
//   data = (int*)testImg.data_ptr();
//   for(int i = 0; i < datasize; i++ )
//   {
//      if( *data == QImage::Format_ARGB32_Premultiplied )
//      {
//         if( secondRun.contains(i) )
//            thirdRun.append(i);
//      }
//      data++;
//   }
//   if( thirdRun.count() == 1 )
//      offset = thirdRun.first();
//   offsetDetected = true;
//}

//#ifdef QT_COMPILER_SUPPORTS_SSE4_1
//#define BYTE_MUL_SSE2(result, pixelVector, alphaChannel, colorMask, half) \
//{ \
//    /* 1. separate the colors in 2 vectors so each color is on 16 bits \
//       (in order to be multiplied by the alpha \
//       each 32 bit of dstVectorAG are in the form 0x00AA00GG \
//       each 32 bit of dstVectorRB are in the form 0x00RR00BB */\
//    __m128i pixelVectorAG = _mm_srli_epi16(pixelVector, 8); \
//    __m128i pixelVectorRB = _mm_and_si128(pixelVector, colorMask); \
// \
//    /* 2. multiply the vectors by the alpha channel */\
//    pixelVectorAG = _mm_mullo_epi16(pixelVectorAG, alphaChannel); \
//    pixelVectorRB = _mm_mullo_epi16(pixelVectorRB, alphaChannel); \
// \
//    /* 3. divide by 255, that's the tricky part. \
//       we do it like for BYTE_MUL(), with bit shift: X/255 ~= (X + X/256 + rounding)/256 */ \
//    /** so first (X + X/256 + rounding) */\
//    pixelVectorRB = _mm_add_epi16(pixelVectorRB, _mm_srli_epi16(pixelVectorRB, 8)); \
//    pixelVectorRB = _mm_add_epi16(pixelVectorRB, half); \
//    pixelVectorAG = _mm_add_epi16(pixelVectorAG, _mm_srli_epi16(pixelVectorAG, 8)); \
//    pixelVectorAG = _mm_add_epi16(pixelVectorAG, half); \
// \
//    /** second divide by 256 */\
//    pixelVectorRB = _mm_srli_epi16(pixelVectorRB, 8); \
//    /** for AG, we could >> 8 to divide followed by << 8 to put the \
//        bytes in the correct position. By masking instead, we execute \
//        only one instruction */\
//    pixelVectorAG = _mm_andnot_si128(colorMask, pixelVectorAG); \
// \
//    /* 4. combine the 2 pairs of colors */ \
//    result = _mm_or_si128(pixelVectorAG, pixelVectorRB); \
//}
//#endif

//bool QMyImage::forcePremulFormat()
//{
//   if (isNull() || format() == QImage::Format_ARGB32_Premultiplied)
//      return true;
//   if (format() != QImage::Format_ARGB32)
//      return false;
//   if (!offsetDetected)
//      detectOffset();
//   if (offset < 0)
//      return false;
//   setFormat(QImage::Format_ARGB32_Premultiplied);
//   return true;
//}

////  x/255 = (x*257+257)>>16
//bool QMyImage::convert_ARGB_to_ARGB_PM_inplace()
//{
//   if( isNull() || format() ==  QImage::Format_ARGB32_Premultiplied )
//      return true;
//   if( format() != QImage::Format_ARGB32 )
//      return false;
//   if( !offsetDetected )
//      detectOffset();
//   if( offset < 0 )
//      return false;

//#ifndef QT_COMPILER_SUPPORTS_SSE4_1
//    uint *buffer = reinterpret_cast<uint*>( const_cast<uchar *>(((const QMyImage*)this)->bits()) );
//    int count = byteCount()/4;
//    for (int i = 0; i < count; ++i)
//    {
//        *buffer = qPremultiply(*buffer);
//        buffer++;
//    }
//#else
//    // extra pixels on each line
//    const int spare = width() & 3;
//    // width in pixels of the pad at the end of each line
//    const int pad = (bytesPerLine() >> 2) - width();
//    const int iter = width() >> 2;
//    int _height = height();

//    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
//    const __m128i nullVector = _mm_setzero_si128();
//    const __m128i half = _mm_set1_epi16(0x80);
//    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);

//    __m128i *d = reinterpret_cast<__m128i*>( const_cast<uchar *>(((const QMyImage*)this)->bits()) );
//    while (_height--) {
//        const __m128i *end = d + iter;

//        for (; d != end; ++d) {
//            const __m128i srcVector = _mm_loadu_si128(d);
//            const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask);
//            if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) {
//                // opaque, data is unchanged
//            } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) == 0xffff) {
//                // fully transparent
//                _mm_storeu_si128(d, nullVector);
//            } else {
//                __m128i alphaChannel = _mm_srli_epi32(srcVector, 24);
//                alphaChannel = _mm_or_si128(alphaChannel, _mm_slli_epi32(alphaChannel, 16));

//                __m128i result;
//                BYTE_MUL_SSE2(result, srcVector, alphaChannel, colorMask, half);
//                result = _mm_or_si128(_mm_andnot_si128(alphaMask, result), srcVectorAlpha);
//                _mm_storeu_si128(d, result);
//            }
//        }

//        QRgb *p = reinterpret_cast<QRgb*>(d);
//        QRgb *pe = p+spare;
//        for (; p != pe; ++p) {
//            if (*p < 0x00ffffff)
//                *p = 0;
//            else if (*p < 0xff000000)
//                *p = qPremultiply(*p);
//        }

//        d = reinterpret_cast<__m128i*>(p+pad);
//    }
//#endif
//    setFormat(QImage::Format_ARGB32_Premultiplied);
//    return true;
//}

//inline QRgb qUnpremultiply_sse4(QRgb p)
//{
//   const uint alpha = qAlpha(p);
//   if (alpha == 255 || alpha == 0)
//      return p;
//   const uint invAlpha = qt_inv_premul_factor[alpha];
//   const __m128i via = _mm_set1_epi32(invAlpha);
//   const __m128i vr = _mm_set1_epi32(0x8000);
//   __m128i vl = _mm_cvtepu8_epi32(_mm_cvtsi32_si128(p));
//   vl = _mm_mullo_epi32(vl, via);
//   vl = _mm_add_epi32(vl, vr);
//   vl = _mm_srai_epi32(vl, 16);
//   vl = _mm_insert_epi32(vl, alpha, 3);
//   vl = _mm_packus_epi32(vl, vl);
//   vl = _mm_packus_epi16(vl, vl);
//   return _mm_cvtsi128_si32(vl);
//}

//bool QMyImage::convert_ARGB_PM_to_ARGB_inplace()
//{
//   if( isNull() || format() ==  QImage::Format_ARGB32 )
//      return true;
//   if( !offsetDetected )
//      detectOffset();
//   if( format() != QImage::Format_ARGB32_Premultiplied || offset < 0)
//      return false;
//    uint *buffer = reinterpret_cast<uint*>( const_cast<uchar *>(((const QMyImage*)this)->bits()) );
//    int count = byteCount()/4;
//    for (int i = 0; i < count; ++i)
//    {
//       //*buffer = qUnpremultiply(*buffer++);
//       //*buffer = qUnpremultiply_sse4(*buffer++);
//       QRgb p = *buffer;
//       const uint alpha = qAlpha(p);
//       // Alpha 255 and 0 are the two most common values, which makes them beneficial to short-cut.
//       if (alpha == 0)
//          *buffer = 0;
//       if (alpha != 255)
//       {
//          // (p*(0x00ff00ff/alpha)) >> 16 == (p*255)/alpha for all p and alpha <= 256.
//          const uint invAlpha = qt_inv_premul_factor[alpha];
//          // We add 0x8000 to get even rounding. The rounding also ensures that qPremultiply(qUnpremultiply(p)) == p for all p.
//          *buffer = qRgba((qRed(p)*invAlpha + 0x8000) >> 16, (qGreen(p)*invAlpha + 0x8000) >> 16, (qBlue(p)*invAlpha + 0x8000) >> 16, alpha);
//       }
//    }
//   setFormat(QImage::Format_ARGB32);
//   return true;
//}


//void QMyImage::convert_ARGB_to_ARGB_PM_inplace()
//{
//    //Q_ASSERT(data->format() == QImage::Format_ARGB32);
//
//    // extra pixels on each line
//    const int spare = width() & 3;
//    // width in pixels of the pad at the end of each line
//    const int pad = (bytesPerLine() >> 2) - width();
//    const int iter = width() >> 2;
//    int _height = height();
//
//    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
//    const __m128i nullVector = _mm_setzero_si128();
//    const __m128i half = _mm_set1_epi16(0x80);
//    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
//
//    __m128i *d = reinterpret_cast<__m128i*>( const_cast<uchar *>(((const QMyImage*)this)->bits()) );
//    while (_height--) {
//        const __m128i *end = d + iter;
//
//        for (; d != end; ++d) {
//            const __m128i srcVector = _mm_loadu_si128(d);
//            const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask);
//            if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) {
//                // opaque, data is unchanged
//            } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) == 0xffff) {
//                // fully transparent
//                _mm_storeu_si128(d, nullVector);
//            } else {
//                __m128i alphaChannel = _mm_srli_epi32(srcVector, 24);
//                alphaChannel = _mm_or_si128(alphaChannel, _mm_slli_epi32(alphaChannel, 16));
//
//                __m128i result;
//                BYTE_MUL_SSE2(result, srcVector, alphaChannel, colorMask, half);
//                result = _mm_or_si128(_mm_andnot_si128(alphaMask, result), srcVectorAlpha);
//                _mm_storeu_si128(d, result);
//            }
//        }
//
//        QRgb *p = reinterpret_cast<QRgb*>(d);
//        QRgb *pe = p+spare;
//        for (; p != pe; ++p) {
//            if (*p < 0x00ffffff)
//                *p = 0;
//            else if (*p < 0xff000000)
//                *p = qPremultiply(*p);
//        }
//
//        d = reinterpret_cast<__m128i*>(p+pad);
//    }
//
//    //data->format = QImage::Format_ARGB32_Premultiplied;
//    setFormat(QImage::Format_ARGB32_Premultiplied);
//    //return true;
//} 
//void QMyImage::convert_ARGB_PM_to_ARGB_inplace()
//{
//    uint *buffer = reinterpret_cast<uint*>( const_cast<uchar *>(((const QMyImage*)this)->bits()) );
//    int count = byteCount()/4;
//    for (int i = 0; i < count; ++i)
//        buffer[i] = qUnpremultiply(buffer[i]);
//   setFormat(QImage::Format_ARGB32);
//}
//
///*
//bool convert_ARGB_to_ARGB_PM_inplace_sse2(QMyImage/*Data* / *data/*, Qt::ImageConversionFlags* /)
//{
//    //Q_ASSERT(data->format() == QImage::Format_ARGB32);
//
//    // extra pixels on each line
//    const int spare = data->width() & 3;
//    // width in pixels of the pad at the end of each line
//    const int pad = (data->bytesPerLine() >> 2) - data->width();
//    const int iter = data->width() >> 2;
//    int height = data->height();
//
//    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
//    const __m128i nullVector = _mm_setzero_si128();
//    const __m128i half = _mm_set1_epi16(0x80);
//    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
//
//    __m128i *d = reinterpret_cast<__m128i*>(data->bits());
//    while (height--) {
//        const __m128i *end = d + iter;
//
//        for (; d != end; ++d) {
//            const __m128i srcVector = _mm_loadu_si128(d);
//            const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask);
//            if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) {
//                // opaque, data is unchanged
//            } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) == 0xffff) {
//                // fully transparent
//                _mm_storeu_si128(d, nullVector);
//            } else {
//                __m128i alphaChannel = _mm_srli_epi32(srcVector, 24);
//                alphaChannel = _mm_or_si128(alphaChannel, _mm_slli_epi32(alphaChannel, 16));
//
//                __m128i result;
//                BYTE_MUL_SSE2(result, srcVector, alphaChannel, colorMask, half);
//                result = _mm_or_si128(_mm_andnot_si128(alphaMask, result), srcVectorAlpha);
//                _mm_storeu_si128(d, result);
//            }
//        }
//
//        QRgb *p = reinterpret_cast<QRgb*>(d);
//        QRgb *pe = p+spare;
//        for (; p != pe; ++p) {
//            if (*p < 0x00ffffff)
//                *p = 0;
//            else if (*p < 0xff000000)
//                *p = qPremultiply(*p);
//        }
//
//        d = reinterpret_cast<__m128i*>(p+pad);
//    }
//
//    //data->format = QImage::Format_ARGB32_Premultiplied;
//    data->setFormat();
//    return true;
//} */

QImage *cVideoFile::ConvertYUVToRGB(bool PreviewMode,AVFrame *Frame) 
{
   //AUTOTIMER(at,"cVideoFile::ConvertYUVToRGB");
   //LONGLONG cp = curPCounter();
   //LONGLONG cppart = cp;

   int W = Frame->width * aspectRatio;  W -= (W%4);   // W must be a multiple of 4 ????
   int H = Frame->height;
   QMyImage *retImage = NULL;

   if( yuv2rgbImage == NULL )
      yuv2rgbImage = new QImage(W,H,QTPIXFMT);
   //LastImage = QImage(W,H,QTPIXFMT);

   // Allocate structure for RGB image
   //AVFrame *FrameBufferRGB = ALLOCFRAME();
   if( FrameBufferRGB == 0 )
      FrameBufferRGB = ALLOCFRAME();

   //qDebug() << "YUV2RGB prep " << PC2time(curPCounter()-cppart,true);
   //cppart = curPCounter();
   if (FrameBufferRGB != NULL) 
   {
      FrameBufferRGB->format = PIXFMT;
      avpicture_fill(
         (AVPicture *)FrameBufferRGB,        // Buffer to prepare
         //LastImage.bits(),                   // Buffer which will contain the image data
         yuv2rgbImage->bits(),
         PIXFMT,                             // The format in which the picture data is stored (see http://wiki.aasimon.org/doku.php?id=Libav:pixelformat)
         W,                                  // The width of the image in pixels
         H                                   // The height of the image in pixels
         );

   //qDebug() << "YUV2RGB avpicture_fill " << PC2time(curPCounter()-cppart,true);
   //cppart = curPCounter();
      // Get a converter from libswscale
      //struct SwsContext *img_convert_ctx=sws_getContext(
      //   Frame->width,                                                     // Src width
      //   Frame->height,                                                    // Src height
      //   (PixelFormat)Frame->format,                                       // Src Format
      //   W,                                                                // Destination width
      //   H,                                                                // Destination height
      //   PIXFMT,                                                           // Destination Format
      //      SWS_BICUBIC,NULL,NULL,NULL);                                      // flags,src Filter,dst Filter,param
      //if(img_convert_ctx == 0) 
         img_convert_ctx = /*sws_getContext*/sws_getCachedContext(img_convert_ctx,
            Frame->width,                                                     // Src width
            Frame->height,                                                    // Src height
            (AVPixelFormat)Frame->format,                                       // Src Format
            W,                                                                // Destination width
            H,                                                                // Destination height
            PIXFMT,                                                           // Destination Format
               /*SWS_FAST_BILINEAR*/SWS_BICUBIC /*| SWS_ACCURATE_RND*/,NULL,NULL,NULL);                                      // flags,src Filter,dst Filter,param
   //qDebug() << "YUV2RGB sws_getContext " << PC2time(curPCounter()-cppart,true);
   //cppart = curPCounter();
      if (img_convert_ctx!=NULL) {
         int ret;
         {
            //AUTOTIMER(scale,"sws_scale");
            ret = sws_scale(
               img_convert_ctx,                                           // libswscale converter
               Frame->data,                                               // Source buffer
               Frame->linesize,                                           // Source Stride ?
               0,                                                         // Source SliceY:the position in the source image of the slice to process, that is the number (counted starting from zero) in the image of the first row of the slice
               Frame->height,                                             // Source SliceH:the height of the source slice, that is the number of rows in the slice
               FrameBufferRGB->data,                                      // Destination buffer
               FrameBufferRGB->linesize                                   // Destination Stride
               );
   //qDebug() << "YUV2RGB sws_scale " << PC2time(curPCounter()-cppart,true);
   //cppart = curPCounter();
         }
         if (ret > 0) 
         {
            //AUTOTIMER(scale,"assign & convert");
            bool bNeedCrop = ApplicationConfig->Crop1088To1080 && H == 1088 && W == 1920;
            bool bNeedScale = PreviewMode && Frame->height > ApplicationConfig->MaxVideoPreviewHeight;

            if( bNeedCrop && bNeedScale )
               LastImage = yuv2rgbImage->copy(0,4,1920,1080).scaledToHeight(ApplicationConfig->MaxVideoPreviewHeight);//.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            else if( bNeedCrop )
               LastImage = yuv2rgbImage->copy(0,4,1920,1080);//.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            else if( bNeedScale )
               LastImage = yuv2rgbImage->scaledToHeight(ApplicationConfig->MaxVideoPreviewHeight);//.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            else
            {
               if( LastImage.isNull() || LastImage.size() != yuv2rgbImage->size() )
                  LastImage = QImage(yuv2rgbImage->size(), QImage::Format_ARGB32);
               memcpy(LastImage.bits(), yuv2rgbImage->bits(), yuv2rgbImage->byteCount());
               //LastImage = *yuv2rgbImage;//->convertToFormat(QImage::Format_ARGB32_Premultiplied);
            }
            //// Auto crop image if 1088 format
            //if ((ApplicationConfig->Crop1088To1080) && (LastImage.height() == 1088) && (LastImage.width() == 1920))  
            //   LastImage = yuv2rgbImage->copy(0,4,1920,1080);
            //   //LastImage = LastImage.copy(0,4,1920,1080);
            //// Reduce image size for preview mode
            //if ((PreviewMode) && (LastImage.height()>ApplicationConfig->MaxVideoPreviewHeight)) 
            //   LastImage = LastImage.scaledToHeight(ApplicationConfig->MaxVideoPreviewHeight);
         }
         //sws_freeContext(img_convert_ctx);
      }

      // free FrameBufferRGB because we don't need it in the future
      //FREEFRAME(&FrameBufferRGB);
   }
   //cppart = curPCounter();
   //QImage *retImage = new QImage(LastImage.convertToFormat(QImage::Format_ARGB32_Premultiplied));
   //QImage *retImage = new QImage(LastImage);
   //QImage *retImage = new QImage(yuv2rgbImage->convertToFormat(QImage::Format_ARGB32_Premultiplied));
   //qDebug() << "YUV2RGB convert " << PC2time(curPCounter()-cppart,true);
   //qDebug() << "YUV2RGB " << PC2time(curPCounter()-cp,true);
   retImage = new QMyImage(LastImage/*.convertToFormat(QImage::Format_ARGB32_Premultiplied)*/);
   if( retImage->format() != QImage::Format_ARGB32_Premultiplied )
      //QMyImage(*retImage).convert_ARGB_PM_to_ARGB_inplace();
      //QMyImage(*retImage).convert_ARGB_to_ARGB_PM_inplace();
      QMyImage(*retImage).forcePremulFormat();
      //QMyImage(*retImage).forcePremulFormat();
      //convert_ARGB_to_ARGB_PM_inplace_sse2(retImage);
   return retImage;
}

//====================================================================================================================
// Load a video frame
// DontUseEndPos default=false
QImage *cVideoFile::ImageAt(bool PreviewMode,int64_t Position,cSoundBlockList *SoundTrackBloc,bool Deinterlace,
                            double Volume,bool ForceSoundOnly,bool DontUseEndPos,int NbrDuration) 
{
   if (!IsValide) 
      return NULL;
   if (!IsOpen) 
      OpenCodecAndFile();

   if (PreviewMode && !SoundTrackBloc) 
   {
      // for speed improvment, try to find image in cache (only for interface)
      cLuLoImageCacheObject *ImageObject = ApplicationConfig->ImagesCache.FindObject(ressourceKey,fileKey,modifDateTime,imageOrientation,ApplicationConfig->Smoothing,true);
      if (!ImageObject) 
         return ReadFrame(PreviewMode,Position*1000,DontUseEndPos,Deinterlace,SoundTrackBloc,Volume,ForceSoundOnly);

      if (ImageObject->Position == Position && ImageObject->CachePreviewImage)
         return new QImage(ImageObject->CachePreviewImage->copy());

      if (ImageObject->CachePreviewImage) 
      {
         delete ImageObject->CachePreviewImage;
         ImageObject->CachePreviewImage = NULL;
      }
      ImageObject->Position = Position;
      ImageObject->CachePreviewImage = ReadFrame(PreviewMode,Position*1000,DontUseEndPos,Deinterlace,SoundTrackBloc,Volume,ForceSoundOnly,NbrDuration);
      if (ImageObject->CachePreviewImage) 
         return new QImage(ImageObject->CachePreviewImage->copy());
      else 
         return NULL;

   } else 
      return ReadFrame(PreviewMode,Position*1000,DontUseEndPos,Deinterlace,SoundTrackBloc,Volume,ForceSoundOnly);
}

AVFrame *cVideoFile::YUVAt(bool PreviewMode, int64_t Position, cSoundBlockList *SoundTrackBloc, bool Deinterlace,
   double Volume, bool ForceSoundOnly, bool DontUseEndPos, int NbrDuration)
{
   if (!IsValide)
      return NULL;
   if (!IsOpen)
      OpenCodecAndFile();

   if (PreviewMode && !SoundTrackBloc)
   {
      //// for speed improvment, try to find image in cache (only for interface)
      //cLuLoImageCacheObject *ImageObject = ApplicationConfig->ImagesCache.FindObject(ressourceKey, fileKey, modifDateTime, imageOrientation, ApplicationConfig->Smoothing, true);
      //if (!ImageObject)
      //   return ReadFrame(PreviewMode, Position * 1000, DontUseEndPos, Deinterlace, SoundTrackBloc, Volume, ForceSoundOnly);

      //if (ImageObject->Position == Position && ImageObject->CachePreviewImage)
      //   return new QImage(ImageObject->CachePreviewImage->copy());

      //if (ImageObject->CachePreviewImage)
      //{
      //   delete ImageObject->CachePreviewImage;
      //   ImageObject->CachePreviewImage = NULL;
      //}
      //ImageObject->Position = Position;
      //ImageObject->CachePreviewImage = ReadFrame(PreviewMode, Position * 1000, DontUseEndPos, Deinterlace, SoundTrackBloc, Volume, ForceSoundOnly, NbrDuration);
      //if (ImageObject->CachePreviewImage)
      //   return new QImage(ImageObject->CachePreviewImage->copy());
      //else
         return NULL;

   }
   else
      return ReadYUVFrame(PreviewMode, Position * 1000, DontUseEndPos, Deinterlace, SoundTrackBloc, Volume, ForceSoundOnly);
}

//====================================================================================================================

int cVideoFile::getThreadFlags(AVCodecID ID) {
    int Ret=0;
    switch (ID) {
        case AV_CODEC_ID_PRORES:
        case AV_CODEC_ID_MPEG1VIDEO:
        case AV_CODEC_ID_DVVIDEO:
        case AV_CODEC_ID_MPEG2VIDEO:   Ret = FF_THREAD_SLICE;                    break;
        case AV_CODEC_ID_H264 :        Ret = FF_THREAD_FRAME|FF_THREAD_SLICE;    break;
        default:                       Ret = FF_THREAD_FRAME;                    break;
    }
    return Ret;
}

//====================================================================================================================

bool cVideoFile::OpenCodecAndFile() 
{
   //qDebug() << "open video";
   //getMemInfo();
   QMutexLocker locker(&accessMutex);

   // Ensure file was previously checked
   if (!IsValide) 
      return false;
   if (!IsInformationValide) 
      GetFullInformationFromFile();

   if( IsOpen )
      return true;
   // Clean memory if a previous file was loaded
   locker.unlock();
   CloseCodecAndFile(); // could this happen???
   locker.relock();

   //**********************************
   // Open audio stream
   //**********************************
   if (AudioStreamNumber != -1) 
   {

      // Open the file and get a LibAVFormat context and an associated LibAVCodec decoder
      if (avformat_open_input(&LibavAudioFile,FileName().toLocal8Bit(),NULL,NULL) != 0) 
         return false;
      LibavAudioFile->flags |= AVFMT_FLAG_GENPTS;       // Generate missing pts even if it requires parsing future NbrFrames.
      if (avformat_find_stream_info(LibavAudioFile,NULL) < 0) 
      {
         avformat_close_input(&LibavAudioFile);
         return false;
      }

      AVStream *AudioStream = LibavAudioFile->streams[AudioStreamNumber];

      // Setup STREAM options
      AudioStream->discard = AVDISCARD_DEFAULT;

      // Find the decoder for the audio stream and open it
      AudioDecoderCodec = avcodec_find_decoder(AudioStream->codec->codec_id);

      // Setup decoder options
      AVDictionary * av_opts = NULL;
      #if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
      AudioStream->codec->debug_mv          = 0;                    // Debug level (0=nothing)
      AudioStream->codec->debug             = 0;                    // Debug level (0=nothing)
      AudioStream->codec->workaround_bugs   = 1;                    // Work around bugs in encoders which sometimes cannot be detected automatically : 1=autodetection
      AudioStream->codec->idct_algo         = FF_IDCT_AUTO;         // IDCT algorithm, 0=auto
      AudioStream->codec->skip_frame        = AVDISCARD_DEFAULT;    // ???????
      AudioStream->codec->skip_idct         = AVDISCARD_DEFAULT;    // ???????
      AudioStream->codec->skip_loop_filter  = AVDISCARD_DEFAULT;    // ???????
      AudioStream->codec->error_concealment = 3;
      AudioStream->codec->thread_count      = getCpuCount();
      AudioStream->codec->thread_type       = getThreadFlags(AudioStream->codec->codec_id);
      #else
      av_dict_set(&av_opts, "thread_count", QString("%1").arg(getCpuCount()).toLocal8Bit().constData(), 0);
      #endif
      if ((AudioDecoderCodec == NULL) || (avcodec_open2(AudioStream->codec,AudioDecoderCodec, &av_opts) < 0))
      {
         //Mutex.unlock();
         av_dict_free(&av_opts);
         return false;
      }
      av_dict_free(&av_opts);
      IsVorbis = (strcmp(AudioDecoderCodec->name,"vorbis") == 0);

      if(LibavAudioFile->start_time != AV_NOPTS_VALUE )
         LibavStartTime = abs(LibavAudioFile->start_time);
      else
         LibavStartTime = 0; 
      AVStartTime = QTime(0,0,0,0).addMSecs((LibavStartTime*1000)/AV_TIME_BASE);
      //qDebug() << "AVSTartTime " << AVStartTime.toString("hh:mm:ss.zzz");
      //AVStartTime    = QTime(0,0,0,0);
      //LibavStartTime = 0;
   }

   //**********************************
   // Open video stream
   //**********************************
   if ((VideoStreamNumber != -1) && (!MusicOnly)) 
   {
      // Open the file and get a LibAVFormat context and an associated LibAVCodec decoder
      if (avformat_open_input(&LibavVideoFile,FileName().toLocal8Bit(),NULL,NULL) != 0) 
         return false;
      LibavVideoFile->flags |= AVFMT_FLAG_GENPTS;       // Generate missing pts even if it requires parsing future NbrFrames.
      if (avformat_find_stream_info(LibavVideoFile,NULL) < 0) 
      {
         avformat_close_input(&LibavVideoFile);
         return false;
      }

      AVStream *VideoStream = LibavVideoFile->streams[VideoStreamNumber];

      // Setup STREAM options
      VideoStream->discard = AVDISCARD_DEFAULT;

      // Find the decoder for the video stream and open it
      VideoDecoderCodec = avcodec_find_decoder(VideoStream->codec->codec_id);

      // Setup decoder options
      AVDictionary * av_opts = NULL;
      #if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
      VideoStream->codec->debug_mv          = 0;                    // Debug level (0=nothing)
      VideoStream->codec->debug             = 0;                    // Debug level (0=nothing)
      VideoStream->codec->workaround_bugs   = 1;                    // Work around bugs in encoders which sometimes cannot be detected automatically : 1=autodetection
      VideoStream->codec->idct_algo         = FF_IDCT_AUTO;         // IDCT algorithm, 0=auto
      VideoStream->codec->skip_frame        = AVDISCARD_DEFAULT;    // ???????
      VideoStream->codec->skip_idct         = AVDISCARD_DEFAULT;    // ???????
      VideoStream->codec->skip_loop_filter  = AVDISCARD_DEFAULT;    // ???????
      VideoStream->codec->error_concealment = 3;
      VideoStream->codec->thread_count      = getCpuCount();
      VideoStream->codec->thread_type       = getThreadFlags(VideoStream->codec->codec_id);
      #else
      av_dict_set(&av_opts, "thread_count", QString("%1").arg(getCpuCount()).toLocal8Bit().constData(), 0);
      #endif
      // Hack to correct wrong frame rates that seem to be generated by some codecs
      if (VideoStream->codec->time_base.num > 1000 && VideoStream->codec->time_base.den == 1)
         VideoStream->codec->time_base.den = 1000;

      if ((VideoDecoderCodec == NULL) || (avcodec_open2(VideoStream->codec, VideoDecoderCodec, &av_opts) < 0))
      {
         av_dict_free(&av_opts);
         return false;
      }
      av_dict_free(&av_opts);
      if(LibavVideoFile->start_time != AV_NOPTS_VALUE )
         LibavStartTime = abs(LibavVideoFile->start_time);
      else
         LibavStartTime = 0; 
      AVStartTime = QTime(0,0,0,0).addMSecs((LibavStartTime*1000)/AV_TIME_BASE);
      //qDebug() << "AVSTartTime " << AVStartTime.toString("hh:mm:ss.zzz");
   }
   IsOpen = true;
   //getMemInfo();
   return IsOpen;
}

//*********************************************************************************************************************************************
// Base object for music definition
//*********************************************************************************************************************************************

cMusicObject::cMusicObject(cApplicationConfig *ApplicationConfig):cVideoFile(ApplicationConfig) 
{
    Volume      = -1;                            // Volume as % from 1% to 150% or -1=auto
    AllowCredit = true;                          // // if true, this music will appear in credit title
    ForceFadIn  = 0;
    ForceFadOut = 0;
    Reset(OBJECTTYPE_MUSICFILE);
    startCode = eDefault_Start;
    startOffset = 0;
}

//====================================================================================================================
// Overloaded function use to dertermine if format of media file is correct
bool cMusicObject::CheckFormatValide(QWidget *Window) 
{
   bool IsOk = IsValide;

   // try to open file
   if (!OpenCodecAndFile()) 
   {
      QString ErrorMessage = QApplication::translate("MainWindow","Format not supported","Error message");
      CustomMessageBox(Window,QMessageBox::Critical,QApplication::translate("MainWindow","Error","Error message"),ShortName()+"\n\n"+ErrorMessage,QMessageBox::Close);
      IsOk = false;
   }

   // check if file have at least one sound track compatible
   if ((IsOk) && (AudioStreamNumber == -1)) 
   {
      QString ErrorMessage = QApplication::translate("MainWindow","No audio track found","Error message")+"\n";
      CustomMessageBox(Window,QMessageBox::Critical,QApplication::translate("MainWindow","Error","Error message"),ShortName()+"\n\n"+ErrorMessage,QMessageBox::Close);
      IsOk = false;

   } 
   else 
   {
      if (!((LibavAudioFile->streams[AudioStreamNumber]->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_S16) || (LibavAudioFile->streams[AudioStreamNumber]->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_U8)))
      {
         QString ErrorMessage = QApplication::translate("MainWindow","This application support only audio track with unsigned 8 bits or signed 16 bits sample format","Error message")+"\n";
         CustomMessageBox(Window,QMessageBox::Critical,QApplication::translate("MainWindow","Error","Error message"),ShortName()+"\n\n"+ErrorMessage,QMessageBox::Close);
         IsOk = false;
      }
   }
   // close file if it was opened
   CloseCodecAndFile();

   return IsOk;
}

//====================================================================================================================

void cMusicObject::SaveToXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel)
{
   QDomDocument DomDocument;
   QString xmlFragment;
   ExXmlStreamWriter xlmStream(&xmlFragment);
   SaveToXMLex(xlmStream, ElementName, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
   DomDocument.setContent(xmlFragment);
   ParentElement->appendChild(DomDocument.firstChildElement());
   return;
   /*
   QDomDocument    DomDocument;
    QDomElement     Element=DomDocument.createElement(ElementName);
    QString         TheFileName;

    if (ReplaceList) {
        TheFileName=ReplaceList->GetDestinationFileName(FileName());
    } else if (PathForRelativPath!="") {
        if (ForceAbsolutPath) TheFileName=QDir(QFileInfo(PathForRelativPath).absolutePath()).absoluteFilePath(FileName());
            else TheFileName=QDir(QFileInfo(PathForRelativPath).absolutePath()).relativeFilePath(FileName());
    } else TheFileName=FileName();

    Element.setAttribute("FilePath",     TheFileName);
    Element.setAttribute("iStartPos",    QTime(0,0,0,0).msecsTo(StartTime));
    Element.setAttribute("iEndPos",      QTime(0,0,0,0).msecsTo(EndTime));
    Element.setAttribute("Volume",       QString("%1").arg(Volume,0,'f'));
    Element.setAttribute("AllowCredit",  AllowCredit?"1":"0");
    Element.setAttribute("ForceFadIn",   qlonglong(ForceFadIn));
    Element.setAttribute("ForceFadOut",  qlonglong(ForceFadOut));
    Element.setAttribute("GivenDuration",QTime(0,0,0,0).msecsTo(GetGivenDuration()));
    if (IsComputedAudioDuration) {
        Element.setAttribute("RealAudioDuration",      QTime(0,0,0,0).msecsTo(GetRealAudioDuration()));
        Element.setAttribute("IsComputedAudioDuration",IsComputedAudioDuration?"1":0);
        Element.setAttribute("SoundLevel",             QString("%1").arg(SoundLevel,0,'f'));
    }
    ParentElement->appendChild(Element);
    */
}
void cMusicObject::SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel)
{
   xmlStream.writeStartElement(ElementName);
   QString         TheFileName;

   if (ReplaceList)
   {
      TheFileName = ReplaceList->GetDestinationFileName(FileName());
   }
   else if (PathForRelativPath != "")
   {
      if (ForceAbsolutPath) 
         TheFileName = QDir(QFileInfo(PathForRelativPath).absolutePath()).absoluteFilePath(FileName());
      else 
         TheFileName = QDir(QFileInfo(PathForRelativPath).absolutePath()).relativeFilePath(FileName());
   }
   else 
      TheFileName = FileName();

   xmlStream.writeAttribute("FilePath", TheFileName);
   xmlStream.writeAttribute("iStartPos", QTime(0, 0, 0, 0).msecsTo(StartTime));
   xmlStream.writeAttribute("iEndPos", QTime(0, 0, 0, 0).msecsTo(EndTime));
   xmlStream.writeAttribute("Volume", QString("%1").arg(Volume, 0, 'f'));
   xmlStream.writeAttribute("AllowCredit", AllowCredit );
   xmlStream.writeAttribute("ForceFadIn", qlonglong(ForceFadIn));
   xmlStream.writeAttribute("ForceFadOut", qlonglong(ForceFadOut));
   xmlStream.writeAttribute("GivenDuration", QTime(0, 0, 0, 0).msecsTo(GetGivenDuration()));
   if (IsComputedAudioDuration)
   {
      xmlStream.writeAttribute("RealAudioDuration", QTime(0, 0, 0, 0).msecsTo(GetRealAudioDuration()));
      xmlStream.writeAttribute("IsComputedAudioDuration", IsComputedAudioDuration );
      xmlStream.writeAttribute("SoundLevel", QString("%1").arg(SoundLevel, 0, 'f'));
   }
   xmlStream.writeEndElement();
}

//====================================================================================================================

bool cMusicObject::LoadFromXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag) {
    if ((ParentElement->elementsByTagName(ElementName).length()>0)&&(ParentElement->elementsByTagName(ElementName).item(0).isElement()==true)) {
        QDomElement Element=ParentElement->elementsByTagName(ElementName).item(0).toElement();

        QString FileName=Element.attribute("FilePath","");
        if ((!QFileInfo(FileName).exists())&&(PathForRelativPath!="")) {
            FileName=QDir::cleanPath(QDir(PathForRelativPath).absoluteFilePath(FileName));
            // Fixes a previous bug in relative path
            #ifndef Q_OS_WIN
            if (FileName.startsWith("/..")) {
                if (FileName.contains("/home/")) FileName=FileName.mid(FileName.indexOf("/home/"));
                if (FileName.contains("/mnt/"))  FileName=FileName.mid(FileName.indexOf("/mnt/"));
            }
            #endif
        }
        if (GetInformationFromFile(FileName,AliasList,ModifyFlag)&&(CheckFormatValide(NULL))) {
            // Old format prior to ffDiaporama 2.2.2014.0308
            if (Element.hasAttribute("StartPos")) StartTime = QTime().fromString(Element.attribute("StartPos"));
            if (Element.hasAttribute("EndPos"))   EndTime = QTime().fromString(Element.attribute("EndPos"));
            // New format since ffDiaporama 2.2.2014.0308
            if (Element.hasAttribute("iStartPos")) StartTime = QTime(0,0,0,0).addMSecs(Element.attribute("iStartPos").toLongLong());
            if (Element.hasAttribute("iEndPos"))   EndTime = QTime(0,0,0,0).addMSecs(Element.attribute("iEndPos").toLongLong());

            if (Element.hasAttribute("Volume"))                  Volume=GetDoubleValue(Element,"Volume");
            if (Element.hasAttribute("GivenDuration"))           SetGivenDuration(QTime(0,0,0,0).addMSecs(Element.attribute("GivenDuration").toLongLong()));
            if (Element.hasAttribute("IsComputedDuration"))      IsComputedAudioDuration=Element.attribute("IsComputedDuration")=="1";
            if (Element.hasAttribute("IsComputedAudioDuration")) IsComputedAudioDuration=Element.attribute("IsComputedAudioDuration")=="1";
            if (Element.hasAttribute("RealDuration"))            SetRealAudioDuration(QTime(0,0,0,0).addMSecs(Element.attribute("RealDuration").toLongLong()));
            if (Element.hasAttribute("RealAudioDuration"))       SetRealAudioDuration(QTime(0,0,0,0).addMSecs(Element.attribute("RealAudioDuration").toLongLong()));
            if (Element.hasAttribute("SoundLevel"))              SoundLevel =GetDoubleValue(Element,"SoundLevel");
            if (Element.hasAttribute("AllowCredit"))        AllowCredit=Element.attribute("AllowCredit")=="1";
            if (Element.hasAttribute("ForceFadIn"))         ForceFadIn =Element.attribute("ForceFadIn").toLongLong();
            if (Element.hasAttribute("ForceFadOut"))        ForceFadOut=Element.attribute("ForceFadOut").toLongLong();
            return true;
        } else return false;
    } else return false;
}

//====================================================================================================================

QTime cMusicObject::GetDuration() {
    return EndTime.addMSecs(-QTime(0,0,0,0).msecsTo(StartTime));
}

//====================================================================================================================

qreal cMusicObject::GetFading(int64_t Position,bool SlideHaveFadIn,bool SlideHaveFadOut) 
{
   int64_t RealFadIN = ForceFadIn;
   int64_t RealFadOUT= ForceFadOut;
   int64_t Duration  = QTime(0,0,0,0).msecsTo(GetDuration());
   qreal   RealVolume= Volume;
   // If fade duration longer than duration, then reduce them
   if (Duration < (RealFadIN + RealFadOUT)) 
   {
      qreal Ratio = qreal(RealFadIN+RealFadOUT)/qreal(Duration);
      RealFadIN  = int64_t(qreal(RealFadIN)/Ratio);
      RealFadOUT = int64_t(qreal(RealFadOUT)/Ratio);
   }
   if (RealVolume == -1) 
      RealVolume = 1;
   if ((!SlideHaveFadIn) && (Position < RealFadIN)) 
   {
      qreal PCTDone = ComputePCT(SPEEDWAVE_SINQUARTER,double(Position)/double(RealFadIN));
      RealVolume = RealVolume*PCTDone;
   }
   if ((!SlideHaveFadOut) && (Position>(Duration - RealFadOUT))) 
   {
      if( RealFadOUT > 0 )
      {
         qreal PCTDone = ComputePCT(SPEEDWAVE_SINQUARTER,double(Position-(Duration-RealFadOUT))/double(RealFadOUT));
         RealVolume = RealVolume*(1-PCTDone);
      }
      else
         RealVolume = 0;
   }
   if (RealVolume < 0) 
      RealVolume = 0;
   //if (RealVolume == -1) 
   //   RealVolume = 1;
   return RealVolume;
}


// Remark: Position must use AV_TIMEBASE Unit
QImage *cMusicObject::ReadFrame(bool PreviewMode, int64_t Position, bool DontUseEndPos, bool /*Deinterlace*/, cSoundBlockList *SoundTrackBloc, double Volume, bool /*ForceSoundOnly*/, int NbrDuration) 
{
   //qDebug() << "cMusicObject::ReadFrame PreviewMode " << PreviewMode << " Position " << Position << " DontUseEndPos " << DontUseEndPos << " Volume " << Volume << " NbrDuration " << NbrDuration;
   // Ensure file was previously open
   if (!IsOpen && !OpenCodecAndFile()) 
      return NULL;

   double dEndFile = double(QTime(0,0,0,0).msecsTo(DontUseEndPos ? GetRealDuration() : EndTime))/1000.0;
   Position += LibavStartTime;///AV_TIME_BASE;
   dEndFile += double(QTime(0,0).msecsTo(AVStartTime))/1000.0;//LibavStartTime/AV_TIME_BASE;
   if (dEndFile == 0) 
   {
      ToLog(LOGMSG_CRITICAL,"Error in cVideoFile::ReadFrame : dEndFile=0 ?????");
      return NULL;
   }
   if (Position < 0) 
      Position = 0;

   cVideoFile::sAudioContext AudioContext;
   AudioContext.SoundTrackBloc = SoundTrackBloc;
   AudioContext.AudioStream    = (AudioStreamNumber >= 0 && SoundTrackBloc) ? LibavAudioFile->streams[AudioStreamNumber] : NULL;
   AudioContext.FPSSize        = SoundTrackBloc ? SoundTrackBloc->SoundPacketSize*SoundTrackBloc->NbrPacketForFPS : 0;
   AudioContext.FPSDuration    = AudioContext.FPSSize ? (double(AudioContext.FPSSize)/(SoundTrackBloc->Channels*SoundTrackBloc->SampleBytes*SoundTrackBloc->SamplingRate))*AV_TIME_BASE : 0;
   AudioContext.TimeBase       = AudioContext.AudioStream ? double(AudioContext.AudioStream->time_base.den)/double(AudioContext.AudioStream->time_base.num) : 0;
   AudioContext.DstSampleSize  = SoundTrackBloc ? (SoundTrackBloc->SampleBytes*SoundTrackBloc->Channels) : 0;
   AudioContext.NeedResampling = false;
   AudioContext.AudioLenDecoded= 0;
   AudioContext.Counter        = 20; // Retry counter (when len>0 and avcodec_decode_audio4 fail to retreave frame, we retry counter time before to discard the packet)
   AudioContext.Volume         = Volume;
   AudioContext.dEndFile       = &dEndFile;
   AudioContext.NbrDuration    = NbrDuration;
   AudioContext.DontUseEndPos  = DontUseEndPos;
   if (!AudioContext.AudioStream )
      return NULL;

   if (!AudioContext.FPSDuration) 
   {
      if (PreviewMode)            
         AudioContext.FPSDuration = double(AV_TIME_BASE) / ApplicationConfig->PreviewFPS;
      else                    
         AudioContext.FPSDuration = double(AV_TIME_BASE) / double(SoundTrackBloc->SamplingRate);
   }


   Mutex.lock();

   // If position >= end of file : disable audio (only if IsComputedAudioDuration)
   double dPosition = double(Position) / AV_TIME_BASE;
   if (dPosition > 0 && dPosition >= dEndFile-0.1 && IsComputedAudioDuration) 
   {
      AudioContext.AudioStream = NULL; // Disable audio

      // If not then change Position to end file - a FPS to prepare a last image
      Position = dEndFile * AV_TIME_BASE - AudioContext.FPSDuration;
      dPosition = double(Position) / AV_TIME_BASE;
      if (SoundTrackBloc) 
         SoundTrackBloc->UseLatestData();
   }

   AudioContext.ContinueAudio = (AudioContext.AudioStream) && (SoundTrackBloc);
   bool ResamplingContinue = (Position != 0);
   AudioContext.AudioFramePosition = dPosition;

   if (AudioContext.ContinueAudio) 
   {
      AudioContext.NeedResampling = ((AudioContext.AudioStream->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_S16)
         || (AudioContext.AudioStream->CODEC_OR_PAR->channels != SoundTrackBloc->Channels)
         || (AudioContext.AudioStream->CODEC_OR_PAR->sample_rate != SoundTrackBloc->SamplingRate));

      //qDebug() << "needResampling " << AudioContext.NeedResampling;
      // Calc if we need to seek to a position
      int64_t Start = /*SoundTrackBloc->LastReadPosition;*/SoundTrackBloc->CurrentPosition;
      int64_t End   = Start + SoundTrackBloc->GetDuration();
      int64_t Wanted = AudioContext.FPSDuration * AudioContext.NbrDuration;
      //qDebug() << "Start " << Start << " End " << End << " Position " << Position << " Wanted " << Wanted;

      // ?????
      if ( (Position >= Start && Position + Wanted <= End) /*|| Start < 0 */) 
         AudioContext.ContinueAudio = false;

      if ( AudioContext.ContinueAudio && ( Position == 0 || Start < 0 || LastAudioReadPosition < 0 /*|| Position < Start*/ || Position > End + 1500000 ) ) 
      {
         if (Position < 0) 
            Position = 0;
         SoundTrackBloc->ClearList();                // Clear soundtrack list
         ResamplingContinue = false;
         LastAudioReadPosition = 0;
         SeekErrorCount = 0;
         int64_t seekPos = Position - SoundTrackBloc->WantedDuration * 1000.0;
         /*bool bSeekRet = */SeekFile(NULL,AudioContext.AudioStream,seekPos);        // Always seek one FPS before to ensure eventual filter have time to init
         //qDebug() << "seek to " << seekPos << " is " << (bSeekRet?"true":"false");
         AudioContext.AudioFramePosition = Position / AV_TIME_BASE;
      }

      // Prepare resampler
      if (AudioContext.ContinueAudio && AudioContext.NeedResampling)
      {
         if (!ResamplingContinue) 
            CloseResampler();
         CheckResampler(AudioContext.AudioStream->CODEC_OR_PAR->channels,SoundTrackBloc->Channels,
            AVSampleFormat(AudioContext.AudioStream->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT),SoundTrackBloc->SampleFormat,
            AudioContext.AudioStream->CODEC_OR_PAR->sample_rate,SoundTrackBloc->SamplingRate
            ,AudioContext.AudioStream->CODEC_OR_PAR->channel_layout
            ,av_get_default_channel_layout(SoundTrackBloc->Channels)
            );
      }
   }

   //*************************************************************************************************************************************
   // Decoding process : Get StreamPackets until endposition is reached (if sound is wanted) 
   //*************************************************************************************************************************************
   while (AudioContext.ContinueAudio) 
   {
      AVPacket *StreamPacket = new AVPacket();
      if (!StreamPacket) 
      {
         AudioContext.ContinueAudio = false;
      } 
      else 
      {
         av_init_packet(StreamPacket);
         StreamPacket->flags |= AV_PKT_FLAG_KEY;
         int err;
         if ( (err = av_read_frame(LibavAudioFile,StreamPacket)) < 0) 
         {
            //qDebug() << "av_read_frame gives " << err;
            // If error reading frame then we considere we have reach the end of file
            if (!IsComputedAudioDuration) 
            {
               dEndFile = qreal(SoundTrackBloc->/*LastReadPosition*/CurrentPosition) / AV_TIME_BASE;
               dEndFile += qreal(SoundTrackBloc->GetDuration()) / 1000;
               if (dEndFile - LibavStartTime/ AV_TIME_BASE == double(QTime(0,0,0,0).msecsTo(EndTime)) / 1000) 
                  EndTime = QTime(0,0,0).addMSecs((dEndFile - LibavStartTime)*1000);
               SetRealAudioDuration(QTime(0,0,0,0).addMSecs(qlonglong((dEndFile-LibavStartTime/ AV_TIME_BASE)*1000)));
            }
            AudioContext.ContinueAudio = false;
            //qDebug() << "cVideoFile::ReadFrame audio, set AudioContext.ContinueAudio to false!!!!! err is "<<err;

            // Use data in TempData to create a latest block
            SoundTrackBloc->UseLatestData();
         } 
         else 
         {
            DecodeAudio(&AudioContext, StreamPacket, Position);
            StreamPacket = NULL;
         }
      }
      // Continue with a new one
      if (StreamPacket != NULL) 
      {
         AV_FREE_PACKET(StreamPacket); // Free the StreamPacket that was allocated by previous call to av_read_frame
         delete StreamPacket;
         StreamPacket = NULL;
      }
   }

   Mutex.unlock();
   return NULL;
}
#endif

