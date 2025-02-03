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

#include "cShotComposer.h"
#include "cTexteFrameComboBox.h"
#include "CustomCtrl/cCShapeComboBox.h"
#include "cColorComboBox.h"

#include "DlgInfoFile/DlgInfoFile.h"
#include "DlgText/DlgTextEdit.h"

//====================================================================================================================

cShotComposer::cShotComposer(cDiaporamaObject *DiaporamaObject, cApplicationConfig *ApplicationConfig, QWidget *parent) : QCustomDialog(ApplicationConfig, parent)
{
   switch (DiaporamaObject->pDiaporama->ImageGeometry)
   {
      case GEOMETRY_4_3:      DisplayW = 1440;    DisplayH = 1080;     break;
      case GEOMETRY_40_17:    DisplayW = 1920;    DisplayH = 816;      break;
      case GEOMETRY_16_9:
      default:                DisplayW = 1920;    DisplayH = 1080;     break;
   }
   CurrentSlide = DiaporamaObject;
   ProjectGeometry = DisplayH / DisplayW;
   ProjectGeometry = GetDoubleValue(QString("%1").arg(ProjectGeometry, 0, 'e'));  // Rounded to same number as style managment
   TypeWindowState = TypeWindowState_withsplitterpos;
   CurrentShotNbr = 0;
   CurrentCompoObject = NULL;
   CurrentCompoObjectNbr = -1;
   InRefreshControls = false;
   InSelectionChange = false;
   BlockSelectMode = SELECTMODE_NONE;
   NoPrepUndo = false;
   actionAddImageClipboard = NULL;
   actionPaste = NULL;
   hasMaskControls = false;
}

