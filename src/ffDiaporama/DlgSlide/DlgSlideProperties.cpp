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

#include "cCustomShotTable.h"
#include "cCustomBlockTable.h"
#include "cInteractiveZone.h"

#include "DlgSlideProperties.h"
#include "ui_DlgSlideProperties.h"

#include "cTextFrame.h"
#include "cTexteFrameComboBox.h"

#include "DlgImage/DlgImageCorrection.h"
#include "DlgFileExplorer/DlgFileExplorer.h"
#include "DlgRuler/DlgRulerDef.h"
#include "DlgChapter/DlgChapter.h"

#include <QClipboard>
#include <QMimeData>
#include <QMessageBox>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QMenu>
#include <QButtonGroup>

#include <QQuickView> 
#include <QQuickWidget> 
#include <QQuickStyle>
#include <QScrollBar>

#define ICON_FRAMING_CUSTOM                 ":/img/action_cancel.png"
#define ICON_FRAMING_FULL                   ":/img/AdjustWH.png"
#define ICON_FRAMING_WIDTH                  ":/img/AdjustW.png"
#define ICON_FRAMING_HEIGHT                 ":/img/AdjustH.png"
#define ICON_GLOBALCONF                     ":/img/db.png"
#define ICON_USERCONF                       ":/img/db_update.png"

#define ICON_VISIBLE_OK                     ":/img/Visible_OK.png"
#define ICON_VISIBLE_KO                     ":/img/Visible_KO.png"
#define ICON_SOUND_OK                       ":/img/object_sound.png"
#define ICON_SOUND_KO                       ":/img/sound_KO.png"
#define ICON_OBJECT_TEXT                    ":/img/object_text.png"
#define ICON_OBJECT_SOUND                   ":/img/object_sound.png"
#define ICON_OBJECT_MOVIE                   ":/img/object_movie.png"
#define ICON_OBJECT_IMAGE                   ":/img/object_image.png"
#define ICON_OBJECT_TEXTHIDE                ":/img/object_textHide.png"
#define ICON_OBJECT_MOVIEHIDE               ":/img/object_movieHide.png"
#define ICON_OBJECT_IMAGEHIDE               ":/img/object_imageHide.png"

//********************************************************************************************************************************
// DlgSlideProperties : Slide Dialog
//********************************************************************************************************************************

DlgSlideProperties::DlgSlideProperties(cDiaporamaObject *DiaporamaObject,cApplicationConfig *ApplicationConfig,QWidget *parent):cShotComposer(DiaporamaObject,ApplicationConfig,parent),ui(new Ui::DlgSlideProperties) 
{
    ui->setupUi(this);
    Splitter                        = ui->SplitterTop;
    OkBt                            = ui->OKBT;
    CancelBt                        = ui->CancelBt;
    HelpBt                          = ui->HelpBt;
    HelpFile                        = "0119";
    UndoBt                          = ui->UndoBT;
    InteractiveZone                 = ui->InteractiveZone;
    BlockTable                      = ui->BlockTable;
    ShotTable                       = ui->ShotTable;
    InteractiveZone->MagneticRuler  = ApplicationConfig->SlideRuler;
    InteractiveZone->DisplayW       = DisplayW;
    InteractiveZone->DisplayH       = DisplayH;
    RedoneStyleCoordinate           = false;         // specific flag for GMapsObject
    CurrentShot                     = NULL;
    FramingCB_CurrentBrush          = NULL;
    FramingCB_CurrentShot           = -1;
    actionAddImageClipboard         = ui->actionAddImageClipboard;
    actionPaste                     = ui->actionPaste;
    DlgWorkingTaskDialog            = NULL;
}

//====================================================================================================================

DlgSlideProperties::~DlgSlideProperties() 
{
   InRefreshControls=true; // To ensure no controls will change during delete
   if (DlgWorkingTaskDialog) 
   {
      DlgWorkingTaskDialog->close();
      delete DlgWorkingTaskDialog;
      DlgWorkingTaskDialog = NULL;
   }
   delete wheelButtons;
   delete ui;
   CurrentSlide->pDiaporama->CloseUnusedLibAv(CurrentSlide->pDiaporama->CurrentCol);
}

//====================================================================================================================
// Initialise dialog

void DlgSlideProperties::DoInitDialog() 
{
   cShotComposer::DoInitDialog();

   ////QQuickView* qmlView = new QQuickView();
   ////QWidget* container = QWidget::createWindowContainer(qmlView, this);
   ////qmlView->setSource(QUrl("qrc:///mapviewer.qml"));
   ////qDebug() << QQuickStyle::availableStyles();
   ////QQuickStyle::setStyle("Universal");
   //QQuickWidget* view = new QQuickWidget;
   //view->setSource(QUrl::fromLocalFile("myqmlfile.qml"));
   //view->setResizeMode(QQuickWidget::SizeRootObjectToView);
   //view->show();
   //QGridLayout* qgl = findChild<QGridLayout*>("gridLayout");
   //qgl->removeItem(ui->myVerticalSpacer);

   //qgl->addWidget(view, qgl->rowCount(), 0, 1, 5);
   //qgl->addWidget(new QLineEdit(), qgl->rowCount(), 1, 1, 2, Qt::AlignTop);
   //qgl->addWidget(new QCheckBox(), qgl->rowCount(), 1, 1, 2, Qt::AlignTop);
   //qgl->addItem(ui->myVerticalSpacer, qgl->rowCount(), 1, 1, 1);
   //view->show();

   ui->TextClipArtCB->PrepareTable();

   setWindowTitle(windowTitle()+" - "+QApplication::translate("DlgSlideProperties","Slide")+QString(" %1/%2").arg(CurrentSlide->pDiaporama->CurrentCol+1).arg(CurrentSlide->pDiaporama->slides.count()));
   ui->OKPreviousBT->setEnabled(CurrentSlide->pDiaporama->CurrentCol>0);
   ui->OKNextBT->setEnabled(CurrentSlide->pDiaporama->CurrentCol<CurrentSlide->pDiaporama->slides.count()-1);
   ui->RulersBT->setIcon(QIcon(QString(InteractiveZone->MagneticRuler!=0?ICON_RULER_ON:ICON_RULER_OFF)));

   BlockTable->ProjectGeometry=ProjectGeometry;

   //******************************

   CompositionList         = NULL;

   // Re-entrence flags
   InRefreshStyleControls = false;
   StopMajFramingStyle    = false;
   InDisplayDuration      = false;

   ui->SlideNameED->setText(CurrentSlide->SlideName);

   if (ui->TextFramingStyleCB->view()->width()<500)    ui->TextFramingStyleCB->view()->setFixedWidth(500);

   ui->ShotDurationED->setCurrentSection(QDateTimeEdit::SecondSection);
   ui->ShotDurationED->setCurrentSectionIndex(2);
   ui->ShotDurationED->MsecStep=(ApplicationConfig->DefaultStandard==STANDARD_PAL?qreal(1000)/qreal(25):qreal(1001)/qreal(30));

   // Speed wave
   ui->SpeedWaveCB->AddProjectDefault(CurrentSlide->pDiaporama->BlockAnimSpeedWave);

   // Init block animation type
   ui->BlockAnimCB->addItem(QApplication::translate("DlgSlideProperties","None"));
   ui->BlockAnimCB->addItem(QApplication::translate("DlgSlideProperties","Multiple block turn"));
   ui->BlockAnimCB->addItem(QApplication::translate("DlgSlideProperties","Dissolve"));

   // Init Dissolve animation value
   ui->DissolveCB->addItem(QApplication::translate("DlgSlideProperties","Appear"));
   ui->DissolveCB->addItem(QApplication::translate("DlgSlideProperties","Disappear"));
   ui->DissolveCB->addItem(QApplication::translate("DlgSlideProperties","Blink at slow speed"));
   ui->DissolveCB->addItem(QApplication::translate("DlgSlideProperties","Blink at medium speed"));
   ui->DissolveCB->addItem(QApplication::translate("DlgSlideProperties","Blink at fast speed"));
   ui->DissolveCB->addItem(QApplication::translate("DlgSlideProperties","Blink at very fast speed"));

   // Force icon in contextual menu
   ui->actionAddTextBlock->setIconVisibleInMenu(true);
   ui->actionAddFile->setIconVisibleInMenu(true);
   ui->actionEditText->setIconVisibleInMenu(true);
   ui->actionEditImage->setIconVisibleInMenu(true);
   ui->actionUpBlock->setIconVisibleInMenu(true);
   ui->actionDownBlock->setIconVisibleInMenu(true);
   ui->actionSetVisible->setIconVisibleInMenu(true);
   ui->actionSetHide->setIconVisibleInMenu(true);
   ui->actionTakeSound->setIconVisibleInMenu(true);
   ui->actionCopy->setIconVisibleInMenu(true);
   ui->actionCut->setIconVisibleInMenu(true);
   ui->actionPaste->setIconVisibleInMenu(true);
   ui->actionRemoveBlock->setIconVisibleInMenu(true);
   ui->actionInfo->setIconVisibleInMenu(true);
   ui->actionTop->setIconVisibleInMenu(true);
   ui->actionMiddle->setIconVisibleInMenu(true);
   ui->actionBottom->setIconVisibleInMenu(true);
   ui->actionLeft->setIconVisibleInMenu(true);
   ui->actionCenter->setIconVisibleInMenu(true);
   ui->actionRight->setIconVisibleInMenu(true);
   ui->actionAddShot->setIconVisibleInMenu(true);
   ui->actionRemoveShot->setIconVisibleInMenu(true);
   ui->actionMoveLeft->setIconVisibleInMenu(true);
   ui->actionMoveRight->setIconVisibleInMenu(true);
   ui->actionDistributeHoriz->setIconVisibleInMenu(true);
   ui->actionDistributeVert->setIconVisibleInMenu(true);
   ui->actionAddSimpleTextBlock->setIconVisibleInMenu(true);
   ui->actionAddClipArtTextBlock->setIconVisibleInMenu(true);
   ui->actionSaveAsProjectTitleModel->setIconVisibleInMenu(true);
   ui->actionSaveAsCptTitleModels->setIconVisibleInMenu(true);
   ui->actionSaveAsCreditTitleModels->setIconVisibleInMenu(true);
   ui->actionAddGMapsMap->setIconVisibleInMenu(true);
   ui->actionAddImageClipboard->setIconVisibleInMenu(true);

   ShotTable->setRowCount(1);

   // Define handler
   connect(ui->OKPreviousBT,SIGNAL(clicked()),this,SLOT(OKPrevious()));
   connect(ui->OKNextBT,SIGNAL(clicked()),this,SLOT(OKNext()));
   connect(ui->RulersBT,SIGNAL(clicked()),this,SLOT(s_RulersBt()));
   connect(ui->SaveModelBT,SIGNAL(pressed()),this,SLOT(s_SlideSet_SaveModel()));
   connect(ui->actionSaveAsProjectTitleModel,SIGNAL(triggered()),this,SLOT(s_SlideSet_SaveAsProjectTitleModel()));
   connect(ui->actionSaveAsCptTitleModels,SIGNAL(triggered()),this,SLOT(s_SlideSet_SaveAsCptTitleModels()));
   connect(ui->actionSaveAsCreditTitleModels,SIGNAL(triggered()),this,SLOT(s_SlideSet_SaveAsCreditTitleModels()));

   connect(ui->actionCopy,SIGNAL(triggered()),this,SLOT(s_BlockTable_Copy()));
   connect(ui->actionCut,SIGNAL(triggered()),this,SLOT(s_BlockTable_Cut()));
   connect(ui->actionPaste,SIGNAL(triggered()),this,SLOT(s_BlockTable_Paste()));

   connect(ui->actionSetVisible,SIGNAL(triggered()),this,SLOT(s_BlockSettings_ToggleVisibleState()));
   connect(ui->actionSetHide,SIGNAL(triggered()),this,SLOT(s_BlockSettings_ToggleVisibleState()));
   connect(ui->actionTakeSound,SIGNAL(triggered()),this,SLOT(s_BlockSettings_GetSound()));

   connect(ui->actionUpBlock,SIGNAL(triggered()),this,SLOT(s_BlockTable_MoveBlockUp()));
   connect(ui->actionDownBlock,SIGNAL(triggered()),this,SLOT(s_BlockTable_MoveBlockDown()));

   connect(ui->actionTop,SIGNAL(triggered()),this,SLOT(s_BlockTable_AlignTop()));
   connect(ui->actionMiddle,SIGNAL(triggered()),this,SLOT(s_BlockTable_AlignMiddle()));
   connect(ui->actionBottom,SIGNAL(triggered()),this,SLOT(s_BlockTable_AlignBottom()));
   connect(ui->actionLeft,SIGNAL(triggered()),this,SLOT(s_BlockTable_AlignLeft()));
   connect(ui->actionCenter,SIGNAL(triggered()),this,SLOT(s_BlockTable_AlignCenter()));
   connect(ui->actionRight,SIGNAL(triggered()),this,SLOT(s_BlockTable_AlignRight()));
   connect(ui->actionDistributeHoriz,SIGNAL(triggered()),this,SLOT(s_BlockTable_DistributeHoriz()));
   connect(ui->actionDistributeVert,SIGNAL(triggered()),this,SLOT(s_BlockTable_DistributeVert()));

   connect(ui->SlideNameED,SIGNAL(textEdited(QString)),this,SLOT(s_SlideSet_SlideNameChange(QString)));
   connect(ui->ChapterBT,SIGNAL(clicked()),this,SLOT(s_SlideSet_ChapterInformation()));
   connect(ui->ShotDurationED,SIGNAL(timeChanged(QTime)),this,SLOT(s_ShotTable_DurationChange(QTime)));

   connect(ui->ArrangeBT,SIGNAL(pressed()),this,SLOT(s_BlockSettings_Arrange()));
   connect(ui->EditBT,SIGNAL(pressed()),this,SLOT(s_BlockSettings_Edit()));
   connect(ui->actionEditText,SIGNAL(triggered()),this,SLOT(s_BlockSettings_TextEditor()));
   connect(ui->actionEditImage,SIGNAL(triggered()),this,SLOT(s_BlockSettings_ImageEditCorrect()));
   connect(ui->InfoBlockBT,SIGNAL(clicked()),this,SLOT(s_BlockSettings_Information()));
   connect(ui->actionInfo,SIGNAL(triggered()),this,SLOT(s_BlockSettings_Information()));

   connect(ShotTable,SIGNAL(itemSelectionChanged()),this,SLOT(s_ShotTable_SelectionChanged()));
   connect(ShotTable,SIGNAL(RightClickEvent(QMouseEvent *)),this,SLOT(s_ShotTable_RightClickEvent(QMouseEvent *)));
   connect(ShotTable,SIGNAL(DragMoveItem()),this,SLOT(s_ShotTable_DragMoveItem()));
   connect(ui->AddShot,SIGNAL(pressed()),this,SLOT(s_ShotTable_AddShot()));
   connect(ui->actionAddShot,SIGNAL(triggered()),this,SLOT(s_ShotTable_AddShot()));
   connect(ui->RemoveShot,SIGNAL(pressed()),this,SLOT(s_ShotTable_RemoveShot()));
   connect(ui->actionRemoveShot,SIGNAL(triggered()),this,SLOT(s_ShotTable_RemoveShot()));
   connect(ui->actionMoveLeft,SIGNAL(triggered()),this,SLOT(s_ShotTable_MoveLeft()));
   connect(ui->actionMoveRight,SIGNAL(triggered()),this,SLOT(s_ShotTable_MoveRight()));

   connect(ui->SpeedWaveCB,SIGNAL(currentIndexChanged(int)),this,SLOT(s_BlockSettings_SpeedWave(int)));

   // Block animation
   connect(ui->BlockAnimCB,SIGNAL(currentIndexChanged(int)),this,SLOT(s_BlockSettings_BlockAnimType(int)));
   connect(ui->TurnZED,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_BlockAnimTurnZValue(int)));         connect(ui->TurnZSlider,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_BlockAnimTurnZValue(int)));         connect(ui->ResetTurnZBT,SIGNAL(released()),this,SLOT(s_BlockSettings_BlockAnimTurnZReset()));
   connect(ui->TurnXED,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_BlockAnimTurnXValue(int)));         connect(ui->TurnXSlider,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_BlockAnimTurnXValue(int)));         connect(ui->ResetTurnXBT,SIGNAL(released()),this,SLOT(s_BlockSettings_BlockAnimTurnXReset()));
   connect(ui->TurnYED,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_BlockAnimTurnYValue(int)));         connect(ui->TurnYSlider,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_BlockAnimTurnYValue(int)));         connect(ui->ResetTurnYBT,SIGNAL(released()),this,SLOT(s_BlockSettings_BlockAnimTurnYReset()));
   connect(ui->DissolveCB,SIGNAL(currentIndexChanged(int)),this,SLOT(s_BlockSettings_BlockAnimDissolveType(int)));

   // Block table/scene part
   connect(BlockTable,SIGNAL(itemSelectionChanged()),this,SLOT(s_BlockTable_SelectionChanged()));
   connect(BlockTable,SIGNAL(DoubleClickEvent(QMouseEvent *)),this,SLOT(s_BlockTable_ItemDoubleClicked(QMouseEvent *)));
   connect(BlockTable,SIGNAL(RightClickEvent(QMouseEvent *)),this,SLOT(s_BlockTable_ItemRightClicked(QMouseEvent *)));
   connect(BlockTable,SIGNAL(DragMoveBlock(int,int)),this,SLOT(s_BlockTable_DragMoveBlock(int,int)));
   connect(BlockTable,SIGNAL(DragDropFiles(QList<QUrl>)),this,SLOT(s_BlockTable_DragDropFiles(QList<QUrl>)));
   connect(ui->AddTextBlock,SIGNAL(pressed()),this,SLOT(s_BlockTable_AddNewTextBlock()));
   connect(ui->actionAddTextBlock,SIGNAL(triggered()),this,SLOT(s_BlockTable_AddNewTextBlock()));
   connect(ui->actionAddSimpleTextBlock,SIGNAL(triggered()),this,SLOT(s_BlockTable_AddNewSimpleTextBlock()));
   connect(ui->actionAddClipArtTextBlock,SIGNAL(triggered()),this,SLOT(s_BlockTable_AddNewClipArtTextBlock()));
   connect(ui->AddFileBlock,SIGNAL(pressed()),this,SLOT(s_BlockTable_AddNewFileBlock()));
   connect(ui->actionAddFile,SIGNAL(triggered()),this,SLOT(s_BlockTable_AddNewFileBlock()));
   connect(ui->actionRemoveBlock,SIGNAL(triggered()),this,SLOT(s_BlockTable_RemoveBlock()));

   connect(ui->AddSpecialBlock,SIGNAL(pressed()),this,SLOT(s_BlockTable_AddSpecialBlock()));
   connect(ui->actionAddImageClipboard,SIGNAL(triggered()),this,SLOT(s_BlockTable_AddImageClipboard()));
   connect(ui->actionAddGMapsMap,SIGNAL(triggered()),this,SLOT(s_BlockTable_AddGMapsMapBlock()));

   // Style buttons
   connect(ui->FramingStyleCB,SIGNAL(itemSelectionHaveChanged()),this,SLOT(s_BlockSettings_ChangeFramingStyle()));
   connect(ui->TextFramingStyleCB,SIGNAL(currentIndexChanged(int)),this,SLOT(s_BlockSettings_ChangeTextFramingStyle(int)));

   // Text annimation
   connect(ui->ZoomSlider,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_TextAnimZoom(int)));
   connect(ui->ZoomED,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_TextAnimZoom(int)));
   connect(ui->ZoomResetBT,SIGNAL(released()),this,SLOT(s_BlockSettings_TextAnimZoomReset()));
   connect(ui->ScrollXSlider,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_TextAnimScrollX(int)));
   connect(ui->ScrollXED,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_TextAnimScrollX(int)));
   connect(ui->ScrollXResetBT,SIGNAL(released()),this,SLOT(s_BlockSettings_TextAnimScrollXReset()));
   connect(ui->ScrollYSlider,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_TextAnimScrollY(int)));
   connect(ui->ScrollYED,SIGNAL(valueChanged(int)),this,SLOT(s_BlockSettings_TextAnimScrollY(int)));
   connect(ui->ScrollYResetBT,SIGNAL(released()),this,SLOT(s_BlockSettings_TextAnimScrollYReset()));

   // Interactive zone
   connect(InteractiveZone,SIGNAL(StartSelectionChange()),this,SLOT(s_BlockTable_StartSelectionChange()));
   connect(InteractiveZone,SIGNAL(EndSelectionChange()),this,SLOT(s_BlockTable_EndSelectionChange()));
   connect(InteractiveZone,SIGNAL(RightClickEvent(QMouseEvent *)),this,SLOT(s_BlockTable_ItemRightClicked(QMouseEvent *)));
   connect(InteractiveZone,SIGNAL(DoubleClickEvent(QMouseEvent *)),this,SLOT(s_BlockTable_ItemDoubleClicked(QMouseEvent *)));
   connect(InteractiveZone,SIGNAL(TransformBlock(qreal,qreal,qreal,qreal,qreal,qreal,qreal,qreal)),this,       SLOT(s_BlockSettings_IntZoneTransformBlocks(qreal,qreal,qreal,qreal,qreal,qreal,qreal,qreal)));
   connect(InteractiveZone,SIGNAL(DisplayTransformBlock(qreal,qreal,qreal,qreal,qreal,qreal,qreal,qreal)),this,SLOT(s_BlockSettings_IntZoneDisplayTransformBlocks(qreal,qreal,qreal,qreal,qreal,qreal,qreal,qreal)));

   connect(InteractiveZone,SIGNAL(TransformBlocks(qreal,qreal,qreal,qreal,QRectF)),this,SLOT(s_BlockSettings_TransformBlocks(qreal,qreal,qreal,qreal,QRectF)));
   connect(InteractiveZone,SIGNAL(RotateBlocks(qreal,qreal,qreal)),this,SLOT(s_BlockSettings_RotateBlocks(qreal,qreal,qreal)));
   connect(InteractiveZone,SIGNAL(ZoomSlide(qreal, qreal, qreal)),this,SLOT(s_ZoomSlide(qreal, qreal, qreal)));

   wheelMode = cInteractiveZone::eMoveHor;
   wheelButtons = new QButtonGroup(this);
   wheelButtons->addButton(ui->tbWM_MoveHor, cInteractiveZone::eMoveHor);
   wheelButtons->addButton(ui->tbWM_MoveVer, cInteractiveZone::eMoveVer);
   wheelButtons->addButton(ui->tbWM_StretchHor, cInteractiveZone::eResize);
   wheelButtons->addButton(ui->tbWM_RotateX, cInteractiveZone::eRotateX);
   wheelButtons->addButton(ui->tbWM_RotateY, cInteractiveZone::eRotateY);
   wheelButtons->addButton(ui->tbWM_RotateZ, cInteractiveZone::eRotateZ);
   wheelButtons->addButton(ui->tbWM_Zoom, cInteractiveZone::eZoomInOut);
   wheelButtons->addButton(ui->tbWM_SlideHorizontal, cInteractiveZone::eSlideHorizontal);
   wheelButtons->addButton(ui->tbWM_SlideVertical, cInteractiveZone::eSlideVertical);
   connect( wheelButtons, SIGNAL(buttonClicked(int)), this,SLOT(setWheelMode(int)));
}

