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

#include "cLuLoImageCache.h"

// Include some common various class
#include "cBaseAppConfig.h"

// Include some additional standard class
#include <QFileInfo>
#include <QImageReader>
#include <smmintrin.h>

QMutex MemoryMutex;

QImage *samsungPanoramaFix(QString filename, QSize Size)
{
   QFile f(filename);
   f.open(QIODevice::ReadOnly);
   QByteArray ba = f.readAll();
   f.close();
   QImage *q = 0;
   //if (ba.indexOf(QString("SEFT"), ba.length() - 10) < 0)
   if (ba.indexOf(QByteArray("SEFT"), ba.length() - 10) < 0) // tocheck
         return q;

   int pos = 0;
   bool inData = false;
   while (pos < ba.length())
   {
      unsigned char c = ba[pos];
      if (c == 0xff)
      {
         pos++;
         c = ba[pos];
         //QString token = "unknown";

         // tokens without len
         if (inData)
         {
            if (c == 0x0)
               ;// token = "skip";
            else if (c < 0xd0 || c > 0xd7)
            {
               //token = "ill code ?";
               if (pos > 0)
                  ba.truncate(pos - 2);
               //ba.append(unsigned char(0xff));
               //ba.append(unsigned char(0xd9));
               ba.append(char (0xff));
               ba.append(char (0xd9));
               QBuffer buffer(&ba);
               buffer.open(QIODevice::ReadOnly);
               // read image from device
               QImageReader img2(&buffer);
               if( !Size.isNull())
                  img2.setScaledSize(Size);
               q = new QImage(img2.read().convertToFormat(QImage::Format_ARGB32_Premultiplied));
               return q;
            }
            else
               ;// token = "continue Data";
            if (c == 0xd9)
               inData = false;
         }
         switch (c)
         {
            //case 0xd8: token = "SOI"; break;
            //case 0xd9: token = "EOI"; break;
            case 0xdA: /*token = "SOS"; */inData = true; break;
         }
         switch (c)
         {
            //case 0xc0: token = "SOF0"; break;
            //case 0xc1: token = "SOF1"; break;
            //case 0xc2: token = "SOF2"; break;
            //case 0xc3: token = "SOF3"; break;
            //case 0xc4: token = "DHT"; break;
            //case 0xc5: token = "SOF5"; break;
            //case 0xc6: token = "SOF6"; break;
            //case 0xc7: token = "SOF7"; break;
            //case 0xc8: token = "JPG"; break;
            //case 0xc9: token = "SOF9"; break;
            //case 0xca: token = "SOF10"; break;
            //case 0xcb: token = "SOF11"; break;
            //case 0xcc: token = "DAC"; break;
            //case 0xcd: token = "SOF13"; break;
            //case 0xce: token = "SOF14"; break;
            //case 0xcf: token = "SOF15"; break;

            //case 0xdB: token = "DQT"; break;
            //case 0xdD: token = "DRI"; break;

            //case 0xe0: token = "APP0"; break;
            //case 0xe1: token = "APP1"; break;

            //case 0xfe: token = "COM"; break;
         }
         //if (token.compare("skip") != 0)
         //   qDebug() << "found ff at " << pos - 2 << " with " << c << " " << hex << c << " " << token << " inData " << inData;
      }
      pos++;
   }
   return q;
}

//*********************************************************************************************************************************************
// Base object for image cache manipulation
//*********************************************************************************************************************************************
// Constructor for image file

cLuLoImageCacheObject::cLuLoImageCacheObject(qlonglong TheRessourceKey,qlonglong TheFileKey,QDateTime TheModifDateTime,int TheImageOrientation,QString TheFilterString,bool TheSmoothing,cLuLoImageCache *Parent) 
{
   blockingObject = false;
   RessourceKey        = TheRessourceKey;
   FileKey             = TheFileKey;       // Full filename
   objKey              = NULL; 
   objSubKey           = -1;
   ModifDateTime       = TheModifDateTime;
   FilterString        = TheFilterString;
   Smoothing           = TheSmoothing;
   CacheRenderImage    = NULL;
   CachePreviewImage   = NULL;
   ImageOrientation    = TheImageOrientation;                 // Image orientation (EXIF)
   LuLoImageCache      = Parent;
   Position            = 0;
   ByteCount           = 0;
   #ifdef CACHESTATS
      nValidatePreviewCalled = 0;
      nValidateRenderCalled = 0;
      nValidateRenderNCCalled = 0;
   #endif
      lastAccess = QTime(23, 59);// QTime::currentTime();
}