//====================================================================================================================
// Initialise dialog
void cShotComposer::DoInitDialog()
{
   GETUI("cbSourceConnect")->setVisible(false);
#ifndef ENABLE_EXGUI
   GETUI("pbMovingsOnTop")->setVisible(false);
   GETUI("cbRotateZOrigin")->setVisible(false);
   GETUI("cm3DFrame")->setVisible(false);
   GETUI("lblMaskCoordinates")->setVisible(false);
   GETUI("cmUseMask")->setVisible(false);
   GETUI("cmInvertMask")->setVisible(false);
   GETUI("lbMaskPosX")->setVisible(false);
   GETUI("MaskPosX")->setVisible(false);
   GETUI("lbMaskPosY")->setVisible(false);
   GETUI("MaskPosY")->setVisible(false);
   GETUI("lbMaskWidth")->setVisible(false);
   GETUI("MaskWidth")->setVisible(false);
   GETUI("lbMaskHeight")->setVisible(false);
   GETUI("MaskHeight")->setVisible(false);
   GETUI("lbMaskRotate")->setVisible(false);
   GETUI("RotateZMaskSLD")->setVisible(false);
   GETUI("RotateMaskZED")->setVisible(false);
   GETUI("ResetRotateMask")->setVisible(false);
#endif

   hasMaskControls = GETDOUBLESPINBOX("MaskPosX") != 0;
   Splitter->setCollapsible(0, false);
   Splitter->setCollapsible(1, false);
   InteractiveZone->DiaporamaObject = CurrentSlide;
   InteractiveZone->BlockTable = BlockTable;
   InteractiveZone->setObjectPointers(&CurrentCompoObject,&CurrentCompoObjectNbr);
   BlockTable->ApplicationConfig = ApplicationConfig;
   BlockTable->CurrentSlide = CurrentSlide;
   if (ShotTable) 
      ShotTable->DiaporamaObject = CurrentSlide;

   //if (GETUI("InheritDownCB")) connect(GETCHECKBOX("InheritDownCB"), SIGNAL(clicked()), this, SLOT(s_BlockSettings_BlockInheritances()));

   // Block settings : Coordinates
   if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
   {
      GETDOUBLESPINBOX("PosXEd")->setDecimals(2);     GETDOUBLESPINBOX("PosXEd")->setSingleStep(1);       GETDOUBLESPINBOX("PosXEd")->setSuffix("%");
      GETDOUBLESPINBOX("PosYEd")->setDecimals(2);     GETDOUBLESPINBOX("PosYEd")->setSingleStep(1);       GETDOUBLESPINBOX("PosYEd")->setSuffix("%");
      GETDOUBLESPINBOX("WidthEd")->setDecimals(2);    GETDOUBLESPINBOX("WidthEd")->setSingleStep(1);      GETDOUBLESPINBOX("WidthEd")->setSuffix("%");
      GETDOUBLESPINBOX("HeightEd")->setDecimals(2);   GETDOUBLESPINBOX("HeightEd")->setSingleStep(1);     GETDOUBLESPINBOX("HeightEd")->setSuffix("%");
   }
   else
   { // DisplayUnit==DISPLAYUNIT_PIXELS 
      GETDOUBLESPINBOX("PosXEd")->setDecimals(0);     GETDOUBLESPINBOX("PosXEd")->setSingleStep(1);       GETDOUBLESPINBOX("PosXEd")->setSuffix("");
      GETDOUBLESPINBOX("PosYEd")->setDecimals(0);     GETDOUBLESPINBOX("PosYEd")->setSingleStep(1);       GETDOUBLESPINBOX("PosYEd")->setSuffix("");
      GETDOUBLESPINBOX("WidthEd")->setDecimals(0);    GETDOUBLESPINBOX("WidthEd")->setSingleStep(1);      GETDOUBLESPINBOX("WidthEd")->setSuffix("");
      GETDOUBLESPINBOX("HeightEd")->setDecimals(0);   GETDOUBLESPINBOX("HeightEd")->setSingleStep(1);     GETDOUBLESPINBOX("HeightEd")->setSuffix("");
   }
   //connect(GETUI("PosXEd"), SIGNAL(valueChanged(double)), this, SLOT(s_BlockSettings_PosXValue(double)));
   //connect(GETUI("PosYEd"), SIGNAL(valueChanged(double)), this, SLOT(s_BlockSettings_PosYValue(double)));
   //connect(GETUI("WidthEd"), SIGNAL(valueChanged(double)), this, SLOT(s_BlockSettings_PosWidthValue(double)));
   //connect(GETUI("HeightEd"), SIGNAL(valueChanged(double)), this, SLOT(s_BlockSettings_PosHeightValue(double)));

   // Block settings : Rotations
   GETSPINBOX("RotateXED")->setRange(-360, 360);      GETSLIDER("RotateXSLD")->setRange(-360, 360);
   GETSPINBOX("RotateYED")->setRange(-360, 360);      GETSLIDER("RotateYSLD")->setRange(-360, 360);
   GETSPINBOX("RotateZED")->setRange(-360, 360);      GETSLIDER("RotateZSLD")->setRange(-360, 360);
   //connect(GETUI("RotateXED"), SIGNAL(valueChanged(int)), this, SLOT(s_BlockSettings_RotateXValue(int)));     connect(GETUI("RotateXSLD"), SIGNAL(valueChanged(int)), this, SLOT(s_BlockSettings_RotateXValue(int)));        connect(GETUI("ResetRotateXBT"), SIGNAL(released()), this, SLOT(s_BlockSettings_ResetRotateXValue()));
   //connect(GETUI("RotateYED"), SIGNAL(valueChanged(int)), this, SLOT(s_BlockSettings_RotateYValue(int)));     connect(GETUI("RotateYSLD"), SIGNAL(valueChanged(int)), this, SLOT(s_BlockSettings_RotateYValue(int)));        connect(GETUI("ResetRotateYBT"), SIGNAL(released()), this, SLOT(s_BlockSettings_ResetRotateYValue()));
   //connect(GETUI("RotateZED"), SIGNAL(valueChanged(int)), this, SLOT(s_BlockSettings_RotateZValue(int)));     connect(GETUI("RotateZSLD"), SIGNAL(valueChanged(int)), this, SLOT(s_BlockSettings_RotateZValue(int)));        connect(GETUI("ResetRotateZBT"), SIGNAL(released()), this, SLOT(s_BlockSettings_ResetRotateZValue()));


   // Init combo box Background form
   for (int i = 0; i < ShapeFormDefinition.count(); i++) 
      if (ShapeFormDefinition.at(i).Enable) 
         GETCOMBOBOX("BackgroundFormCB")->addItem(ShapeFormDefinition.at(i).Name, QVariant(i));
   MakeFormIcon(GETCOMBOBOX("BackgroundFormCB"));

   if (GETCOMBOBOX("ShadowEffectCB")->view()->width() < 500)        
      GETCOMBOBOX("ShadowEffectCB")->view()->setFixedWidth(500);

   // Init combo box Background shadow form
   auto ShadowEffectCB = GETCOMBOBOX("ShadowEffectCB");
   ShadowEffectCB->addItem(QApplication::translate("DlgSlideProperties", "None"));
   ShadowEffectCB->addItem(QApplication::translate("DlgSlideProperties", "Shadow upper left"));
   ShadowEffectCB->addItem(QApplication::translate("DlgSlideProperties", "Shadow upper right"));
   ShadowEffectCB->addItem(QApplication::translate("DlgSlideProperties", "Shadow bottom left"));
   ShadowEffectCB->addItem(QApplication::translate("DlgSlideProperties", "Shadow bottom right"));
   GETSPINBOX("ShadowEffectED")->setRange(1, 100);

   // Init combo box external border style
   auto PenStyleCB = GETCOMBOBOX("PenStyleCB");
   PenStyleCB->addItem("");    PenStyleCB->setItemData(PenStyleCB->count() - 1, (int)Qt::SolidLine);
   PenStyleCB->addItem("");    PenStyleCB->setItemData(PenStyleCB->count() - 1, (int)Qt::DashLine);
   PenStyleCB->addItem("");    PenStyleCB->setItemData(PenStyleCB->count() - 1, (int)Qt::DotLine);
   PenStyleCB->addItem("");    PenStyleCB->setItemData(PenStyleCB->count() - 1, (int)Qt::DashDotLine);
   PenStyleCB->addItem("");    PenStyleCB->setItemData(PenStyleCB->count() - 1, (int)Qt::DashDotDotLine);
   MakeBorderStyleIcon(PenStyleCB);

   // Init shape Borders
   GETSPINBOX("PenSizeEd")->setMinimum(0);    GETSPINBOX("PenSizeEd")->setMaximum(30);

   // Init combo box Background opacity
   GETCOMBOBOX("OpacityCB")->addItem("100%");
   GETCOMBOBOX("OpacityCB")->addItem(" 75%");
   GETCOMBOBOX("OpacityCB")->addItem(" 50%");
   GETCOMBOBOX("OpacityCB")->addItem(" 25%");

   connect(GETUI("ShapeSizePosCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapeSizePos(int)));
   connect(GETUI("BackgroundFormCB"), SIGNAL(itemSelectionHaveChanged()), this, SLOT(s_BlockSettings_ShapeBackgroundForm()));
   connect(GETUI("TextClipArtCB"), SIGNAL(itemSelectionHaveChanged()), this, SLOT(s_BlockSettings_ShapeTextClipArtChIndex()));
   connect(GETUI("OpacityCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapeOpacity(int)));
   connect(GETUI("PenStyleCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapePenStyle(int)));
   connect(GETUI("ShadowEffectCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapeShadowFormValue(int)));
   connect(GETUI("ShadowEffectED"), SIGNAL(valueChanged(int)), this, SLOT(s_BlockSettings_ShapeShadowDistanceValue(int)));
   connect(GETUI("PenColorCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapePenColor(int)));
   connect(GETUI("PenSizeEd"), SIGNAL(valueChanged(int)), this, SLOT(s_BlockSettings_ShapePenSize(int)));
   connect(GETUI("ShadowColorCB"), SIGNAL(currentIndexChanged(int)), this, SLOT(s_BlockSettings_ShapeShadowColor(int)));
   connect(GETUI("BlockShapeStyleBT"), SIGNAL(pressed()), this, SLOT(s_BlockSettings_BlockShapeStyleBT()));

   s_Event_ClipboardChanged();           // Setup clipboard button state
   connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(s_Event_ClipboardChanged()));
   bConnectedSource = false;

   // mask 
   if (hasMaskControls)
   {
      if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
      {
         GETDOUBLESPINBOX("MaskPosX")->setDecimals(2);      GETDOUBLESPINBOX("MaskPosX")->setSingleStep(1);     GETDOUBLESPINBOX("MaskPosX")->setSuffix("%");
         GETDOUBLESPINBOX("MaskPosY")->setDecimals(2);      GETDOUBLESPINBOX("MaskPosY")->setSingleStep(1);     GETDOUBLESPINBOX("MaskPosY")->setSuffix("%");
         GETDOUBLESPINBOX("MaskWidth")->setDecimals(2);     GETDOUBLESPINBOX("MaskWidth")->setSingleStep(1);    GETDOUBLESPINBOX("MaskWidth")->setSuffix("%");
         GETDOUBLESPINBOX("MaskHeight")->setDecimals(2);    GETDOUBLESPINBOX("MaskHeight")->setSingleStep(1);   GETDOUBLESPINBOX("MaskHeight")->setSuffix("%");
         GETDOUBLESPINBOX("MaskPosX")->setRange(-100, 100); //GETDOUBLESPINBOX("MaskPosX")->setValue(CurrentCompoObject->posX() * 100 / Ratio_X);
         GETDOUBLESPINBOX("MaskPosY")->setRange(-100, 100); //GETDOUBLESPINBOX("MaskPosY")->setValue(CurrentCompoObject->posY() * 100 / Ratio_Y);
         GETDOUBLESPINBOX("MaskWidth")->setRange(1, 100);   //GETDOUBLESPINBOX("MaskWidth")->setValue(CurrentCompoObject->width() * 100 / Ratio_X);
         GETDOUBLESPINBOX("MaskHeight")->setRange(1, 100);  //GETDOUBLESPINBOX("MaskHeight")->setValue(CurrentCompoObject->height() * 100 / Ratio_Y);
      }
      else
      { // DisplayUnit==DISPLAYUNIT_PIXELS
         GETDOUBLESPINBOX("MaskPosX")->setDecimals(0);     GETDOUBLESPINBOX("MaskPosX")->setSingleStep(1);       GETDOUBLESPINBOX("MaskPosX")->setSuffix("");
         GETDOUBLESPINBOX("MaskPosY")->setDecimals(0);     GETDOUBLESPINBOX("MaskPosY")->setSingleStep(1);       GETDOUBLESPINBOX("MaskPosY")->setSuffix("");
         GETDOUBLESPINBOX("MaskWidth")->setDecimals(0);    GETDOUBLESPINBOX("MaskWidth")->setSingleStep(1);      GETDOUBLESPINBOX("MaskWidth")->setSuffix("");
         GETDOUBLESPINBOX("MaskHeight")->setDecimals(0);   GETDOUBLESPINBOX("MaskHeight")->setSingleStep(1);     GETDOUBLESPINBOX("MaskHeight")->setSuffix("");
         GETDOUBLESPINBOX("MaskPosX")->setRange(-2 * InteractiveZone->DisplayW, 2 * InteractiveZone->DisplayW);  //GETDOUBLESPINBOX("MaskPosX")->setValue(CurrentCompoObject->posX()*InteractiveZone->DisplayW / Ratio_X);
         GETDOUBLESPINBOX("MaskPosY")->setRange(-2 * InteractiveZone->DisplayH, 2 * InteractiveZone->DisplayH);  //GETDOUBLESPINBOX("MaskPosY")->setValue(CurrentCompoObject->posY()*InteractiveZone->DisplayH / Ratio_Y);
         GETDOUBLESPINBOX("MaskWidth")->setRange(3, 2 * InteractiveZone->DisplayW);                              //GETDOUBLESPINBOX("MaskWidth")->setValue(CurrentCompoObject->width()*InteractiveZone->DisplayW / Ratio_X);
         GETDOUBLESPINBOX("MaskHeight")->setRange(3, 2 * InteractiveZone->DisplayH);                             //GETDOUBLESPINBOX("MaskHeight")->setValue(CurrentCompoObject->height()*InteractiveZone->DisplayH / Ratio_Y);
      }
   }
   // Init combo box mask shape form
   //for (int i = 0; i < ShapeFormDefinition.count(); i++) 
   //   if (ShapeFormDefinition.at(i).Enable) 
   //      GETCOMBOBOX("cbMaskShape")->addItem(ShapeFormDefinition.at(i).Name, QVariant(i));
   //MakeFormIcon(GETCOMBOBOX("cbMaskShape"));
   // init z-rotation-origin combobox
   QComboBox *rotOriginCB = GETCOMBOBOX("cbRotateZOrigin");
   if (rotOriginCB != 0)
   {
      rotOriginCB->addItem("center");          rotOriginCB->setItemData(rotOriginCB->count() - 1, (int)eRotCenter);
      rotOriginCB->addItem("top left");        rotOriginCB->setItemData(rotOriginCB->count() - 1, (int)eRotTopLeft);
      rotOriginCB->addItem("top center");      rotOriginCB->setItemData(rotOriginCB->count() - 1, (int)eRotTopMiddle);
      rotOriginCB->addItem("top right");       rotOriginCB->setItemData(rotOriginCB->count() - 1, (int)eRotTopRight);
      rotOriginCB->addItem("mid left");        rotOriginCB->setItemData(rotOriginCB->count() - 1, (int)eRotCenterLeft);
      rotOriginCB->addItem("mid right");       rotOriginCB->setItemData(rotOriginCB->count() - 1, (int)eRotCenterRight);
      rotOriginCB->addItem("bottom left");     rotOriginCB->setItemData(rotOriginCB->count() - 1, (int)eRotBottomLeft);
      rotOriginCB->addItem("bottom center");   rotOriginCB->setItemData(rotOriginCB->count() - 1, (int)eRotBottomMiddle);
      rotOriginCB->addItem("bottom right");    rotOriginCB->setItemData(rotOriginCB->count() - 1, (int)eRotBottomRight);
   }
}

//====================================================================================================================
// Utility functions
//====================================================================================================================

// Fill background combobox
void cShotComposer::MakeFormIcon(QComboBox *UICB)
{
   for (int i = 0; i < UICB->count(); i++)
   {
      cCompositionObject Object(eCOMPOSITIONTYPE_BACKGROUND, 0, ApplicationConfig, this);
      Object.setRect(QRectF(0, 0, 1, 1));
      Object.BackgroundForm = UICB->itemData(i).toInt();
      Object.Opacity = 4;
      Object.PenSize = 1;
      Object.PenStyle = Qt::SolidLine;
      Object.PenColor = "#000000";
      Object.BackgroundBrush->ColorD = "#FFFFFF";
      Object.BackgroundBrush->BrushType = BRUSHTYPE_SOLID;
      QPixmap  Image(UICB->iconSize());
      QPainter Painter;
      Painter.begin(&Image);
      Painter.fillRect(QRect(0, 0, UICB->iconSize().width(), UICB->iconSize().height()), "#ffffff");
      Object.DrawCompositionObject(NULL, &Painter, 1, UICB->iconSize(), true, 0, NULL, 1, 1, NULL, false, 0, false);
      Painter.end();
      UICB->setItemIcon(i, QIcon(Image));
   }
}

// Fill border combobox
void cShotComposer::MakeBorderStyleIcon(QComboBox *UICB) 
{
   for (int i = 0; i < UICB->count(); i++)
   {
      QPixmap  Image(32, 32);
      QPainter Painter;
      Painter.begin(&Image);
      Painter.fillRect(QRect(0, 0, 32, 32), "#ffffff");
      QPen Pen;
      Pen.setColor(Qt::black);
      Pen.setStyle((Qt::PenStyle)UICB->itemData(i).toInt());
      Pen.setWidth(2);
      Painter.setPen(Pen);
      Painter.setBrush(QBrush(QColor("#ffffff")));
      Painter.drawLine(0, 16, 32, 16);
      Painter.end();
      UICB->setItemIcon(i, QIcon(Image));
   }
}

void cShotComposer::ComputeBlockRatio(cCompositionObject *Block, double &Ratio_X, double &Ratio_Y)
{
   if (!Block)
      return;
   getShapeRatio(Block->BackgroundForm,&Ratio_X, &Ratio_Y);
   /*
   QRectF r = Block->appliedRect(InteractiveZone->DisplayW, InteractiveZone->DisplayH);
   QRectF tmpRect = PolygonToRectF(ComputePolygon(Block->BackgroundForm, r));
   Ratio_X = (Block->width() * InteractiveZone->DisplayW) / tmpRect.width();
   Ratio_Y = (Block->height() * InteractiveZone->DisplayH) / tmpRect.height();
   qDebug() << "BlockRatio now " << Ratio_X << " " << Ratio_Y;

   r = Block->appliedRect(100,100);
   tmpRect = PolygonToRectF(ComputePolygon(Block->BackgroundForm, r));
   Ratio_X = (Block->width() * 100.0) / tmpRect.width();
   Ratio_Y = (Block->height() * 100.0) / tmpRect.height();
   qDebug() << "BlockRatio now " << Ratio_X << " " << Ratio_Y;
   qreal rx, ry;
   getShapeRatio(Block->BackgroundForm,&rx, &ry);
   qDebug() << "BlockRatio now " << rx << " " << ry;
   */
}

void cShotComposer::ResetThumbs(bool ResetAllThumbs) 
{
   if (ShotTable)
      for (int i = (ResetAllThumbs ? 0 : CurrentShotNbr); i < CurrentSlide->shotList.count(); i++)
      {
         if (i == 0)
            ApplicationConfig->SlideThumbsTable->ClearThumb(CurrentSlide->ThumbnailKey);
         ShotTable->RepaintCell(i);
      }
}

void cShotComposer::ApplyToContexte(bool ResetAllThumbs)
{
   if (!CurrentCompoObject)
      return;

   // Apply to GlobalComposition objects
   CurrentCompoObject->CopyBlockPropertiesTo(CurrentSlide->ObjectComposition.getCompositionObject(CurrentCompoObject->index()));

   // Apply to Shots Composition objects
   for (int i = 0; i < CurrentSlide->shotList.count(); i++)
   {
      CurrentCompoObject->CopyBlockPropertiesTo(CurrentSlide->shotList.at(i)->ShotComposition.getCompositionObject(CurrentCompoObject->index()));
      pAppConfig->ImagesCache.RemoveImageObject(CurrentSlide->shotList.at(i)->ShotComposition.getCompositionObject(CurrentCompoObject->index())->BackgroundBrush);
      // remove shot-related chached Images
      pAppConfig->ImagesCache.RemoveImageObject(CurrentSlide->shotList.at(i));
   }

   // Reset thumbs if needed
   ResetThumbs(ResetAllThumbs);

   // Reset blocks table
   RefreshBlockTable(-1/*CurrentCompoObjectNbr*/);

   // Reset controls
   RefreshControls(true);
}

cCompositionObject *cShotComposer::GetGlobalCompositionObject(int IndexKey)
{
   int CurrentBlock = BlockTable->currentRow();
   if ((CurrentBlock < 0) || (CurrentBlock >= CompositionList->compositionObjects.count()))
      return NULL;
   return CurrentSlide->ObjectComposition.getCompositionObject(IndexKey);
}

//====================================================================================================================

void cShotComposer::s_Event_ClipboardChanged()
{
   bool    HasImage = (QApplication::clipboard()) && (QApplication::clipboard()->mimeData()) && (QApplication::clipboard()->mimeData()->hasImage());
   bool    CanPaste = (QApplication::clipboard()) && (QApplication::clipboard()->mimeData()) && (QApplication::clipboard()->mimeData()->hasFormat("ffDiaporama/block"));
   if (actionAddImageClipboard)
      actionAddImageClipboard->setEnabled(HasImage);
   if (actionPaste)
      actionPaste->setEnabled(CanPaste);
}

//====================================================================================================================

void cShotComposer::RefreshControls(bool UpdateInteractiveZone) {
   InRefreshControls = true;
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   bool bEnable = CurrentCompoObject && CurrentCompoObject->isVisible();
   if ((BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObject)/*&&(CurrentCompoObject->IsVisible)*/)
   {
      qreal Ratio_X, Ratio_Y;
      ComputeBlockRatio(CurrentCompoObject, Ratio_X, Ratio_Y);

      if (GETUI("InheritDownCB"))
      {
         GETUI("InheritDownCB")->setEnabled(CurrentShotNbr != 0);
         if (GETCHECKBOX("InheritDownCB")->isChecked() != CurrentCompoObject->hasInheritance())
            GETCHECKBOX("InheritDownCB")->setChecked(CurrentCompoObject->hasInheritance());
      }

      // Position, size and rotation
      GETUI("PosSize_X")->setEnabled(bEnable);
      GETUI("PosSize_Y")->setEnabled(bEnable);
      GETUI("PosSize_Width")->setEnabled(bEnable);
      GETUI("PosSize_Height")->setEnabled(bEnable);
      GETDOUBLESPINBOX("PosXEd")->setEnabled(bEnable);
      GETDOUBLESPINBOX("PosYEd")->setEnabled(bEnable);
      GETDOUBLESPINBOX("WidthEd")->setEnabled(bEnable);
      GETDOUBLESPINBOX("HeightEd")->setEnabled(bEnable);

      if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
      {
         GETDOUBLESPINBOX("PosXEd")->setRange(-200, 200); GETDOUBLESPINBOX("PosXEd")->setValue(CurrentCompoObject->posX() * 100 /*/ Ratio_X*/);
         GETDOUBLESPINBOX("PosYEd")->setRange(-200, 200); GETDOUBLESPINBOX("PosYEd")->setValue(CurrentCompoObject->posY() * 100 /*/ Ratio_Y*/);
         GETDOUBLESPINBOX("WidthEd")->setRange(1, 200);   GETDOUBLESPINBOX("WidthEd")->setValue(CurrentCompoObject->width() * 100 / Ratio_X);
         GETDOUBLESPINBOX("HeightEd")->setRange(1, 200);  GETDOUBLESPINBOX("HeightEd")->setValue(CurrentCompoObject->height() * 100 / Ratio_Y);
      }
      else
      { 
         // DisplayUnit==DISPLAYUNIT_PIXELS
         double value = CurrentCompoObject->posX()*InteractiveZone->DisplayW/* / Ratio_X*/;
         //double roundedValue = QString::number(value, 'f', 0).toDouble();
         GETDOUBLESPINBOX("PosXEd")->setRange(-2 * InteractiveZone->DisplayW, 2 * InteractiveZone->DisplayW);  
         GETDOUBLESPINBOX("PosXEd")->setValue(value);
         //if(value != roundedValue)
         //   GETDOUBLESPINBOX("PosXEd")->setStyleSheet("background-color: yellow");
         //else
         //   GETDOUBLESPINBOX("PosXEd")->setStyleSheet("background-color: white");

         value = CurrentCompoObject->posY()*InteractiveZone->DisplayH /*/ Ratio_Y*/;
         //roundedValue = QString::number(value, 'f', 0).toDouble();
         GETDOUBLESPINBOX("PosYEd")->setRange(-2 * InteractiveZone->DisplayH, 2 * InteractiveZone->DisplayH);  
         GETDOUBLESPINBOX("PosYEd")->setValue(value);
         //if(value != roundedValue)
         //   GETDOUBLESPINBOX("PosYEd")->setStyleSheet("background-color: yellow");
         //else
         //   GETDOUBLESPINBOX("PosYEd")->setStyleSheet("background-color: white");

         value = CurrentCompoObject->width()*InteractiveZone->DisplayW / Ratio_X;
         //roundedValue = QString::number(value, 'f', 0).toDouble();
         GETDOUBLESPINBOX("WidthEd")->setRange(3, 2 * InteractiveZone->DisplayW);                             
         GETDOUBLESPINBOX("WidthEd")->setValue(value);
         //if(value != roundedValue)
         //   GETDOUBLESPINBOX("WidthEd")->setStyleSheet("background-color: yellow");
         //else
         //   GETDOUBLESPINBOX("WidthEd")->setStyleSheet("background-color: white");

         value = CurrentCompoObject->height()*InteractiveZone->DisplayH / Ratio_Y;
         //roundedValue = QString::number(value, 'f', 0).toDouble();
         GETDOUBLESPINBOX("HeightEd")->setRange(3, 2 * InteractiveZone->DisplayH);                             
         GETDOUBLESPINBOX("HeightEd")->setValue(value);
         //if(value != roundedValue)
         //   GETDOUBLESPINBOX("HeightEd")->setStyleSheet("background-color: yellow");
         //else
         //   GETDOUBLESPINBOX("HeightEd")->setStyleSheet("background-color: white");
      }

      // Rotation
      GETUI("Rotate_X")->setEnabled(bEnable);
      GETSPINBOX("RotateXED")->setEnabled(bEnable);
      GETUI("ResetRotateXBT")->setEnabled(bEnable);
      GETSLIDER("RotateXSLD")->setEnabled(bEnable);
      GETUI("Rotate_Y")->setEnabled(bEnable);
      GETSPINBOX("RotateYED")->setEnabled(bEnable);
      GETUI("ResetRotateYBT")->setEnabled(bEnable);
      GETSLIDER("RotateYSLD")->setEnabled(bEnable);
      GETUI("Rotate_Z")->setEnabled(bEnable);
      GETSPINBOX("RotateZED")->setEnabled(bEnable);
      GETUI("ResetRotateZBT")->setEnabled(bEnable);
      GETSLIDER("RotateZSLD")->setEnabled(bEnable);
      if(GETUI("cbRotateZOrigin") != 0)
         GETUI("cbRotateZOrigin")->setEnabled(bEnable);

      GETSPINBOX("RotateXED")->setValue(CurrentCompoObject->xAngle()); GETSLIDER("RotateXSLD")->setValue(CurrentCompoObject->xAngle());
      GETSPINBOX("RotateYED")->setValue(CurrentCompoObject->yAngle()); GETSLIDER("RotateYSLD")->setValue(CurrentCompoObject->yAngle());
      GETSPINBOX("RotateZED")->setValue(CurrentCompoObject->zAngle()); GETSLIDER("RotateZSLD")->setValue(CurrentCompoObject->zAngle());
      SetCBIndex(GETCOMBOBOX("cbRotateZOrigin"), CurrentCompoObject->zRotationOrigin());

      // Shape part
      GETUI("BlockShapeStyleBT")->setEnabled(bEnable);
      GETUI("BlockShapeStyleED")->setEnabled(bEnable);
      GETUI("BackgroundFormCB")->setEnabled(bEnable);
      GETUI("PenSizeEd")->setEnabled(bEnable);
      GETUI("PenColorCB")->setEnabled(CurrentCompoObject->PenSize != 0 && bEnable);
      GETUI("PenStyleCB")->setEnabled(CurrentCompoObject->PenSize != 0 && bEnable);
      GETUI("OpacityCB")->setEnabled(bEnable);
      GETUI("ShadowEffectCB")->setEnabled(bEnable);
      GETUI("ShadowEffectED")->setEnabled(CurrentCompoObject->FormShadow != 0 && bEnable);
      GETUI("ShadowColorCB")->setEnabled(CurrentCompoObject->FormShadow != 0 && bEnable);

      SetCBIndex(GETCOMBOBOX("BackgroundFormCB"), CurrentCompoObject->BackgroundForm);
      GETSPINBOX("PenSizeEd")->setValue(int(CurrentCompoObject->PenSize));
      GETCOMBOBOX("OpacityCB")->setCurrentIndex(CurrentCompoObject->Opacity);
      GETCOMBOBOX("ShadowEffectCB")->setCurrentIndex(CurrentCompoObject->FormShadow);
      GETSPINBOX("ShadowEffectED")->setValue(int(CurrentCompoObject->FormShadowDistance));
      ((cColorComboBox *)GETCOMBOBOX("PenColorCB"))->SetCurrentColor(CurrentCompoObject->PenColor);
      ((cCColorComboBox *)GETCOMBOBOX("ShadowColorCB"))->SetCurrentColor(&CurrentCompoObject->FormShadowColor);

      for (int i = 0; i < GETCOMBOBOX("PenStyleCB")->count(); i++)
         if (GETCOMBOBOX("PenStyleCB")->itemData(i).toInt() == CurrentCompoObject->PenStyle)
         {
            if (i != GETCOMBOBOX("PenStyleCB")->currentIndex()) GETCOMBOBOX("PenStyleCB")->setCurrentIndex(i);
            break;
         }
      cCompositionObjectMask *mask = CurrentCompoObject->getMask();
      // mask Values
      if (hasMaskControls)
      {
         if (CurrentCompoObject->usingMask())
         {
            GETCHECKBOX("cmUseMask")->setChecked(true);
            GETCHECKBOX("cmInvertMask")->setChecked(mask->invertedMask());
            if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
            {
               GETDOUBLESPINBOX("MaskPosX")->setValue(mask->getX()*100.0);
               GETDOUBLESPINBOX("MaskPosY")->setValue(mask->getY()*100.0);
               GETDOUBLESPINBOX("MaskWidth")->setValue(mask->width()*100.0);
               GETDOUBLESPINBOX("MaskHeight")->setValue(mask->height()*100.0);
            }
            else
            {
               GETDOUBLESPINBOX("MaskPosX")->setValue(CurrentCompoObject->width() * InteractiveZone->DisplayW / Ratio_X * mask->getX());
               GETDOUBLESPINBOX("MaskPosY")->setValue(CurrentCompoObject->height() * InteractiveZone->DisplayH / Ratio_Y * mask->getY());
               GETDOUBLESPINBOX("MaskWidth")->setValue((CurrentCompoObject->width() * InteractiveZone->DisplayW / Ratio_X) * mask->width());
               GETDOUBLESPINBOX("MaskHeight")->setValue((CurrentCompoObject->height() * InteractiveZone->DisplayH / Ratio_Y) * mask->height());
            }
            GETSPINBOX("RotateMaskZED")->setValue(mask->zAngle());
            GETSLIDER("RotateZMaskSLD")->setValue(mask->zAngle());
         }
         else
         {
            GETCHECKBOX("cmUseMask")->setChecked(false);
            GETCHECKBOX("cmInvertMask")->setChecked(false);
            //GETDOUBLESPINBOX("MaskPosX")->setValue(0);
            //GETDOUBLESPINBOX("MaskPosY")->setValue(0);
            //GETDOUBLESPINBOX("MaskWidth")->setValue(0);
            //GETDOUBLESPINBOX("MaskHeight")->setValue(0);
            //GETSPINBOX("RotateMaskZED")->setValue(0); 
            //GETSLIDER("RotateZMaskSLD")->setValue(0);
         }
         setMaskingEnabled(true);
         GETCHECKBOX("cm3DFrame")->setChecked(CurrentCompoObject->using3DFrame());
      }
   }
   else
   {
      if (GETUI("InheritDownCB"))
      {
         GETUI("InheritDownCB")->setEnabled(false);
         GETCHECKBOX("InheritDownCB")->setChecked(false);
      }

      // Position, size and rotation
      GETUI("PosSize_X")->setEnabled(false);
      GETUI("PosSize_Y")->setEnabled(false);
      GETUI("PosSize_Width")->setEnabled(false);
      GETUI("PosSize_Height")->setEnabled(false);
      GETDOUBLESPINBOX("PosXEd")->setEnabled(false);  GETDOUBLESPINBOX("PosXEd")->setValue(0);
      GETDOUBLESPINBOX("PosYEd")->setEnabled(false);  GETDOUBLESPINBOX("PosYEd")->setValue(0);
      GETDOUBLESPINBOX("WidthEd")->setEnabled(false);  GETDOUBLESPINBOX("WidthEd")->setValue(0); GETDOUBLESPINBOX("WidthEd")->setRange(0, 2 * InteractiveZone->DisplayW);
      GETDOUBLESPINBOX("HeightEd")->setEnabled(false);  GETDOUBLESPINBOX("HeightEd")->setValue(0); GETDOUBLESPINBOX("HeightEd")->setRange(0, 2 * InteractiveZone->DisplayH);


      // Rotation
      GETUI("Rotate_X")->setEnabled(false); GETSPINBOX("RotateXED")->setEnabled(false); GETUI("ResetRotateXBT")->setEnabled(false); GETSLIDER("RotateXSLD")->setEnabled(false);
      GETUI("Rotate_Y")->setEnabled(false); GETSPINBOX("RotateYED")->setEnabled(false); GETUI("ResetRotateYBT")->setEnabled(false); GETSLIDER("RotateYSLD")->setEnabled(false);
      GETUI("Rotate_Z")->setEnabled(false); GETSPINBOX("RotateZED")->setEnabled(false); GETUI("ResetRotateZBT")->setEnabled(false); GETSLIDER("RotateZSLD")->setEnabled(false);

      GETSPINBOX("RotateXED")->setValue(0); GETSLIDER("RotateXSLD")->setValue(0);
      GETSPINBOX("RotateYED")->setValue(0); GETSLIDER("RotateYSLD")->setValue(0);
      GETSPINBOX("RotateZED")->setValue(0); GETSLIDER("RotateZSLD")->setValue(0);

      // Shape part
      GETUI("BlockShapeStyleBT")->setEnabled(false);
      GETUI("BlockShapeStyleED")->setEnabled(false);
      GETUI("BackgroundFormCB")->setEnabled(false);         GETCOMBOBOX("BackgroundFormCB")->setCurrentIndex(-1);
      GETUI("PenSizeEd")->setEnabled(false);         GETSPINBOX("PenSizeEd")->setValue(0);

      GETUI("PenColorCB")->setEnabled(false);         ((cColorComboBox *)GETCOMBOBOX("PenColorCB"))->SetCurrentColor(NULL);
      GETUI("PenStyleCB")->setEnabled(false);
      GETUI("OpacityCB")->setEnabled(false);         GETCOMBOBOX("OpacityCB")->setCurrentIndex(-1);
      GETUI("ShadowEffectCB")->setEnabled(false);         GETCOMBOBOX("ShadowEffectCB")->setCurrentIndex(-1);
      GETUI("ShadowEffectED")->setEnabled(false);         GETSPINBOX("ShadowEffectED")->setValue(0);
      GETUI("ShadowColorCB")->setEnabled(false);         ((cCColorComboBox *)GETCOMBOBOX("ShadowColorCB"))->SetCurrentColor(NULL);
      // mask Values
      setMaskingEnabled(false);
      //GETCOMBOBOX("cbMaskShape")->setCurrentIndex(-1);
      GETDOUBLESPINBOX("MaskPosX")->setValue(0);
      GETDOUBLESPINBOX("MaskPosY")->setValue(0);
      GETDOUBLESPINBOX("MaskWidth")->setValue(0);
      GETDOUBLESPINBOX("MaskHeight")->setValue(0);
      GETSPINBOX("RotateMaskZED")->setValue(0); 
      GETSLIDER("RotateZMaskSLD")->setValue(0);
   }

   // Set control visible or hide depending on TextClipArt
   bool textControlVisible = (!CurrentCompoObject || CurrentCompoObject->tParams.TextClipArtName == "");
   GETUI("BlockShapeStyleBT")->setVisible(textControlVisible);
   GETUI("BlockShapeStyleSpacer")->setVisible(textControlVisible);
   GETUI("BlockShapeStyleED")->setVisible(textControlVisible);
   GETUI("BackgroundFormCB")->setVisible(textControlVisible);
   GETUI("BackgroundFormLabel")->setVisible(textControlVisible);
   GETUI("PenSizeEd")->setVisible(textControlVisible);
   GETUI("PenColorCB")->setVisible(textControlVisible);
   GETUI("PenStyleCB")->setVisible(textControlVisible);
   GETUI("PenLabel")->setVisible(textControlVisible);
   GETUI("TextClipArtCB")->setVisible((CurrentCompoObject) && (CurrentCompoObject->tParams.TextClipArtName != ""));
   GETUI("TextClipArtLabel")->setVisible((CurrentCompoObject) && (CurrentCompoObject->tParams.TextClipArtName != ""));
   if ((CurrentCompoObject) && (CurrentCompoObject->tParams.TextClipArtName != ""))
      ((cCTexteFrameComboBox *)GETCOMBOBOX("TextClipArtCB"))->SetCurrentTextFrame(CurrentCompoObject->tParams.TextClipArtName);

   InRefreshControls = false;
   QApplication::restoreOverrideCursor();

   // Refresh interactive zone display and shot thumbnail
   if (UpdateInteractiveZone)  InteractiveZone->RefreshDisplay();
   else                        InteractiveZone->repaint(); // else refresh selection only
}

//====================================================================================================================

void cShotComposer::s_BlockSettings_BlockInheritances()
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   if (CurrentShotNbr == 0)
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "InheritDownCB", true);
   CurrentCompoObject->setInheritance(GETCHECKBOX("InheritDownCB")->isChecked());
   if (CurrentCompoObject->hasInheritance())
   {
      // Search this block in previous shot
      cCompositionObject *SourceCompo = NULL;
      SourceCompo = CurrentSlide->shotList[CurrentShotNbr - 1]->ShotComposition.getCompositionObject(CurrentCompoObject->index());
      //for (int Block = 0; Block < CurrentSlide->List[CurrentShotNbr-1]->ShotComposition.List.count(); Block++)
      //  if (CurrentSlide->List[CurrentShotNbr-1]->ShotComposition.List[Block]->IndexKey == CurrentCompoObject->IndexKey)
      //      SourceCompo = CurrentSlide->List[CurrentShotNbr-1]->ShotComposition.List[Block];

      if ((SourceCompo) && (CustomMessageBox(this, QMessageBox::Question, QApplication::translate("DlgSlideProperties", "Reactivate the inheritance of changes"),
         QApplication::translate("DlgSlideProperties", "Do you want to apply to this block the properties it has in the previous shot?"),
         QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes))
      {
         bool ContAPPLY = true;
         int  ShotNum = CurrentShotNbr;
         while ((ContAPPLY) && (ShotNum < CurrentSlide->shotList.count()))
         {
            for (int Block = 0; ContAPPLY && Block < CurrentSlide->shotList[CurrentShotNbr]->ShotComposition.compositionObjects.count(); Block++)
               for (int ToSearch = 0; ContAPPLY && ToSearch < CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects.count(); ToSearch++)
                  if (CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch]->index() == CurrentCompoObject->index())
                  {
                     cCompositionObject *ShotObject = CurrentSlide->shotList[ShotNum]->ShotComposition.compositionObjects[ToSearch];
                     if (ShotObject->hasInheritance())
                     {
                        ShotObject->setRect(SourceCompo->getRect());
                        ShotObject->setZAngle(SourceCompo->zAngle());
                        ShotObject->setXAngle(SourceCompo->xAngle());
                        ShotObject->setYAngle(SourceCompo->yAngle());
                        ShotObject->setSpeedWave(SourceCompo->SpeedWave());
                        ShotObject->setAnimType(SourceCompo->AnimType());
                        ShotObject->setZTurns(SourceCompo->ZTurns());
                        ShotObject->setXTurns(SourceCompo->XTurns());
                        ShotObject->setYTurns(SourceCompo->YTurns());
                        ShotObject->setDissolveType(SourceCompo->DissolveType());
                        ShotObject->BackgroundBrush->CopyFromBrushDefinition(SourceCompo->BackgroundBrush);
                     }
                     else
                        ContAPPLY = false;
                  }
            ShotNum++;
         }
      }
   }
   ApplyToContexte(false);
}

//====================================================================================================================
// Handler for position, size & rotation controls
//====================================================================================================================

//========= X position
void cShotComposer::s_BlockSettings_PosXValue(double Value)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "PosXEd", false);
   qreal Ratio_X, Ratio_Y;
   ComputeBlockRatio(CurrentCompoObject, Ratio_X, Ratio_Y);
   if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
      CurrentCompoObject->setX(Value / 100 /** Ratio_X*/);                            // DisplayUnit==DISPLAYUNIT_PERCENT
   else
      CurrentCompoObject->setX((Value / InteractiveZone->DisplayW /** Ratio_X*/));    // DisplayUnit==DISPLAYUNIT_PIXELS
   // Apply values of previous shot to all shot for this object
   //APPLY1TONEXT(x);
   APPLY_GETSET_TONEXT(X);
   ApplyToContexte(false);
   // test
   /*
   if( CurrentShotNbr > 0 )
   {
      // Search this block in previous shot
      cCompositionObject *SourceCompo = NULL;
      SourceCompo = CurrentSlide->shotList[CurrentShotNbr - 1]->ShotComposition.getCompositionObject(CurrentCompoObject->index());
      qDebug() << "XPos move from " << SourceCompo->getX() << " to " << CurrentCompoObject->getX();
      qDebug() << "this width " << CurrentCompoObject->width() << " src width " << SourceCompo->BackgroundBrush->ZoomFactor();
      qDebug() << "SRC-XPos move from " << SourceCompo->BackgroundBrush->X() << " to " << CurrentCompoObject->BackgroundBrush->X();
      qreal deltaW = SourceCompo->BackgroundBrush->ZoomFactor() / CurrentCompoObject->width();
      //qreal deltaW = CurrentCompoObject->width() / SourceCompo->BackgroundBrush->ZoomFactor();
      qreal deltaX = (CurrentCompoObject->getX()+CurrentCompoObject->width()/2) - (SourceCompo->getX()+SourceCompo->width()/2);
      qreal deltaSrc = SourceCompo->BackgroundBrush->X() + deltaX * deltaW;
      qDebug() << "deltaW " << deltaW << " deltaX " << deltaX;
      qDebug() << "SRC-XPos move from " << SourceCompo->BackgroundBrush->X() << " to " << deltaSrc;
      qreal blockCenter = SourceCompo->BackgroundBrush->X() + SourceCompo->BackgroundBrush->ZoomFactor()/2;
      qreal nblockpos = blockCenter + deltaX - SourceCompo->BackgroundBrush->ZoomFactor()/2;
      qreal Ratio_X, Ratio_Y;
      ComputeBlockRatio(CurrentCompoObject, Ratio_X, Ratio_Y);
      qDebug() << "ratiox " << Ratio_X << " ratioy " << Ratio_Y;
      qreal dx = CurrentCompoObject->getX() - SourceCompo->getX(); // x-verschiebung in %
      qreal dxs = dx / SourceCompo->BackgroundBrush->AspectRatio(); // x-verschiebung in %, scaliert
      qreal fullWidthP =  1.0/sqrt( 1.0 + SourceCompo->BackgroundBrush->AspectRatio()*SourceCompo->BackgroundBrush->AspectRatio() );
      qreal fullHeightP =  fullWidthP/SourceCompo->BackgroundBrush->AspectRatio();
      nblockpos = blockCenter + deltaX/fullWidthP - SourceCompo->BackgroundBrush->ZoomFactor()/2;
      qDebug() << "blockCenter " << blockCenter << " nblockpos " <<nblockpos;

      qreal XFactor, YFactor;
      CurrentCompoObject->getMovementFactors(AUTOCOMPOSIZE_FULLSCREEN,GEOMETRY_16_9,&XFactor, &YFactor);
      nblockpos = blockCenter + deltaX*XFactor - SourceCompo->BackgroundBrush->ZoomFactor()/2;
      qDebug() << "XFactor " << XFactor << " YFactor "<< YFactor << " nblockpos " << nblockpos;
      if( bConnectedSource )
         CurrentCompoObject->BackgroundBrush->setX(nblockpos);
      //qreal dxs2 = SourceCompo->BackgroundBrush->MediaObject->width();
   } */
}

//========= Y position
void cShotComposer::s_BlockSettings_PosYValue(double Value)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "PosYEd", false);

   qreal Ratio_X, Ratio_Y;
   ComputeBlockRatio(CurrentCompoObject, Ratio_X, Ratio_Y);

   if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
      CurrentCompoObject->setY(Value / 100 /** Ratio_Y*/ );           // DisplayUnit==DISPLAYUNIT_PERCENT
   else
      CurrentCompoObject->setY(Value / InteractiveZone->DisplayH /** Ratio_Y*/);    // DisplayUnit==DISPLAYUNIT_PIXELS
   // Apply values of previous shot to all shot for this object
   APPLY_GETSET_TONEXT(Y);
   ApplyToContexte(false);
   // test
   /*
   if( CurrentShotNbr > 0 )
   {
      // Search this block in previous shot
      cCompositionObject *SourceCompo = NULL;
      SourceCompo = CurrentSlide->shotList[CurrentShotNbr - 1]->ShotComposition.getCompositionObject(CurrentCompoObject->index());
      //qDebug() << "XPos move from " << SourceCompo->getX() << " to " << CurrentCompoObject->getX();
      //qDebug() << "this width " << CurrentCompoObject->width() << " src width " << SourceCompo->BackgroundBrush->ZoomFactor();
      //qDebug() << "SRC-XPos move from " << SourceCompo->BackgroundBrush->X() << " to " << CurrentCompoObject->BackgroundBrush->X();
      //qreal deltaW = SourceCompo->BackgroundBrush->ZoomFactor() / CurrentCompoObject->width();
      ////qreal deltaW = CurrentCompoObject->width() / SourceCompo->BackgroundBrush->ZoomFactor();
      //qreal deltaY = CurrentCompoObject->getY() - SourceCompo->getY();
      qreal deltaY = (CurrentCompoObject->getY()+CurrentCompoObject->height()/2) - (SourceCompo->getY()+SourceCompo->height()/2);
      //qreal deltaSrc = SourceCompo->BackgroundBrush->X() + deltaX * deltaW;
      //qDebug() << "deltaW " << deltaW << " deltaX " << deltaX;
      //qDebug() << "SRC-XPos move from " << SourceCompo->BackgroundBrush->X() << " to " << deltaSrc;
      qreal blockCenter = SourceCompo->BackgroundBrush->Y() + SourceCompo->BackgroundBrush->ZoomFactor()*SourceCompo->BackgroundBrush->AspectRatio()/2;
      //qreal nblockpos = blockCenter + deltaX - SourceCompo->BackgroundBrush->ZoomFactor()/2;
      //qreal Ratio_X, Ratio_Y;
      //ComputeBlockRatio(CurrentCompoObject, Ratio_X, Ratio_Y);
      //qDebug() << "ratiox " << Ratio_X << " ratioy " << Ratio_Y;
      //qreal dx = CurrentCompoObject->getX() - SourceCompo->getX(); // x-verschiebung in %
      //qreal dxs = dx / SourceCompo->BackgroundBrush->AspectRatio(); // x-verschiebung in %, scaliert
      //qreal fullWidthP =  1.0/sqrt( 1.0 + SourceCompo->BackgroundBrush->AspectRatio()*SourceCompo->BackgroundBrush->AspectRatio() );
      //qreal fullHeightP =  fullWidthP/SourceCompo->BackgroundBrush->AspectRatio();
      ////qreal nblockpos = blockCenter + deltaX/fullWidthP - SourceCompo->BackgroundBrush->ZoomFactor()/2;
      //qDebug() << "blockCenter " << blockCenter << " nblockpos " <<nblockpos;

      qreal XFactor, YFactor;
      CurrentCompoObject->getMovementFactors(AUTOCOMPOSIZE_FULLSCREEN,GEOMETRY_16_9,&XFactor, &YFactor);
      qreal nblockpos = blockCenter + deltaY*YFactor - SourceCompo->BackgroundBrush->ZoomFactor()*SourceCompo->BackgroundBrush->AspectRatio()/2;
      qDebug() << "XFactor " << XFactor << " YFactor "<< YFactor << " nblockpos " << nblockpos;
      if( bConnectedSource )
         CurrentCompoObject->BackgroundBrush->setY(nblockpos);
   }      */
}

//========= Width
void cShotComposer::s_BlockSettings_PosWidthValue(double Value)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "WidthEd", false);

   qreal Ratio_X, Ratio_Y;
   ComputeBlockRatio(CurrentCompoObject, Ratio_X, Ratio_Y);

   if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
      CurrentCompoObject->setWidth((Value / 100)*Ratio_X);
   else
      CurrentCompoObject->setWidth((Value / InteractiveZone->DisplayW)*Ratio_X);
   if (CurrentCompoObject->BackgroundBrush->LockGeometry())
      CurrentCompoObject->setHeight(((CurrentCompoObject->width()*InteractiveZone->DisplayW)*CurrentCompoObject->BackgroundBrush->AspectRatio()) / InteractiveZone->DisplayH);
   else
      CurrentCompoObject->BackgroundBrush->setAspectRatio( (CurrentCompoObject->height()*InteractiveZone->DisplayH) / (CurrentCompoObject->width()*InteractiveZone->DisplayW));
   // Apply values of previous shot to all shot for this object
   APPLY_GETSETA_TONEXT(size, setSize);
   APPLY_GETSETA_TONEXT(BackgroundBrush->AspectRatio, BackgroundBrush->setAspectRatio);
   //APPLY1TONEXT(BackgroundBrush->AspectRatio);
   //APPLY3TONEXT(w,h,BackgroundBrush->AspectRatio);
   ApplyToContexte(false);
}

//========= Height
void cShotComposer::s_BlockSettings_PosHeightValue(double Value)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "HeightEd", false);

   qreal Ratio_X, Ratio_Y;
   ComputeBlockRatio(CurrentCompoObject, Ratio_X, Ratio_Y);

   if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
      CurrentCompoObject->setHeight((Value / 100)*Ratio_Y);
   else
      CurrentCompoObject->setHeight((Value / InteractiveZone->DisplayH)*Ratio_Y);
   if (CurrentCompoObject->BackgroundBrush->LockGeometry())
      CurrentCompoObject->setWidth(((CurrentCompoObject->height()*InteractiveZone->DisplayH) / CurrentCompoObject->BackgroundBrush->AspectRatio()) / InteractiveZone->DisplayW);
   else
      CurrentCompoObject->BackgroundBrush->setAspectRatio( (CurrentCompoObject->height()*InteractiveZone->DisplayH) / (CurrentCompoObject->width()*InteractiveZone->DisplayW));
   // Apply values of previous shot to all shot for this object
   APPLY_GETSETA_TONEXT(size, setSize);
   APPLY_GETSETA_TONEXT(BackgroundBrush->AspectRatio, BackgroundBrush->setAspectRatio);
   //APPLY1TONEXT(BackgroundBrush->AspectRatio);
   //APPLY3TONEXT(w,h,BackgroundBrush->AspectRatio);
   ApplyToContexte(false);
}

