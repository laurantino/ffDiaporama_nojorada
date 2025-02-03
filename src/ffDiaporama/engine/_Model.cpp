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
#include "CustomCtrl/_QCustomDialog.h"
#include "_Model.h"
#include "cApplicationConfig.h"
#include "_Diaporama.h"

#define ICON_CUSTOMIZEDSLIDE ":/img/action_edit.png"
#define ICON_CUSTOMMODEL     ":/img/color.png"

//=============================================================================================================================

cModelListItem::cModelListItem(cModelList *Parent,QString FileName,QSize ThumbnailSize):QObject(Parent) 
{
   this->Parent        = Parent;
   this->FileName      = FileName == "*" ? "" : FileName;
   this->ThumbnailSize = ThumbnailSize;
   Name                = FileName == "*" ? FileName : QFileInfo(FileName).baseName();
   IsCustom            = FileName == "*" ? true : !FileName.startsWith(ModelFolder);

   if (FileName != "*") 
      Model = LoadModelFile(Parent->ModelType,FileName);

   // Compute Duration
   if (Parent->ModelType == ffd_MODELTYPE_THUMBNAIL) 
   {
      Duration = 0;
   } 
   else 
   {
      cDiaporama *Diaporama = new cDiaporama(Parent->ApplicationConfig,false,this);
      Diaporama->slides.append(new cDiaporamaObject(Diaporama));
      Diaporama->slides[0]->LoadModelFromXMLData(Parent->ModelType,Model,&ResKeyList,false);
      Duration = Diaporama->slides[0]->GetDuration();
      delete Diaporama;
   }
}

//=============================================================================================================================