//====================================================================================================================

void DlgSlideProperties::resizeEvent(QResizeEvent *ev) 
{
   QDialog::resizeEvent(ev);
   QScrollBar *hscrollbar = ShotTable->horizontalScrollBar();
   int scrollbarheight = 21;
   if (hscrollbar)
   {
      qDebug() << "scrollbar height = " << hscrollbar->height();
      scrollbarheight = hscrollbar->height();
   }
   ShotTable->setFixedHeight((ApplicationConfig->TimelineHeight/3)*2 + scrollbarheight);
   //qDebug() << "ApplicationConfig->TimelineHeight " << ApplicationConfig->TimelineHeight;
   //qDebug() << "ShotTable->height() " <<ShotTable->height();
   //qDebug() << "ShotTable->viewport()->height() " << ShotTable->viewport()->height();
   //qDebug() << "new RowHeight is " << ApplicationConfig->TimelineHeight / 2 + (ShotTable->height() - ShotTable->viewport()->height());
   //ShotTable->setFixedHeight(ApplicationConfig->TimelineHeight/2+(ShotTable->height()-ShotTable->viewport()->height()));
//   //InteractiveZone->RefreshDisplay();
}

//====================================================================================================================

void DlgSlideProperties::showEvent(QShowEvent *ev) 
{
   QCustomDialog::showEvent(ev);

   if (CurrentShot == NULL) 
   {
      // Init ShotTable
      ShotTable->setUpdatesEnabled(false);

      ShotTable->setRowCount(1);
      ShotTable->setRowHeight(0,ShotTable->viewport()->height());
      ShotTable->setColumnCount(CurrentSlide->shotList.count()/*-1*/);

      for (int i = 0; i < CurrentSlide->shotList.count(); i++) 
         ShotTable->setColumnWidth(i, CurrentSlide->pDiaporama->GetWidthForHeight(ShotTable->rowHeight(0)));
      ShotTable->setCurrentCell(0,0);
      ShotTable->setUpdatesEnabled(true);

      ui->ZoomED->setFixedWidth(ui->ScrollXED->width());
   }
}

//====================================================================================================================
// Save object before modification for cancel button

void DlgSlideProperties::PrepareGlobalUndo() {
    Undo=new QDomDocument(APPLICATION_NAME);
    QDomElement root=Undo->createElement("UNDO-DLG");                                               // Create xml document and root
    CurrentSlide->SaveToXML(root,"UNDO-DLG-OBJECT",CurrentSlide->pDiaporama->ProjectFileName,true,NULL,NULL,false);     // Save object
    Undo->appendChild(root);                                                                        // Add object to xml document
}

//====================================================================================================================
// Apply Undo : call when user click on Cancel button

void DlgSlideProperties::DoGlobalUndo() {
    QDomElement root=Undo->documentElement();
    if (root.tagName()=="UNDO-DLG") CurrentSlide->LoadFromXML(root,"UNDO-DLG-OBJECT","",NULL,NULL,false);
}

//====================================================================================================================

void DlgSlideProperties::SaveAdditionnalWindowState(QDomElement &domDocument) {
    int             ScrollBarValue=ui->scrollArea->verticalScrollBar()->value();
    QDomDocument    DomDocument;
    QDomElement     Element=DomDocument.createElement("ScrollBar");
    Element.setAttribute("ScrollBarValue",ScrollBarValue);
    domDocument.appendChild(Element);
}

void DlgSlideProperties::RestoreAdditionnalWindowState(QDomElement domDocument) {
    if ((domDocument.elementsByTagName("ScrollBar").length()>0)&&(domDocument.elementsByTagName("ScrollBar").item(0).isElement()==true)) {
        QDomElement Element=domDocument.elementsByTagName("ScrollBar").item(0).toElement();
        if (Element.hasAttribute("ScrollBarValue")) {
            int ScrollBarValue=Element.attribute("ScrollBarValue").toInt();
            ui->scrollArea->verticalScrollBar()->setValue(ScrollBarValue);
        }
    }
}

//====================================================================================================================

void DlgSlideProperties::keyReleaseEvent(QKeyEvent *event) {
    if ((focusWidget()==BlockTable)||(focusWidget()==InteractiveZone)||(focusWidget()==ShotTable)) {
        if (event->modifiers()==Qt::ControlModifier) {
            switch (event->key()) {
                case Qt::Key_C      : s_BlockTable_Copy();                      break;
                case Qt::Key_X      : s_BlockTable_Cut();                       break;
                case Qt::Key_V      : s_BlockTable_Paste();                     break;
                case Qt::Key_H      : s_BlockSettings_ToggleVisibleState();     break;
                default             : QCustomDialog::keyReleaseEvent(event);    break;
            }
        } else if (event->modifiers()==Qt::NoModifier) {
            switch (event->key()) {
                case Qt::Key_Delete :
                    if ((InteractiveZone->hasFocus())||(BlockTable->hasFocus())) s_BlockTable_RemoveBlock();
                    else if (ShotTable->hasFocus()) s_ShotTable_RemoveShot();
                    break;
                default :
                    QCustomDialog::keyReleaseEvent(event);
                    break;
            }
        } else QCustomDialog::keyReleaseEvent(event);
    } else QCustomDialog::keyReleaseEvent(event);
}

//====================================================================================================================
// Call when user click on Ok button

bool DlgSlideProperties::DoAccept() {
    InRefreshControls=true;
    CurrentSlide->pDiaporama->UpdateChapterInformation();
    CurrentSlide->releaseCachObjects();
    CurrentSlide->ComputeOptimisationFlags();
    emit(SetModifyFlag());
    return true;
}

void DlgSlideProperties::OKPrevious() {
    InRefreshControls=true;
    SaveWindowState();  // Save Window size and position
    CurrentSlide->pDiaporama->UpdateChapterInformation();
    CurrentSlide->releaseCachObjects();
    CurrentSlide->ComputeOptimisationFlags();
    emit(SetModifyFlag());
    done(2);            // Close the box
}

void DlgSlideProperties::OKNext() {
    InRefreshControls=true;
    SaveWindowState();  // Save Window size and position
    CurrentSlide->pDiaporama->UpdateChapterInformation();
    CurrentSlide->releaseCachObjects();
    CurrentSlide->ComputeOptimisationFlags();
    emit(SetModifyFlag());
    done(3);            // Close the box
}

void DlgSlideProperties::DoRejet() {
    InRefreshControls=true;
    CurrentSlide->ComputeOptimisationFlags();
}

//====================================================================================================================

void DlgSlideProperties::PreparePartialUndo(int,QDomElement root,bool) {
    // Save current shot number
    root.setAttribute("CurrentShot",CurrentShotNbr);

    // Save blocktable current selection
    QModelIndexList SelList=BlockTable->selectionModel()->selectedIndexes();
    QString         Selection;
    for (int i=0;i<SelList.count();i++) Selection=Selection+(Selection.length()>0?"###":"")+QString("%1").arg(SelList.at(i).row());
    root.setAttribute("Selection",Selection);
    CurrentSlide->SaveToXML(root,"UNDO-DLG-OBJECT",CurrentSlide->pDiaporama->ProjectFileName,true,NULL,NULL,false);     // Save all
}

//====================================================================================================================

