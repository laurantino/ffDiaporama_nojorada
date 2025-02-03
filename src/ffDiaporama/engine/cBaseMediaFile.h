// changed by gerd
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

#ifndef CBASEMEDIAFILE_H
#define CBASEMEDIAFILE_H

// Basic inclusions (common to all files)
#include "_GlobalDefines.h"

// Include some common various class
#include "cApplicationConfig.h"

// Include some additional standard class
#include <QString>
#include <QTime>
#include <QDateTime>
#include <QImage>

#if QT_VERSION >= 0x050000
    #include <QtSvg/QtSvg>
#else
    #include <QtSvg>
#endif

// Include some common various class
#include "cDeviceModelDef.h"                // Contains Libav include
#include "cSoundBlockList.h"
#include "cCustomIcon.h"

extern bool Exiv2WithPreview;
extern int  Exiv2MajorVersion, Eviv2MinorVersion, Exiv2PatchVersion;

// Utility defines and constant to manage angles
const double dPI=     3.14159265358979323846;
#define RADIANS(a)    (a*dPI/180)
#define DEGREES(a)    (a*180/dPI)
#define KMTOMILES(KM) ((KM*0.621371192))

// Utility defines and macro used to managed GPS and Google Pixel unit
#define GPS2PIXEL_X(GPSX,ZOOMLEVEL,SCALE)   ((256*(GPSX+180)/360)*(pow((double)2,(int)ZOOMLEVEL))*SCALE)
#define GPS2PIXEL_Y(GPSY,ZOOMLEVEL,SCALE)   ((256/2-log(tan((dPI/4)+RADIANS(GPSY)/2))*(256/(2*dPI)))*(pow((double)2,(int)ZOOMLEVEL))*SCALE)
#define PIXEL2GPS_X(PIXELX,ZOOMLEVEL,SCALE) (((PIXELX)/(SCALE*(pow((double)2,(int)ZOOMLEVEL))))*360/256-180)
#define PIXEL2GPS_Y(PIXELY,ZOOMLEVEL,SCALE) (DEGREES((atan(exp((-((PIXELY)/(SCALE*(pow((double)2,(int)ZOOMLEVEL))))+256/2)/(256/(2*dPI))))-(dPI/4))*2))

// Distance computation: See wikipedia at http://fr.wikipedia.org/wiki/Distance_du_grand_cercle
#define DISTANCE(GPS0x,GPS0y,GPS1x,GPS1y)   (2*6371*asin(sqrt(pow(sin((RADIANS(GPS1y)-RADIANS(GPS0y))/2),2)+cos(RADIANS(GPS0y))*cos(RADIANS(GPS1y))*pow(sin((RADIANS(GPS1x)-RADIANS(GPS0x))/2),2))))

#ifndef Q_OS_WIN
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "ffTools.h"
int geometryFromSize(double width, double height);
//============================================
// Class for exporting project
//============================================
class cReplaceObject 
{
   public:
      QString SourceFileName;
      QString DestFileName;
      cReplaceObject(QString SourceFileName,QString DestFileName) 
      {
         this->SourceFileName = SourceFileName; 
         this->DestFileName = DestFileName;
      }
};

class cReplaceObjectList
{
   public:
      QList<cReplaceObject> replaceObjects;
      cReplaceObjectList();

      void SearchAppendObject(QString SourceFileName);
      QString GetDestinationFileName(QString SourceFileName);
};

//****************************************************************************************************************************************************************
// abstract base-class for media-files
class cBaseMediaFile 
{
   protected:
      OBJECTTYPE ObjectType;          

      qlonglong fileKey;        // Key index of this file in the Files table of the database
      qlonglong folderKey;      // Key index of the folder containing this file in the Folders table of the database
      qlonglong ressourceKey;   // Key index of this ressource in the slidethumb table of the database
      QString   cachedFileName; // To speed up browser

      QTime RealAudioDuration;  // Real duration of audio
      QTime RealVideoDuration;  // Real duration of video
      QTime GivenDuration;      // Duration as given by libav/ffmpeg
      qreal SoundLevel;         // Sound level
                                               
      int64_t   fileSize;       // filesize
      QDateTime creatDateTime;  // Original date/time
      QDateTime modifDateTime;  // Last modified date/time
                                