cLuLoImageCacheObject::cLuLoImageCacheObject(void *_objKey, cLuLoImageCache *Parent)
{
   blockingObject = false;
   RessourceKey        = -1;
   FileKey             = -1;
   objKey              = _objKey; 
   objSubKey           = -1;
   //ModifDateTime       = TheModifDateTime;
   //FilterString        = TheFilterString;
   Smoothing           = false;
   CacheRenderImage    = NULL;
   CachePreviewImage   = NULL;
   ImageOrientation    = 0;               
   LuLoImageCache      = Parent;
   Position            = 0;
   ByteCount           = 0;
   #ifdef CACHESTATS
      nValidatePreviewCalled = 0;
      nValidateRenderCalled = 0;
      nValidateRenderNCCalled = 0;
   #endif
   lastAccess = QTime(23,59);//QTime::currentTime();
}

      
cLuLoImageCacheObject::cLuLoImageCacheObject(void *_objKey, int subKey, cLuLoImageCache *Parent)
{
   blockingObject = false;
   RessourceKey        = -1;
   FileKey             = -1;
   objKey              = _objKey; 
   objSubKey           = subKey;
   //ModifDateTime       = TheModifDateTime;
   //FilterString        = TheFilterString;
   Smoothing           = false;
   CacheRenderImage    = NULL;
   CachePreviewImage   = NULL;
   ImageOrientation    = 0;               
   LuLoImageCache      = Parent;
   Position            = 0;
   ByteCount           = 0;
   #ifdef CACHESTATS
      nValidatePreviewCalled = 0;
      nValidateRenderCalled = 0;
      nValidateRenderNCCalled = 0;
   #endif
   lastAccess = QTime::currentTime();
}


//===============================================================================

cLuLoImageCacheObject::~cLuLoImageCacheObject() 
{
   #ifdef CACHESTATS
      qDebug() << "delete imageCacheObject rkey " << RessourceKey << " fkey " << FileKey << " size " << ByteCount << "  nValidatePreviewCalled " << nValidatePreviewCalled << " nValidateRenderCalled " << nValidateRenderCalled << " nValidateRenderNCCalled " << nValidateRenderNCCalled-nValidateRenderCalled;
      //long nValidatePreviewCalled;
      //long nValidateRenderCalled;
      //long nValidateRenderNCCalled;
   #endif
   if (CachePreviewImage!=NULL) 
   {
      if (CachePreviewImage != CacheRenderImage) 
         delete CachePreviewImage;
      CachePreviewImage = NULL;
   }
   if (CacheRenderImage != NULL) 
   {
      delete CacheRenderImage;
      CacheRenderImage = NULL;
   }
}

//===============================================================================

QImage cLuLoImageCacheObject::ValidateCacheRenderImageNC(qlonglong RessourceKey) 
{
#ifdef CACHESTATS
   nValidateRenderNCCalled++;
#endif
   static bool cacheCleared = false;
   MemoryMutex.lock();
   LuLoImageCache->FreeMemoryToMaxValue(this);
   if (CacheRenderImage == NULL) 
   {
      if (RessourceKey != -1) 
      {
         QImage Image;
         ToLog(LOGMSG_INFORMATION,QApplication::translate("MainWindow","Loading file from ressource"));
         LuLoImageCache->getThumbsTable()->GetThumb(&RessourceKey,&Image);
         CacheRenderImage = new QImage(Image.convertToFormat(QImage::Format_ARGB32_Premultiplied));
         if (CacheRenderImage && CacheRenderImage->isNull()) 
         {
            ToLog(LOGMSG_CRITICAL,QApplication::translate("MainWindow","Error loading ressource"));
            delete CacheRenderImage;
            CacheRenderImage = NULL;
         }
      } 
      else 
      {
         QString FileName = LuLoImageCache->getFilesTable()->GetFileName(FileKey);

         // Load image from disk
         ToLog(LOGMSG_INFORMATION,QApplication::translate("MainWindow","Loading file :")+QFileInfo(FileName).fileName());
         //qDebug() << "cLuLoImageCacheObject::ValidateCacheRenderImageNC Loading file :" << FileName;
         QImageReader Img(FileName);
         QImage tmpImage = Img.read();
         if (FileName.endsWith(".jpg", Qt::CaseInsensitive))
            QMyImage(tmpImage).forcePremulFormat();
         else
            tmpImage = tmpImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
         CacheRenderImage = new QImage(tmpImage);
         //CacheRenderImage = new QImage(Img.read().convertToFormat(QImage::Format_ARGB32_Premultiplied));
         if (CacheRenderImage->isNull())
         {
            QSize s;
            CacheRenderImage = samsungPanoramaFix(FileName, s);
         }
         if (CacheRenderImage && CacheRenderImage->isNull())
         {
            ToLog(LOGMSG_CRITICAL,QApplication::translate("MainWindow","Error loading file :")+FileName);
            delete CacheRenderImage;
            CacheRenderImage = NULL;
         }
      }
      if (!CacheRenderImage)
      {
         if( cacheCleared )
            ToLog(LOGMSG_CRITICAL,QApplication::translate("MainWindow","Error allocating memory for render image"));
         if( !cacheCleared )
         {
            LuLoImageCache->clearAllImages();
            cacheCleared = true;
            MemoryMutex.unlock();
            return ValidateCacheRenderImageNC(RessourceKey); 
         }
      }

      // If image is ok then apply exif orientation (if needed)
#if QT_VERSION < 0x050400 || QT_VERSION > 0x050401
      if (CacheRenderImage) 
      {
         //*CacheRenderImage = CacheRenderImage->scaled(CacheRenderImage->width()*2, CacheRenderImage->height()*2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
         int rotation = 0;
         if (ImageOrientation == 8) rotation = -90;      // Rotating image anti-clockwise by 90 degrees...
         else if (ImageOrientation == 3) rotation = 180; // Rotating image clockwise by 180 degrees...
         else if (ImageOrientation == 6) rotation = 90;  // Rotating image clockwise by 90 degrees...
         if( rotation )
         {
            QTransform matrix;
            matrix.rotate(rotation);
            QImage *NewImage = new QImage(CacheRenderImage->transformed(matrix,Smoothing?Qt::SmoothTransformation:Qt::FastTransformation));
            if (NewImage) 
            {
               if (NewImage->isNull()) 
                  delete NewImage; 
               else 
               {
                  delete CacheRenderImage;
                  CacheRenderImage = NewImage;
               }
            }
         }
      }
#endif
      // If error
      if (CacheRenderImage && CacheRenderImage->isNull()) 
      {
         delete CacheRenderImage;
         CacheRenderImage = NULL;
      }

   }
   if (CacheRenderImage == NULL) 
      ToLog(LOGMSG_CRITICAL,"Error in cLuLoImageCacheObject::ValidateCacheRenderImage() : return NULL");
   else
      cacheCleared = false;
   ByteCount = (CacheRenderImage ? CacheRenderImage->QT_IMAGESIZE_IN_BYTES() : 0) + ((CachePreviewImage && CachePreviewImage != CacheRenderImage) ? CachePreviewImage->QT_IMAGESIZE_IN_BYTES() : 0);
   MemoryMutex.unlock();
   return (CacheRenderImage ? *CacheRenderImage : QImage());
}

