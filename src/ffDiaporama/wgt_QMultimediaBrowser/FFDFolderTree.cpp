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

#include "FFDFolderTree.h"

#include <QFileInfoList>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QMenu>

#include <errno.h>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <QSettings>
    #include <QPixmapCache>
#endif

#define TAG "<to expand>"
//====================================================================================================================

FFDFolderTree::FFDFolderTree(QWidget *parent):QTreeWidget(parent) 
{
   ApplicationConfig = NULL;
   FolderTable = NULL;
   IsRemoveAllowed = false;
   IsRenameAllowed = false;
   IsCreateFolderAllowed = false;

   setContextMenuPolicy(Qt::CustomContextMenu);
   connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(s_itemExpanded(QTreeWidgetItem *)));
   connect(this, SIGNAL(customContextMenuRequested(const QPoint)), this, SLOT(s_ContextMenu(const QPoint)));
}

//====================================================================================================================

void FFDFolderTree::InitDrives() 
{
    foreach(cDriveDesc HDD,ApplicationConfig->DriveList->drives)
    #if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
        if (HDD.Path.startsWith("/media/")
            ||(HDD.Path=="/")
            ||(HDD.Label==PersonalFolder)
            ||(HDD.Label==QApplication::translate("FFDFolderTree","Clipart"))
            ||(HDD.Path.startsWith("/mnt/")&&FolderTable->ShowMntDrive)
        )
    #endif
            addTopLevelItem(CreateItem(HDD.Label,HDD.Path,QIcon(QPixmap().fromImage(HDD.IconDrive))));
}

//====================================================================================================================

void FFDFolderTree::keyReleaseEvent(QKeyEvent *event) 
{
   if (event->matches(QKeySequence::Delete)) 
   { 
      if (IsRemoveAllowed) 
         emit ActionRemoveFolder(); 
   }
   else if (event->key() == Qt::Key_F5)     
      emit ActionRefreshAll();
   else 
      QTreeWidget::keyReleaseEvent(event);
}

//====================================================================================================================

void FFDFolderTree::s_ContextMenu(const QPoint) 
{
   QMenu *ContextMenu = new QMenu(this);
   ContextMenu->addAction(CreateMenuAction(QApplication::style()->standardIcon(QStyle::SP_BrowserReload),         QApplication::translate("FFDFolderTree","Refresh all"),1,false,false,this));       //":/img/Refresh.png"
   ContextMenu->addAction(CreateMenuAction(QApplication::style()->standardIcon(QStyle::SP_BrowserReload),         QApplication::translate("FFDFolderTree","Refresh from here"),2,false,false,this)); //":/img/Refresh.png"
   if (IsCreateFolderAllowed)  
      ContextMenu->addAction(CreateMenuAction(QApplication::style()->standardIcon(QStyle::SP_FileDialogNewFolder),QApplication::translate("FFDFolderTree","Create new subfolder"),3,false,false,this));
   if (IsRemoveAllowed)        
      ContextMenu->addAction(CreateMenuAction(QApplication::style()->standardIcon(QStyle::SP_TrashIcon),          QApplication::translate("FFDFolderTree","Remove folder"),4,false,false,this));   //":/img/trash.png"
   if (IsRenameAllowed)        
      ContextMenu->addAction(CreateMenuAction(QIcon(":/img/action_edit.png"),                                     QApplication::translate("FFDFolderTree","Rename folder"),5,false,false,this));
   QAction *Action = ContextMenu->exec(QCursor::pos());
   if (Action) 
   {
      int ActionType = Action->data().toInt();
      QString SubFolderName = "";
      QString FolderName = "";
      QString FolderPath = "";
      bool Ok;
      switch (ActionType) 
      {
         case 1 : emit ActionRefreshAll();   break;  // Refresh all
         case 2 : emit ActionRefreshHere();  break;  // Refresh from here
         case 4 : emit ActionRemoveFolder(); break;  // Remove folder
         case 5 : emit ActionRenameFolder(); break;  // Rename folder
         case 3 :
            FolderPath = GetFolderPath(currentItem(),false);
            SubFolderName = CustomInputDialog(this,QApplication::translate("FFDFolderTree","Create folder"),QApplication::translate("FFDFolderTree","Folder:"),QLineEdit::Normal,"",&Ok);
            if (Ok && !SubFolderName.isEmpty()) 
            {
               if (!FolderPath.endsWith(QDir::separator())) 
                  FolderName = FolderPath + QDir::separator() + SubFolderName; 
               else 
                  FolderName = FolderPath + SubFolderName;
   #if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
               if (FolderName.startsWith("~")) FolderName=QDir::homePath()+FolderName.mid(1);
   #else
               if (FolderName.startsWith(PersonalFolder)) 
                  FolderName = QDir::homePath() + FolderName.mid(PersonalFolder.length());
               FolderName = QDir::toNativeSeparators(FolderName);
   #endif
               if (QDir().mkdir(FolderName)) 
                  RefreshItemByPath(GetFolderPath(currentItem(),true),false); 
               else 
               {
                  QString ErrorMsg = QString(QApplication::translate("FFDFolderTree","Error %1:")).arg(errno)+QString().fromLocal8Bit(strerror(errno));
                  CustomMessageBox(this,QMessageBox::Critical,QApplication::translate("FFDFolderTree","Create folder"),
                     QApplication::translate("FFDFolderTree","Impossible to create folder !")+"\n\n"+ErrorMsg,QMessageBox::Ok,QMessageBox::Ok);
               }
               FolderPath = GetFolderPath(currentItem(),true);
               if (!FolderPath.endsWith(QDir::separator())) 
                  FolderName = FolderPath + QDir::separator() + SubFolderName; 
               else 
                  FolderName = FolderPath + SubFolderName;
               SetSelectItemByPath(FolderName);
            }
            break; // Create new subfolder
      }
   }
   // delete menu
   while (ContextMenu->actions().count()) delete ContextMenu->actions().takeLast();
   delete ContextMenu;
}