      int  imageWidth;          // Widht of normal image
      int  imageHeight;         // Height of normal image
      int  imageOrientation;    // EXIF ImageOrientation (or -1)
      int  geometry;            // Image geometry of the embeded image or video
      double aspectRatio;       // Aspect ratio
      cApplicationConfig *ApplicationConfig;  
      QString            ObjectName;          // ObjectName in XML .ffd file

      QMutex accessMutex;

   public:
      enum    ImageSizeFmt {FULLWEB,SIZEONLY,FMTONLY,GEOONLY};

      bool               IsValide;            // if true if object if initialised
      bool               IsInformationValide; // if true if ExtendedProperties are full initialised in the database

      cBaseMediaFile(cApplicationConfig *ApplicationConfig);
      virtual ~cBaseMediaFile();

      // some pure virtual functions (needs to be overloaded in derived classes)
      virtual QString GetFileTypeStr() = 0;
      virtual QImage *GetDefaultTypeIcon(cCustomIcon::IconSize) = 0;
      virtual bool LoadBasicInformationFromDatabase(QDomElement *, QString, QString, QStringList *, bool *, TResKeyList *, bool) = 0;
      virtual void SaveBasicInformationToDatabase(QDomElement *, QString, QString, bool, cReplaceObjectList *, QList<qlonglong> *, bool) = 0;
      virtual void SaveBasicInformationToDatabase(ExXmlStreamWriter &xmlStream, QString, QString, bool, cReplaceObjectList *, QList<qlonglong> *, bool) = 0;
      virtual QString GetTechInfo(QStringList *) = 0;
      virtual QString GetTAGInfo(QStringList *) = 0; // Return TAG information as formated string


      qlonglong FileKey() { return fileKey; }
      void setFileKey(qlonglong rkey) { fileKey = rkey; }
      qlonglong FolderKey() { return folderKey; }
      void setFolderKey(qlonglong rkey) { folderKey = rkey; }
      qlonglong RessourceKey() { return ressourceKey; }
      void setRessourceKey(qlonglong rkey) { ressourceKey = rkey; }
      QString CachedFileName() { return cachedFileName; }

      virtual QString FileName();
      virtual QString ShortName();
      virtual QImage  *ImageAt(bool /*PreviewMode*/) {return NULL;}
      virtual QImage  *ImageAt(bool /*PreviewMode*/, QSizeF Size) { return NULL; }

      virtual bool LoadFromXML(QDomElement *,QString,QString,QStringList *,bool *,TResKeyList *,bool )        {return true;}
      virtual void SaveToXML(QDomElement *,QString,QString,bool,cReplaceObjectList *,QList<qlonglong> *,bool) {}
      virtual void SaveToXMLex(ExXmlStreamWriter &xmlStream, QString, QString, bool, cReplaceObjectList *, QList<qlonglong> *, bool) {}

                   
      virtual void Reset();
      virtual bool GetInformationFromFile(QString& GivenFileName,QStringList *AliasList,bool *ModifyFlag,qlonglong FolderKey=-1);
      virtual bool CheckFormatValide(QWidget *)                                                                                                        {return true;}
      virtual bool GetFullInformationFromFile(bool IsPartial=false);
      virtual bool GetChildFullInformationFromFile(bool,cCustomIcon *,QStringList *)                                                                   {return true;}

      // return information from basic properties
      virtual QString GetFileDateTimeStr(bool Created=false);       // Return file date/time as formated string
      virtual QString GetFileSizeStr();                             // Return file size as formated string
      virtual QString GetImageSizeStr(ImageSizeFmt Fmt=FULLWEB);    // Return image size as formated string
      virtual QString GetImageGeometryStr();                        // Return image geometry as formated string

      // return information from extended properties
      virtual QStringList GetSummaryText(QStringList *ExtendedProperties);    // return 3 lines to display Summary of media file in dialog box which need them

      // return icon
      virtual QImage GetIcon(cCustomIcon::IconSize Size,bool useDelayed);

   public:
      bool IsComputedAudioDuration; // True if audio duration was computed
      bool IsComputedVideoDuration; // True if video duration was computed