QImage *cLuLoImageCacheObject::ValidateCacheRenderImage() 
{
   lastAccess = QTime::currentTime();
   #ifdef CACHESTATS
      nValidateRenderCalled++;
   #endif
   QImage Img = ValidateCacheRenderImageNC(RessourceKey);
   //return (!Img.isNull()) ? new QImage(Img) : NULL;
   return (!Img.isNull()) ? new QImage(Img/*.copy()*/) : NULL;
}

//===============================================================================

QImage *cLuLoImageCacheObject::ValidateCachePreviewImage() 
{
   lastAccess = QTime::currentTime();
#ifdef CACHESTATS
   nValidatePreviewCalled++;
#endif
   //QTime t;
   //t.start();
   //qDebug() << "FreeMemoryToMaxValue takes " << t.elapsed() << " mSec";
   MemoryMutex.lock();
   LuLoImageCache->FreeMemoryToMaxValue(this);
   if (CachePreviewImage == NULL) 
   {
      // if CacheRenderImage exist then copy it
      if ( CacheRenderImage  && !CacheRenderImage->isNull() ) 
      {
         if (CacheRenderImage->height() <= PREVIEWMAXHEIGHT) 
            CachePreviewImage = CacheRenderImage;
         else 
            CachePreviewImage = new QImage(CacheRenderImage->scaledToHeight(PREVIEWMAXHEIGHT,Smoothing?Qt::SmoothTransformation:Qt::FastTransformation).convertToFormat(QImage::Format_ARGB32_Premultiplied));
      } 
      else 
      {
         if (RessourceKey != -1) 
         {
            QImage Image;
            ToLog(LOGMSG_INFORMATION,QApplication::translate("MainWindow","Loading file from ressource"));
            LuLoImageCache->getThumbsTable()->GetThumb(&RessourceKey,&Image);

            // then copy it at correct size into CachePreviewImage
            if (Image.height() <= PREVIEWMAXHEIGHT) 
               CachePreviewImage = new QImage(Image.convertToFormat(QImage::Format_ARGB32_Premultiplied));
            else 
               CachePreviewImage = new QImage(Image.scaledToHeight(PREVIEWMAXHEIGHT,Smoothing?Qt::SmoothTransformation:Qt::FastTransformation).convertToFormat(QImage::Format_ARGB32_Premultiplied));
         } 
         else 
         {
            QString FileName = LuLoImageCache->getFilesTable()->GetFileName(FileKey);
            ToLog(LOGMSG_INFORMATION,QApplication::translate("MainWindow","Loading file :")+QFileInfo(FileName).fileName());
            // if no CacheRenderImage then load image directly at correct size
            QImageReader Img(FileName);
            if (Img.canRead()) 
            {
               QSize Size = Img.size();
               if (((ImageOrientation == 8) || (ImageOrientation == 6)) && (Size.width() > PREVIEWMAXHEIGHT)) 
               {
                  Size.setHeight((qreal(Size.height())/qreal(Size.width()))*PREVIEWMAXHEIGHT);
                  Size.setWidth(PREVIEWMAXHEIGHT);
                  Img.setScaledSize(Size);
               } 
               else if ((ImageOrientation != 8) && (ImageOrientation!=6) && (Size.height() > PREVIEWMAXHEIGHT)) 
               {
                  Size.setWidth((qreal(Size.width())/qreal(Size.height()))*PREVIEWMAXHEIGHT);
                  Size.setHeight(PREVIEWMAXHEIGHT);
                  Img.setScaledSize(Size);
               }
               CachePreviewImage = new QImage(Img.read().convertToFormat(QImage::Format_ARGB32_Premultiplied));
               //qDebug() << "Img supports Animation =" << Img.supportsAnimation() << " " << " number of Images " << Img.imageCount();
               if(CachePreviewImage->isNull())
               { 
                  CachePreviewImage = samsungPanoramaFix(FileName, Size);
               }
            }
            if ( CachePreviewImage && CachePreviewImage->isNull()) 
            {
               ToLog(LOGMSG_CRITICAL,QApplication::translate("MainWindow","Error loading file :")+FileName);
               ToLog(LOGMSG_CRITICAL, QApplication::translate("MainWindow", "Image loading Error:") + Img.errorString());
               //qDebug() << Img.errorString();
               delete CachePreviewImage;
               CachePreviewImage = NULL;
            }

#if QT_VERSION < 0x050400 || QT_VERSION > 0x050401
            if (CachePreviewImage) 
            {
               int rotation = 0;
               if (ImageOrientation == 8) rotation = -90;      // Rotating image anti-clockwise by 90 degrees...
               else if (ImageOrientation == 3) rotation = 180; // Rotating image clockwise by 180 degrees...
               else if (ImageOrientation == 6) rotation = 90;  // Rotating image clockwise by 90 degrees...
               if( rotation )
               {
                  QTransform matrix;
                  matrix.rotate(rotation);
                  QImage *NewImage = new QImage(CachePreviewImage->transformed(matrix,Smoothing?Qt::SmoothTransformation:Qt::FastTransformation));
                  if (NewImage) 
                  {
                     if (NewImage->isNull()) 
                        delete NewImage; 
                     else 
                     {
                        delete CachePreviewImage;
                        CachePreviewImage = NewImage;
                     }
                  }
               }
            }
#endif
         }
      }

      // If error
      if ( CachePreviewImage && CachePreviewImage->isNull() ) 
      {
         delete CachePreviewImage;
         CachePreviewImage = NULL;
      }
   }
   if (CachePreviewImage == NULL) 
      ToLog(LOGMSG_CRITICAL,"Error in cLuLoImageCacheObject::CachePreviewImage() : return NULL");
   ByteCount = ((CacheRenderImage) ? CacheRenderImage->QT_IMAGESIZE_IN_BYTES() : 0) + (((CachePreviewImage) && (CachePreviewImage != CacheRenderImage)) ? CachePreviewImage->QT_IMAGESIZE_IN_BYTES() : 0);
   QImage *Ret = (CachePreviewImage) ? new QImage(*CachePreviewImage/*->copy()*/) : NULL;
   MemoryMutex.unlock();
   return Ret;
}

