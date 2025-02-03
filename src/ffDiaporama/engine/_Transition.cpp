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

#include "_Transition.h"
#include "cBrushDefinition.h"
#include "ImageFilters.h"
#ifndef _MSC_VER
#include <x86intrin.h>
#endif
//#include "qt_cuda.h"
#define noMEASURE_TRANSITION

// static global values
cLumaList LumaList_Bar;
cLumaList LumaList_Box;
cLumaList LumaList_Center;
cLumaList LumaList_Checker;
cLumaList LumaList_Clock;
cLumaList LumaList_Snake;
#ifdef LUMA_TEST
cLumaList LumaList_Test;
#endif
cIconList IconList;

QList<TransitionInterface *> exTransitions;

TransitionInterface *getTransitionInterface(int familyID)
{
   foreach(TransitionInterface *ti, exTransitions)
   {
      if (ti->familyID() == familyID)
      {
         return ti;
      }
   }
   return 0;
}
// not used:
//// static local values use to speed background image loading (cache)
//QImage          *LastLoadedBackgroundImage      =NULL;
//QString         LastLoadedBackgroundImageName   ="";

// static local values use to work with luma images
int  LUMADLG_WIDTH=0;
QImage *LumaTmpImage = NULL;

//*********************************************************************************************************************************************
// Global class containing icons of transitions
//*********************************************************************************************************************************************
cIconObject::cIconObject(TRFAMILY TheTransitionFamily,int TheTransitionSubType) 
{
   TransitionFamily = TheTransitionFamily;
   TransitionSubType = TheTransitionSubType;
   //QString Family = QString("%1").arg(TransitionFamily, 2, 10, QChar('0'));
   //QString SubType = QString("%1").arg(TransitionSubType, 2, 10, QChar('0'));
   //QString FileName = QString(":/img/Transitions")+QDir::separator()+QString("tr-")+Family+QString("-")+SubType+QString(".png");
   QString FileName = QString(":/img/Transitions/tr-%1-%2.png")
      .arg(TransitionFamily, 2, 10, QChar('0'))
      .arg(TransitionSubType, 2, 10, QChar('0'));
   Icon = QImage(FileName);
   if (Icon.isNull()) 
   {
      Icon = QImage(QString(":/img/Transitions") + QDir::separator() + QString("tr-icon-error.png"));
      ToLog(LOGMSG_WARNING,"Icon not found:"+QDir(FileName).absolutePath());
   }
}

//====================================================================================================================

cIconObject::cIconObject(TRFAMILY TheTransitionFamily,int TheTransitionSubType,cLumaObject *Luma) 
{
   TransitionFamily = TheTransitionFamily;
   TransitionSubType = TheTransitionSubType;
   if (Luma->OriginalLuma.isNull()) 
      Icon = Luma->GetLuma(32, 32); 
   else 
      Icon = Luma->OriginalLuma.scaled(QSize(32, 32), Qt::IgnoreAspectRatio/*,Qt::SmoothTransformation*/);
}

//*********************************************************************************************************************************************
// Global class containing icons library
//*********************************************************************************************************************************************

cIconList::cIconList() 
{
}

//====================================================================================================================

cIconList::~cIconList() 
{
    List.clear();
}

//====================================================================================================================

QImage *cIconList::GetIcon(TRFAMILY TransitionFamily,int TransitionSubType) 
{
   int i = 0;
   while ((i < List.count()) && ((List[i].TransitionFamily != TransitionFamily) || (List[i].TransitionSubType != TransitionSubType))) 
      i++;
   if (i < List.count()) 
      return new QImage(List[i].Icon);
   else 
      return new QImage(":/img/Transitions/tr-icon-error.png");
}

//*********************************************************************************************************************************************
// Global class for luma object
//*********************************************************************************************************************************************
cLumaObject::cLumaObject(TRFAMILY TrFamily,int TrSubType,QString FileName) 
{
   TransitionSubType = TrSubType;
   TransitionFamily = TrFamily;
   if (FileName.isEmpty())
   {
      DlgLumaImage = GetLuma(LUMADLG_WIDTH, LUMADLG_HEIGHT);
   }
   else
   {
      OriginalLuma = QImage(FileName);
      DlgLumaImage = QImage(OriginalLuma.scaled(LUMADLG_WIDTH, LUMADLG_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)).convertToFormat(QImage::Format_ARGB32_Premultiplied);
      Name = QFileInfo(FileName).baseName();
   }
   lWidth = lHeight = 0;
}

QImage cLumaObject::GetLuma(int DestImageWith, int DestImageHeight)
{
   QImage Luma;
   //qDebug() << "get Luma for " << DestImageWith << "x" << DestImageHeight;
   if (DestImageWith == LUMADLG_WIDTH && DestImageHeight == LUMADLG_HEIGHT)
      Luma = DlgLumaImage;
   if (lWidth == DestImageWith &&  lHeight == DestImageHeight && TransitionFamily == lastTransitionFamily && !lastLumaImage.isNull())
      return lastLumaImage;
   if (Luma.isNull())
      switch (TransitionFamily)
      {
         case TRANSITIONFAMILY_LUMA_BAR:    
            Luma = GetLumaBar(DestImageWith, DestImageHeight);     
         break;
         case TRANSITIONFAMILY_LUMA_CLOCK:  
            Luma = GetLumaClock(DestImageWith, DestImageHeight);   
         break;
         default:
         break;
      }
   //if (Luma.isNull()) Luma=OriginalLuma.scaled(QSize(DestImageWith,DestImageHeight),Qt::IgnoreAspectRatio,Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32_Premultiplied);
   //if (Luma.isNull()) Luma=OriginalLuma.scaled(QSize(DestImageWith,DestImageHeight),Qt::KeepAspectRatio,Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32_Premultiplied);
   //if (Luma.isNull()) Luma=OriginalLuma.scaled(QSize(DestImageWith,DestImageHeight),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32_Premultiplied);
   if (Luma.isNull())
   {
      //qDebug() << "get Luma for " << DestImageWith << "x" << DestImageHeight;
      QImage scaled = OriginalLuma.scaledToWidth(DestImageWith, Qt::SmoothTransformation);
      Luma = scaled.copy(0, (scaled.height() - DestImageHeight) / 2, DestImageWith, DestImageHeight).convertToFormat(QImage::Format_ARGB32_Premultiplied);
      //qDebug() << "Luma is " << scaled.width() << "x" << scaled.height();
      //Luma=OriginalLuma.scaled(QSize(DestImageWith,DestImageHeight),Qt::KeepAspectRatio,Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32_Premultiplied);
   }
   lastLumaImage = Luma;
   lWidth = DestImageWith;
   lHeight = DestImageHeight;
   lastTransitionFamily = TransitionFamily;
   return Luma;
}

void cLumaObject::clearCache()
{
    lastLumaImage = QImage();
    lWidth = 0; 
    lHeight = 0;
    lastTransitionFamily = (TRFAMILY)0;
}