      QTime GetRealDuration();
      QTime GetRealAudioDuration();
      QTime GetRealVideoDuration();
      QTime GetGivenDuration();
      void  SetGivenDuration(QTime GivenDuration);
      void  SetRealAudioDuration(QTime RealDuration);
      void  SetRealVideoDuration(QTime RealDuration);
      qreal GetSoundLevel()                         { return SoundLevel;        }
      void  SetSoundLevel(qreal TheSoundLevel)      { SoundLevel=TheSoundLevel; }

      OBJECTTYPE getObjectType() { return ObjectType; }
      void setObjectType(OBJECTTYPE objType) { ObjectType = objType; }
      bool is_Unmanaged()     { return ObjectType == OBJECTTYPE_UNMANAGED; }
      bool is_Managed()       { return ObjectType == OBJECTTYPE_MANAGED; }
      bool is_Folder()        { return ObjectType == OBJECTTYPE_FOLDER; }
      bool is_FFDfile()       { return ObjectType == OBJECTTYPE_FFDFILE; }
      bool is_Imagefile()     { return ObjectType == OBJECTTYPE_IMAGEFILE; }
      bool is_Videofile()     { return ObjectType == OBJECTTYPE_VIDEOFILE; }
      bool is_Musicfile()     { return ObjectType == OBJECTTYPE_MUSICFILE; }
      bool is_Thumbnail()     { return ObjectType == OBJECTTYPE_THUMBNAIL; }
      bool is_Imagevector()   { return ObjectType == OBJECTTYPE_IMAGEVECTOR; }
      bool is_Imageclipboard(){ return ObjectType == OBJECTTYPE_IMAGECLIPBOARD; }
      bool is_Gmapsmap()      { return ObjectType == OBJECTTYPE_GMAPSMAP; }

      bool setSlideThumb(QImage Thumb);
      bool getSlideThumb(QImage *Thumb);

      QDateTime modificationTime() { return modifDateTime; }
      int64_t FileSize() { return fileSize; }     // filesize

      int width() { return imageWidth; }
      int height() { return imageHeight; }
      int Orientation() { return imageOrientation; }       // EXIF ImageOrientation (or -1)
      void setOrientation(int i) { imageOrientation = i; }
      int Geometry() { return geometry; }                  // Image geometry of the embeded image or video
      void setGeometry(int i) { geometry = i; }             // Image geometry of the embeded image or video
      double AspectRatio() { return aspectRatio; }          // Aspect ratio

      virtual bool hasChapters() const { return false; }
      virtual int numChapters() const { return 0; }
      virtual void setNumChapters(int i ){ Q_UNUSED(i) }
};
cBaseMediaFile* getMediaObject(enum OBJECTTYPE objectType, int AllowedFilter, cApplicationConfig* ApplicationConfig);

//*********************************************************************************************************************************************
// Unmanaged file
//*********************************************************************************************************************************************
class cUnmanagedFile : public cBaseMediaFile 
{
   public:
      explicit cUnmanagedFile(cApplicationConfig *ApplicationConfig);

      virtual QString GetFileTypeStr();
      virtual QImage *GetDefaultTypeIcon(cCustomIcon::IconSize Size) { return ApplicationConfig->DefaultFILEIcon.GetIcon(Size); }
      virtual bool LoadBasicInformationFromDatabase(QDomElement *,QString,QString,QStringList *,bool *,TResKeyList *,bool) { return true; /*Nothing to do*/ }
      virtual void SaveBasicInformationToDatabase(QDomElement *,QString,QString,bool,cReplaceObjectList *,QList<qlonglong> *,bool) { /*Nothing to do*/}
      virtual void SaveBasicInformationToDatabase(ExXmlStreamWriter &xmlStream, QString, QString, bool, cReplaceObjectList *, QList<qlonglong> *, bool) { /*Nothing to do*/}
      virtual QString GetTechInfo(QStringList *) { return ""; }
      virtual QString GetTAGInfo(QStringList *)  { return ""; }
};

//*********************************************************************************************************************************************
// Folder
//*********************************************************************************************************************************************
class cFolder : public cBaseMediaFile 
{
   public:
      explicit cFolder(cApplicationConfig *ApplicationConfig);

