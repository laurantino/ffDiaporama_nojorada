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

#ifndef CINTERACTIVEZONE_H
#define CINTERACTIVEZONE_H

#include "cCustomBlockTable.h"

#include <QWidget>


struct SortBlock {
   int   Index;
   qreal Position;
   SortBlock(int index, qreal position)
   {
      Index = index;
      Position = position;
   }
};
//SortBlock MakeSortBlock(int Index,qreal Position);

class cInteractiveZone : public QWidget 
{
   Q_OBJECT
    enum TRANSFOTYPE {NOTYETDEFINED,MOVEBLOCK,RESIZEDOWNLEFT,RESIZEUPLEFT,RESIZELEFT,RESIZEUPRIGHT,RESIZERIGHT,RESIZEDOWNRIGHT,RESIZEUP,RESIZEDOWN};
   struct sDualQReal {
      qreal   Screen;
      qreal   Block;
      sDualQReal() {}
      sDualQReal(qreal s, qreal b) { Screen = s; Block = b; }
   };
   cCompositionObject** pCurrentCompoObject;        // Current block object (if selection mode = SELECTMODE_ONE)
   int *pCurrentCompoObjectNbr;                     // Number of Current block object (if selection mode = SELECTMODE_ONE)

public:
   enum DISPLAYMODE {DisplayMode_BlockShape,DisplayMode_TextMargin} DisplayMode;
   enum eWheelMode { eMoveHor, eMoveVer, eResize, eRotateZ, eRotateX, eRotateY, eZoomInOut, eSlideVertical, eSlideHorizontal };
   eWheelMode wheelMode;

   double            DisplayW,DisplayH;              // wanted size
   int               MagneticRuler;                  // rulers
   cCustomBlockTable *BlockTable;
   cDiaporamaObject  *DiaporamaObject;
   int               CurrentShotNbr;                 // Current shot number
   QImage            *BackgroundImage;
   QImage            *ForegroundImage;
   QRectF            SceneRect,ScreenRect;
   QRectF            FullSelRect,CurSelRect,CurSelScreenRect;
   bool              IsCapture;                      // True if there is an active capture
   QPoint            CapturePos;
   QList<bool>       IsSelected;
   int               NbrSelected;
   bool              LockGeometry;
   bool              IsRefreshQueued;

   // Selection rectangle properties
   qreal               AspectRatio;                    // Aspect ratio of the (multi)selection rectangle
   qreal               Sel_X,Sel_Y,Sel_W,Sel_H;        // Position and size of the current (multi)selection rectangle
   qreal               RSel_X,RSel_Y,RSel_W,RSel_H;    // Real position and size of the current (multi)selection rectangle - without shape adjustement

   // Transformations
   TRANSFOTYPE         TransfoType;
   qreal               Move_X,Move_Y;                  // Blocks move
   qreal               Scale_X,Scale_Y;                // Blocks resize

   explicit cInteractiveZone(QWidget *parent = 0);
   ~cInteractiveZone();

   void setGeometry(const QRect &r);
   void setGeometry(int x, int y, int w, int h);

   void setWheelMode(int i) { wheelMode = eWheelMode(i); } 
   void setObjectPointers(cCompositionObject** CurrentCompoObject, int* CurrentCompoObjectNbr)
   {
      pCurrentCompoObject = CurrentCompoObject;
      pCurrentCompoObjectNbr = CurrentCompoObjectNbr;
   }

protected:
   virtual void  paintEvent(QPaintEvent *event);

   virtual void mouseDoubleClickEvent(QMouseEvent *event);
   virtual void mouseMoveEvent(QMouseEvent *event);
   virtual void mousePressEvent(QMouseEvent *event);
   virtual void mouseReleaseEvent(QMouseEvent *event);
   virtual void keyPressEvent(QKeyEvent *event);
   virtual void keyReleaseEvent(QKeyEvent *event);
   virtual void resizeEvent(QResizeEvent *);
   virtual void wheelEvent(QWheelEvent * event);

signals:
   void StartSelectionChange();
   void EndSelectionChange();
   void RightClickEvent(QMouseEvent *ev);
   void DoubleClickEvent(QMouseEvent *ev);
   void TransformBlock(qreal Move_X,qreal Move_Y,qreal Scale_X,qreal Scale_Y,qreal Sel_X,qreal Sel_Y,qreal Sel_W,qreal Sel_H);
   void DisplayTransformBlock(qreal Move_X,qreal Move_Y,qreal Scale_X,qreal Scale_Y,qreal Sel_X,qreal Sel_Y,qreal Sel_W,qreal Sel_H);
   void TransformBlocks(qreal DeltaX,qreal DeltaY,qreal DeltaW,qreal DeltaH, QRectF selRect);
   void RotateBlocks(qreal xAngle, qreal yAngle, qreal zAngle);
   void ZoomSlide(qreal zoomDiff, qreal slideX, qreal slideY);

   public slots:
      void DifferedEmitRightClickEvent();
      void DifferedEmitDoubleClickEvent();
      void RefreshDisplay();

private:
   bool IsInRect(QPointF Pos,QRectF Rect);
   bool IsInSelectedRect(QPointF Pos);
   bool isInGap(qreal pos, qreal gapCenter, qreal gapSize);
   void GetForDisplayUnit(double &DisplayW,double &DisplayH);
   void UpdateIsSelected();
   void ManageCursor(QPointF Pos,Qt::KeyboardModifiers Modifiers);
   void ManageCursor(QPointF Pos,QVector<QRect>,Qt::KeyboardModifiers Modifiers);
   void ComputeRulers(QList<sDualQReal> &MagnetVert,QList<sDualQReal> &MagnetHoriz);
   QRectF ComputeNewCurSelRect();
   QRectF ComputeNewCurSelScreenRect();
   QRectF ApplyModifAndScaleFactors(int Block,QRectF Ref,bool ApplyShape);
   QRectF ApplyModifAndScaleFactors(cCompositionObject* compObj,QRectF Ref,bool ApplyShape, bool isSelected);
   void   DrawSelect(QPainter &Painter,QRectF Rect,bool WithHandles);
   QVector<QRect> createHandleRects(QRectF r);
   bool snap(qreal pos, qreal mag, int range);
};

#endif // CINTERACTIVEZONE_H
