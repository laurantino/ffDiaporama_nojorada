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

#include "cInteractiveZone.h"

#define HANDLESIZEX     8
#define HANDLESIZEX_2   (HANDLESIZEX/2)
#define HANDLESIZEY     8
#define HANDLESIZEY_2   (HANDLESIZEY/2)
#define HANDLEMAGNETX   14
#define HANDLEMAGNETY   10

#define RULER_HORIZ_SCREENBORDER    0x0001
#define RULER_HORIZ_TVMARGIN        0x0002
#define RULER_HORIZ_SCREENCENTER    0x0004
#define RULER_HORIZ_UNSELECTED      0x0008
#define RULER_VERT_SCREENBORDER     0x0010
#define RULER_VERT_TVMARGIN         0x0020
#define RULER_VERT_SCREENCENTER     0x0040
#define RULER_VERT_UNSELECTED       0x0080

#define MINVALUE                    0.002       // Never less than this value for width or height

//====================================================================================================================

cInteractiveZone::cInteractiveZone(QWidget *parent) : QWidget(parent)
{
   pCurrentCompoObject = 0;
   pCurrentCompoObjectNbr = 0;
   //ToLog(LOGMSG_DEBUGTRACE, "IN:cInteractiveZone::cInteractiveZone");
   BlockTable = NULL;
   DiaporamaObject = NULL;
   BackgroundImage = NULL;
   ForegroundImage = NULL;
   IsCapture = false;
   TransfoType = NOTYETDEFINED;
   setMouseTracking(true);
   AspectRatio = 1;
   Sel_X = 0;
   Sel_Y = 0;
   Sel_W = 0;
   Sel_H = 0;
   RSel_X = 0;
   RSel_Y = 0;
   RSel_W = 0;
   RSel_H = 0;
   Move_X = 0;
   Move_Y = 0;
   Scale_X = 0;
   Scale_Y = 0;
   CurrentShotNbr = 0;
   DisplayMode = DisplayMode_BlockShape;
   IsRefreshQueued = false;
   wheelMode = eMoveHor;
}

//====================================================================================================================

cInteractiveZone::~cInteractiveZone()
{
   //ToLog(LOGMSG_DEBUGTRACE, "IN:cInteractiveZone::~cInteractiveZone");
   if (BackgroundImage != NULL)
   {
      delete BackgroundImage;
      BackgroundImage = NULL;
   }
   if (ForegroundImage != NULL)
   {
      delete ForegroundImage;
      ForegroundImage = NULL;
   }
}

//====================================================================================================================

void cInteractiveZone::GetForDisplayUnit(double &DisplayW, double &DisplayH)
{
   //ToLog(LOGMSG_DEBUGTRACE, "IN:cInteractiveZone::GetForDisplayUnit");

   if (DiaporamaObject->pDiaporama->ImageGeometry == GEOMETRY_4_3) { DisplayW = 1440; DisplayH = 1080; }
   else if (DiaporamaObject->pDiaporama->ImageGeometry == GEOMETRY_16_9) { DisplayW = 1920; DisplayH = 1080; }
   else if (DiaporamaObject->pDiaporama->ImageGeometry == GEOMETRY_40_17) { DisplayW = 1920; DisplayH = 816; }
   else { DisplayW = 0; DisplayH = 0; }
}

//====================================================================================================================
void cInteractiveZone::DifferedEmitRightClickEvent() 
{
   emit RightClickEvent(NULL);
}

//====================================================================================================================
void cInteractiveZone::DifferedEmitDoubleClickEvent() 
{
   emit DoubleClickEvent(NULL);
}

//====================================================================================================================
QRectF cInteractiveZone::ApplyModifAndScaleFactors(int Block, QRectF Ref, bool ApplyShape)
{
   cCompositionObject* compObj = BlockTable->CompositionList->compositionObjects.at(Block);
   return ApplyModifAndScaleFactors(compObj, Ref, ApplyShape, IsSelected.at(Block));
}

QRectF cInteractiveZone::ApplyModifAndScaleFactors(cCompositionObject* compObj, QRectF Ref, bool ApplyShape, bool isSelected)
{
   qreal  NewX = compObj->posX();
   qreal  NewY = compObj->posY();
   qreal  NewW = compObj->width();
   qreal  NewH = compObj->height();
   qreal  RatioScale_X = (RSel_W + Scale_X) / RSel_W;
   qreal  RatioScale_Y = (RSel_H + Scale_Y) / RSel_H;
   QList<QPolygonF> Pol = ComputePolygon(compObj->BackgroundForm, NewX*Ref.width(), NewY*Ref.height(), NewW*Ref.width(), NewH*Ref.height());
   QPolygonF PolU(Pol.at(0));
   for (int i = 1; i < Pol.count(); i++)
      PolU = PolU.united(Pol.at(i));
   QRectF tmpRect = PolU.boundingRect();

   qreal  Decal_X = (tmpRect.left() - NewX*Ref.width()) / Ref.width();
   qreal  Decal_Y = (tmpRect.top() - NewY*Ref.height()) / Ref.height();
   qreal  Ratio_X = (NewW*Ref.width()) / tmpRect.width();
   qreal  Ratio_Y = (NewH*Ref.height()) / tmpRect.height();

   if (IsCapture && TransfoType != NOTYETDEFINED && compObj->isVisible() && isSelected)
   {
      NewX = RSel_X + Move_X + (NewX - RSel_X)*RatioScale_X;
      NewY = RSel_Y + Move_Y + (NewY - RSel_Y)*RatioScale_Y;
      NewW = NewW*RatioScale_X;
      if (NewW < 0.002)
         NewW = 0.002;
      if (compObj->BackgroundBrush->LockGeometry())
         NewH = ((NewW*Ref.width()) * compObj->BackgroundBrush->AspectRatio()) / Ref.height();
      else
         NewH = NewH * RatioScale_Y;
      if (NewH < 0.002)
         NewH = 0.002;
      if (ApplyShape)
      {
         NewX = NewX + Decal_X*RatioScale_X;
         NewY = NewY + Decal_Y*RatioScale_Y;
      }
   }
   else if (isSelected)
   {
      NewX = NewX + Decal_X;
      NewY = NewY + Decal_Y;
   }

   if (ApplyShape)
   {
      NewW = NewW / Ratio_X;
      NewH = NewH / Ratio_Y;
   }
   return QRectF(NewX*Ref.width(), NewY*Ref.height(), NewW*Ref.width(), NewH*Ref.height());
}

//====================================================================================================================

void cInteractiveZone::RefreshDisplay()
{
   //ToLog(LOGMSG_DEBUGTRACE, "IN:cInteractiveZone::RefreshDisplay");
   if ((!BlockTable) || (!BlockTable->CompositionList)) return;

   IsRefreshQueued = false;
   if (BlockTable->updatesEnabled())
   {
      ScreenRect = QRectF(0, 0, DisplayW, DisplayH);
      //qDebug() << "ScreenRect " << ScreenRect;
      // Prepare BackgroundImage if not exist
      if (!BackgroundImage)
      {
         QPainter Painter;
         BackgroundImage = new QImage(DisplayW, DisplayH, QImage::Format_ARGB32_Premultiplied);
         Painter.begin(BackgroundImage);
         DiaporamaObject->pDiaporama->PrepareBackground(DiaporamaObject->pDiaporama->GetObjectIndex(DiaporamaObject), QSize(DisplayW, DisplayH), &Painter, 0, 0);
         Painter.end();
      }

      if (ForegroundImage != NULL)
      {
         delete ForegroundImage;
         ForegroundImage = NULL;
      }

      ForegroundImage = new QImage(BackgroundImage->scaled(QSize(this->width() + 2, this->height() + 2), Qt::KeepAspectRatio, Qt::SmoothTransformation));
      //qDebug() << "ForegroundImage " << ForegroundImage->width() << "x" << ForegroundImage->height();
      //qDebug() << "this " << this->width() << "x" << this->height();
      SceneRect = QRect((this->width() + 2 - ForegroundImage->width()) / 2, (this->height() + 2 - ForegroundImage->height()) / 2, ForegroundImage->width(), ForegroundImage->height());
      //qDebug() << "SceneRect " <<SceneRect;

      UpdateIsSelected();

      // Draw image of the scene under the background
      QPainter P;
      P.begin(ForegroundImage);
      P.save();

      int StartVideoPos = 0;
      int i = 0;
      //for (int i = 0; i < BlockTable->CompositionList->List.count(); i++) 
      foreach(cCompositionObject* compObj, BlockTable->CompositionList->compositionObjects)
      {
         if (compObj->isVisible())
         {
            // If it's a video block, calc video position
            if (compObj->isVideo())
            {
               StartVideoPos = QTime(0, 0).msecsTo(((cVideoFile *)compObj->BackgroundBrush->MediaObject)->StartTime);
               for (int k = 0; k < CurrentShotNbr; k++)
               {
                  cCompositionObject* prevObj = DiaporamaObject->shotList[k]->ShotComposition.getCompositionObject(compObj->index());
                  if( prevObj && prevObj->isVisible() )
                     StartVideoPos += DiaporamaObject->shotList[k]->StaticDuration;
               }
            }
            else
               StartVideoPos = 0;

            QRectF NewRect = ApplyModifAndScaleFactors(i, SceneRect, false);
            compObj->DrawCompositionObject(DiaporamaObject, &P, double(ForegroundImage->height()) / double(1080),ForegroundImage->size(), true, StartVideoPos,
               NULL, 1, 1, NULL, DiaporamaObject->shotList[CurrentShotNbr]->StaticDuration, false,
               (IsCapture) && (TransfoType != NOTYETDEFINED), 
               QRectF( NewRect.left() / SceneRect.width(), NewRect.top() / SceneRect.height(), NewRect.width() / SceneRect.width(), NewRect.height() / SceneRect.height()),
               (DisplayMode == DisplayMode_TextMargin) && (compObj->isVisible()) && (IsSelected[i]));
         }
         i++;
      }
      P.restore();
      P.end();
      repaint();
   }
}