//========= X Rotation value
void cShotComposer::s_BlockSettings_RotateXValue(int Value)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "RotateXED", false);
   CurrentCompoObject->setXAngle(Value);
   // Apply values of previous shot to all shot for this object
   //APPLY1TONEXT(RotateXAxis);
   APPLY_GETSETA_TONEXT(xAngle, setXAngle);
   ApplyToContexte(false);
}

//========= Y Rotation value
void cShotComposer::s_BlockSettings_RotateYValue(int Value)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "RotateYED", false);
   CurrentCompoObject->setYAngle(Value);
   // Apply values of previous shot to all shot for this object
   APPLY_GETSETA_TONEXT(yAngle, setYAngle);
   //APPLY1TONEXT(RotateYAxis);
   ApplyToContexte(false);
}

//========= Z Rotation value
void cShotComposer::s_BlockSettings_RotateZValue(int Value)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "RotateZED", false);
   CurrentCompoObject->setZAngle(Value);
   // Apply values of previous shot to all shot for this object
   APPLY_GETSETA_TONEXT(zAngle, setZAngle);
   //APPLY1TONEXT(RotateZAxis);
   ApplyToContexte(false);
}

void cShotComposer::on_cbRotateZOrigin_currentIndexChanged(int index)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   CurrentCompoObject->setZRotationOrigin(GETCOMBOBOX("cbRotateZOrigin")->itemData(index).toInt());
   ApplyToContexte(false);
}

