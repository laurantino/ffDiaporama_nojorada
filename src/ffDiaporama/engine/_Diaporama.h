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

#ifndef CDIAPORAMA_H
#define CDIAPORAMA_H

// Basic inclusions (common to all files)
#include "_GlobalDefines.h"
#include "_Transition.h"

// Include some additional standard class
#include "cApplicationConfig.h"
#include "cBaseMediaFile.h"
#include "cVideoFile.h"
#include "cBrushDefinition.h"
#include "cTextFrame.h"

#include "DlgWorkingTask/DlgWorkingTask.h"


// some forward-declarations 
class cCompositionList;
class cCompositionObject;
class cDiaporamaShot;
class cDiaporamaObjectInfo;
class cDiaporama;
class cDiaporamaObject;

// Shot type definition
enum eShotType {eSHOTTYPE_STATIC,eSHOTTYPE_MOBIL,eSHOTTYPE_VIDEO};

//============================================
// Block animations
//============================================
#define BLOCKANIMTYPE_NONE                  0
#define BLOCKANIMTYPE_MULTIPLETURN          1
#define BLOCKANIMTYPE_DISSOLVE              2

enum eBlockAnimValue { eAPPEAR, eDISAPPEAR, eBLINK_SLOW, eBLINK_MEDIUM, eBLINK_FAST, eBLINK_VERYFAST };
//============================================
// Text margins
//============================================
enum eTextMargins { eFULLSHAPE, eSHAPEDEFAULT, eCUSTOM };

//============================================
// Default values
//============================================
#define DEFAULT_FONT_FAMILY                 "Arial"
#define DEFAULT_FONT_SIZE                   12
#define DEFAULT_FONT_COLOR                  "#ffffaa"
#define DEFAULT_FONT_SHADOWCOLOR            "#333333"
#define DEFAULT_FONT_ISBOLD                 false
#define DEFAULT_FONT_ISITALIC               false
#define DEFAULT_FONT_ISUNDERLINE            false
#define DEFAULT_FONT_HALIGN                 1
#define DEFAULT_FONT_VALIGN                 1
#define DEFAULT_FONT_TEXTEFFECT             5

#define DEFAULT_SHAPE_OPACITY               0
#define DEFAULT_SHAPE_BORDERSIZE            0
#define DEFAULT_SHAPE_BORDERCOLOR           "#333333"
#define DEFAULT_SHAPE_SHADOWCOLOR           "#000000"
#define DEFAULT_SHAPE_INTERNALBORDERSIZE    6
#define DEFAULT_SHAPE_INTERNALBORDERCOLOR1  "#808080"
#define DEFAULT_SHAPE_INTERNALBORDERCOLOR2  "#c0c0c0"
#define DEFAULT_SHAPE_BRUSHCOLORD           "#808080"

// helper-struct for Rotations
enum eRotationCenter { 
   eRotCenter, 
   eRotTopLeft, eRotTopMiddle, eRotTopRight,
   eRotCenterLeft, eRotCenterMiddle, eRotCenterRight,
   eRotBottomLeft, eRotBottomMiddle, eRotBottomRight,
   eRotFree
};
struct sRotationInfo
{ 
   Qt::Axis axis;
   eRotationCenter center;
   qreal rotationCenterX, rotationCenterY;
   sRotationInfo(Qt::Axis requiredAxis) {
      axis = requiredAxis;
      center = eRotCenter;
      rotationCenterX = rotationCenterY = 0;
   }
};

//*********************************************************************************************************************************************
// Base object for composition definition
//*********************************************************************************************************************************************

// a Workspace-Rect
// used to propagate a Workspace-Rect to a std QRectf
class WSRect : public QRectF
{
   public:
      WSRect() : QRectF(0,0,1,1){};
      WSRect(QRectF r) : QRectF(r.x(), r.y(), r.width(), r.height()) {}
      void setX(qreal x) { moveLeft(x) ;}
      void setY(qreal y) { moveTop(y) ;}
      void setTopLeft(const QPointF &p) { moveLeft(p.x()); moveTop(p.y()); }
      QRectF applyTo(double w, double h) const
      {
         return QRectF(x()*w, y()*h, width()*w, height()*h);
      }
      QRectF applyTo(QRectF srcRect) const
      {
         return QRectF(srcRect.left() + x()*srcRect.width(), 
                  srcRect.top() + y()*srcRect.height(), 
                  width()*srcRect.width(), 
                  height()*srcRect.height());
      }
      QSizeF appliedSize(double w, double h) const
      {
         return QSizeF(width()*w, height()*h);
      }
      QPointF appliedPos(double w, double h) const
      {
         return QPointF(x()*w, y()*h);
      }
      void setRect(QRectF r)
      {
         *(QRectF*)this = r;
      }
      void setRect(qreal x, qreal y, qreal width, qreal height)
      {
         (*(QRectF*)this).setRect(x, y, width, height);
      }
};