//====================================================================================================================

void cInteractiveZone::paintEvent(QPaintEvent *)
{
   if (!ForegroundImage)
      return;

   QPainter Painter(this);
   Painter.save();
   Painter.translate(SceneRect.left(), SceneRect.top());
   Painter.drawImage(-1, -1, *ForegroundImage);
   Painter.setBrush(Qt::NoBrush);

   UpdateIsSelected();

   if (DisplayMode != DisplayMode_BlockShape)
   {
      Painter.restore();
      return;
   }

   int CurSelect = 0;

   //QRectF xFullSelRect;
   //QRectF xCurSelRect;
   //QRectF xCurSelScreenRect;
   // Draw blue frame borders when multi-select
   FullSelRect = QRectF();
   CurSelRect = QRectF();
   CurSelScreenRect = QRectF();
   for (int i = 0; i < BlockTable->CompositionList->compositionObjects.count(); i++)
   {
      cCompositionObject* curObj = BlockTable->CompositionList->compositionObjects.at(i);
      if (curObj->isVisible() && IsSelected.at(i))
      {
         QRectF FullRect = curObj->appliedRect(ScreenRect.width(), ScreenRect.height());

         QRectF NewRect = ApplyModifAndScaleFactors(curObj, SceneRect, true, true);
         QRectF NewRectScreen = ApplyModifAndScaleFactors(curObj, ScreenRect, true, true);
         if (NbrSelected > 1)
         {
            QPen pen(Qt::white);
            pen.setJoinStyle(Qt::RoundJoin);
            pen.setWidth(1);
            pen.setStyle(Qt::DashLine);
            Painter.setPen(pen);
            DrawSelect(Painter, QRectF(NewRect.left() + 1, NewRect.top() + 1, NewRect.width(), NewRect.height()), false);
            pen.setColor(Qt::blue);
            Painter.setPen(pen);
            DrawSelect(Painter, NewRect, false);
         }
         else if(curObj->usingMask() )
         {
            //// calculate mask-rect
            //QRectF realMaskRect;
            //bool inverted = mask.invertedMask();
            //if (inverted)
            //   realMaskRect = WSRect(maskRect).applyTo(orgRect.width(), orgRect.height());
            //else
            //   realMaskRect = WSRect(maskRect).applyTo(orgRect);
            //// get the form-polygon
            //QList<QPolygonF> masking = ComputePolygon(BackgroundForm, realMaskRect);

            QRectF maskRect = curObj->getMask()->appliedRect(NewRect);
            //QList<QPolygonF> masking = ComputePolygon(BackgroundForm, realMaskRect);
            //QTransform tr;
            //if (curObj->getMask()->zAngle() != 0 || curObj->xAngle() != 0 || curObj->yAngle() != 0)
            //{
            //   QPointF maskCenter = maskRect.center();
            //   tr.translate(maskCenter.x(), maskCenter.y());
            //   tr.rotate(curObj->getMask()->zAngle());
            //   tr.rotate(curObj->xAngle(), Qt::XAxis);
            //   tr.rotate(curObj->yAngle(), Qt::YAxis);
            //   tr.translate(-maskCenter.x(), -maskCenter.y());
            //   maskRect = tr.mapRect(maskRect);
            //}
            QPen pen(Qt::white);
            pen.setJoinStyle(Qt::RoundJoin);
            pen.setWidth(2);
            pen.setStyle(Qt::DashLine);
            Painter.setPen(pen);
            DrawSelect(Painter, maskRect, false);
         }

         FullSelRect = FullSelRect.united(FullRect);
         CurSelRect = CurSelRect.united(NewRect);
         CurSelScreenRect = CurSelScreenRect.united(NewRectScreen);
         CurSelect++;
      }
   }
   if (!IsCapture)
   {
      Sel_X = CurSelScreenRect.left() / ScreenRect.width();
      Sel_Y = CurSelScreenRect.top() / ScreenRect.height();
      Sel_W = CurSelScreenRect.width() / ScreenRect.width();
      Sel_H = CurSelScreenRect.height() / ScreenRect.height();
      RSel_X = FullSelRect.left() / ScreenRect.width();
      RSel_Y = FullSelRect.top() / ScreenRect.height();
      RSel_W = FullSelRect.width() / ScreenRect.width();
      RSel_H = FullSelRect.height() / ScreenRect.height();
   }
   if (!IsCapture && NbrSelected > 0)
   {
      if (CurSelRect.isEmpty())
      {
         Sel_W = 0.02;
         Sel_H = 0.02;
         CurSelRect.setWidth(2);     CurSelScreenRect.setWidth(2);
         CurSelRect.setHeight(2);    CurSelScreenRect.setHeight(2);
      }
      if (FullSelRect.isEmpty())
      {
         RSel_W = 0.02;
         RSel_H = 0.02;
         FullSelRect.setWidth(2);
         FullSelRect.setHeight(2);
      }
      AspectRatio = FullSelRect.height() / FullSelRect.width();
   }

   // Draw rulers if they was enabled
   if (MagneticRuler != 0)
   {
      QList<sDualQReal> MagnetVert;
      QList<sDualQReal> MagnetHoriz;
      ComputeRulers(MagnetVert, MagnetHoriz);

      // Draw rulers
      Painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
      QPen Pen1 = QPen(QColor(0, 255, 0)), Pen2 = QPen(QColor(0, 255, 0));
      Pen1.setWidth(1);
      Pen1.setStyle(Qt::DotLine);
      Pen2.setWidth(1);
      Pen2.setStyle(Qt::DashDotDotLine);
      foreach(sDualQReal sdr, MagnetVert)
      {
         if (sdr.Block == 0 || sdr.Block == SceneRect.width() ||                                                    // borders of the screen
            (sdr.Block == SceneRect.width()*0.5 - 1) || (sdr.Block == SceneRect.width()*0.5) ||                     // center of the screen
            (sdr.Block == SceneRect.width()*0.05) || (sdr.Block == SceneRect.width() - SceneRect.width()*0.05))     // TV Margins
            Painter.setPen(Pen1);
         else
            Painter.setPen(Pen2);
         if (sdr.Block != 0 && sdr.Block != SceneRect.width())
         {                                                   // don't draw screen borders
            Painter.drawLine(sdr.Block, 0, sdr.Block, SceneRect.height());
         }
      }
      foreach(sDualQReal sdr, MagnetHoriz)
      {
         if ((sdr.Block == 0) || (sdr.Block == SceneRect.height()) ||                                                 // borders of the screen
            (sdr.Block == SceneRect.height()*0.5 - 1) || (sdr.Block == SceneRect.height()*0.5) ||                     // centers of the screen
            (sdr.Block == SceneRect.height()*0.05) || (sdr.Block == SceneRect.height() - SceneRect.height()*0.05))    // TV Margins
            Painter.setPen(Pen1);
         else
            Painter.setPen(Pen2);
         if (sdr.Block != 0 && sdr.Block != SceneRect.height())
         {                                                // don't draw screen borders
            Painter.drawLine(0, sdr.Block, SceneRect.width(), sdr.Block);
         }
      }
   }

   // Draw select frame border
   if (NbrSelected > 0)
   {
      QPen pen(Qt::red);
      pen.setWidth(2);
      pen.setStyle(Qt::DashLine);
      Painter.setPen(pen);
      if (CurSelRect.isValid())
         DrawSelect(Painter, CurSelRect, true);
   }
   if (NbrSelected == 1)
   {
      qDebug() << " drawn one object, CurrentShotNbr is " << CurrentShotNbr;
      cCompositionObject* cobj = *pCurrentCompoObject;
      if (pCurrentCompoObjectNbr != 0)
      {
         QPointF prevPos;
         cCompositionObject* vprevObj = 0;
         for (int k = 0; k < CurrentShotNbr; k++)
         {
            cCompositionObject* prevObj = DiaporamaObject->shotList[k]->ShotComposition.getCompositionObject(cobj->index());
            if (prevObj /*&& prevObj->isVisible()*/)
            {
               prevPos = prevObj->pos();
               vprevObj = prevObj;
            }
         }
         if (vprevObj)
         {
            qDebug() << "CurrentCompoObjectNbr is " << *pCurrentCompoObjectNbr << " pos is " << cobj->pos() << " prev Pos is " << prevPos;
            QRectF pos1 = ApplyModifAndScaleFactors(vprevObj, SceneRect, true, false);
            QRectF pos2 = ApplyModifAndScaleFactors(cobj, SceneRect, true, true);
            qDebug() << "pos1 " << pos1 << " pos2 " << pos2 << " SceneRect is " << SceneRect;
            //Painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
            Painter.setCompositionMode(QPainter::CompositionMode_Source);
            QPen Pen1 = QPen(QColor(0, 255, 0));
            Pen1.setWidth(3);
            Pen1.setStyle(Qt::SolidLine);
            Painter.setPen(Pen1);
            //Painter.drawLine(pos1.center(), pos2.center());



            QLineF line(pos1.center(), pos2.center());
            double angle = std::atan2(-line.dy(), line.dx());
            qreal arrowSize = 10;
            QPointF arrowP1 = line.p2() - QPointF(sin(angle + M_PI / 3) * arrowSize,
               cos(angle + M_PI / 3) * arrowSize);
            QPointF arrowP2 = line.p2() - QPointF(sin(angle + M_PI - M_PI / 3) * arrowSize,
               cos(angle + M_PI - M_PI / 3) * arrowSize);
            QPolygonF arrowHead;
            arrowHead << line.p2() << arrowP2 << arrowP1;

            Painter.drawLine(line);
            Painter.drawLine(pos2.center(), arrowP1);
            Painter.drawLine(pos2.center(), arrowP2);
         }
      }
   }

   Painter.restore();
}