void DlgSlideProperties::ApplyPartialUndo(int,QDomElement root) {
    ShotTable->setUpdatesEnabled(false);
    BlockTable->setUpdatesEnabled(false);
    InRefreshControls=true;

    // Restore all slide
    CurrentSlide->LoadFromXML(root,"UNDO-DLG-OBJECT","",NULL,NULL,false);   // Restore all
    // Reset shot table
    while (ShotTable->columnCount()>0) ShotTable->removeColumn(ShotTable->columnCount()-1);
    for (int i = 0; i<CurrentSlide->shotList.count(); i++)
    {
        ShotTable->insertColumn(i);
        ShotTable->setColumnWidth(i,CurrentSlide->pDiaporama->GetWidthForHeight(ShotTable->rowHeight(0)));
    }

    // Restore current shot context
    CurrentShotNbr                  =root.attribute("CurrentShot").toInt();
    ShotSelectMode                  =SELECTMODE_ONE;
    CurrentShot = CurrentSlide->shotList[CurrentShotNbr];
    CompositionList                 =&CurrentShot->ShotComposition;
    BlockTable->CompositionList     =CompositionList;
    BlockTable->CurrentShotNbr      =CurrentShotNbr;
    InteractiveZone->CurrentShotNbr =CurrentShotNbr;
    ShotTable->setCurrentCell(0,CurrentShotNbr);

    // Reset blocktable
    BlockTable->setRowCount(CompositionList->compositionObjects.count());
    for (int i=0;i<BlockTable->rowCount();i++) 
       BlockTable->setRowHeight(i,48+2+((((cApplicationConfig *)ApplicationConfig)->TimelineHeight-TIMELINEMINHEIGHT)/20+1)*3);

    // Restore blocktable selection
    QStringList SelList=root.attribute("Selection").split("###");
    BlockTable->clearSelection();
    BlockTable->setCurrentCell(SelList[0].toInt(),0,QItemSelectionModel::Select|QItemSelectionModel::Current);
    for (int i=1;i<SelList.count();i++) 
       BlockTable->setCurrentCell(SelList[i].toInt(),0,QItemSelectionModel::Select);

    // Remove thumb if it's first shot
    if (CurrentShotNbr==0) 
    {
        ApplicationConfig->ImagesCache.RemoveImageObject(CurrentSlide->ThumbnailKey,-1);
        ApplicationConfig->SlideThumbsTable->ClearThumb(CurrentSlide->ThumbnailKey);
    }

    InRefreshControls=false;
    CurrentSlide->pDiaporama->UpdateChapterInformation();
    BlockTable->setUpdatesEnabled(true);
    ShotTable->setUpdatesEnabled(true);
    InteractiveZone->RefreshDisplay();
    RefreshControls(true);
}

//====================================================================================================================

void DlgSlideProperties::RefreshStyleControls() 
{
   if (InRefreshStyleControls) return;
   InRefreshStyleControls = true;

   bool IsVisible = (BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObject);
   ui->FramingStyleCB    ->setEnabled(IsVisible);
   ui->TextFramingStyleCB->setEnabled(IsVisible);
   ui->ShapeSizePosCB    ->setEnabled(IsVisible);

   if (IsVisible) 
   {
      ui->BlockShapeStyleED->setText(ApplicationConfig->StyleBlockShapeCollection.GetStyleName(CurrentCompoObject->GetBlockShapeStyle()));

      ui->ShapeSizePosCB->setUpdatesEnabled(false);
      ui->ShapeSizePosCB->clear();
      int ImageType = CurrentCompoObject->BackgroundBrush->GetImageType();
      int AutoCompo = CurrentCompoObject->GetAutoCompoSize(CurrentSlide->pDiaporama->ImageGeometry);

      if (AutoCompo == AUTOCOMPOSIZE_CUSTOM) {                                                                                  // AUTOCOMPOSIZE_CUSTOM
         ui->ShapeSizePosCB->addItem(QApplication::translate("DlgImageCorrection","Custom size"),QVariant(AUTOCOMPOSIZE_CUSTOM));
         ui->ShapeSizePosCB->setCurrentIndex(ui->ShapeSizePosCB->count()-1);
      }
      if ((ImageType!=AUTOCOMPOSIZE_CUSTOM)&&(ImageType!=IMAGETYPE_PHOTOLANDSCAPE)&&(ImageType!=IMAGETYPE_PHOTOPORTRAIT)) {   // AUTOCOMPOSIZE_REALSIZE
         ui->ShapeSizePosCB->addItem(QApplication::translate("DlgImageCorrection","Real image size"),QVariant(AUTOCOMPOSIZE_REALSIZE));
         if (AutoCompo==AUTOCOMPOSIZE_REALSIZE) ui->ShapeSizePosCB->setCurrentIndex(ui->ShapeSizePosCB->count()-1);
      }
      ui->ShapeSizePosCB->addItem(QApplication::translate("DlgImageCorrection","Full screen size"),QVariant(AUTOCOMPOSIZE_FULLSCREEN));           // AUTOCOMPOSIZE_FULLSCREEN
      if (AutoCompo==AUTOCOMPOSIZE_FULLSCREEN) ui->ShapeSizePosCB->setCurrentIndex(ui->ShapeSizePosCB->count()-1);
      ui->ShapeSizePosCB->addItem(QApplication::translate("DlgImageCorrection","TV margins"),QVariant(AUTOCOMPOSIZE_TVMARGINS));                  // AUTOCOMPOSIZE_TVMARGINS
      if (AutoCompo==AUTOCOMPOSIZE_TVMARGINS) ui->ShapeSizePosCB->setCurrentIndex(ui->ShapeSizePosCB->count()-1);
      ui->ShapeSizePosCB->addItem(QApplication::translate("DlgImageCorrection","Two thirds screen"),QVariant(AUTOCOMPOSIZE_TWOTHIRDSSCREEN));     // AUTOCOMPOSIZE_TWOTHIRDSSCREEN
      if (AutoCompo==AUTOCOMPOSIZE_TWOTHIRDSSCREEN) ui->ShapeSizePosCB->setCurrentIndex(ui->ShapeSizePosCB->count()-1);
      ui->ShapeSizePosCB->addItem(QApplication::translate("DlgImageCorrection","Half screen"),QVariant(AUTOCOMPOSIZE_HALFSCREEN));                // AUTOCOMPOSIZE_HALFSCREEN
      if (AutoCompo==AUTOCOMPOSIZE_HALFSCREEN) ui->ShapeSizePosCB->setCurrentIndex(ui->ShapeSizePosCB->count()-1);
      ui->ShapeSizePosCB->addItem(QApplication::translate("DlgImageCorrection","Third screen"),QVariant(AUTOCOMPOSIZE_THIRDSCREEN));              // AUTOCOMPOSIZE_THIRDSCREEN
      if (AutoCompo==AUTOCOMPOSIZE_THIRDSCREEN) ui->ShapeSizePosCB->setCurrentIndex(ui->ShapeSizePosCB->count()-1);
      ui->ShapeSizePosCB->addItem(QApplication::translate("DlgImageCorrection","Quarter screen"),QVariant(AUTOCOMPOSIZE_QUARTERSCREEN));          // AUTOCOMPOSIZE_QUARTERSCREEN
      if (AutoCompo==AUTOCOMPOSIZE_QUARTERSCREEN) ui->ShapeSizePosCB->setCurrentIndex(ui->ShapeSizePosCB->count()-1);
      ui->ShapeSizePosCB->setUpdatesEnabled(true);

      StopMajFramingStyle=true;
      if (!CurrentCompoObject->BackgroundBrush->MediaObject || CurrentCompoObject->BackgroundBrush->MediaObject->is_Unmanaged()) 
      {
         // It's a text block

         if (ui->FramingStyleCB->isVisible())         ui->FramingStyleCB->setVisible(false);
         if (!ui->TextFramingStyleCB->isVisible())    ui->TextFramingStyleCB->setVisible(true);
         if (!ui->TextFramingStyleLabel->isVisible()) ui->TextFramingStyleLabel->setVisible(true);

         if (ui->TextFramingStyleCB->itemText(0)!=QApplication::translate("DlgImageCorrection","Unlock")) {
            ui->TextFramingStyleCB->setUpdatesEnabled(false);
            ui->TextFramingStyleCB->clear();
            ui->TextFramingStyleCB->addItem(QIcon(ICON_GEOMETRY_UNLOCKED),QApplication::translate("DlgImageCorrection","Unlock"),QVariant(AUTOFRAMING_CUSTOMLOCK));
            ui->TextFramingStyleCB->addItem(QIcon(ICON_GEOMETRY_LOCKED),  QApplication::translate("DlgImageCorrection","Lock to this geometry"),QVariant(AUTOFRAMING_CUSTOMUNLOCK));
            ui->TextFramingStyleCB->addItem(QIcon(ICON_GEOMETRY_PROJECT), QApplication::translate("DlgImageCorrection","Lock to project geometry"),QVariant(AUTOFRAMING_CUSTOMPRJLOCK));
         }
         int Index=1;
         if (!CurrentCompoObject->BackgroundBrush->LockGeometry()) Index=0; else if (CurrentCompoObject->BackgroundBrush->AspectRatio()==ProjectGeometry) Index=2;
         if (ui->TextFramingStyleCB->currentIndex()!=Index) ui->TextFramingStyleCB->setCurrentIndex(Index);
         if (!ui->TextFramingStyleCB->updatesEnabled()) ui->TextFramingStyleCB->setUpdatesEnabled(true);

      } else {
         // It's Image or video or ...
         if (ui->TextFramingStyleCB->isVisible()) ui->TextFramingStyleCB->setVisible(false);
         if (!ui->FramingStyleCB->isVisible())    ui->FramingStyleCB->setVisible(true);

         if ((FramingCB_CurrentBrush!=CurrentCompoObject->BackgroundBrush)||(FramingCB_CurrentShot!=CurrentShotNbr)) {

            int Position=0;
            // Compute position of video
            if (ISVIDEO(CurrentCompoObject->BackgroundBrush)) {
               for (int i = 0; i<ShotTable->currentColumn(); i++) for (int j = 0; j<CurrentSlide->shotList[i]->ShotComposition.compositionObjects.count(); j++)
                  if (CurrentSlide->shotList[i]->ShotComposition.compositionObjects[j]->index() == CurrentCompoObject->index())
                  {
                     if (CurrentSlide->shotList[i]->ShotComposition.compositionObjects[j]->isVisible()) Position += CurrentSlide->shotList[i]->StaticDuration;
                  }
            }

            QImage *CachedImage=NULL;
            if (CurrentCompoObject->BackgroundBrush->MediaObject) 
            {
               if (!CurrentCompoObject->BackgroundBrush->MediaObject->is_Videofile()) 
                  CachedImage=CurrentCompoObject->BackgroundBrush->MediaObject->ImageAt(true);
               else
                  CachedImage=((cVideoFile *)CurrentCompoObject->BackgroundBrush->MediaObject)->ImageAt(true,Position+QTime(0,0,0,0).msecsTo(((cVideoFile *)CurrentCompoObject->BackgroundBrush->MediaObject)->StartTime),NULL,CurrentCompoObject->BackgroundBrush->Deinterlace,1,false,false);
            }
            if ((CachedImage)&&(CachedImage->format()!=QImage::Format_ARGB32_Premultiplied)) {
               QImage *NewCachedImage=new QImage(CachedImage->convertToFormat(QImage::Format_ARGB32_Premultiplied));
               delete CachedImage;
               CachedImage=NewCachedImage;
            }

            if (CachedImage) {
               ui->FramingStyleCB->X=CurrentCompoObject->BackgroundBrush->X();
               ui->FramingStyleCB->Y=CurrentCompoObject->BackgroundBrush->Y();
               ui->FramingStyleCB->ZoomFactor=CurrentCompoObject->BackgroundBrush->ZoomFactor();
               ui->FramingStyleCB->AspectRatio=CurrentCompoObject->BackgroundBrush->AspectRatio();
               ui->FramingStyleCB->PrepareFramingStyleTable(true,/*FILTERFRAMING_ALL*/FILTERFRAMING_IMAGE|FILTERFRAMING_PROJECT,CurrentCompoObject->BackgroundBrush,CachedImage,CurrentCompoObject->BackgroundForm,ProjectGeometry);
               delete CachedImage;
            }
         }
         ui->FramingStyleCB->SetCurrentFraming(CurrentCompoObject->BackgroundBrush->GetCurrentFramingStyle(ProjectGeometry));
      }
      //ui->BackgroundFormCB->PrepareFrameShapeTable(true,0,CurrentCompoObject->BackgroundForm);
      ui->BackgroundFormCB->SetCurrentFrameShape(CurrentCompoObject->BackgroundForm);
      StopMajFramingStyle=false;

      FramingCB_CurrentBrush=CurrentCompoObject->BackgroundBrush;
      FramingCB_CurrentShot =CurrentShotNbr;

   } else {
      FramingCB_CurrentBrush=NULL;
      FramingCB_CurrentShot =CurrentShotNbr;
      ui->ShapeSizePosCB->clear();
      if (!ui->FramingStyleCB->isVisible())        ui->FramingStyleCB->setVisible(true);
      if (ui->TextFramingStyleCB->isVisible())     ui->TextFramingStyleCB->setVisible(false);
      if (!ui->TextFramingStyleLabel->isVisible()) ui->TextFramingStyleLabel->setVisible(false);
      ui->BlockShapeStyleED->setText("");
      ui->FramingStyleCB->clear();
      ui->BackgroundFormCB->clear();
   }

   InRefreshStyleControls=false;
}

//====================================================================================================================

