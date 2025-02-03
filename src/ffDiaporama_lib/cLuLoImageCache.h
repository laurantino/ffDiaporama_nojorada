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

#ifndef _cLuLoImageCACHE_H
#define _cLuLoImageCACHE_H

#include <QImage>
#include <QDateTime>
#include "cDatabase.h"

class QMyImage : public QImage
{
   static int offset;
   static bool offsetDetected;
   static void detectOffset();
   void setFormat(QImage::Format format) { *((int*)data_ptr() + offset) = format; }
   public:
   QMyImage(const QImage & image) : QImage(image) {}
   bool convert_ARGB_PM_to_ARGB_inplace();
   bool convert_ARGB_to_ARGB_PM_inplace();
   bool forcePremulFormat();
};

//===================================================
#define noCACHESTATS
class cLuLoImageCache;

class cLuLoImageCacheObject 
{
protected:
   bool blockingObject;
   cLuLoImageCacheObject() { blockingObject = false; }
   public:
      qlonglong       RessourceKey;
      qlonglong       FileKey;             // index of the file in the home user database
      void            *objKey;
      int             objSubKey;
      QDateTime       ModifDateTime;
      bool            Smoothing;           // Smoothing
      QImage          *CachePreviewImage;  // Cache image (Preview mode)
      QImage          *CacheRenderImage;   // Cache image (Full image mode)
      QString         FilterString;        // Filter string                    [For LULOOBJECT_IMAGE]
      int             ImageOrientation;    // Image orientation (EXIF)         [For LULOOBJECT_IMAGE]
      int64_t         Position;            // Position in video                [For LULOOBJECT_VIDEO]
      cLuLoImageCache *LuLoImageCache;     // Link to parent LuLoImageCache collection
      qlonglong       ByteCount;
      QTime lastAccess;

      // Constructor for image file
      cLuLoImageCacheObject(qlonglong RessourceKey,qlonglong FileKey,QDateTime ModifDateTime,int ImageOrientation,QString FilterString,bool Smoothing,cLuLoImageCache *Parent);
      cLuLoImageCacheObject(void *objKey, cLuLoImageCache *Parent);
      cLuLoImageCacheObject(void *objKey, int subKey, cLuLoImageCache *Parent);
      virtual ~cLuLoImageCacheObject();

      QImage *ValidateCachePreviewImage();

      QImage ValidateCacheRenderImageNC(qlonglong RessourceKey);   // ValidateCacheRenderImage without copy image
      QImage *ValidateCacheRenderImage();

      void setPreviewImage(const QImage &image);
      void setRenderImage(const QImage &image);
      void clearImages();
      #ifdef CACHESTATS
         long nValidatePreviewCalled;
         long nValidateRenderCalled;
         long nValidateRenderNCCalled;
      #endif
      bool isBlockingObject() { return blockingObject; }
      friend class cLuLoImageCache;
};

class cLuLoBlockCacheObject : public cLuLoImageCacheObject
{
public:
   cLuLoBlockCacheObject(void *ObjectKey, int ObjectSubKey, qlonglong byteCount, cLuLoImageCache *Parent)
   {
      blockingObject = true;
      RessourceKey = -1;
      FileKey=-1;             
      objKey = ObjectKey;
      objSubKey = ObjectSubKey;
      //QDateTime       ModifDateTime;
      //bool            Smoothing;           // Smoothing
      CachePreviewImage = nullptr;  // Cache image (Preview mode)
      CacheRenderImage = nullptr;   // Cache image (Full image mode)
      //QString         FilterString;        // Filter string                    [For LULOOBJECT_IMAGE]
      //int             ImageOrientation;    // Image orientation (EXIF)         [For LULOOBJECT_IMAGE]
      //int64_t         Position;            // Position in video                [For LULOOBJECT_VIDEO]
      LuLoImageCache = Parent;     // Link to parent LuLoImageCache collection
      ByteCount = byteCount;
      //QTime lastAccess;
   }
   virtual ~cLuLoBlockCacheObject()
   {
      ByteCount = 0;
   }
};

typedef QList<cLuLoImageCacheObject *> cacheList;
//===================================================

class cLuLoImageCache 
{
      cacheList cacheObjects;   // Fifo list
      cFilesTable *FilesTable;
      cSlideThumbsTable *ThumbsTable;
      int64_t MaxValue;       // Max memory used
      cLuLoImageCacheObject *findCacheObj(void *objKey, int subKey = -1);
      int findCacheObj(qlonglong RessourceKey, qlonglong FileKey, bool Smoothing);

   public:
      cLuLoImageCache();
      ~cLuLoImageCache();

      void setMaxValue(int64_t Value) { MaxValue = Value; FreeMemoryToMaxValue(0);  }
      int64_t getMaxValue() { return MaxValue; }
      cFilesTable *getFilesTable() { return FilesTable; }
      void setFilesTable(cFilesTable *t) { FilesTable = t; }
      cSlideThumbsTable *getThumbsTable() { return ThumbsTable; }
      void setThumbsTable(cSlideThumbsTable *t) { ThumbsTable = t; }

      void clear();
      void clearAllImages();
      cLuLoImageCacheObject *FindObject(qlonglong RessourceKey,qlonglong FileKey,QDateTime ModifDateTime,int ImageOrientation,bool Smoothing,bool SetAtTop);
      void FreeMemoryToMaxValue(cLuLoImageCacheObject *DontFree);
      int64_t MemoryUsed();
      int PercentageMemoryUsed();
      void RemoveImageObject(qlonglong RessourceKey,qlonglong FileKey);    // Special case for slide dialog : Remove all object of this key
      void RemoveOlder(QTime time);

      cLuLoImageCacheObject *FindObject(void *objKey);
      bool addPreviewImage(void *objKey, const QImage &image);
      bool addRenderImage(void *objKey, const QImage &image);
      QImage getPreviewImage(void *objKey);
      QImage getRenderImage(void *objKey);
      bool addImage(void *objKey, const QImage &image, bool bPreview) { if(bPreview) return addPreviewImage(objKey, image); return addRenderImage(objKey, image); }
      QImage getImage(void *objKey, bool bPreview) { if(bPreview) return getPreviewImage(objKey); return getRenderImage(objKey); }
      void RemoveImageObject(void *objKey);

      cLuLoImageCacheObject *FindObject(void *objKey, int subKey);
      bool addPreviewImage(void *objKey, int subKey, const QImage &image);
      bool addRenderImage(void *objKey, int subKey, const QImage &image);
      QImage getPreviewImage(void *objKey, int subKey);
      QImage getRenderImage(void *objKey, int subKey);
      bool addImage(void *objKey, int subKey, const QImage &image, bool bPreview) { if(bPreview) return addPreviewImage(objKey, subKey, image); return addRenderImage(objKey, subKey, image); }
      QImage getImage(void *objKey, int subKey, bool bPreview) { if(bPreview) return getPreviewImage(objKey, subKey); return getRenderImage(objKey, subKey); }
      void RemoveImageObject(void *objKey, int subKey);
      void RemoveObjectImages();

      void addBlockingItem(cLuLoBlockCacheObject *);

      void dumpCache(const QString &message);
};

#endif // _cLuLoImageCACHE_H