//====================================================================================================================

void cInteractiveZone::DrawSelect(QPainter &Painter, QRectF Rect, bool WithHandles) 
{
   Painter.drawRect(Rect);
   if (WithHandles)
   {
      QPen OldPen = Painter.pen();
      QPen pen = OldPen;
      pen.setStyle(Qt::SolidLine);
      Painter.setPen(pen);
      QVector<QRect> v = createHandleRects(Rect);
      Painter.drawRects(v);
      Painter.setPen(OldPen);
   }
}

//====================================================================================================================

void cInteractiveZone::UpdateIsSelected()
{
   QModelIndexList SelList = BlockTable->selectionModel()->selectedIndexes();
   IsSelected.clear();
   NbrSelected = 0;
   LockGeometry = false;

   for (int i = 0; i < BlockTable->rowCount(); i++)
      IsSelected.append(false);
   for (int i = 0; i < SelList.count(); i++)
      IsSelected[SelList.at(i).row()] = BlockTable->CompositionList->compositionObjects[SelList.at(i).row()]->isVisible();

   for (int i = 0; i < IsSelected.count(); i++)
      if (IsSelected[i])
      {
         NbrSelected++;
         if (BlockTable->CompositionList->compositionObjects[i]->BackgroundBrush->LockGeometry() )
            LockGeometry = true;
      }
}

//====================================================================================================================

bool cInteractiveZone::IsInRect(QPointF Pos, QRectF Rect) 
{
   QPointF nPos(Pos.x() - SceneRect.left(), Pos.y() - SceneRect.top());
   return (Rect.contains(nPos));
}

//====================================================================================================================

bool cInteractiveZone::IsInSelectedRect(QPointF Pos) 
{
   UpdateIsSelected();
   QPointF nPos(Pos.x() - SceneRect.left(), Pos.y() - SceneRect.top());
   for (int i = 0; i < BlockTable->CompositionList->compositionObjects.count(); i++) 
      if (BlockTable->CompositionList->compositionObjects.at(i)->isVisible() && IsSelected.at(i))
      {
         QRectF ObjRect = ApplyModifAndScaleFactors(i, SceneRect, true);
         if (ObjRect.contains(nPos)) 
            return true;
      }
   return false;
}

//====================================================================================================================
Qt::CursorShape cursors[9] = 
{
   Qt::SizeBDiagCursor, Qt::SizeFDiagCursor, Qt::SizeHorCursor, Qt::SizeBDiagCursor, 
   Qt::SizeHorCursor,   Qt::SizeFDiagCursor, Qt::SizeVerCursor, Qt::SizeVerCursor, 
   Qt::OpenHandCursor
};

void cInteractiveZone::ManageCursor(QPointF Pos, QVector<QRect> v, Qt::KeyboardModifiers Modifiers) 
{
   QPoint nPos(Pos.x() - SceneRect.left(), Pos.y() - SceneRect.top());
   int i = 0;
   foreach(QRect r, v)
   {
      if( r.contains(nPos) )
      {
         setCursor(cursors[i]);
         return;
      }
      i++;
   }

   if (IsInSelectedRect(Pos))
   {
      if (Modifiers == Qt::NoModifier)
         setCursor(Qt::OpenHandCursor);
      else if (Modifiers == Qt::ControlModifier || Modifiers == Qt::ShiftModifier)  
         setCursor(Qt::PointingHandCursor);
      else if (Modifiers == (Qt::ControlModifier | Qt::ShiftModifier))
         setCursor(Qt::CrossCursor);
      else
         setCursor(Qt::ArrowCursor);
   }
   else 
      setCursor(Qt::ArrowCursor);   // standard
}

void cInteractiveZone::ManageCursor(QPointF Pos, Qt::KeyboardModifiers Modifiers) 
{
   if (IsInRect(Pos, QRect(CurSelRect.left() - HANDLESIZEX_2, CurSelRect.bottom() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY)))
      setCursor(Qt::SizeBDiagCursor);  // Bottom left
   else if (IsInRect(Pos, QRect(CurSelRect.left() - HANDLESIZEX / 2, CurSelRect.top() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY)))                          
      setCursor(Qt::SizeFDiagCursor);  // Top left
   else if (IsInRect(Pos, QRect(CurSelRect.left() - HANDLESIZEX / 2, CurSelRect.top() + CurSelRect.height() / 2 - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY)))   
      setCursor(Qt::SizeHorCursor);    // Left
   else if (IsInRect(Pos, QRect(CurSelRect.right() - HANDLESIZEX / 2, CurSelRect.top() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY)))                         
      setCursor(Qt::SizeBDiagCursor);  // Top right
   else if (IsInRect(Pos, QRect(CurSelRect.right() - HANDLESIZEX / 2, CurSelRect.top() + CurSelRect.height() / 2 - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY)))   
      setCursor(Qt::SizeHorCursor);    // Right
   else if (IsInRect(Pos, QRect(CurSelRect.right() - HANDLESIZEX / 2, CurSelRect.bottom() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY)))                      
      setCursor(Qt::SizeFDiagCursor);  // Bottom right
   else if (IsInRect(Pos, QRect(CurSelRect.left() + CurSelRect.width() / 2 - HANDLESIZEX_2, CurSelRect.top() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY)))     
      setCursor(Qt::SizeVerCursor);    // Top
   else if (IsInRect(Pos, QRect(CurSelRect.left() + CurSelRect.width() / 2 - HANDLESIZEX_2, CurSelRect.bottom() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY)))  
      setCursor(Qt::SizeVerCursor);    // Bottom
   else if (IsInSelectedRect(Pos))
   {
      if (Modifiers == Qt::NoModifier)
         setCursor(Qt::OpenHandCursor);
      else if (Modifiers == Qt::ControlModifier || Modifiers == Qt::ShiftModifier)  
         setCursor(Qt::PointingHandCursor);
      else if (Modifiers == (Qt::ControlModifier | Qt::ShiftModifier))
         setCursor(Qt::CrossCursor);
      else
         setCursor(Qt::ArrowCursor);
   }
   else 
      setCursor(Qt::ArrowCursor);   // standard
}

//====================================================================================================================

QRectF cInteractiveZone::ComputeNewCurSelRect()
{
   QRectF NewCurSelRect;
   int CurSelect = 0;
   for (int i = 0; i < BlockTable->CompositionList->compositionObjects.count(); i++)
   {
      if (BlockTable->CompositionList->compositionObjects.at(i)->isVisible() && IsSelected.at(i))
      {
         QRectF NewRect = ApplyModifAndScaleFactors(i, SceneRect, true);
         NewCurSelRect = NewCurSelRect.united(NewRect);
         CurSelect++;
      }
   }
   return NewCurSelRect;
}

QRectF cInteractiveZone::ComputeNewCurSelScreenRect()
{
   QRectF NewCurSelRect;
   int CurSelect = 0;
   for (int i = 0; i < BlockTable->CompositionList->compositionObjects.count(); i++)
   {
      if (BlockTable->CompositionList->compositionObjects.at(i)->isVisible() && IsSelected.at(i))
      {
         QRectF NewRect = ApplyModifAndScaleFactors(i, ScreenRect, true);
         NewCurSelRect = NewCurSelRect.united(NewRect);
         CurSelect++;
      }
   }
   return NewCurSelRect;
}

//====================================================================================================================