// Compute image for some luma to improve quality
QImage cLumaObject::GetLumaBar(int DestImageWith,int DestImageHeight) {
   //qDebug() << "getLumaBar " <<DestImageWith <<"x"  << DestImageHeight;
    QPainter        P;
    QLinearGradient Gradient;
    int             i,With,Height,Cur;
    int             Max=DestImageWith;      if (Max<DestImageHeight) Max=DestImageHeight;

    QImage RetImage(DestImageWith,DestImageHeight,QImage::Format_ARGB32_Premultiplied);
    P.begin(&RetImage);
    switch (TransitionSubType) {
        case Bar128:        Gradient=QLinearGradient(QPointF(DestImageWith/2,0),QPointF(DestImageWith/2,DestImageHeight));
                            Gradient.setColorAt(0,Qt::white);
                            Gradient.setColorAt(1,Qt::black);
                            P.fillRect(0,0,DestImageWith,DestImageHeight,QBrush(Gradient));
                            break;
        case Bar182:        Gradient=QLinearGradient(QPointF(DestImageWith/2,0),QPointF(DestImageWith/2,DestImageHeight));
                            Gradient.setColorAt(0,Qt::black);
                            Gradient.setColorAt(1,Qt::white);
                            P.fillRect(0,0,DestImageWith,DestImageHeight,QBrush(Gradient));
                            break;
        case Bar146:        Gradient=QLinearGradient(QPointF(0,DestImageHeight/2),QPointF(DestImageWith,DestImageHeight/2));
                            Gradient.setColorAt(0,Qt::black);
                            Gradient.setColorAt(1,Qt::white);
                            P.fillRect(0,0,DestImageWith,DestImageHeight,QBrush(Gradient));
                            break;
        case Bar164:        Gradient=QLinearGradient(QPointF(0,DestImageHeight/2),QPointF(DestImageWith,DestImageHeight/2));
                            Gradient.setColorAt(0,Qt::white);
                            Gradient.setColorAt(1,Qt::black);
                            P.fillRect(0,0,DestImageWith,DestImageHeight,QBrush(Gradient));
                            break;
        case Bar219:        Gradient=QLinearGradient(QPointF(0,Max),QPointF(Max,0));
                            Gradient.setColorAt(0,Qt::black);
                            Gradient.setColorAt(1,Qt::white);
                            P.scale(qreal(DestImageWith)/qreal(Max),qreal(DestImageHeight)/qreal(Max));
                            P.fillRect(0,0,Max,Max,QBrush(Gradient));
                            break;
        case Bar291:        Gradient=QLinearGradient(QPointF(0,Max),QPointF(Max,0));
                            Gradient.setColorAt(0,Qt::white);
                            Gradient.setColorAt(1,Qt::black);
                            P.scale(qreal(DestImageWith)/qreal(Max),qreal(DestImageHeight)/qreal(Max));
                            P.fillRect(0,0,Max,Max,QBrush(Gradient));
                            break;
        case Bar237:        Gradient=QLinearGradient(QPointF(0,0),QPointF(Max,Max));
                            Gradient.setColorAt(0,Qt::white);
                            Gradient.setColorAt(1,Qt::black);
                            P.scale(qreal(DestImageWith)/qreal(Max),qreal(DestImageHeight)/qreal(Max));
                            P.fillRect(0,0,Max,Max,QBrush(Gradient));
                            break;
        case Bar273:        Gradient=QLinearGradient(QPointF(0,0),QPointF(Max,Max));
                            Gradient.setColorAt(0,Qt::black);
                            Gradient.setColorAt(1,Qt::white);
                            P.scale(qreal(DestImageWith)/qreal(Max),qreal(DestImageHeight)/qreal(Max));
                            P.fillRect(0,0,Max,Max,QBrush(Gradient));
                            break;
        case BilinearA1:    Gradient=QLinearGradient(QPointF(0,0),QPointF(Max,Max));
                            Gradient.setColorAt(0,Qt::black);
                            Gradient.setColorAt(0.5,Qt::white);
                            Gradient.setColorAt(1,Qt::black);
                            P.scale(qreal(DestImageWith)/qreal(Max),qreal(DestImageHeight)/qreal(Max));
                            P.fillRect(0,0,Max,Max,QBrush(Gradient));
                            break;
        case BilinearA9:    Gradient=QLinearGradient(QPointF(0,Max),QPointF(Max,0));
                            Gradient.setColorAt(0,Qt::black);
                            Gradient.setColorAt(0.5,Qt::white);
                            Gradient.setColorAt(1,Qt::black);
                            P.scale(qreal(DestImageWith)/qreal(Max),qreal(DestImageHeight)/qreal(Max));
                            P.fillRect(0,0,Max,Max,QBrush(Gradient));
                            break;
        case BilinearA4:    Gradient=QLinearGradient(QPointF(0,DestImageHeight/2),QPointF(DestImageWith,DestImageHeight/2));
                            Gradient.setColorAt(0,Qt::black);
                            Gradient.setColorAt(0.5,Qt::white);
                            Gradient.setColorAt(1,Qt::black);
                            P.fillRect(0,0,DestImageWith,DestImageHeight,QBrush(Gradient));
                            break;
        case BilinearA8:    Gradient=QLinearGradient(QPointF(DestImageWith/2,0),QPointF(DestImageWith/2,DestImageHeight));
                            Gradient.setColorAt(0,Qt::black);
                            Gradient.setColorAt(0.5,Qt::white);
                            Gradient.setColorAt(1,Qt::black);
                            P.fillRect(0,0,DestImageWith,DestImageHeight,QBrush(Gradient));
                            break;
        case BilinearB1:    Gradient=QLinearGradient(QPointF(0,0),QPointF(Max,Max));
                            Gradient.setColorAt(0,Qt::white);
                            Gradient.setColorAt(0.5,Qt::black);
                            Gradient.setColorAt(1,Qt::white);
                            P.scale(qreal(DestImageWith)/qreal(Max),qreal(DestImageHeight)/qreal(Max));
                            P.fillRect(0,0,Max,Max,QBrush(Gradient));
                            break;
        case BilinearB9:    Gradient=QLinearGradient(QPointF(0,Max),QPointF(Max,0));
                            Gradient.setColorAt(0,Qt::white);
                            Gradient.setColorAt(0.5,Qt::black);
                            Gradient.setColorAt(1,Qt::white);
                            P.scale(qreal(DestImageWith)/qreal(Max),qreal(DestImageHeight)/qreal(Max));
                            P.fillRect(0,0,Max,Max,QBrush(Gradient));
                            break;
        case BilinearB4:    Gradient=QLinearGradient(QPointF(0,DestImageHeight/2),QPointF(DestImageWith,DestImageHeight/2));
                            Gradient.setColorAt(0,Qt::white);
                            Gradient.setColorAt(0.5,Qt::black);
                            Gradient.setColorAt(1,Qt::white);
                            P.fillRect(0,0,DestImageWith,DestImageHeight,QBrush(Gradient));
                            break;
        case BilinearB8:    Gradient=QLinearGradient(QPointF(DestImageWith/2,0),QPointF(DestImageWith/2,DestImageHeight));
                            Gradient.setColorAt(0,Qt::white);
                            Gradient.setColorAt(0.5,Qt::black);
                            Gradient.setColorAt(1,Qt::white);
                            P.fillRect(0,0,DestImageWith,DestImageHeight,QBrush(Gradient));
                            break;
        case StoreA446:     With=DestImageWith/4;
                            Cur=0;
                            for (i=0;i<4;i++) {
                                if (i==3) With=DestImageWith-Cur;
                                Gradient=QLinearGradient(QPointF(Cur,DestImageHeight/2),QPointF(Cur+With,DestImageHeight/2));
                                Gradient.setColorAt(0,Qt::black);
                                Gradient.setColorAt(1,Qt::white);
                                P.fillRect(Cur,0,With,DestImageHeight,QBrush(Gradient));
                                Cur=Cur+With;
                            }
                            break;
        case StoreA846:     With=DestImageWith/8;
                            Cur=0;
                            for (i=0;i<8;i++) {
                                if (i==7) With=DestImageWith-Cur;
                                Gradient=QLinearGradient(QPointF(Cur,DestImageHeight/2),QPointF(Cur+With,DestImageHeight/2));
                                Gradient.setColorAt(0,Qt::black);
                                Gradient.setColorAt(1,Qt::white);
                                P.fillRect(Cur,0,With,DestImageHeight,QBrush(Gradient));
                                Cur=Cur+With;
                            }
                            break;
        case StoreB482:     Height=DestImageHeight/4;
                            Cur=0;
                            for (i=0;i<4;i++) {
                                if (i==3) Height=DestImageHeight-Cur;
                                Gradient=QLinearGradient(QPointF(DestImageWith/2,Cur),QPointF(DestImageWith/2,Cur+Height));
                                Gradient.setColorAt(0,Qt::black);
                                Gradient.setColorAt(1,Qt::white);
                                P.fillRect(0,Cur,DestImageWith,Height,QBrush(Gradient));
                                Cur=Cur+Height;
                            }
                            break;
        case StoreB882:     Height=DestImageHeight/8;
                            Cur=0;
                            for (i=0;i<8;i++) {
                                if (i==7) Height=DestImageHeight-Cur;
                                Gradient=QLinearGradient(QPointF(DestImageWith/2,Cur),QPointF(DestImageWith/2,Cur+Height));
                                Gradient.setColorAt(0,Qt::black);
                                Gradient.setColorAt(1,Qt::white);
                                P.fillRect(0,Cur,DestImageWith,Height,QBrush(Gradient));
                                Cur=Cur+Height;
                            }
                            break;
        case ZBar01:        Height=DestImageHeight/2;
                            Cur=0;
                            Gradient=QLinearGradient(QPointF(0,DestImageHeight/2),QPointF(DestImageWith,DestImageHeight/2));
                            for (i=0;i<2;i++) {
                                if (i==0) {
                                    Gradient.setColorAt(0,Qt::black);
                                    Gradient.setColorAt(1,Qt::white);
                                } else {
                                    Gradient.setColorAt(0,Qt::white);
                                    Gradient.setColorAt(1,Qt::black);
                                    Height=DestImageHeight-Height;
                                }
                                P.fillRect(0,Cur,DestImageWith,Height,QBrush(Gradient));
                                Cur=Cur+Height;
                            }
                            break;
        case ZBar02:        Height=DestImageHeight/2;
                            Cur=0;
                            Gradient=QLinearGradient(QPointF(0,DestImageHeight/2),QPointF(DestImageWith,DestImageHeight/2));
                            for (i=0;i<2;i++) {
                                if (i==0) {
                                    Gradient.setColorAt(0,Qt::white);
                                    Gradient.setColorAt(1,Qt::black);
                                } else {
                                    Gradient.setColorAt(0,Qt::black);
                                    Gradient.setColorAt(1,Qt::white);
                                    Height=DestImageHeight-Height;
                                }
                                P.fillRect(0,Cur,DestImageWith,Height,QBrush(Gradient));
                                Cur=Cur+Height;
                            }
                            break;
        case ZBar03:        With=DestImageWith/2;
                            Cur=0;
                            Gradient=QLinearGradient(QPointF(DestImageWith/2,0),QPointF(DestImageWith/2,DestImageHeight));
                            for (i=0;i<2;i++) {
                                if (i==0) {
                                    Gradient.setColorAt(0,Qt::black);
                                    Gradient.setColorAt(1,Qt::white);
                                } else {
                                    Gradient.setColorAt(0,Qt::white);
                                    Gradient.setColorAt(1,Qt::black);
                                    With=DestImageWith-With;
                                }
                                P.fillRect(Cur,0,With,DestImageHeight,QBrush(Gradient));
                                Cur=Cur+With;
                            }
                            break;
        case ZBar04:        With=DestImageWith/2;
                            Cur=0;
                            Gradient=QLinearGradient(QPointF(DestImageWith/2,0),QPointF(DestImageWith/2,DestImageHeight));
                            for (i=0;i<2;i++) {
                                if (i==0) {
                                    Gradient.setColorAt(0,Qt::white);
                                    Gradient.setColorAt(1,Qt::black);
                                } else {
                                    Gradient.setColorAt(0,Qt::black);
                                    Gradient.setColorAt(1,Qt::white);
                                    With=DestImageWith-With;
                                }
                                P.fillRect(Cur,0,With,DestImageHeight,QBrush(Gradient));
                                Cur=Cur+With;
                            }
                            break;
        default:            P.end();
                            return QImage();
                            break;
    }
    P.end();
    return RetImage;
}

QImage cLumaObject::GetLumaClock(int DestImageWith,int DestImageHeight) {
    QPainter P;
    qreal    Angle=0;
    QColor   c1,c2,c3;
    int      NbrC=0;
    int      Max=DestImageWith;     if (Max<DestImageHeight) Max=DestImageHeight;

    switch (TransitionSubType) {
        case ClockA1:   Angle=45;   c1=Qt::white;    c2=Qt::black;   c3=Qt::white;  NbrC=3;     break;
        case ClockA2:   Angle=90;   c1=Qt::white;    c2=Qt::black;   c3=Qt::white;  NbrC=3;     break;
        case ClockA3:   Angle=135;  c1=Qt::white;    c2=Qt::black;   c3=Qt::white;  NbrC=3;     break;
        case ClockA4:   Angle=0;    c1=Qt::white;    c2=Qt::black;   c3=Qt::white;  NbrC=3;     break;
        case ClockA6:   Angle=180;  c1=Qt::white;    c2=Qt::black;   c3=Qt::white;  NbrC=3;     break;
        case ClockA7:   Angle=-45;  c1=Qt::white;    c2=Qt::black;   c3=Qt::white;  NbrC=3;     break;
        case ClockA8:   Angle=-90;  c1=Qt::white;    c2=Qt::black;   c3=Qt::white;  NbrC=3;     break;
        case ClockA9:   Angle=-135; c1=Qt::white;    c2=Qt::black;   c3=Qt::white;  NbrC=3;     break;
        case ClockB1:   Angle=-135; c1=Qt::black;    c2=Qt::white;                  NbrC=2;     break;
        case ClockB2:   Angle=-90;  c1=Qt::black;    c2=Qt::white;                  NbrC=2;     break;
        case ClockB3:   Angle=-45;  c1=Qt::black;    c2=Qt::white;                  NbrC=2;     break;
        case ClockB4:   Angle=180;  c1=Qt::black;    c2=Qt::white;                  NbrC=2;     break;
        case ClockB6:   Angle=0;    c1=Qt::black;    c2=Qt::white;                  NbrC=2;     break;
        case ClockB7:   Angle=135;  c1=Qt::black;    c2=Qt::white;                  NbrC=2;     break;
        case ClockB8:   Angle=90;   c1=Qt::black;    c2=Qt::white;                  NbrC=2;     break;
        case ClockB9:   Angle=45;   c1=Qt::black;    c2=Qt::white;                  NbrC=2;     break;
        case ClockC1:   Angle=-135; c1=Qt::white;    c2=Qt::black;                  NbrC=2;     break;
        case ClockC2:   Angle=-90;  c1=Qt::white;    c2=Qt::black;                  NbrC=2;     break;
        case ClockC3:   Angle=-45;  c1=Qt::white;    c2=Qt::black;                  NbrC=2;     break;
        case ClockC4:   Angle=180;  c1=Qt::white;    c2=Qt::black;                  NbrC=2;     break;
        case ClockC6:   Angle=0;    c1=Qt::white;    c2=Qt::black;                  NbrC=2;     break;
        case ClockC7:   Angle=135;  c1=Qt::white;    c2=Qt::black;                  NbrC=2;     break;
        case ClockC8:   Angle=90;   c1=Qt::white;    c2=Qt::black;                  NbrC=2;     break;
        case ClockC9:   Angle=45;   c1=Qt::white;    c2=Qt::black;                  NbrC=2;     break;
        default:        return QImage();                                                        break;
    }

    QImage RetImage(DestImageWith,DestImageHeight,QImage::Format_ARGB32_Premultiplied);
    P.begin(&RetImage);
    P.scale(qreal(DestImageWith)/qreal(Max),qreal(DestImageHeight)/qreal(Max));
    QConicalGradient Gradient(Max/2,Max/2,Angle);
    Gradient.setColorAt(0,c1);
    if (NbrC==2) Gradient.setColorAt(1,c2); else { Gradient.setColorAt(0.5,c2); Gradient.setColorAt(1,c3); }
    P.fillRect(0,0,Max,Max,QBrush(Gradient));
    P.end();
    //    RetImage.save(QString("/home/dominique/2/%1.png").arg(Name),"PNG");
    return RetImage;
}

//*********************************************************************************************************************************************
// Global class containing luma library
//*********************************************************************************************************************************************

cLumaList::cLumaList() 
{
    Geometry=GEOMETRY_16_9;
    LUMADLG_WIDTH=int((double(LUMADLG_HEIGHT)/double(9))*double(16));
}

//====================================================================================================================