class cCompositionObjectContext : public QObject 
{
   Q_OBJECT
   public:
      QSizeF               size;
      bool                 PreviewMode;
      bool                 AddStartPos;
      int64_t              VideoPosition,StartPosToAdd,ShotDuration;
      cSoundBlockList      *SoundTrackMontage;
      double               BlockPctDone,ImagePctDone;
      cCompositionObject   *PrevCompoObject;
      bool                 UseBrushCache,/*Transfo,*/EnableAnimation;
      //QRectF               xNewRect; not used
      QRectF               curRect;
      double               curRotateZAxis, curRotateXAxis, curRotateYAxis;
      sRotationInfo        ZRotationInfo;
      double               curTxtZoomLevel,curTxtScrollX,curTxtScrollY;
      QRectF               orgRect;
      double               DestOpacity;
      bool                 IsCurrentObject;
      QList<QPolygonF>     PolygonList;
      QRectF               ShapeRect;
      cDiaporamaShot       *CurShot;
      cDiaporamaShot       *PreviousShot;
      cDiaporamaObjectInfo *Info;
      int                  ObjectNumber;
      // mask-parameters
      QRectF maskRect;
      int maskBackgroundForm;
      double maskRotateZAxis; 


      explicit cCompositionObjectContext(int ObjectNumber,bool PreviewMode,bool IsCurrentObject,cDiaporamaObjectInfo *Info, QSizeF size,
                                                   cDiaporamaShot *CurShot,cDiaporamaShot *PreviousShot,cSoundBlockList *SoundTrackMontage,bool AddStartPos,
                                                   int64_t ShotDuration);
      virtual ~cCompositionObjectContext();
      void Compute();
};
typedef QList<cCompositionObjectContext *> CompositionContextList;

//**********************************

class cCompositionObjectMask : /*public QObject, */public xmlReadWrite 
{
   WSRect wsRect;
   double RotateZAxis; // Rotation from Z axis
   bool invertMask;
   public:
      cCompositionObjectMask();
      cCompositionObjectMask(cCompositionObjectMask *src);

      double posX() const { return wsRect.x(); }
      double posY() const { return wsRect.y(); }
      QPointF pos() const { return wsRect.topLeft(); }
      double getX() const { return wsRect.x(); }
      void setX(double d) { wsRect.setX(d); }
      double getY() const { return wsRect.y(); }
      void setY(double d) { wsRect.setY(d); }
      void setPosition(double newx, double newy) { wsRect.setTopLeft(QPointF(newx, newy)); }
      void setPosition(QPointF p) { wsRect.setTopLeft(p); }

      // size of the object
      double height() const { return wsRect.height(); }
      void setHeight(double d) { wsRect.setHeight(d); }
      double width() const { return wsRect.width(); }
      void setWidth(double d) { wsRect.setWidth(d); }
      QSizeF size() const { return wsRect.size(); }
      void setSize(double newHeight,double newWidth) { wsRect.setSize(QSizeF(newHeight, newWidth)); }
      void setSize(QSizeF size) { wsRect.setSize(size); }
      void setRect(QRectF rect) { setPosition(rect.topLeft()); setSize(rect.size()); }
      QRectF getRect() const { return wsRect; }
      QRectF appliedRect(double width, double height) const { return wsRect.applyTo(width, height); }
      QRectF appliedRect(QRectF r) const { return wsRect.applyTo(r); }

      double zAngle() const { return RotateZAxis; }
      void setZAngle(double angle) { RotateZAxis = angle; }
      bool hasRotation();

      bool invertedMask() const { return invertMask; }
      void setInvertMask(bool b) { invertMask = b; }

      void copyFrom(cCompositionObjectMask*srcMask);

      bool sameAs(const cCompositionObjectMask *other) const;
};
typedef QList<cCompositionObjectMask*> maskList;

class cCompositionObject : public QObject, public xmlReadWrite 
{
   Q_OBJECT
   eTypeComposition  TypeComposition;  // Type of composition object (COMPOSITIONTYPE_BACKGROUND, COMPOSITIONTYPE_OBJECT, COMPOSITIONTYPE_SHOT)
   int IndexKey;         
   bool IsVisible;        // True if block is visible during this shot
   bool BlockInheritance; // If true and not first shot then use the same value as in precedent shot
   // Coordinates attributs of the object (Shot values)
   //double x,y,w,h;     // Position (x,y) relative to workspace and size (width,height) in % of workspace-size with 1=100
   WSRect wsRect;
   double RotateZAxis; // Rotation from Z axis
   sRotationInfo        ZRotationInfo;
   double RotateXAxis; // Rotation from X axis
   double RotateYAxis; // Rotation from Y axis