void cInteractiveZone::keyPressEvent(QKeyEvent *event) 
{
   //ToLog(LOGMSG_DEBUGTRACE, "IN:cInteractiveZone::keyPressEvent");
   if (!BlockTable || !BlockTable->CompositionList) 
      return;

   ManageCursor(mapFromGlobal(QCursor::pos()), event->modifiers()); 
   if (NbrSelected > 0)
   {
      double StepX = 1.0 / SceneRect.width();
      double StepY = 1.0 / SceneRect.height();

      Move_X = 0;
      Scale_X = 0;
      Move_Y = 0;
      Scale_Y = 0;

      switch (event->key())
      {
         case Qt::Key_Left:
            if (event->modifiers() == Qt::ShiftModifier) 
            { 
               Move_X = -StepX; 
               Scale_X = StepX; 
               Move_Y = LockGeometry ? (AspectRatio*Move_X*SceneRect.width()) / SceneRect.height() : 0; 
               Scale_Y = LockGeometry ? (AspectRatio*Scale_X*SceneRect.width()) / SceneRect.height() : 0; 
            }
            else if (event->modifiers() == Qt::ControlModifier) 
            { 
               Scale_X = -StepX; 
               Scale_Y = LockGeometry ? (AspectRatio*Scale_X*SceneRect.width()) / SceneRect.height() : 0; 
            }
            else
               Move_X = -StepX;
            break;
         case Qt::Key_Right:
            if (event->modifiers() == Qt::ShiftModifier) 
            { 
               Move_X = StepX; 
               Scale_X = -StepX; 
               Move_Y = LockGeometry ? (AspectRatio*Move_X*SceneRect.width()) / SceneRect.height() : 0; 
               Scale_Y = LockGeometry ? (AspectRatio*Scale_X*SceneRect.width()) / SceneRect.height() : 0; 
            }
            else if (event->modifiers() == Qt::ControlModifier) 
            { 
               Scale_X = StepX; 
               Scale_Y = LockGeometry ? (AspectRatio*Scale_X*SceneRect.width()) / SceneRect.height() : 0; 
            }
            else
               Move_X = StepX;
            break;
         case Qt::Key_Up:
            if (event->modifiers() == Qt::ShiftModifier) 
            { 
               Move_Y = -StepY; 
               Scale_Y = StepY; 
               Move_X = LockGeometry ? ((Move_Y*SceneRect.height()) / AspectRatio) / SceneRect.width() : 0; 
               Scale_X = LockGeometry ? ((Scale_Y*SceneRect.height()) / AspectRatio) / SceneRect.width() : 0; 
            }
            else if (event->modifiers() == Qt::ControlModifier) 
            { 
               Scale_Y = -StepY; 
               Scale_X = LockGeometry ? ((Scale_Y*SceneRect.height()) / AspectRatio) / SceneRect.width() : 0; 
            }
            else
               Move_Y = -StepY;
            break;
         case Qt::Key_Down:
            if (event->modifiers() == Qt::ShiftModifier) 
            { 
               Move_Y = StepY; 
               Scale_Y = -StepY; 
               Move_X = LockGeometry ? ((Move_Y*SceneRect.height()) / AspectRatio) / SceneRect.width() : 0; 
               Scale_X = LockGeometry ? ((Scale_Y*SceneRect.height()) / AspectRatio) / SceneRect.width() : 0; 
            }
            else if (event->modifiers() == Qt::ControlModifier) 
            { 
               Scale_Y = StepY; 
               Scale_X = LockGeometry ? ((Scale_Y*SceneRect.height()) / AspectRatio) / SceneRect.width() : 0; 
            }
            else
               Move_Y = StepY;
            break;
         default:
            QWidget::keyPressEvent(event);
            break;
      }
      if (Move_X != 0 || Move_Y != 0 || Scale_X != 0 || Scale_Y != 0)
         emit TransformBlock(Move_X, Move_Y, Scale_X, Scale_Y, RSel_X, RSel_Y, RSel_W, RSel_H);
   }
   else
      QWidget::keyPressEvent(event);
}

//====================================================================================================================

void cInteractiveZone::keyReleaseEvent(QKeyEvent *event) 
{
   //ToLog(LOGMSG_DEBUGTRACE, "IN:cInteractiveZone::keyReleaseEvent");
   if (!BlockTable || !BlockTable->CompositionList) 
      return;

   ManageCursor(mapFromGlobal(QCursor::pos()), event->modifiers());
   QWidget::keyReleaseEvent(event);
}

//====================================================================================================================

inline bool inRange(qreal value, qreal base, qreal range)
{
   return abs(value - base) < range;
}

void cInteractiveZone::ComputeRulers(QList<sDualQReal> &MagnetVert, QList<sDualQReal> &MagnetHoriz)
{
   //qreal hEcart = 0.005*ScreenRect.width();
   qreal vEcart = 0.005*ScreenRect.height();
   qreal h1 = CurSelScreenRect.left(); 
   qreal v1 = CurSelScreenRect.top();
   qreal h2 = CurSelScreenRect.left() + (CurSelScreenRect.width() / 2);
   qreal v2 = CurSelScreenRect.top() + (CurSelScreenRect.height() / 2);
   qreal h3 = CurSelScreenRect.right();
   qreal v3 = CurSelScreenRect.bottom();

   if ((MagneticRuler & RULER_VERT_SCREENBORDER) != 0)
   {
      MagnetVert.append(sDualQReal(0, 0));   // Left screen
      MagnetVert.append(sDualQReal(ScreenRect.width(), SceneRect.width())); // Right screen
   }
   if ((MagneticRuler & RULER_VERT_TVMARGIN) != 0)
   {
      MagnetVert.append(sDualQReal(ScreenRect.width()*0.05, SceneRect.width()*0.05));  // 5% Left TV Margins
      MagnetVert.append(sDualQReal(ScreenRect.width() - ScreenRect.width()*0.05, SceneRect.width() - SceneRect.width()*0.05)); // 5% Right TV Margins
   }
   if ((MagneticRuler & RULER_VERT_SCREENCENTER) != 0)
      MagnetVert.append(sDualQReal(ScreenRect.width()*0.5, SceneRect.width()*0.5));

   if ((MagneticRuler & RULER_HORIZ_SCREENBORDER) != 0)
   {
      MagnetHoriz.append(sDualQReal(0, 0));  // Top screen
      MagnetHoriz.append(sDualQReal(ScreenRect.height(), SceneRect.height())); // Bottom screen
   }
   if ((MagneticRuler & RULER_HORIZ_TVMARGIN) != 0)
   {
      MagnetHoriz.append(sDualQReal(ScreenRect.height()*0.05, SceneRect.height()*0.05));  // 5% Up TV Margins
      MagnetHoriz.append(sDualQReal(ScreenRect.height() - ScreenRect.height()*0.05, SceneRect.height() - SceneRect.height()*0.05));   // 5% Bottom TV Margins
   }
   if ((MagneticRuler & RULER_HORIZ_SCREENCENTER) != 0)    
      MagnetHoriz.append(sDualQReal(ScreenRect.height()*0.5, SceneRect.height()*0.5));

   // Unselected object
   if (IsCapture)
   {
      for (int i = BlockTable->CompositionList->compositionObjects.count() - 1; i >= 0; i--)
      {
         cCompositionObject *obj = BlockTable->CompositionList->compositionObjects[i];
         if ((!IsSelected[i]) && (obj->isVisible()))
         {
            QList<QPolygonF> PolScene = ComputePolygon(obj->BackgroundForm, obj->appliedRect(SceneRect.width(), SceneRect.height()) );
            QList<QPolygonF> PolScreen = ComputePolygon(obj->BackgroundForm, obj->appliedRect(ScreenRect.width(), ScreenRect.height()) );

            QPolygonF PolSceneU(PolScene.at(0));
            for (int j = 1; j < PolScene.count(); j++)
               PolSceneU = PolSceneU.united(PolScene.at(j));
            QRectF tmpSceneRect = PolSceneU.boundingRect();

            QPolygonF PolScreenU(PolScreen.at(0));
            for (int j = 1; j < PolScreen.count(); j++)
               PolScreenU = PolScreenU.united(PolScreen.at(j));
            QRectF tmpScreenRect = PolScreenU.boundingRect();

            /*
            if ((BlockTable->CompositionList->List[i]->RotateXAxis!=0)||(BlockTable->CompositionList->List[i]->RotateYAxis!=0)||(BlockTable->CompositionList->List[i]->RotateZAxis!=0)) {
            QTransform   Matrix;
            Matrix.rotate(BlockTable->CompositionList->List[i]->RotateXAxis,Qt::XAxis);
            Matrix.rotate(BlockTable->CompositionList->List[i]->RotateYAxis,Qt::YAxis);
            Matrix.rotate(BlockTable->CompositionList->List[i]->RotateZAxis,Qt::ZAxis);

            QPointF      Center=tmpSceneRect.center();
            QPainterPath Path;
            PolSceneU.translate(-Center.x(),-Center.y());
            Path.addPolygon(PolSceneU);
            tmpSceneRect=Path.toFillPolygon(Matrix).boundingRect();
            tmpSceneRect.translate(Center);

            Center=tmpScreenRect.center();
            Path  =QPainterPath();
            PolScreenU.translate(-Center.x(),-Center.y());
            Path.addPolygon(PolScreenU);
            tmpScreenRect=Path.toFillPolygon(Matrix).boundingRect();
            tmpScreenRect.translate(Center);
            }
            */
            qreal x1 = tmpScreenRect.left();
            qreal y1 = tmpScreenRect.top();
            qreal x2 = tmpScreenRect.center().x();
            qreal y2 = tmpScreenRect.center().y();
            qreal x3 = tmpScreenRect.right();
            qreal y3 = tmpScreenRect.bottom();

            if ((MagneticRuler & RULER_VERT_UNSELECTED) != 0)
            {
               if ( isInGap(x1,h1,vEcart) || isInGap(x1,h2,vEcart) || isInGap(x1,h3,vEcart) )
                  MagnetVert.append(sDualQReal(x1, tmpSceneRect.left()));
               if ( isInGap(x2,h1,vEcart) || isInGap(x2,h2,vEcart) || isInGap(x2,h3,vEcart) )
                  MagnetVert.append(sDualQReal(x2, tmpSceneRect.center().x()));
               if ( isInGap(x3,h1,vEcart) || isInGap(x3,h2,vEcart) || isInGap(x3,h3,vEcart) )
                  MagnetVert.append(sDualQReal(x3, tmpSceneRect.right()));
               // Add intermediate rulers if exist and block don't rotated
               if ((ShapeFormDefinition.at(obj->BackgroundForm).AdditonnalRulerX.count() > 0) && !obj->hasRotation())
                  for (int AddR = 0; AddR < ShapeFormDefinition.at(obj->BackgroundForm).AdditonnalRulerX.count(); AddR++)
                  {
                     double PosXScreen = tmpScreenRect.left() + tmpScreenRect.width()*ShapeFormDefinition.at(obj->BackgroundForm).AdditonnalRulerX.at(AddR);
                     double PosXScene = tmpSceneRect.left() + tmpSceneRect.width()* ShapeFormDefinition.at(obj->BackgroundForm).AdditonnalRulerX.at(AddR);
                     if ( isInGap(PosXScreen,h1,vEcart) || isInGap(PosXScreen,h2,vEcart) || isInGap(PosXScreen,h3,vEcart) )
                        MagnetVert.append(sDualQReal(PosXScreen, PosXScene));
                  }
            }

            if ((MagneticRuler & RULER_HORIZ_UNSELECTED) != 0)
            {
               if ( isInGap(y1,v1,vEcart) || isInGap(y1,v2,vEcart) || isInGap(y1,v3,vEcart) )
                  MagnetHoriz.append(sDualQReal(y1, tmpSceneRect.top()));
               if ( isInGap(y2,v1,vEcart) || isInGap(y2,v2,vEcart) || isInGap(y2,v3,vEcart) )
                  MagnetHoriz.append(sDualQReal(y2, tmpSceneRect.center().y()));
               if ( isInGap(y3,v1,vEcart) || isInGap(y3,v2,vEcart) || isInGap(y3,v3,vEcart) )
                  MagnetHoriz.append(sDualQReal(y3, tmpSceneRect.bottom()));
               // Add intermediate rulers if exist and block don't rotated
               if ((ShapeFormDefinition.at(obj->BackgroundForm).AdditonnalRulerY.count() > 0) && !obj->hasRotation())
                  for (int AddR = 0; AddR<ShapeFormDefinition.at(BlockTable->CompositionList->compositionObjects[i]->BackgroundForm).AdditonnalRulerY.count(); AddR++)
                  {
                     double PosYScreen = tmpScreenRect.top() + tmpScreenRect.height()*ShapeFormDefinition.at(obj->BackgroundForm).AdditonnalRulerY.at(AddR);
                     double PosYScene = tmpSceneRect.top() + tmpSceneRect.height()* ShapeFormDefinition.at(obj->BackgroundForm).AdditonnalRulerY.at(AddR);
                     if ( isInGap(PosYScreen,v1,vEcart) || isInGap(PosYScreen,v2,vEcart) || isInGap(PosYScreen,v3,vEcart) )
                        MagnetHoriz.append(sDualQReal(PosYScreen, PosYScene));
                  }
            }
         }
      }
   }
   // Clean collections
   for (int i = MagnetHoriz.count() - 1; i >= 0; i--)
      for (int j = 0; j < i; j++)
         if (int(MagnetHoriz[j].Block) == int(MagnetHoriz[i].Block))
         {
            MagnetHoriz.removeAt(i);
            break;
         }
   for (int i = MagnetVert.count() - 1; i >= 0; i--)
      for (int j = 0; j < i; j++)
         if (int(MagnetVert[j].Block) == int(MagnetVert[i].Block))
         {
            MagnetVert.removeAt(i);
            break;
         }
}