cLumaList::~cLumaList() 
{
    List.clear();
}

//====================================================================================================================

void cLumaList::ScanDisk(QString Path,TRFAMILY TransitionFamily) 
{
   bool ScanDisk=true;
   int  i;

   List.clear();
   switch (TransitionFamily) 
   {
      case TRANSITIONFAMILY_LUMA_BAR:
         for ( i = 0; i < TRANSITIONMAXSUBTYPE_LUMABAR; i++) 
            List.append(cLumaObject(TransitionFamily,List.count(),""));
         ScanDisk = false;
      break;
      case TRANSITIONFAMILY_LUMA_CLOCK:
         for ( i = 0; i < TRANSITIONMAXSUBTYPE_LUMACLOCK; i++) 
            List.append(cLumaObject(TransitionFamily,List.count(),""));
      default:
         break;
   }
   if (ScanDisk) 
   {
      QDir Folder(Path);
      QFileInfoList Files = Folder.entryInfoList();
      for (int i = 0; i < Files.count(); i++) 
         if (Files[i].isFile() && ( QString(Files[i].suffix()).toLower() == "png" || QString(Files[i].suffix()).toLower() == "pgm" || QString(Files[i].suffix()).toLower() == "svg" || QString(Files[i].suffix()).toLower() == "jpg") )
            List.append(cLumaObject(TransitionFamily,List.count(),Files[i].absoluteFilePath()));
      // Sort list by name
      for (int i = 0; i < List.count(); i++) 
         for (int j = 0; j < List.count()-1; j++) 
            if (List[j].Name > List[j+1].Name) 
               List.QT_SWAP(j,j+1);
   }
   // Register icons for this list
   for (int i = 0; i < List.count(); i++) 
      IconList.List.append(cIconObject(TransitionFamily,i,&List[i]));
}

//====================================================================================================================

void cLumaList::SetGeometry(ffd_GEOMETRY TheGeometry) 
{
    if (Geometry==TheGeometry) return;
    Geometry=TheGeometry;
    switch (Geometry) {
    case GEOMETRY_4_3   : LUMADLG_WIDTH=int((double(LUMADLG_HEIGHT)/double(3))*double(4));    break;
    case GEOMETRY_16_9  : LUMADLG_WIDTH=int((double(LUMADLG_HEIGHT)/double(9))*double(16));   break;
    case GEOMETRY_40_17 :
    default             : LUMADLG_WIDTH=int((double(LUMADLG_HEIGHT)/double(17))*double(40));  break;
    }
    for (int i=0;i<List.count();i++) {
        List[i].DlgLumaImage=QImage();
        List[i].DlgLumaImage=List[i].GetLuma(LUMADLG_WIDTH,LUMADLG_HEIGHT);
    }
}

void cLumaList::clearCache()
{
    for (int i=0;i<List.count();i++) 
        List[i].clearCache();
}

void cLumaList::releaseAll()
{
   clearCache();
   List.clear();
   IconList.List.clear();
}

//============================================================================================
// Public utility function use to register transitions
//============================================================================================

int RegisterNoLumaTransitions() 
{
   exTransitions.append(new TransitionsBasic());
   exTransitions.append(new TransitionsZoom());
   exTransitions.append(new TransitionsSlide());
   exTransitions.append(new TransitionsPush());
   exTransitions.append(new TransitionsDeform());
   foreach(TransitionInterface *ti, exTransitions)
   {
      for (int i = 0; i < ti->numTransitions(); i++)
         IconList.List.append(cIconObject((TRFAMILY)ti->familyID(), i));
   }
   //for(int i = 0; i < TRANSITIONMAXSUBTYPE_BASE; i++)       IconList.List.append(cIconObject(TRANSITIONFAMILY_BASE,i));
   //for(int i = 0; i < TRANSITIONMAXSUBTYPE_ZOOMINOUT; i++)  IconList.List.append(cIconObject(TRANSITIONFAMILY_ZOOMINOUT,i));
   //for(int i = 0; i < TRANSITIONMAXSUBTYPE_SLIDE; i++)      IconList.List.append(cIconObject(TRANSITIONFAMILY_SLIDE,i));
   //for(int i = 0; i < TRANSITIONMAXSUBTYPE_PUSH; i++)       IconList.List.append(cIconObject(TRANSITIONFAMILY_PUSH,i));
   //for(int i = 0; i < TRANSITIONMAXSUBTYPE_DEFORM; i++)     IconList.List.append(cIconObject(TRANSITIONFAMILY_DEFORM,i));
   return IconList.List.count();
}

void ReleaseNoLumaTransitions()
{
   while (!exTransitions.isEmpty())
      delete exTransitions.takeFirst();
}

int RegisterLumaTransitions()
{
   int     PreviousListNumber = IconList.List.count();
   QString Path;

   exTransitions.append(new TransitionsLumaBar());
   //Path = "luma/Bar";     LumaList_Bar.ScanDisk(Path, TRANSITIONFAMILY_LUMA_BAR);
   exTransitions.append(new TransitionsLumaBox());
   //Path = "luma/Box";     LumaList_Box.ScanDisk(Path, TRANSITIONFAMILY_LUMA_BOX);
   exTransitions.append(new TransitionsLumaCenter());
   //Path = "luma/Center";  LumaList_Center.ScanDisk(Path, TRANSITIONFAMILY_LUMA_CENTER);
   exTransitions.append(new TransitionsLumaChecker());
   //Path = "luma/Checker"; LumaList_Checker.ScanDisk(Path, TRANSITIONFAMILY_LUMA_CHECKER);
   exTransitions.append(new TransitionsLumaClock());
   //Path = "luma/Clock";   LumaList_Clock.ScanDisk(Path, TRANSITIONFAMILY_LUMA_CLOCK);
   exTransitions.append(new TransitionsLumaSnake());
   //Path = "luma/Snake";   LumaList_Snake.ScanDisk(Path, TRANSITIONFAMILY_LUMA_SNAKE);
   #ifdef LUMA_TEST
   //Path = "luma/videoporama";      LumaList_Test.ScanDisk(Path, TRANSITIONFAMILY_LUMA_TEST);
   //Path = "luma/openshot/common";  LumaList_Test.ScanDisk(Path, TRANSITIONFAMILY_LUMA_TEST);
   Path = "luma/openshot/extra";  LumaList_Test.ScanDisk(Path, TRANSITIONFAMILY_LUMA_TEST);
   #endif
   return IconList.List.count() - PreviousListNumber;
}

void ReleaseLumaTransitions()
{
   LumaList_Bar.releaseAll();
   LumaList_Box.releaseAll();
   LumaList_Center.releaseAll();
   LumaList_Checker.releaseAll();
   LumaList_Clock.releaseAll();
   LumaList_Snake.releaseAll();
#ifdef LUMA_TEST
   LumaList_Test.releaseAll();
#endif
   delete LumaTmpImage;
   LumaTmpImage = NULL;
}

void clearLumaCache()
{
   LumaList_Bar.clearCache();
   LumaList_Center.clearCache();
   LumaList_Center.clearCache();
   LumaList_Checker.clearCache();
   LumaList_Clock.clearCache();
   LumaList_Box.clearCache();
   LumaList_Snake.clearCache();
#ifdef LUMA_TEST
   LumaList_Test.clearCache();
#endif
}

//============================================================================================
// Private utility function use to rotate an image
//============================================================================================

QImage RotateImage(double TheRotateXAxis,double TheRotateYAxis,double TheRotateZAxis,QImage *OldImg) 
{
    double dw=double(OldImg->width());
    double dh=double(OldImg->height());
    double hyp=sqrt(dw*dw+dh*dh);

    QImage   Img(hyp,hyp,QImage::Format_ARGB32_Premultiplied);
    QPainter Painter;
    Painter.begin(&Img);
    Painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing|QPainter::SmoothPixmapTransform);
    Painter.setCompositionMode(QPainter::CompositionMode_Source);
    Painter.fillRect(QRect(0,0,hyp,hyp),Qt::transparent);
    Painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // All coordonates from center
    QTransform  Matrix;
    Matrix.translate(hyp/2,hyp/2);
    if (TheRotateZAxis!=0) Matrix.rotate(TheRotateZAxis,Qt::ZAxis);   // Standard axis
    if (TheRotateXAxis!=0) Matrix.rotate(TheRotateXAxis,Qt::XAxis);   // Rotate from X axis
    if (TheRotateYAxis!=0) Matrix.rotate(TheRotateYAxis,Qt::YAxis);   // Rotate from Y axis
    Painter.setWorldTransform(Matrix,false);
    Painter.drawImage(-(dw)/2,-(dh)/2,*OldImg);

    Painter.end();
    return Img;
}

//============================================================================================
//  Basic transition
//      0       No transition
//      1       Dissolve with gradual disappearance of the image A
//      2       Dissolve with no modification of the image A
//      3       Fade to black
//      4       Fade with blur
//============================================================================================

void Transition_Basic(int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int,int) 
{
   QImage  ImgA,ImgB;
   int     DestImageWith  =ImageA->width();
   int     DestImageHeight=ImageA->height();
   int     MaxRA,MaxRB;

   WorkingPainter->save();
   switch (TransitionSubType) 
   {
      case 0:
         WorkingPainter->drawImage(0,0,*ImageB);
         break;
      case 1:     // Dissolve with gradual disappearance of the image A
         WorkingPainter->setOpacity(1-PCT);
         WorkingPainter->drawImage(0,0,*ImageA);
         WorkingPainter->setOpacity(PCT);
         WorkingPainter->drawImage(0,0,*ImageB);
         //WorkingPainter->setOpacity(1);
         break;
      case 2:     // Dissolve with no modification of the image A
         //WorkingPainter->setOpacity(1-PCT);
         WorkingPainter->drawImage(0,0,*ImageA);
         WorkingPainter->setOpacity(PCT);
         WorkingPainter->drawImage(0,0,*ImageB);
         //WorkingPainter->setOpacity(1);
         break;
      case 3:     // Fade to black
         if (PCT<0.5) {
            WorkingPainter->setOpacity(1-PCT*2);
            WorkingPainter->drawImage(0,0,*ImageA);
         } else {
            WorkingPainter->setOpacity((PCT-0.5)*2);
            WorkingPainter->drawImage(0,0,*ImageB);
         }
         //WorkingPainter->setOpacity(1);
         break;
      case 4 :    // Blur
         if (PCT<0.5) 
         {
            ImgA = ImageA->scaledToHeight(DestImageHeight/4);
            MaxRA = ImgA.width()/4; 
            if (MaxRA > ImgA.height()/4) 
               MaxRA = ImgA.height()/4;
            ffdFilter::FltBlur(ImgA,int(PCT*MaxRA));
            WorkingPainter->drawImage(QRect(0,0,DestImageWith,DestImageHeight),ImgA,QRect(0,0,ImgA.width(),ImgA.height()));
            if (PCT>0.4) 
            {
               WorkingPainter->setOpacity((PCT-0.4)*5);
               ImgB =ImageB->scaledToHeight(DestImageHeight/4);
               MaxRB = ImgB.width()/4; 
               if (MaxRB > ImgB.height()/4) 
                  MaxRB = ImgB.height()/4;
               ffdFilter::FltBlur(ImgB,int((0.5-(PCT/2))*MaxRB));
               WorkingPainter->drawImage(QRect(0,0,DestImageWith,DestImageHeight),ImgB,QRect(0,0,ImgB.width(),ImgB.height()));
               //WorkingPainter->setOpacity(1);
            }
         } 
         else 
         {
            ImgB = ImageB->scaledToHeight(DestImageHeight/4);
            MaxRB = ImgB.width()/4; 
            if (MaxRB > ImgB.height()/4) 
               MaxRB = ImgB.height()/4;
            ffdFilter::FltBlur(ImgB,int((0.5-(PCT/2))*MaxRB));
            WorkingPainter->drawImage(QRect(0,0,DestImageWith,DestImageHeight),ImgB,QRect(0,0,ImgB.width(),ImgB.height()));
            if (PCT>0.6) 
               WorkingPainter->setOpacity(1);
         }
         break;
   }
   WorkingPainter->restore();
}