void cLuLoImageCacheObject::setPreviewImage(const QImage &image)
{
   if( CachePreviewImage != NULL )
      delete CachePreviewImage;
   CachePreviewImage = new QImage( image );
   ByteCount = ((CacheRenderImage) ? CacheRenderImage->QT_IMAGESIZE_IN_BYTES() : 0) + (((CachePreviewImage) && (CachePreviewImage != CacheRenderImage)) ? CachePreviewImage->QT_IMAGESIZE_IN_BYTES() : 0);
}

void cLuLoImageCacheObject::setRenderImage(const QImage &image)
{
   if( CacheRenderImage != NULL )
      delete CacheRenderImage;
   CacheRenderImage = new QImage( image );
   ByteCount = ((CacheRenderImage) ? CacheRenderImage->QT_IMAGESIZE_IN_BYTES() : 0) + (((CachePreviewImage) && (CachePreviewImage != CacheRenderImage)) ? CachePreviewImage->QT_IMAGESIZE_IN_BYTES() : 0);
}

void cLuLoImageCacheObject::clearImages()
{
   if( CacheRenderImage ) 
      delete CacheRenderImage;
   CacheRenderImage = NULL;
   if( CachePreviewImage && CachePreviewImage != CacheRenderImage )
      delete CachePreviewImage;
   CachePreviewImage = NULL;
   ByteCount = 0;
}
//*********************************************************************************************************************************************
// List object for image cache manipulation
//*********************************************************************************************************************************************

cLuLoImageCache::cLuLoImageCache() 
{
   MaxValue    = 1024*1024*1024;
   FilesTable  = NULL;
   ThumbsTable = NULL;
}

//===============================================================================

cLuLoImageCache::~cLuLoImageCache() 
{
   clear();
}

//===============================================================================

void cLuLoImageCache::clear() 
{
   while (cacheObjects.count() > 0) 
      delete cacheObjects.takeLast();
}

void cLuLoImageCache::clearAllImages() 
{
   //dumpCache("before clearAllImages");
   cacheList::iterator it = cacheObjects.begin();
   while( it != cacheObjects.end() )
   {
      if( (*it)->objKey && !(*it)->isBlockingObject() )
      {
         delete (*it);
         it = cacheObjects.erase(it);
      }
      else
      {
         (*it)->clearImages();
         it++;
      }
   }
   //foreach(cLuLoImageCacheObject * co, cacheObjects) 
   //   co->clearImages();
}

//===============================================================================
// Find object corresponding to FileName - if object not found then create one
//===============================================================================

// Image version
cLuLoImageCacheObject *cLuLoImageCache::FindObject(qlonglong RessourceKey,qlonglong FileKey,QDateTime ModifDateTime,int ImageOrientation,bool Smoothing,bool SetAtTop) 
{
   int i = 0;
   QMutexLocker locker(&MemoryMutex);

   i = findCacheObj(RessourceKey, FileKey, Smoothing );
   if ( i >= 0 )
   { 
      if (SetAtTop && i > 0) 
      { // If item is not the first
         cacheObjects.move(i,0);
         i = 0;
      }
      return cacheObjects[i]; 
   }
   cacheObjects.prepend(new cLuLoImageCacheObject(RessourceKey,FileKey,ModifDateTime,ImageOrientation,"",Smoothing,this));     // Append a new object at first position
   return cacheObjects.first();
}