      virtual QString GetFileTypeStr();
      virtual QImage *GetDefaultTypeIcon(cCustomIcon::IconSize Size) { return ApplicationConfig->DefaultFOLDERIcon.GetIcon(Size); }
      virtual bool LoadBasicInformationFromDatabase(QDomElement *,QString,QString,QStringList *,bool *,TResKeyList *,bool)  { return true; /*Nothing to do*/}
      virtual void SaveBasicInformationToDatabase(QDomElement *,QString,QString,bool,cReplaceObjectList *,QList<qlonglong> *,bool) { /*Nothing to do*/}
      virtual void SaveBasicInformationToDatabase(ExXmlStreamWriter &xmlStream, QString, QString, bool, cReplaceObjectList *, QList<qlonglong> *, bool) { /*Nothing to do*/}
      virtual QString GetTechInfo(QStringList *) { return ""; }
      virtual QString GetTAGInfo(QStringList *)  { return ""; }

      virtual bool GetChildFullInformationFromFile(bool IsPartial,cCustomIcon *Icon,QStringList *ExtendedProperties);
};

//*********************************************************************************************************************************************
// ffDiaporama project file
//*********************************************************************************************************************************************
class cLocation;
class cffDProjectFile : public cBaseMediaFile, public xmlReadWrite
{
   // TAG values
   QString     title;              // 30 or 200 char depending on ID3V2 compatibility option
   QString     author;             // 30 or 200 char depending on ID3V2 compatibility option
   QString     album;              // 30 or 200 char depending on ID3V2 compatibility option
   QDate       eventDate;
   bool        overrideDate;
   QString     longDate;           // Project dates
   QString     comment;            // Free text - free size
   QString     composer;           // ffDiaporama version
   int         nbrSlide;           // (Number of slide in project)
   QString     ffDRevision;        // ffD Application version (in reverse date format)
   QString     defaultLanguage;    // Default Language (ISO 639 language code)
   int         nbrChapters;        // Number of chapters in the file
   QStringList chaptersProperties; // Properties of chapters
   cLocation   *location;          // a link to a cLocation object

   public:
      explicit cffDProjectFile(cApplicationConfig *ApplicationConfig);
      ~cffDProjectFile();

      void InitDefaultValues();

      virtual QString GetFileTypeStr();
      virtual QImage  *GetDefaultTypeIcon(cCustomIcon::IconSize Size) { return ApplicationConfig->DefaultFFDIcon.GetIcon(Size); }
      virtual bool LoadBasicInformationFromDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes);
      virtual void SaveBasicInformationToDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
      virtual void SaveBasicInformationToDatabase(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel) {}
      virtual QString GetTechInfo(QStringList *ExtendedProperties);
      virtual QString GetTAGInfo(QStringList *ExtendedProperties);
      
      virtual void SetRealDuration(QTime RealDuration) { SetRealAudioDuration(RealDuration); } // Special case for project : audioduration is project duration
      virtual bool GetChildFullInformationFromFile(bool IsPartial,cCustomIcon *Icon,QStringList *ExtendedProperties);


      void SaveToXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
      void SaveToXMLex(ExXmlStreamWriter &xmlStream, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel);
      bool LoadFromXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes,bool IsPartial);

      QString Title() const { return title; }
      void setTitle(const QString &s) { title = s; }
      QString Author() const { return author; }
      void setAuthor(const QString &s) { author = s; }
      QString Album() const { return album; }
      void setAlbum(const QString &s) { album = s; }
      QString Comment() const { return comment; }
      void setComment(const QString &s) { comment = s; }
      QString Composer() const { return composer; }
      void setComposer(const QString &s) { composer = s; }
      QString Revision() const { return ffDRevision; }
      void setRevision(const QString &s) { ffDRevision = s; }
      QDate EventDate() const { return eventDate; }
      void setEventDate(const QDate &d) { eventDate = d; }
      QString LongDate() const { return longDate; }
      void setLongDate(const QString &s) { longDate = s; }
      QString DefaultLanguage() const { return defaultLanguage; }
      void setDefaultLanguage(const QString &s) { defaultLanguage = s; }
      cLocation *Location() const  { return location; }
      bool OverrideDate() const { return overrideDate; }
      void setOverrideDate(bool b) { overrideDate = b; }

      virtual bool hasChapters() const { return nbrChapters > 0; }
      virtual int numChapters() const { return nbrChapters; }
      virtual void setNumChapters(int i) { nbrChapters = i; }

      int numSlides() const { return nbrSlide; }
      void setNumSlides(int i) { nbrSlide = i; }
      //virtual const QStringList &chapterProps() const { return chaptersProperties; }
      QStringList /*const */*chapterProps() /*const*/ { return &chaptersProperties; }

      void clearLocation();
      cLocation *createLocation();
};