   int BlockSpeedWave;  // Speed wave for block animation
   int BlockAnimType;   // Type of block animation (#define BLOCKANIMTYPE_)
   int TurnZAxis;       // BLOCKANIMTYPE_MULTIPLETURN : Number of turn from Z axis
   int TurnXAxis;       // BLOCKANIMTYPE_MULTIPLETURN : Number of turn from X axis
   int TurnYAxis;       // BLOCKANIMTYPE_MULTIPLETURN : Number of turn from Y axis
   eBlockAnimValue Dissolve;        // BLOCKANIMTYPE_DISSOLVE     : Dissolve value
   //maskList masks;
   cCompositionObjectMask mask;
   bool useMask;
   bool useTextMask;
   bool use3dFrame;

   public:
      bool isShot() { return TypeComposition == eCOMPOSITIONTYPE_SHOT; }
      eTypeComposition getCompositionType() { return TypeComposition; }
      void setCompositionType(eTypeComposition type) { TypeComposition = type; }

      int index() const { return IndexKey; }
      void setIndex(int i) { IndexKey = i; }

      bool isVisible() const { return IsVisible; }
      void setVisible(bool b) { IsVisible = b; }

      bool hasInheritance() const { return BlockInheritance; }
      void setInheritance(bool b) { BlockInheritance = b; }

      // about position of the object
      double posX() const { return wsRect.x(); }
      double posY() const { return wsRect.y(); }
      QPointF pos() const { return wsRect.topLeft(); }
      double getX() const { return wsRect.x(); }
      void setX(double d) { wsRect.setX(d); }
      double getY() const { return wsRect.y(); }
      void setY(double d) { wsRect.setY(d); }
      void setPosition(double newx, double newy) { wsRect.setTopLeft(QPointF(newx, newy)); }
      void setPosition(QPointF p) { wsRect.setTopLeft(p); }

      // size of the object
      double height() const { return wsRect.height(); }
      void setHeight(double d) { wsRect.setHeight(d); }
      double width() const { return wsRect.width(); }
      void setWidth(double d) { wsRect.setWidth(d); }
      QSizeF size() const { return wsRect.size(); }
      void setSize(double newHeight,double newWidth) { wsRect.setSize(QSizeF(newHeight, newWidth)); }
      void setSize(QSizeF size) { wsRect.setSize(size); }
      void setRect(QRectF rect) { setPosition(rect.topLeft()); setSize(rect.size()); }
      QRectF getRect() const { return wsRect; }
      QRectF appliedRect(double width, double height) const { return wsRect.applyTo(width, height); }

      // about rotations
      double xAngle() const { return RotateXAxis; }
      double yAngle() const { return RotateYAxis; }
      double zAngle() const { return RotateZAxis; }
      int zRotationOrigin() const { return ZRotationInfo.center; }
      void setXAngle(double angle) { RotateXAxis = angle; }
      void setYAngle(double angle) { RotateYAxis = angle; }
      void setZAngle(double angle) { RotateZAxis = angle; }
      void setZRotationOrigin(int origin) { ZRotationInfo.center = eRotationCenter(origin); }
      bool hasRotation();

      // Block Animation
      int SpeedWave() const { return BlockSpeedWave; } 
      void setSpeedWave(int i) { BlockSpeedWave = i; }
      int AnimType() const { return BlockAnimType; } 
      void setAnimType(int i) { BlockAnimType = i; }
      int XTurns() const { return TurnXAxis; }
      void setXTurns(int i) { TurnXAxis = i; }
      int YTurns() const { return TurnYAxis; }
      void setYTurns(int i) { TurnYAxis = i; }
      int ZTurns() const { return TurnZAxis; }
      void setZTurns(int i) { TurnZAxis = i; }
      bool hasTurns();

      eBlockAnimValue DissolveType() const { return Dissolve; }       // BLOCKANIMTYPE_DISSOLVE     : Dissolve value
      void setDissolveType(eBlockAnimValue val) { Dissolve = val; }

      // Attribut of the shape part (Global values)
      int getBackgroundForm() const { return BackgroundForm; }       // Type of the form : 0=None, 1=Rectangle, 2=RoundRect, 3=Buble, 4=Ellipse, 5=Triangle UP (Polygon)
      void setBackgroundForm(int i) { BackgroundForm = i; }          // Type of the form : 0=None, 1=Rectangle, 2=RoundRect, 3=Buble, 4=Ellipse, 5=Triangle UP (Polygon)
      cBrushDefinition *BackgroundBrush;                             // Brush of the background of the form
      int  getPenSize() const { return PenSize; }                    // Width of the border of the form
      void setPenSize(int i) { PenSize = i; }                        // Width of the border of the form
      int  getPenStyle() const { return PenStyle; }                  // Style of the pen border of the form
      void setPenStyle(int i) { PenStyle = i; }                      // Style of the pen border of the form
      QString getPenColor() const { return PenColor; }               // Color of the border of the form
      int getFormShadow() const { return FormShadow; }               // 0=none, 1=shadow up-left, 2=shadow up-right, 3=shadow bt-left, 4=shadow bt-right
      int getFormShadowDistance() const { return FormShadowDistance; } // Distance from form to shadow
      QString getFormShadowColor() const { return FormShadowColor; } // Color of the shadow of the form
      int getOpacity() const { return Opacity; }                     // Opacity of the form