QImage cModelListItem::PrepareImage(int64_t Position,cDiaporama *DiaporamaToUse,cDiaporamaObject *DiaporamaObjectToUse,QSize *ForcedThumbnailSize) 
{
   QImage           Thumb;
   cDiaporama       *Diaporama = DiaporamaObjectToUse ? DiaporamaObjectToUse->pDiaporama : NULL;
   cDiaporamaObject *DiaporamaObject = DiaporamaObjectToUse;

   if (!Diaporama) 
   {
      Diaporama = new cDiaporama(Parent->ApplicationConfig,false,this);
      if (!DiaporamaToUse) 
      {
         Diaporama->ProjectInfo()->setTitle  (QApplication::translate("cModelList","Project title"));
         Diaporama->ProjectInfo()->setAlbum  (QApplication::translate("cModelList","Project album"));
         Diaporama->ProjectInfo()->setAuthor (QApplication::translate("cModelList","Project author"));
         Diaporama->ProjectInfo()->setComment(QApplication::translate("cModelList","Project comment"));
      } 
      else 
      {
         Diaporama->ProjectInfo()->setTitle        (DiaporamaToUse->ProjectInfo()->Title());
         Diaporama->ProjectInfo()->setAlbum        (DiaporamaToUse->ProjectInfo()->Album());
         Diaporama->ProjectInfo()->setAuthor       (DiaporamaToUse->ProjectInfo()->Author());
         Diaporama->ProjectInfo()->setComment      (DiaporamaToUse->ProjectInfo()->Comment());
         Diaporama->ProjectInfo()->setEventDate    (DiaporamaToUse->ProjectInfo()->EventDate());
         Diaporama->ProjectInfo()->setOverrideDate (DiaporamaToUse->ProjectInfo()->OverrideDate());
         Diaporama->ProjectInfo()->setLongDate     (DiaporamaToUse->ProjectInfo()->LongDate());
         Diaporama->ProjectInfo()->SetGivenDuration(DiaporamaToUse->ProjectInfo()->GetGivenDuration());
         Diaporama->ProjectInfo()->setNumSlides    (DiaporamaToUse->ProjectInfo()->numSlides());
         Diaporama->ProjectInfo()->setNumChapters  (DiaporamaToUse->ProjectInfo()->numChapters());
      }
   }

   if (!DiaporamaObject) 
   {
      Diaporama->slides.append(new cDiaporamaObject(Diaporama));
      if (Parent->ModelType == ffd_MODELTYPE_THUMBNAIL) 
      {
         DiaporamaObject = Diaporama->ProjectThumbnail;
         if (Name != "*") 
            DiaporamaObject->LoadModelFromXMLData(Parent->ModelType,Model,&ResKeyList,false);
      } 
      else 
      {
         DiaporamaObject = Diaporama->slides[0];
         DiaporamaObject->StartNewChapter          = true;
         DiaporamaObject->ChapterName              = QApplication::translate("cModelList","Chapter title");
         DiaporamaObject->OverrideProjectEventDate = true;
         DiaporamaObject->ChapterEventDate         = QDate::currentDate();
         DiaporamaObject->OverrideChapterLongDate  = true;
         DiaporamaObject->ChapterLongDate          = FormatLongDate(Diaporama->slides[0]->ChapterEventDate);
         DiaporamaObject->LoadModelFromXMLData(Parent->ModelType,Model,&ResKeyList,false);
      }
   } 
   else 
   {
      DiaporamaObject->ChapterName = QApplication::translate("cModelList","Chapter title");
      if (Parent->ModelType == ffd_MODELTYPE_THUMBNAIL) 
      {
         if (Name != "*") 
            DiaporamaObject->LoadModelFromXMLData(Parent->ModelType,Model,&ResKeyList,false);
      } 
      else 
      {
         DiaporamaObject->SlideName=QString("<%AUTOTS_%1%>").arg(Name);
         DiaporamaObject->LoadModelFromXMLData(Parent->ModelType,Model,&ResKeyList,false);
      }
   }

   //**************** Compute CurShot
   cDiaporamaObjectInfo *Info = new cDiaporamaObjectInfo();
   Info->Current.Object = DiaporamaObject;
   Info->Current.Object_InObjectTime = Position;

   // calculate wich sequence in the current object is
   int CurPos = 0;

   while ((Info->Current.Object_ShotSequenceNumber < Info->Current.Object->shotList.count() - 1) && ((CurPos + DiaporamaObject->shotList[Info->Current.Object_ShotSequenceNumber]->StaticDuration) <= Position))
   {
      CurPos = CurPos + DiaporamaObject->shotList[Info->Current.Object_ShotSequenceNumber]->StaticDuration;
      Info->Current.Object_ShotSequenceNumber++;
   }
   Info->Current.Object_CurrentShot = DiaporamaObject->shotList[Info->Current.Object_ShotSequenceNumber];
   if (Info->Current.Object_ShotSequenceNumber<DiaporamaObject->shotList.count() - 1) 
      Info->Current.Object_ShotDuration = Info->Current.Object_CurrentShot->StaticDuration;
   else 
      Info->Current.Object_ShotDuration = Info->Current.Object_CurrentShot->pDiaporamaObject->GetDuration()-CurPos;

   // calculate CurrentObject_PCTDone
   Info->Current.Object_PCTDone = (double(Position)-double(CurPos))/(double(Info->Current.Object_ShotDuration));

   //**************** Draw image

   Thumb = QImage(ForcedThumbnailSize ? *ForcedThumbnailSize : ThumbnailSize, QImage::Format_ARGB32_Premultiplied);


   //ICI:************** Ajouter le chargement du fond !!!!!!!
   Thumb.fill(Qt::black);

   QPainter P;
   P.begin(&Thumb);
   P.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing|QPainter::SmoothPixmapTransform);
   P.setCompositionMode(QPainter::CompositionMode_SourceOver);
   CompositionContextList PreparedBrushList;

   int ShotNumber = Info->Current.Object_ShotSequenceNumber;
   cDiaporamaShot *PreviousShot = (ShotNumber>0 ? DiaporamaObject->shotList[ShotNumber - 1] : NULL);

   // Construct collection
   for (int j = 0; j < Info->Current.Object_CurrentShot->ShotComposition.compositionObjects.count(); j++)
      PreparedBrushList.append(new cCompositionObjectContext(j,true,true,Info,QSizeF(Thumb.width(),Thumb.height()+1),
         Info->Current.Object_CurrentShot,PreviousShot,NULL,0,0));
   // Compute each item of the collection
   for (int aa = 0; aa < PreparedBrushList.count(); aa++) 
      PreparedBrushList[aa]->Compute();

   // Draw collection
   for (int j = 0; j < Info->Current.Object_CurrentShot->ShotComposition.compositionObjects.count(); j++) 
   {
      cCompositionObjectContext *context = PreparedBrushList[j];
      Info->Current.Object_CurrentShot->ShotComposition.compositionObjects[j]->DrawCompositionObject(DiaporamaObject,&P,double(Thumb.height())/double(1080),
         Thumb.size(),
         context->PreviewMode,
         context->VideoPosition + context->StartPosToAdd,
         context->SoundTrackMontage,
         context->BlockPctDone, context->ImagePctDone,
         context->PrevCompoObject,100,
         true,false,QRectF(),false,context);
   }
   //PreparedBrushList.clear();
   deleteList(PreparedBrushList);

   // Add custom model of customized icon if needed
   if (IsCustom && !ForcedThumbnailSize) 
   {
      if ( Name == "*" )  
         P.drawImage(Thumb.width()-26,Thumb.height()-26,QImage(ICON_CUSTOMIZEDSLIDE));
      else
         P.drawImage(Thumb.width()-26,Thumb.height()-26,QImage(ICON_CUSTOMMODEL));
   }

   P.end();
   delete Info;

   //**************** Cleaning
   if (!DiaporamaObjectToUse) 
      delete Diaporama;

   return Thumb;
}