//*********************************************************************************************************************************************
// Image file
//*********************************************************************************************************************************************
class cImageFile : public cBaseMediaFile 
{
   protected:
      bool NoExifData;
   public:
      explicit cImageFile(cApplicationConfig *ApplicationConfig);
      ~cImageFile();

      virtual QString GetFileTypeStr();
      virtual QImage *GetDefaultTypeIcon(cCustomIcon::IconSize Size) { return ( is_Thumbnail() ? ApplicationConfig->DefaultThumbIcon : ApplicationConfig->DefaultIMAGEIcon).GetIcon(Size); }
      virtual bool LoadBasicInformationFromDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes);
      virtual void SaveBasicInformationToDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
      virtual void SaveBasicInformationToDatabase(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel) {}
      virtual QString GetTechInfo(QStringList *ExtendedProperties);
      virtual QString GetTAGInfo(QStringList *ExtendedProperties);

      virtual bool GetChildFullInformationFromFile(bool IsPartial,cCustomIcon *Icon,QStringList *ExtendedProperties);
      virtual bool CheckFormatValide(QWidget *Window);
      virtual bool GetInformationFromFile(QString& GivenFileName,QStringList *AliasList,bool *ModifyFlag,qlonglong FolderKey);
      virtual QImage *ImageAt(bool PreviewMode);
      virtual QImage  *ImageAt(bool PreviewMode, QSizeF Size);
};

//*********************************************************************************************************************************************
// Google maps map
//*********************************************************************************************************************************************
class cImageClipboard : public cImageFile 
{
   public:
      explicit cImageClipboard(cApplicationConfig *ApplicationConfig);
      ~cImageClipboard();

      virtual QString GetFileTypeStr() { return QApplication::translate("cBaseMediaFile","Image from clipboard","File type"); }
      virtual QImage *GetDefaultTypeIcon(cCustomIcon::IconSize Size) { return ApplicationConfig->DefaultIMAGEIcon.GetIcon(Size); }
      virtual bool LoadBasicInformationFromDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes);
      virtual void SaveBasicInformationToDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
      virtual void SaveBasicInformationToDatabase(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel);

      virtual bool GetInformationFromFile(QString& GivenFileName,QStringList *AliasList,bool *ModifyFlag,qlonglong FolderKey);
      virtual QString FileName()  { return QString(":/img/%1").arg(ressourceKey); }
      virtual QString ShortName() { return QString(":/img/%1").arg(ressourceKey); }
      virtual bool GetChildFullInformationFromFile(bool IsPartial,cCustomIcon *Icon,QStringList *ExtendedProperties);
      virtual QStringList GetSummaryText(QStringList *ExtendedProperties);   // return 3 lines to display Summary of media file in dialog box which need them

      virtual bool LoadFromXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes);
      virtual void SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel);
      virtual void SaveToXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
};

//*********************************************************************************************************************************************
// Google maps map
//*********************************************************************************************************************************************
class cGMapsMap : public cImageClipboard 
{
   public:
      static const int SectionWith   = 640;    // With of a section
      static const int SectionHeight = 600;    // Height of a section

      struct RequestSection {
         QRectF  Rect;                       // Portion of the destination image where this section is
         QString GoogleRequest;              // Google request to create this portion
      };

      QList<cLocation *>      locations;      // List of location (should be QList<cLocation *> but use void* because of .h chain)
      QList<RequestSection>   RequestList;    // List of pending Google requests to be used to create the map (if the list is empty then the map is fully created)
      int                     ZoomLevel;      // Google Zoom level of the actual map
      int                     Scale;          // Google Scale level of the actual map
      double                  MapCx;          // Center X position of the actual map in Google pixel unit
      double                  MapCy;          // Center Y position of the actual map in Google pixel unit
      bool                    IsMapValide;    // True if map was succesfully generated

