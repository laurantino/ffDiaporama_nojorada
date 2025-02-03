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

#ifndef CSHOTCOMPOSER_H
#define CSHOTCOMPOSER_H

// Basic inclusions (common to all files)
#include "CustomCtrl/_QCustomDialog.h"
#include "engine/_Diaporama.h"
#include "DlgSlide/cInteractiveZone.h"
#include "DlgSlide/cCustomBlockTable.h"
#include "DlgSlide/cCustomShotTable.h"
#include <QCheckBox>
#include <QToolButton>

class cShotComposer : public QCustomDialog {
Q_OBJECT
   bool bConnectedSource;
   bool hasMaskControls;

public:
    // Undo actions
    enum UNDOACTION_ID {
        UNDOACTION_FULL_SLIDE
    };

    QAction                     *actionAddImageClipboard;
    QAction                     *actionPaste;

    bool                        InRefreshControls;
    bool                        InSelectionChange;
    bool                        NoPrepUndo;

    double                      DisplayW,DisplayH;
    cDiaporamaObject            *CurrentSlide;              // Current slide
    double                      ProjectGeometry;
    cInteractiveZone            *InteractiveZone;
    cCustomBlockTable           *BlockTable;
    cCustomShotTable            *ShotTable;
    cCompositionObject          *CurrentCompoObject;        // Current block object (if selection mode = SELECTMODE_ONE)
    int                         CurrentCompoObjectNbr;      // Number of Current block object (if selection mode = SELECTMODE_ONE)
    int                         CurrentShotNbr;             // Current shot number (if selection mode = SELECTMODE_ONE)
    SELECTMODE                  BlockSelectMode;            // Current block selection mode
    cCompositionList            *CompositionList;           // Link to current block List
    QList<bool>                 IsSelected;                 // Table of selection state in the current block list
    int                         NbrSelected;                // Number of selected blocks

    explicit                    cShotComposer(cDiaporamaObject *DiaporamaObject,cApplicationConfig *ApplicationConfig,QWidget *parent = 0);
    virtual void                DoInitDialog();

    // Utility function used to apply modification from one shot to next shot and/or global composition
    virtual void                ResetThumbs(bool ResetAllThumbs);
    virtual void                ApplyToContexte(bool ResetAllThumbs);
    virtual cCompositionObject  *GetGlobalCompositionObject(int IndexKey);      // Return CompositionObject in the global composition list for specific IndexKey

    // Update embedded controls
    virtual void                RefreshControls(bool UpdateInteractiveZone=true);

protected:
    void s_BlockSettings_BlockInheritances();
    // Block settings : Coordinates
    void s_BlockSettings_PosXValue(double);
    void s_BlockSettings_PosYValue(double);
    void s_BlockSettings_PosWidthValue(double);
    void s_BlockSettings_PosHeightValue(double);

    // Block settings : Rotations
    void s_BlockSettings_RotateZValue(int);
    void s_BlockSettings_RotateXValue(int);
    void s_BlockSettings_RotateYValue(int);
    void s_BlockSettings_ResetRotateXValue()     { s_BlockSettings_RotateXValue(0); }
    void s_BlockSettings_ResetRotateYValue()     { s_BlockSettings_RotateYValue(0); }
    void s_BlockSettings_ResetRotateZValue()     { s_BlockSettings_RotateZValue(0); }

    void setMaskingEnabled(bool bEnabled);
protected slots:
    void s_Event_ClipboardChanged();
    void on_InheritDownCB_clicked() { s_BlockSettings_BlockInheritances(); }

    // Block settings : Coordinates
    void on_PosXEd_valueChanged(double d) { s_BlockSettings_PosXValue(d); }
    void on_PosYEd_valueChanged(double d) { s_BlockSettings_PosYValue(d); }
    void on_WidthEd_valueChanged(double d) { s_BlockSettings_PosWidthValue(d); }
    void on_HeightEd_valueChanged(double d) { s_BlockSettings_PosHeightValue(d); }


    // Block settings : Rotations
    void on_RotateXED_valueChanged(int value) { s_BlockSettings_RotateXValue(value); }
    void on_RotateXSLD_valueChanged(int value) { s_BlockSettings_RotateXValue(value); }
    void on_ResetRotateXBT_clicked() { s_BlockSettings_ResetRotateXValue(); }