      // Attributes of the shape part (Global values)
      int     BackgroundForm;     // Type of the form : 0=None, 1=Rectangle, 2=RoundRect, 3=Buble, 4=Ellipse, 5=Triangle UP (Polygon)
      int     PenSize;            // Width of the border of the form
      int     PenStyle;           // Style of the pen border of the form
      QString PenColor;           // Color of the border of the form
      int     FormShadow;         // 0=none, 1=shadow up-left, 2=shadow up-right, 3=shadow bt-left, 4=shadow bt-right
      int     FormShadowDistance; // Distance from form to shadow
      QString FormShadowColor;    // Color of the shadow of the form
      int     Opacity;            // Opacity of the form
      double  destOpacity() { return (Opacity == 1 ? 0.75 : Opacity == 2 ? 0.50 : Opacity == 3 ? 0.25 : 1.0); }

      // Attribut of the text part (Global values)
      struct textParams {
         QString Text;            // Text of the object
         QString TextClipArtName; // ClipArt name (if text clipart mode)
         QString FontName;        // font name
         int     FontSize;        // font size
         QString FontColor;       // font color
         QString FontShadowColor; // font shadow color
         bool    IsBold;          // true if bold mode
         bool    IsItalic;        // true if Italic mode
         bool    IsUnderline;     // true if Underline mode
         int     HAlign;          // Horizontal alignement : 0=left, 1=center, 2=right, 3=justif
         int     VAlign;          // Vertical alignement : 0=up, 1=center, 2=bottom
         int     StyleText;       // Style : 0=normal, 1=outerline, 2=shadow up-left, 3=shadow up-right, 4=shadow bt-left, 5=shadow bt-right
         eTextMargins TMType;     // Text margins type (0=full shape, 1=shape default, 2=custom)
         QRectF textMargins;
         Qt::AlignmentFlag alignment() { return (HAlign == 0) ? Qt::AlignLeft : (HAlign == 1) ? Qt::AlignHCenter : (HAlign == 2) ? Qt::AlignRight : Qt::AlignJustify; }
         QFont font() { return QFont(FontName, FontSize, IsBold ? QFont::Bold : QFont::Normal, IsItalic ? QFont::StyleItalic : QFont::StyleNormal); }
      };
      struct textShotParams {
         int     TxtZoomLevel;    // Zoom Level for text
         int     TxtScrollX;      // Scrolling X for text
         int     TxtScrollY;      // Scrolling Y for text
      };
      textParams tParams;
      textShotParams tShotParams;
      //QTextDocument *textDocument;
      QString resolvedText;

      // Optimisation flags
      bool IsTextEmpty;
      bool IsFullScreen;
      bool IsStatic;
      bool KenBurnsSlide;
      bool KenBurnsZoom;
      bool fullScreenVideo;

      cApplicationConfig *ApplicationConfig;

      explicit cCompositionObject(eTypeComposition TypeComposition,int IndexKey,cApplicationConfig *TheApplicationConfig,QObject *Parent);
               ~cCompositionObject();

      void InitDefaultValues();
      void CopyBlockPropertiesTo(cCompositionObject *DestBlock);
      void CopyBlockProperties(cCompositionObject *SourceBlock,cCompositionObject *DestBlock);
      void CopyFromCompositionObject(cCompositionObject *CompositionObjectToCopy);
      void DrawCompositionObject(cDiaporamaObject *Object,QPainter *Painter, double  ADJUST_RATIO, QSizeF size, bool PreviewMode, int64_t Position,
            cSoundBlockList *SoundTrackMontage, double BlockPctDone, double ImagePctDone, cCompositionObject *PreviousCompositionObject,
            int64_t ShotDuration, bool EnableAnimation,
            bool Transfo=false, QRectF NewRect = QRectF(),
            bool DisplayTextMargin=false, cCompositionObjectContext *PreparedBrush=NULL);
      void DrawCompositionObjectText(cDiaporamaObject *Object, QPainter *Painter, double  ADJUST_RATIO, QSizeF size, bool PreviewMode, int64_t Position,
         cSoundBlockList *SoundTrackMontage, double BlockPctDone, double ImagePctDone, cCompositionObject *PreviousCompositionObject,
         int64_t ShotDuration, bool EnableAnimation,
         bool Transfo = false, QRectF NewRect = QRectF(),
         bool DisplayTextMargin = false, cCompositionObjectContext *PreparedBrush = NULL);
      void SaveToXML(QDomElement &domDocument,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,bool CheckTypeComposition,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool SaveBrush,bool IsModel);
      void SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, bool CheckTypeComposition, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool SaveBrush, bool IsModel);
      bool LoadFromXML(QDomElement domDocument,QString ElementName,QString PathForRelativPath,cCompositionList *ObjectComposition,QStringList *AliasList,bool CheckTypeComposition,TResKeyList *ResKeyList,bool DuplicateRes,bool RestoreBrush,cCompositionObject *GlobalBlock=NULL);

      QRectF GetTextMargin(QRectF Workspace,double  ADJUST_RATIO);
      void   ApplyTextMargin(int TMType);

      // Style managment functions
      int  GetAutoCompoSize(int ffDProjectGeometry);
      void ApplyAutoCompoSize(int AutoCompoStyle,int ffDProjectGeometry,bool AllowMove=true);
      void getMovementFactors(int AutoCompoStyle,int ffDProjectGeometry, qreal *xFactor, qreal *yFactor);

      // never used:
      //QString GetCoordinateStyle();
      //void    ApplyCoordinateStyle(QString StyleDef);

      QString GetTextStyle();
      void    ApplyTextStyle(QString StyleDef);

      QString GetBackgroundStyle();
      void    ApplyBackgroundStyle(QString StyleDef);

      QString GetBlockShapeStyle();
      void    ApplyBlockShapeStyle(QString StyleDef);

      void ComputeOptimisationFlags(const cCompositionObject *Previous);

      bool sameAs(const cCompositionObject* prevCompositionObject) const;
      bool isMoving(const cCompositionObject* prevCompositionObject) const;
      bool isOnScreen(const cCompositionObject* prevCompositionObject) const;

      bool isVideo();
      bool hasFilter();
      void preLoad(bool bPreview);

      cCompositionObjectMask *getMask() { return &mask; }
      const cCompositionObjectMask *getMask() const { return &mask; }
      //void setMask(cCompositionObjectMask *m) { delete mask; mask = m; useMask = (m != 0); }
      bool usingMask() const  { return useMask; }
      void setUseMask(bool b) { useMask = b; }
      bool using3DFrame() { return use3dFrame; }
      void setUse3DFrame(bool b) { use3dFrame = b; }

      QPointF getRotationOrigin(const QRectF &shapeRect, eRotationCenter rtc);
      private:
      QRectF  GetPrivateTextMargin();
};
typedef QList<cCompositionObject*> CompositionObjectList;