//====================================================================================================================

void cInteractiveZone::mouseMoveEvent(QMouseEvent *event) 
{
   //ToLog(LOGMSG_DEBUGTRACE,"IN:cInteractiveZone::mouseMoveEvent");   // Remove : too much
   if (!BlockTable || !BlockTable->CompositionList) 
      return;

   if (!IsCapture)
   {
      ManageCursor(event->pos(), event->modifiers());
      return;
   }
   // *************************************************************************
   // Create rulers
   // *************************************************************************
   QList<sDualQReal>   MagnetVert;
   QList<sDualQReal>   MagnetHoriz;
   ComputeRulers(MagnetVert, MagnetHoriz);

   // *************************************************************************
   // Calc transformation
   // *************************************************************************
   QRectF  NewCurSelRect;
   qreal   DX = qreal(event->pos().x() - CapturePos.x()) / SceneRect.width();
   qreal   DY = qreal(event->pos().y() - CapturePos.y()) / SceneRect.height();
   Move_X = 0;
   Move_Y = 0;
   Scale_X = 0;
   Scale_Y = 0;
   CurSelScreenRect = ComputeNewCurSelScreenRect();

   // Top left
   if (TransfoType == RESIZEUPLEFT)
   {
      // Adjust DX and DY for resize not less than 0
      if (DX >= Sel_W - MINVALUE)  
         DX = Sel_W - MINVALUE;
      if (DY >= Sel_H - MINVALUE)  
         DY = Sel_H - MINVALUE;
      Move_X = DX;
      Move_Y = LockGeometry ? (AspectRatio*Move_X*ScreenRect.width()) / ScreenRect.height() : DY;
      Scale_X = -Move_X;
      Scale_Y = -Move_Y;
      NewCurSelRect = ComputeNewCurSelRect();

      // Apply magnetic rules vertical
      for (int Ruler = 0; Ruler < MagnetVert.count(); Ruler++) 
         if ((MagnetVert[Ruler].Block < NewCurSelRect.right()) && snap(NewCurSelRect.left(), MagnetVert[Ruler].Block,HANDLEMAGNETX) )
         {
            qreal NewW = CurSelScreenRect.right() - MagnetVert[Ruler].Screen;
            qreal OldW = CurSelScreenRect.width();
            qreal Trans = NewW / OldW;
            Move_X = -(NewW - OldW + (CurSelScreenRect.left() - FullSelRect.left())*Trans - (CurSelScreenRect.left() - FullSelRect.left())) / ScreenRect.width();
            if (LockGeometry) 
               Move_Y = (AspectRatio*Move_X*ScreenRect.width()) / ScreenRect.height();
            Scale_Y = -Move_Y;
            Scale_X = -Move_X;
            break;
         }

      // Apply magnetic rules horizontal
      for (int Ruler = 0; Ruler < MagnetHoriz.count(); Ruler++) 
         if (MagnetHoriz[Ruler].Block < NewCurSelRect.bottom() && snap(NewCurSelRect.top(), MagnetHoriz[Ruler].Block, HANDLEMAGNETY) )
         {
            qreal NewH = CurSelScreenRect.bottom() - MagnetHoriz[Ruler].Screen;
            qreal OldH = CurSelScreenRect.height();
            qreal Trans = NewH / OldH;
            Move_Y = -(NewH - OldH + (CurSelScreenRect.top() - FullSelRect.top())*Trans - (CurSelScreenRect.top() - FullSelRect.top())) / ScreenRect.height();
            if (LockGeometry) 
               Move_X = (Move_Y*ScreenRect.height() / AspectRatio) / ScreenRect.width();
            Scale_Y = -Move_Y;
            Scale_X = -Move_X;
            break;
         }

      // Left
   }
   else if (TransfoType == RESIZELEFT)
   {
      // Adjust DX and DY for resize not less than 0
      if (DX >= Sel_W - MINVALUE)  
         DX = Sel_W - MINVALUE;
      if (DY >= Sel_H - MINVALUE)  
         DY = Sel_H - MINVALUE;
      Move_X = DX;
      Scale_X = -Move_X;
      if (LockGeometry)
      {
         Scale_Y = (AspectRatio*Scale_X*ScreenRect.width()) / ScreenRect.height();
         Move_Y = -Scale_Y / 2;
      }
      NewCurSelRect = ComputeNewCurSelRect();

      // Apply magnetic rules vertical
      for (int Ruler = 0; Ruler < MagnetVert.count(); Ruler++) 
         if (MagnetVert[Ruler].Block < NewCurSelRect.right() && snap(NewCurSelRect.left(), MagnetVert[Ruler].Block, HANDLEMAGNETX) )
         {
            qreal NewW = CurSelScreenRect.right() - MagnetVert[Ruler].Screen;
            qreal OldW = CurSelScreenRect.width();
            qreal Trans = NewW / OldW;
            Move_X = -(NewW - OldW + (CurSelScreenRect.left() - FullSelRect.left())*Trans - (CurSelScreenRect.left() - FullSelRect.left())) / ScreenRect.width();
            Scale_X = -Move_X;
            if (LockGeometry)
            {
               Scale_Y = (AspectRatio*Scale_X*ScreenRect.width()) / ScreenRect.height();
               Move_Y = -Scale_Y / 2;
            }
            break;
         }

      // Bottom left
   }
   else if (TransfoType == RESIZEDOWNLEFT)
   {
      // Adjust DX and DY for resize not less than 0
      if (Sel_W != 0)
      {
         if (DX >= Sel_W - MINVALUE)     
            DX = Sel_W - MINVALUE;
         if (DY <= -(Sel_H - MINVALUE))  
            DY = -(Sel_H - MINVALUE);
      }
      Move_X = DX;
      Scale_X = -Move_X;
      Move_Y = 0;
      Scale_Y = LockGeometry ? (AspectRatio*Scale_X*ScreenRect.width()) / ScreenRect.height() : DY;

      NewCurSelRect = ComputeNewCurSelRect();

      // Apply magnetic rules vertical
      for (int Ruler = 0; Ruler < MagnetVert.count(); Ruler++) 
         if (MagnetVert[Ruler].Block < NewCurSelRect.right() && snap(NewCurSelRect.left(), MagnetVert[Ruler].Block, HANDLEMAGNETX) )
         {
            qreal NewW = CurSelScreenRect.right() - MagnetVert[Ruler].Screen;
            qreal OldW = CurSelScreenRect.width();
            qreal Trans = NewW / OldW;
            Move_X = -(NewW - OldW + (CurSelScreenRect.left() - FullSelRect.left())*Trans - (CurSelScreenRect.left() - FullSelRect.left())) / ScreenRect.width();
            Scale_X = -Move_X;
            if (LockGeometry) Scale_Y = -(AspectRatio*Move_X*ScreenRect.width()) / ScreenRect.height();
            break;
         }

      // Apply magnetic rules horizontal
      for (int Ruler = 0; Ruler<MagnetHoriz.count(); Ruler++) 
         if (MagnetHoriz[Ruler].Block > NewCurSelRect.top() && snap(NewCurSelRect.bottom(), MagnetHoriz[Ruler].Block, HANDLEMAGNETY) )
         {
            qreal NewH = MagnetHoriz[Ruler].Screen - CurSelScreenRect.top();
            qreal OldH = CurSelScreenRect.height();
            qreal Trans = NewH / OldH;
            Scale_Y = (NewH - OldH + (FullSelRect.bottom() - CurSelScreenRect.bottom())*Trans - (FullSelRect.bottom() - CurSelScreenRect.bottom())) / ScreenRect.height();

            if (LockGeometry)
            {
               Move_X = -(Scale_Y*ScreenRect.height() / AspectRatio) / ScreenRect.width();
               Scale_X = -Move_X;
            }
            break;
         }

      // Top right
   }
   else if (TransfoType == RESIZEUPRIGHT)
   {
      // Adjust DX and DY for resize not less than 0
      if (DX <= -(Sel_W - MINVALUE))  
         DX = -(Sel_W - MINVALUE);
      if (DY >= Sel_H - MINVALUE)     
         DY = Sel_H - MINVALUE;
      Move_X = 0;
      Scale_X = DX;
      Move_Y = LockGeometry ? (-AspectRatio*Scale_X*ScreenRect.width()) / ScreenRect.height() : DY;
      Scale_Y = -Move_Y;
      NewCurSelRect = ComputeNewCurSelRect();

      // Apply magnetic rules vertical
      for (int Ruler = 0; Ruler<MagnetVert.count(); Ruler++) 
         if (MagnetVert[Ruler].Block > NewCurSelRect.left() && snap(NewCurSelRect.right(), MagnetVert[Ruler].Block, HANDLEMAGNETX) )
         {
            qreal NewW = MagnetVert[Ruler].Screen - CurSelScreenRect.left();
            qreal OldW = CurSelScreenRect.width();
            qreal Trans = NewW / OldW;
            Scale_X = (NewW - OldW + (FullSelRect.right() - CurSelScreenRect.right())*Trans - (FullSelRect.right() - CurSelScreenRect.right())) / ScreenRect.width();
            if (LockGeometry) 
               Move_Y = -(AspectRatio*Scale_X*ScreenRect.width()) / ScreenRect.height();
            Scale_Y = -Move_Y;
            break;
         }

      // Apply magnetic rules horizontal
      for (int Ruler = 0; Ruler < MagnetHoriz.count(); Ruler++) 
         if (MagnetHoriz[Ruler].Block < NewCurSelRect.bottom() && snap(NewCurSelRect.top(), MagnetHoriz[Ruler].Block, HANDLEMAGNETY) )
         {
            qreal NewH = CurSelScreenRect.bottom() - MagnetHoriz[Ruler].Screen;
            qreal OldH = CurSelScreenRect.height();
            qreal Trans = NewH / OldH;
            Move_Y = -(NewH - OldH + (CurSelScreenRect.top() - FullSelRect.top())*Trans - (CurSelScreenRect.top() - FullSelRect.top())) / ScreenRect.height();
            if (LockGeometry) Scale_X = -(Move_Y*ScreenRect.height() / AspectRatio) / ScreenRect.width();
            Scale_Y = -Move_Y;
            break;
         }
      // Right
   }
   else if (TransfoType == RESIZERIGHT)
   {
      // Adjust DX and DY for resize not less than 0
      if (DX <= -(Sel_W - MINVALUE)) 
         DX = -(Sel_W - MINVALUE);
      if (DY <= -(Sel_H - MINVALUE)) 
         DY = -(Sel_H - MINVALUE);
      Move_X = 0;
      Scale_X = DX;
      if (LockGeometry)
      {
         Scale_Y = (AspectRatio*Scale_X*ScreenRect.width()) / ScreenRect.height();
         Move_Y = -Scale_Y / 2;
      }
      NewCurSelRect = ComputeNewCurSelRect();

      // Apply magnetic rules vertical
      for (int Ruler = 0; Ruler<MagnetVert.count(); Ruler++) 
         if (MagnetVert[Ruler].Block > NewCurSelRect.left() && snap(NewCurSelRect.right(), MagnetVert[Ruler].Block, HANDLEMAGNETX))
         {
            qreal NewW = MagnetVert[Ruler].Screen - CurSelScreenRect.left();
            qreal OldW = CurSelScreenRect.width();
            qreal Trans = NewW / OldW;
            Scale_X = (NewW - OldW + (FullSelRect.right() - CurSelScreenRect.right())*Trans - (FullSelRect.right() - CurSelScreenRect.right())) / ScreenRect.width();
            if (LockGeometry)
            {
               Scale_Y = (AspectRatio*Scale_X*ScreenRect.width()) / ScreenRect.height();
               Move_Y = -Scale_Y / 2;
            }
            break;
         }
      // Bottom right
   }
   else if (TransfoType == RESIZEDOWNRIGHT)
   {
      // Adjust DX and DY for resize not less than 0
      if (DX <= -(Sel_W - MINVALUE)) 
         DX = -(Sel_W - MINVALUE);
      if (DY <= -(Sel_H - MINVALUE)) 
         DY = -(Sel_H - MINVALUE);
      Move_X = 0;
      Scale_X = DX;
      Move_Y = 0;
      Scale_Y = LockGeometry ? (AspectRatio*Scale_X*ScreenRect.width()) / ScreenRect.height() : DY;
      NewCurSelRect = ComputeNewCurSelRect();

      // Apply magnetic rules vertical
      for (int Ruler = 0; Ruler < MagnetVert.count(); Ruler++) 
         if (MagnetVert[Ruler].Block > NewCurSelRect.left() && snap(NewCurSelRect.right(), MagnetVert[Ruler].Block, HANDLEMAGNETX) )
         {
            qreal NewW = MagnetVert[Ruler].Screen - CurSelScreenRect.left();
            qreal OldW = CurSelScreenRect.width();
            qreal Trans = NewW / OldW;
            Scale_X = (NewW - OldW + (FullSelRect.right() - CurSelScreenRect.right())*Trans - (FullSelRect.right() - CurSelScreenRect.right())) / ScreenRect.width();
            if (LockGeometry) Scale_Y = (AspectRatio*Scale_X*ScreenRect.width()) / ScreenRect.height();
            break;
         }

      // Apply magnetic rules horizontal
      for (int Ruler = 0; Ruler<MagnetHoriz.count(); Ruler++) 
         if (MagnetHoriz[Ruler].Block > NewCurSelRect.top() && snap(NewCurSelRect.bottom(), MagnetHoriz[Ruler].Block, HANDLEMAGNETY) )
         {
            qreal NewH = MagnetHoriz[Ruler].Screen - CurSelScreenRect.top();
            qreal OldH = CurSelScreenRect.height();
            qreal Trans = NewH / OldH;
            Scale_Y = (NewH - OldH + (FullSelRect.bottom() - CurSelScreenRect.bottom())*Trans - (FullSelRect.bottom() - CurSelScreenRect.bottom())) / ScreenRect.height();
            if (LockGeometry) 
               Scale_X = (Scale_Y*ScreenRect.height() / AspectRatio) / ScreenRect.width();
            break;
         }
      // Top
   }
   else if (TransfoType == RESIZEUP)
   {
      // Adjust DX and DY for resize not less than 0
      if (DY >= Sel_H - MINVALUE)  
         DY = Sel_H - MINVALUE;
      Move_Y = DY;
      Scale_Y = -Move_Y;
      if (LockGeometry)
      {
         Scale_X = (Scale_Y*ScreenRect.height() / AspectRatio) / ScreenRect.width();
         Move_X = -Scale_X / 2;
      }
      NewCurSelRect = ComputeNewCurSelRect();

      // Apply magnetic rules horizontal
      for (int Ruler = 0; Ruler < MagnetHoriz.count(); Ruler++) 
         if (MagnetHoriz[Ruler].Block < NewCurSelRect.bottom() && snap(NewCurSelRect.top(), MagnetHoriz[Ruler].Block, HANDLEMAGNETY) )
         {
            qreal NewH = CurSelScreenRect.bottom() - MagnetHoriz[Ruler].Screen;
            qreal OldH = CurSelScreenRect.height();
            qreal Trans = NewH / OldH;
            Move_Y = -(NewH - OldH + (CurSelScreenRect.top() - FullSelRect.top())*Trans - (CurSelScreenRect.top() - FullSelRect.top())) / ScreenRect.height();
            Scale_Y = -Move_Y;
            if (LockGeometry)
            {
               Scale_X = (Scale_Y*ScreenRect.height() / AspectRatio) / ScreenRect.width();
               Move_X = -Scale_X / 2;
            }
            break;
         }
      // Bottom
   }
   else if (TransfoType == RESIZEDOWN)
   {
      // Adjust DX and DY for resize not less than 0
      if (DY <= -(Sel_H - MINVALUE)) 
         DY = -(Sel_H - MINVALUE);
      Move_Y = 0;
      Scale_Y = DY;
      if (LockGeometry)
      {
         Scale_X = (Scale_Y*ScreenRect.height() / AspectRatio) / ScreenRect.width();
         Move_X = -Scale_X / 2;
      }
      NewCurSelRect = ComputeNewCurSelRect();

      // Apply magnetic rules horizontal
      for (int Ruler = 0; Ruler<MagnetHoriz.count(); Ruler++) 
         if (MagnetHoriz[Ruler].Block > NewCurSelRect.top() && snap(NewCurSelRect.bottom(), MagnetHoriz[Ruler].Block, HANDLEMAGNETY) )
         {
            qreal NewH = MagnetHoriz[Ruler].Screen - CurSelScreenRect.top();
            qreal OldH = CurSelScreenRect.height();
            qreal Trans = NewH / OldH;
            Scale_Y = (NewH - OldH + (FullSelRect.bottom() - CurSelScreenRect.bottom())*Trans - (FullSelRect.bottom() - CurSelScreenRect.bottom())) / ScreenRect.height();
            if (LockGeometry)
            {
               Scale_X = (Scale_Y*ScreenRect.height() / AspectRatio) / ScreenRect.width();
               Move_X = -Scale_X / 2;
            }
            break;
         }
      // Move
   }
   else if (TransfoType == MOVEBLOCK)
   {
      Move_X = DX;
      Move_Y = DY;

      if (MagneticRuler != 0)
      {
         QRectF NewCurSelRect = ComputeNewCurSelRect();

         // Apply magnetic rules vertical
         for (int Ruler = 0; Ruler < MagnetVert.count(); Ruler++)
         {
            if ( snap(NewCurSelRect.left(), MagnetVert[Ruler].Block, HANDLEMAGNETX) ) 
            { 
               Move_X = qreal(MagnetVert[Ruler].Screen - CurSelScreenRect.left()) / ScreenRect.width();   
               break; 
            }
            else if (snap(NewCurSelRect.right(), MagnetVert[Ruler].Block, HANDLEMAGNETX) ) 
            { 
               Move_X = qreal(MagnetVert[Ruler].Screen - CurSelScreenRect.right()) / ScreenRect.width();   
               break; 
            }
            else if (snap(NewCurSelRect.center().x(), MagnetVert[Ruler].Block, HANDLEMAGNETX) ) 
            { 
               Move_X = qreal(MagnetVert[Ruler].Screen - CurSelScreenRect.center().x()) / ScreenRect.width();   
               break; 
            }
         }

         // Apply magnetic rules horizontal
         for (int Ruler = 0; Ruler < MagnetHoriz.count(); Ruler++)
         {
            if (snap(NewCurSelRect.top(), MagnetHoriz[Ruler].Block, HANDLEMAGNETY) ) 
            { 
               Move_Y = qreal(MagnetHoriz[Ruler].Screen - CurSelScreenRect.top()) / ScreenRect.height();  
               break; 
            }
            else if (snap(NewCurSelRect.bottom(), MagnetHoriz[Ruler].Block, HANDLEMAGNETY) ) 
            { 
               Move_Y = qreal(MagnetHoriz[Ruler].Screen - CurSelScreenRect.bottom()) / ScreenRect.height();  
               break; 
            }
            else if (snap(NewCurSelRect.center().y(), MagnetHoriz[Ruler].Block, HANDLEMAGNETY) ) 
            { 
               Move_Y = qreal(MagnetHoriz[Ruler].Screen - CurSelScreenRect.center().y()) / ScreenRect.height();  
               break; 
            }
         }
      }
   }
   if (!IsRefreshQueued)
   {
      IsRefreshQueued = true;
      QTimer::singleShot(LATENCY, this, SLOT(RefreshDisplay()));
   }
   if (NbrSelected == 1) 
      emit DisplayTransformBlock(Move_X, Move_Y, Scale_X, Scale_Y, RSel_X, RSel_Y, RSel_W, RSel_H);
}