//=============================================================================================================================

cModelListItem::~cModelListItem() {
}

//=============================================================================================================================

QDomDocument cModelListItem::LoadModelFile(ffd_MODELTYPE TypeModel,QString ModelFileName) 
{
   QFile        file(ModelFileName);
   QString      errorStr,ErrorMsg;
   int          errorLine,errorColumn;
   QDomDocument domDocument;

   if (!file.open(QFile::ReadOnly | QFile::Text)) 
   {
      switch (TypeModel) 
      {
         case ffd_MODELTYPE_THUMBNAIL:
            ErrorMsg=QApplication::translate("cModelList","Error reading default thumbnail file","Error message");
            break;
         case ffd_MODELTYPE_PROJECTTITLE:
         case ffd_MODELTYPE_CHAPTERTITLE:
         case ffd_MODELTYPE_CREDITTITLE:
            ErrorMsg=QApplication::translate("cModelList","Error reading title model file","Error message");
            break;
      }
      ErrorMsg = ErrorMsg + "\n" + ModelFileName;
      CustomMessageBox(NULL,QMessageBox::Critical,QApplication::translate("cModelList","Error","Error message"),ErrorMsg,QMessageBox::Close);
      return domDocument;
   } 

   QTextStream InStream(&file);
   QString     ffDPart;
   QString     OtherPart = "<!DOCTYPE ffDiaporama>\n";
   bool        EndffDPart = false;

#if QT_VERSION < 0x050000
   InStream.setCodec("UTF-8");
#endif
   while (!InStream.atEnd()) 
   {
      QString Line = InStream.readLine();
      if (!EndffDPart) 
      {
         ffDPart.append(Line);
         if (Line == "</Model>") 
            EndffDPart = true;
      } 
      else 
      {
         OtherPart.append(Line);
         if (Line.endsWith("/>")) 
         {
            QDomDocument ResDoc;
            if (ResDoc.setContent(OtherPart,true,&errorStr,&errorLine,&errorColumn)) 
            {
               QDomElement ResElem = ResDoc.documentElement();
               if (ResElem.tagName() == "Ressource") 
               {
                  int         Width   = ResElem.attribute("Width").toInt();
                  int         Height  = ResElem.attribute("Height").toInt();
                  qlonglong   Key     = ResElem.attribute("Key").toLongLong();
                  QImage      Thumb(Width,Height,QImage::Format_ARGB32_Premultiplied);
                  QByteArray  Compressed = QByteArray::fromHex(ResElem.attribute("Image").toUtf8());
                  QByteArray  Decompressed = qUncompress(Compressed);
                  Thumb.loadFromData(Decompressed);
                  ResKeyList.append(Parent->ApplicationConfig->SlideThumbsTable->AppendThumb(Key,Thumb));
               }
            }
            // Go to next ressource
            OtherPart = "<!DOCTYPE ffDiaporama>\n";
         }
      }
   }

   file.close();

   repairFonts(ffDPart);

   // Now import ffDPart
   if (!domDocument.setContent(ffDPart, true, &errorStr, &errorLine,&errorColumn)) 
   {
      switch (TypeModel) {
         case ffd_MODELTYPE_THUMBNAIL:     
            ErrorMsg = QApplication::translate("cModelList","Error reading content of default thumbnail file","Error message");     
         break;
         case ffd_MODELTYPE_PROJECTTITLE:
         case ffd_MODELTYPE_CHAPTERTITLE:
         case ffd_MODELTYPE_CREDITTITLE:
            ErrorMsg=QApplication::translate("cModelList","Error reading content of title model file","Error message");
         break;
      }
      ErrorMsg = ErrorMsg + "\n" + ModelFileName;
      CustomMessageBox(NULL,QMessageBox::Critical,QApplication::translate("cModelList","Error","Error message"),ErrorMsg,QMessageBox::Close);
   }
   return domDocument;
}