//====================================================================================================================
// Private utility function to be use to know if a folder have child (depends on ShowHidden property)
//      FilePath : Path to check

bool FFDFolderTree::IsFolderHaveChild(QString FilePath) 
{
   if (FilePath.startsWith(QApplication::translate("FFDFolderTree","Clipart"))) 
      FilePath = ClipArtFolder+FilePath.mid(QApplication::translate("FFDFolderTree","Clipart").length());
#if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
   if (FilePath.startsWith("~")) FilePath=QDir::homePath()+FilePath.mid(1);
#else
   if (FilePath.startsWith(PersonalFolder)) 
      FilePath = QDir::homePath() + FilePath.mid(PersonalFolder.length());
   FilePath = QDir::toNativeSeparators(FilePath);
#endif
   QFileInfoList List = QDir(FilePath).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | (FolderTable->ShowHiddenFilesAndDir ? QDir::Hidden : QDir::Dirs));
   int i = 0; 
   while (i < List.count()) 
      if ((FolderTable->ShowHiddenFilesAndDir) || (!List[i].fileName().startsWith("."))) 
         i++; 
      else 
         List.removeAt(i);
   return List.count() > 0;
}

//====================================================================================================================
// Private utility function to be use to know if drive's folder is readonly (CD/DVD ROM)
//      FilePath : Path to check

bool FFDFolderTree::IsReadOnlyDrive(QString Folder) 
{
   bool IsReadOnly = false;
   for (int i = 0; i < ApplicationConfig->DriveList->drives.count(); i++)
      if (Folder.startsWith(ApplicationConfig->DriveList->drives[i].Path))
      {
         IsReadOnly = ApplicationConfig->DriveList->drives[i].IsReadOnly;
         break;
      }
   return IsReadOnly;
}

//====================================================================================================================
// Private utility function to create a new QTreeWidgetItem
//      Text     : Text to be shown
//      Icon     : Icon for the Item
//      FilePath : Path to get Icon