    void on_RotateYED_valueChanged(int value) { s_BlockSettings_RotateYValue(value); }
    void on_RotateYSLD_valueChanged(int value) { s_BlockSettings_RotateYValue(value); }
    void on_ResetRotateYBT_clicked() { s_BlockSettings_ResetRotateYValue(); }

    void on_RotateZED_valueChanged(int value) { s_BlockSettings_RotateZValue(value); }
    void on_RotateZSLD_valueChanged(int value) { s_BlockSettings_RotateZValue(value); }
    void on_ResetRotateZBT_clicked() { s_BlockSettings_ResetRotateZValue(); }
    void on_cbRotateZOrigin_currentIndexChanged(int index);

   //connect(GETUI("ShapeSizePosCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapeSizePos(int)));
   //connect(GETUI("BackgroundFormCB"), SIGNAL(itemSelectionHaveChanged()), this, SLOT(s_BlockSettings_ShapeBackgroundForm()));
   //connect(GETUI("TextClipArtCB"), SIGNAL(itemSelectionHaveChanged()), this, SLOT(s_BlockSettings_ShapeTextClipArtChIndex()));
   //connect(GETUI("OpacityCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapeOpacity(int)));
   //connect(GETUI("PenStyleCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapePenStyle(int)));
   //connect(GETUI("ShadowEffectCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapeShadowFormValue(int)));
   //connect(GETUI("ShadowEffectED"), SIGNAL(valueChanged(int)), this, SLOT(s_BlockSettings_ShapeShadowDistanceValue(int)));
   //connect(GETUI("PenColorCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapePenColor(int)));
   //connect(GETUI("PenSizeEd"), SIGNAL(valueChanged(int)), this, SLOT(s_BlockSettings_ShapePenSize(int)));
   //connect(GETUI("ShadowColorCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapeShadowColor(int)));
   //connect(GETUI("BlockShapeStyleBT"), SIGNAL(pressed()), this, SLOT(s_BlockSettings_BlockShapeStyleBT()));

    // Block settings : Shape
    void s_BlockSettings_BlockShapeStyleBT();
    void s_BlockSettings_ShapeTextClipArtChIndex();
    void s_BlockSettings_ShapeBackgroundForm();
    void s_BlockSettings_ShapeOpacity(int);
    void s_BlockSettings_ShapeShadowFormValue(int);
    void s_BlockSettings_ShapeShadowDistanceValue(int);
    void s_BlockSettings_ShapePenSize(int);
    void s_BlockSettings_ShapePenColor(int);
    void s_BlockSettings_ShapePenStyle(int);
    void s_BlockSettings_ShapeShadowColor(int);

    // Block settings : Alignment
    void s_BlockTable_AlignTop();
    void s_BlockTable_AlignMiddle();
    void s_BlockTable_AlignBottom();
    void s_BlockTable_AlignLeft();
    void s_BlockTable_AlignCenter();
    void s_BlockTable_AlignRight();
    void s_BlockTable_DistributeHoriz();
    void s_BlockTable_DistributeVert();

    // Block table
    void s_BlockTable_SelectionChanged();            // User select a block in the BlockTable widget
    void s_BlockTable_MoveBlockUp();
    void s_BlockTable_MoveBlockDown();
    void s_BlockTable_DragMoveBlock(int,int);
    void s_BlockTable_RemoveBlock();

    void s_BlockSettings_Information();
    void s_BlockSettings_TextEditor();

    // Block settings/Interactive zone messages
    void s_BlockSettings_IntZoneTransformBlocks(qreal DeltaX,qreal DeltaY,qreal ScaleX,qreal ScaleY,qreal Sel_X,qreal Sel_Y,qreal Sel_W,qreal Sel_H);
    void s_BlockSettings_IntZoneDisplayTransformBlocks(qreal DeltaX,qreal DeltaY,qreal ScaleX,qreal ScaleY,qreal Sel_X,qreal Sel_Y,qreal Sel_W,qreal Sel_H);
    void on_cbSourceConnect_stateChanged(int state) { bConnectedSource = state == Qt::Checked; }
    void s_BlockSettings_TransformBlocks(qreal DeltaX,qreal DeltaY,qreal DeltaW,qreal DeltaH, QRectF selRect);
    void s_BlockSettings_RotateBlocks(qreal xAngle, qreal yAngle, qreal zAngle);
    void s_ZoomSlide(qreal zoomDiff, qreal slideX, qreal slideY);


