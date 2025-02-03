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


#ifndef _TRANSITION_H
#define _TRANSITION_H

#define noLUMA_TEST
// Transition family definition
enum TRFAMILY {
   TRANSITIONFAMILY_BASE,
   TRANSITIONFAMILY_ZOOMINOUT,
   TRANSITIONFAMILY_SLIDE,
   TRANSITIONFAMILY_PUSH,
   TRANSITIONFAMILY_LUMA_BAR,
   TRANSITIONFAMILY_LUMA_BOX,
   TRANSITIONFAMILY_LUMA_CENTER,
   TRANSITIONFAMILY_LUMA_CHECKER,
   TRANSITIONFAMILY_LUMA_CLOCK,
   TRANSITIONFAMILY_LUMA_SNAKE,
   TRANSITIONFAMILY_DEFORM
#ifdef LUMA_TEST
   , TRANSITIONFAMILY_LUMA_TEST
#endif
};

//============================================

// Transition subtype for LUMA_BAR transitions
enum TRLUMABAR {
   Bar128,    Bar146,    Bar164,    Bar182,
   Bar219,    Bar237,    Bar273,    Bar291,
   BilinearA1,BilinearA4,BilinearA8,BilinearA9,
   BilinearB1,BilinearB4,BilinearB8,BilinearB9,
   StoreA446, StoreA846, StoreB482, StoreB882,
   ZBar01,    ZBar02,    ZBar03,    ZBar04,
   TRANSITIONMAXSUBTYPE_LUMABAR
};

// Transition subtype for TRANSITIONFAMILY_LUMA_CLOCK transitions
enum TRLUMACLOCK {
   ClockA1,ClockA2,ClockA3,ClockA4,
   ClockA6,ClockA7,ClockA8,ClockA9,
   ClockB1,ClockB2,ClockB3,ClockB4,
   ClockB6,ClockB7,ClockB8,ClockB9,
   ClockC1,ClockC2,ClockC3,ClockC4,
   ClockC6,ClockC7,ClockC8,ClockC9,
   //ClockD1,ClockD2,
   TRANSITIONMAXSUBTYPE_LUMACLOCK
};

//============================================

// No luma transition : number of sub type
#define TRANSITIONMAXSUBTYPE_BASE           5
#define TRANSITIONMAXSUBTYPE_ZOOMINOUT      36
#define TRANSITIONMAXSUBTYPE_SLIDE          22
#define TRANSITIONMAXSUBTYPE_PUSH           20
#define TRANSITIONMAXSUBTYPE_DEFORM         7

// static local values use to work with luma images
#define     LUMADLG_HEIGHT  80
extern int  LUMADLG_WIDTH;

//============================================

#include "_GlobalDefines.h"
#include "cApplicationConfig.h"
#include "ffd_interfaces.h"

#include <QImage>
#include <QPainter>

//*********************************************************************************************************************************************
// Global class containing icons of transitions
//*********************************************************************************************************************************************
class cLumaObject;
class cIconObject 
{
   public:
      QImage      Icon;                       // The icon
      TRFAMILY    TransitionFamily;           // Transition family
      int         TransitionSubType;          // Transition type in the family

      cIconObject(TRFAMILY TransitionFamily,int TransitionSubType);
      cIconObject(TRFAMILY TransitionFamily,int TransitionSubType,cLumaObject *Luma);
};

//*********************************************************************************************************************************************
// Global class containing icons library
//*********************************************************************************************************************************************

class cIconList 
{
   public:
      QList<cIconObject>  List;                       // list of icons

      cIconList();
      ~cIconList();

      QImage *GetIcon(TRFAMILY TransitionFamily,int TransitionSubType);
};

//*********************************************************************************************************************************************
// Global class containing luma library
//*********************************************************************************************************************************************
class cLumaObject 
{
   public:
      QImage      OriginalLuma;
      QImage      DlgLumaImage;
      QImage      lastLumaImage;
      TRFAMILY   lastTransitionFamily;
      int lWidth, lHeight;
      QString     Name;
      int         TransitionSubType;
      TRFAMILY   TransitionFamily;

      cLumaObject(TRFAMILY TrFamily,int TrSubType,QString FileName);
      QImage GetLuma(int DestImageWith,int DestImageHeight);
      void    clearCache();

   private:
      QImage GetLumaBar(int DestImageWith,int DestImageHeight);
      QImage GetLumaClock(int DestImageWith,int DestImageHeight);
};

//*****************************************************************