//============================================================================================
//  Zoom transition
//      0       ImageA is reduced to Border Left Center
//      1       ImageB is enlarged from Border Left Center
//      2       ImageA is reduced to Border Right Center
//      3       ImageB is enlarged from Border Right Center
//      4       ImageA is reduced to Border Top Center
//      5       ImageB is enlarged from Border Top Center
//      6       ImageA is reduced to Border Bottom Center
//      7       ImageB is enlarged from Border Bottom Center
//      8       ImageA is reduced to Upper Left Corner
//      9       ImageB is enlarged from Upper Left Corner
//      10      ImageA is reduced to Upper Right Corner
//      11      ImageB is enlarged from Upper Right Corner
//      12      ImageA is reduced to Bottom Left Corner
//      13      ImageB is enlarged from Bottom Left Corner
//      14      ImageA is reduced to Bottom Right Corner
//      15      ImageB is enlarged from Bottom Right Corner
//      16      ImageA is reduced to Center
//      17      ImageB is enlarged from Center
//============================================================================================
void Transition_Zoom_ex(int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight) ;
void Transition_Zoom(int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight) 
{
   if( TransitionSubType > 17 )
   {
      Transition_Zoom_ex(TransitionSubType,PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);
      return;
   }
    bool    Reverse=(TransitionSubType & 0x1)==1;
    QPoint  box;
    int     wt= int(double(DestImageWith)*(Reverse?(1-PCT):PCT));
    int     ht= int(double(DestImageHeight)*(Reverse?(1-PCT):PCT));

    switch (TransitionSubType) {
        case 0 :
        case 1 : box=QPoint(0,(DestImageHeight-ht)/2);                      break;  // Border Left Center
        case 2 :
        case 3 : box=QPoint(DestImageWith-wt,(DestImageHeight-ht)/2);       break;  // Border Right Center
        case 4 :
        case 5 : box=QPoint((DestImageWith-wt)/2,0);                        break;  // Border Top Center
        case 6 :
        case 7 : box=QPoint((DestImageWith-wt)/2,DestImageHeight-ht);       break;  // Border Bottom Center
        case 8 :
        case 9 : box=QPoint(0,0);                                           break;  // Upper Left Corner
        case 10:
        case 11: box=QPoint(DestImageWith-wt,0);                            break;  // Upper Right Corner
        case 12:
        case 13: box=QPoint(0,DestImageHeight-ht);                          break;  // Bottom Left Corner
        case 14:
        case 15: box=QPoint(DestImageWith-wt,DestImageHeight-ht);           break;  // Bottom Right Corner
        case 16:
        case 17: box=QPoint((DestImageWith-wt)/2,(DestImageHeight-ht)/2);   break;  // Center
    }

    // Draw transformed image
    if (!Reverse) {
        // Old image will desapear progressively during the second half time of the transition
        if (PCT<0.5) WorkingPainter->drawImage(0,0,*ImageA); else {
            WorkingPainter->setOpacity(1-(PCT-0.5)*2);
            WorkingPainter->drawImage(0,0,*ImageA);
            WorkingPainter->setOpacity(1);
        }
    } else {
        // New image will apear immediatly during the old image is moving out
        WorkingPainter->drawImage(0,0,*ImageB);
    }
    //if( cQtCuda::getCuda()->isOk() )
    //  WorkingPainter->drawImage(box,cQtCuda::getCuda()->scaled((Reverse?*ImageA:*ImageB),wt,ht,Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
    //else
      WorkingPainter->drawImage(box,(Reverse?ImageA:ImageB)->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
    //WorkingPainter->drawImage(box,(Reverse?ImageA:ImageB)->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
}

void Transition_Zoom_ex(int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight) 
{
   TransitionSubType -= 18;
   bool    Reverse = (TransitionSubType & 0x1)==1;
   QPoint  box;
   int     wt = int(double(DestImageWith)*(Reverse?(1-PCT):PCT));
   int     ht = int(double(DestImageHeight)*(Reverse?(1-PCT):PCT));

   switch (TransitionSubType) 
   {
      case 0 :
      case 1 : box = QPoint(0,(DestImageHeight-ht)/2);                      break;  // Border Left Center
      case 2 :       
      case 3 : box = QPoint(DestImageWith-wt,(DestImageHeight-ht)/2);       break;  // Border Right Center
      case 4 :       
      case 5 : box = QPoint((DestImageWith-wt)/2,0);                        break;  // Border Top Center
      case 6 :       
      case 7 : box = QPoint((DestImageWith-wt)/2,DestImageHeight-ht);       break;  // Border Bottom Center
      case 8 :       
      case 9 : box = QPoint(0,0);                                           break;  // Upper Left Corner
      case 10:       
      case 11: box = QPoint(DestImageWith-wt,0);                            break;  // Upper Right Corner
      case 12:       
      case 13: box = QPoint(0,DestImageHeight-ht);                          break;  // Bottom Left Corner
      case 14:       
      case 15: box = QPoint(DestImageWith-wt,DestImageHeight-ht);           break;  // Bottom Right Corner
      case 16:       
      case 17: box = QPoint((DestImageWith-wt)/2,(DestImageHeight-ht)/2);   break;  // Center
   }

   //// Draw transformed image
   //if (!Reverse) 
   //{
   //   // Old image will desapear progressively during the second half time of the transition
   //   if (PCT<0.5) WorkingPainter->drawImage(0,0,*ImageA); else 
   //   {
   //      WorkingPainter->setOpacity(1-(PCT-0.5)*2);
   //      WorkingPainter->drawImage(0,0,*ImageA);
   //      WorkingPainter->setOpacity(1);
   //   }
   //} 
   //else 
   //{
   //   // New image will apear immediatly during the old image is moving out
   //   WorkingPainter->drawImage(0,0,*ImageB);
   //}
   WorkingPainter->drawImage(0,0,(Reverse?*ImageB:*ImageA)/*->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation)*/);
   WorkingPainter->setOpacity(Reverse ? 1- PCT : PCT);
   WorkingPainter->drawImage(box,(Reverse?ImageA:ImageB)->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
   WorkingPainter->setOpacity(1);
}

//============================================================================================
//  Slide transition
//      0       ImageB appear moving since left to right
//      1       ImageB appear moving since right to left
//      2       ImageB appear moving since up to down
//      3       ImageB appear moving since down to up
//      4       ImageB appear moving since the upper left corner
//      5       ImageB appear moving since the upper right corner
//      6       ImageB appear moving since the lower left corner
//      7       ImageB appear moving since the lower right corner
//      8       ImageA disappear moving from left to right
//      9       ImageA disappear moving from right to left
//      10      ImageA disappear moving from up to down
//      11      ImageA disappear moving from down to up
//      12      ImageA disappear moving from the upper left corner
//      13      ImageA disappear moving from the upper right corner
//      14      ImageA disappear moving from the lower left corner
//      15      ImageA disappear moving from the lower right corner
//      16      ImageB is cut into two and each part moves to the sides left and right
//      17      ImageA is cut into two and each part moves from the sides left and right
//      18      ImageB is cut into two and each part moves to the sides top and bottom
//      19      ImageA is cut into two and each part moves from the sides top and bottom
//      20      ImageB is cut into four and each part moves to the sides left and right and top and bottom
//      21      ImageA is cut into four and each part moves from the sides left and right and top and bottom
//============================================================================================

void Transition_Slide(int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight) {
    bool    Reverse=((TransitionSubType<16) &&(TransitionSubType>=8))||
                    ((TransitionSubType>=16)&&((TransitionSubType & 0x1)>0));
    if (Reverse) PCT=(1-PCT);

    QRect   box1,box2,box3,box4,box5,box6,box7,box8;
    int     BoxNum =(TransitionSubType<16)?1:(TransitionSubType<20)?2:3;
    int     PCTW   =int(PCT*double(DestImageWith));
    int     PCTH   =int(PCT*double(DestImageHeight));


    switch (TransitionSubType) {
        case 0 :
        case 8 :    box1=QRect(DestImageWith-PCTW,0,PCTW,DestImageHeight);                      box2=QRect(0,0,PCTW,DestImageHeight);                           break;      // Since left to right
        case 1 :
        case 9 :    box1=QRect(0,0,PCTW,DestImageHeight);                                       box2=QRect(DestImageWith-PCTW,0,PCTW,DestImageHeight);          break;      // Since right to left
        case 2 :
        case 10:    box1=QRect(0,DestImageHeight-PCTH,DestImageWith,PCTH);                      box2=QRect(0,0,DestImageWith,PCTH);                             break;      // Since up to down
        case 3 :
        case 11:    box1=QRect(0,0,DestImageWith,PCTH);                                         box2=QRect(0,DestImageHeight-PCTH,DestImageWith,PCTH);          break;      // Since down to up
        case 4 :
        case 12:    box1=QRect(DestImageWith-PCTW,DestImageHeight-PCTH,PCTW,PCTH);              box2=QRect(0,0,PCTW,PCTH);                                      break;      // Since the upper left corner
        case 5 :
        case 13:    box1=QRect(0,DestImageHeight-PCTH,PCTW,PCTH);                               box2=QRect(DestImageWith-PCTW,0,PCTW,PCTH);                     break;      // Since the upper right corner
        case 6 :
        case 14:    box1=QRect(DestImageWith-PCTW,0,PCTW,PCTH);                                 box2=QRect(0,DestImageHeight-PCTH,PCTW,PCTH);                   break;      // Since the lower left corner
        case 7 :
        case 15:    box1=QRect(0,0,PCTW,PCTH);                                                  box2=QRect(DestImageWith-PCTW,DestImageHeight-PCTH,PCTW,PCTH);  break;      // Since the lower right corner

        // Cut image and slide each part : 2 parts image
        case 16:
        case 17:    PCTW=int(PCT*double(DestImageWith/2));
                    box1=QRect((DestImageWith/2)-PCTW,0,PCTW,DestImageHeight);                  box2=QRect(0,0,PCTW,DestImageHeight);                                       // left part
                    box3=QRect((DestImageWith/2),0,PCTW,DestImageHeight);                       box4=QRect(DestImageWith-PCTW,0,PCTW,DestImageHeight);                      // right part
                    break;      // Since left and right
        case 18:
        case 19:    PCTH=int(PCT*double(DestImageHeight/2));
                    box1=QRect(0,(DestImageHeight/2)-PCTH,DestImageWith,PCTH);                  box2=QRect(0,0,DestImageWith,PCTH);                                         // top part
                    box3=QRect(0,(DestImageHeight/2),DestImageWith,PCTH);                       box4=QRect(0,DestImageHeight-PCTH,DestImageWith,PCTH);                      // bottom part
                    break;      // Since top and bottom

        // Cut image and slide each part : 4 parts image
        case 20:
        case 21:    PCTW=int(PCT*double(DestImageWith/2));
                    PCTH=int(PCT*double(DestImageHeight/2));
                    box1=QRect((DestImageWith/2)-PCTW,(DestImageHeight/2)-PCTH,PCTW,PCTH);      box2=QRect(0,                 0,PCTW,PCTH);                                 // left-top part
                    box3=QRect((DestImageWith/2),     (DestImageHeight/2)-PCTH,PCTW,PCTH);      box4=QRect(DestImageWith-PCTW,0,PCTW,PCTH);                                 // right part

                    box5=QRect((DestImageWith/2)-PCTW,(DestImageHeight/2),     PCTW,PCTH);      box6=QRect(0,                 DestImageHeight-PCTH,PCTW,PCTH);              // left-top part
                    box7=QRect((DestImageWith/2),     (DestImageHeight/2),     PCTW,PCTH);      box8=QRect(DestImageWith-PCTW,DestImageHeight-PCTH,PCTW,PCTH);              // right part
                    break;      // Since top and bottom
    }
    // Draw transformed image
    if (!Reverse) {
        // Old image will desapear progressively during the second half time of the transition
        if (PCT<0.5) WorkingPainter->drawImage(0,0,*ImageA); else {
            WorkingPainter->setOpacity(1-(PCT-0.5)*2);
            WorkingPainter->drawImage(0,0,*ImageA);
            WorkingPainter->setOpacity(1);
        }
    } else {
        // New image will apear immediatly during the old image is moving out
        WorkingPainter->drawImage(0,0,*ImageB);
    }
    WorkingPainter->drawImage(box2,Reverse?*ImageA:*ImageB,box1);
    if (BoxNum>1) WorkingPainter->drawImage(box4,Reverse?*ImageA:*ImageB,box3);
    if (BoxNum>2) {
        WorkingPainter->drawImage(box6,Reverse?*ImageA:*ImageB,box5);
        WorkingPainter->drawImage(box8,Reverse?*ImageA:*ImageB,box7);
    }
}

//============================================================================================
//  Push transition
//      0       ImageB push ImageA Since left to right
//      1       ImageB push ImageA Since right to left
//      2       ImageB push ImageA Since up to down
//      3       ImageB push ImageA Since down to up
//      4       ImageB zoom in from border Left Center   + ImageA zoom out to border Right Center
//      5       ImageB zoom in from border Right Center  + ImageA zoom out to border Left Center
//      6       ImageB zoom in from border Top Center    + ImageA zoom out to border bottom Center
//      7       ImageB zoom in from border bottom Center + ImageA zoom out to border Top Center
//      8       Rotating from y axis
//      9       Rotating from y axis
//      10      Rotating from x axis
//      11      Rotating from x axis
//      12      1/2 Rotating from y axis (flip)
//      13      1/2 Rotating from y axis (flip)
//      14      1/2 Rotating from x axis (flip)
//      15      1/2 Rotating from x axis (flip)
//      16      1/2 Rotating from y axis (flip) with zoom
//      17      1/2 Rotating from y axis (flip) with zoom
//      18      1/2 Rotating from x axis (flip) with zoom
//      19      1/2 Rotating from x axis (flip) with zoom
//============================================================================================

#define     PCTPART    0.25

void Transition_Push(int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight) {
    QRect       box1,box2;
    QRect       box3,box4;
    QPoint      box;
    int         wt,ht;
    int         PCTW=int(PCT*double(DestImageWith));
    int         PCTH=int(PCT*double(DestImageHeight));
    int         PCTWB=int((1-PCT)*double(DestImageWith));
    int         PCTHB=int((1-PCT)*double(DestImageHeight));
    double      Rotate,dw,dh,ddw,ddh;
    QImage      Img,ImgZoomed;
    QPainter    P;
    double      ZOOMFACTOR=0.4;     // à calculer selon la transition width/hyp pour horiz et height/hyp pour vert !

    // for transition 16 to 19
    if ((TransitionSubType>=16)&&(TransitionSubType<=19)) {
        double Hyp=sqrt( double(DestImageWith*DestImageWith+DestImageHeight*DestImageHeight))*0.707106781; // COS/SIN 45°
        if (TransitionSubType<=17) ZOOMFACTOR=(DestImageWith/Hyp)/2; else ZOOMFACTOR=(DestImageHeight/Hyp)/2;
        if (PCT<PCTPART) {
            ddh=DestImageHeight*(ZOOMFACTOR*PCT*2);
            ddw=DestImageWith*(ZOOMFACTOR*PCT*2);
        } else if (PCT<1-PCTPART) {
            ddh=DestImageHeight*(ZOOMFACTOR*PCTPART*2);
            ddw=DestImageWith*(ZOOMFACTOR*PCTPART*2);
        } else {
            ddh=DestImageHeight*(ZOOMFACTOR*(1-PCT)*2);
            ddw=DestImageWith*(ZOOMFACTOR*(1-PCT)*2);
        }
    }

    switch (TransitionSubType) {
    case 0 :    // Since left to right
        box1=QRect(DestImageWith-PCTW,0,PCTW,DestImageHeight);
        box2=QRect(0,0,PCTW,DestImageHeight);
        box3=QRect(0,0,PCTWB,DestImageHeight);
        box4=QRect(DestImageWith-PCTWB,0,PCTWB,DestImageHeight);
        WorkingPainter->drawImage(box4,*ImageA,box3);
        WorkingPainter->drawImage(box2,*ImageB,box1);
        break;
    case 1 :    // Since right to left
        box1=QRect(0,0,PCTW,DestImageHeight);
        box2=QRect(DestImageWith-PCTW,0,PCTW,DestImageHeight);
        box3=QRect(DestImageWith-PCTWB,0,PCTWB,DestImageHeight);
        box4=QRect(0,0,PCTWB,DestImageHeight);
        WorkingPainter->drawImage(box4,*ImageA,box3);
        WorkingPainter->drawImage(box2,*ImageB,box1);
        break;
    case 2 :    // Since up to down
        box1=QRect(0,DestImageHeight-PCTH,DestImageWith,PCTH);
        box2=QRect(0,0,DestImageWith,PCTH);
        box3=QRect(0,0,DestImageWith,PCTHB);
        box4=QRect(0,DestImageHeight-PCTHB,DestImageWith,PCTHB);
        WorkingPainter->drawImage(box4,*ImageA,box3);
        WorkingPainter->drawImage(box2,*ImageB,box1);
        break;
    case 3 :    // Since down to up
        box1=QRect(0,0,DestImageWith,PCTH);
        box2=QRect(0,DestImageHeight-PCTH,DestImageWith,PCTH);
        box3=QRect(0,DestImageHeight-PCTHB,DestImageWith,PCTHB);
        box4=QRect(0,0,DestImageWith,PCTHB);
        WorkingPainter->drawImage(box4,*ImageA,box3);
        WorkingPainter->drawImage(box2,*ImageB,box1);
        break;
    case 4 :    // Enterring : zoom in from border Left Center - Previous image : zoom out to border Right Center
        wt=int(double(DestImageWith)*(1-PCT));
        ht=int(double(DestImageHeight)*(1-PCT));
        box=QPoint(DestImageWith-wt,(DestImageHeight-ht)/2);
        WorkingPainter->drawImage(box,ImageA->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
        wt=int(double(DestImageWith)*PCT);
        ht=int(double(DestImageHeight)*PCT);
        box=QPoint(0,(DestImageHeight-ht)/2);
        WorkingPainter->drawImage(box,ImageB->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
        break;
    case 5 :    // Enterring : zoom in from border Right Center - Previous image : zoom out to border Left Center
        wt=int(double(DestImageWith)*(1-PCT));
        ht=int(double(DestImageHeight)*(1-PCT));
        box=QPoint(0,(DestImageHeight-ht)/2);
        WorkingPainter->drawImage(box,ImageA->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
        wt=int(double(DestImageWith)*PCT);
        ht=int(double(DestImageHeight)*PCT);
        box=QPoint(DestImageWith-wt,(DestImageHeight-ht)/2);
        WorkingPainter->drawImage(box,ImageB->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
        break;
    case 6 :    // Enterring : zoom in from border Top Center - Previous image : zoom out to border bottom Center
        wt=int(double(DestImageWith)*(1-PCT));
        ht=int(double(DestImageHeight)*(1-PCT));
        box=QPoint((DestImageWith-wt)/2,DestImageHeight-ht);
        WorkingPainter->drawImage(box,ImageA->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
        wt=int(double(DestImageWith)*PCT);
        ht=int(double(DestImageHeight)*PCT);
        box=QPoint((DestImageWith-wt)/2,0);
        WorkingPainter->drawImage(box,ImageB->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
        break;
    case 7 :    // Enterring : zoom in from border bottom Center - Previous image : zoom out to border Top Center
        wt=int(double(DestImageWith)*(1-PCT));
        ht=int(double(DestImageHeight)*(1-PCT));
        box=QPoint((DestImageWith-wt)/2,0);
        WorkingPainter->drawImage(box,ImageA->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
        wt=int(double(DestImageWith)*PCT);
        ht=int(double(DestImageHeight)*PCT);
        box=QPoint((DestImageWith-wt)/2,DestImageHeight-ht);
        WorkingPainter->drawImage(box,ImageB->scaled(QSize(wt,ht),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
        break;
    case 8 :    // Rotating from y axis
        if (PCT<0.5) {
            Rotate=double(90)*(PCT*2);
            Img=RotateImage(0,Rotate,0,ImageA);
        } else {
            Rotate=double(-90)*((1-PCT)*2);
            Img=RotateImage(0,Rotate,0,ImageB);
        }
        dw=(double(DestImageWith)-double(Img.width()))/2;
        dh=(double(DestImageHeight)-double(Img.height()))/2;
        WorkingPainter->drawImage(QPointF(dw,dh),Img);
        break;
    case 9 :    // Rotating from y axis
        if (PCT<0.5) {
            Rotate=double(-90)*(PCT*2);
            Img=RotateImage(0,Rotate,0,ImageA);
        } else {
            Rotate=double(90)*((1-PCT)*2);
            Img=RotateImage(0,Rotate,0,ImageB);
        }
        dw=(double(DestImageWith)-double(Img.width()))/2;
        dh=(double(DestImageHeight)-double(Img.height()))/2;
        WorkingPainter->drawImage(QPointF(dw,dh),Img);
        break;
    case 10 :    // Rotating from x axis
        if (PCT<0.5) {
            Rotate=double(90)*(PCT*2);
            Img=RotateImage(Rotate,0,0,ImageA);
        } else {
            Rotate=double(-90)*((1-PCT)*2);
            Img=RotateImage(Rotate,0,0,ImageB);
        }
        dw=(double(DestImageWith)-double(Img.width()))/2;
        dh=(double(DestImageHeight)-double(Img.height()))/2;
        WorkingPainter->drawImage(QPointF(dw,dh),Img);
        break;
    case 11 :    // Rotating from x axis
        if (PCT<0.5) {
            Rotate=double(-90)*(PCT*2);
            Img=RotateImage(Rotate,0,0,ImageA);
        } else {
            Rotate=double(90)*((1-PCT)*2);
            Img=RotateImage(Rotate,0,0,ImageB);
        }
        dw=(double(DestImageWith)-double(Img.width()))/2;
        dh=(double(DestImageHeight)-double(Img.height()))/2;
        WorkingPainter->drawImage(QPointF(dw,dh),Img);
        break;
    case 12 :    // 1/2 Rotating from y axis (flip)
        dw=DestImageWith/2;
        WorkingPainter->drawImage(QRectF(0,0,dw,DestImageHeight),*ImageA,QRectF(0,0,dw,DestImageHeight));
        WorkingPainter->drawImage(QRectF(dw,0,dw,DestImageHeight),*ImageB,QRectF(dw,0,dw,DestImageHeight));
        if (PCT<0.5) {
            Rotate=double(90)*(PCT*2);
            Img=RotateImage(0,Rotate,0,ImageA);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(DestImageWith/2,dh,Img.width()/2,Img.height()),Img,QRectF(Img.width()/2,0,Img.width()/2,Img.height()));
        } else {
            Rotate=double(-90)*((1-PCT)*2);
            Img=RotateImage(0,Rotate,0,ImageB);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,dh,Img.width()/2,Img.height()),Img,QRectF(0,0,Img.width()/2,Img.height()));
        }
        break;
    case 13 :    // 1/2 Rotating from y axis (flip)
        dw=DestImageWith/2;
        WorkingPainter->drawImage(QRectF(0,0,dw,DestImageHeight),*ImageB,QRectF(0,0,dw,DestImageHeight));
        WorkingPainter->drawImage(QRectF(dw,0,dw,DestImageHeight),*ImageA,QRectF(dw,0,dw,DestImageHeight));
        if (PCT<0.5) {
            Rotate=double(-90)*(PCT*2);
            Img=RotateImage(0,Rotate,0,ImageA);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,dh,Img.width()/2,Img.height()),Img,QRectF(0,0,Img.width()/2,Img.height()));
        } else {
            Rotate=double(90)*((1-PCT)*2);
            Img=RotateImage(0,Rotate,0,ImageB);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(DestImageWith/2,dh,Img.width()/2,Img.height()),Img,QRectF(Img.width()/2,0,Img.width()/2,Img.height()));
        }
        break;
    case 14 :    // 1/2 Rotating from x axis (flip)
        dh=DestImageHeight/2;
        WorkingPainter->drawImage(QRectF(0,0,DestImageWith,dh),*ImageA,QRectF(0,0,DestImageWith,dh));
        WorkingPainter->drawImage(QRectF(0,dh,DestImageWith,dh),*ImageB,QRectF(0,dh,DestImageWith,dh));
        if (PCT<0.5) {
            Rotate=double(90)*(PCT*2);
            Img=RotateImage(Rotate,0,0,ImageA);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,DestImageHeight/2,Img.width(),Img.height()/2),Img,QRectF(0,Img.height()/2,Img.width(),Img.height()/2));
        } else {
            Rotate=double(-90)*((1-PCT)*2);
            Img=RotateImage(Rotate,0,0,ImageB);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,dh,Img.width(),Img.height()/2),Img,QRectF(0,0,Img.width(),Img.height()/2));
        }
        break;
    case 15 :    // 1/2 Rotating from x axis (flip)
        dh=DestImageHeight/2;
        WorkingPainter->drawImage(QRectF(0,0,DestImageWith,dh),*ImageB,QRectF(0,0,DestImageWith,dh));
        WorkingPainter->drawImage(QRectF(0,dh,DestImageWith,dh),*ImageA,QRectF(0,dh,DestImageWith,dh));
        if (PCT<0.5) {
            Rotate=double(-90)*(PCT*2);
            Img=RotateImage(Rotate,0,0,ImageA);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,dh,Img.width(),Img.height()/2),Img,QRectF(0,0,Img.width(),Img.height()/2));
        } else {
            Rotate=double(90)*((1-PCT)*2);
            Img=RotateImage(Rotate,0,0,ImageB);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,DestImageHeight/2,Img.width(),Img.height()/2),Img,QRectF(0,Img.height()/2,Img.width(),Img.height()/2));
        }
        break;
    case 16 :    // 1/2 Rotating from y axis (flip) with zoom
        dw=DestImageWith/2;
        WorkingPainter->drawImage(QRectF(ddw/2,ddh/2,dw-ddw/2,DestImageHeight-ddh),*ImageA,QRectF(0,0,dw,DestImageHeight));
        WorkingPainter->drawImage(QRectF(dw,ddh/2,dw-ddw/2,DestImageHeight-ddh),*ImageB,QRectF(dw,0,dw,DestImageHeight));

        ImgZoomed=QImage(DestImageWith,DestImageHeight,QImage::Format_ARGB32_Premultiplied);
        P.begin(&ImgZoomed);
        P.setCompositionMode(QPainter::CompositionMode_Source);
        P.fillRect(QRect(0,0,DestImageWith,DestImageHeight),Qt::transparent);
        P.setCompositionMode(QPainter::CompositionMode_SourceOver);

        if (PCT<0.5) {
            P.drawImage(QRectF(ddw/2,ddh/2,DestImageWith-ddw,DestImageHeight-ddh),*ImageA,QRectF(0,0,ImageA->width(),ImageA->height()));
            P.end();
            Rotate=double(90)*(PCT*2);
            Img=RotateImage(0,Rotate,0,&ImgZoomed);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(DestImageWith/2,dh,Img.width()/2,Img.height()),Img,QRectF(Img.width()/2,0,Img.width()/2,Img.height()));
        } else {
            P.drawImage(QRectF(ddw/2,ddh/2,DestImageWith-ddw,DestImageHeight-ddh),*ImageB,QRectF(0,0,ImageB->width(),ImageB->height()));
            P.end();
            Rotate=double(-90)*((1-PCT)*2);
            Img=RotateImage(0,Rotate,0,&ImgZoomed);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,dh,Img.width()/2,Img.height()),Img,QRectF(0,0,Img.width()/2,Img.height()));
        }
        break;
    case 17 :    // 1/2 Rotating from y axis (flip) with zoom
        dw=DestImageWith/2;
        WorkingPainter->drawImage(QRectF(ddw/2,ddh/2,dw-ddw/2,DestImageHeight-ddh),*ImageB,QRectF(0,0,dw,DestImageHeight));
        WorkingPainter->drawImage(QRectF(dw,ddh/2,dw-ddw/2,DestImageHeight-ddh),*ImageA,QRectF(dw,0,dw,DestImageHeight));

        ImgZoomed=QImage(DestImageWith,DestImageHeight,QImage::Format_ARGB32_Premultiplied);
        P.begin(&ImgZoomed);
        P.setCompositionMode(QPainter::CompositionMode_Source);
        P.fillRect(QRect(0,0,DestImageWith,DestImageHeight),Qt::transparent);
        P.setCompositionMode(QPainter::CompositionMode_SourceOver);

        if (PCT<0.5) {
            P.drawImage(QRectF(ddw/2,ddh/2,DestImageWith-ddw,DestImageHeight-ddh),*ImageA,QRectF(0,0,ImageA->width(),ImageA->height()));
            P.end();
            Rotate=double(-90)*(PCT*2);
            Img=RotateImage(0,Rotate,0,&ImgZoomed);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,dh,Img.width()/2,Img.height()),Img,QRectF(0,0,Img.width()/2,Img.height()));
        } else {
            P.drawImage(QRectF(ddw/2,ddh/2,DestImageWith-ddw,DestImageHeight-ddh),*ImageB,QRectF(0,0,ImageB->width(),ImageB->height()));
            P.end();
            Rotate=double(90)*((1-PCT)*2);
            Img=RotateImage(0,Rotate,0,&ImgZoomed);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(DestImageWith/2,dh,Img.width()/2,Img.height()),Img,QRectF(Img.width()/2,0,Img.width()/2,Img.height()));
        }
        break;
    case 18 :    // 1/2 Rotating from x axis (flip) with zoom
        dh=DestImageHeight/2;
        WorkingPainter->drawImage(QRectF(ddw/2,ddh/2,DestImageWith-ddw,dh-ddh/2),*ImageA,QRectF(0,0,DestImageWith,dh));
        WorkingPainter->drawImage(QRectF(ddw/2,dh,DestImageWith-ddw,dh-ddh/2),*ImageB,QRectF(0,dh,DestImageWith,dh));

        ImgZoomed=QImage(DestImageWith,DestImageHeight,QImage::Format_ARGB32_Premultiplied);
        P.begin(&ImgZoomed);
        P.setCompositionMode(QPainter::CompositionMode_Source);
        P.fillRect(QRect(0,0,DestImageWith,DestImageHeight),Qt::transparent);
        P.setCompositionMode(QPainter::CompositionMode_SourceOver);

        if (PCT<0.5) {
            P.drawImage(QRectF(ddw/2,ddh/2,DestImageWith-ddw,DestImageHeight-ddh),*ImageA,QRectF(0,0,ImageA->width(),ImageA->height()));
            P.end();
            Rotate=double(90)*(PCT*2);
            Img=RotateImage(Rotate,0,0,&ImgZoomed);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,DestImageHeight/2,Img.width(),Img.height()/2),Img,QRectF(0,Img.height()/2,Img.width(),Img.height()/2));
        } else {
            P.drawImage(QRectF(ddw/2,ddh/2,DestImageWith-ddw,DestImageHeight-ddh),*ImageB,QRectF(0,0,ImageB->width(),ImageB->height()));
            P.end();
            Rotate=double(-90)*((1-PCT)*2);
            Img=RotateImage(Rotate,0,0,&ImgZoomed);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,dh,Img.width(),Img.height()/2),Img,QRectF(0,0,Img.width(),Img.height()/2));
        }
        break;
    case 19 :    // 1/2 Rotating from x axis (flip) with zoom
        dh=DestImageHeight/2;
        WorkingPainter->drawImage(QRectF(ddw/2,ddh/2,DestImageWith-ddw,dh-ddh/2),*ImageB,QRectF(0,0,DestImageWith,dh));
        WorkingPainter->drawImage(QRectF(ddw/2,dh,DestImageWith-ddw,dh-ddh/2),*ImageA,QRectF(0,dh,DestImageWith,dh));

        ImgZoomed=QImage(DestImageWith,DestImageHeight,QImage::Format_ARGB32_Premultiplied);
        P.begin(&ImgZoomed);
        P.setCompositionMode(QPainter::CompositionMode_Source);
        P.fillRect(QRect(0,0,DestImageWith,DestImageHeight),Qt::transparent);
        P.setCompositionMode(QPainter::CompositionMode_SourceOver);

        if (PCT<0.5) {
            P.drawImage(QRectF(ddw/2,ddh/2,DestImageWith-ddw,DestImageHeight-ddh),*ImageA,QRectF(0,0,ImageA->width(),ImageA->height()));
            P.end();
            Rotate=double(-90)*(PCT*2);
            Img=RotateImage(Rotate,0,0,&ImgZoomed);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,dh,Img.width(),Img.height()/2),Img,QRectF(0,0,Img.width(),Img.height()/2));
        } else {
            P.drawImage(QRectF(ddw/2,ddh/2,DestImageWith-ddw,DestImageHeight-ddh),*ImageB,QRectF(0,0,ImageB->width(),ImageB->height()));
            P.end();
            Rotate=double(90)*((1-PCT)*2);
            Img=RotateImage(Rotate,0,0,&ImgZoomed);
            dw=(double(DestImageWith)-double(Img.width()))/2;
            dh=(double(DestImageHeight)-double(Img.height()))/2;
            WorkingPainter->drawImage(QRectF(dw,DestImageHeight/2,Img.width(),Img.height()/2),Img,QRectF(0,Img.height()/2,Img.width(),Img.height()/2));
        }
        break;
    }
}