cLuLoImageCacheObject *cLuLoImageCache::FindObject(void *objKey)
{
   cacheList::iterator it = cacheObjects.begin();
   cLuLoImageCacheObject *cacheObject = NULL;
   while( it != cacheObjects.end() && cacheObject == NULL )
   {
      if( (*it)->objKey == objKey ) 
         cacheObject = *it;
      it++;
   }
   return cacheObject;
}

cLuLoImageCacheObject *cLuLoImageCache::FindObject(void *objKey, int subKey)
{
   cacheList::iterator it = cacheObjects.begin();
   cLuLoImageCacheObject *cacheObject = NULL;
   while( it != cacheObjects.end() && cacheObject == NULL )
   {
      if( (*it)->objKey == objKey && (*it)->objSubKey == subKey ) 
         cacheObject = *it;
      it++;
   }
   return cacheObject;
}

//===============================================================================
// Special case for Image object : Remove all image object of this key
void cLuLoImageCache::RemoveImageObject(qlonglong RessourceKey,qlonglong FileKey) 
{
   MemoryMutex.lock();
   int i = cacheObjects.count()-1;
   while (i >= 0) 
   {
      if (RessourceKey != -1 && cacheObjects[i]->RessourceKey == RessourceKey) 
         delete cacheObjects.takeAt(i);
      if (RessourceKey == -1 && cacheObjects[i]->FileKey == FileKey)
         delete cacheObjects.takeAt(i);
      i--;
   }
   MemoryMutex.unlock();
}

void cLuLoImageCache::RemoveImageObject(void *objKey)
{
   MemoryMutex.lock();
   int i = cacheObjects.count()-1;
   while (i >= 0) 
   {
      if ( cacheObjects[i]->objKey == objKey ) 
         delete cacheObjects.takeAt(i);
      i--;
   }
   MemoryMutex.unlock();
}

void cLuLoImageCache::RemoveImageObject(void *objKey, int subKey)
{
   MemoryMutex.lock();
   int i = cacheObjects.count()-1;
   while (i >= 0) 
   {
      if ( cacheObjects[i]->objKey == objKey && cacheObjects[i]->objSubKey == subKey) 
         delete cacheObjects.takeAt(i);
      i--;
   }
   MemoryMutex.unlock();
}

void cLuLoImageCache::RemoveObjectImages()
{
   MemoryMutex.lock();
   int i = cacheObjects.count()-1;
   while (i >= 0) 
   {
      if ( cacheObjects[i]->objKey != 0 && !cacheObjects[i]->isBlockingObject())
         delete cacheObjects.takeAt(i);
      i--;
   }
   MemoryMutex.unlock();
}

void cLuLoImageCache::RemoveOlder(QTime time)
{
   MemoryMutex.lock();
   int i = cacheObjects.count()-1;
   //qDebug() << "RemoveOlder than " << time.toString("hh:mm") << " cacheObjects: " << i;
   while (i >= 0)
   {
      if ( cacheObjects[i]->lastAccess <= time && !cacheObjects[i]->isBlockingObject())
      {
         cLuLoImageCacheObject *toDel = cacheObjects.takeAt(i);
         //qDebug() << "remove Older freeing " << toDel->ByteCount << " bytes";
         delete toDel;
      }
      i--;
   }
   //qDebug() << " cacheObjects: " << cacheObjects.count();
   MemoryMutex.unlock();
}

//===============================================================================

int64_t cLuLoImageCache::MemoryUsed() 
{
   int64_t MemUsed = 0;
   for (auto cacheObject : cacheObjects)
      MemUsed += cacheObject->ByteCount;
   return MemUsed;
}

int cLuLoImageCache::PercentageMemoryUsed()
{
   int64_t used = MemoryUsed();
   return used*100LL/MaxValue;
}
//===============================================================================

void cLuLoImageCache::FreeMemoryToMaxValue(cLuLoImageCacheObject *DontFree) 
{
   // 1st step : ensure used memory is less than max allowed
   int64_t Memory    = MemoryUsed();
   int64_t MaxMemory = MaxValue;
   int dontDeleteCount = 1;
   //qDebug() << "LuLoImageCache #objects in cache" <<  cacheObjects.count() << " bytes used " << Memory << " of max " << MaxValue;
   if (Memory > MaxMemory) 
   {
      if (DontFree) 
      {
         // Ensure DontFree object is at top of list
         int index = cacheObjects.indexOf(DontFree);
         if( index > 0 )
            cacheObjects.move(index, 0);
      }
      bool moved = true;
      while (moved)
      {
         moved = false;
         for (int i = dontDeleteCount; i < cacheObjects.count(); i++)
         {
            if (cacheObjects.at(i)->isBlockingObject())
            {
               if (i > dontDeleteCount)
               {
                  cacheObjects.move(i, dontDeleteCount);
                  dontDeleteCount++;
                  moved = true;
                  break;
               }
            }
         }
      }
      //MemoryMutex.lock();
      if (Memory > MaxMemory) 
      {
         QString DisplayLog=QString("Free memory for max value (%1 Mb) : Before=%2 cached objects for %3 Mb").arg(MaxMemory/(1024*1024)).arg(cacheObjects.count()).arg(MaxMemory/(1024*1024));
         while (((Memory = MemoryUsed()) > MaxMemory) && (cacheObjects.count() > dontDeleteCount))
            delete cacheObjects.takeLast();    // Never delete first object
         ToLog(LOGMSG_INFORMATION,DisplayLog+QString(" - After=%1 cached objects for %2 Mb").arg(cacheObjects.count()).arg(Memory/(1024*1024)));
      }
      // 2st step : ensure we are able to allocate a 128 Mb block
      void *block = NULL;
      while (block == NULL && cacheObjects.count() > dontDeleteCount)
      {
         block = malloc(128*1024*1024);
         if (block == NULL && cacheObjects.count() > dontDeleteCount)  // Never delete first object
         {  
            delete cacheObjects.takeLast();
            ToLog(LOGMSG_INFORMATION,QString("Free memory to ensure enough space to work - After=%1 cached objects for %2 Mb").arg(cacheObjects.count()).arg(Memory/(1024*1024)));
         }
      }
      if (block) 
         free(block);
      //MemoryMutex.unlock();
   }
}