//====================================================================================================================
// Handler for shape
//====================================================================================================================

void cShotComposer::s_BlockSettings_BlockShapeStyleBT() 
{
   if (!ISBLOCKVALIDEVISIBLE()) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, false);
   QString ActualStyle = CurrentCompoObject->GetBlockShapeStyle();
   QString Item = ApplicationConfig->StyleBlockShapeCollection.PopupCollectionMenu(this, ApplicationConfig, ActualStyle);
   GETBUTTON("BlockShapeStyleBT")->setDown(false);
   if (Item != "")
   {
      CurrentCompoObject->ApplyBlockShapeStyle(ApplicationConfig->StyleBlockShapeCollection.GetStyleDef(Item));
      ApplyToContexte(true);
   }
}

//========= Text ClipArt
void cShotComposer::s_BlockSettings_ShapeTextClipArtChIndex()
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   if (CurrentCompoObject->tParams.TextClipArtName == "")
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "TextClipArtCB", false);
   CurrentCompoObject->tParams.TextClipArtName = ((cCTexteFrameComboBox *)GETCOMBOBOX("TextClipArtCB"))->GetCurrentTextFrame();
   cTextFrameObject *TFO = &TextFrameList.List[TextFrameList.SearchImage(CurrentCompoObject->tParams.TextClipArtName)];
   CurrentCompoObject->tParams.textMargins = TFO->MarginRect();
   //CurrentCompoObject->tParams.TMx = TFO->TMx;
   //CurrentCompoObject->tParams.TMy = TFO->TMy;
   //CurrentCompoObject->tParams.TMw = TFO->TMw;
   //CurrentCompoObject->tParams.TMh = TFO->TMh;
   CurrentCompoObject->ApplyTextStyle(TFO->TextStyle);
   ApplyToContexte(true);
}

