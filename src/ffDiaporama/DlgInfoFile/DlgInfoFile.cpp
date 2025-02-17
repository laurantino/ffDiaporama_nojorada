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

#include "DlgInfoFile.h"
#include "engine/cVideoFile.h"
#include "ui_DlgInfoFile.h"

//====================================================================================================================

DlgInfoFile::DlgInfoFile(cBaseMediaFile *MediaFile,cApplicationConfig *ApplicationConfig, QWidget *parent)
    :QCustomDialog(ApplicationConfig,parent),ui(new Ui::DlgInfoFile)
{
    this->MediaFile=MediaFile;

    ui->setupUi(this);
    //CancelBt=ui->CancelBt;
    OkBt    =ui->OkBt;
    HelpBt  =ui->HelpBt;
    HelpFile="0111";
}

//====================================================================================================================

DlgInfoFile::~DlgInfoFile()
{
    delete ui;
}

//====================================================================================================================
// Initialise dialog

void DlgInfoFile::DoInitDialog() 
{
   DoInitTableWidget(ui->tableWidget,"Propertie;Value");

   if (MediaFile) {
      QStringList TempExtProperties;
      ApplicationConfig->FilesTable->GetExtendedProperties(MediaFile->FileKey(),&TempExtProperties);
      // General file information
      if (MediaFile->is_Imagefile()) 
      {
         cLuLoImageCacheObject *ImageObject=ApplicationConfig->ImagesCache.FindObject(MediaFile->RessourceKey(),MediaFile->FileKey(),MediaFile->modificationTime(),MediaFile->Orientation(),true,false);
         if (ImageObject) {
            QImage *Img=ImageObject->ValidateCachePreviewImage();
            if (Img) {
               QImage NewImg=(Img->width()>Img->height())?Img->scaledToWidth(100,Qt::SmoothTransformation):Img->scaledToHeight(100,Qt::SmoothTransformation);
               ui->FileIconLabel->setPixmap(QPixmap().fromImage(NewImg));
               delete Img;
            }
         }
      } 
      else 
         ui->FileIconLabel->setPixmap(QPixmap().fromImage(MediaFile->GetIcon(cCustomIcon::ICON100,false)));

      ui->FileNameValue->setText(     MediaFile->RessourceKey() == -1 ? MediaFile->ShortName() : "");
      ui->FileSizeValue->setText(     MediaFile->RessourceKey() == -1 ? MediaFile->GetFileSizeStr() : "");
      ui->FileCreatedValue->setText(  MediaFile->GetFileDateTimeStr(true));
      ui->FileModifyValue->setText(   MediaFile->RessourceKey() == -1 ? MediaFile->GetFileDateTimeStr(false) : "");
      QString ExtType=GetInformationValue("Long Format",&TempExtProperties);
      if (ExtType.isEmpty()) 
         ui->FileTypeValue->setText(MediaFile->GetFileTypeStr());
      else 
         ui->FileTypeValue->setText(MediaFile->GetFileTypeStr()+QString("(%1)").arg(ExtType));

      //**************** Video
      if (MediaFile->is_Videofile() && (static_cast<cVideoFile *>(MediaFile)->VideoTrackNbr > 0))
      {
         ui->VideoTable->setUpdatesEnabled(false);
         ui->VideoTitleLabel->setVisible(true);
         ui->VideoTable->setVisible(true);
         DoInitTableWidget(ui->VideoTable,"#;Image Size;Image Format;Image Geometry;Codec;Frame Rate;Bitrate");
         for (int i = 0; i < ((cVideoFile *)MediaFile)->VideoTrackNbr;i++) 
         {
            QString TrackNum=QString("%1").arg(i); while (TrackNum.length()<3) TrackNum="0"+TrackNum;
            TrackNum="Video_"+TrackNum+":";
            QColor Background=((i & 0x01)==0x01)?Qt::white:QColor(0xE0,0xE0,0xE0);
            ui->VideoTable->insertRow(ui->VideoTable->rowCount());
            ui->VideoTable->setItem(ui->VideoTable->rowCount()-1,0,CreateItem(QString("%1").arg(i+1),Qt::AlignLeft|Qt::AlignVCenter,Background));
            ui->VideoTable->setItem(ui->VideoTable->rowCount()-1,1,CreateItem(MediaFile->GetImageSizeStr(cBaseMediaFile::SIZEONLY),Qt::AlignCenter|Qt::AlignVCenter,Background));
            ui->VideoTable->setItem(ui->VideoTable->rowCount()-1,2,CreateItem(MediaFile->GetImageSizeStr(cBaseMediaFile::FMTONLY),Qt::AlignCenter|Qt::AlignVCenter,Background));
            ui->VideoTable->setItem(ui->VideoTable->rowCount()-1,3,CreateItem(MediaFile->GetImageSizeStr(cBaseMediaFile::GEOONLY),Qt::AlignCenter|Qt::AlignVCenter,Background));
            ui->VideoTable->setItem(ui->VideoTable->rowCount()-1,4,CreateItem(GetInformationValue(TrackNum+"Codec",&TempExtProperties),Qt::AlignCenter|Qt::AlignVCenter,Background));
            ui->VideoTable->setItem(ui->VideoTable->rowCount()-1,5,CreateItem(GetInformationValue(TrackNum+"Frame rate",&TempExtProperties),Qt::AlignCenter|Qt::AlignVCenter,Background));
            ui->VideoTable->setItem(ui->VideoTable->rowCount()-1,6,CreateItem(GetInformationValue(TrackNum+"Bitrate",&TempExtProperties),Qt::AlignCenter|Qt::AlignVCenter,Background));
         }
         DoResizeColumnsTableWidget(ui->VideoTable);
         ui->VideoTable->setUpdatesEnabled(true);
      } else {
         ui->VideoTitleLabel->setVisible(false);
         ui->VideoTable->setVisible(false);
      }

      //**************** Chapters
      if (MediaFile->hasChapters()) 
      {
         ui->ChapterTable->setUpdatesEnabled(false);
         ui->ChapterTitleLabel->setVisible(true);
         ui->ChapterTable->setVisible(true);
         int NbrChapter = MediaFile->numChapters();
         QStringList const *props = &TempExtProperties;
         if (MediaFile->is_FFDfile()) 
         {
            DoInitTableWidget(ui->ChapterTable,"#;Slide;Title;Start;End;Duration");
            cffDProjectFile *ffdMediaFile = (cffDProjectFile *)MediaFile;
            props = ffdMediaFile->chapterProps();
         }
         else 
            DoInitTableWidget(ui->ChapterTable,"#;Title;Start;End;Duration");
         for (int i = 0; i < NbrChapter; i++) 
         {
            int     Col=0;
            QString ChapterNum=QString("%1").arg(i); while (ChapterNum.length()<3) ChapterNum="0"+ChapterNum;
            ChapterNum="Chapter_"+ChapterNum+":";
            QColor Background=((i & 0x01)==0x01)?Qt::white:QColor(0xE0,0xE0,0xE0);
            ui->ChapterTable->insertRow(ui->ChapterTable->rowCount());
            ui->ChapterTable->setItem(ui->ChapterTable->rowCount()-1,Col++,CreateItem(QString("%1").arg(i+1),Qt::AlignLeft|Qt::AlignVCenter,Background));
            //QStringList const *props = &TempExtProperties;
            if (MediaFile->is_FFDfile()) 
            {
               //cffDProjectFile *ffdMediaFile = (cffDProjectFile *)MediaFile;
               //props = ffdMediaFile->chapterProps();
               ui->ChapterTable->setItem(ui->ChapterTable->rowCount()-1,Col++,CreateItem(GetInformationValue(ChapterNum+"InSlide", props),Qt::AlignLeft|Qt::AlignVCenter,Background));
               //ui->ChapterTable->setItem(ui->ChapterTable->rowCount()-1,Col++,CreateItem(GetInformationValue(ChapterNum+"title",   &ffdMediaFile->ChaptersProperties),Qt::AlignLeft|Qt::AlignVCenter,Background));
               //ui->ChapterTable->setItem(ui->ChapterTable->rowCount()-1,Col++,CreateItem(GetInformationValue(ChapterNum+"Start",   &ffdMediaFile->ChaptersProperties),Qt::AlignLeft|Qt::AlignVCenter,Background));
               //ui->ChapterTable->setItem(ui->ChapterTable->rowCount()-1,Col++,CreateItem(GetInformationValue(ChapterNum+"End",     &ffdMediaFile->ChaptersProperties),Qt::AlignLeft|Qt::AlignVCenter,Background));
               //ui->ChapterTable->setItem(ui->ChapterTable->rowCount()-1,Col++,CreateItem(GetInformationValue(ChapterNum+"Duration",&ffdMediaFile->ChaptersProperties),Qt::AlignLeft|Qt::AlignVCenter,Background));
            } 
            ui->ChapterTable->setItem(ui->ChapterTable->rowCount()-1,Col++,CreateItem(GetInformationValue(ChapterNum+"title",   props),Qt::AlignLeft|Qt::AlignVCenter,Background));
            ui->ChapterTable->setItem(ui->ChapterTable->rowCount()-1,Col++,CreateItem(GetInformationValue(ChapterNum+"Start",   props),Qt::AlignLeft|Qt::AlignVCenter,Background));
            ui->ChapterTable->setItem(ui->ChapterTable->rowCount()-1,Col++,CreateItem(GetInformationValue(ChapterNum+"End",     props),Qt::AlignLeft|Qt::AlignVCenter,Background));
            ui->ChapterTable->setItem(ui->ChapterTable->rowCount()-1,Col++,CreateItem(GetInformationValue(ChapterNum+"Duration",props),Qt::AlignLeft|Qt::AlignVCenter,Background));
         }
         DoResizeColumnsTableWidget(ui->ChapterTable);
         ui->ChapterTable->setUpdatesEnabled(true);
      } 
      else 
      {
         ui->ChapterTitleLabel->setVisible(false);
         ui->ChapterTable->setVisible(false);
      }

      //**************** Audio
      if ((MediaFile->is_Videofile() || MediaFile->is_Musicfile()) && (((cVideoFile *)MediaFile)->AudioTrackNbr > 0)) 
      {
         ui->AudioTable->setUpdatesEnabled(false);
         ui->AudioTitleLabel->setVisible(true);
         ui->AudioTable->setVisible(true);
         if (MediaFile->is_Videofile()) 
            DoInitTableWidget(ui->AudioTable,"#;Language;Codec;Channels;Bitrate;Frequency;Title");
         else 
            DoInitTableWidget(ui->AudioTable,"#;Language;Codec;Channels;Bitrate;Frequency");
         for (int i=0;i<((cVideoFile *)MediaFile)->AudioTrackNbr;i++) 
         {
            QString TrackNum=QString("%1").arg(i); while (TrackNum.length()<3) TrackNum="0"+TrackNum;
            TrackNum="Audio_"+TrackNum+":";
            QColor Background=((i & 0x01)==0x01)?Qt::white:QColor(0xE0,0xE0,0xE0);
            ui->AudioTable->insertRow(ui->AudioTable->rowCount());
            ui->AudioTable->setItem(ui->AudioTable->rowCount()-1,0,CreateItem(QString("%1").arg(i+1),Qt::AlignLeft|Qt::AlignVCenter,Background));
            ui->AudioTable->setItem(ui->AudioTable->rowCount()-1,1,CreateItem(GetInformationValue(TrackNum+"language",&TempExtProperties),Qt::AlignCenter|Qt::AlignVCenter,Background));
            ui->AudioTable->setItem(ui->AudioTable->rowCount()-1,2,CreateItem(GetInformationValue(TrackNum+"Codec",&TempExtProperties),Qt::AlignCenter|Qt::AlignVCenter,Background));
            ui->AudioTable->setItem(ui->AudioTable->rowCount()-1,3,CreateItem(GetInformationValue(TrackNum+"Channels",&TempExtProperties),Qt::AlignCenter|Qt::AlignVCenter,Background));
            ui->AudioTable->setItem(ui->AudioTable->rowCount()-1,4,CreateItem(GetInformationValue(TrackNum+"Bitrate",&TempExtProperties),Qt::AlignCenter|Qt::AlignVCenter,Background));
            ui->AudioTable->setItem(ui->AudioTable->rowCount()-1,5,CreateItem(GetInformationValue(TrackNum+"Frequency",&TempExtProperties),Qt::AlignCenter|Qt::AlignVCenter,Background));
            if (MediaFile->is_Videofile())
               ui->AudioTable->setItem(ui->AudioTable->rowCount()-1,6,CreateItem(GetInformationValue(TrackNum+"title",&TempExtProperties),Qt::AlignLeft|Qt::AlignVCenter,Background));
         }
         DoResizeColumnsTableWidget(ui->AudioTable);
         ui->AudioTable->setUpdatesEnabled(true);
      } else {
         ui->AudioTitleLabel->setVisible(false);
         ui->AudioTable->setVisible(false);
      }

      //**************** Additionnals
      ui->tableWidget->setUpdatesEnabled(false);
#if QT_VERSION >= 0x050000
      ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
      ui->tableWidget->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
      if (MediaFile->is_FFDfile()) 
      {
         cffDProjectFile *ffdMediaFile = (cffDProjectFile *)MediaFile;
         TempExtProperties.append(QString("%1##%2").arg(QApplication::translate("QCustomFolderTable","Image Geometry","Column header")).arg(ffdMediaFile->GetImageSizeStr(cBaseMediaFile::GEOONLY)));
         TempExtProperties.append(QString("%1##%2").arg(QApplication::translate("QCustomFolderTable","Title","Column header")).arg(ffdMediaFile->Title()));
         TempExtProperties.append(QString("%1##%2").arg(QApplication::translate("QCustomFolderTable","Artist","Column header")).arg(ffdMediaFile->Author()));
         TempExtProperties.append(QString("%1##%2").arg(QApplication::translate("QCustomFolderTable","Album","Column header")).arg(ffdMediaFile->Album()));
         TempExtProperties.append(QString("%1##%2").arg(QApplication::translate("QCustomFolderTable","Comment","Column header")).arg(ffdMediaFile->Comment()));
         TempExtProperties.append(QString("%1##%2 (%3)").arg(QApplication::translate("QCustomFolderTable","Composer","Column header")).arg(ffdMediaFile->Composer()).arg(ffdMediaFile->Revision()));
         TempExtProperties.append(QString("%1##%2").arg(QApplication::translate("Variables","Project slide count")).arg(ffdMediaFile->numSlides()));
         TempExtProperties.append(QString("%1##%2").arg(QApplication::translate("Variables","Short date")).arg(ffdMediaFile->EventDate().toString(ApplicationConfig->ShortDateFormat)));
      } else if (MediaFile->RessourceKey() != -1) {
         MediaFile->GetChildFullInformationFromFile(false,NULL,&TempExtProperties);
      }
      for (int i=0;i<TempExtProperties.count();i++)
         if (!TempExtProperties[i].startsWith("Chapter_")
            && !TempExtProperties[i].startsWith("Video_")
            && !TempExtProperties[i].startsWith("Audio_")
            && !TempExtProperties[i].startsWith("Short Format")
            && !TempExtProperties[i].startsWith("Long Format")
            ) {
               ui->tableWidget->insertRow(ui->tableWidget->rowCount());
               QStringList Value=TempExtProperties[i].split("##");
               ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,0,new QTableWidgetItem(Value[0]));
               ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,1,new QTableWidgetItem(Value[1]));
         }
         DoResizeColumnsTableWidget(ui->tableWidget);
         ui->tableWidget->setUpdatesEnabled(true);
   } else {
      ui->VideoTitleLabel->setVisible(false);
      ui->VideoTable->setVisible(false);
      ui->ChapterTitleLabel->setVisible(false);
      ui->ChapterTable->setVisible(false);
      ui->AudioTitleLabel->setVisible(false);
      ui->AudioTable->setVisible(false);
   }
}