cLuLoImageCacheObject *cLuLoImageCache::findCacheObj(void *objKey, int subKey)
{
   cacheList::ConstIterator it = cacheObjects.cbegin();
   while( it != cacheObjects.cend()  )
   {
      if( (*it)->objKey == objKey && (subKey == -1 || (*it)->objSubKey == subKey) ) 
         return *it;
      it++;
   }
   return NULL;
}

int cLuLoImageCache::findCacheObj(qlonglong RessourceKey, qlonglong FileKey, bool Smoothing)
{
   int iRet = -1;
   cacheList::ConstIterator it = cacheObjects.cbegin();
   while( it != cacheObjects.cend() )
   {
      iRet++;
      if( ((RessourceKey != -1 && (*it)->RessourceKey == RessourceKey) || (FileKey != -1 && (*it)->FileKey == FileKey)) && (*it)->Smoothing == Smoothing )
         return iRet;
      it++;
   }
   return -1;
}

bool cLuLoImageCache::addPreviewImage(void *objKey, const QImage &image)
{
   if( MemoryUsed() + image.QT_IMAGESIZE_IN_BYTES() > MaxValue )
      return false;
   MemoryMutex.lock();
   cLuLoImageCacheObject *cacheObject = findCacheObj(objKey);
   if( cacheObject == NULL )
   {
      cacheObject = new cLuLoImageCacheObject(objKey,this);
      cacheObjects.append(cacheObject);
   }
   cacheObject->setPreviewImage(image);
   MemoryMutex.unlock();
   return true;
}

bool cLuLoImageCache::addRenderImage(void *objKey, const QImage &image)
{
   int64_t memused = MemoryUsed();
   if( memused + image.QT_IMAGESIZE_IN_BYTES() > MaxValue )
   {
      //qDebug() << "addRenderImage not possible, cache is full (" << memused << "/" << MaxValue << ")";
      //dumpCache("cache full");
      return false;
   }
   MemoryMutex.lock();
   cLuLoImageCacheObject *cacheObject = findCacheObj(objKey);
   if( cacheObject == NULL )
   {
      cacheObject = new cLuLoImageCacheObject(objKey,this);
      //MemoryMutex.lock();
      cacheObjects.append(cacheObject);
      //MemoryMutex.unlock();
   }
   cacheObject->setRenderImage(image);
   MemoryMutex.unlock();
   return true;
}

QImage cLuLoImageCache::getPreviewImage(void *objKey)
{
   cLuLoImageCacheObject *cacheObject = findCacheObj(objKey);
   if( cacheObject == NULL || cacheObject->CachePreviewImage == NULL )
      return QImage();
   cacheObject->lastAccess = QTime::currentTime();
   return QImage(*cacheObject->CachePreviewImage);
}

QImage cLuLoImageCache::getRenderImage(void *objKey)
{
   cLuLoImageCacheObject *cacheObject = findCacheObj(objKey);
   if( cacheObject == NULL || cacheObject->CacheRenderImage == NULL )
      return QImage();
   cacheObject->lastAccess = QTime::currentTime();
   return QImage(*cacheObject->CacheRenderImage);
}

bool cLuLoImageCache::addPreviewImage(void *objKey, int subKey, const QImage &image)
{
   if( MemoryUsed() + image.QT_IMAGESIZE_IN_BYTES() > MaxValue )
      return false;
   cLuLoImageCacheObject *cacheObject = findCacheObj(objKey, subKey);
   if( cacheObject == NULL )
   {
      cacheObject = new cLuLoImageCacheObject(objKey,subKey,this);
      MemoryMutex.lock();
      cacheObjects.append(cacheObject);
      MemoryMutex.unlock();
   }
   cacheObject->setPreviewImage(image);
   return true;
}

bool cLuLoImageCache::addRenderImage(void *objKey, int subKey, const QImage &image)
{
   if( MemoryUsed() + image.QT_IMAGESIZE_IN_BYTES() > MaxValue )
      return false;
   MemoryMutex.lock();
   cLuLoImageCacheObject *cacheObject = findCacheObj(objKey, subKey);
   if( cacheObject == NULL )
   {
      cacheObject = new cLuLoImageCacheObject(objKey,subKey,this);
      //MemoryMutex.lock();
      cacheObjects.append(cacheObject);
      //MemoryMutex.unlock();
   }
   cacheObject->setRenderImage(image);
   MemoryMutex.unlock();
   return true;
}