QTreeWidgetItem *FFDFolderTree::CreateItem(QString Text,QString FilePath,QIcon Icon) 
{
   QTreeWidgetItem *Current = new QTreeWidgetItem();
   Current->setText(0,Text);
   Current->setIcon(0,Icon);
   if (IsFolderHaveChild(FilePath)) 
   {
      QTreeWidgetItem *SubItem = new QTreeWidgetItem();
      SubItem->setText(0,TAG);                            // add a tag to sub item
      Current->addChild(SubItem);
   }
   return Current;
}

//====================================================================================================================
// Public utility function to get Path from an Item
//      Item     : Item to get Path from
//      TreeMode : if true, don't make alias interpretation

QString FFDFolderTree::GetFolderPath(const QTreeWidgetItem *Item, bool TreeMode)
{
   if (!Item) 
      return "";

   QString FilePath;

   // Parse Item to Compose path
   while (Item != NULL)
   {
      // if root item
      if (!Item->parent())
      {
         QString RootStr = Item->text(0);
         // For windows we display drive like D:\[Label], then cut after [ if exist
         if (Item->text(0).indexOf("[") != -1)
            RootStr = Item->text(0).left(Item->text(0).indexOf("["));
         if (!TreeMode)
         {
            // Search if text is a registered alias, then replace text with path
            for (int i = 0; i < ApplicationConfig->DriveList->drives.count(); i++)
               if (ApplicationConfig->DriveList->drives[i].Label == RootStr)
               {
                  if (RootStr != PersonalFolder && RootStr != QApplication::translate("FFDFolderTree", "Clipart"))
                     RootStr = ApplicationConfig->DriveList->drives[i].Path;
#if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
                  else if (RootStr == PersonalFolder) RootStr = "~";
#endif
                  else if (RootStr == QApplication::translate("FFDFolderTree", "Clipart"))
                     RootStr = QApplication::translate("FFDFolderTree", "Clipart");
               }
         }
         if (!RootStr.endsWith(QDir::separator()))
            RootStr = RootStr + QDir::separator();
         FilePath = RootStr + FilePath;

         Item = NULL;
      }
      else
      {
         FilePath = Item->text(0) + ((Item->text(0) != QDir().separator()) && (FilePath != QString("")) ? QString(QDir().separator()) : QString("")) + FilePath;
         Item = Item->parent();
      }
   }
   return FilePath;
}

//====================================================================================================================
// Public utility function to get Path from from the current selected Item

QString FFDFolderTree::GetCurrentFolderPath() {
    return GetFolderPath(currentItem(),false);
}

//====================================================================================================================
// we use this signal function instead of overloaded itemExpanded function because we need to modify item and
// overloaded function use const !

void FFDFolderTree::s_itemExpanded(QTreeWidgetItem *item) 
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Expand node if necessary
    if (item->childCount()==1 && item->child(0)->text(0)==TAG)
    {
        setUpdatesEnabled(false);

        QTreeWidgetItem     *CurItem=item;
        QString             Folder=GetFolderPath(CurItem,false);
        QFileInfoList       Directorys;
        int                 i,k;

        if (Folder.startsWith(QApplication::translate("FFDFolderTree","Clipart"))) 
           Folder=ClipArtFolder+Folder.mid(QApplication::translate("FFDFolderTree","Clipart").length());
        #if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
            if (Folder.startsWith("~")) Folder=QDir::homePath()+Folder.mid(1);
        #else
            if (Folder.startsWith(PersonalFolder)) 
               Folder=QDir::homePath()+Folder.mid(PersonalFolder.length());
            Folder=QDir::toNativeSeparators(Folder);
        #endif

        // remove tag to sub item
        QTreeWidgetItem *SubItem=CurItem->child(0);
        CurItem->removeChild(SubItem);
        delete SubItem;

        #ifdef Q_OS_WIN
            bool IsPersonalFolder=Folder.startsWith(PersonalFolder);
            if (IsPersonalFolder) Folder=QDir::homePath()+Folder.mid(PersonalFolder.length());
            Folder=QDir::toNativeSeparators(Folder);
        #endif
            Directorys = QDir(Folder).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | (FolderTable->ShowHiddenFilesAndDir ? QDir::Hidden : QDir::Dirs));
            i = 0;
            while (i < Directorys.count())
               if (FolderTable->ShowHiddenFilesAndDir || !Directorys[i].fileName().startsWith("."))
                  i++;
               else 
                  Directorys.removeAt(i);
            for (k = 0; k < Directorys.count(); k++)
               if (Directorys[k].isDir())
               {
#ifdef Q_OS_WIN
                  if (IsPersonalFolder)
                     CurItem->addChild(CreateItem(Directorys[k].fileName(),PersonalFolder+QDir::separator()+Directorys[k].fileName(),ApplicationConfig->DriveList->GetFolderIcon(Directorys[k].absoluteFilePath())));
                  else
#endif
                     CurItem->addChild(CreateItem(Directorys[k].fileName(), Directorys[k].absoluteFilePath(), ApplicationConfig->DriveList->GetFolderIcon(Directorys[k].absoluteFilePath())));
               }

        // Unselect previous selected item and then select new one
        QList<QTreeWidgetItem *>    List  =selectedItems();
        bool                        IsFind=false;

        for (i = 0; i < List.count(); i++)
           if (List[i] == CurItem)
              IsFind = true;
           else
              List[i]->setSelected(false);

        if (!IsFind) setCurrentItem(CurItem);

        setUpdatesEnabled(true);
    }
    QApplication::restoreOverrideCursor();
}