void DlgSlideProperties::RefreshControls(bool UpdateInteractiveZone) 
{
   if ((InRefreshControls) || (!CompositionList)) 
      return;
   InRefreshControls = true;
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   CurrentSlide->ComputeOptimisationFlags();

   //***********************
   // Tools button
   //***********************
   ui->EditBT->                setEnabled((BlockSelectMode == SELECTMODE_ONE));
   ui->ArrangeBT->             setEnabled((BlockSelectMode == SELECTMODE_ONE) || (BlockSelectMode == SELECTMODE_MULTIPLE));
   ui->InfoBlockBT->           setEnabled((BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObject->BackgroundBrush->BrushType == BRUSHTYPE_IMAGEDISK));

   // actions
   ui->actionTop->             setEnabled((BlockSelectMode == SELECTMODE_MULTIPLE));
   ui->actionMiddle->          setEnabled((BlockSelectMode == SELECTMODE_MULTIPLE));
   ui->actionBottom->          setEnabled((BlockSelectMode == SELECTMODE_MULTIPLE));
   ui->actionLeft->            setEnabled((BlockSelectMode == SELECTMODE_MULTIPLE));
   ui->actionCenter->          setEnabled((BlockSelectMode == SELECTMODE_MULTIPLE));
   ui->actionRight->           setEnabled((BlockSelectMode == SELECTMODE_MULTIPLE));
   ui->actionDistributeHoriz-> setEnabled((NbrSelected >= 3));
   ui->actionDistributeVert->  setEnabled((NbrSelected >= 3));
   ui->actionSetVisible->      setEnabled((BlockSelectMode == SELECTMODE_ONE));
   ui->actionSetHide->         setEnabled((BlockSelectMode == SELECTMODE_ONE));
   ui->actionTakeSound->       setEnabled((BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObject->isVisible()) && (ISVIDEO(CurrentCompoObject->BackgroundBrush)) && (CurrentCompoObject->BackgroundBrush->SoundVolume == 0));
   ui->actionEditImage->       setEnabled((BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObject->isVisible()) && (CurrentCompoObject->BackgroundBrush->BrushType == BRUSHTYPE_IMAGEDISK));
   ui->actionEditText->        setEnabled((BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObject->isVisible()));
   ui->actionInfo->            setEnabled((BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObject->BackgroundBrush->BrushType == BRUSHTYPE_IMAGEDISK));
   ui->actionRemoveBlock->     setEnabled((BlockSelectMode == SELECTMODE_ONE) || (BlockSelectMode == SELECTMODE_MULTIPLE));
   ui->actionUpBlock->         setEnabled((BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObjectNbr > 0));
   ui->actionDownBlock->       setEnabled((BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObjectNbr < BlockTable->rowCount()-1));
   ui->actionCopy->            setEnabled((BlockSelectMode == SELECTMODE_ONE) || (BlockSelectMode == SELECTMODE_MULTIPLE));
   ui->actionCut->             setEnabled((BlockSelectMode == SELECTMODE_ONE) || (BlockSelectMode == SELECTMODE_MULTIPLE));
                                                                              
   if ((CurrentCompoObject)&&(CurrentCompoObject->BackgroundBrush->MediaObject)&&(ui->actionEditImage->isEnabled())) {
      switch (CurrentCompoObject->BackgroundBrush->MediaObject->getObjectType()) {
      case OBJECTTYPE_VIDEOFILE:
         ui->actionEditImage->setText(QApplication::translate("DlgSlideProperties","Correct, reframe or cut video","Action title in slide edit dialog + dialog title of image edit dialog"));
         ui->actionEditImage->setToolTip(QApplication::translate("DlgSlideProperties","Define framing and correction of image or cut video","Tooltip"));
         ui->actionEditImage->setIcon(QIcon(ICON_EDIT_MOVIE));
         break;
      case OBJECTTYPE_IMAGEFILE:
      case OBJECTTYPE_IMAGECLIPBOARD:
         ui->actionEditImage->setText(QApplication::translate("DlgSlideProperties","Correct or reframe image","Action title in slide edit dialog + dialog title of image edit dialog"));
         ui->actionEditImage->setToolTip(QApplication::translate("DlgSlideProperties","Define framing and correction of image","Tooltip"));
         ui->actionEditImage->setIcon(QIcon(ICON_EDIT_IMAGE));
         break;
      case OBJECTTYPE_IMAGEVECTOR:
         ui->actionEditImage->setText(QApplication::translate("DlgSlideProperties","Reframe image vector","Action title in slide edit dialog + dialog title of image edit dialog"));
         ui->actionEditImage->setToolTip(QApplication::translate("DlgSlideProperties","Define framing and correction of image vector","Tooltip"));
         ui->actionEditImage->setIcon(QIcon(ICON_EDIT_IMAGE));
         break;
      case OBJECTTYPE_GMAPSMAP:
         ui->actionEditImage->setText(QApplication::translate("DlgSlideProperties","Modify Google Maps map","Action title in slide edit dialog + dialog title of image edit dialog"));
         ui->actionEditImage->setToolTip(QApplication::translate("DlgSlideProperties","Define Modify Google Maps map and framing and correction of map","Tooltip"));
         ui->actionEditImage->setIcon(QIcon(ICON_EDIT_GMAPS));
         break;
      default:    break;  // avoid warning
      }
   }

   // Change icons for visible and getsound buttons/actions
   if (BlockSelectMode == SELECTMODE_ONE) 
      ui->actionTakeSound->setIcon(QIcon(QString(CurrentCompoObject->BackgroundBrush->SoundVolume == 0 ? ICON_SOUND_OK : ICON_SOUND_KO)));


   //**************************
   // Speed wave
   //**************************
   if ((BlockSelectMode==SELECTMODE_ONE)&&(CurrentCompoObject)&&(CurrentCompoObject->isVisible())) {
      ui->SpeedWaveCB->setEnabled(true);
      ui->SpeedWaveLabel->setEnabled(true);
      ui->SpeedWaveCB->SetCurrentValue(CurrentCompoObject->SpeedWave());
   } else {
      ui->SpeedWaveCB->setEnabled(false);
      ui->SpeedWaveLabel->setEnabled(false);
      ui->SpeedWaveCB->SetCurrentValue(SPEEDWAVE_PROJECTDEFAULT);
   }

   //**************************
   // Text animation controls
   //**************************
   if ((BlockSelectMode==SELECTMODE_ONE)&&(CurrentCompoObject)&&(CurrentCompoObject->isVisible())) {
      ui->ZoomLabel->   setEnabled(true);         ui->ZoomSlider->   setEnabled(true);        ui->ZoomED->   setEnabled(true);        ui->ZoomResetBT->   setEnabled(true);
      ui->ScrollXLabel->setEnabled(true);         ui->ScrollXSlider->setEnabled(true);        ui->ScrollXED->setEnabled(true);        ui->ScrollXResetBT->setEnabled(true);
      ui->ScrollYLabel->setEnabled(true);         ui->ScrollYSlider->setEnabled(true);        ui->ScrollYED->setEnabled(true);        ui->ScrollYResetBT->setEnabled(true);

      ui->ZoomSlider->setValue(CurrentCompoObject->tShotParams.TxtZoomLevel);            ui->ZoomED->setValue(CurrentCompoObject->tShotParams.TxtZoomLevel);
      ui->ScrollXSlider->setValue(CurrentCompoObject->tShotParams.TxtScrollX);           ui->ScrollXED->setValue(CurrentCompoObject->tShotParams.TxtScrollX);
      ui->ScrollYSlider->setValue(CurrentCompoObject->tShotParams.TxtScrollY);           ui->ScrollYED->setValue(CurrentCompoObject->tShotParams.TxtScrollY);
   } else {
      ui->ZoomLabel->   setEnabled(false);        ui->ZoomSlider->   setEnabled(false);       ui->ZoomED->   setEnabled(false);       ui->ZoomResetBT->   setEnabled(false);
      ui->ScrollXLabel->setEnabled(false);        ui->ScrollXSlider->setEnabled(false);       ui->ScrollXED->setEnabled(false);       ui->ScrollXResetBT->setEnabled(false);
      ui->ScrollYLabel->setEnabled(false);        ui->ScrollYSlider->setEnabled(false);       ui->ScrollYED->setEnabled(false);       ui->ScrollYResetBT->setEnabled(false);

      ui->ZoomSlider->setValue(100);              ui->ZoomED->setValue(100);
      ui->ScrollXSlider->setValue(0);             ui->ScrollXED->setValue(0);
      ui->ScrollYSlider->setValue(0);             ui->ScrollYED->setValue(0);
   }
   //**************************
   // Block animation controls
   //**************************
   if ((BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObject) && (CurrentCompoObject->isVisible())) 
   {
      ui->BlockAnimLabel->setEnabled(true);
      ui->BlockAnimCB->setEnabled(true);
      ui->BlockAnimCB->setCurrentIndex(CurrentCompoObject->AnimType());
   } else {
      ui->BlockAnimLabel->setEnabled(false);
      ui->BlockAnimCB->setEnabled(false);
      ui->BlockAnimCB->setCurrentIndex(-1);
   }

   // Multiple turn animation
   if ((BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObject) && (CurrentCompoObject->isVisible()) && (CurrentCompoObject->AnimType() == BLOCKANIMTYPE_MULTIPLETURN)) 
   {
      ui->TurnZLabel->setEnabled(true);       ui->TurnZSlider->setEnabled(true);      ui->TurnZED->setEnabled(true);       ui->ResetTurnZBT->setEnabled(true);
      ui->TurnZLabel->setVisible(true);       ui->TurnZSlider->setVisible(true);      ui->TurnZED->setVisible(true);       ui->ResetTurnZBT->setVisible(true);
      ui->TurnXLabel->setEnabled(true);       ui->TurnXSlider->setEnabled(true);      ui->TurnXED->setEnabled(true);       ui->ResetTurnXBT->setEnabled(true);
      ui->TurnXLabel->setVisible(true);       ui->TurnXSlider->setVisible(true);      ui->TurnXED->setVisible(true);       ui->ResetTurnXBT->setVisible(true);
      ui->TurnYLabel->setEnabled(true);       ui->TurnYSlider->setEnabled(true);      ui->TurnYED->setEnabled(true);       ui->ResetTurnYBT->setEnabled(true);
      ui->TurnYLabel->setVisible(true);       ui->TurnYSlider->setVisible(true);      ui->TurnYED->setVisible(true);       ui->ResetTurnYBT->setVisible(true);
      ui->TurnZED->setValue(CurrentCompoObject->ZTurns());                           ui->TurnZSlider->setValue(CurrentCompoObject->ZTurns());
      ui->TurnXED->setValue(CurrentCompoObject->XTurns());                           ui->TurnXSlider->setValue(CurrentCompoObject->XTurns());
      ui->TurnYED->setValue(CurrentCompoObject->YTurns());                           ui->TurnYSlider->setValue(CurrentCompoObject->YTurns());
   } else {
      ui->TurnZLabel->setVisible(false);      ui->TurnZSlider->setVisible(false);     ui->TurnZED->setVisible(false);     ui->ResetTurnZBT->setVisible(false);
      ui->TurnXLabel->setVisible(false);      ui->TurnXSlider->setVisible(false);     ui->TurnXED->setVisible(false);     ui->ResetTurnXBT->setVisible(false);
      ui->TurnYLabel->setVisible(false);      ui->TurnYSlider->setVisible(false);     ui->TurnYED->setVisible(false);     ui->ResetTurnYBT->setVisible(false);
   }

   // Dissolve animation
   if ((BlockSelectMode == SELECTMODE_ONE) && (CurrentCompoObject) && (CurrentCompoObject->isVisible()) && (CurrentCompoObject->AnimType() == BLOCKANIMTYPE_DISSOLVE)) 
   {
      ui->DissolveLabel->setEnabled(true);      ui->DissolveLabel->setVisible(true);
      ui->DissolveCB->   setEnabled(true);      ui->DissolveCB->   setVisible(true);
      ui->DissolveCB->setCurrentIndex(CurrentCompoObject->DissolveType());
   } 
   else 
   {
      ui->DissolveLabel->setEnabled(false);     ui->DissolveLabel->setVisible(false);
      ui->DissolveCB->   setEnabled(false);     ui->DissolveCB->   setVisible(false);
   }

   RefreshStyleControls();
   
   //cCompositionObjectMask *mask = CurrentCompoObject->getMask();
   //if( mask)
   //   ui->cbMaskShape->SetCurrentFrameShape(mask->getBackgroundForm());

   QApplication::restoreOverrideCursor();
   InRefreshControls = false;

   cShotComposer::RefreshControls(UpdateInteractiveZone);
}

//====================================================================================================================
// Refresh background image of the scene

void DlgSlideProperties::s_RefreshSceneImage() {
    InteractiveZone->RefreshDisplay();
}

//====================================================================================================================
// Display or hide rulers
//====================================================================================================================

void DlgSlideProperties::s_RulersBt() {
    DlgRulerDef Dlg(&InteractiveZone->MagneticRuler,true,ApplicationConfig,this);
    Dlg.InitDialog();
    connect(&Dlg,SIGNAL(RefreshDisplay()),this,SLOT(s_RefreshSceneImage()));
    if (Dlg.exec()==0) {
        ApplicationConfig->SlideRuler=InteractiveZone->MagneticRuler;
        ui->RulersBT->setIcon(QIcon(QString(InteractiveZone->MagneticRuler!=0?ICON_RULER_ON:ICON_RULER_OFF)));
    }
    InteractiveZone->RefreshDisplay();
}

//====================================================================================================================

cCompositionObject *DlgSlideProperties::GetSelectedGlobalCompositionObject() 
{
   int CurrentBlock = BlockTable->currentRow();
   if ((CurrentBlock < 0) || (CurrentBlock >= CompositionList->compositionObjects.count())) 
      return NULL;
   return GetGlobalCompositionObject(CompositionList->compositionObjects[CurrentBlock]->index());
}

//====================================================================================================================

cCompositionObject *DlgSlideProperties::GetSelectedCompositionObject() 
{
   int CurrentBlock = BlockTable->currentRow();
   if ((CurrentBlock < 0) || (CurrentBlock >= CompositionList->compositionObjects.count())) 
      return NULL; 
   return CompositionList->compositionObjects[CurrentBlock];
}

//====================================================================================================================
// SLIDE SETTINGS
//====================================================================================================================

void DlgSlideProperties::s_SlideSet_ChapterInformation() 
{
   AppendPartialUndo(UNDOACTION_FULL_SLIDE,InteractiveZone,true);
   DlgChapter Dlg(CurrentSlide,ApplicationConfig,this);
   Dlg.InitDialog();
   if (Dlg.exec() != 0) 
      RemoveLastPartialUndo(); 
   else 
      CurrentSlide->pDiaporama->UpdateChapterInformation();
   RefreshControls(true);
}

//===================================================================================

void DlgSlideProperties::s_SlideSet_SlideNameChange(QString NewText) {
    if (InRefreshControls) return;  // No action if in control setup
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ui->SlideNameED,false);

    CurrentSlide->SlideName=NewText;
}

//===================================================================================

void DlgSlideProperties::s_SlideSet_SaveModel() {
    QMenu *ContextMenu=new QMenu(this);
    ContextMenu->addAction(ui->actionSaveAsProjectTitleModel);
    ContextMenu->addAction(ui->actionSaveAsCptTitleModels);
    ContextMenu->addAction(ui->actionSaveAsCreditTitleModels);
    ContextMenu->exec(QCursor::pos());
    delete ContextMenu;
    ui->SaveModelBT->setDown(false);
}

//===================================================================================

void DlgSlideProperties::s_SlideSet_SaveAsProjectTitleModel() {
    CurrentSlide->SaveAsNewCustomModelFile(ffd_MODELTYPE_PROJECTTITLE);
    CustomMessageBox(this,QMessageBox::Information,ui->actionSaveAsProjectTitleModel->text(),
                     QApplication::translate("DlgSlideProperties","Slide added in Project title models collection","Information message"),QMessageBox::Close);
}

//===================================================================================

void DlgSlideProperties::s_SlideSet_SaveAsCptTitleModels() {
    CurrentSlide->SaveAsNewCustomModelFile(ffd_MODELTYPE_CHAPTERTITLE);
    CustomMessageBox(this,QMessageBox::Information,ui->actionSaveAsCptTitleModels->text(),
                     QApplication::translate("DlgSlideProperties","Slide added in Chapter title models collection","Information message"),QMessageBox::Close);
}

//===================================================================================

void DlgSlideProperties::s_SlideSet_SaveAsCreditTitleModels() {
    CurrentSlide->SaveAsNewCustomModelFile(ffd_MODELTYPE_CREDITTITLE);
    CustomMessageBox(this,QMessageBox::Information,ui->actionSaveAsCreditTitleModels->text(),
                     QApplication::translate("DlgSlideProperties","Slide added in Credit title models collection","Information message"),QMessageBox::Close);
}

//====================================================================================================================
// SHOT SETTINGS
//====================================================================================================================

void DlgSlideProperties::s_ShotTable_DurationChange(QTime NewValue) 
{
   if(InDisplayDuration || CurrentShot == NULL) 
      return;  // No action if in control setup or if no shot was selected
   AppendPartialUndo(UNDOACTION_FULL_SLIDE,ui->ShotDurationED,false);

   CurrentShot->StaticDuration = QTime(0,0,0,0).msecsTo(NewValue);
   s_ShotTable_DisplayDuration();
}

//===================================================================================

void DlgSlideProperties::s_ShotTable_AddShot() 
{
   if( InRefreshControls || CurrentShot == NULL ) 
      return;  // No action if in control setup or if no shot was selected
   AppendPartialUndo(UNDOACTION_FULL_SLIDE,ShotTable,true);

   // Insert a new empty shot after current shot in the current slide
   CurrentSlide->shotList.insert(CurrentShotNbr + 1, new cDiaporamaShot(CurrentSlide));

   // Fill this new shot with copy of all blocks of current shot
   cDiaporamaShot *imagesequence = CurrentSlide->shotList[CurrentShotNbr + 1];
   for (int i = 0; i < CompositionList->compositionObjects.count(); i++)
   {
      imagesequence->ShotComposition.compositionObjects.append(new cCompositionObject(eCOMPOSITIONTYPE_SHOT, CompositionList->compositionObjects[i]->index(), ApplicationConfig, &imagesequence->ShotComposition));
      imagesequence->ShotComposition.compositionObjects[i]->CopyFromCompositionObject(CompositionList->compositionObjects[i]);
   }

   ShotTable->setUpdatesEnabled(false);
   ShotTable->insertColumn(CurrentShotNbr+1);
   ShotTable->setColumnWidth(CurrentShotNbr+1,CurrentSlide->pDiaporama->GetWidthForHeight(ShotTable->rowHeight(0)));

   // Change current shot to this new shot
   ShotTable->SetCurrentCell(CurrentShotNbr+1);
   ShotTable->setUpdatesEnabled(true);
}

//===================================================================================