    void on_cmUseMask_clicked();
    void on_cmInvertMask_clicked();
    //void on_cbMaskShape_itemSelectionHaveChanged();
    void on_MaskPosX_valueChanged(double d);
    void on_MaskPosY_valueChanged(double d);
    void on_MaskWidth_valueChanged(double d);
    void on_MaskHeight_valueChanged(double d);
    void on_RotateZMaskSLD_valueChanged(int value) { on_RotateMaskZED_valueChanged(value); } 
    void on_RotateMaskZED_valueChanged(int value); 
    void on_ResetRotateMask_clicked() { on_RotateMaskZED_valueChanged(0); }
    void on_cm3DFrame_clicked();
protected:
    void ComputeBlockRatio(cCompositionObject *Block,double &Ratio_X,double &Ratio_Y);
    void RefreshBlockTable(int SetCurrentIndex);
    void MakeFormIcon(QComboBox *UICB);
    void MakeBorderStyleIcon(QComboBox *UICB);

};

//====================================================================================================================
// Define some macros
//====================================================================================================================
#define ICON_RULER_ON                   ":/img/ruler_ok.png"
#define ICON_RULER_OFF                  ":/img/ruler_ko.png"
#define ICON_EDIT_IMAGE                 ":/img/EditImage.png"
#define ICON_EDIT_MOVIE                 ":/img/EditMovie.png"
#define ICON_EDIT_GMAPS                 ":/img/EditGMaps.png"

#define ISVIDEO(OBJECT)                 ((OBJECT->MediaObject)&&(OBJECT->MediaObject->is_Videofile()))
#define ISBLOCKVALIDE()                 ((!InRefreshControls)&&(BlockSelectMode==SELECTMODE_ONE)&&(CurrentCompoObject))
#define ISBLOCKVALIDEVISIBLE()          (ISBLOCKVALIDE()&&(CurrentCompoObject->isVisible()))
#define GETUI(WIDGETNAME)               findChild<QWidget *>(WIDGETNAME)
#define GETDOUBLESPINBOX(WIDGETNAME)    findChild<QDoubleSpinBox *>(WIDGETNAME)
#define GETSPINBOX(WIDGETNAME)          findChild<QSpinBox *>(WIDGETNAME)
#define GETSLIDER(WIDGETNAME)           findChild<QSlider *>(WIDGETNAME)
#define GETCOMBOBOX(WIDGETNAME)         findChild<QComboBox *>(WIDGETNAME)
#define GETBUTTON(WIDGETNAME)           findChild<QToolButton *>(WIDGETNAME)
#define GETCHECKBOX(WIDGETNAME)         findChild<QCheckBox *>(WIDGETNAME)