//====================================================================================================================

cDriveDesc *FFDFolderTree::SearchRealDrive(QString Path) 
{
#if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
   if (Path.startsWith("~")) 
      Path=QDir::homePath()+Path.mid(1);
#else
   if (Path.startsWith(PersonalFolder))
      Path = QDir::homePath() + Path.mid(PersonalFolder.length());
   Path = QDir::toNativeSeparators(Path);
#endif

   if (Path.endsWith(QDir::separator()))
      Path = Path.left(Path.length() - 1);    // Remove endest separator
   if (QDir(Path).canonicalPath() != "")
      Path = QDir(Path).canonicalPath();         // Convert path to physical path
   Path = QDir::toNativeSeparators(Path);
   if (!Path.endsWith(QDir::separator()))
      Path = Path + QDir::separator();             // Add separator to find drive in our list
   for (int i = 0; i < ApplicationConfig->DriveList->drives.count(); i++)
      if ((ApplicationConfig->DriveList->drives[i].Path != QString("/")) && (Path.startsWith(ApplicationConfig->DriveList->drives[i].Path)))
         return &ApplicationConfig->DriveList->drives[i];
   for (int i = 0; i < ApplicationConfig->DriveList->drives.count(); i++)
      if (ApplicationConfig->DriveList->drives[i].Path == QString("/"))
         return &ApplicationConfig->DriveList->drives[i];
   return NULL;
}

//====================================================================================================================

QString FFDFolderTree::RealPathToTreePath(QString Path) 
{
   for (int i = 0; i < ApplicationConfig->DriveList->drives.count(); i++)
      if (ApplicationConfig->DriveList->drives[i].Path != QString("/") && Path.startsWith(ApplicationConfig->DriveList->drives[i].Path))
      {
         if (!ApplicationConfig->DriveList->drives[i].Path.endsWith(QDir::separator())) 
            Path = ApplicationConfig->DriveList->drives[i].Label + Path.mid(ApplicationConfig->DriveList->drives[i].Path.length());
         else 
            Path = ApplicationConfig->DriveList->drives[i].Label + Path.mid(ApplicationConfig->DriveList->drives[i].Path.length() - 1);
         break;
      }
   return Path;
}

//====================================================================================================================
static QString RemoveLabel(QString Path) 
{
#ifdef Q_OS_WIN
   int i = Path.indexOf("[");
   if (i > 0) 
      Path = Path.left(i);
#endif
   return Path;
}