void DlgSlideProperties::s_ShotTable_RemoveShot() 
{
   if (CurrentSlide->shotList.count() < 2) 
      return;               // Don't remove last shot
   if (InRefreshControls || CurrentShot == NULL ) 
      return;   // No action if in control setup or if no shot was selected
   if (ApplicationConfig->AskUserToRemove
      && CustomMessageBox(this,QMessageBox::Question,QApplication::translate("DlgSlideProperties","Remove shot"),
         QApplication::translate("DlgSlideProperties","Are you sure you want to delete this shot?"),
         QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No
      ) 
      return;

   AppendPartialUndo(UNDOACTION_FULL_SLIDE,ShotTable,true);

   InRefreshControls = true;
   CurrentShot = NULL;
   CompositionList = NULL;
   BlockTable->CompositionList = CompositionList;

   // Remove shot from slide
   delete CurrentSlide->shotList.takeAt(CurrentShotNbr);

   // Remove shot from shot table
   int ShotNbr = CurrentShotNbr;
   ShotTable->setUpdatesEnabled(false);
   ShotTable->removeColumn(ShotNbr);
   if (ShotNbr >= CurrentSlide->shotList.count()) 
      ShotNbr--;
   ShotTable->SetCurrentCell(ShotNbr);
   BlockTable->CurrentShotNbr = ShotNbr;
   ShotTable->setUpdatesEnabled(true);
   ResetThumbs(false);

   InRefreshControls = false;
   s_ShotTable_SelectionChanged();
}

//===================================================================================

void DlgSlideProperties::s_ShotTable_DragMoveItem() {
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ShotTable,true);

    int Src=ShotTable->DragItemSource;
    int Dst=ShotTable->DragItemDest;
    if ((Src<0) || (Src >= CurrentSlide->shotList.count()) || (Dst<0) || (Dst>CurrentSlide->shotList.count())) return;

    cDiaporamaShot *ObjSrc = CurrentSlide->shotList.takeAt(Src);
    if (Src<Dst) Dst--;
    CurrentSlide->shotList.insert(Dst, ObjSrc);

    ShotTable->setUpdatesEnabled(false);
    ShotTable->SetCurrentCell(Dst);
    ShotTable->setUpdatesEnabled(true);  // Reset timeline painting
}

//===================================================================================

void DlgSlideProperties::s_ShotTable_MoveLeft() {
    if ((InRefreshControls)||(CurrentShot==NULL)) return;  // No action if in control setup or if no shot was selected
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ShotTable,true);

    if (CurrentShotNbr>0) 
    {
       CurrentSlide->shotList.QT_SWAP(CurrentShotNbr, CurrentShotNbr - 1);
        ShotTable->setUpdatesEnabled(false);
        ShotTable->SetCurrentCell(CurrentShotNbr-1);
        ShotTable->setUpdatesEnabled(true);  // Reset timeline painting
    }
}

//===================================================================================

void DlgSlideProperties::s_ShotTable_MoveRight() 
{
   if(InRefreshControls || CurrentShot == NULL) 
      return;  // No action if in control setup or if no shot was selected
   AppendPartialUndo(UNDOACTION_FULL_SLIDE,ShotTable,true);

   if (CurrentShotNbr<CurrentSlide->shotList.count() - 1)
   {
      CurrentSlide->shotList.QT_SWAP(CurrentShotNbr + 1, CurrentShotNbr);
      ShotTable->setUpdatesEnabled(false);
      ShotTable->SetCurrentCell(CurrentShotNbr+1);
      ShotTable->setUpdatesEnabled(true);  // Reset timeline painting
   }
}

//===================================================================================

void DlgSlideProperties::s_ShotTable_SelectionChanged() 
{
   if (InRefreshControls) 
      return;  // No action if in control setup or if no shot was selected

   if( CurrentShot != NULL )
      CurrentShot->pDiaporamaObject->releaseCachObjects();

   // Because block order can change from a shot to another, try to keep current block by looking at indexkey
   int i = BlockTable->currentRow();
   int IndexKey = -1;
   if ((CompositionList != NULL) && ( i >= 0) && (i < CompositionList->compositionObjects.count())) 
      IndexKey = CompositionList->compositionObjects[i]->index();

   QList<int> selIndices;
   BlockTable->getSelectedIndices(selIndices);
   // Keep link to current shot
   CurrentShotNbr = ShotTable->currentColumn();
   if (CurrentShotNbr < 0 || CurrentShotNbr >= CurrentSlide->shotList.count())
   {
      ShotSelectMode  = SELECTMODE_NONE;
      CurrentShot     = NULL;
      CompositionList = NULL;
      CurrentShotNbr  = -1;
   } 
   else 
   {
      ShotSelectMode  = SELECTMODE_ONE;
      CurrentShot = CurrentSlide->shotList[CurrentShotNbr];
      CompositionList = &CurrentShot->ShotComposition;
   }


   int CurrentBlockTableIndex = 0;
   if (IndexKey!=-1) 
   {
      while (CurrentBlockTableIndex < CompositionList->compositionObjects.count() && CompositionList->compositionObjects[CurrentBlockTableIndex]->index() != IndexKey) 
         CurrentBlockTableIndex++;
      if (CurrentBlockTableIndex >= CompositionList->compositionObjects.count()) 
         CurrentBlockTableIndex = 0;
   }

   BlockTable->CompositionList     = CompositionList;
   BlockTable->CurrentShotNbr      = CurrentShotNbr;
   InteractiveZone->CurrentShotNbr = CurrentShotNbr;

   ui->actionMoveLeft->setEnabled(CurrentShotNbr > 0);
   ui->actionMoveRight->setEnabled(CurrentShotNbr<ShotTable->columnCount()-1);
   ui->RemoveShot->setEnabled(CurrentSlide->shotList.count() > 1);
   ui->actionRemoveShot->setEnabled(CurrentSlide->shotList.count() > 1);

   s_ShotTable_DisplayDuration();
   BlockTable->selectIndices(selIndices);
   RefreshBlockTable(selIndices.count() > 0 ? -1 : 0 /*CurrentBlockTableIndex*/ /*-1*/);
}

//===================================================================================

void DlgSlideProperties::s_ShotTable_DisplayDuration() 
{
    // duration and shot duration
    InDisplayDuration=true;
    ui->SlideDurationLabel->setText(QTime(0,0,0,0).addMSecs(CurrentSlide->GetDuration()).toString("hh:mm:ss.zzz"));
    QDateTimeEdit::Section CurrentSection=ui->ShotDurationED->currentSection();
    int CurrentSectionIndex=ui->ShotDurationED->currentSectionIndex();
    ui->ShotDurationED->setTime(QTime(0,0,0,0).addMSecs(CurrentShot->StaticDuration));
    ui->ShotDurationED->setCurrentSection(CurrentSection);
    ui->ShotDurationED->setCurrentSectionIndex(CurrentSectionIndex);

    // Minimum shot duration display
    int  AddingDuration = 0;
    if (CurrentShotNbr == (CurrentSlide->shotList.count() - 1))
    {   // If it's the last shot
       int TotalDuration = CurrentSlide->GetDuration();
       int Duration = 0;
       for (int i = 0; i < CurrentSlide->shotList.count(); i++) 
          Duration = Duration + CurrentSlide->shotList[i]->StaticDuration;
       if (Duration < TotalDuration) 
          AddingDuration = TotalDuration - Duration;
    }
    if (AddingDuration == 0)
       ui->MinShotDurationLabel->setText("");
    else 
    {
        int     qDuration = CurrentShot->StaticDuration+AddingDuration;
        int     msec     = qDuration % 1000;          qDuration = qDuration/1000;
        int     sec      = qDuration % 60;            qDuration = qDuration/60;
        int     mn       = qDuration % 60;            qDuration = qDuration/60;
        int     hours    = qDuration % 24;
        int     days     = qDuration / 24;
        QString Duration;
        if (days > 0)
           Duration = QString("%1.%2:%3:%4.%5").arg(ito2a(days)).arg(ito2a(hours)).arg(ito2a(mn)).arg(ito2a(sec)).arg(ito3a(msec));
        else
           Duration = QString("%1:%2:%3.%4").arg(ito2a(hours)).arg(ito2a(mn)).arg(ito2a(sec)).arg(ito3a(msec));
        ui->MinShotDurationLabel->setText(QString(QApplication::translate("DlgSlideProperties","Lengthened to %1")).arg(Duration));
    }

    InDisplayDuration = false;
    for (int i = CurrentShotNbr; i < CurrentSlide->shotList.count(); i++) 
       ShotTable->RepaintCell(i);
    if (CurrentShotNbr != CurrentSlide->shotList.count() - 1) 
       ShotTable->RepaintCell(CurrentSlide->shotList.count() - 1);
}

//===================================================================================

void DlgSlideProperties::s_ShotTable_RightClickEvent(QMouseEvent *) 
{
    QMenu *ContextMenu=new QMenu(this);
    ContextMenu->addAction(ui->actionAddShot);
    ContextMenu->addAction(ui->actionRemoveShot);
    ContextMenu->addAction(ui->actionMoveLeft);
    ContextMenu->addAction(ui->actionMoveRight);
    ContextMenu->exec(QCursor::pos());
    delete ContextMenu;
}

//====================================================================================================================
// BLOCK TABLE SETTINGS
//====================================================================================================================

void DlgSlideProperties::s_BlockTable_StartSelectionChange() 
{
    InSelectionChange=true;
}

void DlgSlideProperties::s_BlockTable_EndSelectionChange() 
{
    InSelectionChange=false;
    s_BlockTable_SelectionChanged();
}

//====================================================================================================================
// User double click on a block in the BlockTable widget or in the scene

void DlgSlideProperties::s_BlockTable_ItemDoubleClicked(QMouseEvent *) 
{
   if (BlockSelectMode != SELECTMODE_ONE) 
      return;
   if (CurrentCompoObject->BackgroundBrush->BrushType != BRUSHTYPE_IMAGEDISK)
      s_BlockSettings_TextEditor();
   else
      s_BlockSettings_ImageEditCorrect();
}

//====================================================================================================================

void DlgSlideProperties::s_BlockSettings_Arrange() 
{
    QMenu *ContextMenu=new QMenu(this);
    ContextMenu->addAction(ui->actionTop);
    ContextMenu->addAction(ui->actionMiddle);
    ContextMenu->addAction(ui->actionBottom);
    ContextMenu->addSeparator();
    ContextMenu->addAction(ui->actionLeft);
    ContextMenu->addAction(ui->actionCenter);
    ContextMenu->addAction(ui->actionRight);
    ContextMenu->addSeparator();
    ContextMenu->addAction(ui->actionDistributeHoriz);
    ContextMenu->addAction(ui->actionDistributeVert);
    ContextMenu->addSeparator();
    ContextMenu->addAction(ui->actionUpBlock);
    ContextMenu->addAction(ui->actionDownBlock);
    ContextMenu->exec(QCursor::pos());
    delete ContextMenu;
    ui->ArrangeBT->setDown(false);
}

//====================================================================================================================

void DlgSlideProperties::s_BlockSettings_Edit() {
    QMenu *ContextMenu=new QMenu(this);
    ContextMenu->addAction(ui->actionCut);
    ContextMenu->addAction(ui->actionCopy);
    ContextMenu->addAction(ui->actionPaste);
    ContextMenu->addSeparator();
    ContextMenu->addAction(ui->actionEditText);
    if (ui->actionEditImage->isEnabled()) ContextMenu->addAction(ui->actionEditImage);
    ContextMenu->addSeparator();
    ContextMenu->addAction(ui->actionRemoveBlock);
    if (NbrSelected==1) {   // Single selection
        ContextMenu->addSeparator();
        if (CurrentCompoObject->isVisible()) ContextMenu->addAction(ui->actionSetHide);
            else ContextMenu->addAction(ui->actionSetVisible);
        if (ISVIDEO(CurrentCompoObject->BackgroundBrush)) ContextMenu->addAction(ui->actionTakeSound);
    }
    ContextMenu->exec(QCursor::pos());
    delete ContextMenu;
    ui->EditBT->setDown(false);
}

//====================================================================================================================
// User right click on a block in the BlockTable widget or in the scene