QImage cLuLoImageCache::getPreviewImage(void *objKey, int subKey)
{
   cLuLoImageCacheObject *cacheObject = findCacheObj(objKey, subKey);
   if( cacheObject == NULL || cacheObject->CachePreviewImage == NULL )
      return QImage();
   cacheObject->lastAccess = QTime::currentTime();
   return QImage(*cacheObject->CachePreviewImage);
}

QImage cLuLoImageCache::getRenderImage(void *objKey, int subKey)
{
   cLuLoImageCacheObject *cacheObject = findCacheObj(objKey, subKey);
   if( cacheObject == NULL || cacheObject->CacheRenderImage == NULL )
      return QImage();
   cacheObject->lastAccess = QTime::currentTime();
   return QImage(*cacheObject->CacheRenderImage);
}
void cLuLoImageCache::addBlockingItem(cLuLoBlockCacheObject *cacheObject)
{
   cacheObjects.append(cacheObject);
}

void cLuLoImageCache::dumpCache(const QString &message)
{
   qDebug() << "dumpCache " << message;
   foreach(cLuLoImageCacheObject* co, cacheObjects)
   {
      //qDebug() << (long)((void *)co) << co->FileKey << " " << co->RessourceKey << " " << co->objKey << " " << co->objSubKey << " " << co->CachePreviewImage << " " << co->CacheRenderImage << " " << co->Smoothing << " " << co->lastAccess;
   }
}




int QMyImage::offset = -1;
bool QMyImage::offsetDetected = false;

void QMyImage::detectOffset()
{
   if (offsetDetected)
      return;
   QImage testImg(1, 1, QImage::Format_RGB32);
   int *data = (int*)testImg.data_ptr();
   int datasize = 50;
   QList<int> firstRun;
   QList<int> secondRun;
   QList<int> thirdRun;
   for (int i = 0; i < datasize; i++)
   {
      if (*data == QImage::Format_RGB32)
         firstRun.append(i);
      data++;
   }
   testImg = testImg.convertToFormat(QImage::Format_ARGB32);
   data = (int*)testImg.data_ptr();
   for (int i = 0; i < datasize; i++)
   {
      if (*data == QImage::Format_ARGB32)
      {
         if (firstRun.contains(i))
            secondRun.append(i);
      }
      data++;
   }

   testImg = testImg.convertToFormat(QImage::Format_ARGB32_Premultiplied);
   data = (int*)testImg.data_ptr();
   for (int i = 0; i < datasize; i++)
   {
      if (*data == QImage::Format_ARGB32_Premultiplied)
      {
         if (secondRun.contains(i))
            thirdRun.append(i);
      }
      data++;
   }
   if (thirdRun.count() == 1)
      offset = thirdRun.first();
   offsetDetected = true;
}

#ifdef QT_COMPILER_SUPPORTS_SSE4_1
#define BYTE_MUL_SSE2(result, pixelVector, alphaChannel, colorMask, half) \
{ \
    /* 1. separate the colors in 2 vectors so each color is on 16 bits \
           (in order to be multiplied by the alpha \
                  each 32 bit of dstVectorAG are in the form 0x00AA00GG \
                         each 32 bit of dstVectorRB are in the form 0x00RR00BB */\
    __m128i pixelVectorAG = _mm_srli_epi16(pixelVector, 8); \
    __m128i pixelVectorRB = _mm_and_si128(pixelVector, colorMask); \
 \
    /* 2. multiply the vectors by the alpha channel */\
    pixelVectorAG = _mm_mullo_epi16(pixelVectorAG, alphaChannel); \
    pixelVectorRB = _mm_mullo_epi16(pixelVectorRB, alphaChannel); \
 \
    /* 3. divide by 255, that's the tricky part. \
           we do it like for BYTE_MUL(), with bit shift: X/255 ~= (X + X/256 + rounding)/256 */ \
    /** so first (X + X/256 + rounding) */\
    pixelVectorRB = _mm_add_epi16(pixelVectorRB, _mm_srli_epi16(pixelVectorRB, 8)); \
    pixelVectorRB = _mm_add_epi16(pixelVectorRB, half); \
    pixelVectorAG = _mm_add_epi16(pixelVectorAG, _mm_srli_epi16(pixelVectorAG, 8)); \
    pixelVectorAG = _mm_add_epi16(pixelVectorAG, half); \
 \
    /** second divide by 256 */\
    pixelVectorRB = _mm_srli_epi16(pixelVectorRB, 8); \
    /** for AG, we could >> 8 to divide followed by << 8 to put the \
            bytes in the correct position. By masking instead, we execute \
                    only one instruction */\
    pixelVectorAG = _mm_andnot_si128(colorMask, pixelVectorAG); \
 \
    /* 4. combine the 2 pairs of colors */ \
    result = _mm_or_si128(pixelVectorAG, pixelVectorRB); \
} 
#endif