void FFDFolderTree::SetSelectItemByPath(QString Path) 
{
   //qDebug() << "start SetSelectItemByPath for " << Path;
   //AUTOTIMER(t,"SetSelectItemByPath");
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   setUpdatesEnabled(false);

   QFileInfoList   Directorys;
   int             i, j, k, NbrChild;
   QStringList     Folders;
   QString         CurrentFolder;
   QTreeWidgetItem *Current = NULL;

#if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
   if (Path.startsWith("~")) 
      Path = PersonalFolder + Path.mid(1);
#else
   Path = QDir::toNativeSeparators(Path);
   // Remove labels
   int StartLabel = Path.indexOf("[");
   int EndLabel  = (StartLabel > 0 ? Path.indexOf("]\\") : 0);
   if (EndLabel > 0) 
      EndLabel += 2;
   if ((StartLabel > 0) && (EndLabel > StartLabel)) 
   {
      QString OldPath = Path;
      Path = OldPath.left(StartLabel)+OldPath.right(OldPath.length()-EndLabel);
   }
#endif

   // Create a list with each part of the wanted Path
   Path = QDir::cleanPath(Path);
   Folders = Path.split(QChar('/'), Qt::SkipEmptyParts);

    #if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
    if ((Folders.count() >= 1) && (Folders[0] == "")) 
      Folders[0] = QApplication::translate("FFDFolderTree","System files");
    #endif

    // find node in the tree and expand it if it was not previously expanded
    i = 0;
    Current = topLevelItem(0);

    while (Current != NULL && i < Folders.count() && Folders[i] != "") 
    {
       if (i == 0) 
       {
          // Search in topitemlist : Note : Search in reverse order to give preference to drive instead of /mnt/drive or /media/drive
          j = topLevelItemCount()-1;
          while (( j >= 0) && (RemoveLabel(topLevelItem(j)->text(0)) != Folders[i]) && (RemoveLabel(topLevelItem(j)->text(0)) != Folders[i] + QDir::separator())) 
            j--;
          if (( j >= 0) && ((RemoveLabel(topLevelItem(j)->text(0)) == Folders[i]) || (RemoveLabel(topLevelItem(j)->text(0)) == Folders[i]+QDir::separator())))
             Current = topLevelItem(j); 
          else 
            Current = NULL;
       } 
       else 
       {
          j = 0;
          // Search in current item child list
          while (( j < Current->childCount()) && (Current->child(j)->text(0).compare(Folders[i]) != 0) && (Current->child(j)->text(0) != Folders[i]+QDir::separator())) 
            j++;
          if (( j < Current->childCount()) && ((Current->child(j)->text(0) == Folders[i]) || (Current->child(j)->text(0).compare(Folders[i] + QDir::separator()) == 0))) 
            Current = Current->child(j);
          else 
            Current = NULL;
       }
       if (Current) 
       {
          CurrentFolder   = GetFolderPath(Current,false);
          NbrChild        = Current->childCount();
          if ((NbrChild == 1) && (Current->child(0)->text(0) == TAG)) 
          { // If not initialize
             // remove tag to sub item
             QTreeWidgetItem *SubItem = Current->child(0);
             Current->removeChild(SubItem);
             delete SubItem;
             QString RealPath = CurrentFolder;
             if (RealPath.startsWith(QApplication::translate("FFDFolderTree","Clipart"))) 
               RealPath = ClipArtFolder + RealPath.mid(QApplication::translate("FFDFolderTree","Clipart").length());
#if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
             if (RealPath.startsWith("~")) 
               RealPath = QDir::homePath() + RealPath.mid(1);
#else
             if (RealPath.startsWith(PersonalFolder)) 
               RealPath = QDir::homePath() + RealPath.mid(PersonalFolder.length());
             RealPath = QDir::toNativeSeparators(RealPath);
#endif
             Directorys = QDir(RealPath).entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot|(FolderTable->ShowHiddenFilesAndDir?QDir::Hidden:QDir::Dirs));
             int z = 0; 
             while ( z < Directorys.count()) 
               if ((FolderTable->ShowHiddenFilesAndDir) || (!Directorys[z].fileName().startsWith("."))) 
                  z++; 
               else 
                  Directorys.removeAt(z);
             for (k = 0; k < Directorys.count(); k++) 
               Current->addChild(CreateItem(Directorys[k].fileName(), Directorys[k].absoluteFilePath(), ApplicationConfig->DriveList->GetFolderIcon(Directorys[k].absoluteFilePath())));
          }
          Current->setExpanded(true);
       } 
       else 
         Current = NULL; // Error ????
       i++;
    }

    // Unselect previous selected item and then select new one
    if (Current) 
    {
       QList<QTreeWidgetItem *>  List = selectedItems();
       bool IsFind = false;
       for (i = 0; i < List.count(); i++) 
         if (List[i] == Current) 
            IsFind=true; 
         else 
            List[i]->setSelected(false);
         //AUTOTIMER(t,"setCurrentItem");
       if (!IsFind) 
         setCurrentItem(Current);
    }
    resizeColumnToContents(0);
    setUpdatesEnabled(true);

    QApplication::restoreOverrideCursor();
}