//========= Background forme
void cShotComposer::s_BlockSettings_ShapeBackgroundForm() 
{
   if (!ISBLOCKVALIDEVISIBLE()) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "BackgroundFormCB", false);
   CurrentCompoObject->BackgroundForm = ((cCShapeComboBox *)GETCOMBOBOX("BackgroundFormCB"))->GetCurrentFrameShape();
   ApplyToContexte(true);
}

//========= Opacity
void cShotComposer::s_BlockSettings_ShapeOpacity(int Style) 
{
   if (!ISBLOCKVALIDEVISIBLE()) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "OpacityCB", false);
   CurrentCompoObject->Opacity = Style;
   ApplyToContexte(true);
}

//========= Border pen size
void cShotComposer::s_BlockSettings_ShapePenSize(int) 
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "PenSizeEd", false);
   CurrentCompoObject->PenSize = GETSPINBOX("PenSizeEd")->value();
   GETCOMBOBOX("PenColorCB")->setEnabled(CurrentCompoObject->PenSize != 0);
   GETCOMBOBOX("PenStyleCB")->setEnabled(CurrentCompoObject->PenSize != 0);
   ApplyToContexte(true);
}

//========= Border pen style
void cShotComposer::s_BlockSettings_ShapePenStyle(int index) 
{
   if (!ISBLOCKVALIDEVISIBLE()) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "PenStyleCB", false);
   CurrentCompoObject->PenStyle = GETCOMBOBOX("PenStyleCB")->itemData(index).toInt();
   ApplyToContexte(true);
}