//============================================================================================
//  Deform transition
//      0       Reduces image A by enlarging the image B Since left to right
//      1       Reduces image A by enlarging the image B Since right to left
//      2       Reduces image A by enlarging the image B Since up to down
//      3       Reduces image A by enlarging the image B Since down to up
//============================================================================================

void Transition_Deform(int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight) 
{
   int PCTW = int(PCT*double(DestImageWith));
   int PCTH = int(PCT*double(DestImageHeight));
   int PCTWB = int((1-PCT)*double(DestImageWith));
   int PCTHB = int((1-PCT)*double(DestImageHeight));

   #define NUMROWS 2
   #define NUMCOLS 2
   switch (TransitionSubType) 
   {
      case 0 :    // Since left to right
         WorkingPainter->drawImage(QRect(PCTW,0,DestImageWith-PCTW,DestImageHeight),*ImageA,QRect(0,0,DestImageWith,DestImageHeight));
         WorkingPainter->drawImage(QRect(0,0,PCTW,DestImageHeight),*ImageB,QRect(0,0,DestImageWith,DestImageHeight));
         break;
      case 1 :    // Since right to left
         WorkingPainter->drawImage(QRect(0,0,PCTWB,DestImageHeight),*ImageA,QRect(0,0,DestImageWith,DestImageHeight));
         WorkingPainter->drawImage(QRect(PCTWB,0,DestImageWith-PCTWB,DestImageHeight),*ImageB,QRect(0,0,DestImageWith,DestImageHeight));
         break;
      case 2 :    // Since up to down
         WorkingPainter->drawImage(QRect(0,PCTH,DestImageWith,DestImageHeight-PCTH),*ImageA,QRect(0,0,DestImageWith,DestImageHeight));
         WorkingPainter->drawImage(QRect(0,0,DestImageWith,PCTH),*ImageB,QRect(0,0,DestImageWith,DestImageHeight));
         break;
      case 3 :    // Since down to up
         WorkingPainter->drawImage(QRect(0,0,DestImageWith,PCTHB),*ImageA,QRect(0,0,DestImageWith,DestImageHeight));
         WorkingPainter->drawImage(QRect(0,PCTHB,DestImageWith,DestImageHeight-PCTHB),*ImageB,QRect(0,0,DestImageWith,DestImageHeight));
         break;
      case 4:
         {
            // create rectangles
            int rWidth = DestImageWith/NUMCOLS * (1-PCT);
            int wDelta = DestImageWith/NUMCOLS - rWidth;
            int rheight = DestImageHeight/NUMROWS * (1-PCT);
            int hDelta = DestImageHeight/NUMROWS - rheight;
            QList<QRect> rects;
            for(int i = 0; i < NUMCOLS; i++ )
               for(int j = 0; j < NUMROWS; j++)
                  //rects.append(QRect(i*(rWidth+wDelta),j*(rheight+hDelta),rWidth,rheight));
                  rects.append(QRect(i*(rWidth+wDelta)+wDelta/2,j*(rheight+hDelta)+hDelta/2,rWidth,rheight));
            QPainterPath pPath;
            foreach(QRect r, rects)
               pPath.addRoundedRect(r,5,5);
               //pPath.addRect(r);
            WorkingPainter->drawImage(0,0,*ImageB);
            WorkingPainter->save();
            WorkingPainter->setClipPath(pPath);
            WorkingPainter->setOpacity(1-PCT);
            WorkingPainter->drawImage(0,0,*ImageA);
            WorkingPainter->restore();
         }
         break;
      case 5:
         {
            // create rectangles
            int rWidth = DestImageWith/NUMCOLS * PCT;
            int wDelta = DestImageWith/NUMCOLS - rWidth;
            int rheight = DestImageHeight/NUMROWS * PCT;
            int hDelta = DestImageHeight/NUMROWS - rheight;
            QList<QRect> rects;
            for(int i = 0; i < NUMCOLS; i++ )
               for(int j = 0; j < NUMROWS; j++)
                  //rects.append(QRect(i*(rWidth+wDelta),j*(rheight+hDelta),rWidth,rheight));
                  rects.append(QRect(i*(rWidth+wDelta)+wDelta/2,j*(rheight+hDelta)+hDelta/2,rWidth,rheight));
            QPainterPath pPath;
            foreach(QRect r, rects)
               pPath.addRoundedRect(r,5,5);
               //pPath.addRect(r);
            WorkingPainter->drawImage(0,0,*ImageA);
            WorkingPainter->save();
            WorkingPainter->setClipPath(pPath);
            WorkingPainter->setOpacity(PCT);
            WorkingPainter->drawImage(0,0,*ImageB);
            WorkingPainter->restore();
         }
      break;
      case 6:
         {
            // create rectangles
            int sWidth = DestImageWith/NUMCOLS;
            int sHeight = DestImageHeight/NUMROWS;

            QList<QRect> srcRects;
            QList<QRectF> dstRects;
            for(int i = 0; i < NUMCOLS; i++ )
            {
               for(int j = 0; j < NUMROWS; j++)
               {
                  //rects.append(QRect(i*(rWidth+wDelta),j*(rheight+hDelta),rWidth,rheight));
                  srcRects.append(QRect(i*sWidth,j*sHeight,sWidth,sHeight));
                  qreal nX = i*sWidth + (DestImageWith - i*sWidth)*PCT;
                  qreal nY = j*sHeight + (DestImageHeight - j*sHeight)*PCT;
                  qreal nW = sWidth*(1-PCT);
                  qreal nH = sHeight*(1-PCT);
                  dstRects.append(QRectF(nX,nY,nW,nH));
                  qDebug() << "destRect " << QRectF(nX,nY,nW,nH);
               }
            }
            WorkingPainter->drawImage(0,0,*ImageB);
            for(int i = 0; i < NUMCOLS; i++ )
               for(int j = 0; j < NUMROWS; j++)
                  WorkingPainter->drawImage(dstRects.at(i+j*NUMCOLS),*ImageA,srcRects.at(i+j*NUMCOLS));
         }
      break;
   }
}

