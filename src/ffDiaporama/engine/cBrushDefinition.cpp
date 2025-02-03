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

#include "cBrushDefinition.h"
#include <QtSvg>
#include <engine/cLocation.h>

#include "ImageFilters.h"
#include "_Variables.h"
#include "jhlabs_filter/image/LightFilter.h"
#ifdef WITH_OPENCV
#include "ocv.h"
#endif
//#include "qt_cuda.h"

#define noLOG_TIMING

#define DEFAULT_IMAGEROTATION   0
#define DEFAULT_BRIGHTNESS      0
#define DEFAULT_CONTRAST        0
#define DEFAULT_GAMMA           1
#define DEFAULT_RED             0
#define DEFAULT_GREEN           0
#define DEFAULT_BLUE            0
#define DEFAULT_LOCKGEOMETRY    false
#define DEFAULT_ASPECTRATIO     1
#define DEFAULT_FULLFILLING     false
#define DEFAULT_TYPEBLURSHARPEN 0
#define DEFAULT_GBSSIGMA        0
#define DEFAULT_GBSRADIUS       5
#define DEFAULT_QBSSIGMA        0
#define DEFAULT_DESAT           0
#define DEFAULT_SWIRL           0
#define DEFAULT_IMPLODE         0
#define DEFAULT_ONOFFFILTER     0

//============================================
// Global static
//============================================

cBackgroundList BackgroundList;

#define PI 3.14159265

#define USEBRUSHCACHE

// cache-subkeys
#define SUBKEY_STD      -1
#define SUBKEY_PUSHBACK -2
#define SUBKEY_SLIDE    -3
//*********************************************************************************************************************************************

cAutoFramingDef AUTOFRAMINGDEF[NBR_AUTOFRAMING];

cAutoFramingDef::cAutoFramingDef(QString ToolTip,bool IsInternal,int GeometryType) 
{
   this->ToolTip       = ToolTip;
   this->GeometryType  = GeometryType;
   this->IsInternal    = IsInternal;
}

void AutoFramingDefInit() 
{
   for (int i = 0; i < NBR_AUTOFRAMING; i++) 
      switch(i) 
      {
         case AUTOFRAMING_CUSTOMUNLOCK   : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Custom geometry - unlocked"),false,         AUTOFRAMING_GEOMETRY_CUSTOM);   break;
         case AUTOFRAMING_CUSTOMLOCK     : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Custom geometry - locked"),false,           AUTOFRAMING_GEOMETRY_CUSTOM);   break;
         case AUTOFRAMING_CUSTOMIMGLOCK  : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Custom size - Image geometry"),false,       AUTOFRAMING_GEOMETRY_IMAGE);    break;
         case AUTOFRAMING_CUSTOMPRJLOCK  : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Custom size - Project geometry"),false,     AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_FULLMAX        : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Full image"),false,                         AUTOFRAMING_GEOMETRY_IMAGE);    break;
         case AUTOFRAMING_FULLMIN        : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Full inner image"),true,                    AUTOFRAMING_GEOMETRY_IMAGE);    break;
         case AUTOFRAMING_HEIGHTLEFTMAX  : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project height - to the left"),false,       AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_HEIGHTLEFTMIN  : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project inner height - to the left"),true,  AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_HEIGHTMIDLEMAX : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project height - in the center"),false,     AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_HEIGHTMIDLEMIN : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project inner height - in the center"),true,AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_HEIGHTRIGHTMAX : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project height - to the right"),false,      AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_HEIGHTRIGHTMIN : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project inner height - to the right"),true, AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_WIDTHTOPMAX    : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project width - at the top"),false,         AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_WIDTHTOPMIN    : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project inner width - at the top"),true,    AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_WIDTHMIDLEMAX  : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project width - in the middle"),false,      AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_WIDTHMIDLEMIN  : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project inner width - in the middle"),true, AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_WIDTHBOTTOMMAX : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project width - at the bottom"),false,      AUTOFRAMING_GEOMETRY_PROJECT);  break;
         case AUTOFRAMING_WIDTHBOTTOMMIN : AUTOFRAMINGDEF[i] = cAutoFramingDef(QApplication::translate("Framing styles","Project inner width - at the bottom"),true, AUTOFRAMING_GEOMETRY_PROJECT);  break;
      }
}

//*********************************************************************************************************************************************
// Object for Brush definition
//*********************************************************************************************************************************************

cBrushDefinition::cBrushDefinition(cCompositionObject *TheCompositionObject, cApplicationConfig *TheApplicationConfig) 
{
   TypeComposition   = eCOMPOSITIONTYPE_BACKGROUND;
   ApplicationConfig = TheApplicationConfig;
   CompositionObject = TheCompositionObject;
   MediaObject       = NULL;
   DeleteMediaObject = true;
   pushedImage = false;
   slidingBrush = false;

   InitDefaultValues();
}

//====================================================================================================================
cBrushDefinition::brushParams::brushParams()
{
}

void cBrushDefinition::brushParams::InitDefaultValues()
{
   ImageRotation           = DEFAULT_IMAGEROTATION;         // Image rotation
   X                       = 0;                             // X position (in %) relative to up/left corner
   Y                       = 0;                             // Y position (in %) relative to up/left corner
   ZoomFactor              = 1;                             // Zoom factor (in %)
   Brightness              = DEFAULT_BRIGHTNESS;
   Contrast                = DEFAULT_CONTRAST;
   Gamma                   = DEFAULT_GAMMA;
   Red                     = DEFAULT_RED;
   Green                   = DEFAULT_GREEN;
   Blue                    = DEFAULT_BLUE;
   LockGeometry            = DEFAULT_LOCKGEOMETRY;
   AspectRatio             = DEFAULT_ASPECTRATIO;
   FullFilling             = DEFAULT_FULLFILLING;
   GaussBlurSharpenSigma   = DEFAULT_GBSSIGMA;
   BlurSharpenRadius       = DEFAULT_GBSRADIUS;
   QuickBlurSharpenSigma   = DEFAULT_QBSSIGMA;
   TypeBlurSharpen         = DEFAULT_TYPEBLURSHARPEN; // 0=Quick, 1=Gaussian
   Desat                   = DEFAULT_DESAT;
   Swirl                   = DEFAULT_SWIRL;
   Implode                 = DEFAULT_IMPLODE;
   onOffFilter             = DEFAULT_ONOFFFILTER;
}

bool cBrushDefinition::brushParams::hasFilter()
{
   if(   (GaussBlurSharpenSigma != 0)
      || (QuickBlurSharpenSigma != 0)
      || (Desat != 0)
      || (Swirl != 0)
      || (Implode != 0)
      || (onOffFilter != 0))
      return true;
   return false;
}

bool cBrushDefinition::brushParams::adjustValues(const brushParams &prev, double PctDone)
{
#define ADJUST_VALUE(name) if (prev.name != name) name = animValue(prev.name, name, PctDone)
   if (!prev.hasFilter(eFilterNormalize) && prev.Contrast != Contrast)  
      ADJUST_VALUE(Contrast);
   ADJUST_VALUE(X);
   ADJUST_VALUE(Y);
   ADJUST_VALUE(ZoomFactor);
   ADJUST_VALUE(ImageRotation);
   ADJUST_VALUE(Brightness);
   ADJUST_VALUE(Gamma);
   ADJUST_VALUE(Red);
   ADJUST_VALUE(Green);
   ADJUST_VALUE(Blue);
   ADJUST_VALUE(Desat);
   ADJUST_VALUE(Swirl);
   ADJUST_VALUE(Implode);
   if ((prev.onOffFilter != onOffFilter)
      || (prev.GaussBlurSharpenSigma != GaussBlurSharpenSigma)
      || (prev.QuickBlurSharpenSigma != QuickBlurSharpenSigma)
      || (prev.BlurSharpenRadius != BlurSharpenRadius)
      || (prev.TypeBlurSharpen != TypeBlurSharpen)
      )
      return true;
   return false;
}

bool cBrushDefinition::brushParams::adjustSlideValues(const brushParams &prev, double PctDone)
{
   XOffset = 0;
   YOffset = 0;

   double nX = qMin(X, prev.X);
   double nY = qMin(Y, prev.Y);
   double nZoomFactor = ZoomFactor + qAbs(X - prev.X);
   double nAspect = (ZoomFactor * AspectRatio + qAbs(Y - prev.Y)) / nZoomFactor;
   if( X != prev.X )
      XOffset = (X - prev.X) * PctDone;//PctDone/2;
   if( Y != prev.Y )
      YOffset = (Y - prev.Y) * PctDone;//PctDone/2;
   //qDebug() << "slideAdjust: X " << X << "/" << prev.X << " Y " << Y << "/" << prev.Y << " Zoom " << ZoomFactor  << " AspectRatio " << AspectRatio;
   //qDebug() << "slideAdjust: X " << nX << " Y " << nY << " Zoom " << nZoomFactor  << " AspectRatio " << nAspect;
   //qDebug() << "slideAdjust: XOffset " << XOffset << " YOffset " << YOffset << " for PctDone " << PctDone;
   X = nX;
   Y = nY;
   ZoomFactor = nZoomFactor;
   AspectRatio = nAspect;
   return false;
}

bool cBrushDefinition::brushParams::operator==(const brushParams &rhs)
{
   if( X != rhs.X || Y != rhs.Y )
      return false;
   if( ZoomFactor != rhs.ZoomFactor || ImageRotation != rhs.ImageRotation )
      return false;
   if( Brightness != rhs.Brightness || Contrast != rhs.Contrast || Gamma != rhs.Gamma )
      return false;
   if( Red != rhs.Red || Green != rhs.Green || Blue != rhs.Blue )
      return false;
   if( AspectRatio != rhs.AspectRatio )
      return false;
   if( GaussBlurSharpenSigma != rhs.GaussBlurSharpenSigma || BlurSharpenRadius != rhs.BlurSharpenRadius )
      return false;
   if( TypeBlurSharpen != rhs.TypeBlurSharpen || QuickBlurSharpenSigma != rhs.QuickBlurSharpenSigma )
      return false;
   if( Desat != rhs.Desat || Swirl != rhs.Swirl || Implode != rhs.Implode )
      return false;
   if( onOffFilter != rhs.onOffFilter )
      return false;
   return true;
}

bool cBrushDefinition::brushParams::isSlidingBrush(const brushParams &rhs)
{
   if( ZoomFactor != rhs.ZoomFactor || ImageRotation != 0.0 || ImageRotation != rhs.ImageRotation) // cant slide over rotated images with caching this time
      return false;
   if( Brightness != rhs.Brightness || Contrast != rhs.Contrast || Gamma != rhs.Gamma )
      return false;
   if( Red != rhs.Red || Green != rhs.Green || Blue != rhs.Blue )
      return false;
   if( AspectRatio != rhs.AspectRatio )
      return false;
   if( GaussBlurSharpenSigma != rhs.GaussBlurSharpenSigma || BlurSharpenRadius != rhs.BlurSharpenRadius )
      return false;
   if( TypeBlurSharpen != rhs.TypeBlurSharpen || QuickBlurSharpenSigma != rhs.QuickBlurSharpenSigma )
      return false;
   if( Desat != rhs.Desat || Swirl != rhs.Swirl || Implode != rhs.Implode )
      return false;
   if( onOffFilter != rhs.onOffFilter )
      return false;
   //if( X != rhs.X || Y != rhs.Y )
   //   return false;
   return true;
}

void cBrushDefinition::InitDefaultValues() 
{
   cBaseBrushDefinition::InitDefaultValues();

   SoundVolume             = -1;                            // Volume of soundtrack (-1=auto)
   Deinterlace             = false;                         // Add a YADIF filter to deinterlace video (on/off)
                              
   ImageSpeedWave          = SPEEDWAVE_PROJECTDEFAULT;
   params.InitDefaultValues();
}

//====================================================================================================================

cBrushDefinition::~cBrushDefinition() 
{
   removeFromCache();
   if (MediaObject) 
   {
      if (DeleteMediaObject)
         delete MediaObject;
      MediaObject = NULL;
   }
}

bool cBrushDefinition::hasFilter()
{
   return params.hasFilter();
}

//====================================================================================================================

QBrush *cBrushDefinition::GetBrush(QSizeF Size, bool PreviewMode, int Position, cSoundBlockList *SoundTrackMontage, double PctDone, cBrushDefinition *PreviousBrush, QRectF *srcRect)
{
   QBrush *Br = NULL;
   if( srcRect )
      *srcRect = QRectF(QPointF(0,0), Size);
   if (BrushType == BRUSHTYPE_IMAGEDISK) 
   {
      QImage Img = GetImageDiskBrush(Size, PreviewMode, Position, SoundTrackMontage, PctDone, PreviousBrush, srcRect);
      if (!Img.isNull()) 
         Br = new QBrush(Img);
   } 
   else 
      Br = cBaseBrushDefinition::GetBrush(QRectF(QPointF(0,0),Size));
   return Br;
}

QImage cBrushDefinition::GetImage(QSizeF Size,bool PreviewMode,int Position,cSoundBlockList *SoundTrackMontage,double PctDone,cBrushDefinition *PreviousBrush)
{
   if (BrushType == BRUSHTYPE_IMAGEDISK) 
   {
      return GetImageDiskBrush(Size, PreviewMode, Position, SoundTrackMontage, PctDone, PreviousBrush);
   } 
   return QImage();
}

QRectF cBrushDefinition::GetImageRect(QSizeF Size,bool PreviewMode,int Position,cSoundBlockList *SoundTrackMontage,double PctDone,cBrushDefinition *PreviousBrush)
{
   Q_UNUSED(SoundTrackMontage)
   Q_UNUSED(Position)
   Q_UNUSED(PreviewMode)
   QRectF rect(QPointF(0,0),Size);
   if( slidingBrush && PreviousBrush )
   {
      brushParams p = params;
      p.adjustValues(PreviousBrush->params, PctDone);
      qreal RealImageW = /*RenderImage*/Size.width();
      qreal RealImageH = /*RenderImage*/Size.height();
      qreal Hyp = sqrt(RealImageW*RealImageW+RealImageH*RealImageH);
      qreal HypPixel = Hyp * p.ZoomFactor;
      qreal x = qRound(Hyp * p.X);
      qreal y = qRound(Hyp * p.Y);
      qreal w = qRound(HypPixel);

      brushParams pSlide = params;
      if( PreviousBrush )
         pSlide.adjustSlideValues(PreviousBrush->params, PctDone);
      qreal HypPixels = Hyp * pSlide.ZoomFactor;
      qreal rx = Hyp/2 - RealImageW/2;
      qreal ry = Hyp/2 - RealImageH/2;
      qreal xs = qRound(Hyp * pSlide.X);
      qreal ys = qRound(Hyp * pSlide.Y);
      xs -= rx;
      ys -= ry;
      //HypPixel = Hyp * pSlide.ZoomFactor;
      qreal ws = qRound(HypPixels);
      qreal hs = qRound(w*pSlide.AspectRatio);//qRound(HypPixel * (Size.height() / Size.width()));
      qreal xsoffset = x - xs;
      qreal ysoffset = y - ys;
      qDebug() << "GetImageRect from " << xs << " " << ys <<" " << ws <<" " << hs << " with XOffset " << pSlide.XOffset << " (" << qRound(pSlide.XOffset * HypPixel) 
         << ") YOffset " << pSlide.YOffset << " (" << qRound(pSlide.YOffset *HypPixel) << ") " << xsoffset << " " <<  ysoffset << Qt::endl;
   }
   return rect;
}

//====================================================================================================================