QRectF appliedRect(const QRectF &r, double width, double height);

/*
class cTextCompositionObject : public cCompositionObject
{
public:
   // Attribut of the text part (Global values)
   QString Text;            // Text of the object
   QString TextClipArtName; // ClipArt name (if text clipart mode)
   QString FontName;        // font name
   int     FontSize;        // font size
   QString FontColor;       // font color
   QString FontShadowColor; // font shadow color
   bool    IsBold;          // true if bold mode
   bool    IsItalic;        // true if Italic mode
   bool    IsUnderline;     // true if Underline mode
   int     HAlign;          // Horizontal alignement : 0=left, 1=center, 2=right, 3=justif
   int     VAlign;          // Vertical alignement : 0=up, 1=center, 2=bottom
   int     StyleText;       // Style : 0=normal, 1=outerline, 2=shadow up-left, 3=shadow up-right, 4=shadow bt-left, 5=shadow bt-right
   int     TxtZoomLevel;    // Zoom Level for text
   int     TxtScrollX;      // Scrolling X for text
   int     TxtScrollY;      // Scrolling Y for text
   int     TMType;          // Text margins type (0=full shape, 1=shape default, 2=custom)
   double  TMx, TMy, TMw, TMh; // Text margins
};
*/

//*********************************************************************************************************************************************
// Global class containing composition list
//*********************************************************************************************************************************************

class cCompositionList : public QObject, public xmlReadWrite
{
   Q_OBJECT
   public:
      eTypeComposition TypeComposition;            // Type of composition list
      CompositionObjectList  compositionObjects;   // list of cCompositionObject

      explicit cCompositionList(QObject *Parent);
      ~cCompositionList();

      void SaveToXML(QDomElement &domDocument,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
      void SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel);
      bool LoadFromXML(QDomElement domDocument,QString ElementName,QString PathForRelativPath,cCompositionList *ObjectComposition,QStringList *AliasList,cApplicationConfig *ApplicationConfig,TResKeyList *ResKeyList,bool DuplicateRes);
      cCompositionObject *getCompositionObject(int IndexKey) const;  // get the cCompositionObject with the given Indexkey or NULL
      void movingsOnTop(const cCompositionList* prevCompositions);
      void applyZOrder(const cCompositionList* prevCompositions);
};

//*********************************************************************************************************************************************
// Base object for scene definition
//*********************************************************************************************************************************************
class cDiaporamaShot : public QObject, public xmlReadWrite
{
   Q_OBJECT
   public:
      cDiaporamaObject  *pDiaporamaObject;
      int64_t           StaticDuration;         // Duration (in msec) of the static part animation
      cCompositionList  ShotComposition;        // Shot Composition object list

      explicit cDiaporamaShot(cDiaporamaObject *Parent);
      ~cDiaporamaShot();
               
      void SaveToXML(QDomElement &domDocument,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,bool LimitedInfo,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool IsModel);
      void SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, bool LimitedInfo, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel);
      bool LoadFromXML(QDomElement domDocument,QString ElementName,QString PathForRelativPath,cCompositionList *ObjectComposition,QStringList *AliasList,TResKeyList *ResKeyList,bool DuplicateRes);
      void movingsOnTop(const cDiaporamaShot* prevShot);
      int fullScreenVideoIndex;

};