//====================================================================================================================

void cInteractiveZone::mouseDoubleClickEvent(QMouseEvent *event) 
{
   //ToLog(LOGMSG_DEBUGTRACE, "IN:cInteractiveZone::mouseDoubleClickEvent");
   if (!BlockTable || !BlockTable->CompositionList) 
      return;

   if (event->button() != Qt::LeftButton)
      return;

   if (!(NbrSelected == 1 && IsInSelectedRect(event->pos())))
   {
      if (NbrSelected >= 0) 
         BlockTable->clearSelection();
      // Try to select another block
      int i = BlockTable->CompositionList->compositionObjects.count() - 1;
      while (i >= 0)
      {
         cCompositionObject *obj = BlockTable->CompositionList->compositionObjects[i];
         if (obj->isVisible())
         {
            QRect ObjRect = obj->appliedRect(SceneRect.width(), SceneRect.height()).toRect(); 
            if (IsInRect(event->pos(), ObjRect))
            {
               BlockTable->clearSelection();
               BlockTable->setCurrentCell(i, 0, QItemSelectionModel::Current | QItemSelectionModel::Select);
               break;
            }
         }
         i--;
      }
      UpdateIsSelected();
   }
   if (NbrSelected == 1 && IsInSelectedRect(event->pos())) 
      QTimer::singleShot(250, this, SLOT(DifferedEmitDoubleClickEvent()));    // Append " emit DoubleClickEvent" to the message queue
}