//============================================================================================
//  Luma transition
//============================================================================================

void Transition_LumaSSE(QImage Luma,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight) 
{
   //QImage    Img = ImageB->copy();
   if (LumaTmpImage == NULL || LumaTmpImage->size() != ImageB->size() || LumaTmpImage->format() != ImageB->format())
   {
      delete LumaTmpImage;
      LumaTmpImage = new QImage(ImageB->size(), ImageB->format());
   }
   //memcpy(LumaTmpImage->bits(), ImageB->constBits(), ImageB->byteCount());
   // Apply PCTDone to luma mask
   quint32  limit    = quint8(PCT*double(0xff))+1;
   __m128i *LumaData = (__m128i *)Luma.constBits();
   __m128i *ImgData  = (__m128i *)LumaTmpImage->bits();
   __m128i *ImgDataA = (__m128i *)ImageA->constBits();
   __m128i *ImgDataB = (__m128i *)ImageB->constBits();
   __m128i mmLimit;
   mmLimit = _mm_set1_epi32(limit);
   const __m128i mmMask = _mm_set_epi32(255, 255, 255, 255);
   int size = (DestImageWith * DestImageHeight) / 4;
   for (int i = 0; i < size; i++)
   {
      // take luma-Bits
      __m128i mmLuma = _mm_load_si128(LumaData);
      // mask out rgb, leaving alpha
      mmLuma = _mm_and_si128(mmMask, mmLuma);
      // compare with limits
      mmLuma = _mm_cmpgt_epi32(mmLuma, mmLimit);
      // load image-data (4 pixels in one)
      __m128i mm0 = _mm_load_si128(ImgDataB);
      __m128i mm1 = _mm_load_si128(ImgDataA);
      __m128i result = _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(mm0), _mm_castsi128_ps(mm1), _mm_castsi128_ps(mmLuma)));
      _mm_store_si128(ImgData, result);
      //if (((*LumaData++)& 0xff)>limit) *ImgData=*ImgData2;
      ImgData++;
      ImgDataA++;
      ImgDataB++;
      LumaData++;
   }

   // Draw transformed image
   WorkingPainter->drawImage(0,0, *LumaTmpImage);
}