//====================================================================================================================

class cTreeItemDescriptor 
{
public:
    QTreeWidgetItem *Item;
    bool            IsFound;

    cTreeItemDescriptor(QTreeWidgetItem *TheItem) {
        Item=TheItem;
        IsFound=false;
    }
};

#ifdef Q_OS_WIN
QString CleanTextPath(QString Source) ;
//{
//   int i = Source.indexOf("[");
//   if (i >= 0) 
//      Source = Source.left(i);
//   return Source;
//}
#endif

void FFDFolderTree::RefreshItemByPath(QString Path, bool RefreshAll, int Level) 
{
   Path = QDir::toNativeSeparators(Path);
   QString RealPath = Path;
   int     i, j;

   // Construct RealPath
   for (i = 0; i < ApplicationConfig->DriveList->drives.count(); i++) 
      if (RealPath.startsWith(ApplicationConfig->DriveList->drives[i].Label))
      {
         QString PartPath = RealPath.mid(ApplicationConfig->DriveList->drives[i].Label.length());
         while (PartPath.startsWith(QDir::separator())) 
            PartPath = PartPath.mid(1);
         while (PartPath.endsWith(QDir::separator()))   
            PartPath = PartPath.left(PartPath.length() - 1);
         RealPath = ApplicationConfig->DriveList->drives[i].Path;
         if (!RealPath.endsWith(QDir::separator())) 
            RealPath = RealPath + QDir::separator();
         RealPath = RealPath + PartPath;
         break;
      }
   if (Level == 0 && IsReadOnlyDrive(RealPath)) 
      return;

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   QString             PartPath = Path;
   QStringList         Folders;
   QTreeWidgetItem     *Current = NULL;
   QTreeWidgetItem     *SubItem = NULL;

    // Adjust Path
    #if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
    Path.replace("~",PersonalFolder);
    #endif
    if (Path.startsWith(QApplication::translate("FFDFolderTree","Clipart"))) 
      Path = ClipArtFolder + Path.mid(QApplication::translate("FFDFolderTree","Clipart").length());

    // Create a list with each part of the wanted Path
    while (PartPath.contains(QDir::separator())) 
    {
        Folders.append(PartPath.mid(0,PartPath.indexOf(QDir::separator())));
        PartPath = PartPath.mid(PartPath.indexOf(QDir::separator()) + QString(QDir::separator()).length());
    }
    if (PartPath != "") 
       Folders.append(PartPath);

    // Now we can search corresponding item in the tree
    i=0;
    #ifdef Q_OS_WIN
    while ((Folders.count()>0) && (i<this->topLevelItemCount()) && (CleanTextPath(topLevelItem(i)->text(0))!=Folders[0]) && (CleanTextPath(topLevelItem(i)->text(0))!=Folders[0]+QDir::separator()))  
      i++;
    if ((Folders.count()>0)&&(i<this->topLevelItemCount())&&((CleanTextPath(topLevelItem(i)->text(0))==Folders[0])||(CleanTextPath(topLevelItem(i)->text(0))==Folders[0]+QDir::separator()))) 
    {
    #else
    while ((Folders.count()>0)&&(i<this->topLevelItemCount())&&(topLevelItem(i)->text(0)!=Folders[0])&&(topLevelItem(i)->text(0)!=Folders[0]+QDir::separator())) 
       i++;
    if ((Folders.count()>0)&&(i<this->topLevelItemCount())&&((topLevelItem(i)->text(0)==Folders[0])||(topLevelItem(i)->text(0)==Folders[0]+QDir::separator()))) 
    {
    #endif
        // We have found the toplevel, now down the tree
        Current = topLevelItem(i);
        j = 1;
        while (Current && j < Folders.count()) 
        {
            i=0;
            while ((i<Current->childCount())&&(Current->child(i)->text(0)!=Folders[j])&&(Current->child(i)->text(0)!=Folders[j]+QDir::separator())) 
               i++;
            if ((i<Current->childCount())&&((Current->child(i)->text(0)==Folders[j])||(Current->child(i)->text(0)==Folders[j]+QDir::separator()))) 
            {
                Current = Current->child(i);
                j++;
            } 
            else 
               Current = NULL;
        }
    }

    // Don't check directory if unexpanded TAG is set !
    if ((Current)&&(Current->childCount()==1)&&(Current->child(0)->text(0)==TAG))
        Current=NULL;

    // if item found
    if (Current) 
    {
        if (Level==0) setUpdatesEnabled(false);

        // Set icon for this current item
        Current->setIcon(0,ApplicationConfig->DriveList->GetFolderIcon(RealPath));

        // Check if havechild status have change
        if (IsFolderHaveChild(RealPath)) {
            // if folder now have child
            if (Current->childCount()==0) {
                QTreeWidgetItem *SubItem=new QTreeWidgetItem();
                SubItem->setText(0,TAG);                            // add a tag to sub item
                Current->addChild(SubItem);
            }
        } else {
            // if folder no longer have child
            while (Current->childCount()>0) {
                SubItem=Current->child(0);
                DeleteChildItem(SubItem);
                Current->removeChild(SubItem);
                delete SubItem;
            }
        }

        // Construct a list with actual known folders
        QList<cTreeItemDescriptor>  CurrentList;
        for (i=0;i<Current->childCount();i++) CurrentList.append(cTreeItemDescriptor(Current->child(i)));

        // Construct a second list for real folders
        QFileInfoList   Directorys=QDir(RealPath).entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot|(FolderTable->ShowHiddenFilesAndDir?QDir::Hidden:QDir::Dirs));
        int z=0; while (z<Directorys.count()) if ((FolderTable->ShowHiddenFilesAndDir)||(!Directorys[z].fileName().startsWith("."))) z++; else Directorys.removeAt(z);

        QStringList     FolderToAdd;
        bool            isFound;

        for (i=0;i<Directorys.count();i++) {
            isFound=false;
            // for each item search if it is present in current list
            for (j=0;j<CurrentList.count();j++) if (Directorys[i].fileName()==CurrentList[j].Item->text(0)) {
                CurrentList[j].IsFound=true;
                isFound=true;
                break;
            }
            // if folder not found add it to to FolderToAdd List
            if (!isFound) FolderToAdd.append(Directorys[i].fileName());
        }

        // Now we have 2 lists to work with
        for (i=0;i<CurrentList.count();i++) {
            // update all previously existing folder
            if (CurrentList[i].IsFound) {
                QString ChildPath=Path;
                if (!ChildPath.endsWith(QDir::separator())) ChildPath=ChildPath+QDir::separator();
                ChildPath=ChildPath+CurrentList[i].Item->text(0);
                if (RefreshAll) RefreshItemByPath(ChildPath,RefreshAll,Level+1);
            } else {
                // remove existing folder which no longer exist
                SubItem=CurrentList[i].Item;
                DeleteChildItem(SubItem);
                Current->removeChild(SubItem);
                delete SubItem;
            }
        }
        // add new folder
        QString ChildPath=QDir().absoluteFilePath(RealPath);
        if (!ChildPath.endsWith(QDir::separator())) ChildPath=ChildPath+QDir::separator();
        for (i=0;i<FolderToAdd.count();i++) {
            int ItemBefore=0;
            while ((ItemBefore<Current->childCount())&&(FolderToAdd[i]>=Current->child(ItemBefore)->text(0))) ItemBefore++;
            Current->insertChild(ItemBefore,CreateItem(FolderToAdd[i],ChildPath+FolderToAdd[i],ApplicationConfig->DriveList->GetFolderIcon(ChildPath+FolderToAdd[i])));
        }

        if (Level==0) setUpdatesEnabled(true);
    }

    QApplication::restoreOverrideCursor();
}