//====================================================================================================================

void cInteractiveZone::mousePressEvent(QMouseEvent *event) 
{
   //ToLog(LOGMSG_DEBUGTRACE, "IN:cInteractiveZone::mousePressEvent");
   if (!BlockTable || !BlockTable->CompositionList) 
      return;

   QVector<QRect> v = createHandleRects(CurSelRect);
   ManageCursor(event->pos(), v, event->modifiers());
   emit StartSelectionChange();
   setFocus();
   if (event->button() == Qt::RightButton)
   {
      // Reset selection if no block or only one is selected
      if ((!((NbrSelected > 0) && (IsInSelectedRect(event->pos())))) && (NbrSelected < 2))
      {
         BlockTable->clearSelection();
         // Try to select another block
         int i = BlockTable->CompositionList->compositionObjects.count() - 1;
         while (i >= 0)
         {
            if (BlockTable->CompositionList->compositionObjects[i]->isVisible())
            {
               QRectF ObjRect = ApplyModifAndScaleFactors(i, SceneRect, true);
               if (IsInRect(event->pos(), ObjRect))
               {
                  BlockTable->clearSelection();
                  BlockTable->setCurrentCell(i, 0, QItemSelectionModel::Current | QItemSelectionModel::Select);
                  break;
               }
            }
            i--;
         }
         UpdateIsSelected();
      }
      QTimer::singleShot(250, this, SLOT(DifferedEmitRightClickEvent()));    // Append " emit RightClickEvent" to the message queue

   } 
   if (event->button() == Qt::LeftButton)
   {
      TransfoType = NOTYETDEFINED;

      if ((event->modifiers() == Qt::ControlModifier) || (event->modifiers() == Qt::ShiftModifier))
      {
         // Try to toggle block to a multi block selection (one click to add, new click to remove)
         int i = BlockTable->CompositionList->compositionObjects.count() - 1;
         while (i >= 0)
         {
            if (BlockTable->CompositionList->compositionObjects[i]->isVisible())
            {
               QRectF ObjRect = ApplyModifAndScaleFactors(i, SceneRect, true);
               if (IsInRect(event->pos(), ObjRect))
               {
                  IsSelected[i] = !IsSelected[i];
                  break;
               }
            }
            i--;
         }

         int FirstSelected = -1;
         for (i = 0; i < BlockTable->CompositionList->compositionObjects.count(); i++) 
            if (IsSelected[i])
            {
               FirstSelected = i;
               break;
            }
         BlockTable->clearSelection();
         if (FirstSelected == -1)
         {
            BlockTable->setCurrentCell(i, 0, QItemSelectionModel::Current | QItemSelectionModel::Deselect);
         }
         else
         {
            BlockTable->setCurrentCell(i, 0, QItemSelectionModel::Current | QItemSelectionModel::Select);
            for (i = 0; i < BlockTable->CompositionList->compositionObjects.count(); i++) if ((IsSelected[i]) && (i != FirstSelected))
               BlockTable->setCurrentCell(i, 0, QItemSelectionModel::Select);
         }
      }
      else if (event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier))
      {
         // Try to select a block under current selected block
         if (NbrSelected == 1)
         {
            int i = IsSelected.count() - 1;
            while ((i >= 0) && (!IsSelected[i])) i--;   // Find current selected block
            for (int j = i - 1; j >= 0; j--) if (BlockTable->CompositionList->compositionObjects[j]->isVisible())
            {
               QRectF ObjRect = ApplyModifAndScaleFactors(i, SceneRect, true);
               if (IsInRect(event->pos(), ObjRect))
               {
                  BlockTable->clearSelection();
                  BlockTable->setCurrentCell(j, 0, QItemSelectionModel::Current | QItemSelectionModel::Select);
                  break;
               }
            }
         }

      }
      else if (event->modifiers() == Qt::NoModifier)
      {
         // Resize
         if ((NbrSelected > 0) && (IsInRect(event->pos(), QRect(CurSelRect.left() - HANDLESIZEX_2, CurSelRect.bottom() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY))))                           TransfoType = RESIZEDOWNLEFT; // Bottom left
         else if ((NbrSelected > 0) && (IsInRect(event->pos(), QRect(CurSelRect.left() - HANDLESIZEX_2, CurSelRect.top() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY))))                          TransfoType = RESIZEUPLEFT;   // Top left
         else if ((NbrSelected > 0) && (IsInRect(event->pos(), QRect(CurSelRect.left() - HANDLESIZEX_2, CurSelRect.top() + CurSelRect.height() / 2 - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY))))   TransfoType = RESIZELEFT;     // Left
         else if ((NbrSelected > 0) && (IsInRect(event->pos(), QRect(CurSelRect.right() - HANDLESIZEX_2, CurSelRect.top() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY))))                         TransfoType = RESIZEUPRIGHT;  // Top right
         else if ((NbrSelected > 0) && (IsInRect(event->pos(), QRect(CurSelRect.right() - HANDLESIZEX_2, CurSelRect.top() + CurSelRect.height() / 2 - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY))))   TransfoType = RESIZERIGHT;    // Right
         else if ((NbrSelected > 0) && (IsInRect(event->pos(), QRect(CurSelRect.right() - HANDLESIZEX_2, CurSelRect.bottom() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY))))                      TransfoType = RESIZEDOWNRIGHT;// Bottom right
         else if ((NbrSelected > 0) && (IsInRect(event->pos(), QRect(CurSelRect.left() + CurSelRect.width() / 2 - HANDLESIZEX_2, CurSelRect.top() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY))))     TransfoType = RESIZEUP;       // Top
         else if ((NbrSelected > 0) && (IsInRect(event->pos(), QRect(CurSelRect.left() + CurSelRect.width() / 2 - HANDLESIZEX_2, CurSelRect.bottom() - HANDLESIZEX_2, HANDLESIZEX, HANDLESIZEY))))  TransfoType = RESIZEDOWN;     // Bottom
         else
         {
            // Move
            if (NbrSelected == 0 || !IsInSelectedRect(event->pos()))
            {

               // Replace current selection
               BlockTable->clearSelection();
               int i = BlockTable->CompositionList->compositionObjects.count() - 1;
               while (i >= 0)
               {
                  if (BlockTable->CompositionList->compositionObjects.at(i)->isVisible())
                  {
                     QRectF ObjRect = ApplyModifAndScaleFactors(i, SceneRect, true);
                     if (IsInRect(event->pos(), ObjRect))
                     {
                        BlockTable->clearSelection();
                        BlockTable->setCurrentCell(i, 0, QItemSelectionModel::Current | QItemSelectionModel::Select);
                        break;
                     }
                  }
                  i--;
               }
            }
            else if (IsInSelectedRect(event->pos()))
            {
               TransfoType = MOVEBLOCK;
               setCursor(Qt::ClosedHandCursor);
            }
         }

         if (TransfoType != NOTYETDEFINED)
         {
            IsCapture = true;
            CapturePos = event->pos();
            Move_X = 0;
            Move_Y = 0;
            Scale_X = 0;
            Scale_Y = 0;
         }
      }
   }
   emit EndSelectionChange();
}