//========= Border pen color
void cShotComposer::s_BlockSettings_ShapePenColor(int) 
{
   if (!ISBLOCKVALIDEVISIBLE()) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "PenColorCB", false);
   CurrentCompoObject->PenColor = ((cColorComboBox *)GETCOMBOBOX("PenColorCB"))->GetCurrentColor();
   ApplyToContexte(true);
}

//========= Shape shadow style
void cShotComposer::s_BlockSettings_ShapeShadowFormValue(int value) 
{
   if (!ISBLOCKVALIDEVISIBLE()) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "ShadowEffectCB", false);
   CurrentCompoObject->FormShadow = value;
   GETSPINBOX("ShadowEffectED")->setEnabled(CurrentCompoObject->FormShadow != 0);
   GETCOMBOBOX("ShadowColorCB")->setEnabled(CurrentCompoObject->FormShadow != 0);
   ApplyToContexte(true);
}

//========= Shape shadow distance
void cShotComposer::s_BlockSettings_ShapeShadowDistanceValue(int value) 
{
   if (!ISBLOCKVALIDEVISIBLE()) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "ShadowEffectED", false);
   CurrentCompoObject->FormShadowDistance = value;
   ApplyToContexte(true);
}

//========= shadow color
void cShotComposer::s_BlockSettings_ShapeShadowColor(int) 
{
   if (!ISBLOCKVALIDEVISIBLE()) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, "ShadowColorCB", false);
   CurrentCompoObject->FormShadowColor = ((cCColorComboBox *)GETCOMBOBOX("ShadowColorCB"))->GetCurrentColor();
   ApplyToContexte(true);
}

//====================================================================================================================
// BLOCK TABLE SETTINGS
//====================================================================================================================

//=========  Refresh the BlockTable
void cShotComposer::RefreshBlockTable(int SetCurrentIndex) 
{
   BlockTable->setUpdatesEnabled(false);
   BlockTable->setRowCount(CompositionList->compositionObjects.count());
   int rowHeight = 48 + 2 + ((ApplicationConfig->TimelineHeight - TIMELINEMINHEIGHT) / 20 + 1) * 3;
   for (int i = 0; i < BlockTable->rowCount(); i++) 
      BlockTable->setRowHeight(i, rowHeight );
   BlockTable->setUpdatesEnabled(true);
   if( SetCurrentIndex >= 0 )
   {
      if (BlockTable->currentRow() != SetCurrentIndex)
      {
         BlockTable->clearSelection();
         BlockTable->setCurrentCell(SetCurrentIndex, 0, QItemSelectionModel::Select | QItemSelectionModel::Current);
      }
      else 
         s_BlockTable_SelectionChanged();
      if (BlockTable->rowCount() == 0) 
         s_BlockTable_SelectionChanged();
   }
   InteractiveZone->RefreshDisplay();
}

//========= block selection change
void cShotComposer::s_BlockTable_SelectionChanged()
{
   if (InSelectionChange)
      return;

   QModelIndexList SelList = BlockTable->selectionModel()->selectedIndexes();

   IsSelected.clear();
   for (int i = 0; i < BlockTable->rowCount(); i++)
      IsSelected.append(false);
   for (int i = 0; i < SelList.count(); i++)
      IsSelected[SelList.at(i).row()] = true;

   NbrSelected = 0;
   CurrentCompoObjectNbr = -1;
   CurrentCompoObject = NULL;

   for (int i = 0; i < IsSelected.count(); i++)
      if (IsSelected[i])
      {
         if (NbrSelected == 0)
         {
            CurrentCompoObjectNbr = i;
            CurrentCompoObject = CompositionList->compositionObjects[CurrentCompoObjectNbr];
         }
         NbrSelected++;
      }
   if (NbrSelected == 0)
      BlockSelectMode = SELECTMODE_NONE;
   else if (NbrSelected == 1) 
      BlockSelectMode = SELECTMODE_ONE;
   else
      BlockSelectMode = SELECTMODE_MULTIPLE;

   RefreshControls(false);
}

//====================================================================================================================

void cShotComposer::s_BlockTable_MoveBlockUp() 
{
   s_BlockTable_SelectionChanged(); // Refresh selection
   if (BlockSelectMode != SELECTMODE_ONE) 
      return;
   if (CurrentCompoObjectNbr < 1) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, BlockTable, true);
   CompositionList->compositionObjects.QT_SWAP(CurrentCompoObjectNbr, CurrentCompoObjectNbr - 1);
   // Reset thumbs if needed
   ResetThumbs(false);
   // Reset blocks table
   RefreshBlockTable(CurrentCompoObjectNbr - 1);
}

//====================================================================================================================

void cShotComposer::s_BlockTable_MoveBlockDown() 
{
   s_BlockTable_SelectionChanged(); // Refresh selection
   if (BlockSelectMode != SELECTMODE_ONE) 
      return;
   if (CurrentCompoObjectNbr >= CompositionList->compositionObjects.count() - 1) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, BlockTable, true);
   CompositionList->compositionObjects.QT_SWAP(CurrentCompoObjectNbr + 1, CurrentCompoObjectNbr);
   // Reset thumbs if needed
   ResetThumbs(false);
   // Reset blocks table
   RefreshBlockTable(CurrentCompoObjectNbr + 1);
}

//====================================================================================================================

void cShotComposer::s_BlockTable_DragMoveBlock(int Src, int Dst) 
{
   if (Src >= CompositionList->compositionObjects.count()) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, BlockTable, true);
   if (Src < Dst) 
      Dst--;
   CompositionList->compositionObjects.insert(Dst, CompositionList->compositionObjects.takeAt(Src));
   // Reset thumbs if needed
   ResetThumbs(false);
   // Reset blocks table
   RefreshBlockTable(Dst);
}

//********************************************************************************************************************
//                                                  BLOCKS ALIGNMENT
//********************************************************************************************************************
void cShotComposer::s_BlockTable_AlignTop()
{
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, true);
   for (int i = 0; i < IsSelected.count(); i++)
      if (IsSelected[i])
      {
         CurrentCompoObject = CompositionList->compositionObjects[i];
         CurrentCompoObject->setY(InteractiveZone->Sel_Y);
         APPLY_GETSET_TONEXT(Y);
         //APPLY1TONEXT(y);    // Apply values of previous shot to all shot for this object
      }
   ApplyToContexte(false);
}

void cShotComposer::s_BlockTable_AlignMiddle()
{
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, true);
   for (int i = 0; i < IsSelected.count(); i++)
      if (IsSelected[i])
      {
         CurrentCompoObject = CompositionList->compositionObjects[i];
         CurrentCompoObject->setY((InteractiveZone->Sel_Y + InteractiveZone->Sel_H / 2) - CurrentCompoObject->height() / 2);
         APPLY_GETSET_TONEXT(Y);
      }
   ApplyToContexte(false);
}

void cShotComposer::s_BlockTable_AlignBottom()
{
   double Ratio_X, Ratio_Y;

   AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, true);
   for (int i = 0; i < IsSelected.count(); i++)
      if (IsSelected[i])
      {
         CurrentCompoObject = CompositionList->compositionObjects[i];
         ComputeBlockRatio(CurrentCompoObject, Ratio_X, Ratio_Y);
         CurrentCompoObject->setY(((InteractiveZone->Sel_Y + InteractiveZone->Sel_H) - CurrentCompoObject->height()/Ratio_Y) );
         APPLY_GETSET_TONEXT(Y);    // Apply values of previous shot to all shot for this object
      }
   ApplyToContexte(false);
}

// to check
void cShotComposer::s_BlockTable_AlignLeft()
{
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, true);
   for (int i = 0; i < IsSelected.count(); i++)
   {
      if (IsSelected[i])
      {
         CurrentCompoObject = CompositionList->compositionObjects[i];
         CurrentCompoObject->setX(InteractiveZone->Sel_X);
         APPLY_GETSET_TONEXT(X);    // Apply values of previous shot to all shot for this object
      }
   }
   ApplyToContexte(false);
}

void cShotComposer::s_BlockTable_AlignCenter()
{
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, true);
   for (int i = 0; i < IsSelected.count(); i++)
      if (IsSelected[i])
      {
         CurrentCompoObject = CompositionList->compositionObjects[i];
         CurrentCompoObject->setX((InteractiveZone->Sel_X + InteractiveZone->Sel_W / 2) - CurrentCompoObject->width() / 2);
         APPLY_GETSET_TONEXT(X);    // Apply values of previous shot to all shot for this object
      }
   ApplyToContexte(false);
}