void DlgSlideProperties::s_BlockTable_ItemRightClicked(QMouseEvent *) {
    if (BlockSelectMode==SELECTMODE_NONE) {
        QMenu *ContextMenu=new QMenu(this);
        ContextMenu->addAction(ui->actionCut);
        ContextMenu->addAction(ui->actionCopy);
        ContextMenu->addAction(ui->actionPaste);
        ContextMenu->addSeparator();
        ContextMenu->addAction(ui->actionAddTextBlock);
        ContextMenu->addAction(ui->actionAddFile);
        ContextMenu->addAction(ui->actionAddImageClipboard);
        ContextMenu->addAction(ui->actionAddGMapsMap);
        ContextMenu->exec(QCursor::pos());
        delete ContextMenu;
    } else if (BlockSelectMode==SELECTMODE_ONE) {
        QMenu *ContextMenu=new QMenu(this);
        ContextMenu->addAction(ui->actionCut);
        ContextMenu->addAction(ui->actionCopy);
        ContextMenu->addAction(ui->actionPaste);
        ContextMenu->addSeparator();
        ContextMenu->addAction(ui->actionEditText);
        if (ui->actionEditImage->isEnabled()) ContextMenu->addAction(ui->actionEditImage);
        ContextMenu->addSeparator();
        ContextMenu->addAction(ui->actionRemoveBlock);
        ContextMenu->addSeparator();
        if (CurrentCompoObject->isVisible()) ContextMenu->addAction(ui->actionSetHide);
            else ContextMenu->addAction(ui->actionSetVisible);
        if (ISVIDEO(CurrentCompoObject->BackgroundBrush)) ContextMenu->addAction(ui->actionTakeSound);
        ContextMenu->addSeparator();
        ContextMenu->addAction(ui->actionInfo);
        ContextMenu->addSeparator();
        ContextMenu->addAction(ui->actionUpBlock);
        ContextMenu->addAction(ui->actionDownBlock);
        ContextMenu->exec(QCursor::pos());
        delete ContextMenu;
    } else if (BlockSelectMode==SELECTMODE_MULTIPLE) {
        QMenu *ContextMenu=new QMenu(this);
        ContextMenu->addAction(ui->actionTop);
        ContextMenu->addAction(ui->actionMiddle);
        ContextMenu->addAction(ui->actionBottom);
        ContextMenu->addSeparator();
        ContextMenu->addAction(ui->actionLeft);
        ContextMenu->addAction(ui->actionCenter);
        ContextMenu->addAction(ui->actionRight);
        ContextMenu->addSeparator();
        ContextMenu->addAction(ui->actionDistributeHoriz);
        ContextMenu->addAction(ui->actionDistributeVert);
        ContextMenu->addSeparator();
        ContextMenu->addAction(ui->actionCut);
        ContextMenu->addAction(ui->actionCopy);
        ContextMenu->addAction(ui->actionPaste);
        ContextMenu->addSeparator();
        ContextMenu->addAction(ui->actionRemoveBlock);
        ContextMenu->exec(QCursor::pos());
        delete ContextMenu;
    }
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_AddNewTextBlock() {
    QMenu *ContextMenu=new QMenu(this);
    ContextMenu->addAction(ui->actionAddSimpleTextBlock);
    ContextMenu->addAction(ui->actionAddClipArtTextBlock);
    ContextMenu->exec(QCursor::pos());
    delete ContextMenu;
    ui->AddTextBlock->setDown(false);
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_AddNewSimpleTextBlock() {
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,BlockTable,true);

    int CurrentShotNbr=ShotTable->currentColumn();

    // Create and append a composition block to the object list
    CurrentSlide->ObjectComposition.compositionObjects.append(new cCompositionObject(eCOMPOSITIONTYPE_OBJECT,CurrentSlide->NextIndexKey,ApplicationConfig,&CurrentSlide->ObjectComposition));
    cCompositionObject *CompositionObject=CurrentSlide->ObjectComposition.compositionObjects[CurrentSlide->ObjectComposition.compositionObjects.count()-1];

    // Apply Styles
    CompositionObject->ApplyTextStyle(ApplicationConfig->StyleTextCollection.GetStyleDef(ApplicationConfig->StyleTextCollection.DecodeString(ApplicationConfig->DefaultBlock_Text_TextST)));
    CompositionObject->ApplyBackgroundStyle(ApplicationConfig->StyleTextBackgroundCollection.GetStyleDef(ApplicationConfig->StyleTextBackgroundCollection.DecodeString(ApplicationConfig->DefaultBlock_Text_BackGST)));
    CompositionObject->ApplyBlockShapeStyle(ApplicationConfig->StyleBlockShapeCollection.GetStyleDef(ApplicationConfig->StyleBlockShapeCollection.DecodeString(ApplicationConfig->DefaultBlock_Text_ShapeST)));
    CompositionObject->BackgroundBrush->setLockGeometry(false); // For ApplyAutoCompoSize don't use it
    CompositionObject->ApplyAutoCompoSize(ApplicationConfig->DefaultBlock_AutoSizePos,CurrentSlide->pDiaporama->ImageGeometry);
    CompositionObject->BackgroundBrush->setLockGeometry((ApplicationConfig->DefaultBlock_AutoLocking==AUTOFRAMING_CUSTOMPRJLOCK));
    CompositionObject->BackgroundBrush->setAspectRatio((CompositionObject->height()*(CurrentSlide->pDiaporama->ImageGeometry==GEOMETRY_4_3?1440:CurrentSlide->pDiaporama->ImageGeometry==GEOMETRY_16_9?1080:CurrentSlide->pDiaporama->ImageGeometry==GEOMETRY_40_17?816:1920))/(CompositionObject->width()*1920));

    // Create default text
    QTextDocument       TextDoc(QApplication::translate("DlgSlideProperties","Text","Default text value"));
    QFont               Font = CompositionObject->tParams.font();
    QTextOption         OptionText(CompositionObject->tParams.alignment());
    QTextCursor         Cursor(&TextDoc);
    QTextCharFormat     TCF;
    QTextBlockFormat    TBF;
    Cursor.select(QTextCursor::Document);
    OptionText.setWrapMode(QTextOption::WordWrap);
    Font.setUnderline(CompositionObject->tParams.IsUnderline);
    TextDoc.setDefaultFont(Font);
    TextDoc.setDefaultTextOption(OptionText);
    TCF.setFont(Font);
    TCF.setFontWeight(CompositionObject->tParams.IsBold?QFont::Bold:QFont::Normal);
    TCF.setFontItalic(CompositionObject->tParams.IsItalic);
    TCF.setFontUnderline(CompositionObject->tParams.IsUnderline);
    TCF.setForeground(QBrush(QColor(CompositionObject->tParams.FontColor)));
    TBF.setAlignment(CompositionObject->tParams.alignment());
    Cursor.setCharFormat(TCF);
    Cursor.setBlockFormat(TBF);
    CompositionObject->tParams.Text = TextDoc.toHtml();

    // Now create and append a shot composition block to all shot
    for (int i = 0; i<CurrentSlide->shotList.count(); i++)
    {
       CurrentSlide->shotList[i]->ShotComposition.compositionObjects.append(new cCompositionObject(eCOMPOSITIONTYPE_SHOT, CompositionObject->index(), ApplicationConfig, &CurrentSlide->shotList[i]->ShotComposition));
       CurrentSlide->shotList[i]->ShotComposition.compositionObjects[CurrentSlide->shotList[i]->ShotComposition.compositionObjects.count() - 1]->CopyFromCompositionObject(CompositionObject);
        // Ensure new object is not visible in previous shot
       if (i<CurrentShotNbr) CurrentSlide->shotList[i]->ShotComposition.compositionObjects[CurrentSlide->shotList[i]->ShotComposition.compositionObjects.count() - 1]->setVisible(false);
    }

    // Inc NextIndexKey
    CurrentSlide->NextIndexKey++;

    // Reset thumbs if needed
    ResetThumbs(true);
    // Reset blocks table
    RefreshBlockTable(CompositionList->compositionObjects.count()-1);
    NoPrepUndo=true;
    QTimer::singleShot(250,this,SLOT(s_BlockSettings_TextEditor()));    // Append "Open text editor" to the message queue
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_AddNewClipArtTextBlock() {
    cPopupTexteFrame* popup1 = new cPopupTexteFrame(this);
    QPoint Pos=mapToGlobal(ui->AddTextBlock->pos());
    popup1->DoInitPopup(QPoint(Pos.x()+ui->AddTextBlock->width(),Pos.y()+ui->AddTextBlock->height()));
    popup1->exec();
    QString RessourceName=popup1->GetCurrentTextFrame();
    if (RessourceName=="") return;

    AppendPartialUndo(UNDOACTION_FULL_SLIDE,BlockTable,true);
    int CurrentShotNbr=ShotTable->currentColumn();

    // Create and append a composition block to the object list
    CurrentSlide->ObjectComposition.compositionObjects.append(new cCompositionObject(eCOMPOSITIONTYPE_OBJECT,CurrentSlide->NextIndexKey,ApplicationConfig,&CurrentSlide->ObjectComposition));
    cCompositionObject *CompositionObject=CurrentSlide->ObjectComposition.compositionObjects[CurrentSlide->ObjectComposition.compositionObjects.count()-1];

    // Apply Styles
    CompositionObject->ApplyTextStyle(TextFrameList.List[TextFrameList.SearchImage(RessourceName)].TextStyle);
    CompositionObject->BackgroundBrush->setLockGeometry(false); // For ApplyAutoCompoSize don't use it
    CompositionObject->ApplyAutoCompoSize(ApplicationConfig->DefaultBlock_AutoSizePos,CurrentSlide->pDiaporama->ImageGeometry);
    CompositionObject->BackgroundBrush->setLockGeometry((ApplicationConfig->DefaultBlock_AutoLocking==AUTOFRAMING_CUSTOMPRJLOCK));
    CompositionObject->BackgroundBrush->setAspectRatio((CompositionObject->height()*(CurrentSlide->pDiaporama->ImageGeometry==GEOMETRY_4_3?1440:CurrentSlide->pDiaporama->ImageGeometry==GEOMETRY_16_9?1080:CurrentSlide->pDiaporama->ImageGeometry==GEOMETRY_40_17?816:1920))/(CompositionObject->width()*1920));
    CompositionObject->tParams.TextClipArtName = RessourceName;
    CompositionObject->tParams.textMargins = TextFrameList.List[TextFrameList.SearchImage(RessourceName)].MarginRect();
    //CompositionObject->tParams.TMx                          =TextFrameList.List[TextFrameList.SearchImage(RessourceName)].TMx;
    //CompositionObject->tParams.TMy                          =TextFrameList.List[TextFrameList.SearchImage(RessourceName)].TMy;
    //CompositionObject->tParams.TMw                          =TextFrameList.List[TextFrameList.SearchImage(RessourceName)].TMw;
    //CompositionObject->tParams.TMh                          =TextFrameList.List[TextFrameList.SearchImage(RessourceName)].TMh;

    // Create default text
    QTextDocument       TextDoc(QApplication::translate("DlgSlideProperties","Text","Default text value"));
    QFont               Font = CompositionObject->tParams.font();
    QTextOption         OptionText(CompositionObject->tParams.alignment());
    QTextCursor         Cursor(&TextDoc);
    QTextCharFormat     TCF;
    QTextBlockFormat    TBF;
    Cursor.select(QTextCursor::Document);
    OptionText.setWrapMode(QTextOption::WordWrap);
    Font.setUnderline(CompositionObject->tParams.IsUnderline);
    TextDoc.setDefaultFont(Font);
    TextDoc.setDefaultTextOption(OptionText);
    TCF.setFont(Font);
    TCF.setFontWeight(CompositionObject->tParams.IsBold ? QFont::Bold : QFont::Normal);
    TCF.setFontItalic(CompositionObject->tParams.IsItalic);
    TCF.setFontUnderline(CompositionObject->tParams.IsUnderline);
    TCF.setForeground(QBrush(QColor(CompositionObject->tParams.FontColor)));
    TBF.setAlignment(CompositionObject->tParams.alignment());
    Cursor.setCharFormat(TCF);
    Cursor.setBlockFormat(TBF);
    CompositionObject->tParams.Text = TextDoc.toHtml();
    CompositionObject->resolvedText.clear();

    // Now create and append a shot composition block to all shot
    for (int i = 0; i<CurrentSlide->shotList.count(); i++)
    {
       CurrentSlide->shotList[i]->ShotComposition.compositionObjects.append(new cCompositionObject(eCOMPOSITIONTYPE_SHOT, CompositionObject->index(), ApplicationConfig, &CurrentSlide->shotList[i]->ShotComposition));
       CurrentSlide->shotList[i]->ShotComposition.compositionObjects[CurrentSlide->shotList[i]->ShotComposition.compositionObjects.count() - 1]->CopyFromCompositionObject(CompositionObject);
        // Ensure new object is not visible in previous shot
       if (i<CurrentShotNbr) CurrentSlide->shotList[i]->ShotComposition.compositionObjects[CurrentSlide->shotList[i]->ShotComposition.compositionObjects.count() - 1]->setVisible(false);
    }

    // Inc NextIndexKey
    CurrentSlide->NextIndexKey++;

    // Reset thumbs if needed
    ResetThumbs(true);
    // Reset blocks table
    RefreshBlockTable(CompositionList->compositionObjects.count()-1);
    NoPrepUndo=true;
    QTimer::singleShot(250,this,SLOT(s_BlockSettings_TextEditor()));    // Append "Open text editor" to the message queue
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_DragDropFiles(QList<QUrl> urlList) {
    if (urlList.count()>0) {
        QString     fName;
        QFileInfo   info;
        FileList.clear();
        for (int i=0;i<urlList.count();i++) {
            fName = urlList[i].toLocalFile();           // convert first QUrl to local path
            info.setFile(fName);                        // information about file
            if (info.isFile()) FileList.append(fName);  // append file
        }
        if (FileList.count()>0) {
            if (BlockTable->DragItemDest<0)                      BlockTable->DragItemDest=0;
            if (BlockTable->DragItemDest>BlockTable->rowCount()) BlockTable->DragItemDest=BlockTable->rowCount();
            PositionToInsert=BlockTable->DragItemDest;
            if (DlgWorkingTaskDialog) {
                DlgWorkingTaskDialog->close();
                delete DlgWorkingTaskDialog;
                DlgWorkingTaskDialog=NULL;
            }
            CancelAction=false;
            InteractiveZone->setUpdatesEnabled(false);
            DlgWorkingTaskDialog=new DlgWorkingTask(QApplication::translate("DlgSlideProperties","Add files"),&CancelAction,ApplicationConfig,this);
            DlgWorkingTaskDialog->InitDialog();
            DlgWorkingTaskDialog->SetMaxValue(FileList.count(),0);
            QTimer::singleShot(LATENCY,this,SLOT(s_BlockTable_AddFilesBlock()));
            DlgWorkingTaskDialog->exec();
        }
    }
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_AddNewFileBlock() {
    ui->AddFileBlock->setDown(false);
    FileList.clear();
    DlgFileExplorer Dlg(BROWSER_TYPE_MEDIAFILES,true,false,true,QApplication::translate("DlgSlideProperties","Add files"),ApplicationConfig,this);
    Dlg.InitDialog();
    if (Dlg.exec()==0) FileList=Dlg.GetCurrentSelectedFiles();
    if (FileList.count()==0) return;
    QApplication::processEvents();
    PositionToInsert=BlockTable->rowCount();
    if (DlgWorkingTaskDialog) {
        DlgWorkingTaskDialog->close();
        delete DlgWorkingTaskDialog;
        DlgWorkingTaskDialog=NULL;
    }
    CancelAction=false;
    InteractiveZone->setUpdatesEnabled(false);
    DlgWorkingTaskDialog=new DlgWorkingTask(QApplication::translate("DlgSlideProperties","Add files"),&CancelAction,ApplicationConfig,this);
    DlgWorkingTaskDialog->InitDialog();
    DlgWorkingTaskDialog->SetMaxValue(FileList.count(),0);
    QTimer::singleShot(LATENCY,this,SLOT(s_BlockTable_AddFilesBlock()));
    DlgWorkingTaskDialog->exec();
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_AddFilesBlock() 
{
    if (FileList.count()==0 || CancelAction) 
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        if (DlgWorkingTaskDialog) 
        {
            DlgWorkingTaskDialog->close();
            delete DlgWorkingTaskDialog;
            DlgWorkingTaskDialog=NULL;
        }
        FileList.clear();
        // Reset thumbs if needed
        ResetThumbs(true);
        // Reset blocks table
        RefreshBlockTable(PositionToInsert-1);
        InteractiveZone->setUpdatesEnabled(true);
        QApplication::restoreOverrideCursor();
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Add files
    QString         FileName     = FileList.takeFirst();
    QString         BrushFileName = QFileInfo(FileName).absoluteFilePath();
    QString         Extension    = QFileInfo(BrushFileName).suffix().toLower();
    cBaseMediaFile  *MediaObject = NULL;

    DlgWorkingTaskDialog->DisplayText(QApplication::translate("MainWindow","Add file to project :")+QFileInfo(FileName).fileName());
    DlgWorkingTaskDialog->DisplayProgress(DlgWorkingTaskDialog->MaxValue+DlgWorkingTaskDialog->AddValue-FileList.count());

    if (ApplicationConfig->AllowImageExtension.contains(Extension))             MediaObject=new cImageFile(ApplicationConfig);
    else if (ApplicationConfig->AllowImageVectorExtension.contains(Extension))  MediaObject=new cImageFile(ApplicationConfig);
    else if (ApplicationConfig->AllowVideoExtension.contains(Extension))        MediaObject=new cVideoFile(ApplicationConfig);

    if (MediaObject && MediaObject->GetInformationFromFile(BrushFileName,NULL,NULL,-1) && MediaObject->CheckFormatValide(this)) 
    {
        if (MediaObject->is_Videofile()) 
        {
            // Do analyse
            DlgWorkingTaskDialog->DisplayText2(QApplication::translate("MainWindow","Analyse file:"));
            QList<qreal> Peak,Moyenne;
            DlgWorkingTaskDialog->TimerProgress=0;
            if (!((cVideoFile *)MediaObject)->IsComputedAudioDuration) ((cVideoFile *)MediaObject)->DoAnalyseSound(&Peak,&Moyenne,&CancelAction,&DlgWorkingTaskDialog->TimerProgress);
            DlgWorkingTaskDialog->StopText2();
            // Prepare default values
            ((cVideoFile *)MediaObject)->EndTime = ((cVideoFile *)MediaObject)->GetRealDuration();
            if (((cVideoFile *)MediaObject)->LibavStartTime > 0) 
               ((cVideoFile *)MediaObject)->StartTime = QTime(0,0,0,0).addMSecs(int64_t((double(((cVideoFile *)MediaObject)->LibavStartTime)/AV_TIME_BASE)*1000));
        }
        DoAddBlock(MediaObject,PositionToInsert);
        PositionToInsert++;
    } 
    else 
    {
        delete CurrentSlide->ObjectComposition.compositionObjects.takeAt(PositionToInsert);
        CustomMessageBox(this,QMessageBox::Critical,QApplication::translate("MainWindow","Error","Error message"),FileName+"\n\n"+QApplication::translate("MainWindow","Format not supported","Error message"),QMessageBox::Close);
    }
    QApplication::restoreOverrideCursor();
    QTimer::singleShot(LATENCY,this,SLOT(s_BlockTable_AddFilesBlock()));
}

//====================================================================================================================

void DlgSlideProperties::DoAddBlock(cBaseMediaFile *MediaObject, int PositionToInsert)
{
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, BlockTable, true);

   CurrentSlide->ObjectComposition.compositionObjects.insert(PositionToInsert,new cCompositionObject(eCOMPOSITIONTYPE_OBJECT,CurrentSlide->NextIndexKey,ApplicationConfig,&CurrentSlide->ObjectComposition));
   cCompositionObject  *CompositionObject=CurrentSlide->ObjectComposition.compositionObjects[PositionToInsert];
   cBrushDefinition    *CurrentBrush     =CompositionObject->BackgroundBrush;
   if (MediaObject->is_Videofile()) 
   {
      QString Extension = QFileInfo(MediaObject->FileName()).completeSuffix().toLower();
      CurrentBrush->Deinterlace = (ApplicationConfig->Deinterlace) && ((Extension == "mts") || (Extension == "m2ts") || (Extension == "mod") || (Extension == "ts"));
   }
   CompositionObject->tParams.Text = "";
   CompositionObject->resolvedText.clear();
   CompositionObject->PenSize = 0;
   CurrentBrush->BrushType = BRUSHTYPE_IMAGEDISK;

   // Create an cImageClipboard wrapper
   CurrentBrush->MediaObject = MediaObject;

   // Apply Styles
   CompositionObject->ApplyTextStyle(ApplicationConfig->StyleTextCollection.GetStyleDef(ApplicationConfig->StyleTextCollection.DecodeString(ApplicationConfig->DefaultBlockBA_IMG_TextST)));
   CompositionObject->ApplyBlockShapeStyle(ApplicationConfig->StyleBlockShapeCollection.GetStyleDef(ApplicationConfig->StyleBlockShapeCollection.DecodeString(ApplicationConfig->DefaultBlockBA_IMG_ShapeST)));

   // Apply styles for coordinates
   CurrentBrush->ApplyAutoFraming(ApplicationConfig->DefaultBlockBA[CurrentBrush->GetImageType()].AutoFraming, ProjectGeometry);

   if (MediaObject->is_Imagevector())
   {
      CompositionObject->ApplyAutoCompoSize(AUTOCOMPOSIZE_REALSIZE, CurrentSlide->pDiaporama->ImageGeometry);
      // adjust for image was not too small !
      if ((CompositionObject->width() < 0.2) && (CompositionObject->height() < 0.2))
      {
         while ((CompositionObject->width() < 0.2) && (CompositionObject->height() < 0.2))
         {
            CompositionObject->setWidth(CompositionObject->width() * 2);
            CompositionObject->setHeight(CompositionObject->height() * 2);
         }
         CompositionObject->setPosition((1 - CompositionObject->width()) / 2, (1 - CompositionObject->height()) / 2);
      }
   }
   else
      CompositionObject->ApplyAutoCompoSize(ApplicationConfig->DefaultBlockBA[CurrentBrush->GetImageType()].AutoCompo, CurrentSlide->pDiaporama->ImageGeometry);

   // Now create and append a shot composition block to all shot
   for (int i = 0; i < CurrentSlide->shotList.count(); i++)
   {
      CurrentSlide->shotList[i]->ShotComposition.compositionObjects.insert(PositionToInsert, new cCompositionObject(eCOMPOSITIONTYPE_SHOT, CompositionObject->index(), ApplicationConfig, &CurrentSlide->shotList[i]->ShotComposition));
      CurrentSlide->shotList[i]->ShotComposition.compositionObjects[PositionToInsert]->CopyFromCompositionObject(CompositionObject);
      // Ensure new object is not visible in previous shot
      if (i < CurrentShotNbr) 
         CurrentSlide->shotList[i]->ShotComposition.compositionObjects[PositionToInsert]->setVisible(false);
   }

   // If this object is a video will gain sound from this shots !
   if (MediaObject->is_Videofile())
   {
      for (int k = 0; k < CurrentSlide->shotList.count(); k++)
      {
         for (int l = 0; l < CurrentSlide->shotList[k]->ShotComposition.compositionObjects.count(); l++)
         {
            if (ISVIDEO(CurrentSlide->shotList[k]->ShotComposition.compositionObjects[l]->BackgroundBrush))
            {
               if (k < CurrentShotNbr)
               {
                  // Set this new block to SoundVolume=0
                  if (CurrentBrush->MediaObject->FileKey() == CurrentSlide->shotList[k]->ShotComposition.compositionObjects[l]->BackgroundBrush->MediaObject->FileKey())
                     CurrentSlide->shotList[k]->ShotComposition.compositionObjects[l]->BackgroundBrush->SoundVolume = 0;
               }
               else
               {
                  // Set all other block to SoundVolume=0 and this block to SoundVolume=-1 (auto)
                  if (CurrentBrush->MediaObject->FileKey() != CurrentSlide->shotList[k]->ShotComposition.compositionObjects[l]->BackgroundBrush->MediaObject->FileKey())
                     CurrentSlide->shotList[k]->ShotComposition.compositionObjects[l]->BackgroundBrush->SoundVolume = 0;
                  else 
                     CurrentSlide->shotList[k]->ShotComposition.compositionObjects[l]->BackgroundBrush->SoundVolume = -1;
               }
            }
         }
      }
   }
   // Inc NextIndexKey
   CurrentSlide->NextIndexKey++;
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_AddSpecialBlock() 
{
    QMenu *ContextMenu=new QMenu(this);
    ContextMenu->addAction(ui->actionAddImageClipboard);
    ContextMenu->addAction(ui->actionAddGMapsMap);
    ContextMenu->exec(QCursor::pos());
    delete ContextMenu;
    ui->AddSpecialBlock->setDown(false);
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_AddGMapsMapBlock() 
{
    // Create an GMapsMap wrapper
    cGMapsMap   *MediaObject=new cGMapsMap(ApplicationConfig);
    //MediaObject->CreatDateTime=QDateTime().currentDateTime();
    MediaObject->CreateDefaultImage(CurrentSlide->pDiaporama);   // create default image
    QString s("");
    MediaObject->GetInformationFromFile(s,NULL,NULL,-1);
    DoAddBlock(MediaObject,CurrentSlide->ObjectComposition.compositionObjects.count());
    // Reset thumbs if needed
    ResetThumbs(true);
    // Reset blocks table
    RefreshBlockTable(CurrentSlide->ObjectComposition.compositionObjects.count()-1);
    RedoneStyleCoordinate=true;
    QTimer::singleShot(250,this,SLOT(s_BlockSettings_ImageEditCorrect()));    // Append "Open image editor" to the message queue
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_AddImageClipboard() 
{
    const QMimeData *SlideData=QApplication::clipboard()->mimeData();
    if (!SlideData->hasImage()) return;
    QImage ImageClipboard=qvariant_cast<QImage>(SlideData->imageData());
    if (ImageClipboard.isNull()) return;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    cImageClipboard *MediaObject=new cImageClipboard(ApplicationConfig);
    //MediaObject->CreatDateTime=QDateTime().currentDateTime();
    MediaObject->setSlideThumb(ImageClipboard);
    //ApplicationConfig->SlideThumbsTable->SetThumbs(&MediaObject->RessourceKey,ImageClipboard);
    QString s("");
    MediaObject->GetInformationFromFile(s,NULL,NULL,-1);
    DoAddBlock(MediaObject,BlockTable->rowCount());
    // Reset thumbs if needed
    ResetThumbs(true);
    // Reset blocks table
    RefreshBlockTable(BlockTable->rowCount()-1);
    QApplication::restoreOverrideCursor();
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_Cut() 
{
    if ((BlockSelectMode!=SELECTMODE_ONE)&&(BlockSelectMode!=SELECTMODE_MULTIPLE)) return;
    s_BlockTable_Copy();
    s_BlockTable_RemoveBlock();
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_Copy() 
{
   if (BlockSelectMode != SELECTMODE_ONE  && BlockSelectMode != SELECTMODE_MULTIPLE) 
      return;

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   // Create xml document and root
   QDomDocument Object = QDomDocument(APPLICATION_NAME);
   QDomElement  root = Object.createElement("CLIPBOARD");
   int BlockNum = 0;
   for (int i = 0; i < CompositionList->compositionObjects.count(); i++) 
   {
      if (IsSelected[i]) 
      {
         QDomElement Element = Object.createElement(QString("Block-%1").arg(BlockNum));
         cCompositionObject  *GlobalBlock = GetGlobalCompositionObject(CompositionList->compositionObjects[i]->index());
         GlobalBlock->SaveToXML(Element,"CLIPBOARD-BLOCK-GLOBAL",CurrentSlide->pDiaporama->ProjectFileName,true,true,NULL,NULL,true,false);                // Save global object
         CompositionList->compositionObjects[i]->SaveToXML(Element,"CLIPBOARD-BLOCK-SHOT",CurrentSlide->pDiaporama->ProjectFileName,true,true,NULL,NULL,true,false);     // Save shot object

         // Add shot part to XML
         if ((CompositionList->compositionObjects[i]->BackgroundBrush)&&(Element.elementsByTagName("CLIPBOARD-BLOCK-SHOT").length() > 0) 
            && (Element.elementsByTagName("CLIPBOARD-BLOCK-SHOT").item(0).isElement()==true)) 
         {
            QDomElement CBS = Element.elementsByTagName("CLIPBOARD-BLOCK-SHOT").item(0).toElement();
            if ((CBS.elementsByTagName("BackgroundBrush").length() > 0) && (CBS.elementsByTagName("BackgroundBrush").item(0).isElement()==true)) 
            {
               QDomElement CBB = CBS.elementsByTagName("BackgroundBrush").item(0).toElement();
               CompositionList->compositionObjects[i]->BackgroundBrush->AddShotPartToXML(&CBB);
            }
         }
         root.appendChild(Element);
         BlockNum++;
      }
   }
   root.setAttribute("BlockNbr",BlockNum);
   Object.appendChild(root);

   // Transfert xml document to clipboard
   QMimeData *SlideData = new QMimeData();
   SlideData->setData("ffDiaporama/block",Object.toByteArray());
   QApplication::clipboard()->setMimeData(SlideData);

   RefreshControls();
   QApplication::restoreOverrideCursor();
}

//====================================================================================================================

void DlgSlideProperties::s_BlockTable_Paste() 
{
   const QMimeData *SlideData = QApplication::clipboard()->mimeData();
   if (SlideData->hasImage()) 
      s_BlockTable_AddImageClipboard(); 
   else if (SlideData->hasFormat("ffDiaporama/block")) 
   {
      AppendPartialUndo(UNDOACTION_FULL_SLIDE,BlockTable,true);
      QDomDocument Object = QDomDocument(APPLICATION_NAME);
      Object.setContent(SlideData->data("ffDiaporama/block"));
      if ((Object.elementsByTagName("CLIPBOARD").length() > 0) && (Object.elementsByTagName("CLIPBOARD").item(0).isElement() == true)) 
      {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

         QDomElement root = Object.elementsByTagName("CLIPBOARD").item(0).toElement();
         int BlockNbr = root.attribute("BlockNbr").toInt();

         for (int BlockNum = 0; BlockNum < BlockNbr; BlockNum++) 
            if ((root.elementsByTagName(QString("Block-%1").arg(BlockNum)).length() > 0) && (root.elementsByTagName(QString("Block-%1").arg(BlockNum)).item(0).isElement() == true)) 
            {
               QDomElement Element = root.elementsByTagName(QString("Block-%1").arg(BlockNum)).item(0).toElement();
               // Create and append a composition block to the object list
               CurrentSlide->ObjectComposition.compositionObjects.append(new cCompositionObject(eCOMPOSITIONTYPE_OBJECT,CurrentSlide->NextIndexKey,ApplicationConfig,&CurrentSlide->ObjectComposition));
               cCompositionObject *GlobalBlock = CurrentSlide->ObjectComposition.compositionObjects[CurrentSlide->ObjectComposition.compositionObjects.count()-1];
               GlobalBlock->LoadFromXML(Element,"CLIPBOARD-BLOCK-GLOBAL","",NULL,NULL,true,NULL,true,true);
               GlobalBlock->setIndex(CurrentSlide->NextIndexKey);

               cCompositionObject ShotBlock(eCOMPOSITIONTYPE_SHOT,CurrentSlide->NextIndexKey,ApplicationConfig,this);
               ShotBlock.setIndex(CurrentSlide->NextIndexKey);
               ShotBlock.LoadFromXML(Element,"CLIPBOARD-BLOCK-SHOT","",&CurrentSlide->ObjectComposition,NULL,true,NULL,false,true,GlobalBlock);    // No duplicate ressource on paste
               ShotBlock.BackgroundBrush->MediaObject=GlobalBlock->BackgroundBrush->MediaObject;
               ShotBlock.BackgroundBrush->DeleteMediaObject=false;
               ShotBlock.tParams.Text = GlobalBlock->tParams.Text;
               if (ShotBlock.tParams.Text != "") 
               {
                  ShotBlock.tParams = GlobalBlock->tParams;
                  ShotBlock.resolvedText.clear();
               }
               // Now create and append a shot composition block to all shot
               for (int i = 0; i<CurrentSlide->shotList.count(); i++)
               {
                  CurrentSlide->shotList[i]->ShotComposition.compositionObjects.append(new cCompositionObject(eCOMPOSITIONTYPE_SHOT, CurrentSlide->NextIndexKey, ApplicationConfig, &CurrentSlide->shotList[i]->ShotComposition));
                  CurrentSlide->shotList[i]->ShotComposition.compositionObjects[CurrentSlide->shotList[i]->ShotComposition.compositionObjects.count() - 1]->CopyFromCompositionObject(&ShotBlock);

                  // Copy markers (if exist)
                  for (int j=0;j<ShotBlock.BackgroundBrush->Markers.count();j++)
                     CurrentSlide->shotList[i]->ShotComposition.compositionObjects[CurrentSlide->shotList[i]->ShotComposition.compositionObjects.count() - 1]->BackgroundBrush->Markers.append(ShotBlock.BackgroundBrush->Markers[j]);

                  // Ensure new object is not visible in previous shot
                  if (i < CurrentShotNbr) 
                  {
                     CurrentSlide->shotList[i]->ShotComposition.compositionObjects[CurrentSlide->shotList[i]->ShotComposition.compositionObjects.count() - 1]->setVisible(false);
                     // Ensure unvisible video have no sound !
                     if (ISVIDEO(CurrentSlide->shotList[i]->ShotComposition.compositionObjects[CurrentSlide->shotList[i]->ShotComposition.compositionObjects.count() - 1]->BackgroundBrush))
                        CurrentSlide->shotList[i]->ShotComposition.compositionObjects[CurrentSlide->shotList[i]->ShotComposition.compositionObjects.count() - 1]->BackgroundBrush->SoundVolume = 0;
                  }
               }
               // Inc NextIndexKey
               CurrentSlide->NextIndexKey++;
            }

            // Reset thumbs if needed
            ResetThumbs(true);
            // Reset blocks table
            RefreshBlockTable(CompositionList->compositionObjects.count()-1);

            // Select blocks we just added
            s_BlockTable_StartSelectionChange();
            BlockTable->clearSelection();
            for (int i = 0; i < BlockNbr; i++) 
               BlockTable->setCurrentCell(CompositionList->compositionObjects.count()-1-i,0,i==0?QItemSelectionModel::Select|QItemSelectionModel::Current:QItemSelectionModel::Select);
            s_BlockTable_EndSelectionChange();

            QApplication::restoreOverrideCursor();
      }
   }
}

//********************************************************************************************************************
//
//                                                  CURRENT BLOCK SETTINGS
//
//********************************************************************************************************************

//====================================================================================================================
// Buttons associated to a Dialog box
//====================================================================================================================

//========= Open image/video correction editor
void DlgSlideProperties::s_BlockSettings_ImageEditCorrect() 
{
   if (InRefreshControls || BlockSelectMode != SELECTMODE_ONE || !CurrentCompoObject || !CurrentCompoObject->isVisible() || !ui->actionEditImage->isEnabled())
      return;

   AppendPartialUndo(UNDOACTION_FULL_SLIDE,InteractiveZone,true);
   cBrushDefinition *CurrentBrush = CurrentCompoObject->BackgroundBrush;

   int Position = 0;
   // Compute position of video
   if (CurrentBrush->isVideo()) 
   {
      Position = QTime(0,0,0,0).msecsTo(((cVideoFile*)&CurrentBrush->MediaObject)->StartTime);
      for (int i = 0; i<ShotTable->currentColumn(); i++) 
         for (int j = 0; j < CurrentSlide->shotList[i]->ShotComposition.compositionObjects.count(); j++)
            if ((CurrentSlide->shotList[i]->ShotComposition.compositionObjects[j]->index() == CurrentCompoObject->index()) &&
               (CurrentSlide->shotList[i]->ShotComposition.compositionObjects[j]->isVisible())
               ) 
               Position += CurrentSlide->shotList[i]->StaticDuration;
   }

   QString FileName = CurrentBrush->MediaObject->ShortName();
   bool UpdateSlideName = (CurrentSlide->SlideName == FileName);

   cBrushDefinition SavedBrush(CurrentCompoObject,ApplicationConfig);
   SavedBrush.CopyFromBrushDefinition(CurrentCompoObject->BackgroundBrush);

   DlgImageCorrection Dlg(CurrentCompoObject,&CurrentCompoObject->BackgroundForm,CurrentCompoObject->BackgroundBrush,Position,
      CurrentSlide->pDiaporama->ImageGeometry,CurrentSlide->pDiaporama->ImageAnimSpeedWave,ApplicationConfig,this);
   Dlg.InitDialog();
   if (Dlg.exec() == 0) 
   {
      // Redone application of styles for coordinates (specific to GMaps object when creating)
      if (RedoneStyleCoordinate) 
      {
         CurrentCompoObject->ApplyAutoCompoSize(ApplicationConfig->DefaultBlockBA[CurrentCompoObject->BackgroundBrush->GetImageType()].AutoCompo,CurrentSlide->pDiaporama->ImageGeometry);
         RedoneStyleCoordinate = false;
      }

      FramingCB_CurrentBrush = NULL; // To force a refresh of ui->FramingCB !
      // ??? CurrentBrush->AspectRatio=CurrentBrush->AspectRatio;
      CurrentCompoObject->setHeight((CurrentCompoObject->width()*InteractiveZone->DisplayW*CurrentBrush->AspectRatio())/InteractiveZone->DisplayH);
      if (CurrentShotNbr == 0 && CurrentSlide->ThumbnailKey != -1)
          ApplicationConfig->SlideThumbsTable->ClearThumb(CurrentSlide->ThumbnailKey);

      // Adjust height and width to image stay in screen
      if (((CurrentCompoObject->pos().y() + CurrentCompoObject->height()) * InteractiveZone->DisplayH) > InteractiveZone->DisplayH) 
      {
         //CurrentCompoObject->h=1-CurrentCompoObject->y;
         //CurrentCompoObject->w=((CurrentCompoObject->h*InteractiveZone->DisplayH)/CurrentBrush->AspectRatio)/InteractiveZone->DisplayW;
         CurrentCompoObject->setHeight(1-CurrentCompoObject->pos().y());
         CurrentCompoObject->setWidth(((CurrentCompoObject->height()*InteractiveZone->DisplayH)/CurrentBrush->AspectRatio())/InteractiveZone->DisplayW );
      }

      // if Slide name is name of this file
      if (UpdateSlideName) 
      {
         CurrentSlide->SlideName = CurrentBrush->MediaObject->ShortName();
         ui->SlideNameED->setText(CurrentSlide->SlideName);
      }

      // Lulo object for image and video must be remove
      ApplicationConfig->ImagesCache.RemoveImageObject(CurrentCompoObject->BackgroundBrush->MediaObject->RessourceKey(),CurrentCompoObject->BackgroundBrush->MediaObject->FileKey());

      // Apply settings to other shots
      APPLYBACKGROUNDBRUSH();
      ApplyToContexte(true);

      s_ShotTable_DisplayDuration();
   } 
   else 
   {
      RemoveLastPartialUndo();
   }
}

//========= Toggle visible state

void DlgSlideProperties::s_BlockSettings_ToggleVisibleState() 
{
   if (!ISBLOCKVALIDE()) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE,InteractiveZone,true);

   CurrentCompoObject->setVisible(!CurrentCompoObject->isVisible());

   // Special case for video : ensure only this video have sound
   if (CurrentCompoObject->BackgroundBrush->isVideo()) 
   {
      if (!CurrentCompoObject->isVisible()) 
      {
         CurrentCompoObject->BackgroundBrush->SoundVolume = 0;
      } 
      else 
      {
         bool SomeOneHaveSound = false;
         // Parse table to know if a block have sound for this shot
         for (int i = 0; i < CompositionList->compositionObjects.count(); i++)
            if (CompositionList->compositionObjects[i]->BackgroundBrush->isVideo() && CompositionList->compositionObjects[i]->BackgroundBrush->SoundVolume != 0) 
               SomeOneHaveSound = true;
         // If no block have sound => get sound to this video (-1=auto)
         if (!SomeOneHaveSound) 
            CurrentCompoObject->BackgroundBrush->SoundVolume = -1;
      }
   }
   // Reset thumbs if needed
   ResetThumbs(true);
   // Reset blocks table
   RefreshBlockTable(CurrentCompoObjectNbr);
   InteractiveZone->repaint();
}

//========= Get sound

void DlgSlideProperties::s_BlockSettings_GetSound() 
{
   if (!ISBLOCKVALIDEVISIBLE()) 
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE,InteractiveZone,true);

   // Only if this block is a video and don't have sound yet
   if (CurrentCompoObject->BackgroundBrush->SoundVolume == 0) 
   {
      for (int i = 0; i < CompositionList->compositionObjects.count(); i++)
         if (CurrentCompoObject != CompositionList->compositionObjects[i] && CompositionList->compositionObjects[i]->BackgroundBrush->isVideo()) 
            CompositionList->compositionObjects[i]->BackgroundBrush->SoundVolume = 0;
      CurrentCompoObject->BackgroundBrush->SoundVolume = -1; //(-1=auto)
      // Reset blocks table
      RefreshBlockTable(CurrentCompoObjectNbr);
   }
}

//====================================================================================================================
// Handler for position, size & rotation controls
//====================================================================================================================

//========= ShapeSizePos combo
void DlgSlideProperties::s_BlockSettings_ShapeSizePos(int Index) 
{
   if (!ISBLOCKVALIDEVISIBLE())
      return;
   if ((Index < 0) || (Index >= ui->ShapeSizePosCB->count()))
      return;
   AppendPartialUndo(UNDOACTION_FULL_SLIDE, ui->ShapeSizePosCB, true);
   CurrentCompoObject->ApplyAutoCompoSize(ui->ShapeSizePosCB->itemData(Index).toInt(), CurrentSlide->pDiaporama->ImageGeometry);
   // Apply values of previous shot to all shot for this object
   //APPLY4TONEXT(x,y,w,h);
   APPLY_GETSET_TONEXT(Rect);

   ApplyToContexte(false);
}

//====================================================================================================================
// Text annimation
//====================================================================================================================

//========= Text Zoom Level
void DlgSlideProperties::s_BlockSettings_TextAnimZoom(int Value) {
    if (!ISBLOCKVALIDEVISIBLE()) return;
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ui->ZoomED,false);
    CurrentCompoObject->tShotParams.TxtZoomLevel = Value;
    ui->ZoomSlider->setValue(CurrentCompoObject->tShotParams.TxtZoomLevel);
    ui->ZoomED->setValue(CurrentCompoObject->tShotParams.TxtZoomLevel);
    // Apply values of previous shot to all shot for this object
    APPLY1TONEXT(tShotParams.TxtZoomLevel);
    ApplyToContexte(false);
}

//========= Text scrolling X
void DlgSlideProperties::s_BlockSettings_TextAnimScrollX(int Value) {
    if (!ISBLOCKVALIDEVISIBLE()) return;
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ui->ScrollXED,false);
    CurrentCompoObject->tShotParams.TxtScrollX = Value;
    ui->ScrollXSlider->setValue(CurrentCompoObject->tShotParams.TxtScrollX);
    ui->ScrollXED->setValue(CurrentCompoObject->tShotParams.TxtScrollX);
    // Apply values of previous shot to all shot for this object
    APPLY1TONEXT(tShotParams.TxtScrollX);
    ApplyToContexte(false);
}

//========= Text scrolling Y
void DlgSlideProperties::s_BlockSettings_TextAnimScrollY(int Value) {
    if (!ISBLOCKVALIDEVISIBLE()) return;
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ui->ScrollYED,false);
    CurrentCompoObject->tShotParams.TxtScrollY = Value;
    ui->ScrollYSlider->setValue(CurrentCompoObject->tShotParams.TxtScrollY);
    ui->ScrollYED->setValue(CurrentCompoObject->tShotParams.TxtScrollY);
    // Apply values of previous shot to all shot for this object
    APPLY1TONEXT(tShotParams.TxtScrollY);
    ApplyToContexte(false);
}

//====================================================================================================================
// Handler for speed wave
//====================================================================================================================

void DlgSlideProperties::s_BlockSettings_SpeedWave(int) {
    if (!ISBLOCKVALIDEVISIBLE()) return;
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ui->SpeedWaveCB,true);
    CurrentCompoObject->setSpeedWave(ui->SpeedWaveCB->GetCurrentValue());
    // Apply values of previous shot to all shot for this object
    APPLY_GETSETA_TONEXT(SpeedWave,setSpeedWave);
    //APPLY1TONEXT(BlockSpeedWave);
    ApplyToContexte(false);
}

//====================================================================================================================
// Handler for block animation
//====================================================================================================================

void DlgSlideProperties::s_BlockSettings_BlockAnimType(int Value) {
    if (!ISBLOCKVALIDEVISIBLE()) return;
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ui->BlockAnimCB,true);
    CurrentCompoObject->setAnimType(Value);
    // Apply values of previous shot to all shot for this object
    APPLY_GETSETA_TONEXT(AnimType,setAnimType);
    //APPLY1TONEXT(BlockAnimType);
    ApplyToContexte(false);
}

//========= Multiple block turn X value
void DlgSlideProperties::s_BlockSettings_BlockAnimTurnXValue(int Value) {
    if (!ISBLOCKVALIDEVISIBLE()) return;
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ui->TurnXED,false);
    CurrentCompoObject->setXTurns(Value);
    // Apply values of previous shot to all shot for this object
    APPLY_GETSETA_TONEXT(XTurns,setXTurns);
    //APPLY1TONEXT(TurnXAxis);
    ApplyToContexte(false);
}

//========= Multiple block turn Z value
void DlgSlideProperties::s_BlockSettings_BlockAnimTurnZValue(int Value) {
    if (!ISBLOCKVALIDEVISIBLE()) return;
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ui->TurnZED,false);
    CurrentCompoObject->setZTurns(Value);
    // Apply values of previous shot to all shot for this object
    APPLY_GETSETA_TONEXT(ZTurns,setZTurns);
    //APPLY1TONEXT(TurnZAxis);
    ApplyToContexte(false);
}