void Transition_Luma_Soft(QImage Luma, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight,double softness)
{
   //Transition_LumaSSE(Luma,PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight); is not faster!!
   //return;
   quint32 *ImgData;
   //if (LumaTmpImage == NULL || LumaTmpImage->size() != ImageB->size() || LumaTmpImage->format() != ImageB->format())
   //{
   //   delete LumaTmpImage;
   //   LumaTmpImage = new QImage(ImageB->size(), ImageB->format());
   //}

   // Apply PCTDone to luma mask
   quint32  limit = quint8(PCT*double(0xff)) + 1;
   quint32  limit2 = quint8((qBound(0.0,PCT - softness,1.0))*double(0xff)) + 1;
   int lumaAdd = 255 - limit;

   quint32 *LumaData = (quint32 *)Luma.constBits();
   //ImgData = (quint32 *)LumaTmpImage->bits();

   WorkingPainter->drawImage(0, 0, *ImageB);
   QImage imgB = ImageA->convertToFormat(QImage::Format_ARGB32);
   quint32 *ImgDataB = (quint32 *)imgB.bits();
   int size = DestImageWith*DestImageHeight;
   for (int i = 0; i < size; i++)
   {
      quint8 alpha = (LumaData[i] & 0xff);
      //double nAlpha = alpha *lumaMul;
      //alpha = qBound(0.0, nAlpha,255.0);
      if ((alpha) > limit)
         ImgDataB[i] = ImgDataB[i] | 0xff000000;// | (0xff << 24);
      else if ((alpha) > limit2)
      {
         alpha = qBound(0, alpha+lumaAdd, 255);
         //quint8 newLuma = (alpha - limit2) * lumastep;
         ImgDataB[i] = ImgDataB[i] & 0x00ffffff | (alpha << 24);
      }
      else
         ImgDataB[i] = ImgDataB[i] & 0x00ffffff | 0x0;
   }

   WorkingPainter->drawImage(0, 0, imgB);
   // Draw transformed image
   //WorkingPainter->drawImage(0, 0, *LumaTmpImage);
}

void Transition_Luma_Softold(QImage Luma, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   //WorkingPainter->drawImage(0, 0, *ImageA);

   quint32 *LumaData = (quint32 *)Luma.constBits();
   QImage imgA = ImageA->convertToFormat(QImage::Format_ARGB32);
   QImage imgB = ImageB->convertToFormat(QImage::Format_ARGB32);
   quint32 *ImgDataA = (quint32 *)imgA.bits();
   quint32 *ImgDataB = (quint32 *)imgB.bits();
   int PCTAdd;
   PCTAdd = 510 * PCT - 255;
   //quint32  limit = quint8(PCT*double(0xff)) + 1;
   int size = DestImageWith*DestImageHeight;
   for (int i = 0; i < size; i++)
   {
      quint8 alpha = LumaData[i] & 0xff;
      quint8 alphaB, alphaA;
      //if (alpha < limit)
      {
         //alphaA = qBound(int(0), 255 - (int)(255 - alpha) + PCTAdd, int((ImgDataA[i] & 0xff000000) >> 24));
         alphaB = qBound(int(0), (int)(255 - alpha) + PCTAdd, int((ImgDataB[i] & 0xff000000) >> 24));
         alphaA = qBound(int(0), (int)(255 - alpha), int((ImgDataA[i] & 0xff000000) >> 24));
         //alpha = qBound(int(0), (int)(255 - alpha) + PCTAdd, 255);
         //int iAlpha = 255 - alpha;
         ImgDataB[i] = ImgDataB[i] & 0x00ffffff | (alphaB << 24);
         ImgDataA[i] = ImgDataA[i] & 0x00ffffff | (alphaA << 24);
      }
   }
   WorkingPainter->drawImage(0, 0, imgA);
   WorkingPainter->drawImage(0, 0, imgB);
}

void Transition_Luma_Soft(QImage Luma, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   //WorkingPainter->drawImage(0, 0, *ImageA);
   QPaintDevice *pDev = WorkingPainter->device();
   QImage *destImage = dynamic_cast< QImage* >( pDev );
   QImage imgB;
   if (destImage)
   { 
      imgB = *destImage;
      QPainter p(&imgB);
      p.drawImage(0, 0, *ImageB);
      p.end();
      imgB = imgB.convertToFormat(QImage::Format_ARGB32);
   }
   else
      imgB = ImageB->convertToFormat(QImage::Format_ARGB32);
   //imgB.save("f:\\imgb.png");
   WorkingPainter->drawImage(0, 0, *ImageA);
   quint32 *LumaData = (quint32 *)Luma.constBits();
   //QImage imgB = ImageB->convertToFormat(QImage::Format_ARGB32);
   quint32 *ImgDataB = (quint32 *)imgB.bits();
   int PCTAdd;
   PCTAdd = 510 * PCT - 255;
   quint32  limit = quint8(PCT*double(0xff)) + 1;
   //quint32  limit2 = limit + 25;
   //quint8 lumaAdd = 255 * PCT;
   int size = DestImageWith*DestImageHeight;
   int ilumaAdd = (510 * PCT) - 255;
   if (PCT <= 0)
      ilumaAdd = -255;
   for (int i = 0; i < size; i++)
   {
      int lumaAlpha = 255 - (LumaData[i] & 0xff);
      lumaAlpha += ilumaAdd;
      lumaAlpha = qBound(0, lumaAlpha, int((ImgDataB[i] & 0xff000000) >> 24));
      ImgDataB[i] = ImgDataB[i] & 0x00ffffff | (lumaAlpha << 24);
   }
   WorkingPainter->drawImage(0, 0, imgB);
}