#define APPLY1TONEXT(FIELD) {                                                                                                           \
   bool ContAPPLY = true;                                                                                                                \
   int  ShotNum = CurrentShotNbr+1;                                                                                                      \
   while (ContAPPLY && ShotNum < CurrentSlide->shotList.count()) {                                                                         \
      for (int Block = 0; ContAPPLY && Block < CurrentSlide->shotList[CurrentShotNbr]->ShotComposition.compositionObjects.count(); Block++)                   \
         for (int ToSearch = 0; ContAPPLY && ToSearch < CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects.count(); ToSearch++)                \
            if (CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch]->index() == CurrentCompoObject->index()) {                    \
               cCompositionObject *ShotObject = CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch];                                 \
               if (ShotObject->hasInheritance()) ShotObject->FIELD = CurrentCompoObject->FIELD;                                             \
               else ContAPPLY = false;                                                                                                   \
            }                                                                                                                               \
            ShotNum++;                                                                                                                      \
   }                                                                                                                                   \
}
/*
//#define APPLY2TONEXT(FIELD1,FIELD2) {                                                                                                   \
//    bool ContAPPLY=true;                                                                                                                \
//    int  ShotNum=CurrentShotNbr+1;                                                                                                      \
//    while ((ContAPPLY)&&(ShotNum<CurrentSlide->List.count())) {                                                                         \
//        for (int Block=0;ContAPPLY && Block<CurrentSlide->List[CurrentShotNbr]->ShotComposition.List.count();Block++)                   \
//         for (int ToSearch=0;ContAPPLY && ToSearch<CurrentSlide->List[ShotNum]->ShotComposition.List.count();ToSearch++)                \
//          if (CurrentSlide->List[ShotNum]->ShotComposition.List[ToSearch]->index()==CurrentCompoObject->index()) {                    \
//            cCompositionObject *ShotObject=CurrentSlide->List[ShotNum]->ShotComposition.List[ToSearch];                                 \
//            if (ShotObject->hasInheritance()) {                                                                                        \
//                ShotObject->FIELD1=CurrentCompoObject->FIELD1;                                                                          \
//                ShotObject->FIELD2=CurrentCompoObject->FIELD2;                                                                          \
//            } else ContAPPLY=false;                                                                                                     \
//        }                                                                                                                               \
//        ShotNum++;                                                                                                                      \
//    }                                                                                                                                   \
//}

//#define APPLY3TONEXT(FIELD1,FIELD2,FIELD3) {                                                                                            \
//    bool ContAPPLY=true;                                                                                                                \
//    int  ShotNum=CurrentShotNbr+1;                                                                                                      \
//    while ((ContAPPLY)&&(ShotNum<CurrentSlide->List.count())) {                                                                         \
//        for (int Block=0;ContAPPLY && Block<CurrentSlide->List[CurrentShotNbr]->ShotComposition.List.count();Block++)                   \
//         for (int ToSearch=0;ContAPPLY && ToSearch<CurrentSlide->List[ShotNum]->ShotComposition.List.count();ToSearch++)                \
//          if (CurrentSlide->List[ShotNum]->ShotComposition.List[ToSearch]->index()==CurrentCompoObject->index()) {                    \
//            cCompositionObject *ShotObject=CurrentSlide->List[ShotNum]->ShotComposition.List[ToSearch];                                 \
//            if (ShotObject->hasInheritance()) {                                                                                        \
//                ShotObject->FIELD1=CurrentCompoObject->FIELD1;                                                                          \
//                ShotObject->FIELD2=CurrentCompoObject->FIELD2;                                                                          \
//                ShotObject->FIELD3=CurrentCompoObject->FIELD3;                                                                          \
//            } else ContAPPLY=false;                                                                                                     \
//        }                                                                                                                               \
//        ShotNum++;                                                                                                                      \
//    }                                                                                                                                   \
//}

//#define APPLY4TONEXT(FIELD1,FIELD2,FIELD3,FIELD4) {                                                                                     \
//    bool ContAPPLY=true;                                                                                                                \
//    int  ShotNum=CurrentShotNbr+1;                                                                                                      \
//    while ((ContAPPLY)&&(ShotNum<CurrentSlide->List.count())) {                                                                         \
//        for (int Block=0;ContAPPLY && Block<CurrentSlide->List[CurrentShotNbr]->ShotComposition.List.count();Block++)                   \
//         for (int ToSearch=0;ContAPPLY && ToSearch<CurrentSlide->List[ShotNum]->ShotComposition.List.count();ToSearch++)                \
//          if (CurrentSlide->List[ShotNum]->ShotComposition.List[ToSearch]->index()==CurrentCompoObject->index()) {                    \
//            cCompositionObject *ShotObject=CurrentSlide->List[ShotNum]->ShotComposition.List[ToSearch];                                 \
//            if (ShotObject->hasInheritance()) {                                                                                        \
//                ShotObject->FIELD1=CurrentCompoObject->FIELD1;                                                                          \
//                ShotObject->FIELD2=CurrentCompoObject->FIELD2;                                                                          \
//                ShotObject->FIELD3=CurrentCompoObject->FIELD3;                                                                          \
//                ShotObject->FIELD4=CurrentCompoObject->FIELD4;                                                                          \
//            } else ContAPPLY=false;                                                                                                     \
//        }                                                                                                                               \
//        ShotNum++;                                                                                                                      \
//    }                                                                                                                                   \
//}
*/
#define APPLY_GETSET_TONEXTold(func) { \
   bool ContAPPLY = true; \
   int  ShotNum = CurrentShotNbr+1; \
   while (ContAPPLY && (ShotNum < CurrentSlide->shotList.count()) ) \
   { \
      for (int Block = 0; ContAPPLY && Block < CurrentSlide->shotList[CurrentShotNbr]->ShotComposition.compositionObjects.count(); Block++) \
         for (int ToSearch = 0; ContAPPLY && ToSearch < CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects.count(); ToSearch++) \
            if (CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch]->index() == CurrentCompoObject->index()) \
            { \
               cCompositionObject *ShotObject = CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch]; \
               if (ShotObject->hasInheritance()) \
               { \
                  ShotObject->set##func(CurrentCompoObject->get##func()); \
               } \
               else \
                  ContAPPLY = false; \
       } \
       ShotNum++; \
    } \
 }