class   cLumaList 
{
   public:
      int                 Geometry;
      QList<cLumaObject>  List;                       // list of Luma

      cLumaList();
      ~cLumaList();

      void ScanDisk(QString Path,TRFAMILY TransitionFamily);
      void SetGeometry(ffd_GEOMETRY Geometry);
      void clearCache();
      void releaseAll();
};

// static global values
extern cLumaList LumaList_Bar;
extern cLumaList LumaList_Box;
extern cLumaList LumaList_Center;
extern cLumaList LumaList_Checker;
extern cLumaList LumaList_Clock;
extern cLumaList LumaList_Snake;
#ifdef LUMA_TEST
extern cLumaList LumaList_Test;
#endif
extern cIconList IconList;

int RegisterNoLumaTransitions();
void ReleaseNoLumaTransitions();
int RegisterLumaTransitions();
void ReleaseLumaTransitions();
void clearLumaCache();
int numTransitions(int transitionFamily);

//*********************************************************************************************************************

void DoTransition(TRFAMILY TransitionFamily,int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight);

void Transition_Basic( int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight);
void Transition_Zoom(  int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight);
void Transition_Slide( int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight);
void Transition_Push(  int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight);
void Transition_Deform(int TransitionSubType,double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight);
void Transition_Luma(  QImage Luma,          double PCT,QImage *ImageA,QImage *ImageB,QPainter *WorkingPainter,int DestImageWith,int DestImageHeight);

extern QList<TransitionInterface *> exTransitions;
TransitionInterface *getTransitionInterface(int familyID);
class BasicTransitionLoader
{
   public:
   QImage getIcon(int TransitionFamily, int TransitionSubType) const;
};

class TransitionsBasic : public QObject, public TransitionInterface, public BasicTransitionLoader
{
   Q_OBJECT
   Q_INTERFACES(TransitionInterface)

   public:
      QString pluginName() const { return "Internal: None and basic"; };
      QString familyName() const { return QApplication::translate("DlgTransitionProperties", "None and basic"); };
      int familyID() const { return TRANSITIONFAMILY_BASE; }
      int numTransitions() const { return TRANSITIONMAXSUBTYPE_BASE; }
      QStringList transitions() const override;
      void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
      QImage getIcon(int TransitionSubType) const override { return BasicTransitionLoader::getIcon(familyID(), TransitionSubType); }
};

class TransitionsZoom : public QObject, public TransitionInterface, public BasicTransitionLoader
{
   Q_OBJECT
   Q_INTERFACES(TransitionInterface)

   public:
      QString pluginName() const { return "Internal: Zoom"; };
      QString familyName() const { return QApplication::translate("DlgTransitionProperties", "Zoom"); };
      int familyID() const { return TRANSITIONFAMILY_ZOOMINOUT; }
   int numTransitions() const { return TRANSITIONMAXSUBTYPE_ZOOMINOUT; }
   QStringList transitions() const override;
   void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
   QImage getIcon(int TransitionSubType) const override { return BasicTransitionLoader::getIcon(familyID(), TransitionSubType); }
};

class TransitionsSlide : public QObject, public TransitionInterface, public BasicTransitionLoader
{
   Q_OBJECT
   Q_INTERFACES(TransitionInterface)

   public:
      QString pluginName() const { return "Internal: Slide"; };
      QString familyName() const { return QApplication::translate("DlgTransitionProperties", "Slide"); };
      int familyID() const { return TRANSITIONFAMILY_SLIDE; }
   int numTransitions() const { return TRANSITIONMAXSUBTYPE_SLIDE; }
   QStringList transitions() const override;
   void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
   QImage getIcon(int TransitionSubType) const override { return BasicTransitionLoader::getIcon(familyID(), TransitionSubType); }
};

class TransitionsPush : public QObject, public TransitionInterface, public BasicTransitionLoader
{
   Q_OBJECT;
   Q_INTERFACES(TransitionInterface)

   public:
   QString pluginName() const { return "Internal: Push"; };
   QString familyName() const { return QApplication::translate("DlgTransitionProperties", "Push"); };
   int familyID() const { return TRANSITIONFAMILY_PUSH; }
   int numTransitions() const { return TRANSITIONMAXSUBTYPE_PUSH; }
   QStringList transitions() const override;
   void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
   QImage getIcon(int TransitionSubType) const override { return BasicTransitionLoader::getIcon(familyID(), TransitionSubType); }
};

class TransitionsDeform : public QObject, public TransitionInterface, public BasicTransitionLoader
{
   Q_OBJECT;
   Q_INTERFACES(TransitionInterface)