void Transition_Luma(QImage Luma, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   //return Transition_Luma_Soft(Luma, PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight,0.10);
   return Transition_Luma_Soft(Luma, PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
   //Transition_LumaSSE(Luma,PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight); is not faster!!
   //return;
   quint32 *ImgData;
   if (LumaTmpImage == NULL || LumaTmpImage->size() != ImageB->size() || LumaTmpImage->format() != ImageB->format())
   {
      delete LumaTmpImage;
      LumaTmpImage = new QImage(ImageB->size(), ImageB->format());
   }

   // Apply PCTDone to luma mask
   quint32  limit = quint8(PCT*double(0xff))+1;

   quint32 *LumaData = (quint32 *)Luma.constBits();
   ImgData = (quint32 *)LumaTmpImage->bits();
   quint32 *ImgDataB = (quint32 *)ImageB->constBits();
   quint32 *ImgDataA = (quint32 *)ImageA->constBits();
   int size = DestImageWith*DestImageHeight;
   for (int i = 0; i < size; i++) 
   {
      if ((LumaData[i] & 0xff) > limit)
         ImgData[i] = ImgDataA[i];
      else
         ImgData[i] = ImgDataB[i];
   }

   // Draw transformed image
   WorkingPainter->drawImage(0,0, *LumaTmpImage);
}


//============================================================================================
// Generic public function to do a transition
//============================================================================================

void DoTransition(TRFAMILY TransitionFamily,int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight) 
{
   //static int nTransitions = 0;
   //static uint64_t transitiontotal = 0;
   //static uint64_t transitionfamilytotal[11] = {0,0,0,0,0,0,0,0,0,0,0};
   //static uint64_t transitionfamilycount[11] = {0,0,0,0,0,0,0,0,0,0,0};
   //QTime t;
   //t.start();
#ifdef MEASURE_TRANSITION
   LONGLONG cp = curPCounter();
#endif
   foreach(TransitionInterface *ti, exTransitions)
   {
      if (ti->familyID() == TransitionFamily)
      {
         ti->doTransition(TransitionSubType, PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
         return;
      }
   }
   switch (TransitionFamily) {
         //case TRANSITIONFAMILY_BASE        : Transition_Basic( TransitionSubType,PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
         //case TRANSITIONFAMILY_ZOOMINOUT   : Transition_Zoom(  TransitionSubType,PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
         //case TRANSITIONFAMILY_PUSH        : Transition_Push(  TransitionSubType,PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
         //case TRANSITIONFAMILY_SLIDE       : Transition_Slide( TransitionSubType,PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
         //case TRANSITIONFAMILY_DEFORM      : Transition_Deform(TransitionSubType,PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
         //case TRANSITIONFAMILY_LUMA_BAR    : Transition_Luma(LumaList_Bar.List[TransitionSubType].GetLuma(ImageB->width(),ImageB->height()),    PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
         //case TRANSITIONFAMILY_LUMA_BOX    : Transition_Luma(LumaList_Box.List[TransitionSubType].GetLuma(ImageB->width(),ImageB->height()),    PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
         //case TRANSITIONFAMILY_LUMA_CENTER : Transition_Luma(LumaList_Center.List[TransitionSubType].GetLuma(ImageB->width(),ImageB->height()), PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
         //case TRANSITIONFAMILY_LUMA_CHECKER: Transition_Luma(LumaList_Checker.List[TransitionSubType].GetLuma(ImageB->width(),ImageB->height()),PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
         //case TRANSITIONFAMILY_LUMA_CLOCK  : Transition_Luma(LumaList_Clock.List[TransitionSubType].GetLuma(ImageB->width(),ImageB->height()),  PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
         //case TRANSITIONFAMILY_LUMA_SNAKE  : Transition_Luma(LumaList_Snake.List[TransitionSubType].GetLuma(ImageB->width(),ImageB->height()),  PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
#ifdef LUMA_TEST
         case TRANSITIONFAMILY_LUMA_TEST  : Transition_Luma(LumaList_Test.List[TransitionSubType].GetLuma(ImageB->width(),ImageB->height()),  PCT,ImageA,ImageB,WorkingPainter,DestImageWith,DestImageHeight);    break;
#endif
   }
#ifdef MEASURE_TRANSITION
   cp = curPCounter() - cp;
   qDebug() << "transition takes " << PC2time(cp,true) << " for type " <<ImageA->format() << " and type " <<ImageB->format();
#endif
   //int i = t.elapsed();
   //transitiontotal += i;
   //transitionfamilytotal[TransitionFamilly] += i;
   //transitionfamilycount[TransitionFamilly]++;
   //nTransitions++;
   //for(int x = 0; x <11; x++ )
   //  if(transitionfamilytotal[x] > 0 )
   //     qDebug() << "fam " << x << " count = " << transitionfamilycount[x] << " total = " << transitionfamilytotal[x] << " (" << (transitionfamilytotal[x]/(transitionfamilycount[x]>0?transitionfamilycount[x]:1))<< "/Transition)";

   //qDebug() << "Transition family = " << TransitionFamilly << " subType = " << TransitionSubType << " takes " 
   //  << t.elapsed() << " mSec (total: " << nTransitions << " in " << transitiontotal << " mSecs --> " << (transitiontotal/nTransitions) << " mSec/Transition";
}

int numTransitions(int transitionFamily)
{
   foreach(TransitionInterface *ti, exTransitions)
   {
      if (ti->familyID() == transitionFamily)
      {
         return ti->numTransitions();
      }
   }
   switch(transitionFamily)
   {
      case TRANSITIONFAMILY_BASE:        return TRANSITIONMAXSUBTYPE_BASE;
      case TRANSITIONFAMILY_ZOOMINOUT:   return TRANSITIONMAXSUBTYPE_ZOOMINOUT;
      case TRANSITIONFAMILY_SLIDE:       return TRANSITIONMAXSUBTYPE_SLIDE;
      case TRANSITIONFAMILY_PUSH:        return TRANSITIONMAXSUBTYPE_PUSH;
      case TRANSITIONFAMILY_LUMA_BAR:    return LumaList_Bar.List.count();
      case TRANSITIONFAMILY_LUMA_BOX:    return LumaList_Box.List.count();
      case TRANSITIONFAMILY_LUMA_CENTER: return LumaList_Center.List.count();
      case TRANSITIONFAMILY_LUMA_CHECKER:return LumaList_Checker.List.count();
      case TRANSITIONFAMILY_LUMA_CLOCK:  return LumaList_Clock.List.count();
      case TRANSITIONFAMILY_LUMA_SNAKE:  return LumaList_Snake.List.count();
#ifdef LUMA_TEST
      case TRANSITIONFAMILY_LUMA_TEST:  return LumaList_Test.List.count();
#endif
      case TRANSITIONFAMILY_DEFORM:      return TRANSITIONMAXSUBTYPE_DEFORM;// 7;
   }
   return 0;
}


QImage BasicTransitionLoader::getIcon(int TransitionFamily, int TransitionSubType) const
{
   QString FileName = QString(":/img/Transitions/tr-%1-%2.png")
      .arg(TransitionFamily, 2, 10, QChar('0'))
      .arg(TransitionSubType, 2, 10, QChar('0'));
   QImage Icon = QImage(FileName);
   if (Icon.isNull())
   {
      Icon = QImage(QString(":/img/Transitions") + QDir::separator() + QString("tr-icon-error.png"));
      ToLog(LOGMSG_WARNING, "Icon not found:" + QDir(FileName).absolutePath());
   }
   return Icon;
}

QStringList TransitionsBasic::transitions() const
{
   QStringList l;
   return l;
}

void TransitionsBasic::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   Transition_Basic(TransitionSubType, PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
}

QStringList TransitionsZoom::transitions() const
{
   QStringList l;
   return l;
}

void TransitionsZoom::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   if (TransitionSubType > 17)
   {
      Transition_Zoom_ex(TransitionSubType, PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
      return;
   }
   Transition_Zoom(TransitionSubType, PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
}

QStringList TransitionsSlide::transitions() const
{
   QStringList l;
   return l;
}

void TransitionsSlide::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   Transition_Slide(TransitionSubType, PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
}

QStringList TransitionsPush::transitions() const
{
   QStringList l;
   return l;
}

void TransitionsPush::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   Transition_Push(TransitionSubType, PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
}

QStringList TransitionsDeform::transitions() const
{
   QStringList l;
   return l;
}

void TransitionsDeform::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   Transition_Deform(TransitionSubType, PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
}

TransitionsLumaBar::TransitionsLumaBar()
{
   LumaList_Bar.ScanDisk("luma/Bar", TRANSITIONFAMILY_LUMA_BAR);
}

QStringList TransitionsLumaBar::transitions() const
{
   QStringList l;
   return l;
}

void TransitionsLumaBar::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   Transition_Luma(LumaList_Bar.List[TransitionSubType].GetLuma(ImageB->width(), ImageB->height()), PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
}

QImage TransitionsLumaBar::getIcon(int TransitionSubType) const
{
   return LumaList_Bar.List[TransitionSubType].DlgLumaImage;
}

TransitionsLumaBox::TransitionsLumaBox()
{
   LumaList_Box.ScanDisk("luma/Box", TRANSITIONFAMILY_LUMA_BOX);
}

QStringList TransitionsLumaBox::transitions() const
{
   QStringList l;
   return l;
}

void TransitionsLumaBox::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   Transition_Luma(LumaList_Box.List[TransitionSubType].GetLuma(ImageB->width(), ImageB->height()), PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
}

QImage TransitionsLumaBox::getIcon(int TransitionSubType) const
{
   return LumaList_Box.List[TransitionSubType].DlgLumaImage;
}

TransitionsLumaCenter::TransitionsLumaCenter()
{
   LumaList_Center.ScanDisk("luma/Center", TRANSITIONFAMILY_LUMA_CENTER);
}

QStringList TransitionsLumaCenter::transitions() const
{
   QStringList l;
   return l;
}

void TransitionsLumaCenter::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   Transition_Luma(LumaList_Center.List[TransitionSubType].GetLuma(ImageB->width(), ImageB->height()), PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
}

QImage TransitionsLumaCenter::getIcon(int TransitionSubType) const
{
   return LumaList_Center.List[TransitionSubType].DlgLumaImage;
}

TransitionsLumaChecker::TransitionsLumaChecker()
{
   LumaList_Checker.ScanDisk("luma/Checker", TRANSITIONFAMILY_LUMA_CHECKER);
}

QStringList TransitionsLumaChecker::transitions() const
{
   QStringList l;
   return l;
}

void TransitionsLumaChecker::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   Transition_Luma(LumaList_Checker.List[TransitionSubType].GetLuma(ImageB->width(), ImageB->height()), PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
}

QImage TransitionsLumaChecker::getIcon(int TransitionSubType) const
{
   return LumaList_Checker.List[TransitionSubType].DlgLumaImage;
}

TransitionsLumaClock::TransitionsLumaClock()
{
   LumaList_Clock.ScanDisk("luma/Clock", TRANSITIONFAMILY_LUMA_CLOCK);
}

QStringList TransitionsLumaClock::transitions() const
{
   QStringList l;
   return l;
}

void TransitionsLumaClock::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   Transition_Luma(LumaList_Clock.List[TransitionSubType].GetLuma(ImageB->width(), ImageB->height()), PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
}

QImage TransitionsLumaClock::getIcon(int TransitionSubType) const
{
   return LumaList_Clock.List[TransitionSubType].DlgLumaImage;
}

TransitionsLumaSnake::TransitionsLumaSnake()
{
   LumaList_Snake.ScanDisk("luma/Snake", TRANSITIONFAMILY_LUMA_SNAKE);
}

QStringList TransitionsLumaSnake::transitions() const
{
   QStringList l;
   return l;
}

void TransitionsLumaSnake::doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight)
{
   Transition_Luma(LumaList_Snake.List[TransitionSubType].GetLuma(ImageB->width(), ImageB->height()), PCT, ImageA, ImageB, WorkingPainter, DestImageWith, DestImageHeight);
}

QImage TransitionsLumaSnake::getIcon(int TransitionSubType) const
{
   return LumaList_Snake.List[TransitionSubType].DlgLumaImage;
}