class cMusicList
{
   bool bDontDelete;
   public:
   QList<cMusicObject*> MusicObjects; // List of music definition
   //QList<cSoundBlockList> SoundBlockList;

      cMusicList() {
         bDontDelete = false;
      }
      cMusicList(bool dontDelete) {
         bDontDelete = dontDelete;
      }
      ~cMusicList();
      int count() { return MusicObjects.count(); }
      cMusicObject *MusicObjectAt(int index) { return MusicObjects.at(index); }
      void clear() { MusicObjects.clear(); }
      void insert(int CurIndex, cMusicObject *obj) { MusicObjects.insert(CurIndex, obj); }
      void removeAt(int index) { MusicObjects.removeAt(index); }
      void swap(int index1, int index2) { MusicObjects.QT_SWAP(index1, index2); }
      bool isEmpty() { return MusicObjects.isEmpty(); }
      bool findMusicObject(int64_t position, cMusicObject **pMusicObject, int64_t *startPosition, int *MusicListIndex);
      void resetSoundBlocks();
};

//*********************************************************************************************************************************************
// class containing one slide
//*********************************************************************************************************************************************
class cDiaporamaObject : public QObject, public xmlReadWrite 
{
Q_OBJECT
public:
    cDiaporama *pDiaporama;               // Link to global object
    QString SlideName;                    // Display name of the slide
    cCompositionList ObjectComposition;   // Composition object list
    QList<cDiaporamaShot *> shotList;     // list of scene definition

    // Chapter definition
    bool    StartNewChapter;            // if true then start a new chapter from this slide
    QString ChapterName;                // Chapter name
    bool    OverrideProjectEventDate;   // if true then chapter date is different from project date
    QDate   ChapterEventDate;           // Chapter event date (if OverrideProjectEventDate is true)
    bool    OverrideChapterLongDate;    // if true then chapter long date is different from project long date
    QString ChapterLongDate;            // Chapter long date (if OverrideChapterLongDate is true)
    void    *ChapterLocation;           // Chapter location (NULL if same as project)

    // Background definition
    bool BackgroundType;               // Background type : false=same as precedent - true=new background definition
    cBrushDefinition *BackgroundBrush; // Background brush

    // Music definition
    bool MusicType;                 // Music type : false=same as precedent - true=new playlist definition
    bool MusicPause;                // true if music is pause during this object
    bool MusicReduceVolume;         // true if volume if reduce by MusicReduceFactor
    double MusicReduceFactor;       // factor for volume reduction if MusicReduceVolume is true
    //QList<cMusicObject*> MusicList; // List of music definition
    cMusicList MusicList;

    // Transition
    TRFAMILY  TransitionFamily;    // Transition family
    int       TransitionSubType;   // Transition type in the family
    int64_t   TransitionDuration;  // Transition duration (in msec)
    int       TransitionSpeedWave; // Transition SpeedWave

    qlonglong ThumbnailKey;               // Thumbnail key in the database

    // Object definition
    int NextIndexKey;               // Next index key for cCompositionList (incremental value)

    // Cached data to improve interface speed
    int64_t CachedDuration;             // Real duration of the slide
    int64_t CachedTransitDuration;      // Real duration of the transition of slide
    int64_t CachedStartPosition;        // Start position of the music
    int     CachedMusicIndex;           // Index of slide owner of the music
    int     CachedMusicListIndex;       // Index of track in music table of slide owner of the music
    int     CachedBackgroundIndex;      // Index of slide owner of the background
    bool    CachedHaveFilter;           // True if object in slide have at least one filter
    bool    CachedHaveSound;            // True if object in slide have sound
    double  CachedSoundVolume;          // Max volume in the slide
    bool    CachedMusicFadIN;           // if true fad-in to music during entering transition
    bool    CachedMusicEnd;             // if true then music end during slide
    bool    CachedPrevMusicFadOUT;      // if true then fad-out to previous music during entering transition
    bool    CachedPrevMusicEnd;         // if true then previous music end during entering transition
    int64_t CachedMusicRemaining;       // time left for music after this slide
    int64_t CachedMusicTimePlayed;      // time played during this slide
    int64_t CachedMusicListOffset;


    explicit cDiaporamaObject(cDiaporama *Parent);
    ~cDiaporamaObject();