#define APPLY_GETSET_TONEXT(func) { \
   bool ContAPPLY = true; \
   int  ShotNum = CurrentShotNbr + 1; \
   while (ContAPPLY && (ShotNum < CurrentSlide->shotList.count())) \
   { \
      cCompositionObject* ShotObject = CurrentSlide->shotList[ShotNum]->ShotComposition.getCompositionObject(CurrentCompoObject->index()); \
      if (ShotObject->hasInheritance()) \
      { \
         ShotObject->setRect(CurrentCompoObject->getRect()); \
      } \
      else \
         ContAPPLY = false; \
      ShotNum++; \
   }\
}

#define APPLY_GETSETA_TONEXTold(getter,setter) { \
    bool ContAPPLY=true;                                                                                                                \
    int  ShotNum=CurrentShotNbr+1;                                                                                                      \
        while ((ContAPPLY)&&(ShotNum<CurrentSlide->shotList.count())) {                                                                         \
        for (int Block=0;ContAPPLY && Block<CurrentSlide->shotList[CurrentShotNbr]->ShotComposition.compositionObjects.count();Block++)                   \
         for (int ToSearch=0;ContAPPLY && ToSearch<CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects.count();ToSearch++)                \
          if (CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch]->index()==CurrentCompoObject->index()) {                    \
            cCompositionObject *ShotObject=CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch];                                 \
            if (ShotObject->hasInheritance()) {                                                                                        \
                ShotObject->setter(CurrentCompoObject->getter());                                                                          \
            } else ContAPPLY=false;                                                                                                     \
        }                                                                                                                               \
        ShotNum++;                                                                                                                      \
    }                                                                                                                                   \
}

#define APPLY_GETSETA_TONEXT(getter,setter) { \
   bool ContAPPLY = true; \
   int  ShotNum = CurrentShotNbr + 1; \
   while (ContAPPLY && (ShotNum < CurrentSlide->shotList.count())) \
   { \
      cCompositionObject* ShotObject = CurrentSlide->shotList[ShotNum]->ShotComposition.getCompositionObject(CurrentCompoObject->index()); \
      if (ShotObject->hasInheritance()) \
      { \
         ShotObject->setter(CurrentCompoObject->getter()); \
      } \
      else \
         ContAPPLY = false; \
      ShotNum++; \
   }\
}

#define APPLY_GETSETA_TONEXT_SUBOBJold(subobjgetter,getter,setter) { \
    bool ContAPPLY = true;                                                                                                                \
    int  ShotNum = CurrentShotNbr+1;                                                                                                      \
        while (ContAPPLY && ShotNum < CurrentSlide->shotList.count()) {                                                                         \
        for (int Block = 0; ContAPPLY && Block < CurrentSlide->shotList[CurrentShotNbr]->ShotComposition.compositionObjects.count(); Block++)                   \
         for (int ToSearch = 0; ContAPPLY && ToSearch<CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects.count(); ToSearch++)                \
          if (CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch]->index() == CurrentCompoObject->index()) {                    \
            cCompositionObject *ShotObject = CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch];                                 \
            if (ShotObject->hasInheritance()) {                                                                                        \
                ShotObject->subobjgetter()->setter(CurrentCompoObject->subobjgetter()->getter());                                                                          \
            } else ContAPPLY = false;                                                                                                     \
        }                                                                                                                               \
        ShotNum++;                                                                                                                      \
    }                                                                                                                                   \
}