   public:
      QString pluginName() const { return "Internal: Deform"; };
      QString familyName() const { return QApplication::translate("DlgTransitionProperties", "Deform"); };
      int familyID() const { return TRANSITIONFAMILY_DEFORM; }
   int numTransitions() const { return TRANSITIONMAXSUBTYPE_DEFORM; }
   QStringList transitions() const override;
   void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
   QImage getIcon(int TransitionSubType) const override { return BasicTransitionLoader::getIcon(familyID(), TransitionSubType); }
};

class TransitionsLumaBar : public QObject, public TransitionInterface
{
   Q_OBJECT;
   Q_INTERFACES(TransitionInterface)

   public:
   TransitionsLumaBar();
   QString pluginName() const { return "Internal: Luma-Bar"; };
   QString familyName() const { return QApplication::translate("DlgTransitionProperties", "Luma-Bar"); };
   int familyID() const { return TRANSITIONFAMILY_LUMA_BAR; }
   int numTransitions() const { return LumaList_Bar.List.count(); }
   QStringList transitions() const override;
   void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
   QImage getIcon(int TransitionSubType) const;
};

class TransitionsLumaBox : public QObject, public TransitionInterface
{
   Q_OBJECT;
   Q_INTERFACES(TransitionInterface)

   public:
   TransitionsLumaBox();
   QString pluginName() const { return "Internal: Luma-Box"; };
   QString familyName() const { return QApplication::translate("DlgTransitionProperties", "Luma-Box"); };
   int familyID() const { return TRANSITIONFAMILY_LUMA_BOX; }
   int numTransitions() const { return LumaList_Box.List.count(); }
   QStringList transitions() const override;
   void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
   QImage getIcon(int TransitionSubType) const;
};

class TransitionsLumaCenter : public QObject, public TransitionInterface
{
   Q_OBJECT;
   Q_INTERFACES(TransitionInterface)

   public:
   TransitionsLumaCenter();
   QString pluginName() const { return "Internal: Luma-Center"; };
   QString familyName() const { return QApplication::translate("DlgTransitionProperties", "Luma-Center"); };
   int familyID() const { return TRANSITIONFAMILY_LUMA_CENTER; }
   int numTransitions() const { return LumaList_Center.List.count(); }
   QStringList transitions() const override;
   void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
   QImage getIcon(int TransitionSubType) const;
};

class TransitionsLumaChecker : public QObject, public TransitionInterface
{
   Q_OBJECT;
   Q_INTERFACES(TransitionInterface)

   public:
   TransitionsLumaChecker();
   QString pluginName() const { return "Internal: Luma-Checker"; };
   QString familyName() const { return QApplication::translate("DlgTransitionProperties", "Luma-Checker"); };
   int familyID() const { return TRANSITIONFAMILY_LUMA_CHECKER; }
   int numTransitions() const { return LumaList_Checker.List.count(); }
   QStringList transitions() const override;
   void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
   QImage getIcon(int TransitionSubType) const;
};

class TransitionsLumaClock : public QObject, public TransitionInterface
{
   Q_OBJECT;
   Q_INTERFACES(TransitionInterface)

   public:
   TransitionsLumaClock();
   QString pluginName() const { return "Internal: Luma-Clock"; };
   QString familyName() const { return QApplication::translate("DlgTransitionProperties", "Luma-Clock"); };
   int familyID() const { return TRANSITIONFAMILY_LUMA_CLOCK; }
   int numTransitions() const { return LumaList_Clock.List.count(); }
   QStringList transitions() const override;
   void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
   QImage getIcon(int TransitionSubType) const;
};

class TransitionsLumaSnake : public QObject, public TransitionInterface
{
   Q_OBJECT;
   Q_INTERFACES(TransitionInterface)

   public:
   TransitionsLumaSnake();
   QString pluginName() const { return "Internal: Luma-Snake"; };
   QString familyName() const { return QApplication::translate("DlgTransitionProperties", "Luma-Snake"); };
   int familyID() const { return TRANSITIONFAMILY_LUMA_SNAKE; }
   int numTransitions() const { return LumaList_Snake.List.count(); }
   QStringList transitions() const override;
   void doTransition(int TransitionSubType, double PCT, QImage *ImageA, QImage *ImageB, QPainter *WorkingPainter, int DestImageWith, int DestImageHeight) override;
   QImage getIcon(int TransitionSubType) const;
};
#endif // _TRANSITION_H