//========= Multiple block turn Y value
void DlgSlideProperties::s_BlockSettings_BlockAnimTurnYValue(int Value) {
    if (!ISBLOCKVALIDEVISIBLE()) return;
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ui->TurnYED,false);
    CurrentCompoObject->setYTurns(Value);
    // Apply values of previous shot to all shot for this object
    APPLY_GETSETA_TONEXT(YTurns,setYTurns);
    //APPLY1TONEXT(TurnYAxis);
    ApplyToContexte(false);
}

//========= Dissolve animation
void DlgSlideProperties::s_BlockSettings_BlockAnimDissolveType(int Value) {
    if (!ISBLOCKVALIDEVISIBLE()) return;
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,ui->DissolveCB,true);
    CurrentCompoObject->setDissolveType(eBlockAnimValue(Value));
    // Apply values of previous shot to all shot for this object
    //APPLY1TONEXT(Dissolve);
    APPLY_GETSETA_TONEXT(DissolveType, setDissolveType);
    ApplyToContexte(false);
}

//====================================================================================================================

void DlgSlideProperties::s_BlockSettings_ChangeFramingStyle() {
    if (StopMajFramingStyle) return;
    if (!ISBLOCKVALIDEVISIBLE()) return;
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,InteractiveZone,true);

    cBrushDefinition SavedBrush(CurrentCompoObject,ApplicationConfig);
    SavedBrush.CopyFromBrushDefinition(CurrentCompoObject->BackgroundBrush);

    cBrushDefinition *CurrentBrush=CurrentCompoObject->BackgroundBrush;
    int AutoCompo=CurrentCompoObject->GetAutoCompoSize(CurrentSlide->pDiaporama->ImageGeometry);

    CurrentBrush->ApplyAutoFraming(ui->FramingStyleCB->GetCurrentFraming(),ProjectGeometry);
    CurrentCompoObject->ApplyAutoCompoSize(AutoCompo,CurrentSlide->pDiaporama->ImageGeometry,false);
    CurrentSlide->releaseCachObjects();

    // Apply values of previous shot to all shot for this object
    APPLYBACKGROUNDBRUSH();
    ApplyToContexte(true);
}
//====================================================================================================================