    void    InitDefaultValues();
    QString GetDisplayName();
    int64_t GetCumulTransitDuration();
    int64_t GetDuration();
    void    DrawThumbnail(int ThumbWidth,int ThumbHeight,QPainter *Painter,int AddX,int AddY,int ShotNumber=0);   // Draw Thumb @ position 0
    void    SaveToXML(QDomElement &domDocument,QString ElementName,QString PathForRelativPath,bool ForceAbsolutPath,cReplaceObjectList *ReplaceList,QList<qlonglong> *ResKeyList,bool SaveThumbAllowed);
    void    SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool SaveThumbAllowed);
    bool    LoadFromXML(QDomElement domDocument,QString ElementName,QString PathForRelativPath,QStringList *AliasList,TResKeyList *ResKeyList,bool DuplicateRes,DlgWorkingTask *DlgWorkingTaskDialog=NULL);
    int64_t GetTransitDuration();
    int     GetSpeedWave();
    int     ComputeChapterNumber(cDiaporamaObject **Object=NULL);
    int     GetSlideNumber();
    int     GetAutoTSNumber();
    void setDefaultTransition();

    void ComputeOptimisationFlags();
    // Models part
    void    LoadModelFromXMLData(ffd_MODELTYPE TypeModel,QDomDocument domDocument,TResKeyList *ResKeyList,bool DuplicateRes);
    bool    SaveModelFile(ffd_MODELTYPE TypeModel,QString ModelFileName);
    QString SaveAsNewCustomModelFile(ffd_MODELTYPE TypeModel);

    // Thread functions
    void ThreadedLoadThumb(QDomElement Element);

    cDiaporama *getDiaporama() { return pDiaporama; }
    void releaseCachObjects();
    void preLoadItems(bool bPreview);
    void movingsOnTop(int shotNumber);
};

//*********************************************************************************************************************************************
// Technical class object for rendering
//*********************************************************************************************************************************************
class cDiaporamaObjectInfo 
{
public:
    //=====> All objects
    QImage RenderedImage;                          // Final image rendered
    AVFrame *YUVFrame;
    double FrameDuration;                          // Duration of a frame (in msec)
    bool IsShotStatic;
    bool IsTransitStatic;
    bool hasYUV;

    struct ObjectInfo
    {
       int                 Object_Number;                   // Object number
       int64_t             Object_StartTime;                // Position (in msec) of the first frame relative to the diaporama
       int64_t             Object_InObjectTime;             // Position (in msec) in the object
       cDiaporamaObject    *Object;                         // Link to the current object
       int                 Object_ShotSequenceNumber;       // Number of the shot sequence in the current object
       cDiaporamaShot      *Object_CurrentShot;             // Link to the current shot in the current object
       eShotType           Object_CurrentShotType;          // Type of the current shot : Static/Mobil/Video
       int64_t             Object_ShotDuration;             // Time the static shot end (if CurrentObject_CurrentShotType=SHOTTYPE_STATIC)
       double              Object_PCTDone;                  // PCT achevement for static shot
       int                 Object_BackgroundIndex;          // Object number containing current background definition
       QBrush              *Object_BackgroundBrush;         // Current background brush
       bool                Object_FreeBackgroundBrush;      // True if allow to delete CurrentObject_BackgroundBrush during destructor
       QImage              Object_PreparedBackground;       // Current image produce for background
       cSoundBlockList     *Object_SoundTrackMontage;       // Sound for playing sound from montage track
       bool                Object_FreeSoundTrackMontage;    // True if allow to delete CurrentObject_SoundTrackMontage during destructor
       QImage              Object_PreparedImage;            // Current image prepared
       cSoundBlockList     *Object_MusicTrack;              // Sound for playing music from music track
       bool                Object_FreeMusicTrack;           // True if allow to delete CurrentObject_MusicTrack during destructor
       cMusicObject        *Object_MusicObject;             // Ref to the current playing music

       ~ObjectInfo();
       void init();
       struct ObjectInfo &operator=(const ObjectInfo&);
       void dump();
    };
    ObjectInfo Current;

    //=====> Transitionnal object
    bool      IsTransition;       // True if transition in progress
    double    TransitionPCTDone;  // PCT achevement for transition
    TRFAMILY  TransitionFamily;   // Transition family
    int       TransitionSubType;  // Transition type in the family
    int64_t   TransitionDuration; // Transition duration (in msec)
    ObjectInfo Transit;

    cDiaporamaObjectInfo();
    cDiaporamaObjectInfo(cDiaporamaObjectInfo *PreviousFrame, int64_t TimePosition, cDiaporama *Diaporama, double FrameDuration, bool WantSound);
    ~cDiaporamaObjectInfo();
    void init();
    void Copy(cDiaporamaObjectInfo *PreviousFrame);
    bool ComputeIsShotStatic(cDiaporamaObject *Object,int ShotNumber);
    //bool canUseYUVPassthrough();
    void ensureAudioTracksReady(double WantedDuration, int Channels, int64_t SamplingRate, enum AVSampleFormat SampleFormat);
};

//*********************************************************************************************************************************************
// Global class containing the project
//*********************************************************************************************************************************************
class cDiaporama : public QObject, public xmlReadWrite 
{
      Q_OBJECT
      cApplicationConfig  *applicationConfig;
      cffDProjectFile     *projectInfo;
   public:


      QString                 ThumbnailName;
      cDiaporamaObject        *ProjectThumbnail;