      enum GMapsMapType {
         Roadmap,
         Satellite,
         Terrain,
         Hybrid,
         GMapsMapType_NBR
      } MapType;                              // Type of the map

      enum GMapsImageSize {
         Small,              // 640x360 (half 720p)
         FS720P,             // 1280x720
         FS720X4,            // 2560x1440
         FS720X9,            // 3840x2160
         FS1080P,            // 1920x1080
         FS1080X4,           // 3840x2160
         FS1080X9,           // 5760x3240
         GMapsImageSize_NBR
      } ImageSize;                            // Image size of the map

      explicit cGMapsMap(cApplicationConfig *ApplicationConfig);
      ~cGMapsMap();

      virtual QString GetFileTypeStr() { return QApplication::translate("cBaseMediaFile","Google Maps map","File type"); }
      virtual QImage *GetDefaultTypeIcon(cCustomIcon::IconSize Size) { return ApplicationConfig->DefaultGMapsIcon.GetIcon(Size); }
      virtual bool LoadBasicInformationFromDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes);
      virtual void SaveBasicInformationToDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
      virtual void SaveBasicInformationToDatabase(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel);
      virtual QString GetTechInfo(QStringList *ExtendedProperties);
      virtual QString GetTAGInfo(QStringList *ExtendedProperties);

    virtual void SaveToXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
    virtual void SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel);

    virtual QImage          *ImageAt(bool PreviewMode);

    virtual bool            GetInformationFromFile(QString& GivenFileName,QStringList *AliasList,bool *ModifyFlag,qlonglong FolderKey);
    virtual bool            GetChildFullInformationFromFile(bool IsPartial,cCustomIcon *Icon,QStringList *ExtendedProperties);
    virtual QStringList     GetSummaryText(QStringList *ExtendedProperties);   // return 3 lines to display Summary of media file in dialog box which need them

    virtual QStringList     GetGoogleMapTypeNames();
    virtual QStringList     GetMapTypeNames();
    virtual QStringList     GetImageSizeNames();
    virtual QStringList     GetShortImageSizeNames();

    virtual QString         GetCurrentGoogleMapTypeName();
    virtual QString         GetCurrentMapTypeName();

    virtual QString         GetCurrentImageSizeName(bool Full=true);
    virtual QSize           GetCurrentImageSize();

    virtual QImage          CreateDefaultImage(cDiaporama *Diaporama);  // Create a new empty image (to be fill by requests to Google)
    virtual int             ComputeNbrSection(int Size,int Divisor);    // Compute number of sections needed to create map for current image size
    virtual void            ComputeSectionList();                       // Create sections to request to Google

    virtual QStringList     GetMapSizesPerZoomLevel();                  // Compute Map Size for each Google Maps zoomlevel
    virtual QPoint          GetLocationPoint(int Index);

private:
    virtual QRectF          GetGPSRectF();                              // Return rectangle needed to handle all locations in GPS unit
    virtual QRectF          GetPixRectF();                              // Return rectangle needed to handle all locations in Google pixel unit
    virtual int             GetMinZoomLevelForSize();                   // Return minimum zoom level depending on current image size
};

//*********************************************************************************************************************************************
// Video file
//*********************************************************************************************************************************************
// extern int MAXCACHEIMAGE; !! not used
#ifndef IS_FFMPEG_414
class cImageInCache 
{
   public:
      int64_t     Position;
      AVFrame     *FiltFrame,*FrameBufferYUV;
      cImageInCache(int64_t Position,AVFrame *FiltFrame,AVFrame *FrameBufferYUV);
      ~cImageInCache();
};

class cVideoFile : public cBaseMediaFile 
{
   protected:
      bool                    IsOpen;                   // True if Libav open on this file
      bool                    MusicOnly;                // True if object is a music only file
      bool                    IsVorbis;                 // True if vorbis version must be use instead of MP3/WAV version
      bool                    IsMTS;                    // True if file is a MTS file

      QString                 Container;                // Container type (get from file extension)
      QString                 VideoCodecInfo;           
      QString                 AudioCodecInfo;           
      int64_t                 LastAudioReadPosition;  // Use to keep the last read position to determine if a seek is needed
      //!! not used int64_t                 LastVideoReadPosition;  // Use to keep the last read position to determine if a seek is needed 
                                                        