void cShotComposer::s_BlockTable_AlignRight()
{
   double Ratio_X, Ratio_Y;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, true);
   for (int i = 0; i < IsSelected.count(); i++)
      if (IsSelected[i])
      {
         ComputeBlockRatio(CompositionList->compositionObjects[i], Ratio_X, Ratio_Y);
         CompositionList->compositionObjects[i]->setX((InteractiveZone->Sel_X + InteractiveZone->Sel_W) - CompositionList->compositionObjects[i]->width()/Ratio_X);
         CurrentCompoObject = CompositionList->compositionObjects[i];
         APPLY_GETSET_TONEXT(X);    // Apply values of previous shot to all shot for this object
      }
   ApplyToContexte(false);
}

bool SortBlockLessThan(const SortBlock &s1, const SortBlock &s2)
{
   return s1.Position < s2.Position;
}

void cShotComposer::s_BlockTable_DistributeHoriz()
{
   double Ratio_X, Ratio_Y;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, true);

   // 1st step : compute available space and create list
   QList<SortBlock> List;
   qreal SpaceW = InteractiveZone->Sel_W;
   qreal CurrentX = InteractiveZone->Sel_X;
   for (int i = 0; i < IsSelected.count(); i++)
      if (IsSelected[i])
      {
         ComputeBlockRatio(CompositionList->compositionObjects[i], Ratio_X, Ratio_Y);
         SpaceW = SpaceW - CompositionList->compositionObjects[i]->width()/Ratio_X;
         List.append(SortBlock(i, CompositionList->compositionObjects[i]->posX()));
      }
   SpaceW = SpaceW / qreal(List.count() - 1);

   // 2nd step : sort blocks
   QT_STABLE_SORT(List.begin(), List.end(), SortBlockLessThan);

   // Last step : move blocks
   for (int i = 0; i < List.count(); i++)
   {
      CurrentCompoObject = CompositionList->compositionObjects[List[i].Index];
      ComputeBlockRatio(CurrentCompoObject, Ratio_X, Ratio_Y);
      CurrentCompoObject->setX(CurrentX);
      CurrentX = CurrentX + CurrentCompoObject->width()/Ratio_X + SpaceW;
      APPLY_GETSET_TONEXT(X);    // Apply values of previous shot to all shot for this object
   }
   ApplyToContexte(false);
}

void cShotComposer::s_BlockTable_DistributeVert()
{
   double Ratio_X, Ratio_Y;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, true);

   // 1st step : compute available space and create list
   QList<SortBlock> List;
   qreal SpaceH = InteractiveZone->Sel_H;
   qreal CurrentY = InteractiveZone->Sel_Y;
   for (int i = 0; i < IsSelected.count(); i++)
      if (IsSelected[i])
      {
         ComputeBlockRatio(CompositionList->compositionObjects[i], Ratio_X, Ratio_Y);
         SpaceH = SpaceH - CompositionList->compositionObjects[i]->height()/Ratio_Y;
         List.append(SortBlock(i, CompositionList->compositionObjects[i]->posY()));
      }
   SpaceH = SpaceH / qreal(List.count() - 1);

   // 2nd step : sort blocks
   QT_STABLE_SORT(List.begin(), List.end(), SortBlockLessThan);

   // Last step : move blocks
   for (int i = 0; i < List.count(); i++)
   {
      CurrentCompoObject = CompositionList->compositionObjects[List[i].Index];
      ComputeBlockRatio(CurrentCompoObject, Ratio_X, Ratio_Y);
      CurrentCompoObject->setY(CurrentY);
      CurrentY = CurrentY + CurrentCompoObject->height()/Ratio_Y + SpaceH;
      APPLY_GETSET_TONEXT(Y);    // Apply values of previous shot to all shot for this object
   }
   ApplyToContexte(false);
}

//====================================================================================================================

void cShotComposer::s_BlockTable_RemoveBlock()
{
   if (BlockSelectMode == SELECTMODE_ONE)
   {
      if ((ApplicationConfig->AskUserToRemove) && (CustomMessageBox(this, QMessageBox::Question, QApplication::translate("DlgSlideProperties", "Remove block"), QApplication::translate("DlgSlideProperties", "Are you sure you want to delete this block?"),
         QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No))
         return;
   }
   else if (BlockSelectMode == SELECTMODE_MULTIPLE)
   {
      if ((ApplicationConfig->AskUserToRemove) && (CustomMessageBox(this, QMessageBox::Question, QApplication::translate("DlgSlideProperties", "Remove blocks"), QApplication::translate("DlgSlideProperties", "Are you sure you want to delete these blocks?"),
         QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No))
         return;
   }

   AppendPartialUndo(UNDOACTION_FULL_SLIDE, BlockTable, true);
   for (int i = CompositionList->compositionObjects.count() - 1; i >= 0; i--)
      if (IsSelected[i])
      {
         // Get indexkey of block
         int KeyToDelete = CompositionList->compositionObjects[i]->index();

         // Delete block from all shots of the slide
         for (int j = 0; j < CurrentSlide->shotList.count(); j++)
         {
            int k = 0;
            while (k < CurrentSlide->shotList[j]->ShotComposition.compositionObjects.count())
            {
               if (CurrentSlide->shotList[j]->ShotComposition.compositionObjects[k]->index() == KeyToDelete)
                  delete CurrentSlide->shotList[j]->ShotComposition.compositionObjects.takeAt(k);
               else
                  k++;
            }
         }

         // Delete block from global composition list of the slide
         int k = 0;
         while (k < CurrentSlide->ObjectComposition.compositionObjects.count())
         {
            if (CurrentSlide->ObjectComposition.compositionObjects[k]->index() == KeyToDelete)
               delete CurrentSlide->ObjectComposition.compositionObjects.takeAt(k);
            else
               k++;
         }
      }

   // Reset thumbs if needed
   ResetThumbs(true);
   // Reset blocks table
   RefreshBlockTable(CurrentCompoObjectNbr >= CompositionList->compositionObjects.count() ? CurrentCompoObjectNbr - 1 : CurrentCompoObjectNbr);

   // Ensure nothing is selected
   BlockTable->clearSelection();
}

//====================================================================================================================
//========= Open text editor
void cShotComposer::s_BlockSettings_TextEditor() 
{
   if (!ISBLOCKVALIDEVISIBLE()) 
      return;
   if (!NoPrepUndo) 
      AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, true);
   NoPrepUndo = false;

   InteractiveZone->DisplayMode = cInteractiveZone::DisplayMode_TextMargin;
   InteractiveZone->RefreshDisplay();
   DlgTextEdit Dlg(CurrentSlide->pDiaporama, CurrentCompoObject, ApplicationConfig, &ApplicationConfig->StyleTextCollection, &ApplicationConfig->StyleTextBackgroundCollection, this);
   Dlg.InitDialog();
   connect(&Dlg, SIGNAL(RefreshDisplay()), this, SLOT(s_RefreshSceneImage()));
   if (Dlg.exec() == 0)
   {
      InteractiveZone->DisplayMode = cInteractiveZone::DisplayMode_BlockShape;
      ApplyToContexte(true);
   }
   else
   {
      InteractiveZone->DisplayMode = cInteractiveZone::DisplayMode_BlockShape;
      RemoveLastPartialUndo();
      RefreshControls();
   }
}

//====================================================================================================================
//========= Open Information dialog

void cShotComposer::s_BlockSettings_Information() 
{
   if (!ISBLOCKVALIDEVISIBLE()) 
      return;
   DlgInfoFile Dlg(CurrentCompoObject->BackgroundBrush->MediaObject, ApplicationConfig, this);
   Dlg.InitDialog();
   Dlg.exec();
}

//====================================================================================================================
// Handler for interactive zone
//====================================================================================================================
void cShotComposer::s_BlockSettings_IntZoneTransformBlocks(qreal Move_X, qreal Move_Y, qreal Scale_X, qreal Scale_Y, qreal RSel_X, qreal RSel_Y, qreal RSel_W, qreal RSel_H)
{
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, true);

   for (int i = 0; i < IsSelected.count(); i++)
      if ((IsSelected[i]) && (CompositionList->compositionObjects[i]->isVisible()))
      {
         cCompositionObject* compObj = CompositionList->compositionObjects[i];
         qreal RatioScale_X = (RSel_W + Scale_X) / RSel_W;
         qreal RatioScale_Y = (RSel_H + Scale_Y) / RSel_H;

         compObj->setX(RSel_X + Move_X + (compObj->posX() - RSel_X)*RatioScale_X);
         compObj->setY(RSel_Y + Move_Y + (compObj->posY() - RSel_Y)*RatioScale_Y);
         compObj->setWidth(compObj->width()*RatioScale_X);
         if (compObj->width() < 0.002)
            compObj->setWidth(0.002);
         if (compObj->BackgroundBrush->LockGeometry())
            compObj->setHeight(((compObj->width()*InteractiveZone->DisplayW)*compObj->BackgroundBrush->AspectRatio()) / InteractiveZone->DisplayH);
         else
            compObj->setHeight(compObj->height()*RatioScale_Y);
         if (compObj->height() < 0.002)
            compObj->setHeight(0.002);
         CurrentCompoObject = CompositionList->compositionObjects[i];
         APPLY_GETSET_TONEXT(Rect);
      }
   // Apply values of previous shot to all shot for this object
   //APPLY4TONEXT(x,y,w,h);
   //APPLY_GETSET_TONEXT(Rect);
   ApplyToContexte(false);
}

void cShotComposer::s_BlockSettings_IntZoneDisplayTransformBlocks(qreal Move_X, qreal Move_Y, qreal Scale_X, qreal Scale_Y, qreal RSel_X, qreal RSel_Y, qreal RSel_W, qreal RSel_H) 
{
   InRefreshControls = true;

   int i = CurrentCompoObjectNbr;
   cCompositionObject* compObj = CompositionList->compositionObjects[i];
   qreal RatioScale_X = (RSel_W + Scale_X) / RSel_W;
   qreal RatioScale_Y = (RSel_H + Scale_Y) / RSel_H;

   qreal   x = RSel_X + Move_X + (compObj->posX() - RSel_X)*RatioScale_X;
   qreal   y = RSel_Y + Move_Y + (compObj->posY() - RSel_Y)*RatioScale_Y;
   qreal   w = compObj->width() * RatioScale_X; 
   if (w < 0.002) 
      w = 0.002;
   qreal   h = (compObj->BackgroundBrush->LockGeometry() ? ((w*InteractiveZone->DisplayW)*compObj->BackgroundBrush->AspectRatio()) / InteractiveZone->DisplayH : compObj->height()*RatioScale_Y); 
   if (h < 0.002) 
      h = 0.002;

   qreal Ratio_X;
   qreal Ratio_Y;
   ComputeBlockRatio(compObj, Ratio_X, Ratio_Y);
   if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
   {
      GETDOUBLESPINBOX("PosXEd")->setValue(x * 100 /*/ Ratio_X*/);
      GETDOUBLESPINBOX("PosYEd")->setValue(y * 100 /*/ Ratio_Y*/);
      GETDOUBLESPINBOX("WidthEd")->setValue(w * 100 / Ratio_X);
      GETDOUBLESPINBOX("HeightEd")->setValue(h * 100 / Ratio_Y);
   }
   else
   { // DisplayUnit==DISPLAYUNIT_PIXELS
      GETDOUBLESPINBOX("PosXEd")->setValue(x*InteractiveZone->DisplayW /*/ Ratio_X*/);
      GETDOUBLESPINBOX("PosYEd")->setValue(y*InteractiveZone->DisplayH /*/ Ratio_Y*/);
      GETDOUBLESPINBOX("WidthEd")->setValue(w*InteractiveZone->DisplayW / Ratio_X);
      GETDOUBLESPINBOX("HeightEd")->setValue(h*InteractiveZone->DisplayH / Ratio_Y);
   }
   InRefreshControls = false;
}