      int     CurrentCol;             // Current position in the timeline (column)
      int64_t CurrentPosition;        // Current position in the timeline (msec)

      int     CurrentChapter;
      QString CurrentChapterName;

      bool    IsModify;               // true if project was modify
      QString ProjectFileName;        // Path and name of current file project

      // Output rendering values
      ffd_GEOMETRY ImageGeometry;     // Project image geometry for image rendering
      int          InternalWidth;     // Real width for image rendering
      int          InternalHeight;    // Real height for image rendering

      // Speed wave
      int TransitionSpeedWave;    // Speed wave for transition
      int BlockAnimSpeedWave;     // Speed wave for block animation
      int ImageAnimSpeedWave;     // Speed wave for image framing and correction animation

      // slides objects
      QList<cDiaporamaObject *> slides;                   // list of all media object
      cMusicList masterSound;

      explicit cDiaporama(cApplicationConfig *ApplicationConfig,bool LoadDefaultModel,QObject *Parent);
      ~cDiaporama();

      cApplicationConfig  *ApplicationConfig() { return applicationConfig; }
      cffDProjectFile     *ProjectInfo() { return projectInfo; }

      int          GetHeightForWidth(int WantedWith);
      int          GetWidthForHeight(int WantedHeight);
      int          GetObjectIndex(cDiaporamaObject *ObjectToFind);
      int64_t      GetDuration();
      int64_t      GetPartialDuration(int from,int to);
      int64_t      GetObjectStartPosition(int index);
      int64_t      GetTransitionDuration(int index);
      void         UpdateCachedInformation();
      void         PrepareBackground(int ObjectIndex, QSize size, QPainter *Painter, int AddX, int AddY);
      cMusicObject *GetMusicObject(int ObjectIndex,int64_t &StartPosition,int *CountObject=NULL,int *IndexObject=NULL);
      void         DefineSizeAndGeometry(ffd_GEOMETRY Geometry);                        // Init size and geometry
      bool         SaveFile(QWidget *ParentWindow,cReplaceObjectList *ReplaceList=NULL,QString *ExportFileName=NULL);

      void         UpdateInformation();
      void         UpdateChapterInformation();
      void         UpdateStatInformation();
      cDiaporamaObject *GetChapterDefObject(cDiaporamaObject *Object);
      cDiaporamaObject* insertNewSlide(int index);
      // Thread functions
      struct PrepareMusicBlocContext {
         bool            PreviewMode;
         int             Column;
         int64_t         Position;
         cSoundBlockList *MusicTrack;
         int             NbrDuration;
         bool            FadIn;
         bool            FadOut;
         bool            IsCurrent;
         int64_t         PositionInSlide;
         // for seamless-music:
         int64_t slideStartpos;
      };

      void PrepareMusicBloc(PrepareMusicBlocContext *Context);
      void resetSoundBlocks();
      void LoadSources(cDiaporamaObjectInfo *Info, QSize size, bool PreviewMode, bool AddStartPos, CompositionContextList &PreparedTransitBrushList, CompositionContextList &PreparedBrushList, int NbrDuration=2);
      void getBackgroundBrush(cDiaporamaObjectInfo::ObjectInfo& objInfo, QSize size, bool PreviewMode);
      void LoadSourcesYUV(cDiaporamaObjectInfo *Info, QSize size, bool PreviewMode, bool AddStartPos, CompositionContextList &PreparedTransitBrushList, CompositionContextList &PreparedBrushList, int NbrDuration = 2);
      void DoAssembly(double PCT,cDiaporamaObjectInfo *Info, QSize size,QImage::Format QTFMT=QImage::Format_ARGB32_Premultiplied);

      // Memory
      void CloseUnusedLibAv(int CurrentSlide, bool backwardOnly = false);
      void clearResolvedTexts();
      void clearThumbnailsWithVars();

      void CreateObjectContextList(cDiaporamaObjectInfo *Info, QSize size,bool IsCurrentObject,bool PreviewMode,bool AddStartPos,CompositionContextList &PreparedBrushList);
      void PrepareImage(cDiaporamaObjectInfo *Info, QSize size, bool IsCurrentObject, bool AddStartPos, CompositionContextList &PreparedBrushList);
      void PrepareImageYUV(cDiaporamaObjectInfo *Info, QSize size, bool IsCurrentObject, bool AddStartPos, CompositionContextList &PreparedBrushList);
      void PreLoadItems(int iStartSlide, int iEndSlide, QSemaphore *numSlides, volatile bool *stopPreloading);
      void PreLoadRenderItems(int iStartSlide, int iEndSlide, volatile bool *stopPreloading);
      void PreLoadPreviewItems(int iStartSlide, int iEndSlide, volatile bool *stopPreloading);

      void UpdateGMapsObject(bool ProposeAll=false);

      signals:
         void slideCached(int);
         void cachingEnd();
};

#define deleteList(l) while(!l.isEmpty() ) delete l.takeLast()
#endif // CDIAPORAMA_H