/*
QImage cBrushDefinition::GetImageDiskBrush(QRectF Rect, bool PreviewMode, int64_t Position, cSoundBlockList *SoundTrackMontage, double PctDone, cBrushDefinition *PreviousBrush) 
{
   // If not an image or a video or filename is empty then return an empty Image
   if ((!MediaObject) || ((MediaObject->RessourceKey == -1) && (MediaObject->FileKey == -1) && (MediaObject->ObjectType != OBJECTTYPE_GMAPSMAP))) 
   {    // Allow gmap object without image (default image will be computed in imageat function)
      QImage Ret(Rect.width(),Rect.height(),QImage::Format_ARGB32_Premultiplied);
      QPainter Painter;
      Painter.begin(&Ret);
      Painter.setCompositionMode(QPainter::CompositionMode_Source);
      Painter.fillRect(QRect(0,0,Rect.width(),Rect.height()),Qt::transparent);
      Painter.end();
      return Ret;
   }

   // W and H = 0 when producing sound track in render process
   bool SoundOnly = (Rect.width() == 0 && Rect.height() == 0);
   // when only sound is reqeusted load sound and return a null-image
   if (SoundOnly) 
   {
      // Force loading of sound of video
      if (isVideo()) 
      {
         QImage *RenderImage=((cVideoFile *)MediaObject)->ImageAt(PreviewMode,Position,SoundTrackMontage,Deinterlace,SoundVolume,SoundOnly,false);
         if (RenderImage) 
            delete RenderImage;
      }
      return QImage();
   }


   // here starts the expensive part of getting the brush-image
   // every possible optimization here will improve preview and rendering
   QImage *RenderImage = NULL;

#ifdef USEBRUSHCACHE
   bool brushIsStatic = isStatic(PreviousBrush);
   if( brushIsStatic )
   {
      cLuLoImageCacheObject *cacheObj = ApplicationConfig->ImagesCache.FindObject(this);
      cLuLoImageCacheObject *prevCacheObj = PreviousBrush ? ApplicationConfig->ImagesCache.FindObject(PreviousBrush) : NULL;
      if( !cacheObj && prevCacheObj )
      {
         prevCacheObj->objKey = this;
         cacheObj = prevCacheObj;
      }
      if( cacheObj )
      {
         QImage *cacheImg = NULL;
         if( PreviewMode )
            cacheImg = cacheObj->CachePreviewImage;
         else
            cacheImg = cacheObj->CacheRenderImage;
         if( cacheImg )
         {  
            if( cacheImg->size() ==  Rect.size() )
            {
               qDebug() << "using cached brush";
               return QImage(*cacheImg);
            }
            else if( cacheImg->width() > Rect.width() &&  cacheImg->height() > Rect.height())
               return cacheImg->scaled(Rect.width(),Rect.height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
         }
      }
   }
#endif
   if( isVideo() )
      RenderImage = ((cVideoFile *)MediaObject)->ImageAt(PreviewMode,Position,SoundTrackMontage,this->Deinterlace,this->SoundVolume,false,false);
   else
      RenderImage = MediaObject->ImageAt(PreviewMode);
   //qDebug() << "GetImageDiskBrush from cache " << RenderImage->rect() << " rkey " << (MediaObject ? MediaObject->RessourceKey : -1) << " fkey " << (MediaObject ? MediaObject->FileKey : -1);

   if (RenderImage && isGMapsMap() ) 
      AddMarkerToImage(RenderImage);

   // Compute filters values
   QImage Ret;
   brushParams p = params;
   bool   ProgressifOnOffFilter = false;
   // Adjust values depending on PctDone and previous Filter (if exist)
   if (PreviousBrush)
      ProgressifOnOffFilter = p.adjustValues(PreviousBrush->params, PctDone);

   // Prepare image
   if (RenderImage) 
   {
      if (FullFilling()) 
      {
         // Create brush image with distortion
         Ret = QImage(RenderImage->scaled(Rect.width(),Rect.height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
         delete RenderImage;
         RenderImage = NULL;
      } 
      else 
      {
         // if it's a video and it's PreviewMode, then apply filter now before scale image (why??)
         if (isVideo() && PreviewMode) 
         {
            QImage TempNewRenderImage = RenderImage->convertToFormat(QImage::Format_ARGB32_Premultiplied);
            TempNewRenderImage = ApplyFilters(TempNewRenderImage,p.Brightness,p.Contrast,p.Gamma,p.Red,p.Green,p.Blue,p.Desat,ProgressifOnOffFilter,PreviousBrush,PctDone);
            delete RenderImage;
            RenderImage = new QImage(TempNewRenderImage);
         }

         // Prepare values from sourceimage size
         // if imagesize is same as rect use the whole image
         qreal RealImageW = RenderImage->width();
         qreal RealImageH = RenderImage->height();
         //qreal    Hyp       =floor(sqrt(RealImageW*RealImageW+RealImageH*RealImageH));
         qreal Hyp = qRound(sqrt(RealImageW * RealImageW + RealImageH * RealImageH));
         qreal HypPixel = Hyp * p.ZoomFactor;
         QImage NewRenderImage;

         if(RenderImage->size() == Rect.size() )
         {
            QPainter Painter;
            NewRenderImage = QImage(Rect.width(), Rect.height(), QImage::Format_ARGB32_Premultiplied);
            Painter.begin(&NewRenderImage);
            Painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            Painter.drawImage(0,0,*RenderImage);
            Painter.end();
            delete RenderImage;
            RenderImage=NULL;

         }
         else
         {
            NewRenderImage = QImage(Hyp,Hyp,QImage::Format_ARGB32_Premultiplied);
            // Expand canvas
            QPainter Painter;
            Painter.begin(&NewRenderImage);
            Painter.setCompositionMode(QPainter::CompositionMode_Source);
            Painter.fillRect(QRect(0,0,NewRenderImage.width(),NewRenderImage.height()),Qt::transparent);
            Painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            Painter.drawImage(QPoint((Hyp-RealImageW)/2,(Hyp-RealImageH)/2),*RenderImage);
            Painter.end();
            delete RenderImage;

            // Rotate image (if needed)
            if (p.ImageRotation != 0)
            {
               QTransform matrix;
               matrix.rotate(p.ImageRotation, Qt::ZAxis);
               int W = NewRenderImage.width();
               int H = NewRenderImage.height();
               NewRenderImage = NewRenderImage.transformed(matrix, ApplicationConfig->smoothing());
               int ax = NewRenderImage.width() - W;
               int ay = NewRenderImage.height() - H;
               NewRenderImage = NewRenderImage.copy(ax/2, ay/2, NewRenderImage.width() - ax, NewRenderImage.height() - ay);
            }
            // Get part we need and scaled it to destination size
            p.AspectRatio = qreal(HypPixel * Rect.height()) / qreal(Rect.width());

            int x, y, w, h;
            x = Hyp * p.X;
            y = Hyp * p.Y;
            w = HypPixel;
            h = p.AspectRatio;
            NewRenderImage = NewRenderImage.copy(x,y,w,h)
               .scaled(Rect.size().toSize(),Qt::IgnoreAspectRatio, ApplicationConfig->smoothing());
         }
         // Apply correction filters to DestImage (if it's a video and it's PreviewMode, then filter was apply before)
         if (!((MediaObject) && (MediaObject->ObjectType == OBJECTTYPE_VIDEOFILE) && (PreviewMode)))
            NewRenderImage = ApplyFilters(NewRenderImage, p.Brightness, p.Contrast, p.Gamma, p.Red, p.Green, p.Blue, p.Desat, ProgressifOnOffFilter, PreviousBrush, PctDone);
         if (p.Swirl != 0)   
            FltSwirl(NewRenderImage,-p.Swirl);
         if (p.Implode != 0) 
            FltImplode(NewRenderImage,p.Implode);

         if (!NewRenderImage.isNull() && (NewRenderImage.size() != Rect.size()) )
            Ret = NewRenderImage.scaled(Rect.size().toSize(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
         else 
            Ret = NewRenderImage;
      }
   }
#ifdef USEBRUSHCACHE
   if( brushIsStatic )
   {
      if( PreviewMode )
         ApplicationConfig->ImagesCache.addPreviewImage(this, Ret);
      else
         ApplicationConfig->ImagesCache.addRenderImage(this, Ret);
   }
#endif
   return Ret;
}
*/
QImage cBrushDefinition::GetImageDiskBrush(QSizeF SizeF, bool PreviewMode, int64_t Position, cSoundBlockList *SoundTrackMontage, double PctDone, cBrushDefinition *PreviousBrush, QRectF *srcRect )
{
   //AUTOTIMER(at,"  GetImageDiskBrush");
   // If not an image or a video or filename is empty then return an empty Image
   // Allow gmap object without image (default image will be computed in imageat function)
   if (!MediaObject || (MediaObject->RessourceKey() == -1 && MediaObject->FileKey() == -1 && !MediaObject->is_Gmapsmap())) 
   {    
      QImage Ret(SizeF.toSize(),QImage::Format_ARGB32_Premultiplied);
      Ret.fill(0);
      return Ret;
   }

   // W and H = 0 when producing sound track in render process
   bool SoundOnly = SizeF.isEmpty();
   // when only sound is requested load sound and return a null-image
   if (SoundOnly) 
   {
      // Force loading of sound of video
      if (isVideo()) 
      {
         QImage *RenderImage = ((cVideoFile *)MediaObject)->ImageAt(PreviewMode, Position, SoundTrackMontage, Deinterlace, SoundVolume, SoundOnly, false);
         if (RenderImage) 
            delete RenderImage;
      }
      return QImage();
   }

   bool brushIsStatic = isStatic(PreviousBrush);
   if( !brushIsStatic )
      return GetRealImageDiskBrush(SizeF, PreviewMode, Position, SoundTrackMontage, PctDone, PreviousBrush, srcRect);
   //qDebug() <<"static brush " <<  SizeF << " for %done " << PctDone;
   return GetRealImageDiskBrush(SizeF, PreviewMode, Position, SoundTrackMontage, PctDone, PreviousBrush, srcRect);
}

QRectF animCenterValue(QRectF rfStart,QRectF rfEnd,double PctDone)
{
   QPointF sCenter = rfStart.center(); 
   QPointF eCenter = rfEnd.center(); 
   double xDiff = animValue(sCenter.x(), eCenter.x(),PctDone);
   double yDiff = animValue(sCenter.y(), eCenter.y(),PctDone);
   double wDiff = animDiffValue(rfStart.width(), rfEnd.width(), PctDone);
   double hDiff = animDiffValue(rfStart.height(), rfEnd.height(), PctDone);
   return QRectF( xDiff - wDiff/2, yDiff - hDiff/2, wDiff, hDiff);
   //return QRectF( /*rfStart.x()+*/xDiff-rfStart.x(), /*rfStart.y()+*/yDiff-rfStart.y(), 
   //               animDiffValue(rfStart.width(), rfEnd.width(), PctDone), animDiffValue(rfStart.height(), rfEnd.height(), PctDone) );
}

QRectF animQValue(QRectF rfStart,QRectF rfEnd,double PctDone)
{
   QPointF sCenter = rfStart.center(); 
   QPointF eCenter = rfEnd.center(); 
   double xDiff = animValue(sCenter.x(), eCenter.x(),PctDone);
   double yDiff = animValue(sCenter.y(), eCenter.y(),PctDone);
   double wDiff = animDiffValue(rfStart.width(), rfEnd.width(), PctDone);
   double hDiff = animDiffValue(rfStart.height(), rfEnd.height(), PctDone);
   return QRectF( xDiff - wDiff/2, yDiff - hDiff/2, wDiff, hDiff);
   //return QRectF( /*rfStart.x()+*/xDiff-rfStart.x(), /*rfStart.y()+*/yDiff-rfStart.y(), 
   //               animDiffValue(rfStart.width(), rfEnd.width(), PctDone), animDiffValue(rfStart.height(), rfEnd.height(), PctDone) );
}