//====================================================================================================================

void FFDFolderTree::DeleteChildItem(QTreeWidgetItem *Item) {
    while (Item->childCount()!=0) {
        QTreeWidgetItem *SubItem=Item->child(0);
        DeleteChildItem(SubItem);
        Item->removeChild(SubItem);
        delete SubItem;
    }
}

//====================================================================================================================

void FFDFolderTree::RefreshDriveList() {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    ApplicationConfig->DriveList->UpdateDriveList();
    int i=0;
    while (i<ApplicationConfig->DriveList->drives.count()) {
        if (ApplicationConfig->DriveList->drives[i].Label==QApplication::translate("FFDFolderTree","Clipart"))
            ApplicationConfig->DriveList->drives[i].Flag=1;
        if (ApplicationConfig->DriveList->drives[i].Flag==0) {
            // Drive no longer exist
            int j=0;
            while ((j<topLevelItemCount())&&(topLevelItem(j)->text(0)!=ApplicationConfig->DriveList->drives[i].Label)) j++;
            if ((j<topLevelItemCount())&&(topLevelItem(j)->text(0)==ApplicationConfig->DriveList->drives[i].Label)) {
                // We have found the item in the tree
                QTreeWidgetItem *Item=takeTopLevelItem(j);
                DeleteChildItem(Item);                      // Delete item in tree
                delete Item;
            }
            ApplicationConfig->DriveList->drives.removeAt(i);                    // Delete from drive list
        } else if (ApplicationConfig->DriveList->drives[i].Flag==1) {
            // Drive previously exist
            #if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
            if (ApplicationConfig->DriveList->drives[i].Path.startsWith("/media/")
                ||(ApplicationConfig->DriveList->drives[i].Path=="/")
                ||(ApplicationConfig->DriveList->drives[i].Label==PersonalFolder)
                ||(ApplicationConfig->DriveList->drives[i].Label==QApplication::translate("FFDFolderTree","Clipart"))
                ||(ApplicationConfig->DriveList->drives[i].Path.startsWith("/mnt/"))
            ) {
            #endif
                int j=0;
                while ((j<topLevelItemCount())&&(topLevelItem(j)->text(0)!=ApplicationConfig->DriveList->drives[i].Label)) j++;
                if ((j<topLevelItemCount())&&(topLevelItem(j)->text(0)==ApplicationConfig->DriveList->drives[i].Label)) {
                    // if drive is not a /mnt/ drive and if we not continu to display them, then delete it
                    if ((ApplicationConfig->DriveList->drives[i].Path.startsWith("/mnt/"))&&
                        (!FolderTable->ShowMntDrive)&&
                        (ApplicationConfig->DriveList->drives[i].Label!=QApplication::translate("FFDFolderTree","Clipart"))) {
                        QTreeWidgetItem *Item=takeTopLevelItem(j);
                        DeleteChildItem(Item);                      // Delete item in tree
                        delete Item;
                    }
                } else {
                    // ShowMntDrive have changed, we have to create it
                    if ((!ApplicationConfig->DriveList->drives[i].Path.startsWith("/mnt/"))||(FolderTable->ShowMntDrive))
                        addTopLevelItem(CreateItem(ApplicationConfig->DriveList->drives[i].Label,ApplicationConfig->DriveList->drives[i].Path,QIcon(QPixmap().fromImage(ApplicationConfig->DriveList->drives[i].IconDrive))));
                }
            #if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
            }
            #endif
            i++;
        } else {
            // It's a new drive
            if ((!ApplicationConfig->DriveList->drives[i].Path.startsWith("/mnt/"))||(FolderTable->ShowMntDrive))
                addTopLevelItem(CreateItem(ApplicationConfig->DriveList->drives[i].Label,ApplicationConfig->DriveList->drives[i].Path,QIcon(QPixmap().fromImage(ApplicationConfig->DriveList->drives[i].IconDrive))));
            i++;
        }
    }
    QApplication::restoreOverrideCursor();
}
