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

// Specific inclusions
#include "_Diaporama.h"
#include "_Variables.h"
#include "cLocation.h"
#include "CustomCtrl/_QCustomDialog.h"
#include "DlgGMapsLocation/DlgGMapsGeneration.h"
#include "MainWindow/mainwindow.h"
#include "jhlabs_filter/image/LightFilter.h"

#include <QAbstractTextDocumentLayout>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QStringLiteral>

bool drawSmooth(QPainter *painter, QRectF outRec, QRectF orgRect, QImage *image);
int64_t TotalLoadSources = 0, TotalAssembly = 0, TotalLoadSound = 0;
// Composition parameters
#define DEFAULT_ROTATEZAXIS          0
#define DEFAULT_ROTATEXAXIS          0
#define DEFAULT_ROTATEYAXIS          0
#define DEFAULT_TXTZOOMLEVEL         100
#define DEFAULT_TXTSCROLLX           0
#define DEFAULT_TXTSCROLLY           0
#define DEFAULT_BLOCKANIMTYPE        BLOCKANIMTYPE_NONE
#define DEFAULT_TURNZAXIS            0
#define DEFAULT_TURNXAXIS            0
#define DEFAULT_TURNYAXIS            0
#define DEFAULT_DISSOLVE             eAPPEAR
#define DEFAULT_BACKGROUNDFORM       1
#define DEFAULT_SHAPE_PENSTYLE       Qt::SolidLine
#define DEFAULT_SHAPE_SHADOW         0
#define DEFAULT_SHAPE_SHADOWDISTANCE 5
#define DEFAULT_TRANSITIONDURATION   1000
#define DEFAULT_MUSICTYPE            false
#define DEFAULT_MUSICPAUSE           false
#define DEFAULT_MUSICREDUCEVOLUME    false
#define DEFAULT_MUSICREDUCEFACTOR    0.2
#define DEFAULT_STARTNEWCHAPTER      false
#define DEFAULT_CHAPTEROVERRIDE      false


#define noTEXTPARTTIMING
#define noLOG_DRAWCOMPO

#define qsl_TypeComposition QStringLiteral("TypeComposition")
#define qsl_IsUnderline QStringLiteral("IsUnderline")
double DoubleRound (double value, short numberOfPrecisions)
{
   int p = pow(10.0, numberOfPrecisions);
   double rvalue = (int)(value * p + 0.5) / (double)p;
   return rvalue;
}


//====================================================================================================================

cCompositionObjectContext::cCompositionObjectContext(int ObjectNumber,bool PreviewMode,bool IsCurrentObject,cDiaporamaObjectInfo *Info, QSizeF s,
                                                     cDiaporamaShot *CurShot,cDiaporamaShot *PreviousShot,cSoundBlockList *SoundTrackMontage,bool AddStartPos,
                                                     int64_t ShotDuration) 
   : QObject(0), ZRotationInfo(Qt::ZAxis)
{
    setObjectName("cCompositionObjectContext");

    PrevCompoObject   = NULL;
    size = s;
    this->PreviewMode = PreviewMode;
    UseBrushCache     = false;
    EnableAnimation   = true;
    this->ShotDuration      = ShotDuration;
    this->IsCurrentObject   = IsCurrentObject;
    this->CurShot           = CurShot;
    this->PreviousShot      = PreviousShot;
    this->Info              = Info;
    this->ObjectNumber      = ObjectNumber;
    this->SoundTrackMontage = SoundTrackMontage;
    this->AddStartPos       = AddStartPos;
    //maskBackgroundForm = -1;
    maskRotateZAxis = 0; 
}

cCompositionObjectContext::~cCompositionObjectContext()
{
}

//====================================================================================================================
// Compute
//  BlockPctDone, ImagePctDone
//  VideoPosition, StartPosToAdd, PrevCompoObject
//====================================================================================================================
void cCompositionObjectContext::Compute() 
{
   #ifdef LOG_DRAWCOMPO
   QTime t;
   t.start();
   #endif
   cCompositionObject *Object = CurShot->ShotComposition.compositionObjects[ObjectNumber];
   cDiaporamaObjectInfo::ObjectInfo *info = IsCurrentObject ? &Info->Current : &Info->Transit;
   cDiaporamaObject *CurObject = info->Object;
   int CurTimePosition = info->Object_InObjectTime;
   int BlockSpeedWave  = Object->SpeedWave();
   int ImageSpeedWave  = Object->BackgroundBrush->ImageSpeedWave;

   if (BlockSpeedWave == SPEEDWAVE_PROJECTDEFAULT) 
      BlockSpeedWave = CurShot->pDiaporamaObject->pDiaporama->BlockAnimSpeedWave;
   if (ImageSpeedWave == SPEEDWAVE_PROJECTDEFAULT) 
      ImageSpeedWave = CurShot->pDiaporamaObject->pDiaporama->ImageAnimSpeedWave;

   BlockPctDone = ComputePCT(BlockSpeedWave, info->Object_PCTDone);
   ImagePctDone = ComputePCT(ImageSpeedWave, info->Object_PCTDone);

   // Get PrevCompoObject to enable animation from previous shot
   if (PreviousShot) 
      PrevCompoObject = PreviousShot->ShotComposition.getCompositionObject(Object->index());

   // Calc StartPosToAdd for video depending on AddStartPos
   cVideoFile *Video = Object->isVideo() ?  (cVideoFile *)Object->BackgroundBrush->MediaObject : NULL;
   StartPosToAdd = ((AddStartPos) && (Video) ? QTime(0,0,0,0).msecsTo(Video->StartTime) : 0);
   VideoPosition = 0;

   if (Video) 
   {
      // Calc VideoPosition depending on video set to pause (visible=off) in previous shot
      int ThePosition = 0;
      int TheShot = 0;
      while ((TheShot < CurObject->shotList.count()) && (ThePosition + CurObject->shotList[TheShot]->StaticDuration <= CurTimePosition))
      {
         cCompositionObject* PrevVideoObject = CurObject->shotList[TheShot]->ShotComposition.getCompositionObject(Object->index());
         if( PrevVideoObject && PrevVideoObject->isVisible() )
            VideoPosition += CurObject->shotList[TheShot]->StaticDuration;
         ThePosition += CurObject->shotList[TheShot]->StaticDuration;
         TheShot++;
      }
      VideoPosition += (CurTimePosition - ThePosition);
   } 
   else 
      VideoPosition = CurTimePosition;

    // PreparedBrush->W and PreparedBrush->H = 0 when producing sound track in render process
    if (!Object->isVisible() || size.isEmpty() || Object->size().isEmpty() ) 
      return;

   // Define values depending on BlockPctDone and PrevCompoObject
   curRect = Object->getRect();
   curRotateZAxis  = Object->zAngle() + (EnableAnimation && (Object->AnimType() == BLOCKANIMTYPE_MULTIPLETURN) ? 360*Object->ZTurns() : 0);
   curRotateXAxis  = Object->xAngle() + (EnableAnimation && (Object->AnimType() == BLOCKANIMTYPE_MULTIPLETURN) ? 360*Object->XTurns() : 0);
   curRotateYAxis  = Object->yAngle() + (EnableAnimation && (Object->AnimType() == BLOCKANIMTYPE_MULTIPLETURN) ? 360*Object->YTurns() : 0);
   curTxtZoomLevel = Object->tShotParams.TxtZoomLevel;
   curTxtScrollX   = Object->tShotParams.TxtScrollX;
   curTxtScrollY   = Object->tShotParams.TxtScrollY;

   cCompositionObjectMask *mask = Object->getMask();
   if( mask != 0 && Object->usingMask() )
   {
      maskRect = mask->getRect();
      //maskBackgroundForm = mask->getBackgroundForm();
      maskRotateZAxis = mask->zAngle(); 
   }
   if (PrevCompoObject) 
   {
      //QPainterPath ppath;
      //QRectF rf(qMin(PrevCompoObject->getRect().left(), TheRect.left()), qMin(PrevCompoObject->getRect().top(), TheRect.top()), qMax(PrevCompoObject->getRect().left(), TheRect.left()), qMax(PrevCompoObject->getRect().top(), TheRect.top()));
      ////ppath.moveTo(PrevCompoObject->getRect().topLeft());
      ////ppath.lineTo(TheRect.topLeft());
      //ppath.addEllipse(rf);
      //QPointF pPoint = ppath.pointAtPercent(BlockPctDone);


      curRect = animValue(PrevCompoObject->getRect(), curRect, BlockPctDone);
      //qDebug() << "pos: " << TheRect.topLeft() << " by path: " << pPoint;
      //TheRect.moveTo(pPoint);
      curRotateZAxis = animDiffValue(PrevCompoObject->zAngle(), curRotateZAxis, BlockPctDone);
      curRotateXAxis = animDiffValue(PrevCompoObject->xAngle(), curRotateXAxis, BlockPctDone);
      curRotateYAxis = animDiffValue(PrevCompoObject->yAngle(), curRotateYAxis, BlockPctDone);
      curTxtZoomLevel = animDiffValue(PrevCompoObject->tShotParams.TxtZoomLevel,curTxtZoomLevel, BlockPctDone);
      curTxtScrollX = animDiffValue(PrevCompoObject->tShotParams.TxtScrollX, curTxtScrollX, BlockPctDone);
      curTxtScrollY = animDiffValue(PrevCompoObject->tShotParams.TxtScrollY, curTxtScrollY, BlockPctDone);
      cCompositionObjectMask *prevMask = PrevCompoObject->getMask();
      if( mask && prevMask )
      {
         maskRect = animValue(prevMask->getRect(), maskRect, BlockPctDone); 
         maskRotateZAxis = animDiffValue(prevMask->zAngle(), maskRotateZAxis, BlockPctDone);
      }
   } 
   else 
   {
      if (EnableAnimation && (Object->AnimType() == BLOCKANIMTYPE_MULTIPLETURN)) 
      {
         curRotateZAxis = animDiffValue(0, Object->zAngle() + 360 * Object->ZTurns(), BlockPctDone);
         curRotateXAxis = animDiffValue(0, Object->xAngle() + 360 * Object->XTurns(), BlockPctDone);
         curRotateYAxis = animDiffValue(0, Object->yAngle() + 360 * Object->YTurns(), BlockPctDone);
      }
   }

   //**********************************************************************************
   orgRect.setX(curRect.x() * size.width() );
   orgRect.setY(curRect.y() * size.height() );
   orgRect.setWidth(curRect.width() * size.width() );
   orgRect.setHeight(curRect.height() * size.height() );

   //qDebug() << "Rect for " << BlockPctDone << " is " << TheRect << " giving pixelrect " << orgRect;
   //**********************************************************************************

   if ((orgRect.width() > 0) && (orgRect.height() > 0)) 
   {
      //***********************************************************************************
      // Compute shape
      //***********************************************************************************
#if 0
      orgRect.moveLeft( floor(orgRect.x()) );
      orgRect.moveTop( floor(orgRect.y()) );
      orgRect.setWidth( floor( (orgRect.width()/2)*2) );
      orgRect.setHeight( floor( (orgRect.height()/2)*2) );
#endif
      //**********************************************************************************
      // Opacity and dissolve annimation
      //**********************************************************************************
      DestOpacity = Object->destOpacity();
      if (EnableAnimation) 
      {
         if (Object->AnimType() == BLOCKANIMTYPE_DISSOLVE) 
         {
            double BlinkNumber = 0;
            switch (Object->DissolveType()) 
            {
               case eAPPEAR        : DestOpacity = DestOpacity * BlockPctDone;     break;
               case eDISAPPEAR     : DestOpacity = DestOpacity * (1-BlockPctDone); break;
               case eBLINK_SLOW    : BlinkNumber = 0.25;                           break;
               case eBLINK_MEDIUM  : BlinkNumber = 0.5;                            break;
               case eBLINK_FAST    : BlinkNumber = 1;                              break;
               case eBLINK_VERYFAST: BlinkNumber = 2;                              break;
            }
            if (BlinkNumber != 0) 
            {
               BlinkNumber = BlinkNumber * ShotDuration;
               if (int(BlinkNumber / 1000) != (BlinkNumber/1000)) 
                  BlinkNumber = int(BlinkNumber/1000)+1; 
               else 
                  BlinkNumber = int(BlinkNumber/1000); // Adjust to upper 1000
               double FullPct = BlockPctDone*BlinkNumber*100;
               FullPct = int(FullPct) - int(FullPct/100)*100;
               FullPct = (FullPct/100)*2;
               if (FullPct < 1)
                  DestOpacity = DestOpacity * (1-FullPct);
               else
                  DestOpacity = DestOpacity * (FullPct-1);
            }
         }
      }

      //***********************************************************************************
      // Compute shape
      //***********************************************************************************
      PolygonList = ComputePolygon(Object->BackgroundForm,orgRect);
      //qDebug() << "(Compute) calc poly for " << orgRect << "gives " << PolygonList.at(0) << " with " << PolygonList.at(0).boundingRect();
      ShapeRect   = PolygonToRectF(PolygonList);

      //***********************************************************************************
      // Prepare Transform Matrix
      //***********************************************************************************
      //if ( (Object->TextClipArtName == "") 
      //     && (!((Object->BackgroundBrush->BrushType == BRUSHTYPE_IMAGEDISK) && (Object->BackgroundBrush->MediaObject) && (Object->BackgroundBrush->MediaObject->ObjectType == OBJECTTYPE_IMAGEVECTOR)))
      //     && ( Object->BackgroundBrush->BrushType != BRUSHTYPE_NOBRUSH || Object->PenSize != 0 )
      //     && ( Object->BackgroundBrush->BrushType != BRUSHTYPE_NOBRUSH )
      //    ) 
      //   NeedPreparedBrush = true;
    }
    #ifdef LOG_DRAWCOMPO
    qDebug() << "cCompositionObjectContext::Compute() takes " << t.elapsed() <<" mSec";
    #endif
}

//*********************************************************************************************************************************************
// Base object for composition definition
//*********************************************************************************************************************************************

cCompositionObject::cCompositionObject(eTypeComposition TheTypeComposition,int TheIndexKey,cApplicationConfig *TheApplicationConfig,QObject *Parent) 
   : QObject(Parent), ZRotationInfo(Qt::ZAxis)
{
   setObjectName("cCompositionObject");

   // Attribut of the text object
   ApplicationConfig = TheApplicationConfig;
   TypeComposition   = TheTypeComposition;
   IndexKey          = TheIndexKey;
   BackgroundBrush   = new cBrushDefinition(this,ApplicationConfig);  // ERROR : BackgroundList is global !
   InitDefaultValues();
}

//*********************************************************************************************************************************************

void cCompositionObject::InitDefaultValues() 
{
   IsVisible = true;
   BlockInheritance = true;

   IsFullScreen = false;
   wsRect.setRect(0.25, 0.25, 0.5, 0.5);

   RotateZAxis = DEFAULT_ROTATEZAXIS;  // Rotation from Z axis
   RotateXAxis = DEFAULT_ROTATEXAXIS;  // Rotation from X axis
   RotateYAxis = DEFAULT_ROTATEYAXIS;  // Rotation from Y axis

   // Text part
   IsTextEmpty = true;
   tParams.Text             = "";                       // Text of the object
   tParams.TextClipArtName  = "";                       // Clipart name (if clipart mode)
   tParams.FontName         = DEFAULT_FONT_FAMILY;     // font name
   tParams.FontSize         = DEFAULT_FONT_SIZE;        // font size
   tParams.FontColor        = DEFAULT_FONT_COLOR;       // font color
   tParams.FontShadowColor  = DEFAULT_FONT_SHADOWCOLOR; // font shadow color
   tParams.IsBold           = DEFAULT_FONT_ISBOLD;      // true if bold mode
   tParams.IsItalic         = DEFAULT_FONT_ISITALIC;    // true if Italic mode
   tParams.IsUnderline      = DEFAULT_FONT_ISUNDERLINE; // true if Underline mode
   tParams.HAlign           = DEFAULT_FONT_HALIGN;      // Horizontal alignement : 0=left, 1=center, 2=right, 3=justif
   tParams.VAlign           = DEFAULT_FONT_VALIGN;      // Vertical alignement : 0=up, 1=center, 2=bottom
   tParams.StyleText        = DEFAULT_FONT_TEXTEFFECT;  // Style : 0=normal, 1=outerline, 2=shadow up-left, 3=shadow up-right, 4=shadow bt-left, 5=shadow bt-right
   tShotParams.TxtZoomLevel = DEFAULT_TXTZOOMLEVEL;     // Zoom Level for text
   tShotParams.TxtScrollX   = DEFAULT_TXTSCROLLX;       // Scrolling X for text
   tShotParams.TxtScrollY   = DEFAULT_TXTSCROLLY;       // Scrolling Y for text
   tParams.TMType           = eSHAPEDEFAULT;
   //textDocument = NULL;

   // Shape part
   BackgroundForm     = DEFAULT_BACKGROUNDFORM;       // Type of the form : 0=None, 1=Rectangle, 2=Ellipse
   Opacity            = DEFAULT_SHAPE_OPACITY;        // Style of the background of the form
   PenSize            = DEFAULT_SHAPE_BORDERSIZE;     // Width of the border of the form
   PenStyle           = DEFAULT_SHAPE_PENSTYLE;       // Style of the pen border of the form
   PenColor           = DEFAULT_SHAPE_BORDERCOLOR;    // Color of the border of the form
   FormShadowColor    = DEFAULT_SHAPE_SHADOWCOLOR;    // Color of the shadow of the form
   FormShadow         = DEFAULT_SHAPE_SHADOW;         // 0=none, 1=shadow up-left, 2=shadow up-right, 3=shadow bt-left, 4=shadow bt-right
   FormShadowDistance = DEFAULT_SHAPE_SHADOWDISTANCE; // Distance from form to shadow

   BlockSpeedWave     = SPEEDWAVE_PROJECTDEFAULT;

   // Block animation part
   BlockAnimType = DEFAULT_BLOCKANIMTYPE;
   TurnZAxis     = DEFAULT_TURNZAXIS;            // Number of turn from Z axis
   TurnXAxis     = DEFAULT_TURNXAXIS;            // Number of turn from X axis
   TurnYAxis     = DEFAULT_TURNYAXIS;            // Number of turn from Y axis
   Dissolve      = DEFAULT_DISSOLVE;

   // BackgroundBrush is initilise by object constructor except TypeComposition and key
   BackgroundBrush->TypeComposition = TypeComposition;

   ApplyTextMargin(eSHAPEDEFAULT); // Init TMx,TMy,TMw,TMh (Text margins)
   resolvedText.clear();

   //mask = 0;
   useMask = false;
   useTextMask = false;
   use3dFrame = false;

   KenBurnsSlide = false;
   KenBurnsZoom = false;
   fullScreenVideo = false;
}

//====================================================================================================================
cCompositionObject::~cCompositionObject() 
{
   if (BackgroundBrush) 
   {
      delete BackgroundBrush;
      BackgroundBrush = NULL;
   }
   //if( textDocument )
   //   delete textDocument;
}

//====================================================================================================================
void cCompositionObject::ApplyTextMargin(int TMType) 
{
   if (tParams.TMType == eCUSTOM && TMType == eCUSTOM)   
      return;   // Don't overwrite custom settings
   tParams.TMType = eTextMargins(TMType);
   tParams.textMargins = GetPrivateTextMargin();
}

//====================================================================================================================
QRectF cCompositionObject::GetPrivateTextMargin() 
{
   QRectF RectF;
   if (tParams.TMType == eFULLSHAPE) 
   {
      QRectF X100 = RectF = PolygonToRectF(ComputePolygon(BackgroundForm,0,0,100,100));
      RectF = QRectF(X100.left()/100,X100.top()/100,X100.width()/100,X100.height()/100);
   } 
   else if (tParams.TMType == eSHAPEDEFAULT) 
   {
      if (tParams.TextClipArtName != "") 
      {
         cTextFrameObject *TFO = &TextFrameList.List[TextFrameList.SearchImage(tParams.TextClipArtName)];
         RectF = TFO->MarginRect();//QRectF(TFO->TMx,TFO->TMy,TFO->TMw,TFO->TMh);
      } 
      else 
      {
         RectF = ShapeFormDefinition[BackgroundForm].MarginRect();
      }
   } 
   else 
      RectF = tParams.textMargins;
   return RectF;
}

QRectF appliedRect(const QRectF &r, double width, double height) 
{ 
   return QRectF(r.x()*width, r.y() * height, r.width() * width, r.height() * height); 
}
//====================================================================================================================

QRectF cCompositionObject::GetTextMargin(QRectF Workspace,double  ADJUST_RATIO) 
{
   // if type is ShapeDefault, then adjust with border size
   if (tParams.TMType == eFULLSHAPE || tParams.TMType == eCUSTOM) 
   {
      return ::appliedRect(tParams.textMargins, wsRect.width() * Workspace.width(), wsRect.height() * Workspace.height());
   } 
   else 
   {
      double FullMargin = double(PenSize)*ADJUST_RATIO/double(2);
      return QRectF(ShapeFormDefinition[BackgroundForm].TMx * wsRect.width() * Workspace.width() + FullMargin, 
                    ShapeFormDefinition[BackgroundForm].TMy * wsRect.height() * Workspace.height() + FullMargin,
                    ShapeFormDefinition[BackgroundForm].TMw * wsRect.width() * Workspace.width() - FullMargin*2,
                    ShapeFormDefinition[BackgroundForm].TMh * wsRect.height() * Workspace.height() - FullMargin*2);
   }
}

//====================================================================================================================

void cCompositionObject::SaveToXML(QDomElement &domDocument,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,bool CheckTypeComposition,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool SaveBrush,bool IsModel) 
{
   QDomDocument DomDocument;
   QString xmlFragment;
   ExXmlStreamWriter xlmStream(&xmlFragment);
   SaveToXMLex(xlmStream, ElementName, PathForRelativPath, ForceAbsolutPath, CheckTypeComposition, ReplaceList, ResKeyList, SaveBrush, IsModel);
   DomDocument.setContent(xmlFragment);
   domDocument.appendChild(DomDocument.firstChildElement());
   return;

    /*
    // Force a refresh of IsTextEmpty flag
    QTextDocument TextDocument;
    TextDocument.setHtml(tParams.Text);
    IsTextEmpty = TextDocument.isEmpty();

    QDomDocument DomDocument;
    QDomElement Element=DomDocument.createElement(ElementName);

    Element.setAttribute("TypeComposition",TypeComposition);
    Element.setAttribute("IndexKey",IndexKey);
    Element.setAttribute("IsVisible",IsVisible?"1":"0");
    Element.setAttribute("SameAsPrevShot",BlockInheritance ? "0":"1");// crazy, writes a wrong bool value

    // Attribut of the object
    Element.setAttribute("x",wsRect.x());       // Position x
    Element.setAttribute("y",wsRect.y());       // Position x
    Element.setAttribute("w",wsRect.width());   // size width
    Element.setAttribute("h",wsRect.height());  // size height

    if (RotateZAxis != DEFAULT_ROTATEZAXIS)         Element.setAttribute("RotateZAxis",RotateZAxis);       // Rotation from Z axis
    if (RotateXAxis != DEFAULT_ROTATEXAXIS)         Element.setAttribute("RotateXAxis",RotateXAxis);       // Rotation from X axis
    if (RotateYAxis != DEFAULT_ROTATEYAXIS)         Element.setAttribute("RotateYAxis",RotateYAxis);       // Rotation from Y axis
    if (Opacity != DEFAULT_SHAPE_OPACITY)           Element.setAttribute("BackgroundTransparent",Opacity); // Opacity of the form
    if (BlockSpeedWave != SPEEDWAVE_PROJECTDEFAULT) 
    {
      if( BlockSpeedWave > SPEEDWAVE_SQRT )
      {
         Element.setAttribute("BlockSpeedWave",(int)mapSpeedwave(BlockSpeedWave)); // Block speed wave
         Element.setAttribute("extBlockSpeedWave",BlockSpeedWave); // Block speed wave
      }
      else
         Element.setAttribute("BlockSpeedWave",BlockSpeedWave); // Block speed wave
    }

    // Block animation
    if (BlockAnimType != DEFAULT_BLOCKANIMTYPE) Element.setAttribute("BlockAnimType",BlockAnimType); // Block animation type
    if (TurnZAxis != DEFAULT_TURNZAXIS)         Element.setAttribute("TurnZAxis",TurnZAxis);         // Number of turn from Z axis
    if (TurnXAxis != DEFAULT_TURNXAXIS)         Element.setAttribute("TurnXAxis",TurnXAxis);         // Number of turn from X axis
    if (TurnYAxis != DEFAULT_TURNYAXIS)         Element.setAttribute("TurnYAxis",TurnYAxis);         // Number of turn from Y axis
    if (Dissolve != DEFAULT_DISSOLVE)           Element.setAttribute("Dissolve",Dissolve);           // Dissolve value

    // Text part
    if (!tParams.TextClipArtName.isEmpty()) Element.setAttribute("TextClipArtName",tParams.TextClipArtName);        // ClipArt name (if text clipart mode)

    if ((!IsTextEmpty)&&((!CheckTypeComposition)||(TypeComposition!=eCOMPOSITIONTYPE_SHOT))) {
        Element.setAttribute("Text",tParams.Text); // Text of the object
        if (tParams.Text!="") {
            if (tParams.FontName!=DEFAULT_FONT_FAMILY)             Element.setAttribute("FontName",tParams.FontName);                      // font name
            if (tParams.FontSize!=DEFAULT_FONT_SIZE)                Element.setAttribute("FontSize",tParams.FontSize);                      // font size
            if (tParams.FontColor!=DEFAULT_FONT_COLOR)              Element.setAttribute("FontColor",tParams.FontColor);                    // font color
            if (tParams.FontShadowColor!=DEFAULT_FONT_SHADOWCOLOR)  Element.setAttribute("FontShadowColor",tParams.FontShadowColor);        // font shadow color
            if (tParams.IsBold!=DEFAULT_FONT_ISBOLD)                Element.setAttribute("IsBold",tParams.IsBold?"1":"0");                  // true if bold mode
            if (tParams.IsItalic!=DEFAULT_FONT_ISITALIC)            Element.setAttribute("IsItalic",tParams.IsItalic?"1":"0");              // true if Italic mode
            if (tParams.IsUnderline!=DEFAULT_FONT_ISUNDERLINE)      Element.setAttribute("IsUnderline",tParams.IsUnderline?"1":"0");        // true if Underline mode
            if (tParams.HAlign!=DEFAULT_FONT_HALIGN)                Element.setAttribute("HAlign",tParams.HAlign);                          // Horizontal alignement : 0=left, 1=center, 2=right, 3=justif
            if (tParams.VAlign!=DEFAULT_FONT_VALIGN)                Element.setAttribute("VAlign",tParams.VAlign);                          // Vertical alignement : 0=up, 1=center, 2=bottom
            if (tParams.StyleText!=DEFAULT_FONT_TEXTEFFECT)         Element.setAttribute("StyleText",tParams.StyleText);                    // Style : 0=normal, 1=outerline, 2=shadow up-left, 3=shadow up-right, 4=shadow bt-left, 5=shadow bt-right
        }
    }

    // Shot part of text part
    if (tShotParams.TxtZoomLevel!=DEFAULT_TXTZOOMLEVEL) Element.setAttribute("TxtZoomLevel",tShotParams.TxtZoomLevel);  // Zoom Level for text
    if (tShotParams.TxtScrollX!=DEFAULT_TXTSCROLLX)     Element.setAttribute("TxtScrollX",tShotParams.TxtScrollX);      // Scrolling X for text
    if (tShotParams.TxtScrollY!=DEFAULT_TXTSCROLLY)     Element.setAttribute("TxtScrollY",tShotParams.TxtScrollY);      // Scrolling Y for text

    // Text margins
    Element.setAttribute("TMType",tParams.TMType);                                  // Text margins type
    if (tParams.TMType == eCUSTOM) 
    {
        Element.setAttribute("TMx",tParams.textMargins.x());
        Element.setAttribute("TMy",tParams.textMargins.y());
        Element.setAttribute("TMw",tParams.textMargins.width());
        Element.setAttribute("TMh",tParams.textMargins.height());
    }

    // Shape part
    if (BackgroundForm!=DEFAULT_BACKGROUNDFORM)             Element.setAttribute("BackgroundForm",BackgroundForm);                  // Type of the form : 0=None, 1=Rectangle, 2=Ellipse
    if (PenSize!=DEFAULT_SHAPE_BORDERSIZE)                  Element.setAttribute("PenSize",PenSize);                                // Width of the border of the form
    if (PenStyle!=DEFAULT_SHAPE_PENSTYLE)                   Element.setAttribute("PenStyle",PenStyle);                              // Style of the pen border of the form
    if (PenColor!=DEFAULT_SHAPE_BORDERCOLOR)                Element.setAttribute("PenColor",PenColor);                              // Color of the border of the form
    if (FormShadow!=DEFAULT_SHAPE_SHADOW)                   Element.setAttribute("FormShadow",FormShadow);                          // 0=none, 1=shadow up-left, 2=shadow up-right, 3=shadow bt-left, 4=shadow bt-right
    if (FormShadowDistance!=DEFAULT_SHAPE_SHADOWDISTANCE)   Element.setAttribute("FormShadowDistance",FormShadowDistance);          // Distance from form to shadow
    if (FormShadowColor!=DEFAULT_SHAPE_SHADOWCOLOR)         Element.setAttribute("FormShadowColor",FormShadowColor);                // Shadow color

    if (SaveBrush) 
       BackgroundBrush->SaveToXML(&Element,"BackgroundBrush",PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,IsModel);    // Brush of the background of the form
    
    // mask part
    if( useMask )
    {
       Element.setAttribute("Mask_x",mask.getRect().x());       // Position x
       Element.setAttribute("Mask_y",mask.getRect().y());       // Position x
       Element.setAttribute("Mask_w",mask.getRect().width());   // size width
       Element.setAttribute("Mask_h",mask.getRect().height());  // size height
       Element.setAttribute("Mask_ZAngle", mask.zAngle());
       if( mask.invertedMask() )
          Element.setAttribute("invertMask", 1);
    }
    if( use3dFrame )
       Element.setAttribute("use3dFrame",1);       // Position x
    domDocument.appendChild(Element);
    */
}
void cCompositionObject::SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, bool CheckTypeComposition, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool SaveBrush, bool IsModel)
{    
   // Force a refresh of IsTextEmpty flag
   QTextDocument TextDocument;
   TextDocument.setHtml(tParams.Text);
   IsTextEmpty = TextDocument.isEmpty();

   //QDomDocument DomDocument;
   //QDomElement Element = DomDocument.createElement(ElementName);
   xmlStream.writeStartElement(ElementName);

   xmlStream.writeAttribute(qsl_TypeComposition, TypeComposition);
   xmlStream.writeAttribute("IndexKey", IndexKey);
   xmlStream.writeAttribute("IsVisible", IsVisible);
   xmlStream.writeAttribute("SameAsPrevShot", !BlockInheritance); // wrong, for compatibility

   // Attribut of the object
   xmlStream.writeAttribute("x", wsRect.x());       // Position x
   xmlStream.writeAttribute("y", wsRect.y());       // Position x
   xmlStream.writeAttribute("w", wsRect.width());   // size width
   xmlStream.writeAttribute("h", wsRect.height());  // size height

   if (RotateZAxis != DEFAULT_ROTATEZAXIS)         xmlStream.writeAttribute("RotateZAxis", RotateZAxis);       // Rotation from Z axis
   if (RotateXAxis != DEFAULT_ROTATEXAXIS)         xmlStream.writeAttribute("RotateXAxis", RotateXAxis);       // Rotation from X axis
   if (RotateYAxis != DEFAULT_ROTATEYAXIS)         xmlStream.writeAttribute("RotateYAxis", RotateYAxis);       // Rotation from Y axis
   // sRotationInfo
   xmlStream.writeAttribute("ZRotationAxis", ZRotationInfo.axis);
   xmlStream.writeAttribute("ZRotationCenter", ZRotationInfo.center);
   xmlStream.writeAttribute("ZRotationCenterX", ZRotationInfo.rotationCenterX);
   xmlStream.writeAttribute("ZRotationCenterY", ZRotationInfo.rotationCenterY);

   if (Opacity != DEFAULT_SHAPE_OPACITY)           xmlStream.writeAttribute("BackgroundTransparent", Opacity); // Opacity of the form
   if (BlockSpeedWave != SPEEDWAVE_PROJECTDEFAULT)
   {
      if (BlockSpeedWave > SPEEDWAVE_SQRT)
      {
         xmlStream.writeAttribute(attribute("BlockSpeedWave", (int)mapSpeedwave(BlockSpeedWave))); // Block speed wave
         xmlStream.writeAttribute(attribute("extBlockSpeedWave", BlockSpeedWave)); // Block speed wave
      }
      else
         xmlStream.writeAttribute(attribute("BlockSpeedWave", BlockSpeedWave)); // Block speed wave
   }

   // Block animation
   if (BlockAnimType != DEFAULT_BLOCKANIMTYPE) xmlStream.writeAttribute(attribute("BlockAnimType", BlockAnimType)); // Block animation type
   if (TurnZAxis != DEFAULT_TURNZAXIS)         xmlStream.writeAttribute(attribute("TurnZAxis", TurnZAxis));         // Number of turn from Z axis
   if (TurnXAxis != DEFAULT_TURNXAXIS)         xmlStream.writeAttribute(attribute("TurnXAxis", TurnXAxis));         // Number of turn from X axis
   if (TurnYAxis != DEFAULT_TURNYAXIS)         xmlStream.writeAttribute(attribute("TurnYAxis", TurnYAxis));         // Number of turn from Y axis
   if (Dissolve != DEFAULT_DISSOLVE)           xmlStream.writeAttribute(attribute("Dissolve", Dissolve));           // Dissolve value

   // Text part
   if (!tParams.TextClipArtName.isEmpty()) 
      xmlStream.writeAttribute("TextClipArtName", tParams.TextClipArtName);        // ClipArt name (if text clipart mode)

   if ((!IsTextEmpty) && ((!CheckTypeComposition) || (TypeComposition != eCOMPOSITIONTYPE_SHOT)))
   {
      if (tParams.Text != "")
      {
         if (tParams.FontName != DEFAULT_FONT_FAMILY)             xmlStream.writeAttribute("FontName", tParams.FontName);                      // font name
         if (tParams.FontSize != DEFAULT_FONT_SIZE)                xmlStream.writeAttribute("FontSize", tParams.FontSize);                      // font size
         if (tParams.FontColor != DEFAULT_FONT_COLOR)              xmlStream.writeAttribute("FontColor", tParams.FontColor);                    // font color
         if (tParams.FontShadowColor != DEFAULT_FONT_SHADOWCOLOR)  xmlStream.writeAttribute("FontShadowColor", tParams.FontShadowColor);        // font shadow color
         if (tParams.IsBold != DEFAULT_FONT_ISBOLD)                xmlStream.writeAttribute("IsBold", tParams.IsBold );                  // true if bold mode
         if (tParams.IsItalic != DEFAULT_FONT_ISITALIC)            xmlStream.writeAttribute("IsItalic", tParams.IsItalic );              // true if Italic mode
         if (tParams.IsUnderline != DEFAULT_FONT_ISUNDERLINE)      xmlStream.writeAttribute(qsl_IsUnderline, tParams.IsUnderline );        // true if Underline mode
         if (tParams.HAlign != DEFAULT_FONT_HALIGN)                xmlStream.writeAttribute("HAlign", tParams.HAlign);                          // Horizontal alignement : 0=left, 1=center, 2=right, 3=justif
         if (tParams.VAlign != DEFAULT_FONT_VALIGN)                xmlStream.writeAttribute("VAlign", tParams.VAlign);                          // Vertical alignement : 0=up, 1=center, 2=bottom
         if (tParams.StyleText != DEFAULT_FONT_TEXTEFFECT)         xmlStream.writeAttribute("StyleText", tParams.StyleText);                    // Style : 0=normal, 1=outerline, 2=shadow up-left, 3=shadow up-right, 4=shadow bt-left, 5=shadow bt-right
      }
      xmlStream.writeAttribute("Text", tParams.Text); // Text of the object
   }

   // Shot part of text part
   if (tShotParams.TxtZoomLevel != DEFAULT_TXTZOOMLEVEL) xmlStream.writeAttribute(attribute("TxtZoomLevel", tShotParams.TxtZoomLevel));  // Zoom Level for text
   if (tShotParams.TxtScrollX != DEFAULT_TXTSCROLLX)     xmlStream.writeAttribute(attribute("TxtScrollX", tShotParams.TxtScrollX));      // Scrolling X for text
   if (tShotParams.TxtScrollY != DEFAULT_TXTSCROLLY)     xmlStream.writeAttribute(attribute("TxtScrollY", tShotParams.TxtScrollY));      // Scrolling Y for text

   // Text margins
   xmlStream.writeAttribute(attribute("TMType", tParams.TMType));                                  // Text margins type
   if (tParams.TMType == eCUSTOM)
   {
      xmlStream.writeAttribute(attribute("TMx", tParams.textMargins.x()));
      xmlStream.writeAttribute(attribute("TMy", tParams.textMargins.y()));
      xmlStream.writeAttribute(attribute("TMw", tParams.textMargins.width()));
      xmlStream.writeAttribute(attribute("TMh", tParams.textMargins.height()));
   }

   // Shape part
   if (BackgroundForm != DEFAULT_BACKGROUNDFORM)             xmlStream.writeAttribute("BackgroundForm", BackgroundForm);                  // Type of the form : 0=None, 1=Rectangle, 2=Ellipse
   if (PenSize != DEFAULT_SHAPE_BORDERSIZE)                  xmlStream.writeAttribute("PenSize", PenSize);                                // Width of the border of the form
   if (PenStyle != DEFAULT_SHAPE_PENSTYLE)                   xmlStream.writeAttribute("PenStyle", PenStyle);                              // Style of the pen border of the form
   if (PenColor != DEFAULT_SHAPE_BORDERCOLOR)                xmlStream.writeAttribute("PenColor", PenColor);                              // Color of the border of the form
   if (FormShadow != DEFAULT_SHAPE_SHADOW)                   xmlStream.writeAttribute("FormShadow", FormShadow);                          // 0=none, 1=shadow up-left, 2=shadow up-right, 3=shadow bt-left, 4=shadow bt-right
   if (FormShadowDistance != DEFAULT_SHAPE_SHADOWDISTANCE)   xmlStream.writeAttribute("FormShadowDistance", FormShadowDistance);          // Distance from form to shadow
   if (FormShadowColor != DEFAULT_SHAPE_SHADOWCOLOR)         xmlStream.writeAttribute("FormShadowColor", FormShadowColor);                // Shadow color

                                                                                                                                          // mask part
   if ( /*mask && */useMask)
   {
      xmlStream.writeAttribute("Mask_x", mask.getRect().x());       // Position x
      xmlStream.writeAttribute("Mask_y", mask.getRect().y());       // Position x
      xmlStream.writeAttribute("Mask_w", mask.getRect().width());   // size width
      xmlStream.writeAttribute("Mask_h", mask.getRect().height());  // size height
      xmlStream.writeAttribute("Mask_ZAngle", mask.zAngle());
      if (mask.invertedMask())
         xmlStream.writeAttribute("invertMask", 1);
   }
   if (use3dFrame)
      xmlStream.writeAttribute("use3dFrame", 1);       // Position x

   if (SaveBrush)
      BackgroundBrush->SaveToXMLex(xmlStream, "BackgroundBrush", PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);    // Brush of the background of the form

   xmlStream.writeEndElement();
}


//====================================================================================================================

bool cCompositionObject::LoadFromXML(QDomElement domDocument,QString ElementName,QString PathForRelativPath,cCompositionList *ObjectComposition,QStringList *AliasList,bool CheckTypeComposition,TResKeyList *ResKeyList,bool DuplicateRes,bool RestoreBrush,cCompositionObject *GlobalBlock) 
{
    InitDefaultValues();

    if((domDocument.elementsByTagName(ElementName).length() > 0) && (domDocument.elementsByTagName(ElementName).item(0).isElement() == true))
    {
        QDomElement Element=domDocument.elementsByTagName(ElementName).item(0).toElement();
        bool    IsOk=true;

        if (Element.hasAttribute(qsl_TypeComposition))            TypeComposition =eTypeComposition(Element.attribute(qsl_TypeComposition).toInt());
        //if (Element.hasAttribute("IsVisible"))                  IsVisible       =Element.attribute("IsVisible")=="1";
        getAttrib(IsVisible, Element, "IsVisible");
        getAttrib(BlockInheritance, Element, "SameAsPrevShot");
        //if (Element.hasAttribute("SameAsPrevShot"))
        //   BlockInheritance=Element.attribute("SameAsPrevShot")=="0";

        // Special case for paste operation (indexkey was changed)
        if (GlobalBlock) 
           IndexKey=GlobalBlock->IndexKey; 
        else if (Element.hasAttribute("IndexKey")) 
           IndexKey=Element.attribute("IndexKey").toInt();

        // Attribut of the object
        if (Element.hasAttribute("x"))                          wsRect.setX(GetDoubleValue(Element,"x"));                           // Position x
        if (Element.hasAttribute("y"))                          wsRect.setY(GetDoubleValue(Element,"y"));                           // Position y
        if (Element.hasAttribute("w"))                          wsRect.setWidth(GetDoubleValue(Element,"w"));                           // size width
        if (Element.hasAttribute("h"))                          wsRect.setHeight(GetDoubleValue(Element,"h"));                           // size height
        if (Element.hasAttribute("BackgroundTransparent"))      Opacity         =Element.attribute("BackgroundTransparent").toInt();    // Style Opacity of the background of the form
        getAttrib(RotateZAxis, Element, "RotateZAxis");         // Rotation from Z axis
        getAttrib(RotateXAxis, Element, "RotateXAxis");         // Rotation from X axis
        getAttrib(RotateYAxis, Element, "RotateYAxis");         // Rotation from Y axis
        // sRotationInfo
        if (Element.hasAttribute("ZRotationAxis"))    ZRotationInfo.axis = Qt::Axis( Element.attribute("ZRotationAxis").toInt() );
        if (Element.hasAttribute("ZRotationCenter"))  ZRotationInfo.center = eRotationCenter(Element.attribute("ZRotationCenter").toInt());
        if (Element.hasAttribute("ZRotationCenterX")) ZRotationInfo.rotationCenterX = GetDoubleValue(Element,"ZRotationCenterX");
        if (Element.hasAttribute("ZRotationCenterY")) ZRotationInfo.rotationCenterY = GetDoubleValue(Element, "ZRotationCenterY");


        if (Element.hasAttribute("extBlockSpeedWave"))             
         BlockSpeedWave  = Element.attribute("extBlockSpeedWave").toInt();           // Block speed wave
        else if (Element.hasAttribute("BlockSpeedWave"))
         BlockSpeedWave = Element.attribute("BlockSpeedWave").toInt();           // Block speed wave

        if (Element.hasAttribute("BlockAnimType"))              BlockAnimType   =Element.attribute("BlockAnimType").toInt();            // Block animation type
        if (Element.hasAttribute("TurnZAxis"))                  TurnZAxis       =Element.attribute("TurnZAxis").toInt();                // Number of turn from Z axis
        if (Element.hasAttribute("TurnXAxis"))                  TurnXAxis       =Element.attribute("TurnXAxis").toInt();                // Number of turn from X axis
        if (Element.hasAttribute("TurnYAxis"))                  TurnYAxis       =Element.attribute("TurnYAxis").toInt();                // Number of turn from Y axis
        if (Element.hasAttribute("Dissolve"))                   Dissolve        =eBlockAnimValue(Element.attribute("Dissolve").toInt());// Dissolve value

        // Text part
        if (Element.hasAttribute("TextClipArtName"))            tParams.TextClipArtName =Element.attribute("TextClipArtName");                      // ClipArt name (if text clipart mode)
        if ((Element.hasAttribute("Text")) && ((!CheckTypeComposition) || (TypeComposition!=eCOMPOSITIONTYPE_SHOT))) {
            tParams.Text=Element.attribute("Text");  // Text of the object
            IsTextEmpty=tParams.Text.isEmpty();
            if ((!IsTextEmpty)&&(tParams.Text.startsWith("<!DOCTYPE HTML"))) {
                QTextDocument TextDocument;
                TextDocument.setHtml(tParams.Text);
                IsTextEmpty =TextDocument.isEmpty();
            }
            if (!IsTextEmpty) {
                if (Element.hasAttribute("FontName"))           tParams.FontName        =Element.attribute("FontName");                             // font name
                if (Element.hasAttribute("FontSize"))           tParams.FontSize        =Element.attribute("FontSize").toInt();                     // font size
                if (Element.hasAttribute("FontColor"))          tParams.FontColor       =Element.attribute("FontColor");                            // font color
                if (Element.hasAttribute("FontShadowColor"))    tParams.FontShadowColor =Element.attribute("FontShadowColor");                      // font shadow color
                if (Element.hasAttribute("IsBold"))             tParams.IsBold          =Element.attribute("IsBold")=="1";                          // true if bold mode
                if (Element.hasAttribute("IsItalic"))           tParams.IsItalic        =Element.attribute("IsItalic")=="1";                        // true if Italic mode
                if (Element.hasAttribute(qsl_IsUnderline))        tParams.IsUnderline     =Element.attribute(qsl_IsUnderline)=="1";                     // true if Underline mode
                if (Element.hasAttribute("HAlign"))             tParams.HAlign          =Element.attribute("HAlign").toInt();                       // Horizontal alignement : 0=left, 1=center, 2=right, 3=justif
                if (Element.hasAttribute("VAlign"))             tParams.VAlign          =Element.attribute("VAlign").toInt();                       // Vertical alignement : 0=up, 1=center, 2=bottom
                if (Element.hasAttribute("StyleText"))          tParams.StyleText       =Element.attribute("StyleText").toInt();                    // Style : 0=normal, 1=outerline, 2=shadow up-left, 3=shadow up-right, 4=shadow bt-left, 5=shadow bt-right

                // Conversion from plaintext (ffd <1.3)
                if (!tParams.Text.startsWith("<!DOCTYPE HTML")) {
                    QTextDocument       TextDoc(tParams.Text);
                    QFont               Font=QFont(tParams.FontName,tParams.FontSize*2,tParams.IsBold?QFont::Bold:QFont::Normal,tParams.IsItalic?QFont::StyleItalic:QFont::StyleNormal);    // FontSize is always 10 and size if given with setPointSizeF !
                    QTextOption         OptionText((tParams.HAlign==0)?Qt::AlignLeft:(tParams.HAlign==1)?Qt::AlignHCenter:(tParams.HAlign==2)?Qt::AlignRight:Qt::AlignJustify); // Setup horizontal alignement
                    QTextCursor         Cursor(&TextDoc);
                    QTextCharFormat     TCF;
                    QTextBlockFormat    TBF;

                    Cursor.select(QTextCursor::Document);
                    OptionText.setWrapMode(QTextOption::WordWrap);                                                                              // Setup word wrap text option
                    Font.setUnderline(tParams.IsUnderline);                                                                                             // Set underline

                    TextDoc.setDefaultFont(Font);
                    TextDoc.setDefaultTextOption(OptionText);

                    TCF.setFont(Font);
                    TCF.setFontWeight(tParams.IsBold?QFont::Bold:QFont::Normal);
                    TCF.setFontItalic(tParams.IsItalic);
                    TCF.setFontUnderline(tParams.IsUnderline);
                    TCF.setForeground(QBrush(QColor(tParams.FontColor)));
                    TBF.setAlignment((tParams.HAlign==0)?Qt::AlignLeft:(tParams.HAlign==1)?Qt::AlignHCenter:(tParams.HAlign==2)?Qt::AlignRight:Qt::AlignJustify);
                    Cursor.setCharFormat(TCF);
                    Cursor.setBlockFormat(TBF);
                    tParams.Text=TextDoc.toHtml();
                }
            }
        }

        // Shot part of text part
        tParams.TMType = eFULLSHAPE;   // For compatibility with version prior to 1.5 => force magins type to fullshape
        if (Element.hasAttribute("TxtZoomLevel")) tShotParams.TxtZoomLevel    = Element.attribute("TxtZoomLevel").toInt();             // Zoom Level for text
        if (Element.hasAttribute("TxtScrollX"))   tShotParams.TxtScrollX      = Element.attribute("TxtScrollX").toInt();               // Scrolling X for text
        if (Element.hasAttribute("TxtScrollY"))   tShotParams.TxtScrollY      = Element.attribute("TxtScrollY").toInt();               // Scrolling Y for text
        if (Element.hasAttribute("TMType"))       tParams.TMType              = eTextMargins(Element.attribute("TMType").toInt());                   // Text margins type
        if (Element.hasAttribute("TMx"))          tParams.textMargins.moveLeft(GetDoubleValue(Element,"TMx"));                         // Text margins
        if (Element.hasAttribute("TMy"))          tParams.textMargins.moveTop(GetDoubleValue(Element,"TMy"));                          // Text margins
        if (Element.hasAttribute("TMw"))          tParams.textMargins.setWidth(GetDoubleValue(Element,"TMw"));                         // Text margins
        if (Element.hasAttribute("TMh"))          tParams.textMargins.setHeight(GetDoubleValue(Element,"TMh"));                        // Text margins

        // Shape part
        if (Element.hasAttribute("BackgroundForm"))     BackgroundForm      = Element.attribute("BackgroundForm").toInt();           // Type of the form : 0=None, 1=Rectangle, 2=Ellipse
        if (Element.hasAttribute("PenSize"))            PenSize             = Element.attribute("PenSize").toInt();                  // Width of the border of the form
        if (Element.hasAttribute("PenStyle"))           PenStyle            = Element.attribute("PenStyle").toInt();                 // Style of the pen border of the form
        if (Element.hasAttribute("PenColor"))           PenColor            = Element.attribute("PenColor");                         // Color of the border of the form
        if (Element.hasAttribute("FormShadowColor"))    FormShadowColor     = Element.attribute("FormShadowColor");                  // Color of the shadow of the form
        if (Element.hasAttribute("FormShadow"))         FormShadow          = Element.attribute("FormShadow").toInt();               // 0=none, 1=shadow up-left, 2=shadow up-right, 3=shadow bt-left, 4=shadow bt-right
        if (Element.hasAttribute("FormShadowDistance")) FormShadowDistance  = Element.attribute("FormShadowDistance").toInt();       // Distance from form to shadow

        if ((TypeComposition == eCOMPOSITIONTYPE_SHOT) && (ObjectComposition != NULL)) 
        {
            // Construct link to object from DiaporamaObject->ObjectComposition
           for (auto compositionObject : ObjectComposition->compositionObjects)
           {
              if (compositionObject->IndexKey == IndexKey)
              {
                 BackgroundBrush->MediaObject = compositionObject->BackgroundBrush->MediaObject;
                 BackgroundBrush->DeleteMediaObject = false;
                 tParams.Text = compositionObject->tParams.Text;
                 if (tParams.Text != "")
                 {
                    tParams = compositionObject->tParams;
                    resolvedText.clear();
                 }
                 break;
              }
           }
        }

        // Compatibility with old release : remove case BackgroundForm==0
        if (BackgroundForm == 0) 
        {
            BackgroundForm            = 1;        // Set to rectangle
            PenSize                   = 0;        // border=0
            BackgroundBrush->BrushType= 0;        // brushtype=no brush
        }
        if (RestoreBrush) 
        {
            bool ModifyFlag;
            IsOk=BackgroundBrush->LoadFromXML(&Element,"BackgroundBrush",PathForRelativPath,AliasList,&ModifyFlag,ResKeyList,DuplicateRes);  // Brush of the background of the form
            if (ModifyFlag) ((MainWindow *)ApplicationConfig->TopLevelWindow)->SetModifyFlag(true);
        }

        // Ensure unvisible video have no sound !
        if (!IsVisible && BackgroundBrush->MediaObject && BackgroundBrush->MediaObject->is_Videofile()) 
         BackgroundBrush->SoundVolume = 0;

        // mask part
       if (Element.hasAttribute("Mask_x"))
       {
         useMask = true;
         //mask = new cCompositionObjectMask();
         mask.setX(GetDoubleValue(Element,"Mask_x"));                           // Position x
         mask.setY(GetDoubleValue(Element,"Mask_y"));                           // Position x
         mask.setWidth(GetDoubleValue(Element,"Mask_w"));                           // size width
         mask.setHeight(GetDoubleValue(Element,"Mask_h"));                           // size height
         mask.setZAngle(GetDoubleValue(Element,"Mask_ZAngle"));
         if(Element.hasAttribute("invertMask"))
            mask.setInvertMask(true);

       }
       if (Element.hasAttribute("use3dFrame"))
         use3dFrame = true;
        return IsOk;
    }
    return false;
}

//====================================================================================================================

void cCompositionObject::ComputeOptimisationFlags(const cCompositionObject *Previous) 
{
   QTextDocument TextDocument;
   TextDocument.setHtml(tParams.Text);
   IsTextEmpty = TextDocument.isEmpty();
   bool isFullRect = false;
   QRect r1(0,0,1920,1080);
   QRect r2 = wsRect.applyTo(1920,1080).toRect();
   isFullRect = r1 == r2;
   IsFullScreen = IsTextEmpty && BackgroundBrush->MediaObject
         && IsVisible && BlockAnimType == 0 && BackgroundForm == 1 && PenSize == 0 && Opacity == 0 
         && RotateZAxis == 0 && RotateXAxis == 0 && RotateYAxis == 0 
         && !useMask 
         && isFullRect
         //&& (int(wsRect.x()*10000) == 0) && (int(wsRect.y()*10000) == 0) 
         //&& (int(wsRect.width()*10000) == 10000) && (int(wsRect.height()*10000) == 10000)
         && ( !Previous || ( wsRect == Previous->wsRect &&  RotateXAxis == Previous->RotateXAxis && RotateYAxis == Previous->RotateYAxis && RotateZAxis == Previous->RotateZAxis) );
   // todo: check that BlockAnimType == BLOCKANIMTYPE_DISSOLVE did not make this nonStatic is ok
   IsStatic = !isVideo() 
         && (BlockAnimType == BLOCKANIMTYPE_NONE /*|| BlockAnimType == BLOCKANIMTYPE_DISSOLVE*/) 
         && (!Previous || sameAs(Previous));

   KenBurnsSlide = false;
   if( Previous && BackgroundBrush->isSlidingBrush(Previous->BackgroundBrush) )
   {
      if( size() == Previous->size() 
         && BlockAnimType == Previous->BlockAnimType
      )
         KenBurnsSlide = true;
   }
   KenBurnsZoom = false;
   fullScreenVideo = false;
}

//====================================================================================================================

bool cCompositionObject::sameAs(const cCompositionObject* prevCompositionObject) const
{
   bool IsSame = true;
   if( pos() != prevCompositionObject->pos() )
   {
      //qDebug() << "pos differs";
      IsSame = false;
   }
   if( size() != prevCompositionObject->size() )
   {
      //qDebug() << "size differs";
      IsSame = false;
   }
   if( (RotateXAxis != prevCompositionObject->RotateXAxis) 
       || (RotateYAxis != prevCompositionObject->RotateYAxis) 
       || (RotateZAxis != prevCompositionObject->RotateZAxis) )
   {
      //qDebug() << "rotation differs";
      IsSame = false;
   }
   if( BlockAnimType != 0 )
   {
      //qDebug() << "blockAnim differs";
      IsSame = false;
   }
   if((tShotParams.TxtZoomLevel != prevCompositionObject->tShotParams.TxtZoomLevel)
      || (tShotParams.TxtScrollX   != prevCompositionObject->tShotParams.TxtScrollX)
      || (tShotParams.TxtScrollY   != prevCompositionObject->tShotParams.TxtScrollY) )
   {
      //qDebug() << "zoom or scroll differs";
      IsSame = false;
   }
   if( !BackgroundBrush->isStatic(prevCompositionObject->BackgroundBrush) )
   {
      //qDebug() << "BackgroundBrush differs";
      IsSame = false;
   }
   if( /*mask && */useMask && prevCompositionObject->usingMask() )
      IsSame &= mask.sameAs(prevCompositionObject->getMask());
   return IsSame;
}

bool cCompositionObject::isMoving(const cCompositionObject* prevCompositionObject) const
{
   //if( pos() != prevCompositionObject->pos() )
   if( abs(getX() - prevCompositionObject->getX()) > 0.002 || abs(getY() - prevCompositionObject->getY()) > 0.002 )
      return true;
   return false;
}

bool cCompositionObject::isOnScreen(const cCompositionObject* prevCompositionObject) const
{
   // prevCompositionObject can be null here !!
   QRectF r(0, 0, 1, 1);
   if (prevCompositionObject && isMoving(prevCompositionObject))
   {
      QRectF r2 = prevCompositionObject->wsRect.united(wsRect);
      //qDebug() << "ons moving " << r2 << " intersects " << r2.intersects(r);
      return r2.intersects(r);
   }
   //qDebug() << "ons " << this->wsRect << " intersects " << this->wsRect.intersects(r);
   return wsRect.intersects(r);
}

bool cCompositionObject::isVideo()
{
   //if ((BackgroundBrush->BrushType == BRUSHTYPE_IMAGEDISK) && (BackgroundBrush->MediaObject) && (BackgroundBrush->MediaObject->ObjectType == OBJECTTYPE_VIDEOFILE)) 
   if( BackgroundBrush )
      return BackgroundBrush->isVideo();
      //return true;
   return false;
}

bool cCompositionObject::hasFilter()
{
   if( BackgroundBrush != NULL && BackgroundBrush->hasFilter())
      return true;
   return false;
}

QPointF cCompositionObject::getRotationOrigin(const QRectF &shapeRect, eRotationCenter rtc)
{
   QPointF p(0, 0);
   switch (rtc)
   {
      case eRotTopLeft:
         p = shapeRect.topLeft();
         break;
      case eRotTopMiddle:
         p.setX(shapeRect.left() + shapeRect.width()/2);
         p.setY(shapeRect.top());
         break;
      case eRotTopRight:
         p = shapeRect.topRight();
         break;

      case eRotCenterLeft:
         p.setX(shapeRect.left());
         p.setY(shapeRect.top() + shapeRect.height() / 2);
         break;
      case eRotCenterMiddle: 
         p = shapeRect.center();
         break;
      case eRotCenterRight: 
         p.setX(shapeRect.right());
         p.setY(shapeRect.top() + shapeRect.height() / 2);
         break;

      case eRotBottomLeft:          
         p = shapeRect.bottomLeft();
         break;
      case eRotBottomMiddle: 
         p.setX(shapeRect.left() + shapeRect.width()/2);
         p.setY(shapeRect.bottom());
         break;
      case eRotBottomRight:
         p = shapeRect.bottomRight();
         break;

      case eRotFree:
      case eRotCenter:
      default:
         p = shapeRect.center();
   }
   return p;
}

bool cCompositionObject::hasRotation()
{
   if( RotateZAxis != 0.0 || RotateXAxis != 0.0 || RotateYAxis != 0.0 )
      return true;
   return false;
}

bool cCompositionObject::hasTurns() 
{ 
   if( BlockAnimType == BLOCKANIMTYPE_MULTIPLETURN 
       && ( TurnXAxis != 0 || TurnYAxis != 0 || TurnZAxis != 0 ) )
      return true;
   return false;
}

void cCompositionObject::preLoad(bool bPreview)
{
   if( TypeComposition != eCOMPOSITIONTYPE_OBJECT )
      return;
   if( BackgroundBrush )
      BackgroundBrush->preLoad(bPreview);
}

//====================================================================================================================

int cCompositionObject::GetAutoCompoSize(int ffDProjectGeometry) 
{                         
   int   ImageType      = BackgroundBrush->GetImageType();
   int   AutoCompoStyle = AUTOCOMPOSIZE_CUSTOM;

   // Calc screen size
   qreal ScreenWidth    = qreal(1920);
   qreal ScreenHeight   = qreal(ffDProjectGeometry == GEOMETRY_4_3 ? 1440 : ffDProjectGeometry == GEOMETRY_16_9 ? 1080 : ffDProjectGeometry == GEOMETRY_40_17 ? 816 : 1920);
   qreal ScreenGeometry = ScreenHeight/ScreenWidth;

   // Calc real image size (if it's and image)
   qreal RealImageWidth    = qreal(BackgroundBrush->MediaObject ? BackgroundBrush->MediaObject->width() : ScreenWidth);
   qreal RealImageHeight   = qreal(BackgroundBrush->MediaObject ? BackgroundBrush->MediaObject->height() : ScreenHeight);
   if ( BackgroundBrush->isVideo() && (RealImageHeight == 1088) && (RealImageWidth == 1920) && (pAppConfig->Crop1088To1080)) 
      RealImageHeight = 1080;

   // Calc brush size
   qreal CanvasSize    = sqrt(RealImageWidth*RealImageWidth+RealImageHeight*RealImageHeight);   // Calc hypothenuse of the image to define full canvas
   //qreal ImageWidth    =qRound(CanvasSize*BackgroundBrush->ZoomFactor);
   //qreal ImageHeight   =qRound(CanvasSize*BackgroundBrush->ZoomFactor*BackgroundBrush->AspectRatio);
   qreal ImageWidth    = DoubleRound(CanvasSize*BackgroundBrush->ZoomFactor(),0);
   qreal ImageHeight   = DoubleRound(CanvasSize*BackgroundBrush->ZoomFactor() * BackgroundBrush->AspectRatio() ,0);
   qreal ImageGeometry = DoubleRound(ImageHeight/ImageWidth,4);

   // Calc destination size
   qreal DestWidth      = DoubleRound(ScreenWidth * width(), 0);
   qreal DestHeight     = DoubleRound(ScreenHeight * height(), 0);
   //qreal DestGeometry   =DestHeight/DestWidth;

   if (DestWidth == ImageWidth && DestHeight == ImageHeight) 
   {
      AutoCompoStyle = AUTOCOMPOSIZE_REALSIZE; 
      if (int(DestWidth) == int(ScreenWidth) && int(DestHeight) == int(ScreenHeight))
         AutoCompoStyle = AUTOCOMPOSIZE_FULLSCREEN;
   }
   else 
   {
      // Make adjustement if it's not an image and geometry is locked
      if (ImageType == IMAGETYPE_UNKNOWN && BackgroundBrush->LockGeometry()) 
      {
         if ((ImageHeight * (height()/width())) < ScreenHeight)                       
            ScreenHeight = ScreenHeight * (height()/width());
         else
            ScreenWidth = ScreenWidth / (height()/width());
      }

      // Make adjustement if it's an image and ImageGeometry!=DestGeometry
      if (ImageType != IMAGETYPE_UNKNOWN && ImageGeometry != ScreenGeometry) 
      {
         if ((ImageHeight*(ScreenWidth/ImageWidth)) < ScreenHeight)
            ScreenHeight = ImageHeight*(ScreenWidth/ImageWidth);
         else
            ScreenWidth = ImageWidth*(ScreenHeight/ImageHeight);
      }

      if (int(DestWidth) == int(ScreenWidth) &&     int(DestHeight) == int(ScreenHeight))       AutoCompoStyle = AUTOCOMPOSIZE_FULLSCREEN;
      if (int(DestWidth) == int(ScreenWidth*0.9) && int(DestHeight) == int(ScreenHeight*0.9))   AutoCompoStyle = AUTOCOMPOSIZE_TVMARGINS;
      if (int(DestWidth) == int(2*ScreenWidth/3) && int(DestHeight) == int(2*ScreenHeight/3))   AutoCompoStyle = AUTOCOMPOSIZE_TWOTHIRDSSCREEN;
      if (int(DestWidth) == int(ScreenWidth/2) &&   int(DestHeight) == int(ScreenHeight/2))     AutoCompoStyle = AUTOCOMPOSIZE_HALFSCREEN;
      if (int(DestWidth) == int(ScreenWidth/3) &&   int(DestHeight) == int(ScreenHeight/3))     AutoCompoStyle = AUTOCOMPOSIZE_THIRDSCREEN;
      if (int(DestWidth) == int(ScreenWidth/4) &&   int(DestHeight) == int(ScreenHeight/4))     AutoCompoStyle = AUTOCOMPOSIZE_QUARTERSCREEN;
   }                                                                   
   return AutoCompoStyle;
}

//====================================================================================================================

void cCompositionObject::ApplyAutoCompoSize(int AutoCompoStyle,int ffDProjectGeometry,bool AllowMove) 
{
   int ImageType = BackgroundBrush->GetImageType();

   // Calc screen size
   qreal ScreenWidth  = qreal(ffDProjectGeometry == GEOMETRY_THUMBNAIL ? 600 : 1920);
   qreal ScreenHeight = qreal(ffDProjectGeometry == GEOMETRY_THUMBNAIL ? 800 : ffDProjectGeometry == GEOMETRY_4_3 ? 1440 : ffDProjectGeometry == GEOMETRY_16_9 ? 1080 : ffDProjectGeometry == GEOMETRY_40_17 ? 816 : 1920);
   //qreal ScreenGeometry    =ScreenHeight/ScreenWidth;

   // Calc real image size (if it's and image)
   qreal RealImageWidth  = qreal(BackgroundBrush->MediaObject ? BackgroundBrush->MediaObject->width() : ScreenWidth);
   qreal RealImageHeight = qreal(BackgroundBrush->MediaObject ? BackgroundBrush->MediaObject->height() : ScreenHeight);
   if (BackgroundBrush->isVideo() && RealImageWidth == 1920 && RealImageHeight == 1088 && pAppConfig->Crop1088To1080) 
      RealImageHeight = 1080;

   // Calc brush size
   qreal CanvasSize        = sqrt(RealImageWidth*RealImageWidth+RealImageHeight*RealImageHeight);   // Calc hypothenuse of the image to define full canvas
   //qreal ImageWidth         =qRound(CanvasSize*BackgroundBrush->ZoomFactor);
   //qreal ImageHeight        =qRound(CanvasSize*BackgroundBrush->ZoomFactor*BackgroundBrush->AspectRatio);
   qreal ImageWidth        = CanvasSize*BackgroundBrush->ZoomFactor();
   qreal ImageHeight       = CanvasSize*BackgroundBrush->ZoomFactor()*BackgroundBrush->AspectRatio();
   qreal ImageGeometry     = DoubleRound(ImageHeight/ImageWidth,4);

   // Calc destination size
   qreal DestWidth = ScreenWidth;
   qreal DestHeight= ScreenHeight;
   switch (AutoCompoStyle) 
   {
      case AUTOCOMPOSIZE_CUSTOM           :  DestWidth = ScreenWidth*width();    DestHeight = ScreenHeight*height();      break;      // Keep current value
      case AUTOCOMPOSIZE_REALSIZE         :  DestWidth = ImageWidth;       DestHeight = ImageHeight;         break;
      case AUTOCOMPOSIZE_FULLSCREEN       :  DestWidth = ScreenWidth;      DestHeight = ScreenHeight;        break;
      case AUTOCOMPOSIZE_TVMARGINS        :  DestWidth = ScreenWidth*0.9;  DestHeight = ScreenHeight*0.9;    break;      // TV Margins is 5% each
      case AUTOCOMPOSIZE_TWOTHIRDSSCREEN  :  DestWidth = 2*ScreenWidth/3;  DestHeight = 2*ScreenHeight/3;    break;
      case AUTOCOMPOSIZE_HALFSCREEN       :  DestWidth = ScreenWidth/2;    DestHeight = ScreenHeight/2;      break;
      case AUTOCOMPOSIZE_THIRDSCREEN      :  DestWidth = ScreenWidth/3;    DestHeight = ScreenHeight/3;      break;
      case AUTOCOMPOSIZE_QUARTERSCREEN    :  DestWidth = ScreenWidth/4;    DestHeight = ScreenHeight/4;      break;
   }
   qreal DestGeometry = DestHeight/DestWidth;

   // Make adjustement if it's not an image and geometry is locked
   if (ImageType == IMAGETYPE_UNKNOWN && BackgroundBrush->LockGeometry()) 
   {
      if ((ImageHeight*(height()/width())) < DestHeight)
         DestHeight = DestHeight*(height()/width());
      else
         DestWidth = DestWidth/(height()/width());
   }
   // Make adjustement if it's an image and ImageGeometry!=DestGeometry
   if (ImageType!=IMAGETYPE_UNKNOWN && ImageGeometry!=DestGeometry) 
   {
      if ((ImageHeight*(DestWidth/ImageWidth)) < DestHeight)    
         DestHeight = ImageHeight*(DestWidth/ImageWidth);
      else
         DestWidth = ImageWidth*(DestHeight/ImageHeight);
    }

    // Apply destination size to Composition object
    wsRect.setWidth(DestWidth/ScreenWidth);
    wsRect.setHeight(DestHeight/ScreenHeight);
    if (AllowMove) 
    {
        wsRect.setX(( (ScreenWidth-DestWidth)/2) / ScreenWidth);
        wsRect.setY(( (ScreenHeight-DestHeight)/2) / ScreenHeight);
    }
    RotateZAxis = 0;
    RotateXAxis = 0;
    RotateYAxis = 0;
}                 

void cCompositionObject::getMovementFactors(int AutoCompoStyle,int ffDProjectGeometry, qreal *xFactor, qreal *yFactor)
{
   int ImageType = BackgroundBrush->GetImageType();

   // Calc screen size
   qreal ScreenWidth  = qreal(ffDProjectGeometry == GEOMETRY_THUMBNAIL ? 600 : 1920);
   qreal ScreenHeight = qreal(ffDProjectGeometry == GEOMETRY_THUMBNAIL ? 800 : ffDProjectGeometry == GEOMETRY_4_3 ? 1440 : ffDProjectGeometry == GEOMETRY_16_9 ? 1080 : ffDProjectGeometry == GEOMETRY_40_17 ? 816 : 1920);
   //qreal ScreenGeometry    =ScreenHeight/ScreenWidth;

   // Calc real image size (if it's and image)
   qreal RealImageWidth  = qreal(BackgroundBrush->MediaObject ? BackgroundBrush->MediaObject->width() : ScreenWidth);
   qreal RealImageHeight = qreal(BackgroundBrush->MediaObject ? BackgroundBrush->MediaObject->height() : ScreenHeight);
   if (BackgroundBrush->isVideo() && RealImageWidth == 1920 && RealImageHeight == 1088 && pAppConfig->Crop1088To1080) 
      RealImageHeight = 1080;

   // Calc brush size
   qreal CanvasSize        = sqrt(RealImageWidth*RealImageWidth+RealImageHeight*RealImageHeight);   // Calc hypothenuse of the image to define full canvas
   //qreal ImageWidth         =qRound(CanvasSize*BackgroundBrush->ZoomFactor);
   //qreal ImageHeight        =qRound(CanvasSize*BackgroundBrush->ZoomFactor*BackgroundBrush->AspectRatio);
   qreal ImageWidth        = CanvasSize*BackgroundBrush->ZoomFactor();
   qreal ImageHeight       = CanvasSize*BackgroundBrush->ZoomFactor()*BackgroundBrush->AspectRatio();
   qreal ImageGeometry     = DoubleRound(ImageHeight/ImageWidth,4);

   // Calc destination size
   qreal DestWidth = ScreenWidth;
   qreal DestHeight= ScreenHeight;
   switch (AutoCompoStyle) 
   {
      case AUTOCOMPOSIZE_CUSTOM           :  DestWidth = ScreenWidth*width();    DestHeight = ScreenHeight*height();      break;      // Keep current value
      case AUTOCOMPOSIZE_REALSIZE         :  DestWidth = ImageWidth;       DestHeight = ImageHeight;         break;
      case AUTOCOMPOSIZE_FULLSCREEN       :  DestWidth = ScreenWidth;      DestHeight = ScreenHeight;        break;
      case AUTOCOMPOSIZE_TVMARGINS        :  DestWidth = ScreenWidth*0.9;  DestHeight = ScreenHeight*0.9;    break;      // TV Margins is 5% each
      case AUTOCOMPOSIZE_TWOTHIRDSSCREEN  :  DestWidth = 2*ScreenWidth/3;  DestHeight = 2*ScreenHeight/3;    break;
      case AUTOCOMPOSIZE_HALFSCREEN       :  DestWidth = ScreenWidth/2;    DestHeight = ScreenHeight/2;      break;
      case AUTOCOMPOSIZE_THIRDSCREEN      :  DestWidth = ScreenWidth/3;    DestHeight = ScreenHeight/3;      break;
      case AUTOCOMPOSIZE_QUARTERSCREEN    :  DestWidth = ScreenWidth/4;    DestHeight = ScreenHeight/4;      break;
   }
   qreal DestGeometry = DestHeight/DestWidth;

   // Make adjustement if it's not an image and geometry is locked
   if (ImageType == IMAGETYPE_UNKNOWN && BackgroundBrush->LockGeometry()) 
   {
      if ((ImageHeight*(height()/width())) < DestHeight)
         DestHeight = DestHeight*(height()/width());
      else
         DestWidth = DestWidth/(height()/width());
   }
   // Make adjustement if it's an image and ImageGeometry!=DestGeometry
   if (ImageType!=IMAGETYPE_UNKNOWN && ImageGeometry!=DestGeometry) 
   {
      if ((ImageHeight*(DestWidth/ImageWidth)) < DestHeight)    
         DestHeight = ImageHeight*(DestWidth/ImageWidth);
      else
         DestWidth = ImageWidth*(DestHeight/ImageHeight);
    }

    // Apply destination size to Composition object
    
    *xFactor = (RealImageWidth/CanvasSize) / (DestWidth/ScreenWidth);
    *yFactor = (RealImageHeight/CanvasSize) / (DestHeight/ScreenHeight);
    //wsRect.setWidth(DestWidth/ScreenWidth);
    //wsRect.setHeight(DestHeight/ScreenHeight);
}

//====================================================================================================================

QString cCompositionObject::GetBlockShapeStyle() 
{
   return  QString("###BackgroundForm:%1").arg(BackgroundForm)+
      QString("###PenSize:%1").arg(PenSize)+
      QString("###PenStyle:%1").arg(PenStyle)+
      QString("###FormShadow:%1").arg(FormShadow)+
      QString("###FormShadowDistance:%1").arg(FormShadowDistance)+
      QString("###Opacity:%1").arg(Opacity)+
      "###PenColor:"+PenColor+
      "###FormShadowColor:"+FormShadowColor;
}

void cCompositionObject::ApplyBlockShapeStyle(QString StyleDef) 
{
   QStringList List = StyleDef.split("###");

   // Apply Style
   for (auto s:List)
   {
      if (s.startsWith("BackgroundForm:"))     BackgroundForm = s.mid(QString("BackgroundForm:").length()).toInt();
      else if (s.startsWith("PenSize:"))            PenSize = s.mid(QString("PenSize:").length()).toInt();
      else if (s.startsWith("PenStyle:"))           PenStyle = s.mid(QString("PenStyle:").length()).toInt();
      else if (s.startsWith("FormShadow:"))         FormShadow = s.mid(QString("FormShadow:").length()).toInt();
      else if (s.startsWith("FormShadowDistance:")) FormShadowDistance = s.mid(QString("FormShadowDistance:").length()).toInt();
      else if (s.startsWith("Opacity:"))            Opacity = s.mid(QString("Opacity:").length()).toInt();
      else if (s.startsWith("PenColor:"))           PenColor = s.mid(QString("PenColor:").length());
      else if (s.startsWith("FormShadowColor:"))    FormShadowColor = s.mid(QString("FormShadowColor:").length());
   }
}

//====================================================================================================================

QString cCompositionObject::GetTextStyle() 
{
   return  QString("FontSize:%1").arg(tParams.FontSize) +
      QString("###HAlign:%1").arg(tParams.HAlign) +
      QString("###VAlign:%1").arg(tParams.VAlign) +
      QString("###StyleText:%1").arg(tParams.StyleText)+
      "###FontColor:"+tParams.FontColor+
      "###FontShadowColor:"+tParams.FontShadowColor+
      QString("###Bold:%1").arg(tParams.IsBold?1:0)+
      QString("###Italic:%1").arg(tParams.IsItalic?1:0)+
      QString("###Underline:%1").arg(tParams.IsUnderline?1:0)+
      "###FontName:"+tParams.FontName;
}

void cCompositionObject::ApplyTextStyle(QString StyleDef) 
{
   QStringList List=StyleDef.split("###");

   // Apply Style
   for (const auto& s : List)
   {
      if      (s.startsWith("FontSize:"))        tParams.FontSize       = s.mid(QString("FontSize:").length()).toInt();
      else if (s.startsWith("HAlign:"))          tParams.HAlign         = s.mid(QString("HAlign:").length()).toInt();
      else if (s.startsWith("VAlign:"))          tParams.VAlign         = s.mid(QString("VAlign:").length()).toInt();
      else if (s.startsWith("StyleText:"))       tParams.StyleText      = s.mid(QString("StyleText:").length()).toInt();
      else if (s.startsWith("FontColor:"))       tParams.FontColor      = s.mid(QString("FontColor:").length());
      else if (s.startsWith("FontShadowColor:")) tParams.FontShadowColor= s.mid(QString("FontShadowColor:").length());
      else if (s.startsWith("Bold:"))            tParams.IsBold         = s.mid(QString("Bold:").length()).toInt()==1;
      else if (s.startsWith("Italic:"))          tParams.IsItalic       = s.mid(QString("Italic:").length()).toInt()==1;
      else if (s.startsWith("Underline:"))       tParams.IsUnderline    = s.mid(QString("Underline:").length()).toInt()==1;
      else if (s.startsWith("FontName:"))        tParams.FontName       = s.mid(QString("FontName:").length());
   }

    // Apply to html text
    QTextDocument       TextDoc;
    QFont               Font = tParams.font();//QFont(FontName,FontSize,IsBold?QFont::Bold:QFont::Normal,IsItalic?QFont::StyleItalic:QFont::StyleNormal);
    QTextOption         OptionText(tParams.alignment()/* (HAlign == 0) ? Qt::AlignLeft : (HAlign == 1) ? Qt::AlignHCenter : (HAlign == 2) ? Qt::AlignRight : Qt::AlignJustify*/);
    QTextCursor         Cursor(&TextDoc);
    QTextCharFormat     TCF;
    QTextBlockFormat    TBF;
    TextDoc.setHtml(tParams.Text);
    Cursor.select(QTextCursor::Document);
    OptionText.setWrapMode(QTextOption::WordWrap);
    Font.setUnderline(tParams.IsUnderline);
    TextDoc.setDefaultFont(Font);
    TextDoc.setDefaultTextOption(OptionText);
    TCF.setFont(Font);
    TCF.setFontWeight(tParams.IsBold?QFont::Bold:QFont::Normal);
    TCF.setFontItalic(tParams.IsItalic);
    TCF.setFontUnderline(tParams.IsUnderline);
    TCF.setForeground(QBrush(QColor(tParams.FontColor)));
    TBF.setAlignment(tParams.alignment() /*(HAlign == 0) ? Qt::AlignLeft : (HAlign == 1) ? Qt::AlignHCenter : (HAlign == 2) ? Qt::AlignRight : Qt::AlignJustify*/);
    Cursor.setCharFormat(TCF);
    Cursor.setBlockFormat(TBF);
    tParams.Text = TextDoc.toHtml();
    resolvedText.clear();

}

//====================================================================================================================

QString cCompositionObject::GetBackgroundStyle() 
{
   return  QString("BrushType:%1").arg(BackgroundBrush->BrushType)+
      QString("###PatternType:%1").arg(BackgroundBrush->PatternType)+
      QString("###GradientOrientation:%1").arg(BackgroundBrush->GradientOrientation)+
      "###ColorD:"+BackgroundBrush->ColorD+
      "###ColorF:"+BackgroundBrush->ColorF+
      "###ColorIntermed:"+BackgroundBrush->ColorIntermed+
      QString("###Intermediate:%1").arg(BackgroundBrush->Intermediate,0,'e')+
      "###BrushImage:"+BackgroundBrush->BrushImage;
}

void cCompositionObject::ApplyBackgroundStyle(QString StyleDef) 
{
   QStringList List=StyleDef.split("###");

   // Apply Style
   for (const auto& s : List)
   {
      if      (s.startsWith("BrushType:"))            BackgroundBrush->BrushType           = s.mid(QString("BrushType:").length()).toInt();
      else if (s.startsWith("PatternType:"))          BackgroundBrush->PatternType         = s.mid(QString("PatternType:").length()).toInt();
      else if (s.startsWith("GradientOrientation:"))  BackgroundBrush->GradientOrientation = s.mid(QString("GradientOrientation:").length()).toInt();
      else if (s.startsWith("ColorD:"))               BackgroundBrush->ColorD              = s.mid(QString("ColorD:").length());
      else if (s.startsWith("ColorF:"))               BackgroundBrush->ColorF              = s.mid(QString("ColorF:").length());
      else if (s.startsWith("ColorIntermed:"))        BackgroundBrush->ColorIntermed       = s.mid(QString("ColorIntermed:").length());
      else if (s.startsWith("Intermediate:"))         BackgroundBrush->Intermediate        = GetDoubleValue(s.mid(QString("Intermediate:").length()));
      else if (s.startsWith("BrushImage:"))           BackgroundBrush->BrushImage          = s.mid(QString("BrushImage:").length());
   }
}

//====================================================================================================================

/* never used:
QString cCompositionObject::GetCoordinateStyle() 
{
   QString Style = QString("###X:%1").arg(x, 0, 'e') +
      QString("###Y:%1").arg(y, 0, 'e')+
      QString("###W:%1").arg(w, 0, 'e')+
      QString("###H:%1").arg(h, 0, 'e')+
      QString("###RotateZAxis:%1").arg( RotateZAxis, 0, 'e')+
      QString("###RotateXAxis:%1").arg( RotateXAxis, 0, 'e')+
      QString("###RotateYAxis:%1").arg( RotateYAxis, 0, 'e');
   return Style;
}
*/

/* never used:
void cCompositionObject::ApplyCoordinateStyle(QString StyleDef) 
{
   QStringList List = StyleDef.split("###");
   bool RecalcAspectRatio = true;

   // Apply Style
   for (int i = 0; i < List.count(); i++) 
   {
      if      (List[i].startsWith("X:"))           x               = GetDoubleValue(List[i].mid(QString("X:").length()));
      else if (List[i].startsWith("Y:"))           y               = GetDoubleValue(List[i].mid(QString("Y:").length()));
      else if (List[i].startsWith("W:"))           w               = GetDoubleValue(List[i].mid(QString("W:").length()));
      else if (List[i].startsWith("H:"))           h               = GetDoubleValue(List[i].mid(QString("H:").length()));
      else if (List[i].startsWith("RotateZAxis:")) RotateZAxis     = GetDoubleValue(List[i].mid(QString("RotateZAxis:").length()));
      else if (List[i].startsWith("RotateXAxis:")) RotateXAxis     = GetDoubleValue(List[i].mid(QString("RotateXAxis:").length()));
      else if (List[i].startsWith("RotateYAxis:")) RotateYAxis     = GetDoubleValue(List[i].mid(QString("RotateYAxis:").length()));
   }
   // if not set by style then compute Aspect Ratio
   if (RecalcAspectRatio) 
   {
      double DisplayW = 1920, DisplayH = 1080;
      if (((MainWindow *)ApplicationConfig->TopLevelWindow)->Diaporama->ImageGeometry==GEOMETRY_4_3)        { DisplayW = 1440; DisplayH = 1080; }
      else if (((MainWindow *)ApplicationConfig->TopLevelWindow)->Diaporama->ImageGeometry==GEOMETRY_16_9)  { DisplayW = 1920; DisplayH = 1080; }
      else if (((MainWindow *)ApplicationConfig->TopLevelWindow)->Diaporama->ImageGeometry==GEOMETRY_40_17) { DisplayW = 1920; DisplayH = 816;  }
      BackgroundBrush->AspectRatio = (h*DisplayH)/(w*DisplayW);                                                           
   }
}
*/
//====================================================================================================================

void cCompositionObject::CopyBlockPropertiesTo(cCompositionObject *DestBlock)
{
   if (this == DestBlock || DestBlock == NULL) 
      return;

   // Attribut of the text part
   DestBlock->tParams = tParams;
   DestBlock->resolvedText.clear();

   // Attribut of the shap part
   DestBlock->BackgroundForm     = BackgroundForm;        // Type of the form : 0=None, 1=Rectangle, 2=RoundRect, 3=Buble, 4=Ellipse, 5=Triangle UP (Polygon)
   DestBlock->PenSize            = PenSize;               // Width of the border of the form
   DestBlock->PenStyle           = PenStyle;              // Style of the pen border of the form
   DestBlock->PenColor           = PenColor;              // Color of the border of the form
   DestBlock->FormShadow         = FormShadow;            // 0=none, 1=shadow up-left, 2=shadow up-right, 3=shadow bt-left, 4=shadow bt-right
   DestBlock->FormShadowDistance = FormShadowDistance;    // Distance from form to shadow
   DestBlock->FormShadowColor    = FormShadowColor;       // 0=none, 1=shadow up-left, 2=shadow up-right, 3=shadow bt-left, 4=shadow bt-right
   DestBlock->Opacity            = Opacity;               // Opacity of the form

   // Attribut of the BackgroundBrush of the shap part
   DestBlock->BackgroundBrush->BrushType           = BackgroundBrush->BrushType;
   DestBlock->BackgroundBrush->PatternType         = BackgroundBrush->PatternType;
   DestBlock->BackgroundBrush->GradientOrientation = BackgroundBrush->GradientOrientation;
   DestBlock->BackgroundBrush->ColorD              = BackgroundBrush->ColorD;
   DestBlock->BackgroundBrush->ColorF              = BackgroundBrush->ColorF;
   DestBlock->BackgroundBrush->ColorIntermed       = BackgroundBrush->ColorIntermed;
   DestBlock->BackgroundBrush->Intermediate        = BackgroundBrush->Intermediate;
   DestBlock->BackgroundBrush->BrushImage          = BackgroundBrush->BrushImage;
   //cCompositionObjectMask *pMask = CompositionObjectToCopy->getMask();
   //cCompositionObjectMask *pMask = getMask();
   //if( useMask )
   //{
   //   DestBlock->getMask()->copyFrom(&mask);
   //   DestBlock->setUseMask(true);
   //}
   DestBlock->setUse3DFrame(use3dFrame);
}

void cCompositionObject::CopyBlockProperties(cCompositionObject *SourceBlock,cCompositionObject *DestBlock) 
{
   if (SourceBlock == DestBlock) 
      return;

   SourceBlock->CopyBlockPropertiesTo(DestBlock);
}

//====================================================================================================================

void cCompositionObject::CopyFromCompositionObject(cCompositionObject *CompositionObjectToCopy) 
{
   IsVisible            = CompositionObjectToCopy->IsVisible;
   wsRect               = CompositionObjectToCopy->wsRect;
   RotateZAxis          = CompositionObjectToCopy->RotateZAxis;
   RotateXAxis          = CompositionObjectToCopy->RotateXAxis;
   RotateYAxis          = CompositionObjectToCopy->RotateYAxis;
   BlockSpeedWave       = CompositionObjectToCopy->BlockSpeedWave;
   if (!((CompositionObjectToCopy->BlockAnimType == BLOCKANIMTYPE_DISSOLVE) && ((CompositionObjectToCopy->Dissolve == eAPPEAR) || (CompositionObjectToCopy->Dissolve == eDISAPPEAR)))) 
   {
      BlockAnimType        = CompositionObjectToCopy->BlockAnimType;
      Dissolve             = CompositionObjectToCopy->Dissolve;
   }
   TurnZAxis            = CompositionObjectToCopy->TurnZAxis;
   TurnXAxis            = CompositionObjectToCopy->TurnXAxis;
   TurnYAxis            = CompositionObjectToCopy->TurnYAxis;
   ZRotationInfo = CompositionObjectToCopy->ZRotationInfo;
   Opacity              = CompositionObjectToCopy->Opacity;
   BackgroundForm       = CompositionObjectToCopy->BackgroundForm;
   PenSize              = CompositionObjectToCopy->PenSize;
   PenStyle             = CompositionObjectToCopy->PenStyle;
   PenColor             = CompositionObjectToCopy->PenColor;
   FormShadowColor      = CompositionObjectToCopy->FormShadowColor;
   FormShadow           = CompositionObjectToCopy->FormShadow;
   FormShadowDistance   = CompositionObjectToCopy->FormShadowDistance;
   tParams = CompositionObjectToCopy->tParams;
   resolvedText.clear();
   tShotParams = CompositionObjectToCopy->tShotParams;
   BackgroundBrush->CopyFromBrushDefinition(CompositionObjectToCopy->BackgroundBrush);
   cCompositionObjectMask *pMask = CompositionObjectToCopy->getMask();
   //cCompositionObjectMask *pMask = CompositionObjectToCopy->mask();
   if( CompositionObjectToCopy->useMask )
   {
      useMask = true;
      mask.copyFrom(pMask);
   }
   useTextMask = CompositionObjectToCopy->useTextMask;
   if( CompositionObjectToCopy->use3dFrame )
      use3dFrame = true;
}

//====================================================================================================================
#define NEWDRAW
#define noSEP_TEXT_DRAW
void cCompositionObject::DrawCompositionObjectText(cDiaporamaObject *Object, QPainter *Painter, double  ADJUST_RATIO, QSizeF size, bool /*PreviewMode*/, int64_t /*Position*/,
   cSoundBlockList * /*SoundTrackMontage*/, double /*BlockPctDone*/, double /*ImagePctDone*/, cCompositionObject * /*PreviousCompositionObject*/,
   int64_t /*ShotDuration*/, bool /*EnableAnimation*/,
   bool Transfo , QRectF NewRect,
   bool DisplayTextMargin , cCompositionObjectContext *PreparedBrush)
{
   double width = size.width();
   double height = size.height();

   QRectF              TheRect;
   //double              TheRotateZAxis, TheRotateXAxis, TheRotateYAxis;
   double              TheTxtZoomLevel, TheTxtScrollX, TheTxtScrollY;
   QRectF              orgRect;
   //double              DestOpacity;
   //QList<QPolygonF>    PolygonList;
   QRectF              ShapeRect;
   //// mask
   QRectF maskRect;
   ////int maskBackgroundForm = -1;
   double maskRotateZAxis = 0;
   //QPainterPath textClippingPath;
   QTransform  Matrix; // should be param


   if (PreparedBrush)
   {
      TheRect = PreparedBrush->curRect;
      TheTxtZoomLevel = PreparedBrush->curTxtZoomLevel;
      TheTxtScrollX = PreparedBrush->curTxtScrollX;
      TheTxtScrollY = PreparedBrush->curTxtScrollY;
      orgRect = PreparedBrush->orgRect;
      ShapeRect = PreparedBrush->ShapeRect;

      maskRect = PreparedBrush->maskRect;
      //maskBackgroundForm = PreparedBrush->maskBackgroundForm;
      maskRotateZAxis = PreparedBrush->maskRotateZAxis;
   }
   else
   {
      // Define values depending on BlockPctDone and PrevCompoObject
      TheRect = Transfo ? NewRect : getRect();
      TheTxtZoomLevel = tShotParams.TxtZoomLevel;
      TheTxtScrollX = tShotParams.TxtScrollX;
      TheTxtScrollY = tShotParams.TxtScrollY;
      orgRect = ::appliedRect(TheRect, width, height);

      //***********************************************************************************
      // Compute shape
      //***********************************************************************************
      QList<QPolygonF> PolygonList = ComputePolygon(BackgroundForm, orgRect);
      //qDebug() << "calc poly for " << orgRect << "gives " << PolygonList.at(0) << " with " << PolygonList.at(0).boundingRect();
      ShapeRect = PolygonToRectF(PolygonList);
   }
   QPointF CenterF(ShapeRect.center().x(), ShapeRect.center().y());

   //**********************************************************************************
   // Text part
   //**********************************************************************************
   if (TheTxtZoomLevel > 0 && !IsTextEmpty)
   {
      #ifdef NEWDRAW
      Matrix.translate(CenterF.x(), CenterF.y());      // Translate back to origin
      #endif
      Painter->setWorldTransform(Matrix, false);
      //AUTOTIMER(tt,"drawText");
      if (resolvedText.isEmpty())
      {
         resolvedText = Variable.ResolveTextVariable(Object, tParams.Text);
      }
      QTextDocument   TextDocument;
      QTextDocument   *textDocument = &TextDocument;
      textDocument->setHtml(resolvedText);

      double FullMargin = ((tParams.TMType == eFULLSHAPE) || (tParams.TMType == eCUSTOM)) ? 0 : double(PenSize)*ADJUST_RATIO / double(2);
      QRectF TextMargin;
      double PointSize = (width / double(SCALINGTEXTFACTOR));

      if (tParams.TMType == eFULLSHAPE || tParams.TMType == eCUSTOM)
         TextMargin = ::appliedRect(tParams.textMargins, TheRect.width() * width, TheRect.height() * height);
      else if (tParams.TextClipArtName != "")
      {
         cTextFrameObject *TFO = &TextFrameList.List[TextFrameList.SearchImage(tParams.TextClipArtName)];
         TextMargin = QRectF(TFO->TMx*TheRect.width()*width + FullMargin, TFO->TMy*TheRect.height()*height + FullMargin,
            TFO->TMw*TheRect.width()*width - FullMargin * 2, TFO->TMh*TheRect.height()*height - FullMargin * 2);
      }
      else
      {
         TextMargin = QRectF(ShapeFormDefinition[BackgroundForm].TMx*TheRect.width()*width + FullMargin,
            ShapeFormDefinition[BackgroundForm].TMy*TheRect.height()*height + FullMargin,
            ShapeFormDefinition[BackgroundForm].TMw*TheRect.width()*width - FullMargin * 2,
            ShapeFormDefinition[BackgroundForm].TMh*TheRect.height()*height - FullMargin * 2);
      }
      TextMargin.translate(-ShapeRect.width() / 2, -ShapeRect.height() / 2);

      if (DisplayTextMargin)
      {
         QPen PP(Qt::blue);
         PP.setStyle(Qt::DotLine);
         PP.setWidth(1);
         Painter->setPen(PP);
         Painter->setBrush(Qt::NoBrush);
         Painter->drawRect(TextMargin);
      }

      Painter->setClipRect(TextMargin);
      if (useMask)
      {
         QPainterPath clipPath;
         // calculate mask-rect
         QRectF realMaskRect;
         bool inverted = mask.invertedMask();
         realMaskRect = WSRect(maskRect).applyTo(orgRect.width(), orgRect.height());
         // get the form-polygon
         QList<QPolygonF> masking = ComputePolygon(BackgroundForm, realMaskRect);

         // setup rotation (Z-Axis Center-Rotation)
         QTransform tr;
         if (maskRotateZAxis != 0)
         {
            QPointF maskCenter = realMaskRect.center();
            tr.translate(maskCenter.x(), maskCenter.y());
            tr.rotate(maskRotateZAxis/*mask->zAngle()*/);
            tr.translate(-maskCenter.x(), -maskCenter.y());
            for (int i = 0; i < masking.count(); i++)
            {
               if (maskRotateZAxis)
                  masking[i] = tr.map(masking[i]);
            }
         }
         //qDebug() << "masking: " << masking;
         for (int i = 0; i < masking.count(); i++)
            clipPath.addPolygon(masking.at(i));
         if (inverted)
         {
            QPainterPath rclipPath;
            //rclipPath.addRect(realMaskRect.toRect());
            rclipPath.addRect(TextMargin.translated(ShapeRect.width() / 2, ShapeRect.height() / 2).toRect());
            rclipPath -= clipPath;
            clipPath = rclipPath;
         }
         clipPath.translate(-ShapeRect.width() / 2, -ShapeRect.height() / 2);
         //qDebug() << "masking: " << clipPath;
         Painter->setClipPath(clipPath, Qt::IntersectClip);
      }


      textDocument->setTextWidth(TextMargin.width() / PointSize);
      QRectF  FmtBdRect(0, 0,
         double(textDocument->documentLayout()->documentSize().width())*(TheTxtZoomLevel / 100)*PointSize,
         double(textDocument->documentLayout()->documentSize().height())*(TheTxtZoomLevel / 100)*PointSize);

      int     MaxH = TextMargin.height() > FmtBdRect.height() ? TextMargin.height() : FmtBdRect.height();
      double  DecalX = (TheTxtScrollX / 100) * TextMargin.width() + TextMargin.center().x() - TextMargin.width() / 2 + (TextMargin.width() - FmtBdRect.width()) / 2;
      double  DecalY = (-TheTxtScrollY / 100) * MaxH + TextMargin.center().y() - TextMargin.height() / 2;

      if (tParams.VAlign == 0);                                        //Qt::AlignTop (Nothing to do)
      else if (tParams.VAlign == 1)
         DecalY = DecalY + (TextMargin.height() - FmtBdRect.height()) / 2;       //Qt::AlignVCenter
      else
         DecalY = DecalY + (TextMargin.height() - FmtBdRect.height());         //Qt::AlignBottom)

      QAbstractTextDocumentLayout::PaintContext Context;

      QTextCursor Cursor(textDocument);
      QTextCharFormat TCF;

      Cursor.select(QTextCursor::Document);
      if (tParams.StyleText == 1)
      {
         // Add outline for painting
         TCF.setTextOutline(QPen(QColor(tParams.FontShadowColor)));
         Cursor.mergeCharFormat(TCF);
      }
      else if (tParams.StyleText != 0)
      {
         // Paint shadow of the text
         TCF.setForeground(QBrush(QColor(tParams.FontShadowColor)));
         Cursor.mergeCharFormat(TCF);
         Painter->save();
         switch (tParams.StyleText)
         {
            case 2: Painter->translate(DecalX - 1, DecalY - 1);   break;  //2=shadow up-left
            case 3: Painter->translate(DecalX + 1, DecalY - 1);   break;  //3=shadow up-right
            case 4: Painter->translate(DecalX - 1, DecalY + 1);   break;  //4=shadow bt-left
            case 5: Painter->translate(DecalX + 1, DecalY + 1);   break;  //5=shadow bt-right
         }
         Painter->scale((TheTxtZoomLevel / 100)*PointSize, (TheTxtZoomLevel / 100)*PointSize);
         textDocument->documentLayout()->draw(Painter, Context);
         Painter->restore();
         textDocument->setHtml(resolvedText);     // Restore Text Document
      }
      Painter->save();
      //Painter->setCompositionMode(QPainter::CompositionMode_Clear);
      Painter->translate(DecalX, DecalY);
      Painter->scale((TheTxtZoomLevel / 100)*PointSize, (TheTxtZoomLevel / 100)*PointSize);
      //qDebug() << "scale painter for text to " << (TheTxtZoomLevel/100)*PointSize << " with PointSize = " << PointSize << " for width = " << width;
      textDocument->documentLayout()->draw(Painter, Context);
      Painter->restore();
   }
}

// ADJUST_RATIO=Adjustement ratio for pixel size (all size are given for full hd and adjust for real wanted size)
void cCompositionObject::DrawCompositionObject(cDiaporamaObject *Object, QPainter *DestPainter, double  ADJUST_RATIO, QSizeF size, bool PreviewMode, int64_t Position,
                                               cSoundBlockList *SoundTrackMontage, double BlockPctDone, double ImagePctDone, cCompositionObject *PrevCompoObject,
                                               int64_t ShotDuration, bool EnableAnimation,
                                               bool Transfo, QRectF NewRect, bool DisplayTextMargin, cCompositionObjectContext *PreparedBrush) 
{
//   static int puzzleCount = 0;
   //AUTOTIMER(at,"DrawCompositionObject");
   //autoTimer at("DrawCompositionObject");
   double width = size.width();
   double height = size.height();
#ifdef LOG_DRAWCOMPO
   qDebug() << "cCompositionObject::DrawCompositionObject";
   qDebug() << "ADJUST_RATIO " << ADJUST_RATIO << " width " << width << " height " << height << "PreviewMode " << PreviewMode;
   qDebug() << "Position " << Position << " BlockPctDone " << BlockPctDone << " ImagePctDone " << ImagePctDone << " ShotDuration " << ShotDuration << " EnableAnimation " << EnableAnimation;
   qDebug() << "Transfo " << Transfo;
   qDebug() << "NewRect " << NewRect;
   qDebug() << "PreparedBrush " << (void *) PreparedBrush;
#endif
   bool bIsSameAsPrev = false;
   if( PrevCompoObject )
      bIsSameAsPrev = sameAs(PrevCompoObject);
#ifdef LOG_DRAWCOMPO
   qDebug() << "Type " << TypeComposition << " index " << IndexKey << " SameAsPrev = " << bIsSameAsPrev;
   QTime t;
   t.start();
#endif

   if (!IsVisible) 
      return;

   // W and H = 0 when producing sound track in render process
   bool SoundOnly = (DestPainter == NULL) || (width == 0) || (height == 0) || (Transfo && NewRect.size().isEmpty() ) || (!Transfo && this->size().isEmpty() );

   if (SoundOnly) 
   {
      // if SoundOnly then load Brush of type BRUSHTYPE_IMAGEDISK to SoundTrackMontage
      if (BackgroundBrush->BrushType == BRUSHTYPE_IMAGEDISK && SoundTrackMontage != NULL) 
      {
         QBrush *BR = BackgroundBrush->GetBrush(QSizeF(0,0),PreviewMode,Position,SoundTrackMontage,ImagePctDone,NULL);
         if (BR) 
            delete BR;
      }
      // nothing more to do
      return;
   } 

   if (IsFullScreen) 
   {
#ifdef LOG_DRAWCOMPO
      QTime t;
      t.start();
#endif
      QRectF imageSrcRect;
      QImage Img = BackgroundBrush->GetImageDiskBrush(wsRect.appliedSize(width,height),PreviewMode,Position,SoundTrackMontage,ImagePctDone,PrevCompoObject ? PrevCompoObject->BackgroundBrush : NULL, &imageSrcRect);
      //qDebug() << "srcRect after GetImageDiskBrush " << imageSrcRect << " for image " << Img.size();
      if (!Img.isNull()) 
         DestPainter->drawImage(wsRect.appliedPos(width,height),Img,imageSrcRect);
#ifdef LOG_DRAWCOMPO
      qDebug() << "GetImageDiskBrush takes " << t.elapsed() << " mSec";
#endif
      // nothing more to do

      return;
   } 

   QRectF              TheRect;
   double              TheRotateZAxis,TheRotateXAxis,TheRotateYAxis;
   double              TheTxtZoomLevel,TheTxtScrollX,TheTxtScrollY;
   QRectF              orgRect;
   double              DestOpacity;
   QList<QPolygonF>    PolygonList;
   QRectF              ShapeRect;
   // mask
   QRectF maskRect;
   //int maskBackgroundForm = -1;
   double maskRotateZAxis = 0; 
   QPainterPath textClippingPath;

   if (PreparedBrush) 
   {
      TheRect         = PreparedBrush->curRect;
      TheRotateZAxis  = PreparedBrush->curRotateZAxis;
      TheRotateXAxis  = PreparedBrush->curRotateXAxis;
      TheRotateYAxis  = PreparedBrush->curRotateYAxis;
      TheTxtZoomLevel = PreparedBrush->curTxtZoomLevel;
      TheTxtScrollX   = PreparedBrush->curTxtScrollX;
      TheTxtScrollY   = PreparedBrush->curTxtScrollY;
      orgRect         = PreparedBrush->orgRect;
      DestOpacity     = PreparedBrush->DestOpacity;
      PolygonList     = PreparedBrush->PolygonList;
      ShapeRect       = PreparedBrush->ShapeRect;

      maskRect = PreparedBrush->maskRect;
      //maskBackgroundForm = PreparedBrush->maskBackgroundForm;
      maskRotateZAxis = PreparedBrush->maskRotateZAxis;
   } 
   else 
   {
      // Define values depending on BlockPctDone and PrevCompoObject
      TheRect          = Transfo ? NewRect : getRect();
      TheRotateZAxis   = RotateZAxis + (EnableAnimation && (BlockAnimType == BLOCKANIMTYPE_MULTIPLETURN) ? 360*TurnZAxis : 0);
      TheRotateXAxis   = RotateXAxis + (EnableAnimation && (BlockAnimType == BLOCKANIMTYPE_MULTIPLETURN) ? 360*TurnXAxis : 0);
      TheRotateYAxis   = RotateYAxis + (EnableAnimation && (BlockAnimType == BLOCKANIMTYPE_MULTIPLETURN) ? 360*TurnYAxis : 0);
      TheTxtZoomLevel  = tShotParams.TxtZoomLevel;
      TheTxtScrollX    = tShotParams.TxtScrollX;
      TheTxtScrollY    = tShotParams.TxtScrollY;
      if( useMask )
      {
         maskRect = mask.getRect();
         //maskBackgroundForm = BackgroundForm;//mask->getBackgroundForm();
         maskRotateZAxis = mask.zAngle();
      }

      if (PrevCompoObject) 
      {
         TheRect = animValue(PrevCompoObject->getRect(), TheRect, BlockPctDone);
         QPainterPath ppath;
         ppath.moveTo(PrevCompoObject->getRect().topLeft());
         ppath.lineTo(TheRect.topLeft());
         QPointF pPoint = ppath.pointAtPercent(BlockPctDone);
         qDebug() << "pos: " << TheRect.topLeft() << " by path: " << pPoint;

         TheRotateZAxis = animDiffValue(PrevCompoObject->RotateZAxis,TheRotateZAxis,BlockPctDone);
         TheRotateXAxis = animDiffValue(PrevCompoObject->RotateXAxis,TheRotateXAxis,BlockPctDone);
         TheRotateYAxis = animDiffValue(PrevCompoObject->RotateYAxis,TheRotateYAxis,BlockPctDone);
         TheTxtZoomLevel = animDiffValue(PrevCompoObject->tShotParams.TxtZoomLevel,TheTxtZoomLevel,BlockPctDone);
         TheTxtScrollX = animDiffValue(PrevCompoObject->tShotParams.TxtScrollX,TheTxtScrollX,BlockPctDone);
         TheTxtScrollY = animDiffValue(PrevCompoObject->tShotParams.TxtScrollY,TheTxtScrollY,BlockPctDone);
         if( PrevCompoObject->getMask() )
         {
            maskRect = animValue(PrevCompoObject->getMask()->getRect(), maskRect, BlockPctDone); 
            maskRotateZAxis = animDiffValue(PrevCompoObject->getMask()->zAngle(), maskRotateZAxis, BlockPctDone);
         }
      } 
      else 
      {
         if (EnableAnimation && (BlockAnimType == BLOCKANIMTYPE_MULTIPLETURN)) 
         {
            TheRotateZAxis = animDiffValue(0, RotateZAxis + 360 * TurnZAxis, BlockPctDone);
            TheRotateXAxis = animDiffValue(0, RotateXAxis + 360 * TurnXAxis, BlockPctDone);
            TheRotateYAxis = animDiffValue(0, RotateYAxis + 360 * TurnYAxis, BlockPctDone);
         }
      }

      //**********************************************************************************

      orgRect = ::appliedRect(TheRect, width, height);
      DestOpacity = destOpacity();

      if( !orgRect.size().isEmpty() )
      {
#if 0
         orgRect.moveLeft(floor(orgRect.x()));           orgRect.moveTop(floor(orgRect.y()));
         orgRect.setWidth(floor(orgRect.width()/2)*2);   orgRect.setHeight(floor(orgRect.height()/2)*2);
#endif
         ////orgRect.moveLeft(qRound(orgRect.x()));           orgRect.moveTop(qRound(orgRect.y()));
         ////orgRect.setWidth(qRound(orgRect.width()/2)*2);   orgRect.setHeight(qRound(orgRect.height()/2)*2);
         //**********************************************************************************
         // Opacity and dissolve annimation
         //**********************************************************************************
         if (EnableAnimation) 
         {
            if (BlockAnimType == BLOCKANIMTYPE_DISSOLVE) 
            {
               double BlinkNumber = 0;
               switch (Dissolve) 
               {
                  case eAPPEAR        : DestOpacity = DestOpacity*BlockPctDone;       break;
                  case eDISAPPEAR     : DestOpacity = DestOpacity*(1-BlockPctDone);   break;
                  case eBLINK_SLOW    : BlinkNumber = 0.25;                           break;
                  case eBLINK_MEDIUM  : BlinkNumber = 0.5;                            break;
                  case eBLINK_FAST    : BlinkNumber = 1;                              break;
                  case eBLINK_VERYFAST: BlinkNumber = 2;                              break;
               }
               if (BlinkNumber != 0) 
               {
                  BlinkNumber = BlinkNumber*ShotDuration;
                  if (int(BlinkNumber/1000) != (BlinkNumber/1000)) 
                     BlinkNumber = int(BlinkNumber/1000)+1; 
                  else 
                     BlinkNumber = int(BlinkNumber/1000); // Adjust to upper 1000
                  double FullPct = BlockPctDone*BlinkNumber*100;
                  FullPct = int(FullPct)-int(FullPct/100)*100;
                  FullPct = (FullPct/100)*2;
                  if (FullPct < 1)  
                     DestOpacity = DestOpacity*(1-FullPct);
                  else        
                     DestOpacity = DestOpacity*(FullPct-1);
               }
            }
         }
         //***********************************************************************************
         // Compute shape
         //***********************************************************************************
         PolygonList = ComputePolygon(BackgroundForm,orgRect);
         //qDebug() << "calc poly for " << orgRect << "gives " << PolygonList.at(0) << " with " << PolygonList.at(0).boundingRect();
         ShapeRect   = PolygonToRectF(PolygonList);
      }
   }

   if( !orgRect.size().isEmpty() )
   {
      //AUTOTIMER(dd,"drawing");
#if QT_VERSION >= 0x060000
      QPainter::RenderHints hints = (!PreviewMode || ApplicationConfig == NULL || ApplicationConfig->Smoothing) ?
            QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform  :
            QPainter::Antialiasing | QPainter::TextAntialiasing ;
#else
      QPainter::RenderHints hints = (!PreviewMode || ApplicationConfig == NULL || ApplicationConfig->Smoothing) ?
         QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing | QPainter::NonCosmeticDefaultPen :
         QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing | QPainter::NonCosmeticDefaultPen;
#endif
      DestPainter->save();
      DestPainter->setOpacity(DestOpacity);
      DestPainter->setRenderHints(hints,true);
      DestPainter->setCompositionMode(QPainter::CompositionMode_SourceOver);

      QPainter    *Painter = DestPainter;
      QImage      ShadowImg;

      //***********************************************************************************
      // If shadow, draw all on a buffered image instead of drawing directly to destination painter
      //***********************************************************************************
      if (FormShadow) 
      {
         ShadowImg = QImage(width,height,QImage::Format_ARGB32_Premultiplied);
         Painter = new QPainter();
         Painter->begin(&ShadowImg);
         Painter->setRenderHints(hints,true);
         Painter->setCompositionMode(QPainter::CompositionMode_Source);
         Painter->fillRect(QRect(0,0,width,height),Qt::transparent);
         Painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
      }

      //***********************************************************************************
      // Prepare Transform Matrix
      //***********************************************************************************
      //QPointF CenterF(floor(ShapeRect.center().x()/2)*2, floor(ShapeRect.center().y()/2)*2);
      QPointF CenterF(ShapeRect.center().x(), ShapeRect.center().y());
      //CenterF = getRotationOrigin(ShapeRect, eRotTopRight);
      QTransform  Matrix;
      CenterF = getRotationOrigin(ShapeRect, ZRotationInfo.center);
      Matrix.translate(CenterF.x(),CenterF.y());      // Translate to be sure we are on the center of the shape
      if (TheRotateZAxis != 0) 
      {
         //Matrix.translate(ShapeRect.left(),CenterF.y());      // Translate to be sure we are on the center of the shape
         Matrix.rotate(TheRotateZAxis, Qt::ZAxis);     // Standard axis
         //Matrix.translate(-ShapeRect.left(),-CenterF.y());      // Translate to be sure we are on the center of the shape
      }
      //Matrix.translate(CenterF.x(),CenterF.y());      // Translate to be sure we are on the center of the shape
      if (TheRotateXAxis != 0) 
         Matrix.rotate(TheRotateXAxis, Qt::XAxis);     // Rotate from X axis
      if (TheRotateYAxis != 0) 
         Matrix.rotate(TheRotateYAxis, Qt::YAxis);     // Rotate from Y axis
      #ifdef NEWDRAW
      Matrix.translate(-CenterF.x(),-CenterF.y());      // Translate back to origin
      #endif
      Painter->setWorldTransform(Matrix, false);
      //qDebug() << "     painter matrix is " << Painter->worldTransform();

      if (tParams.TextClipArtName != "") 
      {
         QSvgRenderer SVGImg(tParams.TextClipArtName);
         if (!SVGImg.isValid()) 
         {
            ToLog(LOGMSG_CRITICAL,QString("IN:cCompositionObject:DrawCompositionObject: Error loading ClipArt Image %1").arg(tParams.TextClipArtName));
         } 
         else 
         {
            //SVGImg.render(Painter,QRectF(-W/2,-H/2,W,H));
            #ifdef NEWDRAW
            SVGImg.render(Painter,QRectF(orgRect.x(),orgRect.y(),orgRect.width(),orgRect.height()));
            #else
            SVGImg.render(Painter,QRectF(-orgRect.width()/2,-orgRect.height()/2,orgRect.width(),orgRect.height()));
            #endif
         }
      } 
      else if (BackgroundBrush->BrushType != BRUSHTYPE_NOBRUSH || PenSize != 0) 
      {
         #ifndef NEWDRAW         
         for (int i = 0; i < PolygonList.count(); i++) 
         {
            //qDebug() << "translate " << PolygonList[i] << " with " << CenterF;
            PolygonList[i].translate(-CenterF.x(),-CenterF.y());
            //qDebug() << "translated " << PolygonList[i];
         }
         #endif
         QRectF NewShapeRect = PolygonToRectF(PolygonList);

         //***********************************************************************************
         // Prepare pen
         //***********************************************************************************
         if (PenSize == 0) 
            Painter->setPen(Qt::NoPen); 
         else 
         {
            QPen Pen;
            Pen.setCapStyle(Qt::RoundCap);
            Pen.setJoinStyle(Qt::RoundJoin);
            Pen.setCosmetic(false);
            Pen.setStyle(Qt::SolidLine);
            Pen.setColor(PenColor);
            Pen.setWidthF(double(PenSize)*ADJUST_RATIO);
            Pen.setStyle((Qt::PenStyle)PenStyle);
            Painter->setPen(Pen);
         }

         //***********************************************************************************
         // Prepare brush
         //***********************************************************************************
         //QImage drawImage;
         QTransform MatrixBR;
         QRectF srcRect;
         if (BackgroundBrush->BrushType == BRUSHTYPE_NOBRUSH) 
            Painter->setBrush(Qt::NoBrush); 
         else 
         {
            // Create brush with filter and Ken Burns effect !
            //autoTimer *crbr = new autoTimer("Create brush with filter and Ken Burns effect !");
            QBrush *BR = BackgroundBrush->GetBrush(orgRect.size(), PreviewMode, Position, BackgroundBrush->SoundVolume != 0 ? SoundTrackMontage : NULL, ImagePctDone, PrevCompoObject ? PrevCompoObject->BackgroundBrush : NULL, &srcRect);
            MatrixBR.translate(NewShapeRect.left() + (orgRect.x() - ShapeRect.left()), NewShapeRect.top() + (orgRect.y() - ShapeRect.top()));
            //qDebug() << "srcRect after GetBrush " << srcRect<< " for image " << BR->textureImage().size();
            //delete crbr;
            //AUTOTIMER(kb,"draw brush with filter and Ken Burns effect !");
            if (BR) 
            {
               BR->setTransform(MatrixBR); // Apply transform matrix to the brush
               #ifndef NEWDRAW
               // Avoid phantom lines for image brush
               if (!BR->textureImage().isNull() && (TheRotateZAxis != 0 || TheRotateXAxis != 0 || TheRotateYAxis != 0)) 
               {
                  QImage TempImage(orgRect.width() + 4, orgRect.height()+4, QImage::Format_ARGB32_Premultiplied);
                  TempImage.fill(0);
                  QPainter TempPainter;
                  TempPainter.begin(&TempImage);
                  TempPainter.setRenderHints(hints,true);
                  //TempPainter.setCompositionMode(QPainter::CompositionMode_Source);
                  //TempPainter.fillRect(QRect(0,0,TempImage.width(),TempImage.height()),Qt::transparent);
                  //TempPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                  TempPainter.drawImage(2,2,BR->textureImage());
                  TempPainter.end();
                  delete BR;
                  BR = new QBrush(TempImage);
                  MatrixBR.translate(NewShapeRect.left()+(orgRect.x()-ShapeRect.left())-2,NewShapeRect.top()+(orgRect.y()-ShapeRect.top())-2);
               } 
               else 
               {
                  MatrixBR.translate(NewShapeRect.left()+(orgRect.x()-ShapeRect.left()),NewShapeRect.top()+(orgRect.y()-ShapeRect.top()));
               }
               BR->setTransform(MatrixBR); // Apply transform matrix to the brush
               #endif
               Painter->setBrush(*BR);
               delete BR;
            } 
            else 
            {
               ToLog(LOGMSG_CRITICAL,"Error in cCompositionObject::DrawCompositionObject Brush is NULL !");
               Painter->setBrush(Qt::NoBrush);
            }
         }
         // apply LightFilter
         bool bUseLightfilter = use3dFrame;
         if( bUseLightfilter && /*BackgroundForm > 1 && */PenSize > 0 && !BackgroundBrush->isFromPushback(PreviewMode) )
         {
            //qDebug() << "apply LightFilter, Previewmode = " << PreviewMode;
            LightFilter *f;
            QImage orgImage;
            QBrush brush = Painter->brush();
            brush.setTransform(QTransform());
            orgImage = brush.textureImage(); 
            QImage *filteredImage;
            QImage maskImage(orgImage.width(), orgImage.height(), QImage::Format_ARGB32);
            maskImage.fill(Qt::white);
            QRectF realMaskRect = QRectF(0,0,orgImage.width(), orgImage.height());//WSRect(maskRect).applyTo(orgRect);
            //QRectF realMaskRect = QRectF(1,1,orgImage.width()-2, orgImage.height()-2);//WSRect(maskRect).applyTo(orgRect);
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
            Pen.setColor(Qt::black);
            Pen.setWidthF(double(PenSize)*ADJUST_RATIO);
            p.setPen(Pen);
            p.setBrush(Qt::white);
            //p.setRenderHints(hints,true);

            for (int i = 0; i < masking.count(); i++) 
               p.drawPolygon(masking.at(i));
            p.end();

            f = new LightFilter();
            //f->set
            f->setBumpShape(2);
            f->setBumpHeight(0.5);
            f->setBumpSoftness(2);
            f->setBumpSource(LightFilter::BUMPS_FROM_MAP);
            f->setColorSource(LightFilter::COLORS_FROM_IMAGE);
            ImageFunction2D *bumpfunction = new ImageFunction2D(maskImage);
            f->setBumpFunction((Function2D*)bumpfunction);
            f->getLights().at(0)->setElevation(11.0f * 3.1415927f / 180.0f);
            filteredImage = f->filter(orgImage, 0);
            BackgroundBrush->pushBack(*filteredImage,PreviewMode);
            /*
            QImage tImg(orgImage.width()*2, orgImage.height(), QImage::Format_A2BGR30_Premultiplied);
            tImg.fill(0);
            QPainter pT(&tImg);
            pT.drawImage(0,0,orgImage);
            pT.drawImage(orgImage.width(), 0, *filteredImage);
            pT.end();
            */
            Painter->setPen(Qt::NoPen);
            brush.setTextureImage(*filteredImage);
            brush.setTransform(MatrixBR);
            Painter->setBrush(brush);
            delete filteredImage;
            delete f;
         }
         if( bUseLightfilter /*&& BackgroundForm > 1*/ && PenSize > 0 )
            Painter->setPen(Qt::NoPen);
         if( useMask )
         {
            QPainterPath clipPath;
            // calculate mask-rect
            QRectF realMaskRect;
            bool inverted = mask.invertedMask();
            if( inverted )
               realMaskRect = WSRect(maskRect).applyTo(orgRect.width(), orgRect.height());
            else
               realMaskRect = WSRect(maskRect).applyTo(orgRect);
            // get the form-polygon
            QList<QPolygonF> masking = ComputePolygon(BackgroundForm, realMaskRect);

            // setup rotation (Z-Axis Center-Rotation)
            QTransform tr;
            bool bHasRotation = false;
            if( maskRotateZAxis != 0 || TheRotateXAxis != 0 || TheRotateYAxis != 0)
            {
               QPointF maskCenter = realMaskRect.center();
               tr.translate(maskCenter.x(), maskCenter.y() );
               tr.rotate(maskRotateZAxis);
               tr.rotate(TheRotateXAxis, Qt::XAxis);
               tr.rotate(TheRotateYAxis, Qt::YAxis);
               tr.translate(-maskCenter.x(), -maskCenter.y() );
               bHasRotation = true;
            }
            if( inverted )
            {
               for (int i = 0; i < masking.count(); i++) 
               {
                  if(bHasRotation)
                     masking[i] = tr.map(masking[i]);
               }
               QBrush brush = Painter->brush();
               brush.setTransform(QTransform());
               QImage img = brush.textureImage();
               if( img.isNull() )
               {
                  // its a real brush,
                  // create an image brush from this
                  img = QImage(orgRect.width(), orgRect.height(), QImage::Format_ARGB32_Premultiplied);
                  QPainter p(&img);
                  p.setBrush(brush);
                  p.drawRect(img.rect());
                  p.end();
               } 
               QPainter p(&img);
               p.setCompositionMode( QPainter::CompositionMode_Source);
               p.setBrush(QBrush(Qt::transparent));
               p.setPen(Qt::NoPen);
               for (int i = 0; i < masking.count(); i++) 
               {
                  p.drawPolygon(masking.at(i));
               }
               p.end();
               brush.setTextureImage(img);
               brush.setTransform(MatrixBR);
               Painter->setBrush(brush);
               QList<QPolygonF> drawRect = ComputePolygon(SHAPEFORM_RECTANGLE,orgRect);
               #ifndef NEWDRAW
               drawRect[0].translate(-CenterF.x(),-CenterF.y());
               #endif
               PolygonList = drawRect;
            }
            else
            {
               // rotate and translate the mask
               for (int i = 0; i < masking.count(); i++) 
               {
                  if(bHasRotation)
                     masking[i] = tr.map(masking[i]);
                  #ifndef NEWDRAW
                  masking[i].translate(-CenterF.x(),-CenterF.y());
                  #endif
               }
               // set the mask as the new Draw-Polygon
               PolygonList = masking;

               // use the original rect as a clipping-rect
               QList<QPolygonF> clipping = ComputePolygon(SHAPEFORM_RECTANGLE,orgRect);
               QRectF r = clipping.at(0).boundingRect();
               #ifndef NEWDRAW
               r.translate(-CenterF.x(),-CenterF.y());
               #endif
               QMarginsF margins(Painter->pen().width() / 2, Painter->pen().width() / 2, Painter->pen().width() / 2, Painter->pen().width() / 2);
               r += margins;
               clipPath.addRect(r);
               Painter->setClipPath(clipPath);
            }
         }

         //***********************************************************************************
         // Draw shape (with pen and brush and transform matrix)
         //***********************************************************************************
         if (BackgroundBrush->BrushType == BRUSHTYPE_NOBRUSH) 
            Painter->setCompositionMode(QPainter::CompositionMode_Source);
         // new drawing (faster)
         #ifdef NEWDRAW
         QBrush br = Painter->brush();
         if( !br.textureImage().isNull() )
         {
            QPainterPath pp; 
            // use polys as clipping
            for (int i = 0; i < PolygonList.count(); i++) 
               pp.addPolygon(PolygonList.at(i));
            textClippingPath = pp;
            // rotate pp!!!
            Painter->setClipPath(pp, Qt::IntersectClip);
            //qDebug() << "clipPath is " << pp;
            // draw image
            Painter->setBrush(Qt::NoBrush);
            //QRectF imageSrcRect = BackgroundBrush->GetImageRect(orgRect.size(), PreviewMode, Position, BackgroundBrush->SoundVolume != 0 ? SoundTrackMontage : NULL, ImagePctDone, PrevCompoObject ? PrevCompoObject->BackgroundBrush : NULL);
            //if( !drawSmooth(Painter,orgRect,::appliedRect(TheRect, width, height),&br.textureImage()))
               Painter->drawImage(/*orgRect.x(),orgRect.y()*/ orgRect.topLeft(),br.textureImage(),srcRect);
            //ToLog(LOGMSG_DEBUGTRACE,QString("draw Image to %1 %2 %3 %4").arg(orgRect.left()).arg(orgRect.top()).arg(orgRect.width()).arg(orgRect.height()));
            //Painter->drawImage(0,0,br.textureImage());
            // if Pensize > 0 release clipping and draw border
            if (!useMask || (useMask && mask.invertedMask()))
               Painter->setClipping(false);
            if( Painter->pen().width() > 0 )
               for (int i = 0; i < PolygonList.count(); i++) 
                  Painter->drawPolygon(PolygonList.at(i));
         }
         else
         #endif
         {
         //...new drawing
         //Painter->setBrush(Qt::NoBrush);
         for (int i = 0; i < PolygonList.count(); i++) 
            Painter->drawPolygon(PolygonList.at(i));
         if (BackgroundBrush->BrushType == BRUSHTYPE_NOBRUSH) 
            Painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
         }
      }
      //**********************************************************************************
      // Text part
      //**********************************************************************************
      if(TheTxtZoomLevel > 0  && !IsTextEmpty ) 
      {
         #ifdef SEP_TEXT_DRAW
         DrawCompositionObjectText(Object, DestPainter, ADJUST_RATIO, size, PreviewMode, Position,
            SoundTrackMontage, BlockPctDone, ImagePctDone, PrevCompoObject,
            ShotDuration, EnableAnimation,
            Transfo, NewRect,
            DisplayTextMargin, PreparedBrush);
         #else
         QPainter *tPainter = Painter;
         QImage *tmImage = 0;
         if (useTextMask)
         {
            tmImage = new QImage(size.toSize(), QImage::Format_ARGB32_Premultiplied);
            tmImage->fill(0);
            tPainter = new QPainter(tmImage);
         }
         #ifdef NEWDRAW
         Matrix.translate(CenterF.x(),CenterF.y());      
         #endif
         tPainter->setWorldTransform(Matrix, false);
         //AUTOTIMER(tt,"drawText");
         if( resolvedText.isEmpty() )
         {
            resolvedText = Variable.ResolveTextVariable(Object,tParams.Text);
         }
         QTextDocument   TextDocument;
         QTextDocument   *textDocument = &TextDocument;
         textDocument->setHtml(resolvedText);

         double FullMargin = ( (tParams.TMType == eFULLSHAPE) || (tParams.TMType == eCUSTOM) ) ? 0 : double(PenSize)*ADJUST_RATIO/double(2);
         QRectF TextMargin;
         double PointSize = (width/double(SCALINGTEXTFACTOR));

         if(tParams.TMType == eFULLSHAPE || tParams.TMType == eCUSTOM) 
            TextMargin = ::appliedRect(tParams.textMargins, TheRect.width() * width, TheRect.height() * height);
         else if (tParams.TextClipArtName != "") 
         {
            cTextFrameObject *TFO = &TextFrameList.List[TextFrameList.SearchImage(tParams.TextClipArtName)];
            TextMargin = QRectF(TFO->TMx*TheRect.width()*width+FullMargin,   TFO->TMy*TheRect.height()*height+FullMargin,
                                TFO->TMw*TheRect.width()*width-FullMargin*2, TFO->TMh*TheRect.height()*height-FullMargin*2);
         } 
         else 
         {
            TextMargin = QRectF(ShapeFormDefinition[BackgroundForm].TMx*TheRect.width()*width+FullMargin, 
                                ShapeFormDefinition[BackgroundForm].TMy*TheRect.height()*height+FullMargin,
                                ShapeFormDefinition[BackgroundForm].TMw*TheRect.width()*width-FullMargin*2, 
                                ShapeFormDefinition[BackgroundForm].TMh*TheRect.height()*height-FullMargin*2);
         }
         //qDebug() << "TextMargin: " << TextMargin;
         TextMargin.translate(-ShapeRect.width() / 2, -ShapeRect.height() / 2);
         //CenterF = getRotationOrigin(TextMargin, ZRotationInfo.center);
         //Matrix.translate(CenterF.x(), CenterF.y());
         //tPainter->setWorldTransform(Matrix, false);
         //TextMargin.translate(-CenterF.x(), -CenterF.y());

         if (DisplayTextMargin) 
         {
            QPen PP(Qt::blue);
            PP.setStyle(Qt::DotLine);
            PP.setWidth(1);
            tPainter->setPen(PP);
            tPainter->setBrush(Qt::NoBrush);
            tPainter->drawRect(TextMargin);
         }

         tPainter->setClipRect(TextMargin);

         //qDebug() << "TextMargin: " << TextMargin;
         //qDebug() << "textClippingPath " << textClippingPath;
         //textClippingPath.translate(-ShapeRect.x(), -ShapeRect.y() );
         //textClippingPath.translate(-ShapeRect.width() / 2, -ShapeRect.height() / 2);
         //qDebug() << "textClippingPath " << textClippingPath;
         //if( !textClippingPath.isEmpty())
         //   Painter->setClipPath(textClippingPath, Qt::IntersectClip);

         if (useMask)
         {
            QPainterPath clipPath;
            // calculate mask-rect
            QRectF realMaskRect;
            bool inverted = mask.invertedMask();
            realMaskRect = WSRect(maskRect).applyTo(orgRect.width(), orgRect.height());
            // get the form-polygon
            QList<QPolygonF> masking = ComputePolygon(BackgroundForm, realMaskRect);

            // setup rotation (Z-Axis Center-Rotation)
            QTransform tr;
            if (maskRotateZAxis != 0)
            {
               QPointF maskCenter = realMaskRect.center();
               tr.translate(maskCenter.x(), maskCenter.y());
               tr.rotate(maskRotateZAxis/*mask->zAngle()*/);
               tr.translate(-maskCenter.x(), -maskCenter.y());
               for (int i = 0; i < masking.count(); i++)
               {
                  if (maskRotateZAxis)
                     masking[i] = tr.map(masking[i]);
               }
            }
            //qDebug() << "masking: " << masking;
            for (int i = 0; i < masking.count(); i++)
               clipPath.addPolygon(masking.at(i));
            if (inverted)
            {
               QPainterPath rclipPath;
               //rclipPath.addRect(realMaskRect.toRect());
               rclipPath.addRect(TextMargin.translated(ShapeRect.width() / 2, ShapeRect.height() / 2).toRect());
               rclipPath -= clipPath;
               clipPath = rclipPath;
            }
            clipPath.translate(-ShapeRect.width() / 2, -ShapeRect.height() / 2);
            //qDebug() << "masking: " << clipPath;
            tPainter->setClipPath(clipPath, Qt::IntersectClip);
         }



         textDocument->setTextWidth(TextMargin.width()/PointSize);
         QRectF  FmtBdRect(0,0,
            double(textDocument->documentLayout()->documentSize().width())*(TheTxtZoomLevel/100)*PointSize,
            double(textDocument->documentLayout()->documentSize().height())*(TheTxtZoomLevel/100)*PointSize);

         int     MaxH   = TextMargin.height() > FmtBdRect.height() ? TextMargin.height() : FmtBdRect.height();
         double  DecalX = (TheTxtScrollX/100) * TextMargin.width() + TextMargin.center().x() - TextMargin.width()/2 + (TextMargin.width()-FmtBdRect.width())/2;
         double  DecalY = (-TheTxtScrollY/100) * MaxH + TextMargin.center().y() - TextMargin.height()/2;

         if (tParams.VAlign == 0)      ;                                        //Qt::AlignTop (Nothing to do)
         else if (tParams.VAlign == 1) 
            DecalY = DecalY + (TextMargin.height()-FmtBdRect.height())/2;       //Qt::AlignVCenter
         else
            DecalY = DecalY + (TextMargin.height()-FmtBdRect.height());         //Qt::AlignBottom)

         QAbstractTextDocumentLayout::PaintContext Context;

         QTextCursor Cursor(textDocument);
         QTextCharFormat TCF;

         Cursor.select(QTextCursor::Document);
         if (tParams.StyleText == 1) 
         {
            // Add outline for painting
            TCF.setTextOutline(QPen(QColor(tParams.FontShadowColor)));
            Cursor.mergeCharFormat(TCF);
         } 
         else if (tParams.StyleText != 0) 
         {
            // Paint shadow of the text
            TCF.setForeground(QBrush(QColor(tParams.FontShadowColor)));
            Cursor.mergeCharFormat(TCF);
            tPainter->save();
            switch (tParams.StyleText) 
            {
               case 2: tPainter->translate(DecalX-1,DecalY-1);   break;  //2=shadow up-left
               case 3: tPainter->translate(DecalX+1,DecalY-1);   break;  //3=shadow up-right
               case 4: tPainter->translate(DecalX-1,DecalY+1);   break;  //4=shadow bt-left
               case 5: tPainter->translate(DecalX+1,DecalY+1);   break;  //5=shadow bt-right
            }
            tPainter->scale((TheTxtZoomLevel/100)*PointSize,(TheTxtZoomLevel/100)*PointSize);
            textDocument->documentLayout()->draw(Painter,Context);
            tPainter->restore();
            textDocument->setHtml(resolvedText);     // Restore Text Document
         }
         tPainter->save();
         tPainter->translate(DecalX,DecalY);
         //qDebug() << "text decal " << DecalX << " " << DecalY;
         tPainter->scale((TheTxtZoomLevel/100)*PointSize,(TheTxtZoomLevel/100)*PointSize);
         //qDebug() << "scale painter for text to " << (TheTxtZoomLevel/100)*PointSize << " with PointSize = " << PointSize << " for width = " << width;
         textDocument->documentLayout()->draw(tPainter,Context);
         //qDebug() << "text-painter matrix is " << tPainter->worldTransform();
         tPainter->restore();

         if (useTextMask)
         {
            QPaintDevice *pDev = Painter->device();
            QImage *destImage = dynamic_cast< QImage* >(pDev);
            if (destImage)
            {
               const uchar *src = tmImage->constBits();
               uchar *dst = destImage->bits();
               int count = destImage->width() * destImage->height();
               for (int i = 0; i < count; i++)
               {
                  *dst &= (!*src);
                  dst += 4;
                  src += 4;
               }
            }
            delete tPainter;
            delete tmImage;
         }
         #endif
      }

      //**********************************************************************************
      // Block shadow part
      //**********************************************************************************
      // if shadow, draw buffered image to destination painter
      if (FormShadow) 
      {
         Painter->end();
         delete Painter;

         // 1st step : construct ImgShadow as a mask from ShadowImg
         QImage ImgShadow = ShadowImg.copy();
         QColor SColor = QColor(FormShadowColor);
         QRgb shadowColor = SColor.rgb();
         QRgb *pOrgRGB;
         for(int i = 0; i < height; i++)
         {
            pOrgRGB = (QRgb *)ImgShadow.scanLine(i);
            for(int j = 0; j < width; j++)
            {
               if( qAlpha(*pOrgRGB) )
                  *pOrgRGB = shadowColor;
               pOrgRGB++; 
            }
         }      

         // 2nd step : Draw images
         double Distance = double(FormShadowDistance) * ADJUST_RATIO;
         DestPainter->setOpacity(0.75*DestOpacity); 
         switch (FormShadow) 
         {
            case 1  : DestPainter->drawImage(-Distance, -Distance, ImgShadow); break;  // shadow up-left
            case 2  : DestPainter->drawImage( Distance, -Distance, ImgShadow); break;  // shadow up-right
            case 3  : DestPainter->drawImage(-Distance,  Distance, ImgShadow); break;  // shadow bt-left
            default : DestPainter->drawImage( Distance,  Distance, ImgShadow); break;  // shadow bt-right
         }
         DestPainter->setOpacity(DestOpacity);
         DestPainter->drawImage(0, 0, ShadowImg);
      }
      DestPainter->restore();
   }
#ifdef LOG_DRAWCOMPO
   qDebug() << "cCompositionObject::DrawCompositionObject takes " << t.elapsed() << " mSec";
#endif
}

//*********************************************************************************************************************************************

cCompositionList::cCompositionList(QObject *Parent):QObject(Parent) 
{
   setObjectName("cCompositionList");
   TypeComposition = eCOMPOSITIONTYPE_BACKGROUND;
}

//====================================================================================================================

cCompositionList::~cCompositionList() 
{
   while (compositionObjects.count() > 0) 
      delete compositionObjects.takeLast();
}

//====================================================================================================================

void cCompositionList::SaveToXML(QDomElement &domDocument,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel) 
{
   QDomDocument DomDocument;
   QString xmlFragment;
   ExXmlStreamWriter xlmStream(&xmlFragment);
   SaveToXMLex(xlmStream, ElementName, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
   DomDocument.setContent(xmlFragment);
   domDocument.appendChild(DomDocument.firstChildElement());
   return;

   /*
   QDomDocument DomDocument;
   QDomElement  Element = DomDocument.createElement(ElementName);
   // Save composition list
   Element.setAttribute("TypeComposition",TypeComposition);
   Element.setAttribute("CompositionNumber",compositionObjects.count());
   for (int i = 0; i < compositionObjects.count(); i++) 
      compositionObjects[i]->SaveToXML(Element,"Composition-"+QString("%1").arg(i),PathForRelativPath,ForceAbsolutPath,true,ReplaceList,ResKeyList,true,IsModel);
   domDocument.appendChild(Element);
   */
}

void cCompositionList::SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel)
{
   xmlStream.writeStartElement(ElementName);
   // Save composition list
   xmlStream.writeAttribute(attribute(qsl_TypeComposition, TypeComposition));
   xmlStream.writeAttribute(attribute("CompositionNumber", compositionObjects.count()));
   int i = 0;
   for (const auto compositionObject : compositionObjects)
      compositionObject->SaveToXMLex(xmlStream, "Composition-" + QString("%1").arg(i++), PathForRelativPath, ForceAbsolutPath, true, ReplaceList, ResKeyList, true, IsModel);
   xmlStream.writeEndElement();
}

//====================================================================================================================

bool cCompositionList::LoadFromXML(QDomElement domDocument,QString ElementName,QString PathForRelativPath,cCompositionList *ObjectComposition,QStringList *AliasList,cApplicationConfig *ApplicationConfig,TResKeyList *ResKeyList,bool DuplicateRes) 
{
   Q_UNUSED(ApplicationConfig)
   if ((domDocument.elementsByTagName(ElementName).length() > 0) && (domDocument.elementsByTagName(ElementName).item(0).isElement() == true)) 
   {
      QDomElement Element = domDocument.elementsByTagName(ElementName).item(0).toElement();
      bool IsOk = true;

      // Load composition list
      compositionObjects.clear();
      TypeComposition = eTypeComposition(Element.attribute(qsl_TypeComposition).toInt());
      int CompositionNumber = Element.attribute("CompositionNumber").toInt();
      for (int i = 0; i < CompositionNumber; i++) 
      {
         cCompositionObject *CompositionObject = new cCompositionObject(TypeComposition,0,pAppConfig,this);    // IndexKey will be load from XML
         if (!CompositionObject->LoadFromXML(Element,"Composition-"+QString("%1").arg(i),PathForRelativPath,ObjectComposition,AliasList,true,ResKeyList,DuplicateRes,true)) 
            //IsOk=false;
            delete CompositionObject;
         else 
            compositionObjects.append(CompositionObject);
      }
      return IsOk;
   } 
   return false;
}

cCompositionObject *cCompositionList::getCompositionObject(int IndexKey) const
{
   for(auto compositionObject : compositionObjects)
      if (compositionObject->index() == IndexKey)
         return compositionObject;
   return 0;
}

void cCompositionList::movingsOnTop(const cCompositionList* prevCompositions)
{
   CompositionObjectList  nonMovingCompositionObjects;
   CompositionObjectList  movingCompositionObjects;
   CompositionObjectList::iterator it = compositionObjects.begin();
   while( it != compositionObjects.end() )
   {
      bool isMoving = (*it)->isMoving( prevCompositions->getCompositionObject((*it)->index()) );
      cCompositionObject *prevObj = prevCompositions->getCompositionObject((*it)->index());
      qDebug() << "Object at " << (*it)->appliedRect(1920,1080) << " is " <<  (isMoving ? "" : "not") << " moving from " << prevObj->appliedRect(1920,1080);
      if( (*it)->isMoving( prevCompositions->getCompositionObject((*it)->index()) ) )
         movingCompositionObjects.append((*it));
      else
         nonMovingCompositionObjects.append((*it));
      it++;
   } 
   compositionObjects.clear();
   compositionObjects.append(nonMovingCompositionObjects);
   compositionObjects.append(movingCompositionObjects);
}

void cCompositionList::applyZOrder(const cCompositionList* prevCompositions)
{
   CompositionObjectList  newCompositionObjects;
   CompositionObjectList::const_iterator it = prevCompositions->compositionObjects.cbegin();
   while( it != prevCompositions->compositionObjects.end() )
   {
      newCompositionObjects.append( getCompositionObject((*it)->index()) );
      //if( (*it)->isMoving( prevCompositions->getCompositionObject((*it)->index()) ) )
      //   movingCompositionObjects.append((*it));
      //else
      //   nonMovingCompositionObjects.append((*it));
      it++;
   } 
   compositionObjects.clear();
   compositionObjects.append(newCompositionObjects);
}
//*********************************************************************************************************************************************
//
// Base class containing scene definition
//
//*********************************************************************************************************************************************

cDiaporamaShot::cDiaporamaShot(cDiaporamaObject *DiaporamaObject):QObject(DiaporamaObject),ShotComposition(this) 
{
   setObjectName("cDiaporamaShot");

   pDiaporamaObject                = DiaporamaObject;
   StaticDuration                  = pAppConfig->FixedDuration;    // Duration (in msec) of the static part animation
   ShotComposition.TypeComposition = eCOMPOSITIONTYPE_SHOT;
   fullScreenVideoIndex = -1;
}

//====================================================================================================================

cDiaporamaShot::~cDiaporamaShot() 
{
}

//===============================================================

void cDiaporamaShot::SaveToXML(QDomElement &domDocument,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,bool LimitedInfo,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel) 
{
   QDomDocument DomDocument;
   QString xmlFragment;
   ExXmlStreamWriter xlmStream(&xmlFragment);
   SaveToXMLex(xlmStream, ElementName, PathForRelativPath, ForceAbsolutPath, LimitedInfo, ReplaceList, ResKeyList, IsModel);
   DomDocument.setContent(xmlFragment);
   domDocument.appendChild(DomDocument.firstChildElement());
   return;
   /*
   QDomDocument DomDocument;
   QDomElement Element = DomDocument.createElement(ElementName);

   if (!LimitedInfo) 
      Element.setAttribute("StaticDuration",qlonglong(StaticDuration));                                         // Duration (in msec) of the static part animation
   ShotComposition.SaveToXML(Element,"ShotComposition",PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,IsModel);    // Composition list for this object
   domDocument.appendChild(Element);
   */
}

void cDiaporamaShot::SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, bool LimitedInfo, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel)
{
   QDomDocument DomDocument;
   xmlStream.writeStartElement(ElementName);

   if (!LimitedInfo)
      xmlStream.writeAttribute("StaticDuration", qlonglong(StaticDuration));                                         // Duration (in msec) of the static part animation
   ShotComposition.SaveToXMLex(xmlStream, "ShotComposition", PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);    // Composition list for this object
   xmlStream.writeEndElement();
}

//===============================================================

bool cDiaporamaShot::LoadFromXML(QDomElement domDocument,QString ElementName,QString PathForRelativPath,cCompositionList *ObjectComposition,QStringList *AliasList,TResKeyList *ResKeyList,bool DuplicateRes) {
    if ((domDocument.elementsByTagName(ElementName).length()>0)&&(domDocument.elementsByTagName(ElementName).item(0).isElement()==true)) {
        QDomElement Element=domDocument.elementsByTagName(ElementName).item(0).toElement();
        if (Element.hasAttribute("StaticDuration")) StaticDuration=Element.attribute("StaticDuration").toInt();           // Duration (in msec) of the static part animation
        // Composition list for this object
        ShotComposition.LoadFromXML(Element,"ShotComposition",PathForRelativPath,ObjectComposition,AliasList,pAppConfig,ResKeyList,DuplicateRes);
        return true;
    }
    return false;
}

void cDiaporamaShot::movingsOnTop(const cDiaporamaShot* prevShot)
{
   ShotComposition.movingsOnTop(&prevShot->ShotComposition);
}

//*********************************************************************************************************************************************
// handling MusicList
//*********************************************************************************************************************************************
cMusicList::~cMusicList() 
{ 
   if( !bDontDelete )
      while(MusicObjects.count()) 
         delete MusicObjects.takeLast(); 
}

bool cMusicList::findMusicObject(int64_t position, cMusicObject **pMusicObject, int64_t *startPosition, int *MusicListIndex)
{
   *pMusicObject = NULL;
   *startPosition = -1;
   *MusicListIndex = -1;
   QTime tPos = QTime(0,0).addMSecs(position);
   for(int i = 0; i < MusicObjects.count(); i++ )
   {
      cMusicObject *currentMusicObject = MusicObjects.at(i);
      if (currentMusicObject->startCode != cMusicObject::eDefault_Start)
         position -= currentMusicObject->startOffset;
      if( position >=  QTime(0,0).msecsTo(currentMusicObject->GetDuration()) )
      {
         position -= QTime(0,0).msecsTo(currentMusicObject->GetDuration());
      }
      else
      {
         *pMusicObject = currentMusicObject;
         *startPosition = position;
         *MusicListIndex = i;
         return true;
      }
   }
   return false;
}

void cMusicList::resetSoundBlocks()
{
   for(int i = 0; i < MusicObjects.count(); i++ )
      MusicObjects.at(i)->SoundTrackBloc.reset();      
}

//*********************************************************************************************************************************************
//
// Base object for all media type
//
//*********************************************************************************************************************************************

cDiaporamaObject::cDiaporamaObject(cDiaporama *Diaporama) : QObject(Diaporama), ObjectComposition(this) 
{
   setObjectName("cDiaporamaObject");

   BackgroundBrush         = new cBrushDefinition(NULL,pAppConfig);
   pDiaporama              = Diaporama;
   SlideName               = QApplication::translate("MainWindow","Title","Default slide name when no file");
   NextIndexKey            = 1;
   ThumbnailKey            = -1;
   CachedDuration          = 0;
   CachedTransitDuration   = 0;
   CachedStartPosition     = 0;
   CachedMusicIndex        = 0;
   CachedMusicListIndex    = 0;
   CachedBackgroundIndex   = 0;
   CachedHaveSound         = 0;
   CachedSoundVolume       = 0;
   CachedHaveFilter        = false;
   CachedMusicFadIN        = false;
   CachedPrevMusicFadOUT   = false;
   CachedMusicEnd          = false;
   CachedPrevMusicEnd      = false;
   CachedMusicRemaining    = 0;

   InitDefaultValues();

   // Add an empty scene
   shotList.append(new cDiaporamaShot(this));
}

//====================================================================================================================

void cDiaporamaObject::InitDefaultValues() 
{
   // Set default/initial value
   StartNewChapter                   = DEFAULT_STARTNEWCHAPTER;      // if true then start a new chapter from this slide
   ChapterName                       = QApplication::translate("cModelList","Chapter title");
   OverrideProjectEventDate          = DEFAULT_CHAPTEROVERRIDE;
   ChapterEventDate                  = pDiaporama->ProjectInfo()->EventDate();
   OverrideChapterLongDate           = DEFAULT_CHAPTEROVERRIDE;
   ChapterLocation                   = NULL;
   ChapterLongDate                   = "";
   BackgroundType                    = false;                        // Background type : false=same as precedent - true=new background definition
   BackgroundBrush->BrushType        = BRUSHTYPE_SOLID;
   BackgroundBrush->ColorD           = "#000000";                    // Background color
   MusicType                         = DEFAULT_MUSICTYPE;            // Music type : false=same as precedent - true=new playlist definition
   MusicPause                        = DEFAULT_MUSICPAUSE;           // true if music is pause during this object
   MusicReduceVolume                 = DEFAULT_MUSICREDUCEVOLUME;    // true if volume if reduce by MusicReduceFactor
   MusicReduceFactor                 = DEFAULT_MUSICREDUCEFACTOR;    // factor for volume reduction if MusicReduceVolume is true
   TransitionFamily                 = TRANSITIONFAMILY_BASE;       // Transition family
   TransitionSubType                 = 0;                            // Transition type in the family
   TransitionDuration                = DEFAULT_TRANSITIONDURATION;   // Transition duration (in msec)
   TransitionSpeedWave               = SPEEDWAVE_PROJECTDEFAULT;
   ObjectComposition.TypeComposition = eCOMPOSITIONTYPE_OBJECT;
}

//====================================================================================================================

cDiaporamaObject::~cDiaporamaObject() 
{
   if (BackgroundBrush) 
   {
      delete BackgroundBrush;
      BackgroundBrush = NULL;
   }
   if (ChapterLocation) 
   {
      delete ((cLocation *)ChapterLocation);
      ChapterLocation = NULL;
   }
   while (shotList.count() > 0)
      delete shotList.takeLast();
}

//====================================================================================================================

QString cDiaporamaObject::GetDisplayName() 
{
   return SlideName;
}

//===============================================================
// Draw Thumb
void cDiaporamaObject::DrawThumbnail(int ThumbWidth,int ThumbHeight, QPainter *Painter, int AddX, int AddY, int ShotNumber) 
{
   //ToLog(LOGMSG_CRITICAL,QString("cDiaporamaObject::DrawThumbnail for shot %1").arg(ShotNumber));
   QImage Thumb;
   if (ShotNumber == 0) 
      pAppConfig->SlideThumbsTable->GetThumb(&ThumbnailKey,&Thumb);

   int gThumbWidth, gThumbHeight;
   switch( pDiaporama->ImageGeometry )
   {
      case GEOMETRY_4_3:
         gThumbWidth = 144;
         gThumbHeight = 108;
      break;

      case GEOMETRY_16_9:
         gThumbWidth = 192;
         gThumbHeight = 108;
      break;

      case GEOMETRY_40_17:
         gThumbWidth = 254;
         gThumbHeight = 108;
      break;

   }
   if (Thumb.isNull() || Thumb.width() != gThumbWidth || Thumb.height() != gThumbHeight) 
   {
      //qDebug() << "Thumb.isNull() " << Thumb.isNull() << " Thumb.width() " << Thumb.width() << " Thumb.height() " << Thumb.height() 
      //   << " req width " << ThumbWidth << " req height " << ThumbHeight;
      //ToLog(LOGMSG_CRITICAL,QString("cDiaporamaObject::DrawThumbnail no thumb, draw one"));
      Thumb = QImage(gThumbWidth, gThumbHeight, QImage::Format_ARGB32_Premultiplied);
      Thumb.fill(0);
      QPainter  P;
      P.begin(&Thumb);
      if (shotList.count() > 0)
      {
         cDiaporamaShot *shot = shotList[ShotNumber];
         for (int j = 0; j < shot->ShotComposition.compositionObjects.count(); j++)
         {
            cVideoFile *Video = shot->ShotComposition.compositionObjects[j]->BackgroundBrush->isVideo()  ?
               (cVideoFile *)shot->ShotComposition.compositionObjects[j]->BackgroundBrush->MediaObject : NULL;
            int StartPosToAdd = (Video) ? QTime(0,0,0,0).msecsTo(Video->StartTime) : 0;
            if (ShotNumber != 0) 
            {
               // Calc Start position of the video (depending on visible state)
               int IndexKeyToFind = shot->ShotComposition.compositionObjects[j]->index();
               for (int k = 0; k < ShotNumber; k++)
               {
                  cCompositionObject* p = shotList[k]->ShotComposition.getCompositionObject(IndexKeyToFind);
                  if( p && p->isVisible() )
                     StartPosToAdd += shotList[k]->StaticDuration;
               }
            }
            shot->ShotComposition.compositionObjects[j]->DrawCompositionObject(this, &P, double(gThumbHeight) / 1080, QSizeF(gThumbWidth, gThumbHeight), true, StartPosToAdd, NULL, 0, 0, NULL, false, shot->StaticDuration, false);
         }
      }
      P.end();
      if (ShotNumber == 0) 
         pAppConfig->SlideThumbsTable->SetThumb(&ThumbnailKey, Thumb);
      
   }
   if (Painter) 
      Painter->drawImage(QRectF(AddX,AddY,ThumbWidth,ThumbHeight),Thumb);
      //Painter->drawImage(AddX,AddY,Thumb);
}

//===============================================================

int cDiaporamaObject::GetSpeedWave() 
{
   if (TransitionSpeedWave == SPEEDWAVE_PROJECTDEFAULT) 
      return pDiaporama->TransitionSpeedWave; 
   return TransitionSpeedWave;
}

//===============================================================

int64_t cDiaporamaObject::GetTransitDuration() 
{
   if (TransitionFamily == 0 && TransitionSubType == 0) 
      return 0; 
   return TransitionDuration;
}

int64_t cDiaporamaObject::GetCumulTransitDuration() 
{
   // Adjust duration to ensure transition will be full !
   int ObjectIndex = pDiaporama->GetObjectIndex(this);
   int64_t TransitDuration = GetTransitDuration();
   if (ObjectIndex < (pDiaporama->slides.count()-1)) 
      TransitDuration += pDiaporama->slides[ObjectIndex+1]->GetTransitDuration();
   return TransitDuration;
}

//===============================================================

int64_t cDiaporamaObject::GetDuration() 
{
   int64_t Duration = 0;
   for(const auto shot : shotList)
      Duration += shot->StaticDuration;

   // Adjust duration to ensure transition will be full !
   int64_t TransitDuration = GetCumulTransitDuration();
   if (Duration < TransitDuration) 
      Duration = TransitDuration;

   // Calc minimum duration to ensure all video will be full !
   int MaxMovieDuration = 0;
   for(const auto obj : ObjectComposition.compositionObjects)
   {
   //for (int Block = 0; Block < ObjectComposition.List.count(); Block++)
      //if ((obj->BackgroundBrush->BrushType == BRUSHTYPE_IMAGEDISK) && (obj->BackgroundBrush->MediaObject) && (obj->BackgroundBrush->MediaObject->ObjectType == OBJECTTYPE_VIDEOFILE)) 
      if (obj->isVideo()) 
      {
         int IndexKey            = obj->index();
         int64_t WantedDuration = ((cVideoFile*)obj)->WantedDuration();
         int64_t CurrentDuration = 0;
         for (const auto shot : shotList)
         {
            cCompositionObject* object = shot->ShotComposition.getCompositionObject(IndexKey);
            if(object->isVisible())
            {
               WantedDuration -= shot->StaticDuration;
               if (WantedDuration < 0)
                  WantedDuration = 0;
            }
            CurrentDuration += shot->StaticDuration;
         }
         CurrentDuration += WantedDuration;
         if (MaxMovieDuration < CurrentDuration) 
            MaxMovieDuration = CurrentDuration;
      }
   }
   if (Duration < MaxMovieDuration)  
      Duration = MaxMovieDuration;
   if (Duration < 33)
      Duration = 33;    // In all case minumum duration set to 1/30 sec
    return Duration;
}

//===============================================================
int cDiaporamaObject::ComputeChapterNumber(cDiaporamaObject **Object) 
{
   int i,Number = 0;
   if (Object) 
      *Object = NULL;
   for (i = 0; i < pDiaporama->slides.count() && pDiaporama->slides[i] != this; i++) 
   {
      if (i == 0 || pDiaporama->slides[i]->StartNewChapter) 
      {
         if (pDiaporama->slides[i]->StartNewChapter) 
            Number++;
         if ((Object) && (i < pDiaporama->slides.count())) 
            *Object = pDiaporama->slides[i];
      }
   }
   if (i == 0 || ( i < pDiaporama->slides.count() && pDiaporama->slides[i]->StartNewChapter)) 
   {
      if ( i < pDiaporama->slides.count() && pDiaporama->slides[i]->StartNewChapter) 
         Number++;
      if (Object && i < pDiaporama->slides.count()) 
         *Object = pDiaporama->slides[i];
   }
   return Number;
}

//===============================================================

int cDiaporamaObject::GetSlideNumber() 
{
   int Number = 0;
   for (int i = 0; (i < pDiaporama->slides.count()) && (pDiaporama->slides[i] != this); i++) 
      Number++;
   return Number;
}

//===============================================================

int cDiaporamaObject::GetAutoTSNumber() 
{
   if ((SlideName.length() == QString("<%AUTOTS_000000%>").length()) && (SlideName.startsWith("<%AUTOTS_")) && (SlideName.endsWith("%>")))
      return SlideName.mid(QString("<%AUTOTS_").length(),QString("000000").length()).toInt();
   return -1;
}

void cDiaporamaObject::setDefaultTransition()
{
   if (pAppConfig->RandomTransition)
   {
      quint32 Random = QRandomGenerator::global()->generate();
      Random = int(double(IconList.List.count()) * (double(Random) / double(RAND_MAX)));
      if (Random < IconList.List.count())
      {
         TransitionFamily = IconList.List[Random].TransitionFamily;
         TransitionSubType = IconList.List[Random].TransitionSubType;
      }
   }
   else
   {
      TransitionFamily = pAppConfig->DefaultTransitionFamily;
      TransitionSubType = pAppConfig->DefaultTransitionSubType;
   }
   TransitionDuration = pAppConfig->DefaultTransitionDuration;
}


void cDiaporamaObject::ComputeOptimisationFlags()
{
   cDiaporamaShot* prev = NULL;
   for(auto shot :shotList)
   {
      int index = 0;
      int fullScreenVideoIndex = -1;
      int lastVideoIndex = -1;
      shot->fullScreenVideoIndex = -1;
      for(const auto compositionObject : shot->ShotComposition.compositionObjects)
      {
         cCompositionObject * const prevCompositionObject = prev ? prev->ShotComposition.getCompositionObject(compositionObject->index()) : NULL;
         compositionObject->ComputeOptimisationFlags(prev ? prevCompositionObject : NULL);
         if(compositionObject->isVideo() && compositionObject->isVisible() )
         {
            if(compositionObject->IsFullScreen && 
               (!compositionObject->BackgroundBrush->hasFilter() && (!prev || !prevCompositionObject->hasFilter()) ) )
               fullScreenVideoIndex = index;//compositionObject->index();
            if(compositionObject->BackgroundBrush->SoundVolume != 0 && lastVideoIndex == -1)
               lastVideoIndex = index;//compositionObject->index();
         }
         if (!compositionObject->isVideo() && compositionObject->isVisible() && compositionObject->isOnScreen(prevCompositionObject))
            fullScreenVideoIndex = -1;
         index++;
      }
      if(fullScreenVideoIndex >= 0 && (lastVideoIndex == fullScreenVideoIndex || lastVideoIndex == -1) )
         shot->fullScreenVideoIndex = fullScreenVideoIndex;
      prev = shot;
      //qDebug() << "shot " << shotno << " fullScreenVideoIndex " << fullScreenVideoIndex;
      //shotno++;
   }
}

//===============================================================

void cDiaporamaObject::LoadModelFromXMLData(ffd_MODELTYPE TypeModel,QDomDocument domDocument,TResKeyList *ResKeyList,bool DuplicateRes) 
{
   QString     ErrorMsg;
   QDomElement ProjectDocument = domDocument.documentElement();
   bool        IsOk = false;
   switch (TypeModel)
   {
      case ffd_MODELTYPE_THUMBNAIL:
         IsOk = (ProjectDocument.tagName() == THUMBMODEL_ROOTNAME) && (LoadFromXML(ProjectDocument, THUMBMODEL_ELEMENTNAME, "", NULL, ResKeyList, DuplicateRes));
         ErrorMsg = QApplication::translate("MainWindow", "The file is not a valid thumbnail file", "Error message");
      break;
      case ffd_MODELTYPE_PROJECTTITLE:
      case ffd_MODELTYPE_CHAPTERTITLE:
      case ffd_MODELTYPE_CREDITTITLE:
         IsOk = (ProjectDocument.tagName() == TITLEMODEL_ROOTNAME) && (LoadFromXML(ProjectDocument, TITLEMODEL_ELEMENTNAME, "", NULL, ResKeyList, DuplicateRes));
         ErrorMsg = QApplication::translate("MainWindow", "The file is not a valid title model file", "Error message");
      break;
   }
   if (!IsOk) 
      CustomMessageBox(NULL, QMessageBox::Critical, QApplication::translate("MainWindow", "Error", "Error message"), ErrorMsg, QMessageBox::Close);
}

//===============================================================

bool cDiaporamaObject::SaveModelFile(ffd_MODELTYPE TypeModel,QString ModelFileName) {
    QList<qlonglong>    ResKeyList;
    QFile               file(ModelFileName);
    QDomDocument        domDocument(APPLICATION_NAME);
    QDomElement         root;
    QString             RootName,ElementName;

    // Create xml document and root
    switch (TypeModel) {
        case ffd_MODELTYPE_THUMBNAIL:
            RootName=THUMBMODEL_ROOTNAME;
            ElementName=THUMBMODEL_ELEMENTNAME;
            break;
        case ffd_MODELTYPE_PROJECTTITLE:
        case ffd_MODELTYPE_CHAPTERTITLE:
        case ffd_MODELTYPE_CREDITTITLE:
            RootName=TITLEMODEL_ROOTNAME;
            ElementName=TITLEMODEL_ELEMENTNAME;
            break;
    }
    root=domDocument.createElement(RootName);
    domDocument.appendChild(root);
    SaveToXML(root,ElementName,ModelFileName,true,NULL,&ResKeyList,false);
    // Write file to disk
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        CustomMessageBox(NULL,QMessageBox::Critical,QApplication::translate("MainWindow","Error","Error message"),QApplication::translate("MainWindow","Error creating model file","Error message"),QMessageBox::Close);
        return false;
    } else {
        // Save file now
        QTextStream out(&file);
#if QT_VERSION < 0x060000
        out.setCodec("UTF-8");
#endif
        domDocument.save(out,4);
        // Iterate for ressources
        for (int i=0;i<ResKeyList.count();i++) {
            QImage      Thumbnail;
            qlonglong   Key=ResKeyList[i];
            pAppConfig->SlideThumbsTable->GetThumb(&Key,&Thumbnail);

            QDomElement     Ressource=domDocument.createElement("Ressource");
            QByteArray      ba;
            QBuffer         buf(&ba);

            Thumbnail.save(&buf,"PNG");
            QByteArray Compressed=qCompress(ba,1);
            QByteArray Hexed     =Compressed.toHex();
            Ressource.setAttribute("Key",Key);
            Ressource.setAttribute("Width",Thumbnail.width());
            Ressource.setAttribute("Height",Thumbnail.height());
            Ressource.setAttribute("Image",QString(Hexed));
            Ressource.save(out,0);
        }
        file.close();
        return true;
    }
}

//===============================================================

QString cDiaporamaObject::SaveAsNewCustomModelFile(ffd_MODELTYPE TypeModel) 
{
   QString     NewName,Text;
   cModelList  *ModelList;

   switch (TypeModel) {
   case ffd_MODELTYPE_PROJECTTITLE: ModelList = pAppConfig->PrjTitleModels[pDiaporama->ImageGeometry][MODELTYPE_PROJECTTITLE_CATNUMBER-1];     break;
   case ffd_MODELTYPE_CHAPTERTITLE: ModelList = pAppConfig->CptTitleModels[pDiaporama->ImageGeometry][MODELTYPE_CHAPTERTITLE_CATNUMBER-1];     break;
   case ffd_MODELTYPE_CREDITTITLE:  ModelList = pAppConfig->CreditTitleModels[pDiaporama->ImageGeometry][MODELTYPE_CREDITTITLE_CATNUMBER-1];   break;
   case ffd_MODELTYPE_THUMBNAIL:                
   default:                         ModelList = pAppConfig->ThumbnailModels;                                                               break;
   }

   NewName = ModelList->CustomModelPath; 
   if (!NewName.endsWith(QDir::separator())) 
      NewName.append( QDir::separator() );
   Text=QString("%1").arg(*ModelList->NextNumber);
   (*ModelList->NextNumber)++;
   NewName = NewName + Text + "." + ModelList->ModelSuffix;
   SaveModelFile(TypeModel,NewName);
   ModelList->FillModelType(TypeModel);
   if (TypeModel == ffd_MODELTYPE_THUMBNAIL)
      pDiaporama->ThumbnailName = Text;
   return NewName;
}

void cDiaporamaObject::releaseCachObjects()
{
   foreach(cDiaporamaShot *shot, shotList)
      pAppConfig->ImagesCache.RemoveImageObject(shot);
}

void cDiaporamaObject::preLoadItems(bool bPreview)
{
   foreach(cCompositionObject *compositionObject, ObjectComposition.compositionObjects)
   {
      compositionObject->preLoad(bPreview);
   }
}

void cDiaporamaObject::movingsOnTop(int shotNumber)
{
   if( shotNumber < 1 || shotNumber > shotList.count() )
      return;
   shotList[shotNumber]->movingsOnTop(shotList.at(shotNumber-1));
   for(int i = shotNumber+1; i < shotList.count(); i++ )
   {
      shotList[i]->ShotComposition.applyZOrder(&shotList.at(i-1)->ShotComposition);
   }
}

//===============================================================

void cDiaporamaObject::SaveToXML(QDomElement &domDocument, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool SaveThumbAllowed) 
{
   QDomDocument DomDocument;
   QString xmlFragment;
   ExXmlStreamWriter xlmStream(&xmlFragment);
   SaveToXMLex(xlmStream, ElementName, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, SaveThumbAllowed);
   DomDocument.setContent(xmlFragment);
   domDocument.appendChild(DomDocument.firstChildElement());
   return; 
    /*
    QDomElement     Element=DomDocument.createElement(ElementName);
    QDomElement     SubElement;

    Element.setAttribute("NextIndexKey",    NextIndexKey);

    if (ElementName==THUMBMODEL_ELEMENTNAME) 
    {
        Element.setAttribute("ThumbnailName", pDiaporama->ThumbnailName);
    } 
    else if (ElementName==TITLEMODEL_ELEMENTNAME) 
    {
      // void
    } 
    else 
    {

        // Slide properties
        Element.setAttribute("SlideName", SlideName);

        // Chapter properties
        if (StartNewChapter!=DEFAULT_STARTNEWCHAPTER)                                       Element.setAttribute("StartNewChapter",         StartNewChapter?"1":"0");
        if (OverrideProjectEventDate!=DEFAULT_CHAPTEROVERRIDE)                              Element.setAttribute("OverrideProjectEventDate",OverrideProjectEventDate?"1":"0");
        if (OverrideChapterLongDate!=DEFAULT_CHAPTEROVERRIDE)                               Element.setAttribute("OverrideChapterLongDate", OverrideChapterLongDate?"1":"0");
        if (StartNewChapter && !ChapterName.isEmpty())                                      Element.setAttribute("ChapterName",             ChapterName);
        if (OverrideChapterLongDate && !ChapterLongDate.isEmpty())                          Element.setAttribute("ChapterLongDate",         ChapterLongDate);
        if (OverrideProjectEventDate && (ChapterEventDate!=pDiaporama->ProjectInfo()->EventDate())) Element.setAttribute("ChapterEventDate",        ChapterEventDate.toString(Qt::ISODate));
        if (ChapterLocation) {
            QDomElement SubElement=DomDocument.createElement("ChapterLocation");
            ((cLocation *)ChapterLocation)->SaveToXML(&SubElement,"",PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,false);
            Element.appendChild(SubElement);
        }

        // Transition properties
        SubElement=DomDocument.createElement("Transition");
        SubElement.setAttribute("TransitionFamily",TransitionFamily);                         // Transition family
        SubElement.setAttribute("TransitionSubType",TransitionSubType);                         // Transition type in the family
        if (TransitionDuration!=DEFAULT_TRANSITIONDURATION) SubElement.setAttribute("TransitionDuration",qlonglong(TransitionDuration));            // Transition duration (in msec)
        if (TransitionSpeedWave!=SPEEDWAVE_PROJECTDEFAULT)  SubElement.setAttribute("TransitionSpeedWave",TransitionSpeedWave);                     // Transition speed wave
        Element.appendChild(SubElement);

        // Music properties
        if (MusicType!=DEFAULT_MUSICTYPE)                   Element.setAttribute("MusicType",         MusicType?"1":"0");                           // Music type : false=same as precedent - true=new playlist definition
        if (MusicPause!=DEFAULT_MUSICPAUSE)                 Element.setAttribute("MusicPause",        MusicPause?"1":"0");                          // true if music is pause during this object
        if (MusicReduceVolume!=DEFAULT_MUSICREDUCEVOLUME)   Element.setAttribute("MusicReduceVolume", MusicReduceVolume?"1":"0");                   // true if volume if reduce by MusicReduceFactor
        if (MusicReduceFactor!=DEFAULT_MUSICREDUCEFACTOR)   Element.setAttribute("MusicReduceFactor",QString("%1").arg(MusicReduceFactor,0,'f'));   // factor for volume reduction if MusicReduceVolume is true
        if (MusicList.count()>0) {
            Element.setAttribute("MusicNumber",MusicList.count());                           // Number of file in the playlist
            for (int i=0;i<MusicList.count();i++) MusicList.MusicObjects[i]->SaveToXML(&Element,"Music-"+QString("%1").arg(i),PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,false);
        }

        if ((ThumbnailKey!=-1)&&(SaveThumbAllowed)) {
            Element.setAttribute("ThumbRessource",ThumbnailKey);
            if (ResKeyList) ResKeyList->append(ThumbnailKey);
        }

        // Background properties
        SubElement=DomDocument.createElement("Background");
        SubElement.setAttribute("BackgroundType",BackgroundType?"1":"0");                                        // Background type : false=same as precedent - true=new background definition
        BackgroundBrush->SaveToXML(&SubElement,"BackgroundBrush",PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,false);  // Background brush
        Element.appendChild(SubElement);

    }

    // Global blocks composition table
    ObjectComposition.SaveToXML(Element,"ObjectComposition",PathForRelativPath,ForceAbsolutPath,ReplaceList,ResKeyList,ElementName==TITLEMODEL_ELEMENTNAME);         // ObjectComposition

    // Shots definitions
    Element.setAttribute("ShotNumber", shotList.count());
    for (int i = 0; i<shotList.count(); i++) 
      shotList[i]->SaveToXML(Element, "Shot-" + QString("%1").arg(i), PathForRelativPath, ForceAbsolutPath, (ElementName == THUMBMODEL_ELEMENTNAME), ReplaceList, ResKeyList, ElementName == TITLEMODEL_ELEMENTNAME);

    domDocument.appendChild(Element);
    */
}
void cDiaporamaObject::SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool SaveThumbAllowed)
{
   xmlStream.writeStartElement(ElementName);

   xmlStream.writeAttribute(attribute("NextIndexKey", NextIndexKey));
   xmlStream.writeAttribute(attribute("ShotNumber", shotList.count()));

   if (ElementName == THUMBMODEL_ELEMENTNAME)
   {
      xmlStream.writeAttribute("ThumbnailName", pDiaporama->ThumbnailName);
   }
   else if (ElementName == TITLEMODEL_ELEMENTNAME)
   {
      // void
   }
   else
   {
      // Slide properties
      xmlStream.writeAttribute("SlideName", SlideName);

      // Chapter properties
      if (StartNewChapter != DEFAULT_STARTNEWCHAPTER)             xmlStream.writeAttribute("StartNewChapter", StartNewChapter);
      if (OverrideProjectEventDate != DEFAULT_CHAPTEROVERRIDE)    xmlStream.writeAttribute("OverrideProjectEventDate", OverrideProjectEventDate );
      if (OverrideChapterLongDate != DEFAULT_CHAPTEROVERRIDE)     xmlStream.writeAttribute("OverrideChapterLongDate", OverrideChapterLongDate );
      if (StartNewChapter && !ChapterName.isEmpty())              xmlStream.writeAttribute("ChapterName", ChapterName);
      if (OverrideChapterLongDate && !ChapterLongDate.isEmpty())  xmlStream.writeAttribute("ChapterLongDate", ChapterLongDate);
      if (OverrideProjectEventDate && (ChapterEventDate != pDiaporama->ProjectInfo()->EventDate())) xmlStream.writeAttribute("ChapterEventDate", ChapterEventDate.toString(Qt::ISODate));

      // Music properties
      if (MusicType != DEFAULT_MUSICTYPE)                   xmlStream.writeAttribute("MusicType", MusicType);                           // Music type : false=same as precedent - true=new playlist definition
      if (MusicPause != DEFAULT_MUSICPAUSE)                 xmlStream.writeAttribute("MusicPause", MusicPause);                          // true if music is pause during this object
      if (MusicReduceVolume != DEFAULT_MUSICREDUCEVOLUME)   xmlStream.writeAttribute("MusicReduceVolume", MusicReduceVolume );                   // true if volume if reduce by MusicReduceFactor
      if (MusicReduceFactor != DEFAULT_MUSICREDUCEFACTOR)   xmlStream.writeAttribute("MusicReduceFactor", QString("%1").arg(MusicReduceFactor, 0, 'f'));   // factor for volume reduction if MusicReduceVolume is true
      if (MusicList.count()>0)
         xmlStream.writeAttribute(attribute("MusicNumber", MusicList.count()));                           // Number of file in the playlist
      if ((ThumbnailKey != -1) && (SaveThumbAllowed))
      {
         xmlStream.writeAttribute(attribute("ThumbRessource", ThumbnailKey));
         if (ResKeyList) ResKeyList->append(ThumbnailKey);
      }

      if (ChapterLocation)
      {
         xmlStream.writeStartElement("ChapterLocation");
         ((cLocation *)ChapterLocation)->SaveToXMLex(xmlStream, "", PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, false);
         xmlStream.writeEndElement();
      }

      // Transition properties
      xmlStream.writeStartElement("Transition");
      xmlStream.writeAttribute(attribute("TransitionFamilly", TransitionFamily)); // Transition family
      xmlStream.writeAttribute(attribute("TransitionSubType", TransitionSubType)); // Transition type in the family
      if (TransitionDuration != DEFAULT_TRANSITIONDURATION) xmlStream.writeAttribute(attribute("TransitionDuration", qlonglong(TransitionDuration)));            // Transition duration (in msec)
      if (TransitionSpeedWave != SPEEDWAVE_PROJECTDEFAULT)  xmlStream.writeAttribute(attribute("TransitionSpeedWave", TransitionSpeedWave));                     // Transition speed wave
      xmlStream.writeEndElement();

      if (MusicList.count()>0)
      {
         //xmlStream.writeAttribute(attribute("MusicNumber", MusicList.count()));                           // Number of file in the playlist
         for (int i = 0; i<MusicList.count(); i++) 
            MusicList.MusicObjects[i]->SaveToXMLex(xmlStream, "Music-" + QString("%1").arg(i), PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, false);
      }


      // Background properties
      xmlStream.writeStartElement("Background");
      xmlStream.writeAttribute("BackgroundType", BackgroundType );                                        // Background type : false=same as precedent - true=new background definition
      BackgroundBrush->SaveToXMLex(xmlStream, "BackgroundBrush", PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, false);  // Background brush
      xmlStream.writeEndElement();

   }

   // Global blocks composition table
   ObjectComposition.SaveToXMLex(xmlStream, "ObjectComposition", PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, ElementName == TITLEMODEL_ELEMENTNAME);         // ObjectComposition

                                                                                                                                                                            // Shots definitions
   //xmlStream.writeAttribute(attribute("ShotNumber", shotList.count()));
   for (int i = 0; i<shotList.count(); i++)
      shotList[i]->SaveToXMLex(xmlStream, "Shot-" + QString("%1").arg(i), PathForRelativPath, ForceAbsolutPath, (ElementName == THUMBMODEL_ELEMENTNAME), ReplaceList, ResKeyList, ElementName == TITLEMODEL_ELEMENTNAME);

   xmlStream.writeEndElement();
}

//===============================================================
QList<int> *thumbkeylist = 0;
bool cDiaporamaObject::LoadFromXML(QDomElement domDocument,QString ElementName,QString PathForRelativPath,QStringList *AliasList,
                                   TResKeyList *ResKeyList,bool DuplicateRes,DlgWorkingTask *DlgWorkingTaskDialog) 
{
   if ((ElementName != THUMBMODEL_ELEMENTNAME) && (ElementName != TITLEMODEL_ELEMENTNAME)) 
      InitDefaultValues();

   if ((domDocument.elementsByTagName(ElementName).length() > 0) && (domDocument.elementsByTagName(ElementName).item(0).isElement() == true)) 
   {
      QDomElement Element = domDocument.elementsByTagName(ElementName).item(0).toElement();

      bool IsOk = true;
      bool ModifyFlag = false;

      // Load shot list
      shotList.clear();

      NextIndexKey = Element.attribute("NextIndexKey").toInt();
      if (ElementName == THUMBMODEL_ELEMENTNAME) 
      {
         if (Element.hasAttribute("ThumbnailName")) 
            pDiaporama->ThumbnailName = Element.attribute("ThumbnailName");

      } 
      else if (ElementName==TITLEMODEL_ELEMENTNAME) 
      {
         // void
      } 
      else 
      {
         // Slide properties
         SlideName = Element.attribute("SlideName");
         readBool(&StartNewChapter,Element,"StartNewChapter");
         if (Element.hasAttribute("StartNewChapter"))            
            StartNewChapter = Element.attribute("StartNewChapter") == "1";
         if (Element.hasAttribute("OverrideProjectEventDate"))   OverrideProjectEventDate=Element.attribute("OverrideProjectEventDate")=="1";
         if (Element.hasAttribute("OverrideChapterLongDate"))    OverrideChapterLongDate =Element.attribute("OverrideChapterLongDate")=="1";
         if (Element.hasAttribute("ChapterName"))                ChapterName             =Element.attribute("ChapterName");
         if (Element.hasAttribute("ChapterLongDate"))            ChapterLongDate         =Element.attribute("ChapterLongDate");
         if (Element.hasAttribute("ChapterEventDate"))           ChapterEventDate        =ChapterEventDate.fromString(Element.attribute("ChapterEventDate"),Qt::ISODate);
         ChapterEventDate = OverrideProjectEventDate ? ChapterEventDate : pDiaporama->ProjectInfo()->EventDate();
         ChapterLongDate = OverrideProjectEventDate ? OverrideChapterLongDate ? ChapterLongDate : FormatLongDate(ChapterEventDate) : pDiaporama->ProjectInfo()->LongDate();
         if ((Element.elementsByTagName("ChapterLocation").length() > 0) && (Element.elementsByTagName("ChapterLocation").item(0).isElement() == true)) 
         {
            QDomElement SubElement=Element.elementsByTagName("ChapterLocation").item(0).toElement();
            if (ChapterLocation) 
               delete (cLocation *)ChapterLocation;
            ChapterLocation = new cLocation(pAppConfig);
            ((cLocation *)ChapterLocation)->LoadFromXML(&SubElement,"",PathForRelativPath,AliasList,&ModifyFlag,ResKeyList,DuplicateRes);
         }

         // Compatibility with version prior to 1.7
         if ((pDiaporama->ProjectInfo()->Revision() < "20130725") && ((StartNewChapter) || (GetSlideNumber() == 0))) 
            ChapterName = SlideName;

         // Transition properties
         if ((Element.elementsByTagName("Transition").length() > 0) && (Element.elementsByTagName("Transition").item(0).isElement() == true)) 
         {
            QDomElement SubElement = Element.elementsByTagName("Transition").item(0).toElement();
            TransitionFamily = (TRFAMILY)SubElement.attribute("TransitionFamilly").toInt();                                                // Transition family
            TransitionSubType = SubElement.attribute("TransitionSubType").toInt();                                                           // Transition type in the family
            if (SubElement.hasAttribute("TransitionDuration"))  
               TransitionDuration = SubElement.attribute("TransitionDuration").toInt();                                                      // Transition duration (in msec)
            if (SubElement.hasAttribute("TransitionSpeedWave")) 
               TransitionSpeedWave = SubElement.attribute("TransitionSpeedWave").toInt();    // Transition speed wave
            // Element.appendChild(SubElement); //??? look like a copy-paste error
         }

         // Music properties
         MusicList.clear();
         if (Element.hasAttribute("MusicType"))          MusicType         = Element.attribute("MusicType")=="1";                     // Music type : false=same as precedent - true=new playlist definition
         if (Element.hasAttribute("MusicPause"))         MusicPause        = Element.attribute("MusicPause")=="1";                    // true if music is pause during this object
         if (Element.hasAttribute("MusicReduceVolume"))  MusicReduceVolume = Element.attribute("MusicReduceVolume")=="1";             // true if volume if reduce by MusicReduceFactor
         if (Element.hasAttribute("MusicReduceFactor"))  MusicReduceFactor = GetDoubleValue(Element,"MusicReduceFactor");             // factor for volume reduction if MusicReduceVolume is true
         if (Element.hasAttribute("MusicNumber")) 
         {
            int MusicNumber = Element.attribute("MusicNumber").toInt();                // Number of file in the playlist
            for (int i = 0; i < MusicNumber; i++) 
            {
               cMusicObject *MusicObject = new cMusicObject(pAppConfig);
               if (!MusicObject->LoadFromXML(&Element,"Music-"+QString("%1").arg(i),PathForRelativPath,AliasList,&ModifyFlag)) 
                  IsOk = false;
               if ((DlgWorkingTaskDialog) && (!MusicObject->IsComputedAudioDuration)) 
               {
                  QList<qreal> Peak, Moyenne;
                  DlgWorkingTaskDialog->DisplayText2(QApplication::translate("MainWindow","Analyse file %1").arg(MusicObject->CachedFileName()));
                  MusicObject->DoAnalyseSound(&Peak,&Moyenne,DlgWorkingTaskDialog->CancelActionFlag,&DlgWorkingTaskDialog->TimerProgress);
                  DlgWorkingTaskDialog->StopText2();
                  if (ModifyFlag) 
                     ((MainWindow *)pAppConfig->TopLevelWindow)->SetModifyFlag(true);
               }
               MusicList.MusicObjects.append(MusicObject);
               if (pAppConfig->seamlessMusic)
               {
                  if (i == 0)
                     MusicObject->startCode = cMusicObject::eFrameRelated_Start;
                  pDiaporama->masterSound.MusicObjects.append(MusicObject);
               }
               if (ModifyFlag) 
                  ((MainWindow *)pAppConfig->TopLevelWindow)->SetModifyFlag(true);
            }
         }

         // Compatibility with version  prior to 2.1
         if (Element.hasAttribute("Thumbnail")) 
         {
            int         ThumbWidth   = Element.attribute("ThumbWidth").toInt();
            int         ThumbHeight  = Element.attribute("ThumbHeight").toInt();
            QImage      Thumbnail(ThumbWidth,ThumbHeight,QImage::Format_ARGB32_Premultiplied);
            QByteArray  Compressed   = QByteArray::fromHex(Element.attribute("Thumbnail").toUtf8());
            QByteArray  Decompressed = qUncompress(Compressed);
            Thumbnail.loadFromData(Decompressed);
            pAppConfig->SlideThumbsTable->SetThumb(&ThumbnailKey,Thumbnail);
         }

         bool isDuplicateKey = false;
         if (Element.hasAttribute("ThumbRessource"))
         {
            //if( thumbkeylist == 0)
            //   thumbkeylist = new QList<int>;
            if (ResKeyList) 
            {
               int ResKey=Element.attribute("ThumbRessource").toLongLong();
               //if( thumbkeylist->contains(ResKey))
               //{
               //   isDuplicateKey = true;
               //   //qDebug() << "Warning: duplicate Reskey " << ResKey;
               //}
               //else
               //   thumbkeylist->append(ResKey);
               for (int ResNum = 0; ResNum < ResKeyList->count(); ResNum++) 
                  if (ResKey == ResKeyList->at(ResNum).OrigKey) 
                  {
                     ResKey = ResKeyList->at(ResNum).NewKey;
                     break;
                  }
                  ThumbnailKey = ResKey;
            } 
            else 
            {
               ThumbnailKey = Element.attribute("ThumbRessource").toLongLong();
            }
         }

         // if DuplicateRes (for example during a paste operation)
         if ((DuplicateRes || isDuplicateKey)&& ThumbnailKey != -1)
         {
            QImage Thumbnail;
            pAppConfig->SlideThumbsTable->GetThumb(&ThumbnailKey,&Thumbnail);
            ThumbnailKey = -1;
            pAppConfig->SlideThumbsTable->GetThumb(&ThumbnailKey,&Thumbnail);
         }

         // Background properties
         if ((Element.elementsByTagName("Background").length() > 0) && (Element.elementsByTagName("Background").item(0).isElement() == true)) 
         {
            if (BackgroundBrush->MediaObject) 
            {
               if (BackgroundBrush->DeleteMediaObject) 
                  delete BackgroundBrush->MediaObject;
               BackgroundBrush->MediaObject = NULL;
            }
            QDomElement SubElement = Element.elementsByTagName("Background").item(0).toElement();
            BackgroundType = SubElement.attribute("BackgroundType")=="1"; // Background type : false=same as precedent - true=new background definition
            bool ModifyFlag;
            if (!BackgroundBrush->LoadFromXML(&SubElement,"BackgroundBrush",PathForRelativPath,AliasList,&ModifyFlag,ResKeyList,DuplicateRes)) 
               IsOk = false;
            if (IsOk && ModifyFlag) 
               ((MainWindow *)pAppConfig->TopLevelWindow)->SetModifyFlag(true);
             if (ModifyFlag) 
               ((MainWindow *)pAppConfig->TopLevelWindow)->SetModifyFlag(true);
         }

      }

      // Global blocks composition table
      IsOk = ObjectComposition.LoadFromXML(Element,"ObjectComposition",PathForRelativPath,NULL,AliasList,pAppConfig,ResKeyList,DuplicateRes);         // ObjectComposition

      // Shots definitions
      int ShotNumber = Element.attribute("ShotNumber").toInt();
      for (int i = 0; i < ShotNumber; i++) 
      {
         cDiaporamaShot *imagesequence = new cDiaporamaShot(this);
         if (!imagesequence->LoadFromXML(Element,"Shot-"+QString("%1").arg(i),PathForRelativPath,&ObjectComposition,AliasList,ResKeyList,DuplicateRes)) 
            IsOk = false;
         shotList.append(imagesequence);
      }

      // fix locations definition in shots for version <2.1 20131214
      QList<cBrushDefinition::sMarker> FirstMarkers;
      for (int Obj = 0; Obj < ObjectComposition.compositionObjects.count(); Obj++) 
         if ((ObjectComposition.compositionObjects[Obj]->BackgroundBrush->MediaObject) && (ObjectComposition.compositionObjects[Obj]->BackgroundBrush->MediaObject->is_Gmapsmap())) 
         {
            for (int Shot = 0; Shot < shotList.count(); Shot++)
               for (int ShotObj = 0; ShotObj < shotList[Shot]->ShotComposition.compositionObjects.count(); ShotObj++)
                  if (ObjectComposition.compositionObjects[Obj]->index() == shotList[Shot]->ShotComposition.compositionObjects[ShotObj]->index())
                  {
                     if (shotList[Shot]->ShotComposition.compositionObjects[ShotObj]->BackgroundBrush->Markers.isEmpty())
                     {
                        if (Shot == 0) 
                        {
                           for (int Marker = 0; Marker < ((cGMapsMap *)ObjectComposition.compositionObjects[Obj]->BackgroundBrush->MediaObject)->locations.count(); Marker++) 
                           {
                              cBrushDefinition::sMarker MarkerObj;
                              MarkerObj.MarkerColor="#ffffff";
                              MarkerObj.TextColor="#000000";
                              MarkerObj.Visibility=cBrushDefinition::sMarker::MARKERSHOW;
                              shotList[Shot]->ShotComposition.compositionObjects[ShotObj]->BackgroundBrush->Markers.append(MarkerObj);
                           }
                           FirstMarkers = shotList[Shot]->ShotComposition.compositionObjects[ShotObj]->BackgroundBrush->Markers;
                        } 
                        else 
                           shotList[Shot]->ShotComposition.compositionObjects[ShotObj]->BackgroundBrush->Markers = FirstMarkers;
                     }
                  }
         }

      // Bug fix for ffDRevision between 1.7b3 and 2.0b3
      if ((ElementName != TITLEMODEL_ELEMENTNAME) && (pDiaporama->ProjectInfo()->Revision() > "20131016") && (pDiaporama->ProjectInfo()->Revision() < "20131112")) 
      {
         int AutoTSNumber = GetAutoTSNumber();
         if (AutoTSNumber != -1) 
         {
            int         ModelType    = (AutoTSNumber/100000);
            int         ModelSubType =(AutoTSNumber%100000)/10000;
            int         ModelIndex  = AutoTSNumber-ModelType*100000-ModelSubType*10000;
            cModelList  *ModelList  = (ModelType == ffd_MODELTYPE_PROJECTTITLE) ? pAppConfig->PrjTitleModels[pDiaporama->ImageGeometry][ModelSubType == 9 ? MODELTYPE_PROJECTTITLE_CATNUMBER-1 : ModelSubType] :
                                         (ModelType == ffd_MODELTYPE_CHAPTERTITLE) ? pAppConfig->CptTitleModels[pDiaporama->ImageGeometry][ModelSubType== 9 ? MODELTYPE_CHAPTERTITLE_CATNUMBER-1 : ModelSubType]:
                                         (ModelType == ffd_MODELTYPE_CREDITTITLE) ? pAppConfig->CreditTitleModels[pDiaporama->ImageGeometry][ModelSubType== 9 ? MODELTYPE_CREDITTITLE_CATNUMBER-1 : ModelSubType]:
                                         NULL;
            if ((ModelList) && (ModelIndex >= 0) && (ModelIndex<ModelList->List.count()))
               LoadModelFromXMLData((ffd_MODELTYPE)ModelType,ModelList->List[ModelIndex]->Model,ResKeyList,true);  // Always duplicate ressource
         }
      }

      // Refresh OptimisationFlags
      ComputeOptimisationFlags();

      //**** Compatibility with version prior to 1.5
      for (int i = 0; i < ObjectComposition.compositionObjects.count(); i++) 
      {
         if ((ObjectComposition.compositionObjects.at(i)->BackgroundBrush->OnOffFilters() != 0) || (ObjectComposition.compositionObjects.at(i)->BackgroundBrush->GaussBlurSharpenSigma() != 0)) 
         {
            for (int j = 0; j < shotList.count(); j++)
               for (int k = 0; k < shotList.at(j)->ShotComposition.compositionObjects.count(); k++)
                  if (shotList.at(j)->ShotComposition.compositionObjects.at(k)->index() == ObjectComposition.compositionObjects.at(i)->index())
                  {
                     shotList.at(j)->ShotComposition.compositionObjects.at(k)->BackgroundBrush->setOnOffFilters(ObjectComposition.compositionObjects.at(i)->BackgroundBrush->OnOffFilters());
                     shotList.at(j)->ShotComposition.compositionObjects.at(k)->BackgroundBrush->setGaussBlurSharpenSigma( ObjectComposition.compositionObjects.at(i)->BackgroundBrush->GaussBlurSharpenSigma());
                     shotList.at(j)->ShotComposition.compositionObjects.at(k)->BackgroundBrush->setBlurSharpenRadius( ObjectComposition.compositionObjects.at(i)->BackgroundBrush->BlurSharpenRadius());
                     shotList.at(j)->ShotComposition.compositionObjects.at(k)->BackgroundBrush->setTypeBlurSharpen( ObjectComposition.compositionObjects.at(i)->BackgroundBrush->TypeBlurSharpen());
                  }
            ObjectComposition.compositionObjects.at(i)->BackgroundBrush->setOnOffFilters(0);
            ObjectComposition.compositionObjects.at(i)->BackgroundBrush->setGaussBlurSharpenSigma(0);
            ObjectComposition.compositionObjects.at(i)->BackgroundBrush->setBlurSharpenRadius(5);
         }
      }
      return IsOk;
   } 
   else 
      return false;
}

//*********************************************************************************************************************************************
//
// Global class containing media objects
//
//*********************************************************************************************************************************************

cDiaporama::cDiaporama(cApplicationConfig *TheApplicationConfig,bool LoadDefaultModel,QObject *Parent) : QObject(Parent) , masterSound(true)
{
   setObjectName("cDiaporama");
   applicationConfig           = TheApplicationConfig;
   projectInfo                 = new cffDProjectFile(applicationConfig);
   ProjectThumbnail            = new cDiaporamaObject(this);
   CurrentCol                  = -1;                                                               // Current selected item
   CurrentPosition             = -1;                                                               // Current position (msec)
   CurrentChapter              = -1;
   CurrentChapterName          = QString("");
   IsModify                    = false;                                                            // true if project was modify
   ProjectFileName             = "";                                                               // Path and name of current file project
   projectInfo->setComposer(QString(APPLICATION_NAME)+QString(" ")+CurrentAppName);
   projectInfo->setAuthor(applicationConfig->DefaultAuthor);
   projectInfo->setAlbum(applicationConfig->DefaultAlbum);
   projectInfo->setDefaultLanguage(applicationConfig->DefaultLanguage);
   TransitionSpeedWave         = applicationConfig->DefaultTransitionSpeedWave;                    // Speed wave for transition
   BlockAnimSpeedWave          = applicationConfig->DefaultBlockAnimSpeedWave;                     // Speed wave for block animation
   ImageAnimSpeedWave          = applicationConfig->DefaultImageAnimSpeedWave;                     // Speed wave for image framing and correction animation

   // Set default value
   DefineSizeAndGeometry(applicationConfig->ImageGeometry);                                // Default to 16:9

   if (LoadDefaultModel) 
   {
      // Load default thumbnail
      ThumbnailName = applicationConfig->DefaultThumbnailName;
      int ModelIndex = applicationConfig->ThumbnailModels->SearchModel(applicationConfig->DefaultThumbnailName);
      if ((ModelIndex > 0) && (ModelIndex < applicationConfig->ThumbnailModels->List.count()))
         ProjectThumbnail->LoadModelFromXMLData(ffd_MODELTYPE_THUMBNAIL,applicationConfig->ThumbnailModels->List[ModelIndex]->Model,
                                                &applicationConfig->ThumbnailModels->List[ModelIndex]->ResKeyList,true);    // always duplicate ressource
   }
}

//====================================================================================================================

cDiaporama::~cDiaporama() 
{
   if (projectInfo) 
   {
      delete projectInfo;
      projectInfo = NULL;
   }
   if (ProjectThumbnail) 
   {
      delete ProjectThumbnail;
      ProjectThumbnail = NULL;
   }
   while (slides.count() > 0) 
      delete slides.takeLast();
}

//====================================================================================================================

void cDiaporama::UpdateInformation() 
{
   UpdateChapterInformation();
   UpdateStatInformation();
   UpdateCachedInformation();
}

void cDiaporama::UpdateChapterInformation() 
{
   // Remove all chapters information
   int i=0;
   while ( i < projectInfo->chapterProps()->count()) 
   {
      if (projectInfo->chapterProps()->at(i).startsWith("Chapter_")) 
         projectInfo->chapterProps()->removeAt(i);
      else 
         i++;
   }
   projectInfo->setNumChapters(0);
   // Create new
   for (int i = 0; i < slides.count(); i++) 
   {
      if (i == 0 || slides[i]->StartNewChapter)
      {
         if (slides[i]->StartNewChapter) 
            projectInfo->setNumChapters(projectInfo->numChapters()+1);
         QString ChapterNum = QString("%1").arg(projectInfo->numChapters()); 
         while (ChapterNum.length() < 3) 
            ChapterNum="0"+ChapterNum;
         int NextChapter = i+1;
         int64_t Start = GetObjectStartPosition(i) + ( i > 0 ? slides[i]->GetTransitDuration() : 0) - GetObjectStartPosition(0);
         int64_t Duration = slides[i]->GetDuration() - ( i > 0 ? slides[i]->GetTransitDuration() : 0);
         if (NextChapter < slides.count()) 
            Duration = Duration-slides[NextChapter]->GetTransitDuration();
         while ((NextChapter < slides.count()) && (!slides[NextChapter]->StartNewChapter)) 
         {
            Duration = Duration+slides[NextChapter]->GetDuration();
            NextChapter++;
            if (NextChapter < slides.count()) 
               Duration = Duration-slides[NextChapter]->GetTransitDuration();
         }
         QStringList *chapterProps = projectInfo->chapterProps();
         QString chapterPrep = "Chapter_" + ChapterNum;
         chapterProps->append(chapterPrep + ":InSlide"         + QString("##") + QString("%1").arg(i+1));
         chapterProps->append(chapterPrep + ":Start"           + QString("##") + QTime(0,0,0,0).addMSecs(Start).toString("hh:mm:ss.zzz"));
         chapterProps->append(chapterPrep + ":End"             + QString("##") + QTime(0,0,0,0).addMSecs(Start+Duration).toString("hh:mm:ss.zzz"));
         chapterProps->append(chapterPrep + ":Duration"        + QString("##") + QTime(0,0,0,0).addMSecs(Duration).toString("hh:mm:ss.zzz"));
         chapterProps->append(chapterPrep + ":title"           + QString("##") + (slides[i]->StartNewChapter?slides[i]->ChapterName:projectInfo->Title()));
         chapterProps->append(chapterPrep + ":Date"            + QString("##") + (slides[i]->OverrideProjectEventDate ? slides[i]->ChapterEventDate : projectInfo->EventDate()).toString(applicationConfig->ShortDateFormat));
         chapterProps->append(chapterPrep + ":LongDate"        + QString("##") + (slides[i]->OverrideProjectEventDate ? slides[i]->OverrideChapterLongDate ? slides[i]->ChapterLongDate : FormatLongDate(slides[i]->ChapterEventDate) : projectInfo->LongDate()));
         chapterProps->append(chapterPrep + ":LocationName"    + QString("##") + (slides[i]->ChapterLocation ? ((cLocation *)slides[i]->ChapterLocation)->Name : projectInfo->Location() ? projectInfo->Location()->Name : QApplication::translate("Variables","Project's location not set (Name)")));
         chapterProps->append(chapterPrep + ":LocationAddress" + QString("##") + (slides[i]->ChapterLocation ? ((cLocation *)slides[i]->ChapterLocation)->FriendlyAddress : projectInfo->Location() ? projectInfo->Location()->FriendlyAddress : QApplication::translate("Variables","Project's location not set (Address)")));
      }
   }
   projectInfo->SetRealDuration(QTime(0,0,0,0).addMSecs(GetDuration()));
}

void cDiaporama::UpdateStatInformation() 
{
   QString Text;
   for (int var = 0; var < Variable.Variables.count(); var++) 
   {
      if (Variable.Variables[var].VarName == "STP") 
      {
         // Parse all object to construct values
         QTime VideoDuration = QTime(0,0,0,0);
         int   NbrVideo = 0;
         int   NbrVectorImg = 0;
         int   NbrImage = 0;
         int   NbrText = 0;
         int   NbrAutoSlide = 0;

         //for (int i = 0; i < List.count(); i++) 
         for( auto diaporamaObject : slides)
         {
            if (diaporamaObject->GetAutoTSNumber() == -1)
            {
               //for (int j = 0; j < diaporamaObject->ObjectComposition.List.count(); j++) 
               for(auto compositionObject :  diaporamaObject->ObjectComposition.compositionObjects)
               {
                  if ( compositionObject->isVideo() ) 
                  {
                     NbrVideo++;
                     VideoDuration = VideoDuration.addMSecs(QTime(0,0,0,0).msecsTo(compositionObject->BackgroundBrush->MediaObject->GetRealDuration()));
                  } 
                  else if ((compositionObject->BackgroundBrush->MediaObject) && (compositionObject->BackgroundBrush->MediaObject->is_Imagevector())) 
                  {
                     NbrVectorImg++;
                  } 
                  else if ((compositionObject->BackgroundBrush->MediaObject) && (compositionObject->BackgroundBrush->MediaObject->is_Imagefile())) 
                  {
                     NbrImage++;
                     //Composer.Add(List[i]->ObjectComposition.List[j]->BackgroundBrush->Image->GetInformationValue("composer"));
                  } 
                  else 
                     NbrText++;
               }
            } 
            else 
               NbrAutoSlide++;
         }

         Text = QApplication::translate("Variables","Content:","Project statistics");
         if (slides.count())             Text = Text + (Text.isEmpty()?"·":"\n·") + QApplication::translate("Variables", "%1 slides (%2)").arg(slides.count()).arg(projectInfo->GetRealDuration().toString("hh:mm:ss.zzz"));
         if (projectInfo->numChapters()) Text = Text + (Text.isEmpty()?"·":"\n·") + QApplication::translate("Variables", "%1 chapters").arg(projectInfo->numChapters());
         if (NbrVideo)                 Text = Text + (Text.isEmpty()?"·":"\n·") + QApplication::translate("Variables", "%1 videos (%2)").arg(NbrVideo).arg(VideoDuration.toString("hh:mm:ss.zzz"));
         if (NbrVectorImg)             Text = Text + (Text.isEmpty()?"·":"\n·") + QApplication::translate("Variables", "%3 vector images").arg(NbrVectorImg);
         if (NbrImage)                 Text = Text + (Text.isEmpty()?"·":"\n·") + QApplication::translate("Variables", "%4 photos").arg(NbrImage);
         if (NbrText)                  Text = Text + (Text.isEmpty()?"·":"\n·") + QApplication::translate("Variables", "%5 text blocks").arg(NbrText);
         if (NbrAutoSlide)             Text = Text + (Text.isEmpty()?"·":"\n·") + QApplication::translate("Variables", "%6 automatic slides").arg(NbrAutoSlide);
         //for (int j=0;j<Composer.List.count();j++) Variable.Variables[i].Value=Variable.Variables[i].Value+QString("    %1: %2<br>").arg(Composer.List[j].Composer).arg(Composer.List[j].Count);
         Variable.Variables[var].Value = HTMLConverter()->ToPlainText(Text);
      } 
      else if (Variable.Variables[var].VarName=="STM") 
      {
         // Parse all object to construct values
         QStringList MusicList;
         Text = QApplication::translate("Variables", "Musical content:", "Project statistics");
         for (auto slide : slides) 
         {
            if (slide->MusicType) 
            {
               for (int music = 0; music < slide->MusicList.count(); music++) 
               {
                  if (slide->MusicList.MusicObjects[music]->AllowCredit) 
                  {
                     QStringList TempExtProperties;
                     applicationConfig->FilesTable->GetExtendedProperties(slide->MusicList.MusicObjects[music]->FileKey(),&TempExtProperties);
                     QString TMusc = GetInformationValue("title",&TempExtProperties);
                     QString Album = GetInformationValue("album",&TempExtProperties);
                     QString Date  = GetInformationValue("date",&TempExtProperties);
                     QString Artist= GetInformationValue("artist",&TempExtProperties);
                     QString SubText=(!TMusc.isEmpty()?TMusc:slide->MusicList.MusicObjects[music]->ShortName());
                     if (!Artist.isEmpty()) 
                     {
                        if (!Date.isEmpty())  
                           SubText = SubText + QApplication::translate("Variables", " - © %1 (%2)", "Project statistics-Music").arg(Artist).arg(Date);
                        else
                           SubText = SubText + QApplication::translate("Variables", " - © %1",     "Project statistics-Music").arg(Artist);
                     }
                     if (!Album.isEmpty()) 
                        SubText = SubText + QApplication::translate("Variables", " from «%1»",       "Project statistics-Music").arg(Album);
                     if( !MusicList.contains(SubText) )
                        MusicList.append(SubText);
                  }
               }
            }
         }
         if (MusicList.count() > 0) 
         {
            for (int i = 0; i < MusicList.count(); i++) 
               Text = Text + (MusicList.count() > 1 ? "\n·" : " ") + MusicList[i];
            Variable.Variables[var].Value=HTMLConverter()->ToPlainText(Text);
         } 
         else 
            Variable.Variables[var].Value = " .";
      }
   }
}

//====================================================================================================================

cDiaporamaObject *cDiaporama::GetChapterDefObject(cDiaporamaObject *Object) 
{
   // Find current Object
   cDiaporamaObject *CurChapter = NULL;
   int              ObjectNum = 0;
   while ((ObjectNum < slides.count()) && (slides[ObjectNum] != Object)) 
   {
      if (slides[ObjectNum]->StartNewChapter) 
         CurChapter = slides[ObjectNum];
      ObjectNum++;
   }
   return CurChapter;
}

cDiaporamaObject* cDiaporama::insertNewSlide(int index)
{
   cDiaporamaObject* newDiaporamaObject = new cDiaporamaObject(this);
   slides.insert(index, newDiaporamaObject);
   return newDiaporamaObject;
}

//====================================================================================================================

void cDiaporama::DefineSizeAndGeometry(ffd_GEOMETRY Geometry) 
{
   ImageGeometry   = Geometry;
   InternalHeight  = PREVIEWMAXHEIGHT;
   InternalWidth   = GetWidthForHeight(InternalHeight);
   LumaList_Bar.SetGeometry(ImageGeometry);
   //LumaList_Center.SetGeometry(ImageGeometry);
   LumaList_Center.SetGeometry(GEOMETRY_4_3);
   LumaList_Checker.SetGeometry(ImageGeometry);
   LumaList_Clock.SetGeometry(ImageGeometry);
   LumaList_Box.SetGeometry(ImageGeometry);
   LumaList_Snake.SetGeometry(ImageGeometry);
   switch (Geometry) 
   {
      case GEOMETRY_40_17: projectInfo->setGeometry(IMAGE_GEOMETRY_40_17);  break;
      case GEOMETRY_4_3:   projectInfo->setGeometry(IMAGE_GEOMETRY_4_3);    break;
      default:             projectInfo->setGeometry(IMAGE_GEOMETRY_16_9);   break;
   }
}

//=======================================================
// Return height for width depending on project geometry
//=======================================================
int cDiaporama::GetHeightForWidth(int WantedWith) 
{
   switch (ImageGeometry) 
   {
      case GEOMETRY_4_3:   return int(double(3)*(double(WantedWith)/double(4)));   break;
      case GEOMETRY_40_17: return int(double(17)*(double(WantedWith)/double(40))); break;
      default:             return int(double(9)*(double(WantedWith)/double(16)));  break;
   }
   return 0;
}

//=======================================================
// Return width for height depending on project geometry
//=======================================================
int cDiaporama::GetWidthForHeight(int WantedHeight) 
{
   switch (ImageGeometry) 
   {
      case GEOMETRY_4_3:   return int(double(4)*(double(WantedHeight)/double(3)));   break;
      case GEOMETRY_40_17: return int(double(40)*(double(WantedHeight)/double(17))); break;
      default:             return int(double(16)*(double(WantedHeight)/double(9)));  break;
   }
   return 0;
}

//====================================================================================================================

int64_t cDiaporama::GetTransitionDuration(int index) 
{
   int64_t Duration = 0;
   if ((index >= 0) && (slides.count() > 0) && ( (index < slides.count() ) && ( !((slides[index]->TransitionFamily == 0) && (slides[index]->TransitionSubType == 0))))) 
      Duration = slides[index]->TransitionDuration;
   return Duration;
}

//====================================================================================================================

int64_t cDiaporama::GetDuration() 
{
   int64_t Duration = 0;
   for (int i = 0; i < slides.count(); i++) 
   {
      int64_t ObjDuration = slides[i]->GetDuration() - GetTransitionDuration(i+1);
      Duration += (ObjDuration >= 33 ? ObjDuration : 33);
   }
   return Duration;
}

//====================================================================================================================

int64_t cDiaporama::GetPartialDuration(int from,int to) 
{
   if (from < 0)
      from = 0;
   if (from >= slides.count()) 
      from = slides.count()-1;
   if (to < 0)
      to = 0;
   if (to >= slides.count())
      to = slides.count()-1;
   int64_t Duration = 0;
   for (int i = from; i <= to; i++) 
   {
      int64_t ObjDuration = slides[i]->GetDuration() - GetTransitionDuration(i+1);
      Duration += (ObjDuration >= 33 ? ObjDuration : 33);
   }
   return Duration;
}

//====================================================================================================================

int64_t cDiaporama::GetObjectStartPosition(int index) 
{
   int64_t Duration = 0;
   if ((index >= slides.count()) && (slides.count() > 0)) 
   {
      index = slides.count()-1;
      Duration = slides[index]->GetDuration();
   }
   for (int i = 0; i < index; i++) 
   {
      int64_t ObjDuration = slides[i]->GetDuration()-GetTransitionDuration(i+1);
      Duration += (ObjDuration >= 33 ? ObjDuration : 33);
   }
   return Duration;
}

//====================================================================================================================

int cDiaporama::GetObjectIndex(cDiaporamaObject *ObjectToFind) 
{
   return slides.indexOf(ObjectToFind);
}

//====================================================================================================================

void cDiaporama::PrepareBackground(int Index, QSize size, QPainter *Painter, int AddX, int AddY) 
{
   // Make painter translation to ensure QBrush image will start at AddX AddY position
   Painter->save();
   Painter->translate(AddX,AddY);
   Painter->fillRect(QRect(QPoint(0,0),size),QBrush(Qt::black));
   if (Index >= 0 && Index < slides.count()) 
   {
      QBrush *BR = slides[slides[Index]->CachedBackgroundIndex]->BackgroundBrush->GetBrush(size,true,0,NULL,1,NULL);
      if (BR)
      {
         Painter->fillRect(QRect(QPoint(0, 0), size), *BR);
         delete BR;
      }
   }
   Painter->restore();
}

//====================================================================================================================

void cDiaporama::UpdateCachedInformation() 
{
   int64_t          StartPosition   = 0;
   int              MusicIndex      = -1;
   int              MusicListIndex  = -1;
   int64_t          CumulDuration   = 0;
   int64_t          CurMusicDuration= 0;
   int              BackgroundIndex = 0;
   cDiaporamaObject *PrevObject     = NULL;
   cMusicObject     *CurMusic       = NULL;
   cMusicObject     *PrevMusic      = NULL;

   int64_t MusicListOffset;

   for (int DiaporamaObjectNum = 0; DiaporamaObjectNum < slides.count(); DiaporamaObjectNum++) 
   {
      //qDebug() << "UpdateCachedInformation for object #" << DiaporamaObjectNum;
      cDiaporamaObject *CurObject = slides[DiaporamaObjectNum];

      bool   HaveSound  = false;
      bool   HaveFilter = false;
      double SoundVolume=0;

      // Owner of the background
      if (CurObject->BackgroundType) 
         BackgroundIndex = DiaporamaObjectNum;

      // Owner and start position of the music
      if (DiaporamaObjectNum == 0 || CurObject->MusicType) 
      {
         StartPosition    = 0;
         MusicIndex       = DiaporamaObjectNum;
         CumulDuration    = 0;
         MusicListIndex   = 0;
         CurMusic         = MusicListIndex < slides[MusicIndex]->MusicList.count() ? slides[MusicIndex]->MusicList.MusicObjects[MusicListIndex] : NULL;
         CurMusicDuration = CurMusic?QTime(0,0,0,0).msecsTo(CurMusic->GetDuration()):0;
         if(CurMusic && MusicListIndex == 0 && CurMusic->startCode == cMusicObject::eFrameRelated_Start)
            CurMusic->startOffset = GetObjectStartPosition(DiaporamaObjectNum);
         MusicListOffset = 0;
         //qDebug() << "StartPosition " << StartPosition << " MusicIndex " << MusicIndex << " CurMusicDuration " << CurMusicDuration;
      }

      // Parse ObjectComposition table to determine if slide have sound
      foreach(cCompositionObject* obj,CurObject->ObjectComposition.compositionObjects)
         if( obj->isVideo() )
            HaveSound = true;

      // Parse shots and objects in shot to determine if slide have filter and max SoundVolume
      for (int shot = 0; shot < CurObject->shotList.count(); shot++)
      {
         for (int ObjNum = 0; ObjNum < CurObject->shotList[shot]->ShotComposition.compositionObjects.count(); ObjNum++)
         {
            cCompositionObject *CompoObject = CurObject->shotList[shot]->ShotComposition.compositionObjects[ObjNum];
            if (CompoObject->isVisible() && CompoObject->BackgroundBrush) 
               SoundVolume = CompoObject->BackgroundBrush->SoundVolume;
            HaveFilter = CompoObject->hasFilter();
         }
      }
      CurObject->CachedDuration         = qBound(int64_t(33),CurObject->GetDuration(),CurObject->GetDuration());
      CurObject->CachedTransitDuration  = CurObject->GetTransitDuration();
      CurObject->CachedStartPosition    = StartPosition;
      CurObject->CachedMusicIndex       = MusicIndex;
      CurObject->CachedMusicListIndex   = MusicListIndex;
      CurObject->CachedBackgroundIndex  = BackgroundIndex;
      CurObject->CachedHaveSound        = HaveSound;
      CurObject->CachedSoundVolume      = SoundVolume;
      CurObject->CachedHaveFilter       = HaveFilter;

      // Compute information about music track
      if (CurMusic && PrevMusic && CurMusic == PrevMusic && PrevObject->CachedMusicRemaining <= CurObject->CachedTransitDuration) 
      {
         //qDebug() << " PrevObject->CachedMusicRemaining "  << PrevObject->CachedMusicRemaining<< " CurObject->CachedTransitDuration " <<CurObject->CachedTransitDuration;
         CurMusic = NULL;
      }

      int64_t TrackDuration = CurMusic ? QTime(0,0,0,0).msecsTo(CurMusic->GetDuration()) : 0;
      int64_t DurationLeft  = TrackDuration > CurObject->CachedStartPosition ? TrackDuration - CurObject->CachedStartPosition : 0;
      int64_t NextTransition = PrevObject ? CurObject->GetTransitDuration() : 0;
      int64_t TimePlayed = CurObject->MusicPause ?
         ((!PrevObject) || (!PrevObject->MusicPause) ? CurObject->CachedTransitDuration : 0) :
         DurationLeft > CurObject->CachedDuration-NextTransition ? CurObject->CachedDuration-NextTransition : DurationLeft;
      //qDebug() << "TrackDuration " << TrackDuration << " DurationLeft " << DurationLeft << " NextTransition " << NextTransition << " TimePlayed " << TimePlayed;

      CurObject->CachedMusicTimePlayed = TimePlayed;
      CurObject->CachedMusicRemaining  = DurationLeft > TimePlayed ? DurationLeft-TimePlayed : 0;
      CurObject->CachedPrevMusicFadOUT = PrevMusic && PrevMusic != CurMusic && PrevObject->CachedMusicRemaining > 0 && CurObject->CachedTransitDuration > 0;
      CurObject->CachedMusicFadIN      = CurMusic && PrevMusic && PrevMusic != CurMusic && CurObject->CachedPrevMusicFadOUT && PrevObject->CachedMusicRemaining > 0;
      CurObject->CachedMusicEnd        = CurMusic && (TimePlayed < CurObject->CachedDuration - NextTransition);
      //if( CurMusic )
      //   qDebug() << "TimePlayed " << TimePlayed << " CurObject->CachedDuration " << CurObject->CachedDuration << " NextTransition " << NextTransition;
      CurObject->CachedPrevMusicEnd   = (PrevObject) && (PrevMusic) && (CurObject->CachedTransitDuration > 0) && (PrevObject->CachedMusicRemaining > 0) 
         && (PrevObject->CachedMusicRemaining <= CurObject->CachedTransitDuration);

      // prepare next loop
      PrevObject = CurObject;
      PrevMusic = CurMusic;
      if ((CumulDuration + TimePlayed + NextTransition) >= CurMusicDuration) 
      {
         MusicListIndex++;
         CumulDuration = 0;
         StartPosition = 0;
         if ((MusicIndex >= 0) && (MusicIndex < slides.count()) && (MusicListIndex >= 0) && (MusicListIndex < slides[MusicIndex]->MusicList.count())) 
         {
            CurMusic         = MusicListIndex<slides[MusicIndex]->MusicList.count() ? slides[MusicIndex]->MusicList.MusicObjects[MusicListIndex] : NULL;
            CurMusicDuration = CurMusic ? QTime(0,0,0,0).msecsTo(CurMusic->GetDuration()) : 0;
         } 
         else 
         {
            CurMusic = NULL;
            CurMusicDuration = 0;
            MusicIndex = -1;
            MusicListIndex = -1;
         }
      } 
      else 
      {
         StartPosition += TimePlayed;
         CumulDuration = CumulDuration + TimePlayed;
      }
      CurObject->CachedMusicListOffset = MusicListOffset;
      MusicListOffset += CurObject->MusicPause ? 0 : CurObject->GetDuration();
      //qDebug() << " ";
   }
}

//====================================================================================================================

cMusicObject *cDiaporama::GetMusicObject(int ObjectIndex, int64_t &StartPosition, int *CountObject, int *IndexObject) 
{
   if (ObjectIndex >= slides.count()) 
      return NULL;

   StartPosition = slides[ObjectIndex]->CachedStartPosition;
   if (IndexObject) 
      *IndexObject = slides[ObjectIndex]->CachedMusicIndex;
   if (CountObject) 
   {
      *CountObject = 0;
      if (ObjectIndex >= 0 && ObjectIndex < slides.count()) 
      {
         int i = 0;
         while (i < slides.count() &&  i < slides[ObjectIndex]->CachedMusicIndex) 
         {
            if (slides[i]->MusicType) 
               *CountObject = (*CountObject)+1;
            i++;
         }
      }
   }
   cDiaporamaObject *obj = slides[ObjectIndex];
   if ( (obj->CachedMusicIndex >= 0) && (obj->CachedMusicIndex < slides.count())
        && (obj->CachedMusicListIndex >= 0) && (obj->CachedMusicListIndex < slides[obj->CachedMusicIndex]->MusicList.count())
      )
      return slides[obj->CachedMusicIndex]->MusicList.MusicObjects[obj->CachedMusicListIndex];
   return NULL;
}

//====================================================================================================================
#define STREAM_SAVING
#define noBASE64SAVING
bool cDiaporama::SaveFile(QWidget *ParentWindow, cReplaceObjectList *ReplaceList, QString *ExportFileName) 
{
   QFile            file( (ReplaceList != NULL && ExportFileName != NULL) ? *ExportFileName : ProjectFileName);
   #ifndef STREAM_SAVING
   QDomDocument     domDocument(APPLICATION_NAME);
   
   QDomElement      Element;
   QDomElement      root;
   #endif
   QList<qlonglong> ResKeyList;

   #ifdef STREAM_SAVING
   QFile            fileStream(file.fileName() /*+ "_ex"*/);
   #endif

   UpdateChapterInformation();
   if (!ReplaceList) 
   {
      if (projectInfo->Title() == "" && applicationConfig->DefaultTitleFilling != 0) 
      {
         if (applicationConfig->DefaultTitleFilling == 1) 
         {
            // Fill with project name when project save (if not yet defined)
            projectInfo->setTitle(QFileInfo(ProjectFileName).completeBaseName());
         } 
         else if (applicationConfig->DefaultTitleFilling == 2) 
         {
            // Fill with project folder when project save (if not yet defined)
            projectInfo->setTitle(ProjectFileName);
            if (projectInfo->Title() != "") 
            {
               projectInfo->setTitle(QFileInfo(projectInfo->Title()).absolutePath());
               if (projectInfo->Title().endsWith(QDir::separator())) 
               {
                  projectInfo->setTitle(QFileInfo(ProjectFileName).baseName());
               } 
               else if (projectInfo->Title().lastIndexOf(QDir::separator()) > 0) 
                  projectInfo->setTitle(projectInfo->Title().mid(projectInfo->Title().lastIndexOf(QDir::separator())+1));
            }
         }
         projectInfo->Title().truncate(30);
      }
      if (projectInfo->Author() == "") 
         projectInfo->setAuthor(applicationConfig->DefaultAuthor);
      if (projectInfo->Album() == "")  
         projectInfo->setAlbum(applicationConfig->DefaultAlbum);
      projectInfo->setComposer(QString(APPLICATION_NAME)+QString(" ")+CurrentAppName);
   }
   projectInfo->setRevision(CurrentAppVersion);

   #ifndef STREAM_SAVING
   // Create xml document and root
   root = domDocument.createElement(APPLICATION_ROOTNAME);
   domDocument.appendChild(root);

   // Save project properties
   projectInfo->SaveToXML(&root,"",ProjectFileName,false,ReplaceList,&ResKeyList,false);
   #endif

   #ifdef STREAM_SAVING
   if (!fileStream.open(QFile::WriteOnly | QFile::Text)) 
   {
      if (ParentWindow != NULL) 
         CustomMessageBox(NULL,QMessageBox::Critical,QApplication::translate("MainWindow","Error","Error message"),QApplication::translate("MainWindow","Error creating data file","Error message"),QMessageBox::Close);
      else  
         printf("%s\n",QApplication::translate("MainWindow","Error creating data file","Error message").toLocal8Bit().constData());
      return false;
   }
   ExXmlStreamWriter xmlWriter(&fileStream);
   xmlWriter.writeDTD(QString("<!DOCTYPE %1>\n").arg(APPLICATION_NAME));
   xmlWriter.setAutoFormatting(true);
   xmlWriter.writeStartElement(APPLICATION_ROOTNAME);
   projectInfo->SaveToXMLex(xmlWriter,ProjectFileName,false,ReplaceList,&ResKeyList,false);
   QList<int> trfams;
   for (int i = 0; i < slides.count(); i++)
   {
      int trFam = slides[i]->TransitionFamily;
      if(trFam >= 99 && !trfams.contains(trFam))
         trfams.append(trFam);
   }
   if (trfams.count())
   {
      xmlWriter.writeStartElement("TransitionPlugins");
      for (int i = 0; i < trfams.count(); i++)
      {
         TransitionInterface *iFace = getTransitionInterface(trfams.at(i));
         xmlWriter.writeTextElement("required", iFace->pluginName());
      }
      xmlWriter.writeEndElement();
   }
   #endif

   #ifdef STREAM_SAVING
   ProjectThumbnail->SaveToXMLex(xmlWriter,THUMBMODEL_ELEMENTNAME,ProjectFileName,false,ReplaceList,&ResKeyList,true);
   #else
   ProjectThumbnail->SaveToXML(root, THUMBMODEL_ELEMENTNAME, ProjectFileName, false, ReplaceList, &ResKeyList, true);

   // Save basic information on project
   Element = domDocument.createElement("Project"); 
   Element.setAttribute("ImageGeometry", ImageGeometry);
   #endif

   #ifdef STREAM_SAVING
   xmlWriter.writeStartElement("Project");
   xmlWriter.writeAttribute("ImageGeometry", ImageGeometry);
   xmlWriter.writeAttribute("ObjectNumber", slides.count());
   xmlWriter.writeEndElement();
   #endif

   #ifndef STREAM_SAVING
   // Save object list
   Element.setAttribute("ObjectNumber",slides.count());
   root.appendChild(Element);
   #endif

   for (int i = 0; i < slides.count(); i++) 
   {
      #ifdef STREAM_SAVING
      slides[i]->SaveToXMLex(xmlWriter, "Object-" + (QString("%1").arg(i, 10)).trimmed(), ProjectFileName, false, ReplaceList, &ResKeyList, true);
      #else
      slides[i]->SaveToXML(root, "Object-" + (QString("%1").arg(i, 10)).trimmed(), ProjectFileName, false, ReplaceList, &ResKeyList, true);
      #endif
   }

   #ifndef STREAM_SAVING
   // Write file to disk
   if (!file.open(QFile::WriteOnly | QFile::Text)) 
   {
      if (ParentWindow != NULL) 
         CustomMessageBox(NULL,QMessageBox::Critical,QApplication::translate("MainWindow","Error","Error message"),QApplication::translate("MainWindow","Error creating data file","Error message"),QMessageBox::Close);
      else  
         printf("%s\n",QApplication::translate("MainWindow","Error creating data file","Error message").toLocal8Bit().constData());
      return false;
   }
   #endif

   #ifdef STREAM_SAVING
   xmlWriter.writeEndDocument();
   #endif

   // temp: make ResKeyList uniq
   QList<qlonglong> nResKeyList;
   foreach(qlonglong value, ResKeyList) 
      if( !nResKeyList.contains(value))
         nResKeyList.append(value);
   ResKeyList = nResKeyList;

   #ifndef STREAM_SAVING
   // Save ffDPart in file now
   QTextStream out(&file);
   out.setCodec("UTF-8");
   domDocument.save(out,4);
   #endif

   // Iterate for ressources
   for (int i = 0; i < ResKeyList.count(); i++) 
   {
      QImage      Thumbnail;
      qlonglong   Key = ResKeyList[i];
      applicationConfig->SlideThumbsTable->GetThumb(&Key,&Thumbnail);

      #ifndef STREAM_SAVING
      #ifdef BASE64SAVING
      QDomElement     Ressource = domDocument.createElement("RessourceBase64");
      #else
      QDomElement     Ressource = domDocument.createElement("Ressource");
      #endif
      #endif
      QByteArray      ba;
      QBuffer         buf(&ba);

      Thumbnail.save(&buf,"PNG");
      QByteArray Compressed = qCompress(ba,1);
      #ifdef BASE64SAVING
      QByteArray Hexed = Compressed.toBase64(); // much smaller than toHex !!
      #else
      QByteArray Hexed      = Compressed.toHex();
      #endif

      #ifndef STREAM_SAVING
      Ressource.setAttribute("Key",Key);
      Ressource.setAttribute("Width",Thumbnail.width());
      Ressource.setAttribute("Height",Thumbnail.height());
      Ressource.setAttribute("Image",QString(Hexed));
      Ressource.save(out,0);
      #endif

      #ifdef STREAM_SAVING
      #ifdef BASE64SAVING
      xmlWriter.writeStartElement("RessourceBase64");
      #else
      xmlWriter.writeStartElement("Ressource");
      #endif
      xmlWriter.writeAttribute("Key", Key);
      xmlWriter.writeAttribute("Width", Thumbnail.width());
      xmlWriter.writeAttribute("Height", Thumbnail.height());
      xmlWriter.writeAttribute("Image", QString(Hexed));
      xmlWriter.writeEndElement();
      #endif
   }

   #ifndef STREAM_SAVING
   file.close();
   #endif

   #ifdef STREAM_SAVING
   xmlWriter.writeEndDocument(); 
   fileStream.close();
   #endif
   return true;
}

//============================================================================================
// Function use directly or with thread to prepare an image number Column at given position
// Note : Position is relative to the start of the Column object !
//============================================================================================
void cDiaporama::PrepareMusicBloc(PrepareMusicBlocContext *Context) 
{
   if (Context->Column >= slides.count()) 
   {
      for (int j = 0; j < Context->MusicTrack->NbrPacketForFPS; j++) 
         Context->MusicTrack->AppendNullSoundPacket(Context->Position);
      return;
   }

   cMusicObject  *CurMusic = NULL;
   //...
   //int slideIndex = slides[Context->Column]->CachedMusicIndex;
   if( pAppConfig->seamlessMusic )
   {
      int slideIndex = slides[Context->Column]->CachedMusicIndex;
      cDiaporamaObject *MusicListSlide = NULL;
      if( slideIndex >= 0 )
         MusicListSlide = slides[slides[Context->Column]->CachedMusicIndex];
      int64_t mmMusicPosition = Context->Position + slides[Context->Column]->CachedMusicListOffset;
      mmMusicPosition = Context->slideStartpos + Context->PositionInSlide;
      int64_t startPosition = 0;
      int lastMusicIndex = 0;
      if( MusicListSlide != NULL )
         MusicListSlide->MusicList.findMusicObject(mmMusicPosition, &CurMusic, &startPosition, &lastMusicIndex);
      if (CurMusic == NULL) 
      {
         for (int j = 0; j < Context->MusicTrack->NbrPacketForFPS; j++) 
            Context->MusicTrack->AppendNullSoundPacket(Context->Position);
         //qDebug() << "PrepareMusicBloc leave after AppendNullSoundPacket";
         return;
      }
      //qDebug() << "getMusic from " << startPosition << " for position " << mmMusicPosition << " lastMusicIndex " << lastMusicIndex;
      startPosition += (/*Context->Position + */QTime(0,0,0,0).msecsTo(CurMusic->StartTime));
      if( CurMusic->SoundTrackBloc.SampleFormat == AV_SAMPLE_FMT_NONE) 
      {
         CurMusic->SoundTrackBloc.SetFPS(Context->MusicTrack->WantedDuration, Context->MusicTrack->Channels, Context->MusicTrack->SamplingRate, Context->MusicTrack->SampleFormat);
         Context->MusicTrack->dropTempData();
      }
      CurMusic->ImageAt(Context->PreviewMode, startPosition, &CurMusic->SoundTrackBloc, false, 1, true, false, Context->NbrDuration);
      while(CurMusic->SoundTrackBloc.ListCount() )
         Context->MusicTrack->AppendPacket(CurMusic->SoundTrackBloc.CurrentPosition, CurMusic->SoundTrackBloc.DetachFirstPacket());
      //qDebug() << "PrepareMusicBloc leave after AppendPacket";

      return;
   }
   //...
   int64_t StartPosition = 0;
   CurMusic = GetMusicObject(Context->Column, StartPosition); // Get current music file from column and position
   if (CurMusic == NULL) 
   {
      for (int j = 0; j < Context->MusicTrack->NbrPacketForFPS; j++) 
         Context->MusicTrack->AppendNullSoundPacket(Context->Position);
         //qDebug() << "PrepareMusicBloc leave after AppendNullSoundPacket(2)";
      return;
   }

   cDiaporamaObject *currentSlide = slides[Context->Column];
   if (Context->IsCurrent && Context->Column > 0) 
   {
      cDiaporamaObject *prevSlide = slides[Context->Column-1];
      if (prevSlide->MusicPause && !currentSlide->MusicPause)
         Context->FadIn = true;
      if (!prevSlide->MusicPause && currentSlide->MusicPause) 
         Context->FadOut=true;
   }

   bool IsCurrentTransitionIN = (Context->PositionInSlide < slides[(Context->IsCurrent ? Context->Column : Context->Column+1)]->TransitionDuration);
   bool FadeEffect = 
      (Context->IsCurrent 
       && IsCurrentTransitionIN 
       && (Context->Column > 0) 
       && ( 
         (slides[Context->Column-1]->MusicReduceVolume != currentSlide->MusicReduceVolume) 
         || ((slides[Context->Column-1]->MusicReduceVolume == currentSlide->MusicReduceVolume) && (slides[Context->Column-1]->MusicReduceFactor != currentSlide->MusicReduceFactor))
       )
      );

   if (!currentSlide->MusicPause ) 
   {
      double Factor = 1;
      if (FadeEffect) 
      {
         double  PctDone  = ComputePCT(SPEEDWAVE_SINQUARTER,double(Context->Position)/double(currentSlide->TransitionDuration));
         double  AncReduce = slides[Context->Column-1]->MusicPause ? 0 : slides[Context->Column-1]->MusicReduceVolume ? slides[Context->Column-1]->MusicReduceFactor : 1;
         double  NewReduce = currentSlide->MusicPause ? 0 : currentSlide->MusicReduceVolume ? currentSlide->MusicReduceFactor : 1;
         double  ReduceFactor = AncReduce+(NewReduce-AncReduce)*PctDone;
         Factor = Factor*ReduceFactor;
      } 
      else if (Context->FadIn && IsCurrentTransitionIN) 
      {
         double  PctDone = ComputePCT(SPEEDWAVE_SINQUARTER,double(Context->PositionInSlide)/double(currentSlide->TransitionDuration));
         Factor = Factor*PctDone;
      } 
      else if (Context->FadOut && IsCurrentTransitionIN) 
      {
         double PctDone = ComputePCT(SPEEDWAVE_SINQUARTER,double(Context->PositionInSlide)/double(slides[Context->IsCurrent ? Context->Column : Context->Column+1]->TransitionDuration));
         Factor = Factor*(1-PctDone);
      } 
      else 
      {
         if (currentSlide->MusicReduceVolume) 
            Factor = Factor*currentSlide->MusicReduceFactor;
         Factor = Factor*CurMusic->GetFading(StartPosition+Context->PositionInSlide,Context->FadIn,Context->IsCurrent ? (Context->Column < slides.count()-1 ? slides[Context->Column+1]->CachedPrevMusicFadOUT : false) : Context->FadOut);
      }

      // Get more music bloc at correct position (volume is always 100% @ this point !)
      int transitionTime = 0;
      if( !slides[Context->Column]->MusicType && (slides[Context->Column]->TransitionFamily!=0 || slides[Context->Column]->TransitionSubType!=0) ) 
         transitionTime = slides[Context->Column]->TransitionDuration;
      int64_t mPosition = Context->Position + StartPosition + QTime(0,0,0,0).msecsTo(CurMusic->StartTime) - transitionTime;
      //int64_t mPosition = Context->Position + StartPosition + QTime(0,0,0,0).msecsTo(CurMusic->StartTime);
      //qDebug() << "getMusic from " << mPosition;
      //if( mPosition < QTime(0,0,0,0).msecsTo(CurMusic->EndTime)/*-transitionTime*/)
      if(mPosition >= 0 && mPosition < QTime(0,0,0,0).msecsTo(CurMusic->EndTime)-transitionTime)
         CurMusic->ImageAt(Context->PreviewMode, mPosition, Context->MusicTrack, false, 1, true, false, Context->NbrDuration);
      else
      {
         for (int j = 0; j < Context->MusicTrack->NbrPacketForFPS; j++) 
            Context->MusicTrack->AppendNullSoundPacket(Context->Position);
         //return; no!! must apply volume to the rest of data in buffer if fading is used!!!
         //qDebug() << "done: AppendNullSoundPacket";
      }

      // Apply correct volume to block in queue
      double Volume = CurMusic->Volume;
      if (Volume == -1) 
      {
         if (CurMusic->GetSoundLevel() != -1)
            Volume = double(applicationConfig->DefaultSoundLevel)/double(CurMusic->GetSoundLevel()*100);
         else 
            Volume = 1;
      }
      if (Volume !=1 ) 
         Factor = Factor*Volume;
      if (Factor != 1)
      {
         if( Factor < 0.01 )
            Factor = 0.0;
         Context->MusicTrack->ApplyVolume(Factor);
         //qDebug() << "apply volume " << Factor;
      }
   }
}

void cDiaporama::resetSoundBlocks()
{
   for (int i = 0; i < slides.count(); i++) 
      slides[i]->MusicList.resetSoundBlocks();
}

//============================================================================================
// Function use directly or with thread to prepare an image number Column at given position
//  Extend=amout of padding (top and bottom) for cinema mode with DVD
//  IsCurrentObject : If true : prepare CurrentObject - If false : prepare Transition Object
//============================================================================================
void cDiaporama::CreateObjectContextList(cDiaporamaObjectInfo *Info, QSize size, bool IsCurrentObject, bool PreviewMode, bool AddStartPos, CompositionContextList &PreparedBrushList) 
{
   bool SoundOnly = size.isNull();  // W and H = 0 when producing sound track in render process

   deleteList(PreparedBrushList);

   cDiaporamaObjectInfo::ObjectInfo *info = IsCurrentObject ? &Info->Current : &Info->Transit;
   // return immediatly if we have image
   if ( !SoundOnly && !info->Object_PreparedImage.isNull() ) 
      return;

   int64_t         Duration            = info->Object_ShotDuration;
   cDiaporamaShot  *CurShot            = info->Object_CurrentShot;
   cSoundBlockList *SoundTrackMontage  = info->Object_SoundTrackMontage;
   int             ObjectNumber        = info->Object_Number;
   int             ShotNumber          = info->Object_ShotSequenceNumber;
   cDiaporamaShot  *PreviousShot = (ShotNumber > 0 ? slides.at(ObjectNumber)->shotList.at(ShotNumber - 1) : NULL);

   if (!SoundOnly && (CurShot)) 
   {
      // Construct collection
      int numObjs = CurShot->ShotComposition.compositionObjects.count();
      for (int j = 0; j < numObjs; j++)
         PreparedBrushList.append(new cCompositionObjectContext(j,PreviewMode,IsCurrentObject,Info,size,CurShot,PreviousShot,SoundTrackMontage,AddStartPos,Duration));
   }
}

#define MULTICACHING
typedef struct _range
{
   int start;
   int end;
   bool isStatic;
   _range(int a, int b, bool c) { start = a; end = b; isStatic = c; }
} range;
QMutex moglMutex;

void cDiaporama::PrepareImage(cDiaporamaObjectInfo *Info,QSize size, bool IsCurrentObject, bool AddStartPos, CompositionContextList &PreparedBrushList) 
{
//   static int puzzleCount = 0;
   //autoTimer pi("PrepareImage");
   bool SoundOnly = size.isNull();  // W and H = 0 when producing sound track in render process

   // return immediatly if we have image
   cDiaporamaObjectInfo::ObjectInfo *objInfo = IsCurrentObject ? &Info->Current : &Info->Transit;
   if ( !SoundOnly && !objInfo->Object_PreparedImage.isNull()) 
      return;

   //LONGLONG cp = curPCounter();

   int64_t          Duration        = objInfo->Object_ShotDuration;
   cDiaporamaShot   *CurShot        = objInfo->Object_CurrentShot;
   cDiaporamaObject *CurObject      = objInfo->Object;
   int              CurTimePosition = objInfo->Object_InObjectTime;
   cSoundBlockList  *SoundTrackMontage = objInfo->Object_SoundTrackMontage;

   if (SoundOnly) 
   {
      // if sound only then parse all shot objects to create SoundTrackMontage
      for(auto obj : CurShot->ShotComposition.compositionObjects)
      {
         if( obj->isVideo() && obj->BackgroundBrush->SoundVolume != 0 )
         {
            // Calc StartPosToAdd depending on AddStartPos
            int64_t StartPosToAdd = (AddStartPos ? QTime(0,0,0,0).msecsTo(((cVideoFile *)obj->BackgroundBrush->MediaObject)->StartTime) : 0);

            // Calc VideoPosition depending on video set to pause (visible=off) in previous shot
            int64_t VideoPosition = 0;
            int64_t ThePosition = 0;
            int TheShot = 0;
            while ((TheShot < CurObject->shotList.count()) && (ThePosition + CurObject->shotList[TheShot]->StaticDuration < CurTimePosition))
            {
               cCompositionObject *object = CurObject->shotList[TheShot]->ShotComposition.getCompositionObject(obj->index());
               if (object->isVisible())
                  VideoPosition += CurObject->shotList[TheShot]->StaticDuration;
               //for (int w = 0; w < CurObject->shotList[TheShot]->ShotComposition.compositionObjects.count(); w++)
               //   if (CurObject->shotList[TheShot]->ShotComposition.compositionObjects[w]->index() == obj->index())
               //   {
               //      if (CurObject->shotList[TheShot]->ShotComposition.compositionObjects[w]->isVisible())
               //         VideoPosition += CurObject->shotList[TheShot]->StaticDuration;
               //      break;
               //   }
               ThePosition += CurObject->shotList[TheShot]->StaticDuration;
               TheShot++;
            }
            VideoPosition += (CurTimePosition-ThePosition);

            obj->DrawCompositionObject(CurObject,NULL,double(size.height())/double(1080),QSizeF(0,0),true,VideoPosition+StartPosToAdd,SoundTrackMontage,1,1,NULL,false,CurShot->StaticDuration,true);
         }
      }
      return;
   } 
   QImage Image(size,QImage::Format_ARGB32_Premultiplied);
   Image.fill(0);
   if (Image.isNull()) 
      return;

   QPainter P;
   P.begin(&Image);
   // Prepare a transparent image
   //P.begin(&Image);
   P.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
   //P.setCompositionMode(QPainter::CompositionMode_Source);
   //P.fillRect(QRect(QPoint(0,0),size),Qt::transparent);
   P.setCompositionMode(QPainter::CompositionMode_SourceOver);

   //qDebug() << "PrepareImage stage 1 " << PC2time(curPCounter()-cp,true);
   //cp = curPCounter();

   if (CurShot) 
   {
      // Compute each item of the collection
      int lastStatic = -1;
      QList<range> ranges;
      for (int aa = 0; aa < PreparedBrushList.count(); aa++)
      {
         PreparedBrushList[aa]->Compute();
         cCompositionObject *Object = PreparedBrushList[aa]->CurShot->ShotComposition.compositionObjects[PreparedBrushList[aa]->ObjectNumber];
         if (Object->IsStatic && lastStatic == aa - 1)
            lastStatic = aa;
         if( ranges.isEmpty() || !Object->IsStatic || !ranges.last().isStatic)
            ranges.append(range(aa,aa,Object->IsStatic));
         else
            ranges.last().end++;
      }
      if (PreparedBrushList.count() < 2)
         lastStatic = -1;

      #ifdef MULTICACHING
      QSizeF sizeF = size;//QSizeF(W,H);
      double adjRatio = sizeF.height()/1080.0;
      foreach(range r, ranges)
      {
         //qDebug() << "range " << r.start << " " << r.end << " " << r.isStatic;
         if( r.end > r.start )
         {
            QImage staticPart;
            //if (lastStatic >= 0)
            {
               if (PreparedBrushList[0]->PreviewMode)
                  staticPart = applicationConfig->ImagesCache.getPreviewImage(CurShot, r.end);
               else
                  staticPart = applicationConfig->ImagesCache.getRenderImage(CurShot, r.end);
            }
            if (staticPart.isNull() || staticPart.size() != size)
            {
               // draw multiple items into one cached image
               //qDebug() << "draw objects " <<  r.start << " bis " << r.end << " to obj " << (void *)PreparedBrushList[r.end]->CurShot->ShotComposition.compositionObjects[PreparedBrushList[r.end]->ObjectNumber];
               //qDebug() << "prepare static part " ;
               staticPart = QImage(size, QImage::Format_ARGB32_Premultiplied);
               staticPart.fill(0);

               // Prepare a transparent image
               QPainter PS;
               PS.begin(&staticPart);
               PS.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
               //PS.setCompositionMode(QPainter::CompositionMode_SourceOver);
               //draw the static parts of the composition
               //foreach(cCompositionObject* compositionObject, CurShot->ShotComposition.compositionObjects)
               for(int j = r.start; j <= r.end; j++)
               {
                  cCompositionObject* compositionObject = CurShot->ShotComposition.compositionObjects[j];
                  cCompositionObjectContext *context = PreparedBrushList[j];
                  if (compositionObject->isVisible())
                     compositionObject->DrawCompositionObject(CurObject, &PS, adjRatio, sizeF,
                        context->PreviewMode, context->VideoPosition + context->StartPosToAdd,
                        context->SoundTrackMontage,
                        context->BlockPctDone, context->ImagePctDone,
                        context->PrevCompoObject, Duration,
                        true, false, QRectF(), false, context);
               }
               PS.end(); 
               if (PreparedBrushList[0]->PreviewMode)
                  applicationConfig->ImagesCache.addPreviewImage(CurShot, r.end, staticPart);
               else
                  applicationConfig->ImagesCache.addRenderImage(CurShot, r.end, staticPart);
            }
            //else
            //{
            //   qDebug() << "reuse static bitmap for range " << r.start << " bis " << r.end;
            //}
            P.drawImage(0, 0, staticPart);
         }
         else
         {
            // draw a single item
            cCompositionObject* compositionObject = CurShot->ShotComposition.compositionObjects[r.start];
            cCompositionObjectContext *context = PreparedBrushList[r.start];
            if( compositionObject->isVisible() /*&& j > lastStatic*/ )
            {
               compositionObject->DrawCompositionObject(CurObject, &P, adjRatio, sizeF,
                  context->PreviewMode, context->VideoPosition + context->StartPosToAdd,
                  context->SoundTrackMontage,
                  context->BlockPctDone, context->ImagePctDone,
                  context->PrevCompoObject, Duration,
                  true, false, QRectF(), false, context);
            }
         }
      }

      #else

      // Draw collection
      QSizeF sizeF = size;//QSizeF(W,H);
      double adjRatio = sizeF.height()/1080.0;
      int j = 0;
      QImage staticPart;
      if (lastStatic >= 0)
      {
         if (PreparedBrushList[0]->PreviewMode)
            staticPart = applicationConfig->ImagesCache.getPreviewImage(CurShot);
         else
            staticPart = applicationConfig->ImagesCache.getRenderImage(CurShot);
      }
      if (lastStatic >= 0 && (staticPart.isNull() || staticPart.size() != size))
      {
         //qDebug() << "prepare static part " ;
         staticPart = QImage(size, QImage::Format_ARGB32_Premultiplied);
         staticPart.fill(0);

         // Prepare a transparent image
         QPainter PS;
         PS.begin(&staticPart);
         PS.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing | QPainter::NonCosmeticDefaultPen);
         //PS.setCompositionMode(QPainter::CompositionMode_Source);
         //PS.fillRect(QRect(QPoint(0, 0),size), Qt::transparent);
         PS.setCompositionMode(QPainter::CompositionMode_SourceOver);
         //draw the static parts of the composition
         foreach(cCompositionObject* compositionObject, CurShot->ShotComposition.compositionObjects)
         {
            cCompositionObjectContext *context = PreparedBrushList[j];
            if (compositionObject->isVisible())
               compositionObject->DrawCompositionObject(CurObject, &PS, adjRatio, sizeF,
               context->PreviewMode, context->VideoPosition + context->StartPosToAdd,
               context->SoundTrackMontage,
               context->BlockPctDone, context->ImagePctDone,
               context->PrevCompoObject, Duration,
               true, false, QRectF(), false, context);
            j++;
            if (j > lastStatic)
               break;
         }
         PS.end(); 
         if (PreparedBrushList[0]->PreviewMode)
            applicationConfig->ImagesCache.addPreviewImage(CurShot, staticPart);
         else
            applicationConfig->ImagesCache.addRenderImage(CurShot, staticPart);
      }
      if (lastStatic >= 0)
      {
         qDebug() << "use static part " ;
         P.drawImage(0, 0, staticPart);
      }

      j = 0;
      foreach(cCompositionObject* compositionObject, CurShot->ShotComposition.compositionObjects)
      {
         cCompositionObjectContext *context = PreparedBrushList[j];
         if( compositionObject->isVisible() && j > lastStatic )
         {
            compositionObject->DrawCompositionObject(CurObject, &P, adjRatio, sizeF,
               context->PreviewMode, context->VideoPosition + context->StartPosToAdd,
               context->SoundTrackMontage,
               context->BlockPctDone, context->ImagePctDone,
               context->PrevCompoObject, Duration,
               true, false, QRectF(), false, context);
         }
         j++;
      }

      #endif
   }
   P.end();

   if (IsCurrentObject) 
      Info->Current.Object_PreparedImage = Image; 
   else 
      Info->Transit.Object_PreparedImage = Image;

   //Image.save( QString("f:/Temp/puzzleAll_%1.jpg").arg(puzzleCount++,4,10,QChar('0')));
}

//=============================================================================================================================
// Function use directly or with thread to make assembly of background and images and make mix (sound & music) when transition
//=============================================================================================================================
extern int64_t conversionTime;
void cDiaporama::DoAssembly(double PCT,cDiaporamaObjectInfo *Info, QSize size, QImage::Format QTFMT) 
{
   //autoTimer ass("assembly");
   if ( !Info->RenderedImage.isNull() || size.isNull()) 
      return;    // return immediatly if we already have image or if sound only

   //QImage   Image(W,H,QTFMT);
   QImage   Image(size, QImage::Format_ARGB32_Premultiplied);
   QPainter Painter;

   Painter.begin(&Image);
   Painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing|QPainter::SmoothPixmapTransform);

   // Draw background
   if (Info->IsTransition && ( Info->Current.Object_Number == 0 || Info->Current.Object_BackgroundIndex != Info->Transit.Object_BackgroundIndex)) 
   {
      double Opacity;
      if (Info->Transit.Object && !Info->Transit.Object_PreparedBackground.isNull()) 
         Painter.drawImage(0,0,Info->Transit.Object_PreparedBackground);
      if (!Info->Current.Object_PreparedBackground.isNull()) 
      {
         Opacity = ComputePCT(Info->Current.Object->GetSpeedWave(),Info->TransitionPCTDone);
         if (Info->Transit.Object) 
            Painter.setOpacity(Opacity);
         Painter.drawImage(0,0,Info->Current.Object_PreparedBackground);
      }
      Painter.setOpacity(1);
   } 
   else 
   {
      if (!Info->Current.Object_PreparedBackground.isNull()) 
         Painter.drawImage(0,0,Info->Current.Object_PreparedBackground);
      else 
         Painter.fillRect(QRect(QPoint(0,0),size),Qt::black);
   }

   // Add prepared images and transition
   //autoTimer *ass2 = new autoTimer("ass inner");

   if (Info->IsTransition && !Info->Current.Object_PreparedImage.isNull()) 
   {
      if (Info->Transit.Object_PreparedImage.isNull()) 
      {
         Info->Transit.Object_PreparedImage = QImage(Info->Current.Object_PreparedImage.size(),QImage::Format_ARGB32_Premultiplied);
         Info->Transit.Object_PreparedImage.fill(0);
      }
      DoTransition(Info->TransitionFamily,Info->TransitionSubType,PCT,&Info->Transit.Object_PreparedImage,&Info->Current.Object_PreparedImage,&Painter,size.width(),size.height());
   } 
   else 
      if (!Info->Current.Object_PreparedImage.isNull()) 
         Painter.drawImage(0,0,Info->Current.Object_PreparedImage);
   //delete ass2;
   if(Painter.isActive() )
      Painter.end();
   if( QTFMT != QImage::Format_ARGB32_Premultiplied && !Image.isNull() )
   {
      //AUTOTIMER(ass,"assembly: convert");
      //QElapsedTimer t;
      //t.start();
      Info->RenderedImage = Image;
      QMyImage(Info->RenderedImage).convert_ARGB_PM_to_ARGB_inplace();
      //Info->RenderedImage = Image.convertToFormat(QTFMT);
      //conversionTime += t.nsecsElapsed();

   }
   else   
      Info->RenderedImage = Image;
}

//============================================================================================
// Function use directly or with thread to load source image
// Assume SourceImage is null
// Produce sound only if W and H=0
//============================================================================================
#define noLOGLOAD
void cDiaporama::LoadSources(cDiaporamaObjectInfo *Info, QSize size, bool PreviewMode, bool AddStartPos, CompositionContextList &PreparedTransitBrushList, CompositionContextList &PreparedBrushList, int NbrDuration) 
{
#ifdef LOGLOAD
   LONGLONG cp = curPCounter();
#endif
    if (!Info->Current.Object) 
      return;
    if(!PreviewMode && !Info->IsTransition && Info->Current.Object_CurrentShot->fullScreenVideoIndex >= 0 && pAppConfig->enableYUVPassthrough)
    {
       LoadSourcesYUV(Info, size, PreviewMode, AddStartPos, PreparedTransitBrushList, PreparedBrushList, NbrDuration);
       return;
   }
    //QFutureWatcher<void> ThreadPrepareCurrentMusicBloc;
    QFutureWatcher<void> ThreadPrepareTransitMusicBloc;

    // W and H = 0 when producing sound track in render process
    bool SoundOnly = size.isNull();

    //==============> Music track part

    //if (Info->Current.Object )
    //  qDebug() << "Info->Current Object_Number " << Info->Current.Object_Number << " Object_InObjectTime" << Info->Current.Object_InObjectTime; 
    //if( Info->IsTransition && Info->Transit.Object )
    //    qDebug() << "Info->Transit Object_Number " << Info->Transit.Object_Number << " Object_InObjectTime" << Info->Transit.Object_InObjectTime; 
    if (Info->Current.Object && Info->Current.Object_MusicTrack) 
    {
        PrepareMusicBlocContext Context;
        Context.PreviewMode = PreviewMode;
        Context.Column      = Info->Current.Object_Number;
        Context.Position    = Info->Current.Object_InObjectTime;
        Context.MusicTrack  = Info->Current.Object_MusicTrack;
        Context.NbrDuration = NbrDuration;
        Context.FadIn       = Info->Current.Object->CachedMusicFadIN;
        Context.FadOut      = false;
        Context.IsCurrent   = true;
        Context.PositionInSlide = Info->Current.Object_InObjectTime;
        Context.slideStartpos = Info->Current.Object_StartTime;
        //ThreadPrepareCurrentMusicBloc.setFuture(QtConcurrent::run(this,&cDiaporama::PrepareMusicBloc,&Context));
        PrepareMusicBloc(&Context);
        //Context.MusicTrack->dump();
    }
#ifdef LOGLOAD
    qDebug() << "loadSources stage 1 " << PC2time(curPCounter()-cp,true);
    cp = curPCounter();
#endif
    if (!applicationConfig->seamlessMusic)
    {
       if (Info->IsTransition && Info->Transit.Object && Info->Transit.Object_MusicTrack)
       {
          PrepareMusicBlocContext Context;
          Context.PreviewMode = PreviewMode;
          Context.Column = Info->Transit.Object_Number;
          Context.Position = Info->Transit.Object_InObjectTime;
          Context.MusicTrack = Info->Transit.Object_MusicTrack;
          Context.NbrDuration = NbrDuration;
          Context.FadIn = false;
          Context.FadOut = Info->Current.Object->CachedPrevMusicFadOUT;
          Context.IsCurrent = false;
          Context.PositionInSlide = Info->Current.Object_InObjectTime;
          ThreadPrepareTransitMusicBloc.setFuture(QT_CONCURRENT_RUN_1(this, &cDiaporama::PrepareMusicBloc, &Context));
       }
    }
#ifdef LOGLOAD
    qDebug() << "loadSources stage 2 " << PC2time(curPCounter()-cp,true);
    cp = curPCounter();
#endif


    //==============> Image part

    // Transition Object if a previous was not kept !
    if (Info->IsTransition) 
    {
        if (Info->Transit.Object) 
        {
            PrepareImage(Info, size, false, AddStartPos, PreparedTransitBrushList);
        } 
        else if (Info->Transit.Object_PreparedImage.isNull()) 
        {
            Info->Transit.Object_PreparedImage = QImage(size,QImage::Format_ARGB32_Premultiplied);
            QPainter P;
            P.begin(&Info->Transit.Object_PreparedImage);
            P.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing|QPainter::SmoothPixmapTransform);
            P.fillRect(QRect(QPoint(0,0),size),Qt::black);//Qt::transparent);
            P.end();
        }
    }
#ifdef LOGLOAD
    qDebug() << "loadSources stage 3 " << PC2time(curPCounter()-cp,true);
    cp = curPCounter();
#endif

    // Load Source image
    PrepareImage(Info, size, true, AddStartPos, PreparedBrushList);

#ifdef LOGLOAD
    qDebug() << "loadSources stage 4 " << PC2time(curPCounter()-cp,true);
    cp = curPCounter();
#endif

    //==============> Background part

    if (!SoundOnly) 
    {
        // Search background context for CurrentObject if a previous was not keep !
        if (Info->Current.Object_BackgroundBrush == NULL) 
          getBackgroundBrush(Info->Current, size, PreviewMode);
#ifdef LOGLOAD
    qDebug() << "loadSources stage 5 " << PC2time(curPCounter()-cp,true);
    cp = curPCounter();
#endif

        // same job for Transition Object if a previous was not keep !
        if (Info->Transit.Object && Info->Transit.Object_BackgroundBrush == NULL) 
          getBackgroundBrush(Info->Transit, size, PreviewMode);
#ifdef LOGLOAD
       qDebug() << "loadSources stage 6 " << PC2time(curPCounter()-cp,true);
       cp = curPCounter();
#endif
    }

    //==============> Mixing of music and soundtrack
    //QTime b;
    //b.start();

    //if (Info->Current.Object && Info->Current.Object_MusicTrack && ThreadPrepareCurrentMusicBloc.isRunning()) 
    //  ThreadPrepareCurrentMusicBloc.waitForFinished();
    if (Info->Transit.Object && Info->Transit.Object_MusicTrack && ThreadPrepareTransitMusicBloc.isRunning()) 
      ThreadPrepareTransitMusicBloc.waitForFinished();

#ifdef LOGLOAD
    qDebug() << "loadSources stage 7 " << PC2time(curPCounter()-cp,true);
    cp = curPCounter();
#endif

    // Soundtrack mix with fade in/fade out
    if (/*!applicationConfig->seamlessMusic &&*/ Info->IsTransition && (Info->Current.Object_SoundTrackMontage || Info->Transit.Object_SoundTrackMontage))
    {
        if (Info->Current.Object_SoundTrackMontage == NULL) 
        {
            // if we don't have a Current.Object_SoundTrackMontage, we need to create it because it's the destination !
            Info->Current.Object_SoundTrackMontage = new cSoundBlockList();
            Info->Current.Object_SoundTrackMontage->SetFPS(Info->FrameDuration,2,Info->Transit.Object_SoundTrackMontage->SamplingRate,AV_SAMPLE_FMT_S16);
        }
        // Ensure this track has enough data
        while (Info->Current.Object_SoundTrackMontage->ListCount()<Info->Current.Object_SoundTrackMontage->NbrPacketForFPS)
            Info->Current.Object_SoundTrackMontage->AppendNullSoundPacket(Info->Current.Object_StartTime+Info->Current.Object_InObjectTime);

        // Mix current and transit SoundTrackMontage (if needed)
        // @ the end: only current SoundTrackMontage exist !
        Info->Current.Object_SoundTrackMontage->Mutex.lock();
        for (int i = 0; i < Info->Current.Object_SoundTrackMontage->NbrPacketForFPS; i++) 
        {
            // Mix the 2 sources buffer using the first buffer as destination and remove one paquet from the Transit.Object_SoundTrackMontage
            int16_t *Paquet = Info->Transit.Object_SoundTrackMontage ? Info->Transit.Object_SoundTrackMontage->DetachFirstPacket(true) : NULL;

            //int32_t mix;
            int16_t *Buf1 = i < Info->Current.Object_SoundTrackMontage->ListCount() ? Info->Current.Object_SoundTrackMontage->GetAt(i) : NULL;
            int     Max = Info->Current.Object_SoundTrackMontage->SoundPacketSize / (Info->Current.Object_SoundTrackMontage->SampleBytes*Info->Current.Object_SoundTrackMontage->Channels);

            double  FadeAdjust  = sin(1.5708*double(Info->Current.Object_InObjectTime+(double(i)/double(Info->Current.Object_SoundTrackMontage->NbrPacketForFPS))*double(Info->FrameDuration))/double(Info->TransitionDuration));
            double  FadeAdjust2 = 1-FadeAdjust;

            int16_t *Buf2 = (Paquet != NULL) ? Paquet : NULL;
            if (Buf1 != NULL && Buf2 == NULL) 
            {
                // Apply Fade in to Buf1
                for (int j = 0; j < Max; j++) 
                {
                    // Left channel : Adjust if necessary (16 bits)
                    *Buf1 = (int16_t) qBound(-32768.0, double(*Buf1)*FadeAdjust, 32767.0); Buf1++;
                    // Right channel : Adjust if necessary (16 bits)
                    *Buf1 = (int16_t) qBound(-32768.0, double(*Buf1)*FadeAdjust, 32767.0); Buf1++;
                }
            } 
            else if (Buf1 != NULL && Buf2 != NULL) 
            {
                // do mixing
                for (int j = 0; j < Max; j++) 
                {
                    *Buf1 = (int16_t) qBound(-32768.0, double(*Buf1)*FadeAdjust+double(*Buf2++)*FadeAdjust2, 32767.0); Buf1++;
                    *Buf1 = (int16_t) qBound(-32768.0, double(*Buf1)*FadeAdjust+double(*Buf2++)*FadeAdjust2, 32767.0); Buf1++;
                }
                av_free(Paquet);
            } 
            else if (Buf1 == NULL && Buf2 != NULL) 
            {
                // swap buf1 and buf2
                Info->Current.Object_SoundTrackMontage->SetAt(i,Buf2);
                // Apply Fade out to Buf2
                for (int j = 0; j < Max; j++)  
                {
                    // Left channel : Adjust if necessary (16 bits)
                    *Buf2 = (int16_t) qBound(-32768.0, double(*Buf2)*FadeAdjust2, 32767.0); Buf2++;
                    // Right channel : Adjust if necessary (16 bits)
                    *Buf2 = (int16_t) qBound(-32768.0, double(*Buf2)*FadeAdjust2, 32767.0); Buf2++;
                }
            }
        }
        Info->Current.Object_SoundTrackMontage->Mutex.unlock();
    }
#ifdef LOGLOAD
    qDebug() << "loadSources stage 8 " << PC2time(curPCounter()-cp,true);
    cp = curPCounter();
#endif

    // Mix current and transit music
    // @ the end: only current music exist !
    // Mix the 2 sources buffer using the first buffer as destination and remove one paquet from the PreviousMusicTrack
    if (/*!applicationConfig->seamlessMusic && */Info->IsTransition && Info->Current.Object_MusicTrack)
    {
       Info->Current.Object_MusicTrack->Mutex.lock();
       int Max = Info->Current.Object_MusicTrack->NbrPacketForFPS;
       if (Max > Info->Current.Object_MusicTrack->ListCount())
          Max = Info->Current.Object_MusicTrack->ListCount();
       for (int i = 0; i < Max; i++)
       {
          if (i >= Info->Current.Object_MusicTrack->ListCount())
             Info->Current.Object_MusicTrack->AppendNullSoundPacket(0, true);

          int16_t *Buf1 = Info->Current.Object_MusicTrack->GetAt(i);
          int16_t *Buf2 = Info->Transit.Object_MusicTrack ? Info->Transit.Object_MusicTrack->DetachFirstPacket(true) : NULL;
          int Max = Info->Current.Object_MusicTrack->SoundPacketSize / (Info->Current.Object_MusicTrack->SampleBytes*Info->Current.Object_MusicTrack->Channels);

          if (Buf2)
          {
             if (!Buf1)
             {
                Info->Current.Object_MusicTrack->SetAt(i, Buf2);
             }
             else
             {
                int16_t *B1 = Buf1, *B2 = Buf2;

                for (int j = 0; j < Max; j++)
                {
                   // Left channel : Adjust if necessary (16 bits)
                   *B1 = (int16_t)qBound(-32768, int32_t(*B1 + *B2++), 32767); B1++;
                   // Right channel : Adjust if necessary (16 bits)
                   *B1 = (int16_t)qBound(-32768, int32_t(*B1 + *B2++), 32767); B1++;
                }
                av_free(Buf2);
                Info->Current.Object_MusicTrack->SetAt(i, Buf1);
             }
          }
       }
       Info->Current.Object_MusicTrack->Mutex.unlock();
    }
#ifdef LOGLOAD
    qDebug() << "loadSources stage 9 " << PC2time(curPCounter()-cp,true);
    cp = curPCounter();
#endif
}

void cDiaporama::getBackgroundBrush(cDiaporamaObjectInfo::ObjectInfo &objInfo, QSize size, bool PreviewMode)
{
   // Search background context for CurrentObject if a previous was not keep !
   if (objInfo.Object_BackgroundBrush == NULL)
   {
      if ((objInfo.Object_BackgroundIndex >= slides.count()) || (slides[objInfo.Object_BackgroundIndex]->BackgroundType == false))
         objInfo.Object_BackgroundBrush = new QBrush(Qt::black);   // If no background definition @ first object
      else
         objInfo.Object_BackgroundBrush = slides[objInfo.Object_BackgroundIndex]->BackgroundBrush->GetBrush(size, PreviewMode, 0, NULL, 1, NULL);
      objInfo.Object_PreparedBackground = QImage(size, QImage::Format_ARGB32_Premultiplied);
      QPainter P;
      P.begin(&objInfo.Object_PreparedBackground);
      if (objInfo.Object_BackgroundBrush)
         P.fillRect(QRect(QPoint(0, 0), size), *objInfo.Object_BackgroundBrush);
      else
         P.fillRect(QRect(QPoint(0, 0), size), QBrush(Qt::black));
      P.end();
   }

}
//============================================================================================
void cDiaporama::CloseUnusedLibAv(int CurrentSlide, bool backwardOnly) 
{
   // Parse all unused slides to close unused libav buffer, codec, ...
   for (int i = 0; i < slides.count(); i++) 
   {
      if (i < CurrentSlide-1 || ( i > CurrentSlide+1 && !backwardOnly) ) 
         for(auto obj : slides[i]->ObjectComposition.compositionObjects)
            if(obj->isVideo() )
               ((cVideoFile *)obj->BackgroundBrush->MediaObject)->CloseCodecAndFile();
   }
}

void cDiaporama::clearResolvedTexts()
{
   //for (int i = 0; i < slides.count(); i++)
   for(auto slide : slides)
   {
      for(auto obj : slide->ObjectComposition.compositionObjects)
         obj->resolvedText.clear();
      // walk over the shots
      for(auto shot : slide->shotList)
      {
         for(auto obj : shot->ShotComposition.compositionObjects)
            obj->resolvedText.clear();
      }
   }
}

void cDiaporama::clearThumbnailsWithVars()
{
   for (auto const obj : slides)
      if ((obj->ThumbnailKey != 1) && (Variable.IsObjectHaveVariables(obj)))
         applicationConfig->SlideThumbsTable->ClearThumb(obj->ThumbnailKey);
}


//============================================================================================

void cDiaporama::UpdateGMapsObject(bool ProposeAll) 
{
    cLocation *PrjLocation = projectInfo->Location();
    cLocation *ChpLocation = PrjLocation;
    for (int i=0;i<slides.count();i++) {
        if (slides[i]->StartNewChapter) {
            if (slides[i]->ChapterLocation) ChpLocation=(cLocation *)slides[i]->ChapterLocation;
                else ChpLocation=PrjLocation;
        }
        for (int j = 0; j < slides[i]->ObjectComposition.compositionObjects.count(); j++) 
         if ((slides[i]->ObjectComposition.compositionObjects[j]->BackgroundBrush->MediaObject)&&(slides[i]->ObjectComposition.compositionObjects[j]->BackgroundBrush->MediaObject->is_Gmapsmap())) {
            cGMapsMap *CurrentMap=(cGMapsMap *)slides[i]->ObjectComposition.compositionObjects[j]->BackgroundBrush->MediaObject;
            bool      Propose=false;
            bool      FullRefresh=false;
            for (int loc=0;loc<CurrentMap->locations.count();loc++) {
                cLocation *Location=(cLocation *)CurrentMap->locations[loc];
                if (Location->LocationType==cLocation::PROJECT) {
                    if ((ProposeAll)&&(!CurrentMap->IsMapValide)) {
                        Propose=true;
                    } else if ((PrjLocation)&&((Location->Name!=PrjLocation->Name)||(Location->FriendlyAddress!=PrjLocation->FriendlyAddress)||(Location->GPS_cx!=PrjLocation->GPS_cx)||(Location->GPS_cy!=PrjLocation->GPS_cy))) {
                        if ((PrjLocation)&&((Location->GPS_cx!=PrjLocation->GPS_cx)||(Location->GPS_cy!=PrjLocation->GPS_cy))) {
                            FullRefresh=true;
                            CurrentMap->IsValide=false;
                        }
                        Propose=true;
                    }
                } else if (Location->LocationType==cLocation::CHAPTER) {
                    if ((ProposeAll)&&(!CurrentMap->IsMapValide)) {
                        Propose=true;
                    } else if ((ChpLocation)&&((Location->Name!=ChpLocation->Name)||(Location->FriendlyAddress!=ChpLocation->FriendlyAddress)||(Location->GPS_cx!=ChpLocation->GPS_cx)||(Location->GPS_cy!=ChpLocation->GPS_cy))) {
                        if ((PrjLocation)&&((Location->GPS_cx!=ChpLocation->GPS_cx)||(Location->GPS_cy!=ChpLocation->GPS_cy))) {
                            FullRefresh=true;
                            CurrentMap->IsValide=false;
                        }
                        Propose=true;
                    }
                }
                if (Propose) {
                    cLocation *RealLoc=NULL;
                    slides[i]->ObjectComposition.compositionObjects[j]->BackgroundBrush->GetRealLocation((void **)&Location,(void **)&RealLoc);
                    Propose=(Location!=NULL)&&(RealLoc!=NULL);
                }
            }
            if (Propose) {
                qlonglong PrevRessourceKey = CurrentMap->RessourceKey();
                if ((FullRefresh)&&(CustomMessageBox(applicationConfig->TopLevelWindow,QMessageBox::Question,APPLICATION_NAME,
                             QApplication::translate("DlgGMapsLocation","A map on slide %1 must be regenerated.\nDo you want to do it now?").arg(i+1),
                             QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes)==QMessageBox::Yes)) {
                    DlgGMapsGeneration Dlg(slides[i]->ObjectComposition.compositionObjects[j]->BackgroundBrush,CurrentMap,false,applicationConfig,applicationConfig->TopLevelWindow);
                    Dlg.InitDialog();
                    Dlg.exec();
                }
                // Reset cache of map object
                applicationConfig->ImagesCache.RemoveImageObject(PrevRessourceKey,-1);
                // Reset thumbnail of slide
                applicationConfig->SlideThumbsTable->ClearThumb(slides[i]->ThumbnailKey);
                IsModify=true;
            }
        }
    }
}

void cDiaporama::PreLoadItems(int iStartSlide, int iEndSlide, QSemaphore *numSlides, volatile bool *stopPreloading)
{
   while( iStartSlide <= iEndSlide && *stopPreloading )
   {
      if( numSlides->tryAcquire(1,50) )
      {
         if( pAppConfig->ImagesCache.PercentageMemoryUsed() < 70 )
         {
            slides.at(iStartSlide)->preLoadItems(false);
            iStartSlide++;
         }
         else
         {
            numSlides->release(1);
            QThread::msleep(200);
         }
      }
   }
}

void cDiaporama::PreLoadRenderItems(int iStartSlide, int iEndSlide, volatile bool *stopPreloading)
{
   QThread::currentThread()->setPriority( QThread::LowPriority);
   //qDebug() << "start PreLoadRenderItems " << iStartSlide << " " << iEndSlide << " " << *stopPreloading << " " << pAppConfig->ImagesCache.PercentageMemoryUsed();
   while( iStartSlide < iEndSlide && *stopPreloading && pAppConfig->ImagesCache.PercentageMemoryUsed() < 70 )
   {
      
      if( iStartSlide >= 0 )
      {
         //qDebug() << "preLoadRenderItems for slide " << iStartSlide;
         slides.at(iStartSlide)->preLoadItems(false);
         //qDebug() << "preLoadRenderItems for slide " << iStartSlide << " done";
         emit slideCached(iStartSlide+1);
      }
      iStartSlide++;
   }
   //qDebug() << "done PreLoadRenderItems " << iStartSlide << " " << " " << *stopPreloading << " " << pAppConfig->ImagesCache.PercentageMemoryUsed() << "% memory usage";
   emit cachingEnd();
}

void cDiaporama::PreLoadPreviewItems(int iStartSlide, int iEndSlide, volatile bool *stopPreloading)
{
   QThread::currentThread()->setPriority( QThread::LowPriority);
   while( iStartSlide < iEndSlide && !*stopPreloading && pAppConfig->ImagesCache.PercentageMemoryUsed() < 70 )
   {
      //qDebug() << "preLoadPreviewItems for slide " << iStartSlide;
      slides.at(iStartSlide)->preLoadItems(true);
      //qDebug() << "preLoadPreviewItems for slide " << iStartSlide << " done";
      emit slideCached(iStartSlide+1);
      iStartSlide++;
   }
   emit cachingEnd();
}

//*********************************************************************************************************************************************
// Class object for rendering
//*********************************************************************************************************************************************

cDiaporamaObjectInfo::cDiaporamaObjectInfo() 
{
   init();
}

cDiaporamaObjectInfo::cDiaporamaObjectInfo(cDiaporamaObjectInfo *PreviousFrame, int64_t TimePosition, cDiaporama *Diaporama, double TheFrameDuration, bool WantSound) 
{
   init();
   FrameDuration = TheFrameDuration;

   if (!Diaporama) 
   {
      Current.Object_InObjectTime = TimePosition;
      Transit.Object_InObjectTime = TimePosition;
      return;
   } 

   //==============> Retrieve object information depending on position (in msec)
   // Search which object for given time position
   int AdjustedDuration = (Current.Object_Number < Diaporama->slides.count()) ? Diaporama->slides[Current.Object_Number]->CachedDuration - Diaporama->GetTransitionDuration(Current.Object_Number+1) : 0;
   if (AdjustedDuration < 33) 
      AdjustedDuration = 33; // Not less than 1/30 sec
   while (Current.Object_Number < Diaporama->slides.count() 
          && (Current.Object_StartTime + AdjustedDuration) <= TimePosition) 
   {
      Current.Object_StartTime = Current.Object_StartTime + AdjustedDuration;
      Current.Object_Number++;
      AdjustedDuration = (Current.Object_Number < Diaporama->slides.count()) ? Diaporama->slides[Current.Object_Number]->CachedDuration-Diaporama->GetTransitionDuration(Current.Object_Number+1) : 0;
      if (AdjustedDuration < 33) 
         AdjustedDuration = 33;   // Not less than 1/30 sec
   }
   // Adjust Current.Object_Number
   if (Current.Object_Number >= Diaporama->slides.count()) 
   {
      if (Diaporama->slides.count() > 0) 
      {  // Force to last object
         Current.Object_Number    = Diaporama->slides.count()-1;
         Current.Object           = Diaporama->slides[Current.Object_Number];
         Current.Object_StartTime = Diaporama->GetObjectStartPosition(Current.Object_Number);
      } 
      else 
      {  // Force to first or none object
         Current.Object_Number   = 0;
         Current.Object_StartTime = 0;
         Current.Object = NULL;
      }
   } 
   else 
      Current.Object = Diaporama->slides[Current.Object_Number];

   Current.Object_InObjectTime = TimePosition - Current.Object_StartTime;

   // Now calculate wich sequence in the current object is
   if (Current.Object) 
   {
      int CurPos = 0;
      while (Current.Object_ShotSequenceNumber < (Current.Object->shotList.count() - 1) 
            && (CurPos + Current.Object->shotList[Current.Object_ShotSequenceNumber]->StaticDuration) <= Current.Object_InObjectTime)
      {
         CurPos = CurPos + Current.Object->shotList[Current.Object_ShotSequenceNumber]->StaticDuration;
         Current.Object_ShotSequenceNumber++;
      }
      Current.Object_CurrentShot = Current.Object->shotList[Current.Object_ShotSequenceNumber];
      if (Current.Object_ShotSequenceNumber < Current.Object->shotList.count() - 1)
         Current.Object_ShotDuration = Current.Object_CurrentShot->StaticDuration;
      else 
         Current.Object_ShotDuration = Current.Object_CurrentShot->pDiaporamaObject->CachedDuration - CurPos;

      // calculate CurrentObject_PCTDone
      //Current.Object_PCTDone = (double(Current.Object_InObjectTime) - double(CurPos)) / (double(Current.Object_ShotDuration/*-40*/));
      Current.Object_PCTDone = (double(Current.Object_InObjectTime) - double(CurPos)) / (double(Current.Object_ShotDuration-40));
      //if( Current.Object_PCTDone > 1.0 )
      //   qDebug() << "Current.Object_PCTDone = (" << double(Current.Object_InObjectTime) << " - " << double(CurPos) << ") / " << double(Current.Object_ShotDuration) << " = " << Current.Object_PCTDone;

      // Force all to SHOTTYPE_VIDEO
      Current.Object_CurrentShotType = eSHOTTYPE_VIDEO;

   } 
   else 
   {
      Current.Object_ShotSequenceNumber = 0;
      Current.Object_CurrentShot        = NULL;
      Current.Object_CurrentShotType    = eSHOTTYPE_STATIC;
   }

   // Calculate wich BackgroundIndex to be use (Background type : false=same as precedent - true=new background definition)
   Current.Object_BackgroundIndex = Current.Object_Number;
   while (Current.Object_BackgroundIndex > 0 && !Diaporama->slides[Current.Object_BackgroundIndex]->BackgroundType)
      Current.Object_BackgroundIndex--;

   // Define if for this position we have a transition with a previous object
   if (Current.Object != NULL 
      && (Current.Object->TransitionFamily != 0 || Current.Object->TransitionSubType !=0) 
      && Current.Object_InObjectTime < Current.Object->TransitionDuration) 
   {
      TransitionFamily = Current.Object->TransitionFamily;                      // Transition family
      TransitionSubType = Current.Object->TransitionSubType;                      // Transition type in the family
      TransitionDuration= Current.Object->TransitionDuration;                     // Transition duration (in msec)
      IsTransition      = true;
      TransitionPCTDone = double(Current.Object_InObjectTime)/double(TransitionDuration);

      // If Current.Object is not the first object
      if (Current.Object_Number > 0) 
      {
         Transit.Object_Number        = Current.Object_Number - 1;
         Transit.Object               = Diaporama->slides[Transit.Object_Number];
         Transit.Object_StartTime     = Diaporama->GetObjectStartPosition(Transit.Object_Number);
         Transit.Object_InObjectTime  = TimePosition - Transit.Object_StartTime;
         // Now calculate wich sequence in the Transition object is
         int CurPos  = 0;
         while (Transit.Object_ShotSequenceNumber < (Transit.Object->shotList.count() - 1) 
               && (CurPos + Transit.Object->shotList[Transit.Object_ShotSequenceNumber]->StaticDuration) <= Transit.Object_InObjectTime)
         {
            CurPos = CurPos + Transit.Object->shotList[Transit.Object_ShotSequenceNumber]->StaticDuration;
            Transit.Object_ShotSequenceNumber++;
         }
         Transit.Object_CurrentShot = Transit.Object->shotList[Transit.Object_ShotSequenceNumber];
         if (Transit.Object_ShotSequenceNumber < Transit.Object->shotList.count() - 1)
            Transit.Object_ShotDuration = Transit.Object_CurrentShot->StaticDuration;
         else 
            Transit.Object_ShotDuration = Transit.Object_CurrentShot->pDiaporamaObject->CachedDuration - CurPos;

         Transit.Object_CurrentShotType = eSHOTTYPE_VIDEO;

         // calculate Transit.Object_PCTDone
         //Transit.Object_PCTDone = (double(Transit.Object_InObjectTime) - double(CurPos))/(double(Transit.Object_ShotDuration));
         Transit.Object_PCTDone = (double(Transit.Object_InObjectTime) - double(CurPos))/(double(Transit.Object_ShotDuration-40));

         // Force all to SHOTTYPE_VIDEO
         // Calculate wich BackgroundIndex to be use for transition object (Background type : false=same as precedent - true=new background definition)
         Transit.Object_BackgroundIndex = Transit.Object_Number;
         while (Transit.Object_BackgroundIndex > 0 && !Diaporama->slides[Transit.Object_BackgroundIndex]->BackgroundType)
            Transit.Object_BackgroundIndex--;
      }
   }

   // Search music objects
   int64_t StartPosition;
   if (WantSound && Current.Object != NULL) 
      Current.Object_MusicObject = Diaporama->GetMusicObject(Current.Object_Number,StartPosition);
   if (WantSound && Transit.Object != NULL) 
      Transit.Object_MusicObject = Diaporama->GetMusicObject(Transit.Object_Number,StartPosition);

   //==============> Try to re-use values from PreviousFrame
   if (Current.Object && PreviousFrame) 
   {
      //************ Background
      if (PreviousFrame->Current.Object_BackgroundIndex == Current.Object_BackgroundIndex) 
      {
         Current.Object_BackgroundBrush = PreviousFrame->Current.Object_BackgroundBrush;             // Use the same background
         PreviousFrame->Current.Object_FreeBackgroundBrush = false;                                 // Set tag to not delete previous background
         Current.Object_PreparedBackground = PreviousFrame->Current.Object_PreparedBackground;
      }
      // Background of transition Object
      if (Transit.Object) 
      {
         if (PreviousFrame->Current.Object_BackgroundIndex == Transit.Object_BackgroundIndex) 
         {
            Transit.Object_BackgroundBrush = PreviousFrame->Current.Object_BackgroundBrush;             // Use the same background
            PreviousFrame->Current.Object_FreeBackgroundBrush = false;                                 // Set tag to not delete previous background
            Transit.Object_PreparedBackground = PreviousFrame->Current.Object_PreparedBackground;
         } 
         else if (PreviousFrame->Transit.Object_BackgroundIndex == Transit.Object_BackgroundIndex) 
         {
            Transit.Object_BackgroundBrush = PreviousFrame->Transit.Object_BackgroundBrush;         // Use the same background
            PreviousFrame->Transit.Object_FreeBackgroundBrush = false;                             // Set tag to not delete previous background
            Transit.Object_PreparedBackground = PreviousFrame->Transit.Object_PreparedBackground;
         }
         // Special case to disable free of background brush if transit object and current object use the same
         if (Transit.Object_BackgroundBrush == Current.Object_BackgroundBrush) 
         {
            Transit.Object_FreeBackgroundBrush = false;
         }
      }

      //************ SoundTrackMontage
      if (WantSound && PreviousFrame->Current.Object_Number == Current.Object_Number)
      {
         Current.Object_SoundTrackMontage = PreviousFrame->Current.Object_SoundTrackMontage;        // Use the same SoundTrackMontage
         PreviousFrame->Current.Object_FreeSoundTrackMontage = false;                               // Set tag to not delete previous SoundTrackMontage
      }
      // SoundTrackMontage of transition Object
      if (WantSound && Transit.Object)
      {
         if (PreviousFrame->Current.Object_Number == Transit.Object_Number)
         {
            Transit.Object_SoundTrackMontage = PreviousFrame->Current.Object_SoundTrackMontage;    // Use the same SoundTrackMontage
            PreviousFrame->Current.Object_FreeSoundTrackMontage = false;                           // Set tag to not delete previous SoundTrackMontage
         } 
         else if (PreviousFrame->Transit.Object_Number == Transit.Object_Number)
         {
            Transit.Object_SoundTrackMontage = PreviousFrame->Transit.Object_SoundTrackMontage;    // Use the same SoundTrackMontage
            PreviousFrame->Transit.Object_FreeSoundTrackMontage = false;                           // Set tag to not delete previous SoundTrackMontage
         }
      }

      //************ Music
      if (WantSound && PreviousFrame->Current.Object_MusicObject == Current.Object_MusicObject)
      {
         Current.Object_MusicTrack = PreviousFrame->Current.Object_MusicTrack;                      // Use the same Music track
         PreviousFrame->Current.Object_FreeMusicTrack = false;                                      // Set tag to not delete previous SoundTrackMontage
      }
      // Music of transition Object
      if (WantSound && Transit.Object)
      {
         if (PreviousFrame->Current.Object_MusicObject == Transit.Object_MusicObject)
         {
            Transit.Object_MusicTrack = PreviousFrame->Current.Object_MusicTrack;                  // Use the same SoundTrackMontage
            PreviousFrame->Current.Object_FreeMusicTrack = false;                                  // Set tag to not delete previous SoundTrackMontage
         } 
         else if (PreviousFrame->Transit.Object_MusicObject == Transit.Object_MusicObject)
         {
            Transit.Object_MusicTrack = PreviousFrame->Transit.Object_MusicTrack;                  // Use the same SoundTrackMontage
            PreviousFrame->Transit.Object_FreeMusicTrack = false;                                  // Set tag to not delete previous SoundTrackMontage
         }
         // Special case to disable TransitObject_MusicTrack if transit object and current object use the same
         if (Current.Object_MusicObject == Transit.Object_MusicObject) 
         {
            Transit.Object_FreeMusicTrack = false;
            Transit.Object_MusicTrack = NULL;
         }
      }
      // Definitively check PreviousFrame to know if we realy need to free MusicObject
      if (WantSound && PreviousFrame->Current.Object_FreeMusicTrack 
         && (PreviousFrame->Current.Object_MusicTrack == Current.Object_MusicTrack || PreviousFrame->Current.Object_MusicTrack == Transit.Object_MusicTrack))
         PreviousFrame->Current.Object_FreeMusicTrack = false;
      if (WantSound && PreviousFrame->Transit.Object_FreeMusicTrack 
            && (PreviousFrame->Transit.Object_MusicTrack == Current.Object_MusicTrack 
               || PreviousFrame->Transit.Object_MusicTrack == Transit.Object_MusicTrack
               || PreviousFrame->Transit.Object_MusicTrack == PreviousFrame->Current.Object_MusicTrack)
               )
         PreviousFrame->Transit.Object_FreeMusicTrack = false;

      //************ PreparedImage & RenderedImage
      IsShotStatic = PreviousFrame->Current.Object_Number == Current.Object_Number 
                     && PreviousFrame->Current.Object_CurrentShot == Current.Object_CurrentShot 
                     && ComputeIsShotStatic(Current.Object,Current.Object_ShotSequenceNumber);
      if (IsShotStatic) 
      {// Same shot
         Current.Object_PreparedImage = PreviousFrame->Current.Object_PreparedImage;                 // Use the same PreparedImage
         if (!IsTransition && !PreviousFrame->IsTransition)
            RenderedImage = PreviousFrame->RenderedImage;   // Use the same RenderedImage
      }
      // PreparedImage of transition Object
      if (Transit.Object) 
      {
         IsTransitStatic = ComputeIsShotStatic(Transit.Object,Transit.Object_ShotSequenceNumber);
         if (PreviousFrame->Current.Object_CurrentShot == Transit.Object_CurrentShot && IsTransitStatic)
         {
            Transit.Object_PreparedImage = PreviousFrame->Current.Object_PreparedImage;             // Use the same PreparedImage
         } 
         else if (PreviousFrame->Transit.Object_CurrentShot == Transit.Object_CurrentShot && IsTransitStatic)
         {
            Transit.Object_PreparedImage = PreviousFrame->Transit.Object_PreparedImage;             // Use the same PreparedImage
         }
      }
   }
//   if (Current.Object_Number == 9)
//      Current.dump();
}

cDiaporamaObjectInfo::ObjectInfo::~ObjectInfo()
{
   if (Object_FreeBackgroundBrush && Object_BackgroundBrush) 
   {
      delete Object_BackgroundBrush;
      Object_BackgroundBrush = NULL;
   }
   if (Object_FreeSoundTrackMontage && Object_SoundTrackMontage) 
   {
      delete Object_SoundTrackMontage;
      Object_SoundTrackMontage = NULL;
   }
   if (Object_FreeMusicTrack && Object_MusicTrack) 
   {
      delete Object_MusicTrack;
      Object_MusicTrack = NULL;
   }
}

void cDiaporamaObjectInfo::ObjectInfo::init()
{
   Object_Number                = 0;                 // Object number
   Object_StartTime             = 0;                 // Position (in msec) of the first frame relative to the diaporama
   Object_InObjectTime          = 0;                 // Position (in msec) in the object
   Object                       = NULL;              // Link to the current object
   Object_ShotSequenceNumber    = 0;                 // Number of the shot sequence in the current object
   Object_CurrentShot           = NULL;              // Link to the current shot in the current object
   Object_CurrentShotType       = eSHOTTYPE_STATIC;  // Type of the current shot : Static/Mobil/Video
   Object_ShotDuration          = 0;                 // Time the static shot end (if CurrentObject_CurrentShotType=SHOTTYPE_STATIC)
   Object_PCTDone               = 0;                 // PCT achevement for static shot
   Object_BackgroundIndex       = 0;                 // Object number containing current background definition
   Object_BackgroundBrush       = NULL;              // Current background brush
   Object_FreeBackgroundBrush   = true;              // True if allow to delete CurrentObject_BackgroundBrush during destructor
   Object_SoundTrackMontage     = NULL;              // Sound for playing sound from montage track
   Object_FreeSoundTrackMontage = true;              // True if allow to delete CurrentObject_SoundTrackMontage during destructor
   Object_MusicTrack            = NULL;              // Sound for playing music from music track
   Object_FreeMusicTrack        = true;              // True if allow to delete CurrentObject_MusicTrack during destructor
   Object_MusicObject           = NULL;              // Ref to the current playing music
}

struct cDiaporamaObjectInfo::ObjectInfo &cDiaporamaObjectInfo::ObjectInfo::operator=(const cDiaporamaObjectInfo::ObjectInfo &rhs)
{
    Object_Number                = rhs.Object_Number;               // Object number
    Object_StartTime             = rhs.Object_StartTime;            // Position (in msec) of the first frame relative to the diaporama
    Object_InObjectTime          = rhs.Object_InObjectTime;         // Position (in msec) in the object
    Object                       = rhs.Object;                      // Link to the current object
    Object_ShotSequenceNumber    = rhs.Object_ShotSequenceNumber;   // Number of the shot sequence in the current object
    Object_CurrentShot           = rhs.Object_CurrentShot;          // Link to the current shot in the current object
    Object_CurrentShotType       = rhs.Object_CurrentShotType;      // Type of the current shot : Static/Mobil/Video
    Object_ShotDuration          = rhs.Object_ShotDuration;         // Time the static shot end (if CurrentObject_CurrentShotType=SHOTTYPE_STATIC)
    Object_PCTDone               = rhs.Object_PCTDone;
    Object_BackgroundIndex       = rhs.Object_BackgroundIndex;      // Object number containing current background definition
    Object_BackgroundBrush       = rhs.Object_BackgroundBrush;      // Current background brush
    Object_FreeBackgroundBrush   = false;                           // True if allow to delete CurrentObject_BackgroundBrush during destructor
    Object_PreparedBackground    = rhs.Object_PreparedBackground;   // Current image produce for background
    Object_SoundTrackMontage     = rhs.Object_SoundTrackMontage;    // Sound for playing sound from montage track
    Object_FreeSoundTrackMontage = false;                           // True if allow to delete CurrentObject_SoundTrackMontage during destructor
    Object_PreparedImage         = rhs.Object_PreparedImage;        // Current image prepared
    Object_MusicTrack            = rhs.Object_MusicTrack;           // Sound for playing music from music track
    Object_FreeMusicTrack        = false;                           // True if allow to delete CurrentObject_MusicTrack during destructor
    Object_MusicObject           = rhs.Object_MusicObject;          // Ref to the current playing music
    return *this;
}

void cDiaporamaObjectInfo::ObjectInfo::dump()
{
   qDebug() << Object_Number  // Object number
   << " " << Object_StartTime // Position (in msec) of the first frame relative to the diaporama
   << " " << Object_InObjectTime                // Position (in msec) in the object
   //Object = NULL;              // Link to the current object
   << " " << Object_ShotSequenceNumber                 // Number of the shot sequence in the current object
   //Object_CurrentShot = NULL;              // Link to the current shot in the current object
   //Object_CurrentShotType = eSHOTTYPE_STATIC;  // Type of the current shot : Static/Mobil/Video
   << " " << Object_ShotDuration                 // Time the static shot end (if CurrentObject_CurrentShotType=SHOTTYPE_STATIC)
   << " " << Object_PCTDone                 // PCT achevement for static shot
   //Object_BackgroundIndex = 0;                 // Object number containing current background definition
   //Object_BackgroundBrush = NULL;              // Current background brush
   //Object_FreeBackgroundBrush = true;              // True if allow to delete CurrentObject_BackgroundBrush during destructor
   //Object_SoundTrackMontage = NULL;              // Sound for playing sound from montage track
   //Object_FreeSoundTrackMontage = true;              // True if allow to delete CurrentObject_SoundTrackMontage during destructor
   //Object_MusicTrack = NULL;              // Sound for playing music from music track
   //Object_FreeMusicTrack = true;              // True if allow to delete CurrentObject_MusicTrack during destructor
   //Object_MusicObject = NULL;              // Ref to the current playing music
   ;
}

void cDiaporamaObjectInfo::init()
{
   //==============> Pre-initialise all values
   IsShotStatic                        = false;
   IsTransitStatic                     = false;
   FrameDuration                       = 0;
   // Current object                     
   Current.init();

   // Transitionnal object
   IsTransition                        = false;             // True if transition in progress
   TransitionPCTDone                   = 0;                 // PCT achevement for transition
   TransitionFamily                   = TRANSITIONFAMILY_BASE;
   TransitionSubType                   = 0;
   TransitionDuration                  = 0;
   Transit.init();

   YUVFrame = NULL;
   hasYUV = false;
}

// make a copy of PreviousFrame
void cDiaporamaObjectInfo::Copy(cDiaporamaObjectInfo *PreviousFrame) 
{
    IsShotStatic                        = PreviousFrame->IsShotStatic;
    IsTransitStatic                     = PreviousFrame->IsTransitStatic;
    FrameDuration                       = PreviousFrame->FrameDuration;
    TransitionFamily                   = PreviousFrame->TransitionFamily;                  // Transition family
    TransitionSubType                   = PreviousFrame->TransitionSubType;                  // Transition type in the family
    TransitionDuration                  = PreviousFrame->TransitionDuration;                 // Transition duration (in msec)
                                          
    // Current object                     
    Current = PreviousFrame->Current;
                                          
    // Transitional object
    IsTransition                        = PreviousFrame->IsTransition;                       // True if transition in progress
    TransitionPCTDone                   = PreviousFrame->TransitionPCTDone;                  // PCT achevement for transition
    Transit = PreviousFrame->Transit;
}

bool cDiaporamaObjectInfo::ComputeIsShotStatic(cDiaporamaObject *Object,int ShotNumber) 
{
   bool IsStatic=true;
   if (ShotNumber == 0) 
   {
      for (auto compositionObject : Object->shotList[0]->ShotComposition.compositionObjects)
         if (compositionObject->isVideo()
            || compositionObject->AnimType() != 0)
            IsStatic = false;
   } 
   else
   { 
      for (int i = 0; i < Object->shotList[ShotNumber]->ShotComposition.compositionObjects.count(); i++)
      {
         cCompositionObject* compositionObject = Object->shotList[ShotNumber]->ShotComposition.compositionObjects[i];
         if (compositionObject->isVisible())
         {
            if (compositionObject->isVideo())
               IsStatic = false; 
            else 
            {
               if (!compositionObject->sameAs(Object->shotList[ShotNumber - 1]->ShotComposition.compositionObjects[i]))
                  IsStatic = false;
            }
         }
      }
   }
   return IsStatic;
}

void cDiaporamaObjectInfo::ensureAudioTracksReady(double WantedDuration, int Channels, int64_t SamplingRate, enum AVSampleFormat SampleFormat)
{
   // Ensure MusicTracks are ready
   if (Current.Object && Current.Object_MusicTrack == NULL)
   {
      Current.Object_MusicTrack = new cSoundBlockList();
      Current.Object_MusicTrack->SetFPS(WantedDuration, Channels, SamplingRate, SampleFormat);
   }
   if (Transit.Object && Transit.Object_MusicTrack == NULL
      && Transit.Object_MusicObject != NULL
      && Transit.Object_MusicObject != Current.Object_MusicObject)
   {
      Transit.Object_MusicTrack = new cSoundBlockList();
      Transit.Object_MusicTrack->SetFPS(WantedDuration, Channels, SamplingRate, SampleFormat);
   }

   // Ensure SoundTracks are ready
   if (Current.Object && Current.Object_SoundTrackMontage == NULL)
   {
      Current.Object_SoundTrackMontage = new cSoundBlockList();
      Current.Object_SoundTrackMontage->SetFPS(WantedDuration, Channels, SamplingRate, SampleFormat);
   }
   if (Transit.Object && Transit.Object_SoundTrackMontage == NULL)
   {
      Transit.Object_SoundTrackMontage = new cSoundBlockList();
      Transit.Object_SoundTrackMontage->SetFPS(WantedDuration, Channels, SamplingRate, SampleFormat);
   }
}

//============================================================================================
// Destructor
//============================================================================================
cDiaporamaObjectInfo::~cDiaporamaObjectInfo() 
{
   // CurrentObject -- see dtor of ObjectInfo
   // Transition Object -- see dtor of ObjectInfo
}

cCompositionObjectMask::cCompositionObjectMask()
{
   //BackgroundForm = SHAPEFORM_RECTANGLE;
   //PenSize = 0;
   RotateZAxis = 0;
   invertMask = false;
}

cCompositionObjectMask::cCompositionObjectMask(cCompositionObjectMask *src)
{
   setRect(src->getRect());
   //BackgroundForm = src->getBackgroundForm();
   setZAngle(src->zAngle());
   invertMask = src->invertedMask();
}

void cCompositionObjectMask::copyFrom(cCompositionObjectMask*srcMask)
{
   wsRect = srcMask->wsRect;
   RotateZAxis = srcMask->RotateZAxis;
   invertMask = srcMask->invertMask;
}

bool cCompositionObjectMask::sameAs(const cCompositionObjectMask *other) const
{
   if(wsRect != other->wsRect)
      return false;
   if( RotateZAxis != other->RotateZAxis )
      return false;
   if( invertMask != other->invertMask )
      return false;
   return true;
}

//bool drawSmooth(QPainter *painter, QRectF outRec, QRectF orgRect, QImage *image)
//{
//   return false;
//   qreal topDiff = orgRect.top() - outRec.top();
//   qreal leftDiff = orgRect.left() - outRec.left();
//   qreal one8 = 1.0/8.0;
//   if( (topDiff > one8 && topDiff < one8*6) || ((leftDiff > one8 && leftDiff < one8*6)) )
//   {
//      QPaintDevice *pDev = painter->device();
//      QImage *destImage = dynamic_cast< QImage* >( pDev );
//      if( destImage )
//      {
//         int fillHeight = 0;
//         int fillWidth = 0;
//         if( topDiff >= one8 )
//         {
//            fillHeight = 2;
//            //if( topDiff <= one8*6 ) fillHeight++;
//            //if( topDiff <= one8*3 ) fillHeight++;
//            //if( topDiff >= one8 ) fillHeight++;
//         }
//
//         if( leftDiff >= one8 )
//         {
//            fillWidth = 2;
//            //if( leftDiff >= one8*6 ) fillWidth++;
//            //if( leftDiff >= one8*3 ) fillWidth++;
//            //if( leftDiff >= one8 ) fillWidth++;
//         }
//
//         if( fillHeight )
//         {
//            // get the topline and scale to 4 pixel height
//            QImage topLine = destImage->copy(outRec.left(), outRec.top(), image->width(), 1).scaled(image->width(), 4,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
//            QImage topInsLine = image->copy(0, 0, image->width(), 1).scaled(image->width(), fillHeight,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
//            QPainter p(&topLine);
//            p.drawImage(0,4-fillHeight,topInsLine);
//            p.end();
//            // scale back to single line 
//            topLine = topLine.scaled(topLine.width(),1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
//            // draw the topline
//            painter->drawImage(outRec.topLeft(),topLine); 
//         }
//         if( fillWidth )
//         {
//            // get the topline and scale to 4 pixel height
//            QImage leftLine = destImage->copy(outRec.left(), outRec.top(), 1, image->height() ).scaled(4,image->height(),Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
//            QImage leftInsLine = image->copy(0, 0, 1, image->height()).scaled(fillWidth, image->height(),Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
//            QPainter p(&leftLine);
//            p.drawImage(4-fillWidth,0,leftInsLine);
//            p.end();
//            // scale back to single line 
//            leftLine = leftLine.scaled(1,leftLine.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
//            // draw the topline
//            painter->drawImage(outRec.topLeft(),leftLine); 
//         }
//         // draw the rest
//         painter->drawImage(outRec.left()+(fillWidth?1:0), outRec.top()+(fillHeight?1:0),*image,fillWidth?1:0,fillHeight?1:0);         
//         //qDebug() << orgRect << " " << outRec << " smoothed " << fillHeight << " " << fillWidth;
//         return true;
//      }
//   }
//   return false;
//}

bool drawSmooth(QPainter *painter, QRectF outRec, QRectF orgRect, QImage *image)
{
   //return false;
   qreal topDiff = orgRect.top() - outRec.top();
   qreal leftDiff = orgRect.left() - outRec.left();
   qreal one4 = 1.0/4.0;
   if( (topDiff > one4 && topDiff < one4*3) || ((leftDiff > one4 && leftDiff < one4*3)) )
   {
      QPaintDevice *pDev = painter->device();
      QImage *destImage = dynamic_cast< QImage* >( pDev );
      if( destImage )
      {
         int xOffset = 0;
         int yOffset = 0;
         if( topDiff >= one4 )
         {
            yOffset = 1;
            //if( topDiff <= one8*6 ) fillHeight++;
            //if( topDiff <= one8*3 ) fillHeight++;
            //if( topDiff >= one8 ) fillHeight++;
         }

         if( leftDiff >= one4 )
         {
            xOffset = 1;
            //if( leftDiff >= one8*6 ) fillWidth++;
            //if( leftDiff >= one8*3 ) fillWidth++;
            //if( leftDiff >= one8 ) fillWidth++;
         }

         QImage dImage = destImage->copy(outRec.left(), outRec.top(), outRec.width(), outRec.height()).scaled(outRec.width()*2, outRec.height()*2,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
         QPainter p(&dImage);
         QRect targetRect(xOffset, yOffset, dImage.width() - xOffset*2, dImage.height() - yOffset*2);
         //painter->drawImage(targetRect,image->scaled(image->width()*2, image->height()*2, Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
         p.drawImage(xOffset, yOffset,image->scaled(image->width()*2, image->height()*2, Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
         p.end();
         //if( fillHeight )
         //{
         //   // get the topline and scale to 4 pixel height
         //   QImage topLine = destImage->copy(outRec.left(), outRec.top(), image->width(), 1).scaled(image->width(), 4,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
         //   QImage topInsLine = image->copy(0, 0, image->width(), 1).scaled(image->width(), fillHeight,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
         //   QPainter p(&topLine);
         //   p.drawImage(0,4-fillHeight,topInsLine);
         //   p.end();
         //   // scale back to single line 
         //   topLine = topLine.scaled(topLine.width(),1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
         //   // draw the topline
         //   painter->drawImage(outRec.topLeft(),topLine); 
         //}
         //if( fillWidth )
         //{
         //   // get the topline and scale to 4 pixel height
         //   QImage leftLine = destImage->copy(outRec.left(), outRec.top(), 1, image->height() ).scaled(4,image->height(),Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
         //   QImage leftInsLine = image->copy(0, 0, 1, image->height()).scaled(fillWidth, image->height(),Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
         //   QPainter p(&leftLine);
         //   p.drawImage(4-fillWidth,0,leftInsLine);
         //   p.end();
         //   // scale back to single line 
         //   leftLine = leftLine.scaled(1,leftLine.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
         //   // draw the topline
         //   painter->drawImage(outRec.topLeft(),leftLine); 
         //}
         // draw the rest
         painter->drawImage(outRec.left(), outRec.top(),dImage.scaled(dImage.width()/2, dImage.height()/2));
         //qDebug() << orgRect << " " << outRec << " smoothed " << fillHeight << " " << fillWidth;
         return true;
      }
   }
   return false;
}

void cDiaporama::LoadSourcesYUV(cDiaporamaObjectInfo *Info, QSize size, bool PreviewMode, bool AddStartPos, CompositionContextList &PreparedTransitBrushList, CompositionContextList &PreparedBrushList, int NbrDuration)
{
   #ifdef LOGLOAD
   LONGLONG cp = curPCounter();
   #endif
   if (!Info->Current.Object)
      return;

   QFutureWatcher<void> ThreadPrepareCurrentMusicBloc;
   QFutureWatcher<void> ThreadPrepareTransitMusicBloc;

   // W and H = 0 when producing sound track in render process
   bool SoundOnly = size.isNull();

   //==============> Music track part

   //if (Info->Current.Object )
   //  qDebug() << "Info->Current Object_Number " << Info->Current.Object_Number << " Object_InObjectTime" << Info->Current.Object_InObjectTime; 
   //if( Info->IsTransition && Info->Transit.Object )
   //    qDebug() << "Info->Transit Object_Number " << Info->Transit.Object_Number << " Object_InObjectTime" << Info->Transit.Object_InObjectTime; 
   if (Info->Current.Object && Info->Current.Object_MusicTrack)
   {
      PrepareMusicBlocContext Context;
      Context.PreviewMode = PreviewMode;
      Context.Column = Info->Current.Object_Number;
      Context.Position = Info->Current.Object_InObjectTime;
      Context.MusicTrack = Info->Current.Object_MusicTrack;
      Context.NbrDuration = NbrDuration;
      Context.FadIn = Info->Current.Object->CachedMusicFadIN;
      Context.FadOut = false;
      Context.IsCurrent = true;
      Context.PositionInSlide = Info->Current.Object_InObjectTime;
      Context.slideStartpos = Info->Current.Object_StartTime;
      //ThreadPrepareCurrentMusicBloc.setFuture(QtConcurrent::run(this,&cDiaporama::PrepareMusicBloc,&Context));
      PrepareMusicBloc(&Context);
      //Context.MusicTrack->dump();
   }
   #ifdef LOGLOAD
   qDebug() << "loadSources stage 1 " << PC2time(curPCounter() - cp, true);
   cp = curPCounter();
   #endif
   if (!applicationConfig->seamlessMusic)
   {
      if (Info->IsTransition && Info->Transit.Object && Info->Transit.Object_MusicTrack)
      {
         PrepareMusicBlocContext Context;
         Context.PreviewMode = PreviewMode;
         Context.Column = Info->Transit.Object_Number;
         Context.Position = Info->Transit.Object_InObjectTime;
         Context.MusicTrack = Info->Transit.Object_MusicTrack;
         Context.NbrDuration = NbrDuration;
         Context.FadIn = false;
         Context.FadOut = Info->Current.Object->CachedPrevMusicFadOUT;
         Context.IsCurrent = false;
         Context.PositionInSlide = Info->Current.Object_InObjectTime;
         ThreadPrepareTransitMusicBloc.setFuture(QT_CONCURRENT_RUN_1(this, &cDiaporama::PrepareMusicBloc, &Context));
      }
   }
   #ifdef LOGLOAD
   qDebug() << "loadSources stage 2 " << PC2time(curPCounter() - cp, true);
   cp = curPCounter();
   #endif


   //==============> Image part

   // Transition Object if a previous was not kept !
   if (Info->IsTransition)
   {
      if (Info->Transit.Object)
      {
         PrepareImage(Info, size, false, AddStartPos, PreparedTransitBrushList);
      }
      else if (Info->Transit.Object_PreparedImage.isNull())
      {
         Info->Transit.Object_PreparedImage = QImage(size, QImage::Format_ARGB32_Premultiplied);
         QPainter P;
         P.begin(&Info->Transit.Object_PreparedImage);
         P.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
         P.fillRect(QRect(QPoint(0, 0), size), Qt::black);//Qt::transparent);
         P.end();
      }
   }
   #ifdef LOGLOAD
   qDebug() << "loadSources stage 3 " << PC2time(curPCounter() - cp, true);
   cp = curPCounter();
   #endif

   // Load Source image
   PrepareImageYUV(Info, size, true, AddStartPos, PreparedBrushList);

   #ifdef LOGLOAD
   qDebug() << "loadSources stage 4 " << PC2time(curPCounter() - cp, true);
   cp = curPCounter();
   #endif

   //==============> Background part

   if (!SoundOnly)
   {
      // Search background context for CurrentObject if a previous was not keep !
      if (Info->Current.Object_BackgroundBrush == NULL)
         getBackgroundBrush(Info->Current, size, PreviewMode);
      #ifdef LOGLOAD
      qDebug() << "loadSources stage 5 " << PC2time(curPCounter() - cp, true);
      cp = curPCounter();
      #endif

      // same job for Transition Object if a previous was not keep !
      if (Info->Transit.Object && Info->Transit.Object_BackgroundBrush == NULL)
         getBackgroundBrush(Info->Transit, size, PreviewMode);
      #ifdef LOGLOAD
      qDebug() << "loadSources stage 6 " << PC2time(curPCounter() - cp, true);
      cp = curPCounter();
      #endif
   }

   //==============> Mixing of music and soundtrack
   //QTime b;
   //b.start();

   if (Info->Current.Object && Info->Current.Object_MusicTrack && ThreadPrepareCurrentMusicBloc.isRunning())
      ThreadPrepareCurrentMusicBloc.waitForFinished();
   if (Info->Transit.Object && Info->Transit.Object_MusicTrack && ThreadPrepareTransitMusicBloc.isRunning())
      ThreadPrepareTransitMusicBloc.waitForFinished();

   #ifdef LOGLOAD
   qDebug() << "loadSources stage 7 " << PC2time(curPCounter() - cp, true);
   cp = curPCounter();
   #endif

   // Soundtrack mix with fade in/fade out
   if (/*!applicationConfig->seamlessMusic &&*/ Info->IsTransition && (Info->Current.Object_SoundTrackMontage || Info->Transit.Object_SoundTrackMontage))
   {
      if (Info->Current.Object_SoundTrackMontage == NULL)
      {
         // if we don't have a Current.Object_SoundTrackMontage, we need to create it because it's the destination !
         Info->Current.Object_SoundTrackMontage = new cSoundBlockList();
         Info->Current.Object_SoundTrackMontage->SetFPS(Info->FrameDuration, 2, Info->Transit.Object_SoundTrackMontage->SamplingRate, AV_SAMPLE_FMT_S16);
      }
      // Ensure this track has enough data
      while (Info->Current.Object_SoundTrackMontage->ListCount()<Info->Current.Object_SoundTrackMontage->NbrPacketForFPS)
         Info->Current.Object_SoundTrackMontage->AppendNullSoundPacket(Info->Current.Object_StartTime + Info->Current.Object_InObjectTime);

      // Mix current and transit SoundTrackMontage (if needed)
      // @ the end: only current SoundTrackMontage exist !
      Info->Current.Object_SoundTrackMontage->Mutex.lock();
      for (int i = 0; i < Info->Current.Object_SoundTrackMontage->NbrPacketForFPS; i++)
      {
         // Mix the 2 sources buffer using the first buffer as destination and remove one paquet from the Transit.Object_SoundTrackMontage
         int16_t *Paquet = Info->Transit.Object_SoundTrackMontage ? Info->Transit.Object_SoundTrackMontage->DetachFirstPacket(true) : NULL;

         //int32_t mix;
         int16_t *Buf1 = i < Info->Current.Object_SoundTrackMontage->ListCount() ? Info->Current.Object_SoundTrackMontage->GetAt(i) : NULL;
         int     Max = Info->Current.Object_SoundTrackMontage->SoundPacketSize / (Info->Current.Object_SoundTrackMontage->SampleBytes*Info->Current.Object_SoundTrackMontage->Channels);

         double  FadeAdjust = sin(1.5708*double(Info->Current.Object_InObjectTime + (double(i) / double(Info->Current.Object_SoundTrackMontage->NbrPacketForFPS))*double(Info->FrameDuration)) / double(Info->TransitionDuration));
         double  FadeAdjust2 = 1 - FadeAdjust;

         int16_t *Buf2 = (Paquet != NULL) ? Paquet : NULL;
         if (Buf1 != NULL && Buf2 == NULL)
         {
            // Apply Fade in to Buf1
            for (int j = 0; j < Max; j++)
            {
               // Left channel : Adjust if necessary (16 bits)
               *Buf1 = (int16_t)qBound(-32768.0, double(*Buf1)*FadeAdjust, 32767.0); Buf1++;
               // Right channel : Adjust if necessary (16 bits)
               *Buf1 = (int16_t)qBound(-32768.0, double(*Buf1)*FadeAdjust, 32767.0); Buf1++;
            }
         }
         else if (Buf1 != NULL && Buf2 != NULL)
         {
            // do mixing
            for (int j = 0; j < Max; j++)
            {
               *Buf1 = (int16_t)qBound(-32768.0, double(*Buf1)*FadeAdjust + double(*Buf2++)*FadeAdjust2, 32767.0); Buf1++;
               *Buf1 = (int16_t)qBound(-32768.0, double(*Buf1)*FadeAdjust + double(*Buf2++)*FadeAdjust2, 32767.0); Buf1++;
            }
            av_free(Paquet);
         }
         else if (Buf1 == NULL && Buf2 != NULL)
         {
            // swap buf1 and buf2
            Info->Current.Object_SoundTrackMontage->SetAt(i, Buf2);
            // Apply Fade out to Buf2
            for (int j = 0; j < Max; j++)
            {
               // Left channel : Adjust if necessary (16 bits)
               *Buf2 = (int16_t)qBound(-32768.0, double(*Buf2)*FadeAdjust2, 32767.0); Buf2++;
               // Right channel : Adjust if necessary (16 bits)
               *Buf2 = (int16_t)qBound(-32768.0, double(*Buf2)*FadeAdjust2, 32767.0); Buf2++;
            }
         }
      }
      Info->Current.Object_SoundTrackMontage->Mutex.unlock();
   }
   #ifdef LOGLOAD
   qDebug() << "loadSources stage 8 " << PC2time(curPCounter() - cp, true);
   cp = curPCounter();
   #endif

   // Mix current and transit music
   // @ the end: only current music exist !
   // Mix the 2 sources buffer using the first buffer as destination and remove one paquet from the PreviousMusicTrack
   if (/*!applicationConfig->seamlessMusic && */Info->IsTransition && Info->Current.Object_MusicTrack)
   {
      Info->Current.Object_MusicTrack->Mutex.lock();
      int Max = Info->Current.Object_MusicTrack->NbrPacketForFPS;
      if (Max > Info->Current.Object_MusicTrack->ListCount())
         Max = Info->Current.Object_MusicTrack->ListCount();
      for (int i = 0; i < Max; i++)
      {
         if (i >= Info->Current.Object_MusicTrack->ListCount())
            Info->Current.Object_MusicTrack->AppendNullSoundPacket(0, true);

         int16_t *Buf1 = Info->Current.Object_MusicTrack->GetAt(i);
         int16_t *Buf2 = Info->Transit.Object_MusicTrack ? Info->Transit.Object_MusicTrack->DetachFirstPacket(true) : NULL;
         int Max = Info->Current.Object_MusicTrack->SoundPacketSize / (Info->Current.Object_MusicTrack->SampleBytes*Info->Current.Object_MusicTrack->Channels);

         if (Buf2)
         {
            if (!Buf1)
            {
               Info->Current.Object_MusicTrack->SetAt(i, Buf2);
            }
            else
            {
               int16_t *B1 = Buf1, *B2 = Buf2;

               for (int j = 0; j < Max; j++)
               {
                  // Left channel : Adjust if necessary (16 bits)
                  *B1 = (int16_t)qBound(-32768, int32_t(*B1 + *B2++), 32767); B1++;
                  // Right channel : Adjust if necessary (16 bits)
                  *B1 = (int16_t)qBound(-32768, int32_t(*B1 + *B2++), 32767); B1++;
               }
               av_free(Buf2);
               Info->Current.Object_MusicTrack->SetAt(i, Buf1);
            }
         }
      }
      Info->Current.Object_MusicTrack->Mutex.unlock();
   }
   #ifdef LOGLOAD
   qDebug() << "loadSources stage 9 " << PC2time(curPCounter() - cp, true);
   cp = curPCounter();
   #endif
}

void cDiaporama::PrepareImageYUV(cDiaporamaObjectInfo *Info, QSize size, bool IsCurrentObject, bool AddStartPos, CompositionContextList &PreparedBrushList)
{
//   static int puzzleCount = 0;
   //autoTimer pi("PrepareImage");
   bool SoundOnly = size.isNull();  // W and H = 0 when producing sound track in render process

                                    // return immediatly if we have image
   cDiaporamaObjectInfo::ObjectInfo *objInfo = IsCurrentObject ? &Info->Current : &Info->Transit;
   if (!SoundOnly && !objInfo->Object_PreparedImage.isNull())
      return;

   //LONGLONG cp = curPCounter();

   int64_t          Duration = objInfo->Object_ShotDuration;
   cDiaporamaShot   *CurShot = objInfo->Object_CurrentShot;
   cDiaporamaObject *CurObject = objInfo->Object;
   int              CurTimePosition = objInfo->Object_InObjectTime;
   cSoundBlockList  *SoundTrackMontage = objInfo->Object_SoundTrackMontage;

   if (SoundOnly)
   {
      // if sound only then parse all shot objects to create SoundTrackMontage
      for(auto obj : CurShot->ShotComposition.compositionObjects)
      {
         if (obj->isVideo() && obj->BackgroundBrush->SoundVolume != 0)
         {
            // Calc StartPosToAdd depending on AddStartPos
            int64_t StartPosToAdd = (AddStartPos ? QTime(0, 0, 0, 0).msecsTo(((cVideoFile *)obj->BackgroundBrush->MediaObject)->StartTime) : 0);

            // Calc VideoPosition depending on video set to pause (visible=off) in previous shot
            int64_t VideoPosition = 0;
            int64_t ThePosition = 0;
            int TheShot = 0;
            while ((TheShot < CurObject->shotList.count()) && (ThePosition + CurObject->shotList[TheShot]->StaticDuration < CurTimePosition))
            {
               cCompositionObject *object = CurObject->shotList[TheShot]->ShotComposition.getCompositionObject(obj->index());
               if (object->isVisible())
                  VideoPosition += CurObject->shotList[TheShot]->StaticDuration;

               ThePosition += CurObject->shotList[TheShot]->StaticDuration;
               TheShot++;
            }
            VideoPosition += (CurTimePosition - ThePosition);

            obj->DrawCompositionObject(CurObject, NULL, double(size.height()) / double(1080), QSizeF(0, 0), true, VideoPosition + StartPosToAdd, SoundTrackMontage, 1, 1, NULL, false, CurShot->StaticDuration, true);
         }
      }
      return;
   }

   cCompositionObject* compositionObject = CurShot->ShotComposition.compositionObjects[CurShot->fullScreenVideoIndex];
   cCompositionObjectContext *context = PreparedBrushList[CurShot->fullScreenVideoIndex];
   context->Compute();
   cVideoFile *MediaObject = (cVideoFile *)compositionObject->BackgroundBrush->MediaObject;
   AVFrame *RenderFrame = MediaObject->YUVAt(false, context->VideoPosition + context->StartPosToAdd, SoundTrackMontage, compositionObject->BackgroundBrush->Deinterlace, compositionObject->BackgroundBrush->SoundVolume, SoundOnly, false);

   if(RenderFrame!=NULL)
      Info->YUVFrame = RenderFrame;//av_frame_clone(RenderFrame);
   Info->hasYUV = true;
   return;
/*
   QImage Image(size, QImage::Format_ARGB32_Premultiplied);
   Image.fill(0);
   if (Image.isNull())
      return;

   QPainter P;
   P.begin(&Image);
   // Prepare a transparent image
   //P.begin(&Image);
   P.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing | QPainter::NonCosmeticDefaultPen);
   //P.setCompositionMode(QPainter::CompositionMode_Source);
   //P.fillRect(QRect(QPoint(0,0),size),Qt::transparent);
   P.setCompositionMode(QPainter::CompositionMode_SourceOver);

   //qDebug() << "PrepareImage stage 1 " << PC2time(curPCounter()-cp,true);
   //cp = curPCounter();

   if (CurShot)
   {
      // Compute each item of the collection
      int lastStatic = -1;
      QList<range> ranges;
      for (int aa = 0; aa < PreparedBrushList.count(); aa++)
      {
         PreparedBrushList[aa]->Compute();
         cCompositionObject *Object = PreparedBrushList[aa]->CurShot->ShotComposition.compositionObjects[PreparedBrushList[aa]->ObjectNumber];
         if (Object->IsStatic && lastStatic == aa - 1)
            lastStatic = aa;
         if (ranges.isEmpty() || !Object->IsStatic || !ranges.last().isStatic)
            ranges.append(range(aa, aa, Object->IsStatic));
         else
            ranges.last().end++;
      }
      if (PreparedBrushList.count() < 2)
         lastStatic = -1;

      #ifdef MULTICACHING
      QSizeF sizeF = size;//QSizeF(W,H);
      double adjRatio = sizeF.height() / 1080.0;
      foreach(range r, ranges)
      {
         //qDebug() << "range " << r.start << " " << r.end << " " << r.isStatic;
         if (r.end > r.start)
         {
            QImage staticPart;
            //if (lastStatic >= 0)
            {
               if (PreparedBrushList[0]->PreviewMode)
                  staticPart = applicationConfig->ImagesCache.getPreviewImage(CurShot, r.end);
               else
                  staticPart = applicationConfig->ImagesCache.getRenderImage(CurShot, r.end);
            }
            if (staticPart.isNull() || staticPart.size() != size)
            {
               // draw multiple items into one cached image
               //qDebug() << "draw objects " <<  r.start << " bis " << r.end << " to obj " << (void *)PreparedBrushList[r.end]->CurShot->ShotComposition.compositionObjects[PreparedBrushList[r.end]->ObjectNumber];
               //qDebug() << "prepare static part " ;
               staticPart = QImage(size, QImage::Format_ARGB32_Premultiplied);
               staticPart.fill(0);

               // Prepare a transparent image
               QPainter PS;
               PS.begin(&staticPart);
               PS.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing | QPainter::NonCosmeticDefaultPen);
               //PS.setCompositionMode(QPainter::CompositionMode_SourceOver);
               //draw the static parts of the composition
               //foreach(cCompositionObject* compositionObject, CurShot->ShotComposition.compositionObjects)
               for (int j = r.start; j <= r.end; j++)
               {
                  cCompositionObject* compositionObject = CurShot->ShotComposition.compositionObjects[j];
                  cCompositionObjectContext *context = PreparedBrushList[j];
                  if (compositionObject->isVisible())
                     compositionObject->DrawCompositionObject(CurObject, &PS, adjRatio, sizeF,
                     context->PreviewMode, context->VideoPosition + context->StartPosToAdd,
                     context->SoundTrackMontage,
                     context->BlockPctDone, context->ImagePctDone,
                     context->PrevCompoObject, Duration,
                     true, false, QRectF(), false, context);
               }
               PS.end();
               if (PreparedBrushList[0]->PreviewMode)
                  applicationConfig->ImagesCache.addPreviewImage(CurShot, r.end, staticPart);
               else
                  applicationConfig->ImagesCache.addRenderImage(CurShot, r.end, staticPart);
            }
            //else
            //{
            //   qDebug() << "reuse static bitmap for range " << r.start << " bis " << r.end;
            //}
            P.drawImage(0, 0, staticPart);
         }
         else
         {
            // draw a single item
            cCompositionObject* compositionObject = CurShot->ShotComposition.compositionObjects[r.start];
            cCompositionObjectContext *context = PreparedBrushList[r.start];
            if (compositionObject->isVisible() /*&& j > lastStatic* /)
            {
               compositionObject->DrawCompositionObject(CurObject, &P, adjRatio, sizeF,
                  context->PreviewMode, context->VideoPosition + context->StartPosToAdd,
                  context->SoundTrackMontage,
                  context->BlockPctDone, context->ImagePctDone,
                  context->PrevCompoObject, Duration,
                  true, false, QRectF(), false, context);
            }
         }
      }

      #else

      // Draw collection
      QSizeF sizeF = size;//QSizeF(W,H);
      double adjRatio = sizeF.height() / 1080.0;
      int j = 0;
      QImage staticPart;
      if (lastStatic >= 0)
      {
         if (PreparedBrushList[0]->PreviewMode)
            staticPart = applicationConfig->ImagesCache.getPreviewImage(CurShot);
         else
            staticPart = applicationConfig->ImagesCache.getRenderImage(CurShot);
      }
      if (lastStatic >= 0 && (staticPart.isNull() || staticPart.size() != size))
      {
         //qDebug() << "prepare static part " ;
         staticPart = QImage(size, QImage::Format_ARGB32_Premultiplied);
         staticPart.fill(0);

         // Prepare a transparent image
         QPainter PS;
         PS.begin(&staticPart);
         PS.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing | QPainter::NonCosmeticDefaultPen);
         //PS.setCompositionMode(QPainter::CompositionMode_Source);
         //PS.fillRect(QRect(QPoint(0, 0),size), Qt::transparent);
         PS.setCompositionMode(QPainter::CompositionMode_SourceOver);
         //draw the static parts of the composition
         foreach(cCompositionObject* compositionObject, CurShot->ShotComposition.compositionObjects)
         {
            cCompositionObjectContext *context = PreparedBrushList[j];
            if (compositionObject->isVisible())
               compositionObject->DrawCompositionObject(CurObject, &PS, adjRatio, sizeF,
               context->PreviewMode, context->VideoPosition + context->StartPosToAdd,
               context->SoundTrackMontage,
               context->BlockPctDone, context->ImagePctDone,
               context->PrevCompoObject, Duration,
               true, false, QRectF(), false, context);
            j++;
            if (j > lastStatic)
               break;
         }
         PS.end();
         if (PreparedBrushList[0]->PreviewMode)
            applicationConfig->ImagesCache.addPreviewImage(CurShot, staticPart);
         else
            applicationConfig->ImagesCache.addRenderImage(CurShot, staticPart);
      }
      if (lastStatic >= 0)
      {
         qDebug() << "use static part ";
         P.drawImage(0, 0, staticPart);
      }

      j = 0;
      foreach(cCompositionObject* compositionObject, CurShot->ShotComposition.compositionObjects)
      {
         cCompositionObjectContext *context = PreparedBrushList[j];
         if (compositionObject->isVisible() && j > lastStatic)
         {
            compositionObject->DrawCompositionObject(CurObject, &P, adjRatio, sizeF,
               context->PreviewMode, context->VideoPosition + context->StartPosToAdd,
               context->SoundTrackMontage,
               context->BlockPctDone, context->ImagePctDone,
               context->PrevCompoObject, Duration,
               true, false, QRectF(), false, context);
         }
         j++;
      }

      #endif
   }
   P.end();

   if (IsCurrentObject)
      Info->Current.Object_PreparedImage = Image;
   else
      Info->Transit.Object_PreparedImage = Image;
      */
}