bool QMyImage::forcePremulFormat()
{
   if (isNull() || format() == QImage::Format_ARGB32_Premultiplied)
      return true;
   if (format() != QImage::Format_ARGB32)
      return false;
   if (!offsetDetected)
      detectOffset();
   if (offset < 0)
      return false;
   setFormat(QImage::Format_ARGB32_Premultiplied);
   return true;
}

//  x/255 = (x*257+257)>>16
bool QMyImage::convert_ARGB_to_ARGB_PM_inplace()
{
   if (isNull() || format() == QImage::Format_ARGB32_Premultiplied)
      return true;
   if (format() != QImage::Format_ARGB32)
      return false;
   if (!offsetDetected)
      detectOffset();
   if (offset < 0)
      return false;

   #ifndef QT_COMPILER_SUPPORTS_SSE4_1
   uint *buffer = reinterpret_cast<uint*>(const_cast<uchar *>(((const QMyImage*)this)->bits()));
   int count = byteCount() / 4;
   for (int i = 0; i < count; ++i)
   {
      *buffer = qPremultiply(*buffer);
      buffer++;
   }
   #else
   // extra pixels on each line
   const int spare = width() & 3;
   // width in pixels of the pad at the end of each line
   const int pad = (bytesPerLine() >> 2) - width();
   const int iter = width() >> 2;
   int _height = height();

   const __m128i alphaMask = _mm_set1_epi32(0xff000000);
   const __m128i nullVector = _mm_setzero_si128();
   const __m128i half = _mm_set1_epi16(0x80);
   const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);

   __m128i *d = reinterpret_cast<__m128i*>(const_cast<uchar *>(((const QMyImage*)this)->bits()));
   while (_height--)
   {
      const __m128i *end = d + iter;

      for (; d != end; ++d)
      {
         const __m128i srcVector = _mm_loadu_si128(d);
         const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask);
         if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff)
         {
            // opaque, data is unchanged
         }
         else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) == 0xffff)
         {
            // fully transparent
            _mm_storeu_si128(d, nullVector);
         }
         else
         {
            __m128i alphaChannel = _mm_srli_epi32(srcVector, 24);
            alphaChannel = _mm_or_si128(alphaChannel, _mm_slli_epi32(alphaChannel, 16));

            __m128i result;
            BYTE_MUL_SSE2(result, srcVector, alphaChannel, colorMask, half);
            result = _mm_or_si128(_mm_andnot_si128(alphaMask, result), srcVectorAlpha);
            _mm_storeu_si128(d, result);
         }
      }

      QRgb *p = reinterpret_cast<QRgb*>(d);
      QRgb *pe = p + spare;
      for (; p != pe; ++p)
      {
         if (*p < 0x00ffffff)
            *p = 0;
         else if (*p < 0xff000000)
            *p = qPremultiply(*p);
      }

      d = reinterpret_cast<__m128i*>(p + pad);
   }
   #endif
   setFormat(QImage::Format_ARGB32_Premultiplied);
   return true;
}

inline QRgb qUnpremultiply_sse4(QRgb p)
{
   const uint alpha = qAlpha(p);
   if (alpha == 255 || alpha == 0)
      return p;
   const uint invAlpha = qt_inv_premul_factor[alpha];
   const __m128i via = _mm_set1_epi32(invAlpha);
   const __m128i vr = _mm_set1_epi32(0x8000);
   __m128i vl = _mm_cvtepu8_epi32(_mm_cvtsi32_si128(p));
   vl = _mm_mullo_epi32(vl, via);
   vl = _mm_add_epi32(vl, vr);
   vl = _mm_srai_epi32(vl, 16);
   vl = _mm_insert_epi32(vl, alpha, 3);
   vl = _mm_packus_epi32(vl, vl);
   vl = _mm_packus_epi16(vl, vl);
   return _mm_cvtsi128_si32(vl);
}

bool QMyImage::convert_ARGB_PM_to_ARGB_inplace()
{
   if (isNull() || format() == QImage::Format_ARGB32)
      return true;
   if (!offsetDetected)
      detectOffset();
   if (format() != QImage::Format_ARGB32_Premultiplied || offset < 0)
      return false;
   uint *buffer = reinterpret_cast<uint*>(const_cast<uchar *>(((const QMyImage*)this)->bits()));
   int count = QT_IMAGESIZE_IN_BYTES() / 4;
   for (int i = 0; i < count; ++i)
   {
      //*buffer = qUnpremultiply(*buffer++);
      //*buffer = qUnpremultiply_sse4(*buffer++);
      QRgb p = *buffer;
      const uint alpha = qAlpha(p);
      // Alpha 255 and 0 are the two most common values, which makes them beneficial to short-cut.
      if (alpha == 0)
         *buffer = 0;
      if (alpha != 255)
      {
         // (p*(0x00ff00ff/alpha)) >> 16 == (p*255)/alpha for all p and alpha <= 256.
         const uint invAlpha = qt_inv_premul_factor[alpha];
         // We add 0x8000 to get even rounding. The rounding also ensures that qPremultiply(qUnpremultiply(p)) == p for all p.
         *buffer = qRgba((qRed(p)*invAlpha + 0x8000) >> 16, (qGreen(p)*invAlpha + 0x8000) >> 16, (qBlue(p)*invAlpha + 0x8000) >> 16, alpha);
      }
   }
   setFormat(QImage::Format_ARGB32);
   return true;
}