void cShotComposer::s_BlockSettings_TransformBlocks(qreal DeltaX, qreal DeltaY, qreal DeltaW, qreal DeltaH, QRectF selRect)
{
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, InteractiveZone, true);

   qreal RatioScale_X = (selRect.width() + DeltaW) / selRect.width();
   qreal RatioScale_Y = (selRect.height() + DeltaH) / selRect.height();
   for (int i = 0; i < IsSelected.count(); i++)
   {
      if ((IsSelected[i]) && (CompositionList->compositionObjects[i]->isVisible()))
      {
         cCompositionObject* compObj = CompositionList->compositionObjects[i];
         WSRect prevRect = compObj->getRect();
         WSRect xRect;
         xRect.setX(selRect.x() + DeltaX + (compObj->posX() - selRect.x())*RatioScale_X);
         xRect.setY(selRect.y() + DeltaY + (compObj->posY() - selRect.y())*RatioScale_Y);
         xRect.setHeight(compObj->height()*RatioScale_Y);
         xRect.setWidth(compObj->width()*RatioScale_X);
         if (xRect.width() < 0.002)
            xRect.setWidth(0.002);
         if (compObj->BackgroundBrush->LockGeometry())
            xRect.setHeight(((xRect.width()*InteractiveZone->DisplayW)*compObj->BackgroundBrush->AspectRatio()) / InteractiveZone->DisplayH);
         else
            xRect.setHeight(xRect.height()*RatioScale_Y);
         if (xRect.height() < 0.002)
            xRect.setHeight(0.002);

         if( DeltaW )
         {
            qreal xDiff = prevRect.width() - xRect.width();
            xRect.setX(prevRect.x() + xDiff/2);
         }
         if( DeltaH )
         {
            qreal yDiff = prevRect.height() - xRect.height();
            xRect.setY(prevRect.y() + yDiff/2);
         }

         compObj->setRect( xRect );
         CurrentCompoObject = compObj;
         APPLY_GETSET_TONEXT(Rect);
      }
   }
   ApplyToContexte(false);
}

void cShotComposer::s_BlockSettings_RotateBlocks(qreal xAngle, qreal yAngle, qreal zAngle)
{
   if( xAngle != 0.0 )
      AppendPartialUndo(UNDOACTION_FULL_SLIDE, "RotateXED", false);
   if( yAngle != 0.0 )
      AppendPartialUndo(UNDOACTION_FULL_SLIDE, "RotateYED", false);
   if( zAngle != 0.0 )
      AppendPartialUndo(UNDOACTION_FULL_SLIDE, "RotateZED", false);

   for (int i = 0; i < IsSelected.count(); i++)
   {
      if ((IsSelected[i]) && (CompositionList->compositionObjects[i]->isVisible()))
      {
         cCompositionObject* compObj = CompositionList->compositionObjects[i];
         qreal angle;
         if( xAngle != 0.0 )
         {
            angle = compObj->xAngle() + xAngle;
            if( angle <= 360 && angle >= -360 )
               compObj->setXAngle( angle );
         }
         if( yAngle )
         {
            angle = compObj->yAngle() + yAngle;
            if( angle <= 360 && angle >= -360 )
               compObj->setYAngle( angle);
         }
         if( zAngle )
         {
            angle = compObj->zAngle() + zAngle;
            if( angle <= 360 && angle >= -360 )
               compObj->setZAngle( angle);
         }
         CurrentCompoObject = compObj;
         APPLY_GETSETA_TONEXT(xAngle,setXAngle);
         APPLY_GETSETA_TONEXT(yAngle,setYAngle);
         APPLY_GETSETA_TONEXT(zAngle,setZAngle);
      }
   }
   ApplyToContexte(false);
}

void cShotComposer::s_ZoomSlide(qreal zoomDiff, qreal slideX, qreal slideY)
{
   cBrushDefinition SavedBrush(CurrentCompoObject, ApplicationConfig);

   for (int i = 0; i < IsSelected.count(); i++)
   {
      if ((IsSelected[i]) && (CompositionList->compositionObjects[i]->isVisible()))
      {
         cBrushDefinition *cBrush = CompositionList->compositionObjects[i]->BackgroundBrush;
         SavedBrush.CopyFromBrushDefinition(cBrush/*CurrentCompoObject->BackgroundBrush*/);
         double curZoomFactor = cBrush->ZoomFactor();
         if( curZoomFactor+zoomDiff > 2.0 )
            zoomDiff = 2.0 - curZoomFactor;
         else if( curZoomFactor+zoomDiff < .01 )
            zoomDiff = .01 - curZoomFactor;            
         if( zoomDiff != 0.0 )
         {
            cBrush->setZoomFactor(cBrush->ZoomFactor()+zoomDiff); 
            cBrush->setX(cBrush->X()-zoomDiff/2.0); 
            cBrush->setY(cBrush->Y()-(zoomDiff*cBrush->AspectRatio())/2.0); 
         }
         if( slideX != 0.0 )
            cBrush->setX(cBrush->X()+slideX); 
         if( slideY != 0.0 )
            cBrush->setY(cBrush->Y()+slideY); 
         // Apply values of previous shot to all shot for this object
         APPLYBACKGROUNDBRUSH_POS();
         ApplyToContexte(true);
      }
   }
}

void cShotComposer::setMaskingEnabled(bool bEnable)
{
   GETUI("cmUseMask")->setEnabled(bEnable);
   if( bEnable )
      bEnable = GETCHECKBOX("cmUseMask")->isChecked();
   GETUI("cmInvertMask")->setEnabled(bEnable);
   GETUI("lbMaskPosX")->setEnabled(bEnable);
   GETUI("MaskPosX")->setEnabled(bEnable);
   GETUI("lbMaskPosY")->setEnabled(bEnable);
   GETUI("MaskPosY")->setEnabled(bEnable);
   GETUI("lbMaskWidth")->setEnabled(bEnable);
   GETUI("MaskWidth")->setEnabled(bEnable);
   GETUI("lbMaskHeight")->setEnabled(bEnable);
   GETUI("MaskHeight")->setEnabled(bEnable);
   GETUI("lbMaskRotate")->setEnabled(bEnable);
   GETUI("RotateZMaskSLD")->setEnabled(bEnable);
   GETUI("RotateMaskZED")->setEnabled(bEnable);
   GETUI("ResetRotateMask")->setEnabled(bEnable);
}

void cShotComposer::on_cmUseMask_clicked()
{
   if( !CurrentCompoObject )
      return;
   setMaskingEnabled(true);
   if( GETCHECKBOX("cmUseMask")->isChecked() ) 
      CurrentCompoObject->setUseMask(true);
   else
      CurrentCompoObject->setUseMask(false);
   APPLY_GETSETA_TONEXT(usingMask,setUseMask);
   RefreshControls();
}

void cShotComposer::on_cmInvertMask_clicked()
{
   if( !CurrentCompoObject )
      return;
   if( GETCHECKBOX("cmInvertMask")->isChecked() ) 
      CurrentCompoObject->getMask()->setInvertMask(true);
   else
      CurrentCompoObject->getMask()->setInvertMask(false);
   //APPLY_GETSETA_TONEXT(usingMask,setUseMask);
   RefreshControls();
}

void cShotComposer::on_MaskPosX_valueChanged(double d)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   if( !CurrentCompoObject || !CurrentCompoObject->getMask() )
      return;
   cCompositionObjectMask *mask =  CurrentCompoObject->getMask();
   double newValue;
   if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
      newValue = d/100.0;
   else
   {
      double srcwidth = GETDOUBLESPINBOX("WidthEd")->value();
      newValue = d/srcwidth;
   }
   if( mask->getX() != newValue )
   {
      mask->setX(newValue);
      APPLY_GETSETA_TONEXT_SUBOBJ(getMask,getX,setX);
   }
   RefreshControls();
}

void cShotComposer::on_MaskPosY_valueChanged(double d)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   if( !CurrentCompoObject || !CurrentCompoObject->getMask() )
      return;
   cCompositionObjectMask *mask =  CurrentCompoObject->getMask();
   double newValue = mask->getY();
   if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
      mask->setY(d/100.0);
   else
   {
      double srcheight = GETDOUBLESPINBOX("HeightEd")->value();
      newValue = (d / srcheight);
   }
   if( mask->getY() != newValue )
   {
      mask->setY(newValue);
      APPLY_GETSETA_TONEXT_SUBOBJ(getMask,getY,setY);
   }
   RefreshControls();
}

void cShotComposer::on_MaskWidth_valueChanged(double d)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   if( !CurrentCompoObject || !CurrentCompoObject->getMask() )
      return;
   cCompositionObjectMask *mask =  CurrentCompoObject->getMask();
   double newValue;
   if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
      newValue = d/100.0;
   else
   {
      double srcwidth = GETDOUBLESPINBOX("WidthEd")->value();
      newValue = (d / srcwidth);
   }
   if( mask->width() != newValue )
   {
      mask->setWidth(newValue);
      APPLY_GETSETA_TONEXT_SUBOBJ(getMask,width,setWidth);
   }
   RefreshControls();
}

void cShotComposer::on_MaskHeight_valueChanged(double d)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   if( !CurrentCompoObject || !CurrentCompoObject->getMask() )
      return;
   cCompositionObjectMask *mask =  CurrentCompoObject->getMask();
   double newValue;
   if (ApplicationConfig->DisplayUnit == DISPLAYUNIT_PERCENT)
      newValue = d/100.0;
   else
   {
      double srcheight = GETDOUBLESPINBOX("HeightEd")->value();
      newValue = (d / srcheight);
   }
   if( mask->height() != newValue)
   {
      mask->setHeight(newValue);
      APPLY_GETSETA_TONEXT_SUBOBJ(getMask,height,setHeight);
   }
   RefreshControls();
}

void cShotComposer::on_RotateMaskZED_valueChanged(int value)
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   if( !CurrentCompoObject || !CurrentCompoObject->getMask() )
      return;
   cCompositionObjectMask *mask =  CurrentCompoObject->getMask();
   if( mask->zAngle() != value )
   {
      mask->setZAngle(value);
      APPLY_GETSETA_TONEXT_SUBOBJ(getMask,zAngle,setZAngle);
   }
   RefreshControls();
}

void cShotComposer::on_cm3DFrame_clicked()
{
   if( !CurrentCompoObject )
      return;
   CurrentCompoObject->setUse3DFrame( GETCHECKBOX("cm3DFrame")->isChecked() );
   APPLY_GETSETA_TONEXT(using3DFrame,setUse3DFrame);
   ApplyToContexte(true);
   RefreshControls(true);
}