      // Video part                                     
      int                     VideoStreamNumber;        // Number of the video stream
      QImage                  LastImage;                // Keep last image return
      QList<cImageInCache *>  CacheImage;
      QMap<int64_t, cImageInCache *> YUVCache;

      int                     SeekErrorCount;

      // Audio part
      int                     AudioStreamNumber;                  // Number of the audio stream

   public:
      QTime                   StartTime;                // Start position
      QTime                   EndTime;                  // End position
      QTime                   AVStartTime;              // time from start_time information from the libavfile
      QTime                   AVEndTime;                // AVStartTime + RealDruation
      int64_t                 LibavStartTime;           // copy of the start_time information from the libavfile
      int                     VideoTrackNbr;            // Number of video stream in file
      int                     AudioTrackNbr;            // Number of audio stream in file
      int                     NbrChapters;              // Number of chapters in the file

      explicit                cVideoFile(cApplicationConfig *ApplicationConfig);
                              ~cVideoFile();
      virtual void            Reset(OBJECTTYPE TheWantedObjectType);

      virtual qreal           GetFPSDuration();
      virtual QString         GetFileTypeStr();
      virtual bool            LoadBasicInformationFromDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes);
      virtual void            SaveBasicInformationToDatabase(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
      virtual void            SaveBasicInformationToDatabase(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel) {}
      virtual bool            CheckFormatValide(QWidget *);
      virtual bool            GetChildFullInformationFromFile(bool IsPartial,cCustomIcon *Icon,QStringList *ExtendedProperties);
      virtual QImage          *GetDefaultTypeIcon(cCustomIcon::IconSize Size);

      virtual bool LoadAnalyseSound(QList<qreal> *Peak,QList<qreal> *Moyenne);
      virtual void SaveAnalyseSound(QList<qreal> *Peak,QList<qreal> *Moyenne,qreal MaxMoyenneValue);
                   

      virtual QString  GetTechInfo(QStringList *ExtendedProperties);
      virtual QString  GetTAGInfo(QStringList *ExtendedProperties);
      virtual bool     DoAnalyseSound(QList<qreal> *Peak,QList<qreal> *Moyenne,bool *IsAnalysed,qreal *Analysed);

      virtual int      getThreadFlags(AVCodecID ID);

      virtual bool     OpenCodecAndFile();
      virtual void     CloseCodecAndFile();

      virtual QImage   *ImageAt(bool PreviewMode,int64_t Position,cSoundBlockList *SoundTrackMontage,bool Deinterlace,double Volume,bool ForceSoundOnly,bool DontUseEndPos,int NbrDuration=2);
      virtual AVFrame   *YUVAt(bool PreviewMode, int64_t Position, cSoundBlockList *SoundTrackMontage, bool Deinterlace, double Volume, bool ForceSoundOnly, bool DontUseEndPos, int NbrDuration = 2);
      virtual QImage   *ReadFrame(bool PreviewMode,int64_t Position,bool DontUseEndPos,bool Deinterlace,cSoundBlockList *SoundTrackBloc,double Volume,bool ForceSoundOnly,int NbrDuration=2);
      virtual AVFrame   *ReadYUVFrame(bool PreviewMode, int64_t Position, bool DontUseEndPos, bool Deinterlace, cSoundBlockList *SoundTrackBloc, double Volume, bool ForceSoundOnly, int NbrDuration = 2);
      virtual QImage   *ConvertYUVToRGB(bool PreviewMode,AVFrame *Frame);

      virtual bool     SeekFile(AVStream *VideoStream,AVStream *AudioStream,int64_t Position);
      virtual void     CloseResampler();
      virtual void     CheckResampler(int RSC_InChannels,int RSC_OutChannels,AVSampleFormat RSC_InSampleFmt,AVSampleFormat RSC_OutSampleFmt,int RSC_InSampleRate,int RSC_OutSampleRate
                                         #if (defined(LIBAV)&&(LIBAVVERSIONINT>=9)) || defined(FFMPEG)
                                            ,uint64_t RSC_InChannelLayout,uint64_t RSC_OutChannelLayout
                                         #endif
                                   );
      virtual u_int8_t *Resample(AVFrame *Frame,int64_t *SizeDecoded,int DstSampleSize);

    //*********************
    // video filters part
    //*********************
    AVFilterGraph   *VideoFilterGraph;
    AVFilterContext *VideoFilterIn;
    AVFilterContext *VideoFilterOut;

    virtual int             VideoFilter_Open();
    virtual void            VideoFilter_Close();

      virtual bool hasChapters()  const { return NbrChapters > 0; }
      virtual int numChapters() const { return NbrChapters; }

   protected:
      AVFormatContext         *LibavAudioFile,*LibavVideoFile;    // LibAVFormat contexts
      AVCodec                 *VideoDecoderCodec;                 // Associated LibAVCodec for video stream
      AVCodec                 *AudioDecoderCodec;                 // Associated LibAVCodec for audio stream
      AVFrame                 *FrameBufferYUV;
      bool                    FrameBufferYUVReady;        // true if FrameBufferYUV is ready to convert
      int64_t                 FrameBufferYUVPosition;     // If FrameBufferYUV is ready to convert then keep FrameBufferYUV position
      QImage *yuv2rgbImage;

      // for yuv2rgb-conversion
      AVFrame *FrameBufferRGB;
      struct SwsContext *img_convert_ctx;

      // Audio resampling
      SwrContext              *RSC;
      uint64_t                RSC_InChannelLayout,RSC_OutChannelLayout;
      int                     RSC_InChannels,RSC_OutChannels;
      int                     RSC_InSampleRate,RSC_OutSampleRate;
      AVSampleFormat          RSC_InSampleFmt,RSC_OutSampleFmt;

   //private:
      struct sAudioContext {
         AVStream        *AudioStream;
         cSoundBlockList *SoundTrackBloc;
         int64_t         FPSSize;
         int64_t         FPSDuration;
         int64_t         DstSampleSize;
         bool            DontUseEndPos;
         double          *dEndFile;
         int             NbrDuration;
         double          TimeBase;
         bool            NeedResampling;
         int64_t         AudioLenDecoded;
         int             Counter;
         double          AudioFramePosition;
         double          Volume;
         bool            ContinueAudio;
      };
   //private slots:
      void DecodeAudio(sAudioContext *AudioContext,AVPacket *StreamPacket,int64_t Position);
      void DecodeAudioFrame(sAudioContext *AudioContext,qreal *FramePts,AVFrame *Frame,int64_t Position);
};