#define APPLY_GETSETA_TONEXT_SUBOBJ(subobjgetter,getter,setter) { \
   bool ContAPPLY = true; \
   int  ShotNum = CurrentShotNbr + 1; \
   while (ContAPPLY && (ShotNum < CurrentSlide->shotList.count())) \
   { \
      cCompositionObject* ShotObject = CurrentSlide->shotList[ShotNum]->ShotComposition.getCompositionObject(CurrentCompoObject->index()); \
      if (ShotObject->hasInheritance()) \
      { \
         ShotObject->subobjgetter()->setter(CurrentCompoObject->subobjgetter()->getter()); \
      } \
      else \
         ContAPPLY = false; \
      ShotNum++; \
   }\
}

#define SUBAPPLY(FIELD)\
{  \
   if (CurrentCompoObject->BackgroundBrush->FIELD != SavedBrush.FIELD ) \
      ShotObject->BackgroundBrush->FIELD = CurrentCompoObject->BackgroundBrush->FIELD;\
}

#define SUBAPPLYX(FIELD)\
{  \
   if (CurrentCompoObject->BackgroundBrush->FIELD() != SavedBrush.FIELD() ) \
      ShotObject->BackgroundBrush->set##FIELD(CurrentCompoObject->BackgroundBrush->FIELD());\
}

#define APPLYBACKGROUNDBRUSH() {                                                                                                       \
   bool ContAPPLY = true;                                                                                                                \
   int  ShotNum = CurrentShotNbr + 1;                                                                                                      \
   while (ContAPPLY && (ShotNum < CurrentSlide->shotList.count())) \
   { \
      cCompositionObject* ShotObject = CurrentSlide->shotList[ShotNum]->ShotComposition.getCompositionObject(CurrentCompoObject->index()); \
      if (ShotObject->hasInheritance()) {                                                                                        \
         ShotObject->BackgroundBrush->assignChanges(*CurrentCompoObject->BackgroundBrush,SavedBrush); \
      } \
      else \
         ContAPPLY = false; \
      ShotNum++;                                                                                                                      \
   }                                                                                                                                   \
}
//
//
//#define APPLYBACKGROUNDBRUSH() {                                                                                                       \
//    bool ContAPPLY = true;                                                                                                                \
//    int  ShotNum = CurrentShotNbr + 1;                                                                                                      \
//        while ((ContAPPLY) && (ShotNum < CurrentSlide->shotList.count())) {                                                                         \
//        for (int Block = 0; ContAPPLY && Block<CurrentSlide->shotList[CurrentShotNbr]->ShotComposition.compositionObjects.count(); Block++)                   \
//         for (int ToSearch = 0; ContAPPLY && ToSearch<CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects.count(); ToSearch++)                \
//          if (CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch]->index() == CurrentCompoObject->index()) {                    \
//            cCompositionObject *ShotObject = CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch];                                 \
//            if (ShotObject->hasInheritance()) {                                                                                        \
//                SUBAPPLY(PatternType)                                                                                                   \
//                SUBAPPLY(GradientOrientation)                                                                                           \
//                SUBAPPLY(ColorD)                                                                                                        \
//                SUBAPPLY(ColorF)                                                                                                        \
//                SUBAPPLY(ColorIntermed)                                                                                                 \
//                SUBAPPLY(Intermediate)                                                                                                  \
//                SUBAPPLY(BrushImage)                                                                                                    \
//                SUBAPPLY(MediaObject)                                                                                                   \
//                SUBAPPLY(SoundVolume)                                                                                                   \
//                SUBAPPLY(Deinterlace)                                                                                                   \
//                SUBAPPLYX(ImageRotation)                                                                                                 \
//                SUBAPPLYX(X)                                                                                                             \
//                SUBAPPLYX(Y)                                                                                                             \
//                SUBAPPLYX(ZoomFactor)                                                                                                    \
//                SUBAPPLYX(Brightness)                                                                                                    \
//                SUBAPPLYX(Contrast)                                                                                                      \
//                SUBAPPLYX(Gamma)                                                                                                         \
//                SUBAPPLYX(Red)                                                                                                           \
//                SUBAPPLYX(Green)                                                                                                         \
//                SUBAPPLYX(Blue)                                                                                                          \
//                SUBAPPLYX(LockGeometry)                                                                                                  \
//                SUBAPPLYX(FullFilling)                                                                                                   \
//                SUBAPPLYX(AspectRatio)                                                                                                   \
//                SUBAPPLYX(GaussBlurSharpenSigma)                                                                                         \
//                SUBAPPLYX(BlurSharpenRadius)                                                                                             \
//                SUBAPPLYX(QuickBlurSharpenSigma)                                                                                         \
//                SUBAPPLYX(TypeBlurSharpen)                                                                                               \
//                SUBAPPLYX(Desat)                                                                                                         \
//                SUBAPPLYX(Swirl)                                                                                                         \
//                SUBAPPLYX(Implode)                                                                                                       \
//                SUBAPPLYX(OnOffFilters)                                                                                                   \
//                SUBAPPLY(ImageSpeedWave)                                                                                                \
//                for (int MarkNum=0;MarkNum<ShotObject->BackgroundBrush->Markers.count();MarkNum++) {                                    \
//                    if (MarkNum<SavedBrush.Markers.count()) {                                                                           \
//                        SUBAPPLY(Markers[MarkNum].MarkerColor)                                                                          \
//                        SUBAPPLY(Markers[MarkNum].TextColor)                                                                            \
//                        SUBAPPLY(Markers[MarkNum].LineColor)                                                                            \
//                        SUBAPPLY(Markers[MarkNum].Visibility)                                                                           \
//                    } else {                                                                                                            \
//                        ShotObject->BackgroundBrush->Markers[MarkNum].MarkerColor = CurrentCompoObject->BackgroundBrush->Markers[MarkNum].MarkerColor;    \
//                        ShotObject->BackgroundBrush->Markers[MarkNum].TextColor   = CurrentCompoObject->BackgroundBrush->Markers[MarkNum].TextColor;      \
//                        ShotObject->BackgroundBrush->Markers[MarkNum].LineColor   = CurrentCompoObject->BackgroundBrush->Markers[MarkNum].LineColor;      \
//                        ShotObject->BackgroundBrush->Markers[MarkNum].Visibility  = CurrentCompoObject->BackgroundBrush->Markers[MarkNum].Visibility;     \
//                    }                                                                                                                   \
//                }                                                                                                                       \
//            } else ContAPPLY = false;                                                                                                     \
//        }                                                                                                                               \
//        ShotNum++;                                                                                                                      \
//    }                                                                                                                                   \
//}