void DlgSlideProperties::s_BlockSettings_ChangeTextFramingStyle(int Value) {
    if (StopMajFramingStyle) return;
    if (!ISBLOCKVALIDEVISIBLE()) return;
    AppendPartialUndo(UNDOACTION_FULL_SLIDE,InteractiveZone,true);

    cBrushDefinition *CurrentBrush=CurrentCompoObject->BackgroundBrush;

    // It's a text block
    switch (Value) {
        case 0: // Unlock
            CurrentBrush->setLockGeometry(false);
            break;
        case 1: // Lock to this geometry
            CurrentBrush->setLockGeometry(true);
            CurrentBrush->setAspectRatio((CurrentCompoObject->height()*InteractiveZone->DisplayH)/(CurrentCompoObject->width()*InteractiveZone->DisplayW));
            break;
        case 2: // Lock to project geometry
            CurrentBrush->setLockGeometry(true);
            CurrentBrush->setAspectRatio(ProjectGeometry);
            break;
        default:
            return;
    }
    if (CurrentCompoObject->height() > (CurrentCompoObject->width() * InteractiveZone->DisplayW * CurrentBrush->AspectRatio())/InteractiveZone->DisplayH) 
      CurrentCompoObject->setHeight((CurrentCompoObject->width()*InteractiveZone->DisplayW*CurrentBrush->AspectRatio())/InteractiveZone->DisplayH);
    else 
      CurrentCompoObject->setWidth((CurrentCompoObject->height()*InteractiveZone->DisplayH/CurrentBrush->AspectRatio())/InteractiveZone->DisplayW);

    int CurrentRow=BlockTable->currentRow();
    // Reset thumbs if needed
    ResetThumbs(true);
    // Reset blocks table
    RefreshBlockTable(CurrentRow>0?CurrentRow:0);
}

//void DlgSlideProperties::wheelEvent(QWheelEvent * event)
//{
//   if( InteractiveZone->IsCapture )
//      return;
//   Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
//   qreal step = (event->angleDelta().y()/120)*10;
//   if( modifiers & Qt::ControlModifier )
//      step /= 10.0;
//   step /= 100;
//   switch(wheelMode)
//   {
//      case eMoveHor:
//         s_BlockSettings_IntZoneTransformBlocks(step, 0, 0, 0, 0, 0, 1.0, 1.0);
//      break;
//   }
//}

void DlgSlideProperties::ComputeBlockRatio(cCompositionObject *Block, double &Ratio_X, double &Ratio_Y)
{
   if (!Block)
      return;
   getShapeRatio(Block->BackgroundForm,&Ratio_X, &Ratio_Y);
   //QRectF r = Block->appliedRect(100,100);
   //QRectF tmpRect = PolygonToRectF(ComputePolygon(Block->BackgroundForm, r));
   //Ratio_X = (Block->width() * 100.0) / tmpRect.width();
   //Ratio_Y = (Block->height() * 100.0) / tmpRect.height();
   //qDebug() << "BlockRatio now " << Ratio_X << " " << Ratio_Y;
}

void DlgSlideProperties::setWheelMode(int i)
{
   InteractiveZone->setWheelMode(i);
}

void DlgSlideProperties::on_pbMovingsOnTop_clicked()
{
   CurrentShotNbr = ShotTable->currentColumn();
   CurrentSlide->movingsOnTop(CurrentShotNbr);
   s_ShotTable_SelectionChanged();
}