//*********************************************************************************************************************************************
// Music file
//*********************************************************************************************************************************************
class cMusicObject : public cVideoFile 
{
   public:
      double  Volume;                 // Volume as % from 10% to 150%
      bool    AllowCredit;            // if true, this music will appear in credit title
      int64_t ForceFadIn;             // Fad-IN duration on sound part (or 0 if none)
      int64_t ForceFadOut;            // Fad-OUT duration on sound part (or 0 if none)
      int64_t startOffset;
      enum MusicStartCode { eDefault_Start, eFrameRelated_Start, eDirectStart };
      MusicStartCode startCode;
      cSoundBlockList SoundTrackBloc;

      cMusicObject(cApplicationConfig *ApplicationConfig);

      virtual bool CheckFormatValide(QWidget *);
      void  SaveToXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
      virtual void SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel);
      bool  LoadFromXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag);
      QTime GetDuration();
      qreal GetFading(int64_t Position,bool SlideHaveFadIn,bool SlideHaveFadOut);
      virtual QImage *ReadFrame(bool PreviewMode,int64_t Position,bool DontUseEndPos,bool Deinterlace,cSoundBlockList *SoundTrackBloc,double Volume,bool ForceSoundOnly,int NbrDuration=2);
};

//class QMyImage : public QImage
//{
//   static int offset;
//   static bool offsetDetected;
//   static void detectOffset();
//   void setFormat(QImage::Format format) { *((int*)data_ptr()+offset) = format; }
//   public:
//   	QMyImage(const QImage & image) : QImage(image) {}
//      bool convert_ARGB_PM_to_ARGB_inplace();
//      bool convert_ARGB_to_ARGB_PM_inplace();
//      bool forcePremulFormat();
//};
#endif 
#endif // CBASEMEDIAFILE_H