#define NEW_ROTATE
QImage cBrushDefinition::GetRealImageDiskBrush(QSizeF Size, bool PreviewMode, int64_t Position, cSoundBlockList *SoundTrackMontage, double PctDone, cBrushDefinition *PreviousBrush, QRectF *srcRect)
{
   //static int ri = 0;
   //AUTOTIMER(at, "  GetRealImageDiskBrush");
   //autoTimer at( "  GetRealImageDiskBrush");
   // here starts the expensive part of getting the brush-image
   // every possible optimization here will improve preview and rendering
   QImage *RenderImage = NULL;
   if( srcRect )
      *srcRect = QRectF(QPointF(0,0), Size);
#ifdef USEBRUSHCACHE
   if( slidingBrush && PreviousBrush)
   {
      QImage cacheImg;
      cLuLoImageCacheObject *cacheObj = ApplicationConfig->ImagesCache.FindObject(this);
      if( cacheObj && cacheObj->objSubKey == SUBKEY_SLIDE )
      {
         QImage *cacheImg = NULL;
         if( PreviewMode )
            cacheImg = cacheObj->CachePreviewImage;
         else
            cacheImg = cacheObj->CacheRenderImage;
         if( cacheImg )
         {  
            brushParams pSlide = params;
            if( PreviousBrush )
               pSlide.adjustSlideValues(PreviousBrush->params, PctDone);
            qreal dw = cacheImg->width() - Size.width();
            qreal dh = cacheImg->height() - Size.height();
            qreal xsoffset = 0;
            qreal ysoffset = 0;
            if ( pSlide.ZoomFactor !=  params.ZoomFactor )
            {
               if( pSlide.X - params.X < 0 )
                  xsoffset = dw * PctDone;
               else 
                  xsoffset = dw * (1-PctDone);
            }
            if( dh != 0  )
            {
               if( pSlide.Y - params.Y < 0 )
                  ysoffset = dh * PctDone;
               else
                  ysoffset = dh * (1-PctDone);
            }
            if( srcRect )
               *srcRect = QRectF(QPointF(xsoffset,ysoffset), Size);
            return QImage(*cacheImg);
         }
      }
   }
   bool brushIsStatic = isStatic(PreviousBrush);
   if( brushIsStatic )
   {
      QImage cacheImg;
      cLuLoImageCacheObject *cacheObj = ApplicationConfig->ImagesCache.FindObject(this);
      cLuLoImageCacheObject *prevCacheObj = PreviousBrush ? ApplicationConfig->ImagesCache.FindObject(PreviousBrush) : NULL;
      if( !cacheObj && prevCacheObj )
      {
         prevCacheObj->objKey = this;
         cacheObj = prevCacheObj;
      }
      pushedImage = false;
      if( cacheObj && cacheObj->objSubKey > SUBKEY_SLIDE)
      {
         QImage *cacheImg = NULL;
         if( PreviewMode )
            cacheImg = cacheObj->CachePreviewImage;
         else
            cacheImg = cacheObj->CacheRenderImage;
         if( cacheImg )
         {  
            pushedImage = cacheObj->objSubKey == SUBKEY_PUSHBACK;
            if( cacheObj == prevCacheObj )
               pushedImage = true;
            if( cacheImg->size() ==  Size.toSize() )
               return QImage(*cacheImg);
            else if( cacheImg->width() >= Size.width() &&  cacheImg->height() >= Size.height())
               return cacheImg->scaled(Size.toSize(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
         }
      }
   }
   pushedImage = false;
#endif
   // Compute filters values
   brushParams p = params;
   bool ProgressifOnOffFilter = false;
   // Adjust values depending on PctDone and previous Filter (if exist)
   if (PreviousBrush)
      ProgressifOnOffFilter = p.adjustValues(PreviousBrush->params, PctDone);

   if( isVideo() )
   {
      //autoTimer at( "  GetVideoImage");
      //qDebug() << "get Video Image at " << Position << " sound into " << (void *)SoundTrackMontage;
      RenderImage = ((cVideoFile *)MediaObject)->ImageAt(PreviewMode, Position, SoundTrackMontage, Deinterlace, SoundVolume, false, false);
   }
   else if (isVectorImageFile() && pAppConfig->enableSmoothSVG)
   {
      qreal RealImageW = MediaObject->width();
      qreal RealImageH = MediaObject->height();
      qreal Hyp = sqrt(RealImageW*RealImageW + RealImageH*RealImageH);
      qreal HypPixel = Hyp * p.ZoomFactor;
      //qreal x = qRound(Hyp * p.X);
      //qreal y = qRound(Hyp * p.Y);
      qreal w = qRound(HypPixel);
      qreal h = qRound(HypPixel * (Size.height() / Size.width()));
      qreal svgScaleX = Size.width() / w;
      qreal svgScaleY = Size.height() / h;
      //qDebug() << "svgScaleX is " << svgScaleX << " svgScaleY is " << svgScaleY << " maxScale is " << qMax(svgScaleX, svgScaleY);
      qreal maxScale = qMax(svgScaleX, svgScaleY);
      QSizeF scaledSize = QSizeF(RealImageW, RealImageH) * maxScale;
      RenderImage = MediaObject->ImageAt(PreviewMode, scaledSize);
   }
   else
      RenderImage = MediaObject->ImageAt(PreviewMode);
   //qDebug() << "GetImageDiskBrush from cache " << RenderImage->rect() << " rkey " << (MediaObject ? MediaObject->RessourceKey : -1) << " fkey " << (MediaObject ? MediaObject->FileKey : -1);

   if (RenderImage && isGMapsMap() ) 
      AddMarkerToImage(RenderImage);

   //if( slidingBrush )
   //{
   //   brushParams pSlide = params;
   //   if( PreviousBrush )
   //      pSlide.adjustSlideValues(PreviousBrush->params, PctDone);
   //}

   // Prepare image
   QImage Ret;
   if (!RenderImage) // must be an error
      return Ret;
   if (FullFilling()) 
   {
      // Create brush image with distortion
      Ret = QImage(RenderImage->scaled(Size.toSize(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
      delete RenderImage;
      RenderImage = NULL;
   } 
   else 
   {
      // if it's a video and it's PreviewMode, then apply filter now before scale image (why??)
      if (isVideo() && PreviewMode) 
      {
         QImage TempNewRenderImage = RenderImage->convertToFormat(QImage::Format_ARGB32_Premultiplied);
         /*TempNewRenderImage = */ApplyFilters(TempNewRenderImage, p.Brightness, p.Contrast, p.Gamma, p.Red, p.Green, p.Blue, p.Desat, ProgressifOnOffFilter, PreviousBrush, PctDone);
         delete RenderImage;
         RenderImage = new QImage(TempNewRenderImage);
      }

      // Prepare values from sourceimage size
      // if imagesize is same as rect use the whole image
      qreal RealImageW = RenderImage->width();
      qreal RealImageH = RenderImage->height();
      qreal Hyp = sqrt(RealImageW*RealImageW+RealImageH*RealImageH);
      //qreal Hyp = floor(sqrt(RealImageW*RealImageW+RealImageH*RealImageH));
      //qreal Hyp = qRound(sqrt(RealImageW * RealImageW + RealImageH * RealImageH));
      qreal HypPixel = Hyp * p.ZoomFactor;
      qreal x = qRound(Hyp * p.X);
      qreal y = qRound(Hyp * p.Y);
      qreal w = qRound(HypPixel);
      qreal h = qRound(HypPixel * (Size.height() / Size.width()));
      //qDebug() << "srcRect is " << QRect(x, y, w, h) << " ratio " << h/w;
      //qDebug() << "dstRect is " << QRect(QPoint(0, 0), Size.toSize()) << " ratio " << Size.height() / Size.width();
      //qDebug() << "scaleX is " << Size.width() / w << " scaleY is " << Size.height() / h;

      //if (PreviousBrush)
      //{
      //   QRectF rfStart(Hyp*PreviousBrush->params.X, Hyp*PreviousBrush->params.Y,Hyp * PreviousBrush->params.ZoomFactor, (Hyp * PreviousBrush->params.ZoomFactor) * (Size.height() / Size.width()));
      //   QRectF rfEnd(Hyp*params.X, Hyp*params.Y,Hyp * params.ZoomFactor, (Hyp * params.ZoomFactor) * (Size.height() / Size.width()));
      //   //QRectF rf = animValue(rfStart,rfEnd,PctDone);
      //   QRectF rf = animCenterValue(rfStart,rfEnd,PctDone);
      //   qDebug() << "used rect " << QRectF(x,y,w,h) << " ncalcedrect " << rf << " xdiff " << x-rf.x() << " ydiff " << y-rf.y();
      //   qDebug() << "scaleX " << Size.width()/w << " scaleY " << Size.height()/h << " scaleXnew " << Size.width()/rf.width() << " scaleYnew " << Size.height()/ rf.height();
      //}
      qreal x0 = qRound((Hyp - RealImageW)/2);
      qreal y0 = qRound((Hyp - RealImageH)/2);
      QImage NewRenderImage;

      if(RenderImage->size() == Size.toSize() 
         && ( (x == 0.0 && x0 == 0.0) || (x != 0.0 && x0 != 0.0 && qFuzzyCompare(x,x0)) )
         && ( (y == 0.0 && y0 == 0.0) || (y != 0.0 && y0 != 0.0 && qFuzzyCompare(y,y0)) ) 
         )
      {

         NewRenderImage = *RenderImage;
         delete RenderImage;
         RenderImage = NULL;
      }
      else
      {
         QSize srcSize(w,h);
         int rx,ry;
         rx = Hyp/2 - RealImageW/2;
         ry = Hyp/2 - RealImageH/2;
         if (p.ImageRotation != 0)
         {
            #ifdef NEW_ROTATE
            qreal srcSizeX = w;
            qreal srcSizeY = h;
            qreal destSizeX = Size.width();
            qreal destSizeY = Size.height();
            qreal XOffset = x-(Hyp-RealImageW)/2;
            qreal YOffset = y-(Hyp-RealImageH)/2;
            qreal XCenter = RealImageW/2.0;
            qreal YCenter = RealImageH/2.0;
            qreal scaleX = destSizeX/srcSizeX;
            qreal scaleY = destSizeY/srcSizeY;

            QTransform transform;
            transform.translate(XCenter, YCenter);
            transform.rotate(p.ImageRotation);
            transform.translate(-XCenter,-YCenter);
            transform *= QTransform::fromTranslate(-XOffset, -YOffset);
            transform *= QTransform::fromScale(scaleX,scaleY);

            NewRenderImage = QImage(Size.toSize(),QImage::Format_ARGB32_Premultiplied);
            NewRenderImage.fill(0);
            QPainter p(&NewRenderImage);
            p.setTransform(transform);
            p.drawImage(0,0, *RenderImage);
            #else
            QTransform transform;
            transform.rotate(p.ImageRotation, Qt::ZAxis);

            QRectF rf(0,0,RenderImage->width(), RenderImage->height());
            QRect rf2 = transform.mapRect(rf).toRect();
            qreal rrx = rx-(rf2.width()-RenderImage->width())/2;
            qreal rry = ry-(rf2.height()-RenderImage->height())/2;
            qreal scaleX = Size.width()/w;
            qreal scaleY = Size.height()/h;

            QImage rotatedImage = RenderImage->transformed(transform, Qt::SmoothTransformation );
            //qDebug() << "RenderImage " << RenderImage->size() << " rotatedRect " << rf2.size() << " scale " << scaleX << " " << scaleY;
            //qDebug() << "RenderImage " << RenderImage->size() << " rotatedImage " << rotatedImage.size();
            //QTransform trtransform =  QImage::trueMatrix(transform,RenderImage->width(),RenderImage->height());

            rx -= (rotatedImage.width()-RenderImage->width())/2;
            ry -= (rotatedImage.height()-RenderImage->height())/2;
            //qDebug() << rx << " <--> " << rrx << "  " << ry << " <--> " << rry;
            x -= rx;
            y -= ry;
            //qDebug() << "copy from "<< x << "," << y << " " << w << "x" << h << "  rx/ry " << rx << "x" << ry << " and scale to " << Size;

            transform.scale(scaleX, scaleY);
            transform.translate(x,y);
            //NewRenderImage = RenderImage->transformed(transform, Qt::SmoothTransformation );
            NewRenderImage = QImage(Size.toSize(),QImage::Format_ARGB32_Premultiplied);
            NewRenderImage.fill(0);
            QPainter p(&NewRenderImage);
            p.drawImage(QRectF(QPointF(0,0),Size), rotatedImage,QRectF(x,y,w,h));
            //p.drawImage(QRectF(QPointF(0,0),Size), RenderImage->transformed(transform, Qt::SmoothTransformation ),QRectF(0,0,w,h));
            #endif
         }
         else
         {
//            if( abs(h - Size.toSize().height()) < 2 || abs(w - Size.toSize().width()) < 2 )
//               qDebug() << "minimal resize from " << RenderImage->size() << " to " << Size.toSize() << "(" << Size << ") " << x << " " << x0 << " "  << y << " " << y0;
            x -= rx;
            y -= ry;
            //AUTOTIMER(atx,"  copy and scale");
            //qDebug() << "copy from "<< x << "," << y << " " << w << "x" << h << "  rx/ry " << rx << "x" << ry << " and scale to " << Size << " height was " << qreal(HypPixel * Size.height()) / qreal(Size.width());
            //if( slidingBrush )
            //   qDebug() << "get Image from " << x << " " <<y <<" " <<w <<" " << h;
            if( slidingBrush )
            {
               brushParams pSlide = params;
               if( PreviousBrush )
                  pSlide.adjustSlideValues(PreviousBrush->params, PctDone);
               qreal HypPixel = Hyp * pSlide.ZoomFactor;
               rx = Hyp/2 - RealImageW/2;
               ry = Hyp/2 - RealImageH/2;
               qreal xs = qRound(Hyp * pSlide.X);
               qreal ys = qRound(Hyp * pSlide.Y);
               xs -= rx;
               ys -= ry;
               qreal ws = qRound(HypPixel);
               qreal hs = qRound(ws*pSlide.AspectRatio);
               QSizeF nSize = QSizeF(Size.width() * ws/w, Size.height() * hs/h);
               //qDebug() << "change size from " << Size << " to " << nSize;
               qreal xsoffset = 0;
               qreal ysoffset = 0;
               if ( pSlide.ZoomFactor !=  params.ZoomFactor )
               {
                  if( pSlide.X - params.X < 0 )
                     xsoffset = (nSize.width() - Size.width()) * PctDone;
                  else 
                     xsoffset = (nSize.width() - Size.width()) * (1-PctDone);
               }
               if( ys != y )
               {
                  if( pSlide.Y - params.Y < 0 )
                     ysoffset = (nSize.height() - Size.height()) * PctDone;
                  else
                     ysoffset = (nSize.height() - Size.height()) * (1-PctDone);
               }
               Size = nSize;
               //qDebug() << "get Image from " << xs << " " << ys <<" " << ws <<" " << hs << " with XOffset " << pSlide.XOffset << " (" << qRound(pSlide.XOffset * HypPixel) 
               //   << ") YOffset " << pSlide.YOffset << " (" << qRound(pSlide.YOffset *HypPixel) << ") " << xsoffset << " " <<  ysoffset << endl;
               if( srcRect )
                  *srcRect = QRectF(QPointF(xsoffset,ysoffset), Size);

               x = xs;
               y = ys;
               w = ws;
               h = hs;
            }
            NewRenderImage = QImage(Size.toSize(),QImage::Format_ARGB32_Premultiplied);
            NewRenderImage.fill(0);
            QPainter painter(&NewRenderImage);
            painter.drawImage(QRectF(QPointF(0,0),Size), *RenderImage,QRectF(x,y,w,h));
//            if( abs(/*RenderImage->size().height()*/ h - Size.toSize().height()) < 2 || abs(/*RenderImage->size().width()*/ w - Size.toSize().width()) < 2 )
//               qDebug() << "minimal resize from " << RenderImage->size() << " to " << Size.toSize() << "(" << Size << ")";
         }
         delete RenderImage;
      }
      // Apply correction filters to DestImage (if it's a video and it's PreviewMode, then filter was applied before)
      if (!(MediaObject && MediaObject->is_Videofile() && PreviewMode))
         /*NewRenderImage = */ApplyFilters(NewRenderImage, p.Brightness, p.Contrast, p.Gamma, p.Red, p.Green, p.Blue, p.Desat, ProgressifOnOffFilter, PreviousBrush, PctDone);
      if (p.Swirl != 0)   
         ffdFilter::FltSwirl(NewRenderImage,-p.Swirl);
      if (p.Implode != 0) 
         ffdFilter::FltImplode(NewRenderImage,p.Implode);

      if (!NewRenderImage.isNull() && (NewRenderImage.size() != Size.toSize()) )
         Ret = NewRenderImage.scaled(Size.toSize(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
      else 
         Ret = NewRenderImage;
   }
         /*
         // apply LightFilter
         if( true && BackgroundForm > 1 && PenSize > 0)
         {
            LightFilter *f;
            QImage orgImage;
            //QBrush brush = Painter->brush();
            //brush.setMatrix(QMatrix());
            //orgImage = brush.textureImage(); 
            QImage *filteredImage;
            QImage maskImage(orgImage.width(), orgImage.height(), QImage::Format_ARGB32);
            maskImage.fill(Qt::gray);
            QRectF realMaskRect = QRectF(0,0,orgImage.width(), orgImage.height());//WSRect(maskRect).applyTo(orgRect);
            // get the form-polygon
            QList<QPolygonF> masking = ComputePolygon(BackgroundForm,realMaskRect);
            QPainter p(&maskImage);
            p.setCompositionMode( QPainter::CompositionMode_Source);
            //p.setBrush(QBrush(Qt::transparent));
            QPen Pen;
            Pen.setCapStyle(Qt::RoundCap);
            Pen.setJoinStyle(Qt::RoundJoin);
            Pen.setCosmetic(false);
            Pen.setStyle(Qt::SolidLine);
            Pen.setColor(Qt::white);
            Pen.setWidthF(double(2)*ADJUST_RATIO);
            p.setPen(Pen);
            p.setBrush(Qt::black);
            //p.setRenderHints(hints,true);

            //p.drawRect(0,0,200,200);
            for (int i = 0; i < masking.count(); i++) 
               p.drawPolygon(masking.at(i));
            p.end();

            f = new LightFilter();
            f->setBumpShape(0);
            f->setBumpHeight(0.5);
            f->setBumpSoftness(2);
            f->setBumpSource(LightFilter::BUMPS_FROM_MAP);
            f->setColorSource(LightFilter::COLORS_FROM_IMAGE);
            ImageFunction2D *bumpfunction = new ImageFunction2D(maskImage);
            f->setBumpFunction((Function2D*)bumpfunction);
            f->getLights().at(0)->setElevation(11.0 * 3.1415927 / 180.0);
            filteredImage = f->filter(orgImage, 0);

            Painter->setPen(Qt::NoPen);
            brush.setTextureImage(*filteredImage);
            brush.setTransform(MatrixBR);
            Painter->setBrush(brush);
            delete filteredImage;
            delete f;
         }
         */
#ifdef USEBRUSHCACHE
   if( brushIsStatic || (slidingBrush && PreviousBrush) )
   {
      int subkey = SUBKEY_STD;
      if( slidingBrush && PreviousBrush)
         subkey = SUBKEY_SLIDE;
      //qDebug() << " add to cache, subkey =  " << subkey << " size is " << Ret.size();
      if( PreviewMode )
         ApplicationConfig->ImagesCache.addPreviewImage(this, subkey, Ret);
      else
         ApplicationConfig->ImagesCache.addRenderImage(this, subkey, Ret);
   }
#endif
   return Ret;
}

void cBrushDefinition::pushBack(const QImage &img, bool PreviewMode)
{
   cLuLoImageCacheObject *cacheObj = ApplicationConfig->ImagesCache.FindObject(this);
   //if( cacheObj )
   //   ApplicationConfig->ImagesCache.addImage(this,1,img,PreviewMode);
   //qDebug() << "pushBack for BrushDef " << (void *)this << " preview: " << PreviewMode;
   //cLuLoImageCacheObject *cacheObj = ApplicationConfig->ImagesCache.FindObject(this);
   if( cacheObj )
   {
      cacheObj->objSubKey = SUBKEY_PUSHBACK;
      if( PreviewMode )
      {
         cacheObj->setPreviewImage(img);
      }
      else
      {
         cacheObj->setRenderImage(img);
      }
      pushedImage = true;
   }
}

//====================================================================================================================
// Note:This function is use only by cBrushDefinition !
QImage cBrushDefinition::ApplyFilter(QImage Image) 
{
   if (Image.isNull()) 
      return Image;
   if (Brightness() != 0)                                ffdFilter::FltBrightness(Image,Brightness());
   if (Contrast() != 0 && !hasFilter(eFilterNormalize) ) ffdFilter::FltContrast(Image,Contrast());
   if (Gamma()!=1)                                       ffdFilter::FltGamma(Image,Gamma());
   if (( Red() != 0) || (Green() != 0) || (Blue() != 0)) ffdFilter::FltColorize(Image,Red(),Green(),Blue());
   if ((TypeBlurSharpen() == 0) && (QuickBlurSharpenSigma() < 0)) ffdFilter::FltBlur(Image,-QuickBlurSharpenSigma());
   if ((TypeBlurSharpen() == 0) && (QuickBlurSharpenSigma() > 0)) ffdFilter::FltSharpen(Image,QuickBlurSharpenSigma());
   if ((TypeBlurSharpen() == 1) && (GaussBlurSharpenSigma() < 0)) ffdFilter::FltGaussianBlur(Image,BlurSharpenRadius(),-GaussBlurSharpenSigma());
   if ((TypeBlurSharpen() == 1) && (GaussBlurSharpenSigma() > 0)) ffdFilter::FltGaussianSharpen(Image,BlurSharpenRadius(),GaussBlurSharpenSigma());
   if (hasFilter(eFilterDespeckle)) ffdFilter::FltDespeckle(Image);
   if (hasFilter(eFilterEqualize))  ffdFilter::FltEqualize(Image);
   if (hasFilter(eFilterGray))      ffdFilter::FltGrayscale(Image);
   if (hasFilter(eFilterNegative))  Image.invertPixels(QImage::InvertRgb);
   if (hasFilter(eFilterEmboss))    ffdFilter::FltEmboss(Image,0,1);
   if (hasFilter(eFilterEdge))      ffdFilter::FltEdge(Image);
   if (hasFilter(eFilterAntialias)) ffdFilter::FltAntialias(Image);
   if (hasFilter(eFilterNormalize)) ffdFilter::FltAutoContrast(Image);
   if (hasFilter(eFilterCharcoal))  ffdFilter::FltCharcoal(Image);
   if (hasFilter(eFilterOil))       ffdFilter::FltOilPaint(Image);
   if (Desat()!=0)                  ffdFilter::FltDesaturate(Image,Desat());
   if (Swirl()!=0)                  ffdFilter::FltSwirl(Image,-Swirl());
   if (Implode()!=0)                ffdFilter::FltImplode(Image,Implode());
   return Image;
}
//====================================================================================================================

bool cBrushDefinition::canCacheFilteredImages(double TheBrightness, double TheContrast, double TheGamma, double TheRed, double TheGreen, double TheBlue, double TheDesat, cBrushDefinition* prevBrush)
{
   if (TheBrightness != 0)
      return false;
   if (TheContrast != 0)
      return false;
   if (TheGamma != 1) 
      return false;
   if ((TheRed != 0) || (TheGreen != 0) || (TheBlue != 0)) 
      return false;
   if (TheDesat != 0)
      return false;
   if(prevBrush)
   {
      if( X() != prevBrush->X() )
         return false;
      if (Y() != prevBrush->Y())
         return false;
      if(ZoomFactor() != prevBrush->ZoomFactor())
         return false;
      if(AspectRatio() != prevBrush->AspectRatio())
         return false;
   }
   return true;
}

void cBrushDefinition::ApplyFilters(QImage &NewRenderImage, double TheBrightness, double TheContrast, double TheGamma, double TheRed, double TheGreen, double TheBlue, double TheDesat,
   bool ProgressifOnOffFilter, cBrushDefinition *PreviousBrush, double PctDone)
{
   if (TheBrightness != 0)                                  ffdFilter::FltBrightness(NewRenderImage, TheBrightness);
   if (TheContrast != 0)                                    ffdFilter::FltContrast(NewRenderImage, TheContrast);
   if (TheGamma != 1)                                       ffdFilter::FltGamma(NewRenderImage, TheGamma);
   if ((TheRed != 0) || (TheGreen != 0) || (TheBlue != 0))  ffdFilter::FltColorize(NewRenderImage, TheRed, TheGreen, TheBlue);
   if (TheDesat != 0)                                       ffdFilter::FltDesaturate(NewRenderImage, TheDesat);
      //if (hasFilter(eFilterGray))      FltGrayscale(NewRenderImage, PctDone);

   if ((ProgressifOnOffFilter) || (GaussBlurSharpenSigma() != 0) || (QuickBlurSharpenSigma() != 0) || (OnOffFilters() != 0))
   {
      if( canCacheFilteredImages(TheBrightness, TheContrast, TheGamma, TheRed, TheGreen, TheBlue, TheDesat, PreviousBrush) && ProgressifOnOffFilter )
      {  
         QImage cacheImgNew = ApplicationConfig->ImagesCache.getImage(this,-4,true);
         if( !cacheImgNew.isNull() )
         {
            QImage cacheImgPrev = ApplicationConfig->ImagesCache.getImage(this,-5,true);
            if( !cacheImgPrev.isNull() )
            {
               // Mix images
               QPainter P;
               P.begin(&cacheImgPrev);
               P.setOpacity(PctDone);
               P.drawImage(0, 0, cacheImgNew);
               //P.setOpacity(1);
               P.end();
               NewRenderImage = cacheImgPrev;
               return;
            }
         }
      }

      QImage PreviousImage;
      if (ProgressifOnOffFilter)
         PreviousImage = NewRenderImage.copy();
      // Apply filter to image
      if ((TypeBlurSharpen() == 1) && (GaussBlurSharpenSigma() < 0)) ffdFilter::FltGaussianBlur(NewRenderImage, BlurSharpenRadius(), -GaussBlurSharpenSigma());
      if ((TypeBlurSharpen() == 1) && (GaussBlurSharpenSigma() > 0)) ffdFilter::FltGaussianSharpen(NewRenderImage, BlurSharpenRadius(), GaussBlurSharpenSigma());
      if ((TypeBlurSharpen() == 0) && (QuickBlurSharpenSigma() < 0)) ffdFilter::FltBlur(NewRenderImage, -QuickBlurSharpenSigma());
      if ((TypeBlurSharpen() == 0) && (QuickBlurSharpenSigma() > 0)) ffdFilter::FltSharpen(NewRenderImage, QuickBlurSharpenSigma());
      if (hasFilter(eFilterDespeckle)) ffdFilter::FltDespeckle(NewRenderImage);
      if (hasFilter(eFilterEqualize))  ffdFilter::FltEqualize(NewRenderImage);
      if (hasFilter(eFilterGray))      ffdFilter::FltGrayscale(NewRenderImage);
      if (hasFilter(eFilterNegative))  NewRenderImage.invertPixels(QImage::InvertRgb);
      if (hasFilter(eFilterEmboss))    ffdFilter::FltEmboss(NewRenderImage, 0, 1);
      if (hasFilter(eFilterEdge))      ffdFilter::FltEdge(NewRenderImage);
      if (hasFilter(eFilterAntialias)) ffdFilter::FltAntialias(NewRenderImage);
      if (hasFilter(eFilterNormalize)) ffdFilter::FltAutoContrast(NewRenderImage);
      if (hasFilter(eFilterCharcoal))  ffdFilter::FltCharcoal(NewRenderImage);
      if (hasFilter(eFilterOil))       ffdFilter::FltOilPaint(NewRenderImage);
      if( canCacheFilteredImages(TheBrightness, TheContrast, TheGamma, TheRed, TheGreen, TheBlue, TheDesat, PreviousBrush) && ProgressifOnOffFilter )
         ApplicationConfig->ImagesCache.addImage(this,-4,NewRenderImage,true);   

      if (ProgressifOnOffFilter)
      {
         // Apply previous filter to copied image
         if ((PreviousBrush->TypeBlurSharpen() == 1) && (PreviousBrush->GaussBlurSharpenSigma() < 0))  ffdFilter::FltGaussianBlur(PreviousImage, PreviousBrush->BlurSharpenRadius(), -PreviousBrush->GaussBlurSharpenSigma());
         if ((PreviousBrush->TypeBlurSharpen() == 1) && (PreviousBrush->GaussBlurSharpenSigma() > 0))  ffdFilter::FltGaussianSharpen(PreviousImage, PreviousBrush->BlurSharpenRadius(), PreviousBrush->GaussBlurSharpenSigma());
         if ((PreviousBrush->TypeBlurSharpen() == 0) && (PreviousBrush->QuickBlurSharpenSigma() < 0))  ffdFilter::FltBlur(PreviousImage, -PreviousBrush->QuickBlurSharpenSigma());
         if ((PreviousBrush->TypeBlurSharpen() == 0) && (PreviousBrush->QuickBlurSharpenSigma() > 0))  ffdFilter::FltSharpen(PreviousImage, PreviousBrush->QuickBlurSharpenSigma());
         if (PreviousBrush->hasFilter(eFilterDespeckle))  ffdFilter::FltDespeckle(PreviousImage);
         if (PreviousBrush->hasFilter(eFilterEqualize))   ffdFilter::FltEqualize(PreviousImage);
         if (PreviousBrush->hasFilter(eFilterGray))       ffdFilter::FltGrayscale(PreviousImage);
         if (PreviousBrush->hasFilter(eFilterNegative))   PreviousImage.invertPixels(QImage::InvertRgb);
         if (PreviousBrush->hasFilter(eFilterEmboss))     ffdFilter::FltEmboss(PreviousImage, 0, 1);
         if (PreviousBrush->hasFilter(eFilterEdge))       ffdFilter::FltEdge(PreviousImage);
         if (PreviousBrush->hasFilter(eFilterAntialias))  ffdFilter::FltAntialias(PreviousImage);
         if (PreviousBrush->hasFilter(eFilterNormalize))  ffdFilter::FltAutoContrast(PreviousImage);
         if (PreviousBrush->hasFilter(eFilterCharcoal))   ffdFilter::FltCharcoal(PreviousImage);
         if (PreviousBrush->hasFilter(eFilterOil))        ffdFilter::FltOilPaint(PreviousImage);
         if( canCacheFilteredImages(TheBrightness, TheContrast, TheGamma, TheRed, TheGreen, TheBlue, TheDesat, PreviousBrush) )
            ApplicationConfig->ImagesCache.addImage(this,-5,PreviousImage,true);   

         // Mix images
         QPainter P;
         P.begin(&PreviousImage);
         P.setOpacity(PctDone);
         P.drawImage(0, 0, NewRenderImage);
         //P.setOpacity(1);
         P.end();
         NewRenderImage = PreviousImage;
      }
   }
}
//====================================================================================================================
// Return height for width depending on Rect geometry
int cBrushDefinition::GetHeightForWidth(int WantedWith, QRectF Rect) 
{
   double Ratio = Rect.width()/Rect.height();
   return int(double(WantedWith)/Ratio);
}

//====================================================================================================================
// Return width for height depending on Rect geometry
int cBrushDefinition::GetWidthForHeight(int WantedHeight, QRectF Rect) 
{
   double Ratio = Rect.height()/Rect.width();
   return int(double(WantedHeight)/Ratio);
}

//====================================================================================================================
// create a COMPOSITIONTYPE_SHOT brush as a copy of a given brush
void cBrushDefinition::CopyFromBrushDefinition(cBrushDefinition *BrushToCopy) 
{
   TypeComposition     = eCOMPOSITIONTYPE_SHOT;
   BrushType           = BrushToCopy->BrushType;
   PatternType         = BrushToCopy->PatternType;
   GradientOrientation = BrushToCopy->GradientOrientation;
   ColorD              = BrushToCopy->ColorD;
   ColorF              = BrushToCopy->ColorF;
   ColorIntermed       = BrushToCopy->ColorIntermed;
   Intermediate        = BrushToCopy->Intermediate;
   BrushImage          = BrushToCopy->BrushImage;
   MediaObject         = BrushToCopy->MediaObject;
   DeleteMediaObject   = false;
   SoundVolume         = BrushToCopy->SoundVolume;
   Deinterlace         = BrushToCopy->Deinterlace;

   // Image correction part
   params = BrushToCopy->params;
   ImageSpeedWave        = BrushToCopy->ImageSpeedWave;
                           
   Markers               = BrushToCopy->Markers;
}                            

//====================================================================================================================
void cBrushDefinition::AddShotPartToXML(QDomElement *Element) 
{
   if (!MediaObject) 
      return;
   if (MediaObject->is_Videofile()) 
   {
      Element->setAttribute("SoundVolume",QString("%1").arg(SoundVolume,0,'f'));               // Volume of soundtrack (for video only)
      Element->setAttribute("Deinterlace",Deinterlace?"1":"0");                                  // Add a YADIF filter to deinterlace video (on/off) (for video only)
   } 
   else if (MediaObject->is_Gmapsmap()) 
   {
      // Save marker settings
      QDomDocument    DomDocument;
      for (int MarkerNum = 0; MarkerNum < Markers.count(); MarkerNum++)
      {
         QDomElement SubElement = DomDocument.createElement(QString("Marker_%1").arg(MarkerNum));
         SubElement.setAttribute("MarkerColor", Markers[MarkerNum].MarkerColor);
         SubElement.setAttribute("TextColor", Markers[MarkerNum].TextColor);
         SubElement.setAttribute("LineColor", Markers[MarkerNum].LineColor);
         SubElement.setAttribute("Visibility", int(Markers[MarkerNum].Visibility));
         Element->appendChild(SubElement);
      }
   }
}

void cBrushDefinition::AddShotPartToXML(ExXmlStreamWriter &xmlStream)
{
   if (!MediaObject)
      return;
   if (MediaObject->is_Videofile())
   {
      xmlStream.writeAttribute("SoundVolume", QString("%1").arg(SoundVolume, 0, 'f'));               // Volume of soundtrack (for video only)
      xmlStream.writeAttribute("Deinterlace", Deinterlace );                                  // Add a YADIF filter to deinterlace video (on/off) (for video only)
   }
   else if (MediaObject->is_Gmapsmap())
   {
      // Save marker settings
      QDomDocument    DomDocument;
      for (int MarkerNum = 0; MarkerNum < Markers.count(); MarkerNum++)
      {
         //QDomElement SubElement = DomDocument.createElement(QString("Marker_%1").arg(MarkerNum));
         xmlStream.writeStartElement(QString("Marker_%1").arg(MarkerNum));
         xmlStream.writeAttribute("MarkerColor", Markers[MarkerNum].MarkerColor);
         xmlStream.writeAttribute("TextColor", Markers[MarkerNum].TextColor);
         xmlStream.writeAttribute("LineColor", Markers[MarkerNum].LineColor);
         xmlStream.writeAttribute("Visibility", int(Markers[MarkerNum].Visibility));
         xmlStream.writeEndElement();
      }
   }
}

//====================================================================================================================

void cBrushDefinition::SaveToXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel) 
{
   QDomDocument DomDocument;
   QString xmlFragment;
   ExXmlStreamWriter xlmStream(&xmlFragment);
   SaveToXMLex(xlmStream, ElementName, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
   DomDocument.setContent(xmlFragment);
   ParentElement->appendChild(DomDocument.firstChildElement());
   return;
   /*
    QDomElement     Element       = DomDocument.createElement(ElementName);
    QString         BrushFileName = (MediaObject ? MediaObject->FileName() : "");
    OBJECTTYPE      ObjectType    = (MediaObject ? MediaObject->getObjectType() : OBJECTTYPE_UNMANAGED);

    if ((!BrushFileName.isEmpty())&&(!BrushFileName.startsWith(":/img/"))) {
        if (QDir::toNativeSeparators(BrushFileName).startsWith(ClipArtFolder)) {
            BrushFileName="%CLIPARTFOLDER%"+QDir::toNativeSeparators(BrushFileName).mid(ClipArtFolder.length());
            #ifdef Q_OS_WIN
            BrushFileName=BrushFileName.replace("\\","/");  // Force Linux mode separator
            #endif
        } else if (QDir::toNativeSeparators(BrushFileName).startsWith(ModelFolder)) {
            BrushFileName="%MODELFOLDER%"  +QDir::toNativeSeparators(BrushFileName).mid(ModelFolder.length());
            #ifdef Q_OS_WIN
            BrushFileName=BrushFileName.replace("\\","/");  // Force Linux mode separator
            #endif
        } else {

            if (ReplaceList) {
                BrushFileName=ReplaceList->GetDestinationFileName(BrushFileName);
            } else {
                if ((PathForRelativPath!="")&&(BrushFileName!="")) {
                    if (ForceAbsolutPath)
                        BrushFileName=QDir::cleanPath(QDir(QFileInfo(PathForRelativPath).absolutePath()).absoluteFilePath(BrushFileName));
                    else
                        BrushFileName=QDir::cleanPath(QDir(QFileInfo(PathForRelativPath).absolutePath()).relativeFilePath(BrushFileName));
                }
            }
        }
    }
    // Attribut of the object
    Element.setAttribute("ObjectType",ObjectType);
    Element.setAttribute("TypeComposition",TypeComposition);
    Element.setAttribute("BrushType",BrushType);                                                            // 0=No brush !, 1=Solid one color, 2=Pattern, 3=Gradient 2 colors, 4=Gradient 3 colors

    switch (BrushType) {
        case BRUSHTYPE_PATTERN      :
            Element.setAttribute("PatternType",PatternType);                                                // Type of pattern when BrushType is Pattern (Qt::BrushStyle standard)
            Element.setAttribute("ColorD",ColorD);                                                          // First Color
            break;
        case BRUSHTYPE_GRADIENT3    :
            Element.setAttribute("ColorIntermed",ColorIntermed);                                            // Intermediate Color
            Element.setAttribute("Intermediate",Intermediate);                                              // Intermediate position of 2nd color (in %)
        case BRUSHTYPE_GRADIENT2    :
            Element.setAttribute("ColorF",ColorF);                                                          // Last Color
            Element.setAttribute("GradientOrientation",GradientOrientation);                                // 0=Radial, 1=Up-Left, 2=Up, 3=Up-right, 4=Right, 5=bt-right, 6=bottom, 7=bt-Left, 8=Left
        case BRUSHTYPE_SOLID        :
            Element.setAttribute("ColorD",ColorD);                                                          // First Color
            break;
        case BRUSHTYPE_IMAGELIBRARY :
            Element.setAttribute("BrushImage",BrushImage);                                                  // Image name if image from library
            break;
        case BRUSHTYPE_IMAGEDISK :
            if (MediaObject) 
               switch (MediaObject->getObjectType()) {
                case OBJECTTYPE_VIDEOFILE:
                    if (TypeComposition!=eCOMPOSITIONTYPE_SHOT) {                                                            // Global definition only !
                        Element.setAttribute("BrushFileName",BrushFileName);                                                // File name if image from disk
                        Element.setAttribute("StartPos",((cVideoFile*)MediaObject)->StartTime.toString("HH:mm:ss.zzz"));     // Start position (video only)
                        Element.setAttribute("EndPos",((cVideoFile*)MediaObject)->EndTime.toString("HH:mm:ss.zzz"));         // End position (video only)
                    } else AddShotPartToXML(&Element);
                    break;
                case OBJECTTYPE_IMAGEFILE:
                case OBJECTTYPE_IMAGEVECTOR:
                    if (TypeComposition!=eCOMPOSITIONTYPE_SHOT) {                                                            // Global definition only !
                        Element.setAttribute("BrushFileName",BrushFileName);                                                // File name if image from disk
                        Element.setAttribute("ImageOrientation",MediaObject->Orientation());
                    }
                    break;
                case OBJECTTYPE_IMAGECLIPBOARD:
                    if (TypeComposition!=eCOMPOSITIONTYPE_SHOT) MediaObject->SaveToXML(&Element,"",PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,IsModel);
                    break;
                case OBJECTTYPE_GMAPSMAP:
                    if (TypeComposition!=eCOMPOSITIONTYPE_SHOT) MediaObject->SaveToXML(&Element,"",PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,IsModel);
                        else AddShotPartToXML(&Element);
                    break;
                default: break;
            }
            break;
    }

    // Image correction part
    QDomElement CorrectElement=DomDocument.createElement("ImageCorrection");
    CorrectElement.setAttribute("X",                    X());                 // X position (in %) relative to up/left corner
    CorrectElement.setAttribute("Y",                    Y());                 // Y position (in %) relative to up/left corner
    CorrectElement.setAttribute("ZoomFactor",           ZoomFactor());        // Zoom factor (in %)
    if (ImageRotation()!=DEFAULT_IMAGEROTATION)       CorrectElement.setAttribute("ImageRotation",        ImageRotation());     // Image rotation (in °)
    if (Brightness()!=DEFAULT_BRIGHTNESS)             CorrectElement.setAttribute("Brightness",           Brightness());
    if (Contrast()!=DEFAULT_CONTRAST)                 CorrectElement.setAttribute("Contrast",             Contrast());
    if (Gamma()!=DEFAULT_GAMMA)                       CorrectElement.setAttribute("Gamma",                Gamma());
    if (Red()!=DEFAULT_RED)                           CorrectElement.setAttribute("Red",                  Red());
    if (Green()!=DEFAULT_GREEN)                       CorrectElement.setAttribute("Green",                Green());
    if (Blue()!=DEFAULT_BLUE)                         CorrectElement.setAttribute("Blue",                 Blue());
    if (LockGeometry()!=DEFAULT_LOCKGEOMETRY)         CorrectElement.setAttribute("LockGeometry",         LockGeometry()?1:0);
    if (AspectRatio()!=DEFAULT_ASPECTRATIO)           CorrectElement.setAttribute("AspectRatio",          AspectRatio());
    if (FullFilling()!=DEFAULT_FULLFILLING)           CorrectElement.setAttribute("FullFilling",          FullFilling()?1:0);
    if (TypeBlurSharpen()!=DEFAULT_TYPEBLURSHARPEN)   CorrectElement.setAttribute("TypeBlurSharpen",      TypeBlurSharpen());
    if (GaussBlurSharpenSigma()!=DEFAULT_GBSSIGMA)    CorrectElement.setAttribute("GaussBlurSharpenSigma",GaussBlurSharpenSigma());
    if (BlurSharpenRadius()!=DEFAULT_GBSRADIUS)       CorrectElement.setAttribute("BlurSharpenRadius",    BlurSharpenRadius());
    if (QuickBlurSharpenSigma()!=DEFAULT_QBSSIGMA)    CorrectElement.setAttribute("QuickBlurSharpenSigma",QuickBlurSharpenSigma());
    if (Desat()!=DEFAULT_DESAT)                       CorrectElement.setAttribute("Desat",                Desat());
    if (Swirl()!=DEFAULT_SWIRL)                       CorrectElement.setAttribute("Swirl",                Swirl());
    if (Implode()!=DEFAULT_IMPLODE)                   CorrectElement.setAttribute("Implode",              Implode());
    if (OnOffFilters()!=DEFAULT_ONOFFFILTER)          CorrectElement.setAttribute("OnOffFilter",          OnOffFilters());
    if (ImageSpeedWave!=SPEEDWAVE_PROJECTDEFAULT)   
    {
      if( ImageSpeedWave > SPEEDWAVE_SQRT )
      {
         Element.setAttribute("ImageSpeedWave",(int)mapSpeedwave(ImageSpeedWave)); 
         Element.setAttribute("extImageSpeedWave",ImageSpeedWave); 
      }
      else
         CorrectElement.setAttribute("ImageSpeedWave", ImageSpeedWave);
   }
    Element.appendChild(CorrectElement);

    ParentElement->appendChild(Element);
    */
}

void cBrushDefinition::SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel)
{
   xmlStream.writeStartElement(ElementName);
   QString         BrushFileName = (MediaObject ? MediaObject->FileName() : "");
   OBJECTTYPE      ObjectType = (MediaObject ? MediaObject->getObjectType() : OBJECTTYPE_UNMANAGED);

   if ((!BrushFileName.isEmpty()) && (!BrushFileName.startsWith(":/img/")))
   {
      if (QDir::toNativeSeparators(BrushFileName).startsWith(ClipArtFolder))
      {
         BrushFileName = "%CLIPARTFOLDER%" + QDir::toNativeSeparators(BrushFileName).mid(ClipArtFolder.length());
         #ifdef Q_OS_WIN
         BrushFileName = BrushFileName.replace("\\", "/");  // Force Linux mode separator
         #endif
      }
      else if (QDir::toNativeSeparators(BrushFileName).startsWith(ModelFolder))
      {
         BrushFileName = "%MODELFOLDER%" + QDir::toNativeSeparators(BrushFileName).mid(ModelFolder.length());
         #ifdef Q_OS_WIN
         BrushFileName = BrushFileName.replace("\\", "/");  // Force Linux mode separator
         #endif
      }
      else
      {

         if (ReplaceList)
         {
            BrushFileName = ReplaceList->GetDestinationFileName(BrushFileName);
         }
         else
         {
            if ((PathForRelativPath != "") && (BrushFileName != ""))
            {
               if (ForceAbsolutPath)
                  BrushFileName = QDir::cleanPath(QDir(QFileInfo(PathForRelativPath).absolutePath()).absoluteFilePath(BrushFileName));
               else
                  BrushFileName = QDir::cleanPath(QDir(QFileInfo(PathForRelativPath).absolutePath()).relativeFilePath(BrushFileName));
            }
         }
      }
   }
   // Attribut of the object
   xmlStream.writeAttribute("ObjectType", ObjectType);
   xmlStream.writeAttribute("TypeComposition", TypeComposition);
   xmlStream.writeAttribute("BrushType", BrushType);                                                            // 0=No brush !, 1=Solid one color, 2=Pattern, 3=Gradient 2 colors, 4=Gradient 3 colors

   switch (BrushType)
   {
      case BRUSHTYPE_PATTERN:
         xmlStream.writeAttribute("PatternType", PatternType);                                                // Type of pattern when BrushType is Pattern (Qt::BrushStyle standard)
         xmlStream.writeAttribute("ColorD", ColorD);                                                          // First Color
         break;
      case BRUSHTYPE_GRADIENT3:
         xmlStream.writeAttribute("ColorIntermed", ColorIntermed);                                            // Intermediate Color
         xmlStream.writeAttribute("Intermediate", Intermediate);                                              // Intermediate position of 2nd color (in %)
      case BRUSHTYPE_GRADIENT2:
         xmlStream.writeAttribute("ColorF", ColorF);                                                          // Last Color
         xmlStream.writeAttribute("GradientOrientation", GradientOrientation);                                // 0=Radial, 1=Up-Left, 2=Up, 3=Up-right, 4=Right, 5=bt-right, 6=bottom, 7=bt-Left, 8=Left
      case BRUSHTYPE_SOLID:
         xmlStream.writeAttribute("ColorD", ColorD);                                                          // First Color
         break;
      case BRUSHTYPE_IMAGELIBRARY:
         xmlStream.writeAttribute("BrushImage", BrushImage);                                                  // Image name if image from library
         break;
      case BRUSHTYPE_IMAGEDISK:
         if (MediaObject)
            switch (MediaObject->getObjectType())
            {
               case OBJECTTYPE_VIDEOFILE:
                  if (TypeComposition != eCOMPOSITIONTYPE_SHOT)
                  {                                                            // Global definition only !
                     xmlStream.writeAttribute("BrushFileName", BrushFileName);                                                // File name if image from disk
                     xmlStream.writeAttribute("StartPos", ((cVideoFile*)MediaObject)->StartTime.toString("HH:mm:ss.zzz"));     // Start position (video only)
                     xmlStream.writeAttribute("EndPos", ((cVideoFile*)MediaObject)->EndTime.toString("HH:mm:ss.zzz"));         // End position (video only)
                  }
                  else 
                     AddShotPartToXML(xmlStream);
                  break;
               case OBJECTTYPE_IMAGEFILE:
               case OBJECTTYPE_IMAGEVECTOR:
                  if (TypeComposition != eCOMPOSITIONTYPE_SHOT)
                  {                                                            // Global definition only !
                     xmlStream.writeAttribute("BrushFileName", BrushFileName);                                                // File name if image from disk
                     xmlStream.writeAttribute("ImageOrientation", MediaObject->Orientation());
                  }
                  break;
               case OBJECTTYPE_IMAGECLIPBOARD:
                  if (TypeComposition != eCOMPOSITIONTYPE_SHOT) 
                     MediaObject->SaveToXMLex(xmlStream, "", PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
                  break;
               case OBJECTTYPE_GMAPSMAP:
                  if (TypeComposition != eCOMPOSITIONTYPE_SHOT) 
                     MediaObject->SaveToXMLex(xmlStream, "", PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
                  else 
                     AddShotPartToXML(xmlStream);
                  break;
               default: break;
            }
         break;
   }

   // Image correction part
   xmlStream.writeStartElement("ImageCorrection");
   xmlStream.writeAttribute("X", X());                 // X position (in %) relative to up/left corner
   xmlStream.writeAttribute("Y", Y());                 // Y position (in %) relative to up/left corner
   xmlStream.writeAttribute("ZoomFactor", ZoomFactor());        // Zoom factor (in %)
   if (ImageRotation() != DEFAULT_IMAGEROTATION)       xmlStream.writeAttribute("ImageRotation", ImageRotation());     // Image rotation (in °)
   if (Brightness() != DEFAULT_BRIGHTNESS)             xmlStream.writeAttribute("Brightness", Brightness());
   if (Contrast() != DEFAULT_CONTRAST)                 xmlStream.writeAttribute("Contrast", Contrast());
   if (Gamma() != DEFAULT_GAMMA)                       xmlStream.writeAttribute("Gamma", Gamma());
   if (Red() != DEFAULT_RED)                           xmlStream.writeAttribute("Red", Red());
   if (Green() != DEFAULT_GREEN)                       xmlStream.writeAttribute("Green", Green());
   if (Blue() != DEFAULT_BLUE)                         xmlStream.writeAttribute("Blue", Blue());
   if (LockGeometry() != DEFAULT_LOCKGEOMETRY)         xmlStream.writeAttribute("LockGeometry", LockGeometry() );
   if (AspectRatio() != DEFAULT_ASPECTRATIO)           xmlStream.writeAttribute("AspectRatio", AspectRatio());
   if (FullFilling() != DEFAULT_FULLFILLING)           xmlStream.writeAttribute("FullFilling", FullFilling() );
   if (TypeBlurSharpen() != DEFAULT_TYPEBLURSHARPEN)   xmlStream.writeAttribute("TypeBlurSharpen", TypeBlurSharpen());
   if (GaussBlurSharpenSigma() != DEFAULT_GBSSIGMA)    xmlStream.writeAttribute("GaussBlurSharpenSigma", GaussBlurSharpenSigma());
   if (BlurSharpenRadius() != DEFAULT_GBSRADIUS)       xmlStream.writeAttribute("BlurSharpenRadius", BlurSharpenRadius());
   if (QuickBlurSharpenSigma() != DEFAULT_QBSSIGMA)    xmlStream.writeAttribute("QuickBlurSharpenSigma", QuickBlurSharpenSigma());
   if (Desat() != DEFAULT_DESAT)                       xmlStream.writeAttribute("Desat", Desat());
   if (Swirl() != DEFAULT_SWIRL)                       xmlStream.writeAttribute("Swirl", Swirl());
   if (Implode() != DEFAULT_IMPLODE)                   xmlStream.writeAttribute("Implode", Implode());
   if (OnOffFilters() != DEFAULT_ONOFFFILTER)          xmlStream.writeAttribute("OnOffFilter", OnOffFilters());
   if (ImageSpeedWave != SPEEDWAVE_PROJECTDEFAULT)
   {
      if (ImageSpeedWave > SPEEDWAVE_SQRT)
      {
         xmlStream.writeAttribute("ImageSpeedWave", (int)mapSpeedwave(ImageSpeedWave));
         xmlStream.writeAttribute("extImageSpeedWave", ImageSpeedWave);
      }
      else
         xmlStream.writeAttribute("ImageSpeedWave", ImageSpeedWave);
   }
   xmlStream.writeEndElement();

   xmlStream.writeEndElement();
}

//====================================================================================================================

bool cBrushDefinition::LoadFromXML(QDomElement *ParentElement,QString ElementName,QString PathForRelativPath,QStringList *AliasList,bool *ModifyFlag,TResKeyList *ResKeyList,bool DuplicateRes) 
{
   //AUTOTIMER(a,"cBrushDefinition::LoadFromXML");
    QString     Extension,BrushFileName;
    bool        IsValide  =false;
    OBJECTTYPE  ObjectType=OBJECTTYPE_UNMANAGED;

    InitDefaultValues();

    if (ModifyFlag) *ModifyFlag=false;
    if ((ParentElement->elementsByTagName(ElementName).length()>0)&&(ParentElement->elementsByTagName(ElementName).item(0).isElement()==true)) {
        QDomElement Element=ParentElement->elementsByTagName(ElementName).item(0).toElement();

        // Attribut of the object
        if (Element.hasAttribute("ObjectType")) ObjectType=(OBJECTTYPE)Element.attribute("ObjectType").toInt();
        TypeComposition =Element.attribute("TypeComposition").toInt();
        BrushType       =Element.attribute("BrushType").toInt();                         // 0=No brush !, 1=Solid one color, 2=Pattern, 3=Gradient 2 colors, 4=Gradient 3 colors
        switch (BrushType) {
            case BRUSHTYPE_PATTERN      :
                PatternType         =Element.attribute("PatternType").toInt();          // Type of pattern when BrushType is Pattern (Qt::BrushStyle standard)
                ColorD              =Element.attribute("ColorD");                       // First Color
                break;
            case BRUSHTYPE_GRADIENT3    :
                ColorIntermed       =Element.attribute("ColorIntermed");                // Intermediate Color
                Intermediate        =GetDoubleValue(Element,"Intermediate");            // Intermediate position of 2nd color (in %)
            case BRUSHTYPE_GRADIENT2    :
                ColorF              =Element.attribute("ColorF");                       // Last Color
                GradientOrientation =Element.attribute("GradientOrientation").toInt();  // 0=Radial, 1=Up-Left, 2=Up, 3=Up-right, 4=Right, 5=bt-right, 6=bottom, 7=bt-Left, 8=Left
            case BRUSHTYPE_SOLID        :
                ColorD=Element.attribute("ColorD");                                    // First Color
                break;
            case BRUSHTYPE_IMAGELIBRARY :
                BrushImage=Element.attribute("BrushImage");  // Image name if image from library
                break;
            case BRUSHTYPE_IMAGEDISK :
                if (TypeComposition!=eCOMPOSITIONTYPE_SHOT) {
                    // File name if image from disk
                    BrushFileName=HTMLConverter()->ToPlainText(Element.attribute("BrushFileName"));
                    if ((!BrushFileName.isEmpty())&&(!BrushFileName.startsWith(":/img/"))) {
                        BrushFileName=BrushFileName.replace("<%CLIPARTFOLDER%>",CAF);   BrushFileName=BrushFileName.replace("%CLIPARTFOLDER%",CAF);
                        BrushFileName=BrushFileName.replace("<%MODELFOLDER%>",MFD);     BrushFileName=BrushFileName.replace("%MODELFOLDER%",MFD);
                        if ((PathForRelativPath!="")&&(BrushFileName!="")) BrushFileName=QDir::cleanPath(QDir(PathForRelativPath).absoluteFilePath(BrushFileName));
                        // Fixes a previous bug in relative path
                        #ifndef Q_OS_WIN
                        if (BrushFileName.startsWith("/..")) {
                            if (BrushFileName.contains("/home/")) BrushFileName=BrushFileName.mid(BrushFileName.indexOf("/home/"));
                            if (BrushFileName.contains("/mnt/"))  BrushFileName=BrushFileName.mid(BrushFileName.indexOf("/mnt/"));
                        }
                        #endif
                        Extension=QFileInfo(BrushFileName).suffix().toLower();
                        if (ObjectType==OBJECTTYPE_UNMANAGED) {
                            if      (ApplicationConfig->AllowVideoExtension.contains(Extension.toLower()))       ObjectType=OBJECTTYPE_VIDEOFILE;
                            else if (ApplicationConfig->AllowImageExtension.contains(Extension.toLower()))       ObjectType=OBJECTTYPE_IMAGEFILE;
                            else if (ApplicationConfig->AllowImageVectorExtension.contains(Extension.toLower())) ObjectType=OBJECTTYPE_IMAGEVECTOR;
                        }
                    }
                    if (MediaObject) {
                        delete MediaObject;
                        MediaObject=NULL;
                    }
                    QString empty_name("");
                    switch (ObjectType) {
                        case OBJECTTYPE_IMAGEFILE:
                            MediaObject=new cImageFile(ApplicationConfig);
                            MediaObject->setOrientation(Element.attribute("ImageOrientation").toInt());
                            IsValide=MediaObject->GetInformationFromFile(BrushFileName,AliasList,ModifyFlag,-1) && MediaObject->GetFullInformationFromFile();
                            break;
                        case OBJECTTYPE_IMAGEVECTOR:
                            MediaObject=new cImageFile(ApplicationConfig);
                            MediaObject->setOrientation(Element.attribute("ImageOrientation").toInt());
                            IsValide=MediaObject->GetInformationFromFile(BrushFileName,AliasList,ModifyFlag,-1) && MediaObject->GetFullInformationFromFile();
                            break;
                        case OBJECTTYPE_IMAGECLIPBOARD:
                            MediaObject=new cImageClipboard(ApplicationConfig);
                            MediaObject->LoadFromXML(&Element,"",PathForRelativPath,AliasList,ModifyFlag,ResKeyList,DuplicateRes);
                            IsValide=MediaObject->GetInformationFromFile(empty_name,NULL,NULL,-1);
                            break;
                        case OBJECTTYPE_VIDEOFILE:
                            MediaObject=new cVideoFile(ApplicationConfig);
                            IsValide=MediaObject->GetInformationFromFile(BrushFileName,AliasList,ModifyFlag,-1) && MediaObject->GetFullInformationFromFile();
                            if (IsValide) {
                                ((cVideoFile*)MediaObject)->StartTime = QTime().fromString(Element.attribute("StartPos"));    // Start position (video only)
                                ((cVideoFile*)MediaObject)->EndTime = QTime().fromString(Element.attribute("EndPos"));      // End position (video only)
                            }
                            break;
                        case OBJECTTYPE_GMAPSMAP:
                            MediaObject=new cGMapsMap(ApplicationConfig);
                            MediaObject->LoadFromXML(&Element,"",PathForRelativPath,AliasList,ModifyFlag,ResKeyList,DuplicateRes);
                            IsValide=MediaObject->GetInformationFromFile(empty_name,NULL,NULL,-1);
                            break;
                        case OBJECTTYPE_UNMANAGED:
                        default:
                            // For compatibility with version prior to 20131127
                            for (int i=0;i<ApplicationConfig->AllowImageExtension.count();i++) if (ApplicationConfig->AllowImageExtension[i]==Extension) {
                                MediaObject=new cImageFile(ApplicationConfig);
                                MediaObject->setOrientation(Element.attribute("ImageOrientation").toInt());
                                IsValide=MediaObject->GetInformationFromFile(BrushFileName,AliasList,ModifyFlag,-1) && MediaObject->GetFullInformationFromFile();
                                break;
                            }
                            if (MediaObject==NULL) for (int i=0;i<ApplicationConfig->AllowImageVectorExtension.count();i++) if (ApplicationConfig->AllowImageVectorExtension[i]==Extension) {
                                MediaObject=new cImageFile(ApplicationConfig);
                                MediaObject->setOrientation(Element.attribute("ImageOrientation").toInt());
                                IsValide=MediaObject->GetInformationFromFile(BrushFileName,AliasList,ModifyFlag,-1) && MediaObject->GetFullInformationFromFile();
                                break;
                            }
                            if (MediaObject==NULL) for (int i=0;i<ApplicationConfig->AllowVideoExtension.count();i++) if (ApplicationConfig->AllowVideoExtension[i]==Extension) {
                                MediaObject=new cVideoFile(ApplicationConfig);
                                IsValide=MediaObject->GetInformationFromFile(BrushFileName,AliasList,ModifyFlag,-1) && MediaObject->GetFullInformationFromFile();
                                break;
                            }
                            break;
                    }
                    if ((!IsValide)&&(MediaObject)) {
                        delete MediaObject;
                        MediaObject=NULL;
                    }
                }
                if ((MediaObject)&&(TypeComposition==eCOMPOSITIONTYPE_SHOT)) {
                    switch (MediaObject->getObjectType()) {
                        case OBJECTTYPE_VIDEOFILE:
                            SoundVolume=GetDoubleValue(Element,"SoundVolume");                                          // Volume of soundtrack (for video only)
                            Deinterlace=Element.attribute("Deinterlace")=="1";                                          // Add a YADIF filter to deinterlace video (on/off) (for video only)
                            break;
                        case OBJECTTYPE_IMAGEFILE:
                        case OBJECTTYPE_IMAGEVECTOR:
                           // Old Image transformation (for compatibility with version prio to 1.5)
                            if ((Element.elementsByTagName("ImageTransformation").length()>0)&&(Element.elementsByTagName("ImageTransformation").item(0).isElement()==true)) {
                               QDomElement SubElement=Element.elementsByTagName("ImageTransformation").item(0).toElement();
                               if (SubElement.hasAttribute("BlurSigma"))   params.GaussBlurSharpenSigma=GetDoubleValue(SubElement,"BlurSigma");
                               if (SubElement.hasAttribute("BlurRadius"))  params.BlurSharpenRadius    =GetDoubleValue(SubElement,"BlurRadius");
                               if (SubElement.hasAttribute("OnOffFilter")) params.onOffFilter          =SubElement.attribute("OnOffFilter").toInt();
                               if (GaussBlurSharpenSigma()!=0)             params.TypeBlurSharpen      =1;
                            }
                            break;
                        default: break;
                    }
                }
                if ((MediaObject)&&(MediaObject->is_Gmapsmap())) {
                    // Load marker settings
                    int MarkerNum=0;
                    Markers.clear();
                    while ((Element.elementsByTagName(QString("Marker_%1").arg(MarkerNum)).length()>0)&&(Element.elementsByTagName(QString("Marker_%1").arg(MarkerNum)).item(0).isElement()==true)) {
                        QDomElement SubElement=Element.elementsByTagName(QString("Marker_%1").arg(MarkerNum)).item(0).toElement();
                        sMarker Marker;
                        if (SubElement.hasAttribute("TextColor"))   Marker.TextColor=SubElement.attribute("TextColor");
                        if (SubElement.hasAttribute("MarkerColor")) Marker.MarkerColor=SubElement.attribute("MarkerColor");
                        if (SubElement.hasAttribute("LineColor"))   Marker.LineColor=SubElement.attribute("LineColor");
                        if (SubElement.hasAttribute("Visibility"))  Marker.Visibility =(sMarker::MARKERVISIBILITY)SubElement.attribute("Visibility").toInt();
                        Markers.append(Marker);
                        MarkerNum++;
                    }
                }
                break;
        }

        // Image correction part
        if ((Element.elementsByTagName("ImageCorrection").length()>0)&&(Element.elementsByTagName("ImageCorrection").item(0).isElement()==true)) {
            QDomElement CorrectElement=Element.elementsByTagName("ImageCorrection").item(0).toElement();

            if (CorrectElement.hasAttribute("X"))                       params.X = GetDoubleValue(CorrectElement, "X");                      // X position (in %) relative to up/left corner
            if (CorrectElement.hasAttribute("Y"))                       params.Y = GetDoubleValue(CorrectElement, "Y");                      // Y position (in %) relative to up/left corner
            if (CorrectElement.hasAttribute("ZoomFactor"))              params.ZoomFactor = GetDoubleValue(CorrectElement, "ZoomFactor");             // Zoom factor (in %)
            if (CorrectElement.hasAttribute("ImageRotation"))           params.ImageRotation = GetDoubleValue(CorrectElement, "ImageRotation");          // Image rotation (in °)
            if (CorrectElement.hasAttribute("Brightness"))              params.Brightness = CorrectElement.attribute("Brightness").toInt();
            if (CorrectElement.hasAttribute("Contrast"))                params.Contrast = CorrectElement.attribute("Contrast").toInt();
            if (CorrectElement.hasAttribute("Gamma"))                   params.Gamma = GetDoubleValue(CorrectElement, "Gamma");
            if (CorrectElement.hasAttribute("Red"))                     params.Red = CorrectElement.attribute("Red").toInt();
            if (CorrectElement.hasAttribute("Green"))                   params.Green = CorrectElement.attribute("Green").toInt();
            if (CorrectElement.hasAttribute("Blue"))                    params.Blue = CorrectElement.attribute("Blue").toInt();
            if (CorrectElement.hasAttribute("AspectRatio"))             params.AspectRatio = GetDoubleValue(CorrectElement, "AspectRatio");
            if (CorrectElement.hasAttribute("FullFilling"))             params.FullFilling = CorrectElement.attribute("FullFilling").toInt() == 1;
            if (CorrectElement.hasAttribute("TypeBlurSharpen"))         params.TypeBlurSharpen = GetDoubleValue(CorrectElement, "TypeBlurSharpen");
            if (CorrectElement.hasAttribute("GaussBlurSharpenSigma"))   params.GaussBlurSharpenSigma = GetDoubleValue(CorrectElement, "GaussBlurSharpenSigma");
            if (CorrectElement.hasAttribute("BlurSharpenRadius"))       params.BlurSharpenRadius = GetDoubleValue(CorrectElement, "BlurSharpenRadius");
            if (CorrectElement.hasAttribute("QuickBlurSharpenSigma"))   params.QuickBlurSharpenSigma = CorrectElement.attribute("QuickBlurSharpenSigma").toInt();
            if (CorrectElement.hasAttribute("Desat"))                   params.Desat = GetDoubleValue(CorrectElement, "Desat");
            if (CorrectElement.hasAttribute("Swirl"))                   params.Swirl = GetDoubleValue(CorrectElement, "Swirl");
            if (CorrectElement.hasAttribute("Implode"))                 params.Implode = GetDoubleValue(CorrectElement, "Implode");
            if (CorrectElement.hasAttribute("OnOffFilter"))             params.onOffFilter = CorrectElement.attribute("OnOffFilter").toInt();
            if (CorrectElement.hasAttribute("extImageSpeedWave"))
               ImageSpeedWave = CorrectElement.attribute("extImageSpeedWave").toInt();
            else if (CorrectElement.hasAttribute("ImageSpeedWave"))
               ImageSpeedWave = CorrectElement.attribute("ImageSpeedWave").toInt();

            // If old ImageGeometry value in project file then compute LockGeometry
            if (CorrectElement.hasAttribute("ImageGeometry"))       params.LockGeometry=(CorrectElement.attribute("ImageGeometry").toInt()!=2);
            else if (CorrectElement.hasAttribute("LockGeometry"))   params.LockGeometry=(CorrectElement.attribute("LockGeometry").toInt()==1); // Else load saved value

        }

        return (BrushType==BRUSHTYPE_IMAGEDISK)?(MediaObject!=NULL):true;
    }
    return false;
}

//====================================================================================================================

int cBrushDefinition::GetImageType() 
{
   int ImageType = IMAGETYPE_UNKNOWN;
   if ((MediaObject) && (MediaObject->is_Videofile()))
   {
      ImageType = IMAGETYPE_VIDEOLANDSCAPE;
      if ((qreal(MediaObject->width()) / qreal(MediaObject->height())) < 1)
         ImageType++;
   }
   else if (MediaObject)
   {
      if ((MediaObject->width() > 1080) && (MediaObject->height() > 1080))
         ImageType = IMAGETYPE_PHOTOLANDSCAPE;
      else
         ImageType = IMAGETYPE_CLIPARTLANDSCAPE;
      if ((qreal(MediaObject->width()) / qreal(MediaObject->height())) < 1)
         ImageType++;
   }
   return ImageType;
}

//====================================================================================================================

void cBrushDefinition::ApplyMaskToImageToWorkspace(QImage *SrcImage,QRectF CurSelRect,int BackgroundForm,int AutoFramingStyle) 
{
   // Create shape mask
   int RowHeight = SrcImage->width();
   QImage Image(RowHeight,RowHeight,QImage::Format_ARGB32_Premultiplied);
   QPainter PainterImg;
   PainterImg.begin(&Image);
   PainterImg.setPen(Qt::NoPen);
   PainterImg.fillRect(QRect(0,0,RowHeight,RowHeight),QBrush(0x555555));
   PainterImg.setBrush(Qt::transparent);
   PainterImg.setCompositionMode(QPainter::CompositionMode_Source);
   QList<QPolygonF> List = ComputePolygon(BackgroundForm, CurSelRect);
   for (int i = 0; i < List.count(); i++) 
      PainterImg.drawPolygon(List.at(i));
   PainterImg.setCompositionMode(QPainter::CompositionMode_SourceOver);
   PainterImg.end();

   // Apply mask to workspace image
   PainterImg.begin(SrcImage);
   PainterImg.setOpacity(0.75);
   PainterImg.drawImage(0,0,Image);
   PainterImg.setOpacity(1);
   PainterImg.end();

   // Add Icon (if wanted)
   if ((AutoFramingStyle >= 0) && (AutoFramingStyle < NBR_AUTOFRAMING)) 
   {
      QImage IconGeoImage;
      switch (AUTOFRAMINGDEF[AutoFramingStyle].GeometryType) 
      {
         case AUTOFRAMING_GEOMETRY_CUSTOM :  
            IconGeoImage = AutoFramingStyle == AUTOFRAMING_CUSTOMUNLOCK?QImage(AUTOFRAMING_ICON_GEOMETRY_UNLOCKED) : QImage(AUTOFRAMING_ICON_GEOMETRY_LOCKED);       break;
         case AUTOFRAMING_GEOMETRY_PROJECT : 
            IconGeoImage = QImage(AUTOFRAMING_ICON_GEOMETRY_PROJECT);                                                 break;
         case AUTOFRAMING_GEOMETRY_IMAGE :   
            IconGeoImage = QImage(AUTOFRAMING_ICON_GEOMETRY_IMAGE);                                                   break;
      }
      QPainter P;
      P.begin(SrcImage);
      P.drawImage(SrcImage->width()-IconGeoImage.width()-2,SrcImage->height()-IconGeoImage.height()-2,IconGeoImage);
      P.end();
   }
}

void cBrushDefinition::ApplyMaskToImageToWorkspace(QImage *SrcImage,int AutoFramingStyle,int BackgroundForm,int WantedSize,qreal maxw,qreal maxh,qreal minw,qreal minh,qreal X,qreal Y,qreal ZoomFactor,qreal AspectRatio,qreal ProjectGeometry) 
{
   QRectF CurSelRect;
   switch (AutoFramingStyle) 
   {
      case AUTOFRAMING_CUSTOMUNLOCK  : CurSelRect = QRectF(WantedSize*X,WantedSize*Y,WantedSize*ZoomFactor-1,WantedSize*ZoomFactor*AspectRatio-1);                   break;
      case AUTOFRAMING_CUSTOMLOCK    : CurSelRect = QRectF(WantedSize*X,WantedSize*Y,WantedSize*ZoomFactor-1,WantedSize*ZoomFactor*AspectRatio-1);                   break;
      case AUTOFRAMING_CUSTOMIMGLOCK : CurSelRect = QRectF(WantedSize*X,WantedSize*Y,WantedSize*ZoomFactor-1,WantedSize*ZoomFactor*(maxh/maxw)-1);                   break;
      case AUTOFRAMING_CUSTOMPRJLOCK : CurSelRect = QRectF(WantedSize*X,WantedSize*Y,WantedSize*ZoomFactor-1,WantedSize*ZoomFactor*ProjectGeometry-1);               break;
      case AUTOFRAMING_FULLMAX       : CurSelRect = QRectF((WantedSize-maxw)/2,(WantedSize-maxh)/2,maxw-1,maxh-1);                                                   break;
      case AUTOFRAMING_FULLMIN       : CurSelRect = QRectF((WantedSize-minw)/2,(WantedSize-minh)/2,minw-1,minh-1);                                                   break;
      case AUTOFRAMING_HEIGHTLEFTMAX : CurSelRect = QRectF((WantedSize-maxw)/2,(WantedSize-maxh)/2,maxh/ProjectGeometry-1,maxh-1);                                   break;
      case AUTOFRAMING_HEIGHTLEFTMIN : CurSelRect = QRectF((WantedSize-minw)/2,(WantedSize-minh)/2,minh/ProjectGeometry-1,minh-1);                                   break;
      case AUTOFRAMING_HEIGHTMIDLEMAX: CurSelRect = QRectF((WantedSize-(maxh/ProjectGeometry))/2,(WantedSize-maxh)/2,maxh/ProjectGeometry-1,maxh-1);                 break;
      case AUTOFRAMING_HEIGHTMIDLEMIN: CurSelRect = QRectF((WantedSize-(minh/ProjectGeometry))/2,(WantedSize-minh)/2,minh/ProjectGeometry-1,minh-1);                 break;
      case AUTOFRAMING_HEIGHTRIGHTMAX: CurSelRect = QRectF(WantedSize-(maxh/ProjectGeometry)-(WantedSize-maxw)/2,(WantedSize-maxh)/2,maxh/ProjectGeometry-1,maxh-1); break;
      case AUTOFRAMING_HEIGHTRIGHTMIN: CurSelRect = QRectF(WantedSize-(minh/ProjectGeometry)-(WantedSize-minw)/2,(WantedSize-minh)/2,minh/ProjectGeometry-1,minh-1); break;
      case AUTOFRAMING_WIDTHTOPMAX   : CurSelRect = QRectF((WantedSize-maxw)/2,(WantedSize-maxh)/2,maxw-1,maxw*ProjectGeometry-1);                                   break;
      case AUTOFRAMING_WIDTHTOPMIN   : CurSelRect = QRectF((WantedSize-minw)/2,(WantedSize-minh)/2,minw-1,minw*ProjectGeometry-1);                                   break;
      case AUTOFRAMING_WIDTHMIDLEMAX : CurSelRect = QRectF((WantedSize-maxw)/2,(WantedSize-(maxw*ProjectGeometry))/2,maxw-1,maxw*ProjectGeometry-1);                 break;
      case AUTOFRAMING_WIDTHMIDLEMIN : CurSelRect = QRectF((WantedSize-minw)/2,(WantedSize-(minw*ProjectGeometry))/2,minw-1,minw*ProjectGeometry-1);                 break;
      case AUTOFRAMING_WIDTHBOTTOMMAX: CurSelRect = QRectF((WantedSize-maxw)/2,WantedSize-(maxw*ProjectGeometry)-(WantedSize-maxh)/2,maxw-1,maxw*ProjectGeometry-1); break;
      case AUTOFRAMING_WIDTHBOTTOMMIN: CurSelRect = QRectF((WantedSize-minw)/2,WantedSize-(minw*ProjectGeometry)-(WantedSize-minh)/2,minw-1,minw*ProjectGeometry-1); break;
      default : return;
   }
   ApplyMaskToImageToWorkspace(SrcImage,CurSelRect,BackgroundForm,AutoFramingStyle);
}

int cBrushDefinition::GetCurrentFramingStyle(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return -1;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return -1;

   int aX = params.X * dmax;
   int aY = params.Y * dmax;
   int aW = dmax * params.ZoomFactor;
   int aH = dmax * params.ZoomFactor * params.AspectRatio;

   if (!params.LockGeometry)                                                                                                                                                                               return AUTOFRAMING_CUSTOMUNLOCK;
   if      ((aX == int((dmax-maxw)/2))                            && (aY == int((dmax-maxh)/2))                               && (aW == int(maxw))                 && (aH == int(maxh)))                   return AUTOFRAMING_FULLMAX;
   else if ((aX == int((dmax-minw)/2))                            && (aY == int((dmax-minh)/2))                               && (aW == int(minw))                 && (aH == int(minh)))                   return AUTOFRAMING_FULLMIN;
   else if ((aX == int((dmax-maxw)/2))                            && (aY == int((dmax-maxh)/2))                               && (aW == int(maxh/ProjectGeometry)) && (aH == int(maxh)))                   return AUTOFRAMING_HEIGHTLEFTMAX;
   else if ((aX == int((dmax-minw)/2))                            && (aY == int((dmax-minh)/2))                               && (aW == int(minh/ProjectGeometry)) && (aH == int(minh)))                   return AUTOFRAMING_HEIGHTLEFTMIN;
   else if ((aX == int((dmax-(maxh/ProjectGeometry))/2))          && (aY == int((dmax-maxh)/2))                               && (aW == int(maxh/ProjectGeometry)) && (aH == int(maxh)))                   return AUTOFRAMING_HEIGHTMIDLEMAX;
   else if ((aX == int((dmax-(minh/ProjectGeometry))/2))          && (aY == int((dmax-minh)/2))                               && (aW == int(minh/ProjectGeometry)) && (aH == int(minh)))                   return AUTOFRAMING_HEIGHTMIDLEMIN;
   else if ((aX == int(dmax-(maxh/ProjectGeometry)-(dmax-maxw)/2))&& (aY == int((dmax-maxh)/2))                               && (aW == int(maxh/ProjectGeometry)) && (aH == int(maxh)))                   return AUTOFRAMING_HEIGHTRIGHTMAX;
   else if ((aX == int(dmax-(minh/ProjectGeometry)-(dmax-minw)/2))&& (aY == int((dmax-minh)/2))                               && (aW == int(minh/ProjectGeometry)) && (aH == int(minh)))                   return AUTOFRAMING_HEIGHTRIGHTMIN;
   else if ((aX == int((dmax-maxw)/2))                            && (aY == int((dmax-maxh)/2))                               && (aW == int(maxw))                 && (aH == int(maxw*ProjectGeometry)))   return AUTOFRAMING_WIDTHTOPMAX;
   else if ((aX == int((dmax-minw)/2))                            && (aY == int((dmax-minh)/2))                               && (aW == int(minw))                 && (aH == int(minw*ProjectGeometry)))   return AUTOFRAMING_WIDTHTOPMIN;
   else if ((aX == int((dmax-maxw)/2))                            && (aY == int((dmax-(maxw*ProjectGeometry))/2))             && (aW == int(maxw))                 && (aH == int(maxw*ProjectGeometry)))   return AUTOFRAMING_WIDTHMIDLEMAX;
   else if ((aX == int((dmax-minw)/2))                            && (aY == int((dmax-(minw*ProjectGeometry))/2))             && (aW == int(minw))                 && (aH == int(minw*ProjectGeometry)))   return AUTOFRAMING_WIDTHMIDLEMIN;
   else if ((aX == int((dmax-maxw)/2))                            && (aY == int(dmax-(maxw*ProjectGeometry)-(dmax-maxh)/2))   && (aW == int(maxw))                 && (aH == int(maxw*ProjectGeometry)))   return AUTOFRAMING_WIDTHBOTTOMMAX;
   else if ((aX == int((dmax-minw)/2))                            && (aY == int(dmax-(minw*ProjectGeometry)-(dmax-minh)/2))   && (aW == int(minw))                 && (aH == int(minw*ProjectGeometry)))   return AUTOFRAMING_WIDTHBOTTOMMIN;
   else if (params.AspectRatio == (maxh/maxw))                                                                                                                                                             return AUTOFRAMING_CUSTOMIMGLOCK;
   else if (params.AspectRatio == ProjectGeometry)                                                                                                                                                         return AUTOFRAMING_CUSTOMPRJLOCK;
   else                                                                                                                                                                                                    return AUTOFRAMING_CUSTOMLOCK;
}

//====================================================================================================================

QImage *cBrushDefinition::ImageToWorkspace(QImage *SrcImage, int WantedSize, qreal &maxw, qreal &maxh, qreal &minw, qreal &minh/*,qreal &x1,qreal &x2,qreal &x3,qreal &x4,qreal &y1,qreal &y2,qreal &y3,qreal &y4*/)
{
   QImage  *RetImage    = NULL;
   QImage  *SourceImage = NULL;
   qreal   Hyp          = sqrt(qreal(SrcImage->width()*SrcImage->width()) + qreal(SrcImage->height()*SrcImage->height()));   // Calc hypothenuse of the image to define full canvas
   qreal   DstX, DstY, DstW, DstH;

   ::CalcWorkspace(qreal(SrcImage->width()) * (WantedSize / Hyp), qreal(SrcImage->height()) * (WantedSize / Hyp), params.ImageRotation, WantedSize, &maxw, &maxh, &minw, &minh);   // calc rectangle before rotation
   //qreal  rx = qreal(SrcImage->width()) * (WantedSize/Hyp) / 2;
   //qreal  ry = qreal(SrcImage->height()) * (WantedSize/Hyp) / 2;

   //qreal  xtab[4],ytab[4];
   //double ir_pi_180 = ImageRotation*PI/180.0;
   //xtab[0] = -rx*cos(ir_pi_180) + ry*sin(ir_pi_180) + WantedSize/2;
   //xtab[1] = +rx*cos(ir_pi_180) + ry*sin(ir_pi_180) + WantedSize/2;
   //xtab[2] = -rx*cos(ir_pi_180) - ry*sin(ir_pi_180) + WantedSize/2;
   //xtab[3] = +rx*cos(ir_pi_180) - ry*sin(ir_pi_180) + WantedSize/2;
   //ytab[0] = -rx*sin(ir_pi_180) + ry*cos(ir_pi_180) + WantedSize/2;
   //ytab[1] = +rx*sin(ir_pi_180) + ry*cos(ir_pi_180) + WantedSize/2;
   //ytab[2] = -rx*sin(ir_pi_180) - ry*cos(ir_pi_180) + WantedSize/2;
   //ytab[3] = +rx*sin(ir_pi_180) - ry*cos(ir_pi_180) + WantedSize/2;
   //                                               
   //// Sort xtab and ytab
   //for (int i = 0; i < 4; i++) 
   //   for (int j = 0; j < 3; j++) 
   //   {
   //      if (xtab[j] > xtab[j+1]) { qreal a = xtab[j+1];  xtab[j+1] = xtab[j];  xtab[j] = a;  }
   //      if (ytab[j] > ytab[j+1]) { qreal a = ytab[j+1];  ytab[j+1] = ytab[j];  ytab[j] = a;  }
   //   }
   //maxw = xtab[3] - xtab[0];   minw = xtab[2] - xtab[1];
   //maxh = ytab[3] - ytab[0];   minh = ytab[2] - ytab[1];

   // Rotate image if needed and create a SourceImage
   if (params.ImageRotation != 0)
   {
      QTransform matrix;
      matrix.rotate(params.ImageRotation, Qt::ZAxis);
      SourceImage = new QImage(SrcImage->transformed(matrix, Qt::SmoothTransformation));

      // If no rotation then SourceImage=SrcImage
   }
   else
      SourceImage = SrcImage;

   // Calc coordinates of the part in the source image
   qreal RealImageW = qreal(SourceImage->width());                  // Get real image widht
   qreal RealImageH = qreal(SourceImage->height());                 // Get real image height

   DstX = ((Hyp - RealImageW) / 2)*(WantedSize / Hyp);
   DstY = ((Hyp - RealImageH) / 2)*(WantedSize / Hyp);
   DstW = RealImageW*(WantedSize / Hyp);
   DstH = RealImageH*(WantedSize / Hyp);

   QImage ToUseImage = SourceImage->scaled(DstW, DstH);
   if (SourceImage != SrcImage)
      delete SourceImage;

   if (ToUseImage.format() != QImage::Format_ARGB32_Premultiplied)
      ToUseImage = ToUseImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

   ToUseImage = ApplyFilter(ToUseImage);

   RetImage = new QImage(WantedSize, WantedSize, QImage::Format_ARGB32_Premultiplied);
   QPainter P;
   P.begin(RetImage);
   P.fillRect(QRectF(0, 0, WantedSize, WantedSize), Transparent);
   P.drawImage(QRectF(DstX, DstY, DstW, DstH), ToUseImage, QRectF(0, 0, DstW, DstH));
   P.end();

   return RetImage;
}

//====================================================================================================================

bool cBrushDefinition::CalcWorkspace(qreal &dmax,qreal &maxw,qreal &maxh,qreal &minw,qreal &minh/*,qreal &x1,qreal &x2,qreal &x3,qreal &x4,qreal &y1,qreal &y2,qreal &y3,qreal &y4*/) 
{
   int ImgWidth  = MediaObject ? MediaObject->width() : 0;
   int ImgHeight = MediaObject ? MediaObject->height() : 0;

   if ((ImgWidth == 0) || (ImgHeight == 0)) 
      return false;

   if ( isVideo() && (ImgWidth == 1920) && (ImgHeight == 1088) && (ApplicationConfig->Crop1088To1080) ) 
      ImgHeight = 1080;

   dmax = sqrt(qreal(ImgWidth*ImgWidth) + qreal(ImgHeight*ImgHeight));   // Calc hypothenuse of the image to define full canvas
   return ::CalcWorkspace(ImgWidth, ImgHeight, params.ImageRotation, dmax, &maxw, &maxh, &minw, &minh);
   //dmax=qRound(dmax);
   //// calc rectangle before rotation
   //qreal  rx = qreal(ImgWidth)/2.0;
   //qreal  ry = qreal(ImgHeight)/2.0;

   ////qreal  xtab[4],ytab[4];
   //double ir_pi_180 = ImageRotation*PI/180.0;
   //qreal dm2 = dmax/2;
   //QList<qreal> xlist, ylist;
   //xlist.append(-rx*cos(ir_pi_180) + ry*sin(ir_pi_180) + dm2);
   //xlist.append(+rx*cos(ir_pi_180) + ry*sin(ir_pi_180) + dm2);
   //xlist.append(-rx*cos(ir_pi_180) - ry*sin(ir_pi_180) + dm2);
   //xlist.append(+rx*cos(ir_pi_180) - ry*sin(ir_pi_180) + dm2);

   //ylist.append(-rx*sin(ir_pi_180) + ry*cos(ir_pi_180) + dm2);  
   //ylist.append(+rx*sin(ir_pi_180) + ry*cos(ir_pi_180) + dm2);
   //ylist.append(-rx*sin(ir_pi_180) - ry*cos(ir_pi_180) + dm2);
   //ylist.append(+rx*sin(ir_pi_180) - ry*cos(ir_pi_180) + dm2);

   //// Sort xtab and ytab
   //qSort(xlist);
   //qSort(ylist);
   ////for (int i = 0; i < 4; i++) 
   ////   for (int j = 0; j < 3; j++) 
   ////   {
   ////      if (xtab[j] > xtab[j+1]) { qreal a = xtab[j+1];  xtab[j+1] = xtab[j];  xtab[j] = a;  }
   ////      if (ytab[j] > ytab[j+1]) { qreal a = ytab[j+1];  ytab[j+1] = ytab[j];  ytab[j] = a;  }
   ////   }
   //maxw = floor(xlist[3] - xlist[0]);   minw = floor(xlist[2] - xlist[1]);
   //maxh = floor(ylist[3] - ylist[0]);   minh = floor(ylist[2] - ylist[1]);
   ////x1 = xtab[0];             y1 = ytab[0];
   ////x2 = xtab[1];             y2 = ytab[1];
   ////x3 = xtab[2];             y3 = ytab[2];
   ////x4 = xtab[3];             y4 = ytab[3];
   ////     
   ////qDebug() << "CalcWorkspace gives " << x1 << " " << y1 << " " << x2 << " " << y2 << " " << x3 << " " << y3 << " " << x4 << " " << y4;
   //return true;
}

//====================================================================================================================

void cBrushDefinition::ApplyAutoFraming(int AutoAdjust,qreal ProjectGeometry) 
{
   switch (AutoAdjust) 
   {
      case AUTOFRAMING_FULLMAX       : s_AdjustWH();                           break;
      case AUTOFRAMING_FULLMIN       : s_AdjustMinWH();                        break;
      case AUTOFRAMING_HEIGHTLEFTMAX : s_AdjustHLeft(ProjectGeometry);         break;
      case AUTOFRAMING_HEIGHTLEFTMIN : s_AdjustMinHLeft(ProjectGeometry);      break;
      case AUTOFRAMING_HEIGHTMIDLEMAX: s_AdjustHMidle(ProjectGeometry);        break;
      case AUTOFRAMING_HEIGHTMIDLEMIN: s_AdjustMinHMidle(ProjectGeometry);     break;
      case AUTOFRAMING_HEIGHTRIGHTMAX: s_AdjustHRight(ProjectGeometry);        break;
      case AUTOFRAMING_HEIGHTRIGHTMIN: s_AdjustMinHRight(ProjectGeometry);     break;
      case AUTOFRAMING_WIDTHTOPMAX   : s_AdjustWTop(ProjectGeometry);          break;
      case AUTOFRAMING_WIDTHTOPMIN   : s_AdjustMinWTop(ProjectGeometry);       break;
      case AUTOFRAMING_WIDTHMIDLEMAX : s_AdjustWMidle(ProjectGeometry);        break;
      case AUTOFRAMING_WIDTHMIDLEMIN : s_AdjustMinWMidle(ProjectGeometry);     break;
      case AUTOFRAMING_WIDTHBOTTOMMAX: s_AdjustWBottom(ProjectGeometry);       break;
      case AUTOFRAMING_WIDTHBOTTOMMIN: s_AdjustMinWBottom(ProjectGeometry);    break;
   }
}

void cBrushDefinition::s_AdjustWTop(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal W = maxw;
   params.X =((dmax - W)/2)/dmax;
   params.Y =((dmax - maxh)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

void cBrushDefinition::s_AdjustMinWTop(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal W = minw;
   params.X = ((dmax - W)/2)/dmax;
   params.Y = ((dmax - minh)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

void cBrushDefinition::s_AdjustWMidle(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal W = maxw;
   qreal H = W * params.AspectRatio;
   params.X = ((dmax - W)/2)/dmax;
   params.Y = ((dmax - H)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

void cBrushDefinition::s_AdjustMinWMidle(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal W = minw;
   qreal H = W * params.AspectRatio;
   params.X = ((dmax - W)/2)/dmax;
   params.Y = ((dmax - H)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

void cBrushDefinition::s_AdjustWBottom(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal W = maxw;
   qreal H = W * params.AspectRatio;
   params.X = ((dmax - W)/2)/dmax;
   params.Y = (dmax - H -(dmax - maxh)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

void cBrushDefinition::s_AdjustMinWBottom(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal W = minw;
   qreal H = W * params.AspectRatio;
   params.X = ((dmax-W)/2)/dmax;
   params.Y = (dmax-H-(dmax-minh)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

//====================================================================================================================

void cBrushDefinition::s_AdjustHLeft(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal H = maxh;
   qreal W = H/params.AspectRatio;
   params.X = ((dmax - maxw)/2)/dmax;
   params.Y = ((dmax - H)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

void cBrushDefinition::s_AdjustMinHLeft(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal H = minh;
   qreal W = H/params.AspectRatio;
   params.X = ((dmax - minw)/2)/dmax;
   params.Y = ((dmax - H)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

void cBrushDefinition::s_AdjustHMidle(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal H = maxh;
   qreal W = H/params.AspectRatio;
   params.X = ((dmax - W)/2)/dmax;
   params.Y = ((dmax - H)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

void cBrushDefinition::s_AdjustMinHMidle(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal H = minh;
   qreal W = H/params.AspectRatio;
   params.X = ((dmax - W)/2)/dmax;
   params.Y = ((dmax - H)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

void cBrushDefinition::s_AdjustHRight(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal H = maxh;
   qreal W = H/params.AspectRatio;
   params.X = (dmax - W - (dmax - maxw)/2)/dmax;
   params.Y = ((dmax - H)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

void cBrushDefinition::s_AdjustMinHRight(qreal ProjectGeometry) 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = ProjectGeometry;
   qreal H = minh;
   qreal W = H/params.AspectRatio;
   params.X = (dmax - W - (dmax - minw)/2)/dmax;
   params.Y = ((dmax - H)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

//====================================================================================================================

void cBrushDefinition::s_AdjustWH() 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = maxh/maxw;
   qreal W = maxw;
   qreal H = W*params.AspectRatio;
   params.X = ((dmax - W)/2)/dmax;
   params.Y = ((dmax - H)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

void cBrushDefinition::s_AdjustMinWH() 
{
   qreal dmax,maxw,maxh,minw,minh/*,x1,x2,x3,x4,y1,y2,y3,y4*/;
   //if (!CalcWorkspace(dmax,maxw,maxh,minw,minh,x1,x2,x3,x4,y1,y2,y3,y4)) return;
   if( !CalcWorkspace(dmax,maxw,maxh,minw,minh) ) 
      return;

   params.LockGeometry = true;
   params.AspectRatio = minh/minw;
   qreal W = minw;
   qreal H = W*params.AspectRatio;
   params.X = ((dmax - W)/2)/dmax;
   params.Y = ((dmax - H)/2)/dmax;
   params.ZoomFactor = W/dmax;
   removeFromCache();
}

//====================================================================================================================

void *cBrushDefinition::GetDiaporamaObject() 
{
   QObject *Object = CompositionObject;
   while ((Object) && (Object->objectName() != "cDiaporamaObject")) 
      Object = Object->parent();
   return Object;
}

void *cBrushDefinition::GetDiaporama() 
{
   cDiaporamaObject *Object = (cDiaporamaObject *)GetDiaporamaObject();
   if (Object) 
      return Object->pDiaporama; 
   return NULL;
}

//====================================================================================================================

void cBrushDefinition::GetRealLocation(void **Location,void **RealLocation) 
{
   *RealLocation = *Location;
   if ((*Location) && (((cLocation *)(*Location))->LocationType == cLocation::PROJECT)) 
   {
      cDiaporama *Diaporama = (cDiaporama *)GetDiaporama();
      if (Diaporama) 
         *RealLocation = Diaporama->ProjectInfo()->Location();
   } 
   else if ((*Location) && (((cLocation *)(*Location))->LocationType == cLocation::CHAPTER)) 
   {
      cDiaporamaObject *DiaporamaObject=(cDiaporamaObject *)GetDiaporamaObject();
      if (DiaporamaObject) 
      {
         cDiaporamaObject *ChapterObject = DiaporamaObject->pDiaporama->GetChapterDefObject(DiaporamaObject);
         if ((ChapterObject)&&(ChapterObject->ChapterLocation)) 
            *RealLocation = ChapterObject->ChapterLocation;
         else 
            if (DiaporamaObject)
               *RealLocation = DiaporamaObject->pDiaporama->ProjectInfo()->Location();
      }
   }
}

//====================================================================================================================

void cBrushDefinition::DrawMarker(QPainter *Painter,QPoint Position,int MarkerNum,sMarker::MARKERVISIBILITY Visibility,QSize MarkerSize,cBrushDefinition::sMarker::MARKERSIZE Size,bool DisplayType) 
{
    if (MarkerNum < 0 || MarkerNum >= Markers.count() || Visibility == sMarker::MARKERHIDE || !MediaObject || !MediaObject->is_Gmapsmap()) 
      return;

    cLocation *Location=((cGMapsMap *)MediaObject)->locations[MarkerNum];
    cLocation *RealLoc =NULL;
    GetRealLocation((void **)&Location,(void **)&RealLoc);
    if (!Location) return;

    cLocation::MARKERCOMPO MarkerCompo = Location->MarkerCompo;
    bool HaveIcon = Visibility == sMarker::MARKERTABLE || MarkerCompo == cLocation::ICONNAMEADDR || MarkerCompo == cLocation::ICONNAME || MarkerCompo == cLocation::ICON;
    bool HaveName = Visibility == sMarker::MARKERTABLE || MarkerCompo == cLocation::ICONNAMEADDR || MarkerCompo == cLocation::ICONNAME || MarkerCompo == cLocation::NAME || MarkerCompo == cLocation::NAMEADDR;
    bool HaveAddr = Visibility == sMarker::MARKERTABLE || MarkerCompo == cLocation::ICONNAMEADDR || MarkerCompo == cLocation::NAMEADDR || MarkerCompo == cLocation::ADDR;

    cLocation::LOCATIONTYPE Type=(Location!=NULL?Location->LocationType:cLocation::FREE);
    QFont                   FontNormal,FontBold;
    QTextOption             OptionText;
    int                     IconSize,Spacer,CornerSize;
    double                  FontSize;
    QString                 Name   =RealLoc!=NULL?RealLoc->Name:QApplication::translate("cBrushDefinition","Error: Project's location no set");
    QString                 Address=RealLoc!=NULL?RealLoc->FriendlyAddress:QApplication::translate("cBrushDefinition","Error: Project's location no set");
    sMarker::MARKERSIZE     sSize=(Visibility==sMarker::MARKERTABLE?sMarker::MEDIUM:Size);

    if ((DisplayType)&&(Type==cLocation::PROJECT)) {
        Name=QApplication::translate("cBrushDefinition","Project's location (%1)").arg(Name);
        Address=QApplication::translate("cBrushDefinition","Project's location (%1)").arg(Address);
    } else if ((DisplayType)&&(Type==cLocation::CHAPTER)) {
        Name   =QApplication::translate("cBrushDefinition","Chapter's location (%1)").arg(Name);
        Address=QApplication::translate("cBrushDefinition","Chapter's location (%1)").arg(Address);
    }

    // Setup FontFactor
    switch (sSize) {
        case cBrushDefinition::sMarker::SMALL:      IconSize=24;    FontSize=90;    Spacer=3;   CornerSize=6;   break;
        case cBrushDefinition::sMarker::MEDIUM:     IconSize=32;    FontSize=120;   Spacer=3;   CornerSize=8;   break;
        case cBrushDefinition::sMarker::LARGE:      IconSize=48;    FontSize=180;   Spacer=4;   CornerSize=12;  break;
        case cBrushDefinition::sMarker::VERYLARGE:
        default:
                                                    IconSize=64;    FontSize=240;   Spacer=4;   CornerSize=16;  break;
    }

    if ((Visibility==sMarker::MARKERTABLE)||(Location->MarkerForm!=cLocation::MARKERFORMBUBLE)) CornerSize=0;

    // Draw icon
    if (HaveIcon) {
        QImage Icon =RealLoc!=NULL?RealLoc->GetThumb(IconSize):QImage();
        if (!Icon.isNull()) Painter->drawImage(Position.x()+Spacer+CornerSize/2,Position.y()+Spacer+CornerSize/2,Icon);
    } else IconSize=0;

    int XText=Spacer*(HaveIcon?2:1)+IconSize+CornerSize/2;

    OptionText=QTextOption(Qt::AlignLeft|Qt::AlignVCenter);     // Setup alignement
    OptionText.setWrapMode(QTextOption::NoWrap);                // Setup word wrap text option

    Painter->setPen(QPen(QColor(Markers[MarkerNum].TextColor)));
    if (HaveAddr) 
    {
        FontNormal = getAppFont(Sans9);
        #ifdef Q_OS_WIN
        FontNormal.setPointSizeF(double(FontSize)/ScaleFontAdjust);                         // Scale font
        #else
        FontNormal.setPointSizeF((double(FontSize)/ScaleFontAdjust)*ScreenFontAdjust);      // Scale font
        #endif
        Painter->setFont(FontNormal);
        Painter->drawText(QRect(Position.x()+XText,Position.y()+MarkerSize.height()-Spacer-CornerSize/2-QFontMetrics(FontNormal).height(),
                                MarkerSize.width()-Spacer-XText,QFontMetrics(FontNormal).height()),Address,OptionText);
    }
    if (HaveName) 
    {
        if (!HaveAddr && HaveIcon) 
           FontSize=FontSize*2;
        FontBold = getAppFont(Sans9Bold);
        #ifdef Q_OS_WIN
        FontBold.setPointSizeF(double(FontSize*1.1)/ScaleFontAdjust);                       // Scale font
        #else
        FontBold.setPointSizeF((double(FontSize*1.1)/ScaleFontAdjust)*ScreenFontAdjust);    // Scale font
        #endif
        Painter->setFont(FontBold);
        int H=(HaveIcon?(HaveAddr?IconSize/2:IconSize):QFontMetrics(FontBold).height());
        Painter->drawText(QRect(Position.x()+XText,Position.y()+Spacer+CornerSize/2,
                                MarkerSize.width()-Spacer-XText,H),Name,OptionText);
    }
}

//=====================================================================================================

void cBrushDefinition::ComputeMarkerSize(void *Loc,QSize MapImageSize) {
    QFont       FontNormal,FontBold;
    int         IconSize,Spacer,NameWidth,AddressWidth,FullSpacer,H,CornerSize;
    double      FontSize;
    cLocation   *Location=(cLocation *)Loc;
    cLocation   *RealLoc =NULL;
    GetRealLocation((void **)&Location,(void **)&RealLoc);
    if (!Location) return;

    bool    HaveIcon=(Location->MarkerCompo==cLocation::ICONNAMEADDR)||(Location->MarkerCompo==cLocation::ICONNAME)||(Location->MarkerCompo==cLocation::ICON);
    bool    HaveName=(Location->MarkerCompo==cLocation::ICONNAMEADDR)||(Location->MarkerCompo==cLocation::ICONNAME)||(Location->MarkerCompo==cLocation::NAME)||(Location->MarkerCompo==cLocation::NAMEADDR);
    bool    HaveAddr=(Location->MarkerCompo==cLocation::ICONNAMEADDR)||(Location->MarkerCompo==cLocation::NAMEADDR)||(Location->MarkerCompo==cLocation::ADDR);
    QString Name   =RealLoc!=NULL?RealLoc->Name:QApplication::translate("cBrushDefinition","Error: Project's location no set");
    QString Address=RealLoc!=NULL?RealLoc->FriendlyAddress:QApplication::translate("cBrushDefinition","Error: Project's location no set");

    // Setup FontFactor
    switch (Location->Size) {
        case cBrushDefinition::sMarker::SMALL:      IconSize=24;    FontSize=90;    Spacer=3;   CornerSize=6;   break;
        case cBrushDefinition::sMarker::MEDIUM:     IconSize=32;    FontSize=120;   Spacer=3;   CornerSize=8;   break;
        case cBrushDefinition::sMarker::LARGE:      IconSize=48;    FontSize=180;   Spacer=4;   CornerSize=12;  break;
        case cBrushDefinition::sMarker::VERYLARGE:
        default:                                    IconSize=64;    FontSize=240;   Spacer=4;   CornerSize=16;  break;
    }
    if (!HaveIcon) IconSize=0;
    if (Location->MarkerForm!=cLocation::MARKERFORMBUBLE) CornerSize=0;

    // Setup fonts
    if (HaveAddr) {
        FontNormal = getAppFont(Sans9);
        #ifdef Q_OS_WIN
        FontNormal.setPointSizeF(double(FontSize)/ScaleFontAdjust);                         // Scale font
        #else
        FontNormal.setPointSizeF((double(FontSize)/ScaleFontAdjust)*ScreenFontAdjust);      // Scale font
        #endif
        QFontMetrics fmn(FontNormal);
        AddressWidth=fmn.boundingRect(QRect(0,0,MapImageSize.width(),MapImageSize.height()),0,Address).width()*1.02;
    } else AddressWidth=0;

    if (HaveName) {
        if (!HaveAddr && HaveIcon) 
           FontSize = FontSize*2;
        FontBold = getAppFont(Sans9Bold);
        #ifdef Q_OS_WIN
        FontBold.setPointSizeF(double(FontSize*1.1)/ScaleFontAdjust);                       // Scale font
        #else
        FontBold.setPointSizeF((double(FontSize*1.1)/ScaleFontAdjust)*ScreenFontAdjust);    // Scale font
        #endif
        QFontMetrics fmb(FontBold);
        NameWidth=fmb.boundingRect(QRect(0,0,MapImageSize.width(),MapImageSize.height()),0,Name).width()*1.02;
    } else NameWidth=0;

    if (HaveIcon)                           H=IconSize+Spacer*2;
        else if (HaveName && HaveAddr)      H=QFontMetrics(FontBold).height()+QFontMetrics(FontNormal).height()+Spacer*3;
        else if (HaveName)                  H=QFontMetrics(FontBold).height()+Spacer*2;
        else                                H=QFontMetrics(FontNormal).height()+Spacer*2;

    if (HaveIcon && (HaveName || HaveAddr)) FullSpacer=IconSize+Spacer*3;
        else                                FullSpacer=IconSize+Spacer*2;

    // Compute MarkerSize
    Location->MarkerSize.setWidth((NameWidth>AddressWidth?NameWidth:AddressWidth)+FullSpacer+CornerSize);
    //if (MarkerSize.width()>(MapImageSize.width()*0.6)) MarkerSize.setWidth(MapImageSize.width()*0.6);     // Not more than 60% of the map width
    Location->MarkerSize.setHeight(H+CornerSize);
}

//====================================================================================================================

void cBrushDefinition::AddMarkerToImage(QImage *DestImage) 
{
    if (!MediaObject || !MediaObject->is_Gmapsmap() || !DestImage) 
      return;

    cGMapsMap *CurrentMap=(cGMapsMap *)MediaObject;
    if (!CurrentMap->IsMapValide) return;

    QSize    DestMapSize=CurrentMap->GetCurrentImageSize();
    QPainter Painter;
    Painter.begin(DestImage);
    if (DestImage->size()!=DestMapSize) Painter.setWindow(0,0,DestMapSize.width(),DestMapSize.height());

    for (int i=0;i<CurrentMap->locations.count();i++) if (Markers[i].Visibility!=cBrushDefinition::sMarker::MARKERHIDE) {
        cLocation *Location = CurrentMap->locations[i];
        cLocation *RealLoc =NULL;
        GetRealLocation((void **)&Location,(void **)&RealLoc);

        if ((Location)&&(RealLoc)) {
            QPoint          MarkerPoint=CurrentMap->GetLocationPoint(i);
            QPoint          MarkerPosition;
            QPoint          MarkerStartLine;
            int             GPSPointSize,CornerSize,PenSize;
            QPen            Pen;
            QPainterPath    MarkerPath,PointPath,LinePath;
            int             MakerLineLen=1;

            // Compute distance
            switch (Location->Distance) {
                case cLocation::MARKERDISTNEAR:    MakerLineLen=DestMapSize.height()/24;  break;
                case cLocation::MARKERDISTNORMAL:  MakerLineLen=DestMapSize.height()/17;  break;
                case cLocation::MARKERDISTFAR:     MakerLineLen=DestMapSize.height()/10;  break;
            }

            // Compute sizes
            switch (Location->Size) {
                case cBrushDefinition::sMarker::SMALL:      GPSPointSize=7;     PenSize=1;    CornerSize=6;   break;
                case cBrushDefinition::sMarker::MEDIUM:     GPSPointSize=10;    PenSize=1;    CornerSize=8;   break;
                case cBrushDefinition::sMarker::LARGE:      GPSPointSize=13;    PenSize=2;    CornerSize=12;  break;
                case cBrushDefinition::sMarker::VERYLARGE:
                default:                                    GPSPointSize=16;    PenSize=2;    CornerSize=16;  break;
            }

            // Compute orientation of marker
            if (MarkerPoint.y()>Location->MarkerSize.height()+MakerLineLen) {
                MarkerPosition=QPoint(MarkerPoint.x()-(Location->MarkerSize.width()/2),MarkerPoint.y()-MakerLineLen-Location->MarkerSize.height());
                MarkerStartLine.setY(MarkerPosition.y()+Location->MarkerSize.height()-1);
            } else {
                MarkerPosition=QPoint(MarkerPoint.x()-(Location->MarkerSize.width()/2),MarkerPoint.y()+MakerLineLen);
                MarkerStartLine.setY(MarkerPosition.y()+1);
            }
            if (MarkerPosition.x()<0) MarkerPosition.setX(0);
            if (MarkerPosition.x()+Location->MarkerSize.width()>DestMapSize.width()) MarkerPosition.setX(DestMapSize.width()-Location->MarkerSize.width());
            MarkerStartLine.setX(MarkerPosition.x()+Location->MarkerSize.width()/2);

            // Prepare GPS point path
            switch (Location->MarkerPointForm) {
                case cLocation::MARKERPOINTPOINT:   PointPath.moveTo(MarkerPoint);                                                                              break;
                case cLocation::MARKERPOINTCIRCLE:  PointPath.addEllipse(MarkerPoint,GPSPointSize/2,GPSPointSize/2);                                            break;
                case cLocation::MARKERPOINTRECT:    PointPath.addRect(MarkerPoint.x()-GPSPointSize/2,MarkerPoint.y()-GPSPointSize/2,GPSPointSize,GPSPointSize); break;
            }

            // Prepare line path and marker path
            int BubbleSize=GPSPointSize*2;
            if (BubbleSize>(Location->MarkerSize.width()/4)) BubbleSize=Location->MarkerSize.width()/4;
            switch (Location->MarkerForm) {
                case cLocation::MARKERFORMRECT:
                    LinePath.moveTo(QPoint(MarkerPoint.x()-PenSize*2,MarkerPoint.y()));
                    LinePath.lineTo(QPoint(MarkerStartLine.x()-PenSize*2,MarkerStartLine.y()));
                    LinePath.lineTo(QPoint(MarkerStartLine.x()+PenSize*2,MarkerStartLine.y()));
                    LinePath.lineTo(QPoint(MarkerPoint.x()+PenSize*2,MarkerPoint.y()));
                    LinePath.lineTo(QPoint(MarkerPoint.x()-PenSize*2,MarkerPoint.y()));
                    MarkerPath.addRect(MarkerPosition.x(),MarkerPosition.y(),Location->MarkerSize.width(),Location->MarkerSize.height());
                    break;
                case cLocation::MARKERFORMBUBLE:
                    LinePath.moveTo(MarkerPoint);
                    LinePath.lineTo(QPoint(MarkerStartLine.x()-BubbleSize,MarkerStartLine.y()));
                    LinePath.lineTo(QPoint(MarkerStartLine.x()+BubbleSize,MarkerStartLine.y()));
                    LinePath.lineTo(MarkerPoint);
                    MarkerPath.addRoundedRect(MarkerPosition.x(),MarkerPosition.y(),Location->MarkerSize.width(),Location->MarkerSize.height(),CornerSize,CornerSize);
                    break;
            }
            // Make union
            MarkerPath=MarkerPath.united(LinePath);
            MarkerPath=MarkerPath.united(PointPath);

            Painter.save();
            Painter.setBrush(QBrush(QColor(Markers[i].MarkerColor),Qt::SolidPattern));
            Pen.setColor(QColor(Markers[i].LineColor));
            Pen.setStyle(Qt::SolidLine);
            Pen.setWidth(PenSize);
            Painter.setPen(Pen);
            if (Markers[i].Visibility==cBrushDefinition::sMarker::MARKERSHADE) Painter.setOpacity(0.5);
            Painter.drawPath(MarkerPath);
            PenSize*=2;
            Painter.setClipRect(MarkerPosition.x()+PenSize,MarkerPosition.y()+PenSize,Location->MarkerSize.width()-2*PenSize,Location->MarkerSize.height()-2*PenSize);
            DrawMarker(&Painter,MarkerPosition,i,Markers[i].Visibility,Location->MarkerSize,Location->Size);
            Painter.restore();
        }
    }
    Painter.end();
}

bool cBrushDefinition::isVideo()
{
   if (MediaObject && MediaObject->is_Videofile() )
      return true;
   return false;      
}

bool cBrushDefinition::isImageFile()
{
   if (MediaObject && MediaObject->is_Imagefile() )
      return true;
   return false;      
}

bool cBrushDefinition::isVectorImageFile()
{
   if (MediaObject && MediaObject->is_Imagevector())
      return true;
   return false;
}

bool cBrushDefinition::isGMapsMap()
{
   if (MediaObject && MediaObject->is_Gmapsmap() )
      return true;
   return false;      
}

bool cBrushDefinition::isStatic(cBrushDefinition* prevBrush)
{
   if( isVideo() )
      return false;
   if( prevBrush == NULL )
      return true;
   if( params != prevBrush->params )
      return false;

   QSizeF mySize = CompositionObject->size();
   QSizeF prevSize = prevBrush->CompositionObject->size();
   if( mySize.height() > prevSize.height() || mySize.width() > prevSize.width() )
      return false;
   return true;
}

bool cBrushDefinition::isSlidingBrush(cBrushDefinition* prevBrush)
{
   if( !isImageFile() )
      return false;
   if( prevBrush == NULL )
      return false;
   slidingBrush = false;
   // implementation of sliding brush is faulty! needs more testing
   if( params != prevBrush->params )
   {
      slidingBrush = params.isSlidingBrush(prevBrush->params);
   }
   return slidingBrush;
}

void cBrushDefinition::removeFromCache()
{
   ApplicationConfig->ImagesCache.RemoveImageObject(this);
   pushedImage = false;
   //pushedRenderImage = false;
}

bool cBrushDefinition::assignChanges(const cBrushDefinition& rhs, const cBrushDefinition& savedBrush)
{
#define SUBAPPLYX(FIELD)\
   if (rhs.FIELD() != savedBrush.FIELD() ) \
      set##FIELD(rhs.FIELD());
   if (!cBaseBrushDefinition::assignChanges(rhs, savedBrush))
      return false;
   SUBAPPLYX(X)                                                  
   SUBAPPLYX(Y)                                                  
   SUBAPPLYX(ImageRotation)                                      
   SUBAPPLYX(ZoomFactor)                                         
   SUBAPPLYX(Brightness)                                         
   SUBAPPLYX(Contrast)                                           
   SUBAPPLYX(Gamma)                                              
   SUBAPPLYX(Red)                                                
   SUBAPPLYX(Green)                                              
   SUBAPPLYX(Blue)                                               
   SUBAPPLYX(LockGeometry)                                       
   SUBAPPLYX(AspectRatio)                                        
   SUBAPPLYX(FullFilling)                                        
   SUBAPPLYX(GaussBlurSharpenSigma)                              
   SUBAPPLYX(BlurSharpenRadius)                                  
   SUBAPPLYX(QuickBlurSharpenSigma)                              
   SUBAPPLYX(TypeBlurSharpen)                                    
   SUBAPPLYX(Desat)                                              
   SUBAPPLYX(Swirl)                                              
   SUBAPPLYX(Implode)                                            
   SUBAPPLYX(OnOffFilters)                                       

#define SUBAPPLY(FIELD)\
   if (rhs.FIELD != savedBrush.FIELD ) \
      FIELD = rhs.FIELD;

      
   SUBAPPLY(ImageSpeedWave)                                                                                                
   SUBAPPLY(MediaObject)                                                                                                  
   SUBAPPLY(SoundVolume)                                                                                                  
   SUBAPPLY(Deinterlace)                                                                                                  
   for (int MarkNum = 0; MarkNum < rhs.Markers.count(); MarkNum++)
   {
      if (MarkNum < savedBrush.Markers.count())
      {
         SUBAPPLY(Markers[MarkNum].MarkerColor)                                                                          
         SUBAPPLY(Markers[MarkNum].TextColor)                                                                            
         SUBAPPLY(Markers[MarkNum].LineColor)                                                                            
         SUBAPPLY(Markers[MarkNum].Visibility)                                                                           
      }
      else
      {
         Markers[MarkNum].MarkerColor = rhs.Markers[MarkNum].MarkerColor;  
         Markers[MarkNum].TextColor = rhs.Markers[MarkNum].TextColor;      
         Markers[MarkNum].LineColor = rhs.Markers[MarkNum].LineColor;      
         Markers[MarkNum].Visibility = rhs.Markers[MarkNum].Visibility;    
      }                                                                                                                   
   }                   
   return true;
}

QRectF cBrushDefinition::appliedRect(double size)
{
   return QRectF(params.X * size, params.Y * size, params.ZoomFactor * size, params.ZoomFactor * params.AspectRatio * size);
}

void cBrushDefinition::preLoad(bool bPreview)
{
   if( MediaObject )
   {
      if( MediaObject->is_Imagefile() || MediaObject->is_Imagevector() )
      {
         QImage *img = MediaObject->ImageAt(bPreview);
         if( img )
         {
            delete img;
            cLuLoImageCacheObject *ImageObject = ApplicationConfig->ImagesCache.FindObject(MediaObject->RessourceKey(),MediaObject->FileKey(),MediaObject->modificationTime(),MediaObject->Orientation(), (!bPreview || ApplicationConfig->Smoothing),true);
            if( ImageObject )
               ImageObject->lastAccess = QTime(23,59);
         }
      }
      else if( MediaObject->is_Videofile() || MediaObject->is_Musicfile() )
      {
         ((cVideoFile *)MediaObject)->OpenCodecAndFile();
      }
   }
}

//QImage cBrushDefinition::GetImageDiskBrushImage(QRectF Rect, bool PreviewMode, int64_t Position, cSoundBlockList *SoundTrackMontage, double PctDone, cBrushDefinition *PreviousBrush);
//QImage cBrushDefinition::GetImageDiskBrushVideo(QRectF Rect, bool PreviewMode, int64_t Position, cSoundBlockList *SoundTrackMontage, double PctDone, cBrushDefinition *PreviousBrush);
//QImage cBrushDefinition::GetImageDiskBrushGMaps(QRectF Rect, bool PreviewMode, int64_t Position, cSoundBlockList *SoundTrackMontage, double PctDone, cBrushDefinition *PreviousBrush);