//====================================================================================================================

void cInteractiveZone::mouseReleaseEvent(QMouseEvent *event) 
{
   //ToLog(LOGMSG_DEBUGTRACE, "IN:cInteractiveZone::mouseReleaseEvent");
   if (!BlockTable || !BlockTable->CompositionList || !IsCapture) 
      return;

   IsCapture = false;

   // Block move
   if (Move_X != 0 || Move_Y != 0 || Scale_X != 0 || Scale_Y != 0)
      emit TransformBlock(Move_X, Move_Y, Scale_X, Scale_Y, RSel_X, RSel_Y, RSel_W, RSel_H);
   ManageCursor(event->pos(), event->modifiers());
}

QVector<QRect> cInteractiveZone::createHandleRects(QRectF r)
{
   QVector<QRect> v;  
   //#define  HANDLESIZEX_2 HANDLESIZEX / 2
   
   v.append( QRect(r.left() - HANDLESIZEX_2,                 r.bottom() - HANDLESIZEY_2,               HANDLESIZEX, HANDLESIZEY)); // Bottom left
   v.append( QRect(r.left() - HANDLESIZEX_2,                 r.top() - HANDLESIZEY_2,                  HANDLESIZEX, HANDLESIZEY)); // Top left
   v.append( QRect(r.left() - HANDLESIZEX_2,                 r.top() + r.height() / 2 - HANDLESIZEY_2, HANDLESIZEX, HANDLESIZEY)); // Left
   v.append( QRect(r.right() - HANDLESIZEX_2,                r.top() - HANDLESIZEY_2,                  HANDLESIZEX, HANDLESIZEY)); // Top right
   v.append( QRect(r.right() - HANDLESIZEX_2,                r.top() + r.height() / 2 - HANDLESIZEY_2, HANDLESIZEX, HANDLESIZEY)); // Right
   v.append( QRect(r.right() - HANDLESIZEX_2,                r.bottom() - HANDLESIZEY_2,               HANDLESIZEX, HANDLESIZEY)); // Bottom right
   v.append( QRect(r.left() + r.width() / 2 - HANDLESIZEX_2, r.top() - HANDLESIZEY_2,                  HANDLESIZEX, HANDLESIZEY)); // Top
   v.append( QRect(r.left() + r.width() / 2 - HANDLESIZEX_2, r.bottom() - HANDLESIZEY_2,               HANDLESIZEX, HANDLESIZEY)); // Bottom
   return v;
}

void cInteractiveZone::resizeEvent(QResizeEvent * ev)
{
   Q_UNUSED(ev)
   //qDebug() << "resizeEvent from " << ev->oldSize() << " to " << this->size();
   RefreshDisplay();
}

void cInteractiveZone::setGeometry(const QRect &r)
{
   QWidget::setGeometry(r);
   //qDebug() << "setGeometry " << r;
}

void cInteractiveZone::setGeometry(int x, int y, int w, int h)
{
   QWidget::setGeometry(x,y,w,h);
   //qDebug() << "setGeometry " << QRect(x,y,w,h);
}

void cInteractiveZone::wheelEvent(QWheelEvent * event)
{
   if( IsCapture )
      return;
   //double StepX = 1.0 / SceneRect.width();
   //double StepY = 1.0 / SceneRect.height();
   double StepX = 1.0 / DisplayW;
   double StepY = 1.0 / DisplayH;
   //double ScaleY = 0.0;
   //double ScaleX = 0.0;
   QRectF selRect(RSel_X, RSel_Y,RSel_W, RSel_H);
   Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
   qreal step = (event->angleDelta().y()/120);
   if( modifiers & Qt::ControlModifier )
      step *= 10.0;
   //step /= 100;
   switch(wheelMode)
   {
      case eMoveHor:
         step =  step * StepX;
         //emit TransformBlock(step, 0, 0, 0, 0, 0, 1.0, 1.0);
         emit TransformBlocks(step,0,0,0,selRect);
      break;

      case eMoveVer:
         step =  step * StepY;
         emit TransformBlocks(0,step,0,0,selRect);
      break;

      case eResize:
         Move_Y = -(step*StepY)/2; 
         Scale_Y = step*StepY; 
         Move_X = LockGeometry ? ((Move_Y*DisplayH) / AspectRatio) / DisplayW : 0; 
         Scale_X = LockGeometry ? ((Scale_Y*DisplayH) / AspectRatio) / DisplayW : 0; 
         emit TransformBlocks(Move_X,Move_Y,Scale_X,Scale_Y,selRect);
      break;

      case eRotateX:
         emit RotateBlocks(step,0,0);
      break;

      case eRotateY:
         emit RotateBlocks(0,step,0);
      break;

      case eRotateZ:
         emit RotateBlocks(0,0,step);
      break;

      case eZoomInOut:
         //if( NbrSelected == 1 )
         //{
         //   QRectF selRect = ComputeNewCurSelRect();
         //   QPointF nPos(event->pos().x() - SceneRect.left(), event->pos().y() - SceneRect.top());
         //   qDebug() << event->pos() << " " << IsInSelectedRect(event->pos()) << " " << selRect << " " << nPos;
         //}
         emit ZoomSlide(step/200.0, 0.0, 0.0);
      break;

      case eSlideHorizontal:
         emit ZoomSlide(0.0, step/100, 0.0);
      break;

      case eSlideVertical:
         emit ZoomSlide(0.0, 0.0, step/100);
      break;
   }
   if (!IsRefreshQueued)
   {
      IsRefreshQueued = true;
      QTimer::singleShot(LATENCY, this, SLOT(RefreshDisplay()));
   }
}

//====================================================================================================================
bool cInteractiveZone::isInGap(qreal pos, qreal gapCenter, qreal gapSize2)
{
   return pos > (gapCenter - gapSize2) && pos < (gapCenter + gapSize2);
}

//====================================================================================================================
bool cInteractiveZone::snap(qreal pos, qreal mag, int range)
{
   return pos >= (mag - range) && pos <= (mag + range);
}