//=============================================================================================================================

cModelList::cModelList(cApplicationConfig *ApplicationConfig,ffd_MODELTYPE ModelType,int64_t *NextNumber,ffd_GEOMETRY ProjectGeometry,int DigitCategorie,QString NameCategorie):QObject(ApplicationConfig) 
{
   this->ApplicationConfig = ApplicationConfig;
   this->NextNumber        = NextNumber;
   this->ProjectGeometry   = ProjectGeometry;
   this->DigitCategorie    = DigitCategorie;
   this->NameCategorie     = NameCategorie;
   FillModelType(ModelType);
}

//=============================================================================================================================

cModelList::~cModelList() {
}

//=============================================================================================================================
void cModelList::FillModelType(ffd_MODELTYPE ModelType) 
{
   int StartNumber,EndNumber;
   QString GeoFolder = ProjectGeometry == GEOMETRY_4_3 ? "/4_3" : ProjectGeometry == GEOMETRY_16_9 ? "/16_9" : ProjectGeometry == GEOMETRY_40_17 ? "/40_17" : "";
   List.clear();
   this->ModelType = ModelType;
   StandardModelPath = ModelFolder;                          
   if (!StandardModelPath.endsWith(QDir::separator())) 
      StandardModelPath = StandardModelPath+QDir::separator();
   CustomModelPath  = ApplicationConfig->UserConfigPath;    
   if (!CustomModelPath.endsWith(QDir::separator())) 
      CustomModelPath = CustomModelPath+QDir::separator();

   switch (ModelType) 
   {
      case ffd_MODELTYPE_PROJECTTITLE:
         ToLog(LOGMSG_DEBUGTRACE,QApplication::translate("cModelList","Register Project title slide models (%1)...").arg(GeoFolder));
         StandardModelPath= StandardModelPath+"Titles";
         CustomModelPath  = CustomModelPath+"Titles";
         ModelSuffix      = "tss";
         StartNumber      = QString(QString("1%1").arg(DigitCategorie)+"0000").toInt();
         EndNumber        = QString(QString("1%1").arg(DigitCategorie)+"9999").toInt();
         break;
      case ffd_MODELTYPE_CHAPTERTITLE:
         ToLog(LOGMSG_DEBUGTRACE,QApplication::translate("cModelList","Register Chapter title slide models (%1)...").arg(GeoFolder));
         StandardModelPath= StandardModelPath+"Titles";
         CustomModelPath  = CustomModelPath+"Titles";
         ModelSuffix      = "tss";
         StartNumber      = QString(QString("2%1").arg(DigitCategorie)+"0000").toInt();
         EndNumber        = QString(QString("2%1").arg(DigitCategorie)+"9999").toInt();
         break;
      case ffd_MODELTYPE_CREDITTITLE:
         ToLog(LOGMSG_DEBUGTRACE,QApplication::translate("cModelList","Register Credit title slide models (%1)...").arg(GeoFolder));
         StandardModelPath= StandardModelPath+"Titles";
         CustomModelPath  = CustomModelPath+"Titles";
         ModelSuffix      = "tss";
         StartNumber      = QString(QString("3%1").arg(DigitCategorie)+"0000").toInt();
         EndNumber        = QString(QString("3%1").arg(DigitCategorie)+"9999").toInt();
         break;
      case ffd_MODELTYPE_THUMBNAIL:
      default:
         ToLog(LOGMSG_DEBUGTRACE,QApplication::translate("cModelList","Register thumbnail models..."));
         StandardModelPath= StandardModelPath+"Thumbnails";
         CustomModelPath  = CustomModelPath+"Thumbnails";
         ModelSuffix      = "thb";
         ThumbnailSize    = QSize(THUMB_THUMBWITH,THUMB_THUMBHEIGHT);
         StartNumber      = 100000;
         EndNumber        = 199999;
         break;
   }
   if (ModelType!=ffd_MODELTYPE_THUMBNAIL) 
      switch (ProjectGeometry) 
      {
         case GEOMETRY_4_3:   ThumbnailSize = QSize(156,117);    break;
         case GEOMETRY_40_17: ThumbnailSize = QSize(280,119);    break;
         default:             ThumbnailSize = QSize(208,117);    break;
      }

   StandardModelPath=StandardModelPath+GeoFolder;
   CustomModelPath  =CustomModelPath+GeoFolder;

   if (*NextNumber < (StartNumber/100000)*100000+90000) 
      *NextNumber = (StartNumber/100000)*100000+90000;

   // Load standard model
   QDir Folder(StandardModelPath);
   QFileInfoList Files = Folder.entryInfoList(QStringList(QString("*.%1").arg(ModelSuffix)),QDir::Dirs|QDir::AllDirs|QDir::Files|QDir::Hidden);
   //int i = 0;

   //while (i < Files.count()) 
   //   if ((Files[i].fileName().startsWith(".")) || (Files[i].suffix().toLower()!=ModelSuffix)) 
   //      Files.removeAt(i); 
   //   else i++;
   for (int i = 0; i < Files.count(); i++) 
   {
      QString Number=Files[i].baseName();
      if (Number.endsWith("*")) 
         Number = Number.left(Number.length()-QString("*").length());
      int  iNumber = 0;
      bool IsOk = true;
      iNumber = Number.toInt(&IsOk);
      if ((IsOk) && (iNumber>=StartNumber) && (iNumber<=EndNumber)) 
         List.append(new cModelListItem(this,QDir::toNativeSeparators(Files[i].absoluteFilePath()),ThumbnailSize));
   }

   // Load custom model
   Folder = QDir(CustomModelPath);
   if (!Folder.exists()) 
      Folder.mkpath(Folder.absolutePath());
   Files = Folder.entryInfoList(QStringList(QString("*.%1").arg(ModelSuffix)),QDir::Dirs|QDir::AllDirs|QDir::Files|QDir::Hidden);
   //i = 0;
   //while ( i < Files.count()) 
   //   if ((Files[i].fileName().startsWith("."))||(Files[i].suffix().toLower()!=ModelSuffix)) 
   //      Files.removeAt(i); 
   //   else i++;
   for (int i = 0; i < Files.count(); i++) 
   {
      QString Number = Files[i].baseName();
      if (Number.endsWith("*")) 
         Number = Number.left(Number.length()-QString("*").length());
      int  iNumber = 0;
      bool IsOk = true;
      iNumber = Number.toInt(&IsOk);
      if ((IsOk) && (iNumber>=StartNumber) && (iNumber<=EndNumber)) 
      {
         List.append(new cModelListItem(this,QDir::toNativeSeparators(Files[i].absoluteFilePath()),ThumbnailSize));
         if (iNumber>*NextNumber) 
            *NextNumber = iNumber;
      }
   }
}

//=============================================================================================================================

int cModelList::SearchModel(QString ModelName) 
{
   for(int i = 0; i < List.count(); i++ )
      if( List.at(i)->Name == ModelName )
         return i;
   return -1;
}

//=============================================================================================================================

QDomDocument cModelList::GetModelDocument(QString ModelName) 
{
   for(int i = 0; i < List.count(); i++ )
      if( List.at(i)->Name == ModelName )
         return List.at(i)->Model;
   return QDomDocument();
}
//=============================================================================================================================

cModelListItem *cModelList::AppendCustomModel() 
{
    RemoveCustomModel();
    List.append(new cModelListItem(this,"*",ThumbnailSize));
    return List[List.count()-1];
}

//=============================================================================================================================

void cModelList::RemoveCustomModel() 
{
   if (List.count() > 0 && List[List.count()-1]->Name == "*") 
      List.removeLast();
}