#define APPLYBACKGROUNDBRUSH_POS() { \
   bool ContAPPLY = true; \
   int  ShotNum = CurrentShotNbr + 1; \
   while (ContAPPLY && (ShotNum < CurrentSlide->shotList.count())) \
   { \
      cCompositionObject* ShotObject = CurrentSlide->shotList[ShotNum]->ShotComposition.getCompositionObject(CurrentCompoObject->index()); \
      if (ShotObject->hasInheritance()) \
      { \
                SUBAPPLYX(X) \
                SUBAPPLYX(Y) \
                SUBAPPLYX(ZoomFactor) \
      } \
      else \
         ContAPPLY = false; \
      ShotNum++; \
   } \
}

#define APPLYBACKGROUNDBRUSH_POSold() {                                                                                                       \
    bool ContAPPLY = true;                                                                                                                \
    int  ShotNum = CurrentShotNbr + 1;                                                                                                      \
        while ((ContAPPLY) && (ShotNum < CurrentSlide->shotList.count())) {                                                                         \
        for (int Block = 0; ContAPPLY && Block<CurrentSlide->shotList[CurrentShotNbr]->ShotComposition.compositionObjects.count(); Block++)                   \
         for (int ToSearch = 0; ContAPPLY && ToSearch<CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects.count(); ToSearch++)                \
          if (CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch]->index() == CurrentCompoObject->index()) {                    \
            cCompositionObject *ShotObject = CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch];                                 \
            if (ShotObject->hasInheritance()) {                                                                                        \
                SUBAPPLYX(X)                                                                                                             \
                SUBAPPLYX(Y)                                                                                                             \
                SUBAPPLYX(ZoomFactor)                                                                                                    \
            } else ContAPPLY = false;                                                                                                     \
        }                                                                                                                               \
        ShotNum++;                                                                                                                      \
    }                                                                                                                                   \
}
#endif // CSHOTCOMPOSER_H
