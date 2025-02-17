/*======================================================================
    This file is part of ffDiaporama
    ffDiaporama is a tool to make diaporama as video
    Copyright (C) 2011-2013 Dominique Levray<domledom@laposte.net>

    This program is free software;you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation;either version 2 of the License,or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY;without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program;if not,write to the Free Software Foundation,Inc.,
    51 Franklin Street,Fifth Floor,Boston,MA 02110-1301 USA.
  ======================================================================*/

#include "cDatabase.h"
#include "cBaseAppConfig.h"
#define DATABASEVERSION 9       // Current database version

#define NPROPS
void DisplayLastSQLError(QSqlQuery *Query) 
{
    ToLog(LOGMSG_CRITICAL,Query->lastQuery());
    //ToLog(LOGMSG_CRITICAL, QString("Error %1:%2").arg(Query->lastError().number()).arg(Query->lastError().text()));
    ToLog(LOGMSG_CRITICAL, QString("Error %1:%2").arg(Query->lastError().nativeErrorCode()).arg(Query->lastError().text()));
}

//**************************************************************************************************************************
// cDatabase : encapsulate a SQLite3 database
//**************************************************************************************************************************

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))    
cDatabase::cDatabase(QString DBFNAME) : dbMutex()
#else
cDatabase::cDatabase(QString DBFNAME) : dbMutex(QMutex::Recursive)
#endif
{
   dbPath = DBFNAME;
   ApplicationConfig = NULL;
}

//=====================================================================================================

cDatabase::~cDatabase() 
{
   while (!Tables.isEmpty()) 
      delete Tables.takeLast();
   CloseDB();
}

//=====================================================================================================

bool cDatabase::OpenDB() 
{
   // Find QSLite driver
   db = QSqlDatabase::addDatabase("QSQLITE");

   QDir ApplicationFolder(QFileInfo(dbPath).absolutePath());
   if (!ApplicationFolder.exists()) 
      ApplicationFolder.mkpath(ApplicationFolder.path());
   db.setDatabaseName(dbPath);

   // Open databasee
   if (!db.open()) 
      return false;

   // Disable journalisation to speed query
   QSqlQuery Query(db);
   if (!Query.exec("PRAGMA journal_mode=OFF")) 
   {
      DisplayLastSQLError(&Query);
      return false;
   }
   return true;
}

//=====================================================================================================
// Close database
bool cDatabase::CloseDB() 
{
   if (db.isOpen()) 
      db.close();
   return true;
}

//=====================================================================================================
// Reset database and recreate it
bool cDatabase::ResetDB() 
{
   bool Ret = CloseDB();
   if (QFileInfo(dbPath).exists()) 
      Ret = Ret && QFile::remove(dbPath);
   Ret = Ret && OpenDB();
   if (db.isOpen()) 
      foreach (cDatabaseTable *Table,Tables) 
         Ret = Ret && Table->CreateTable();
   return db.isOpen();
}

//=====================================================================================================
// Return table object corresponding to TableName
cDatabaseTable *cDatabase::GetTable(eTypeTable TableType) 
{
   for(auto Table : Tables)
      if (Table->TypeTable == TableType)
         return Table;
   return NULL;
}

//=====================================================================================================
// Get database version from setting table and upgrade database if needed
bool cDatabase::CheckDatabaseVersion() 
{
   cSettingsTable  *SettingsTable = (cSettingsTable *)GetTable(TypeTable_SettingsTable);
   bool Ret = SettingsTable != NULL;
   if (Ret)
   {
      qlonglong DatabaseVersion = SettingsTable->GetIntValue("Version", 0);
      if (DatabaseVersion < DATABASEVERSION)
      {
         for(auto Table : Tables) 
            Ret = Ret && Table->DoUpgradeTableVersion(DatabaseVersion);
         SettingsTable->SetIntValue("Version", DATABASEVERSION);
      }
   }
   return Ret;
}

//=====================================================================================================
// Validate each table one by one
bool cDatabase::ValidateTables() 
{
   bool Ret = db.isOpen();
   for(auto Table : Tables) 
      Ret = Ret && Table->ValidateTable();
   return Ret;
}

//=====================================================================================================
QSqlError cDatabase::LastError() 
{
   return db.lastError();
}

//**************************************************************************************************************************
// cDatabaseTable : encapsulate a table
//      this class must not been used directly but must be use as ancestor by all table class
//**************************************************************************************************************************
cDatabaseTable::cDatabaseTable(cDatabase *Database) 
{
   this->Database = Database;
   NextIndex = 0;
   TypeTable = TypeTable_Undefined;
}

//=====================================================================================================
cDatabaseTable::~cDatabaseTable() 
{
}

//=====================================================================================================
bool cDatabaseTable::ValidateTable() 
{
   QSqlQuery Query(Database->db);
   if (Query.exec(QString("SELECT MAX(%1) FROM %2").arg(IndexKeyName).arg(TableName)))
   {
      bool Ret = true;
      while (Query.next())
      {
         QVariant Value = Query.value(0);
         if (!Value.isNull()) 
            NextIndex = Value.toLongLong(&Ret);
      }
      if (!Ret) 
         DisplayLastSQLError(&Query);
      return Ret;
   }
   else
   {
      //if (Query.lastError().number() == 1)
      if (Query.lastError().nativeErrorCode() == "1") // ??? tocheck
            return CreateTable();
      else
      {
         DisplayLastSQLError(&Query);
         return false;
      }
   }
}

//=====================================================================================================
bool cDatabaseTable::CreateTable() 
{
   if (!CreateTableQuery.isEmpty())
   {
      QSqlQuery Query(Database->db);
      bool Ret = Query.exec(CreateTableQuery);
      if (!Ret) 
         DisplayLastSQLError(&Query);
      for(const auto IndexQuery : CreateIndexQuery)
         Ret = Ret && Query.exec(IndexQuery);
      if (!Ret) 
         DisplayLastSQLError(&Query);
      return Ret;
   }
   return false;
}

//=====================================================================================================

bool cDatabaseTable::DoUpgradeTableVersion(qlonglong) 
{
    return true;
}

//**************************************************************************************************************************
// cSettingsTable : encapsulate the settings table
//**************************************************************************************************************************
cSettingsTable::cSettingsTable(cDatabase *Database):cDatabaseTable(Database) 
{
    TypeTable = TypeTable_SettingsTable;
    TableName = "Settings";
    IndexKeyName = "Key";
    CreateTableQuery = "create table Settings ("\
                            "Key                bigint primary key,"\
                            "Name               varchar(100),"\
                            "IntValue           bigint,"\
                            "TextValue          text"
                     ")";
    CreateIndexQuery.append("CREATE INDEX idx_Settings_Key ON Settings (Key)");
    CreateIndexQuery.append("CREATE INDEX idx_Settings_Name ON Settings (Name)");
}

//=====================================================================================================

bool cSettingsTable::CreateTable() 
{
   return cDatabaseTable::CreateTable() && SetIntValue("Version",DATABASEVERSION);
}

//=====================================================================================================

qlonglong cSettingsTable::GetIntValue(QString SettingName,qlonglong DefaultValue)
{
   QMutexLocker locker(&Database->dbMutex);
   QSqlQuery Query(Database->db);
   qlonglong  RetValue = DefaultValue;

   Query.prepare(QString("SELECT IntValue FROM %1 WHERE Name=:Name").arg(TableName));
   Query.bindValue(":Name",SettingName,QSql::In);
   if (!Query.exec()) 
      DisplayLastSQLError(&Query); 
   else while (Query.next()) 
   {
      QVariant Value = Query.value(0);
      if (!Value.isNull()) 
      {
         bool Ret = false;
         RetValue = Value.toLongLong(&Ret);
         if (!Ret) 
            RetValue = DefaultValue;
      }
   }
   return RetValue;
}

//=====================================================================================================
bool cSettingsTable::SetIntValue(QString SettingName,qlonglong Value) 
{
   QMutexLocker locker(&Database->dbMutex);
   QSqlQuery Query(Database->db);
   bool Ret = true;

   Query.prepare(QString("UPDATE %1 set IntValue=:IntValue WHERE Name=:Name").arg(TableName));
   Query.bindValue(":IntValue",Value,      QSql::In);
   Query.bindValue(":Name",    SettingName,QSql::In);
   if (!Query.exec() || Query.numRowsAffected() == 0) 
   {
      Query.prepare(QString("INSERT INTO %1 (Key,Name,IntValue) VALUES (:Key,:Name,:IntValue)").arg(TableName));
      Query.bindValue(":Key",     ++NextIndex,QSql::In);
      Query.bindValue(":IntValue",Value,      QSql::In);
      Query.bindValue(":Name",    SettingName,QSql::In);
      Ret = Query.exec();
      if (!Ret) 
         DisplayLastSQLError(&Query);
   }
   return Ret;
}

//=====================================================================================================

QString cSettingsTable::GetTextValue(QString SettingName,QString DefaultValue) 
{
   QMutexLocker locker(&Database->dbMutex);
   QSqlQuery Query(Database->db);
   QString RetValue = DefaultValue;

   Query.prepare(QString("SELECT TextValue FROM %1 WHERE Name=:Name").arg(TableName));
   Query.bindValue(":Name",SettingName,QSql::In);
   if (!Query.exec()) 
      DisplayLastSQLError(&Query); 
   else while (Query.next()) 
   {
      QVariant Value = Query.value(0);
      if (!Value.isNull()) 
         RetValue = Value.toString();
   }
   return RetValue;
}

//=====================================================================================================

bool cSettingsTable::SetTextValue(QString SettingName,QString Value) 
{
   QMutexLocker locker(&Database->dbMutex);
   QSqlQuery Query(Database->db);
   bool Ret = true;

   Query.prepare(QString("UPDATE %1 set TextValue=:TextValue WHERE Name=:Name").arg(TableName));
   Query.bindValue(":TextValue",Value,      QSql::In);
   Query.bindValue(":Name",     SettingName,QSql::In);
   if (!Query.exec() || Query.numRowsAffected() == 0) 
   {
      Query.prepare(QString("INSERT INTO %1 (Key,Name,TextValue) VALUES (:Key,:Name,:TextValue)").arg(TableName));
      Query.bindValue(":Key",      ++NextIndex,QSql::In);
      Query.bindValue(":TextValue",Value,      QSql::In);
      Query.bindValue(":Name",     SettingName,QSql::In);
      Ret = Query.exec();
      if (!Ret) 
         DisplayLastSQLError(&Query);
   }
   return Ret;
}

//=====================================================================================================

bool cSettingsTable::GetIntAndTextValue(QString SettingName,qlonglong *IntValue,QString *TextValue) 
{
   QMutexLocker locker(&Database->dbMutex);
   QSqlQuery Query(Database->db);
   bool Ret = false;

   Query.prepare(QString("SELECT IntValue,TextValue FROM %1 WHERE Name=:Name").arg(TableName));
   Query.bindValue(":Name",SettingName,QSql::In);
   if (!Query.exec()) 
      DisplayLastSQLError(&Query); 
   else while (Query.next()) 
   {
      Ret = true;
      if (Query.value(0).isNull()) 
         Ret = false; 
      else 
         *IntValue = Query.value(0).toLongLong(&Ret);
      if (Query.value(1).isNull()) 
         Ret = false; 
      else 
         *TextValue = Query.value(1).toString();
   }
   return Ret;
}

//=====================================================================================================

bool cSettingsTable::SetIntAndTextValue(QString SettingName,qlonglong IntValue,QString TextValue) 
{
   QMutexLocker locker(&Database->dbMutex);
   bool Ret = true;
   QSqlQuery Query(Database->db);

   Query.prepare(QString("UPDATE %1 set TextValue=:TextValue,IntValue=:IntValue WHERE Name=:Name").arg(TableName));
   Query.bindValue(":TextValue",TextValue,  QSql::In);
   Query.bindValue(":IntValue", IntValue,   QSql::In);
   Query.bindValue(":Name",     SettingName,QSql::In);
   if (!Query.exec() || Query.numRowsAffected()==0) 
   {
      Query.prepare(QString("INSERT INTO %1 (Key,Name,IntValue,TextValue) VALUES (:Key,:Name,:IntValue,:TextValue)").arg(TableName));
      Query.bindValue(":Key",      ++NextIndex,QSql::In);
      Query.bindValue(":TextValue",TextValue,  QSql::In);
      Query.bindValue(":IntValue", IntValue,   QSql::In);
      Query.bindValue(":Name",     SettingName,QSql::In);
      Ret = Query.exec();
      if (!Ret) 
         DisplayLastSQLError(&Query);
   }
   return Ret;
}

//**********************************************************************************************
// cFolderTable : encapsulate folders in the table
//**********************************************************************************************

cFolderTable::cFolderTable(cDatabase *Database):cDatabaseTable(Database) 
{
   TypeTable = TypeTable_FolderTable;
   TableName = "Folders";
   IndexKeyName = "Key";
   CreateTableQuery = "create table Folders ("\
      "Key                bigint primary key,"\
      "Name               varchar(512),"\
      "ParentKey          bigint,"\
      "Timestamp          bigint"
      ")";
   CreateIndexQuery.append("CREATE INDEX idx_Folders_Key  ON Folders (Key)");
   CreateIndexQuery.append("CREATE INDEX idx_Folders_Name ON Folders (ParentKey,Name)");
}

//=====================================================================================================

bool cFolderTable::DoUpgradeTableVersion(qlonglong OldVersion) 
{
   QSqlQuery Query(Database->db);
   bool Ret = true;

   if (OldVersion == 3) 
      Ret = Query.exec("DROP TABLE Folders");

   if (!Ret) 
      DisplayLastSQLError(&Query);
   return Ret;
}

//=====================================================================================================
// Get the key associated to a folder path
// If folder not found in the database, then create it and all his parents
qlonglong cFolderTable::GetFolderKey(QString FolderPath) 
{
   if (FolderPath=="." || FolderPath=="..") 
      return -1;
   QMutexLocker locker(&Database->dbMutex);
   FolderPath = QDir::toNativeSeparators(QDir(FolderPath).absolutePath());
   QMap<QString, qlonglong>::const_iterator it = folderMap.constFind(FolderPath);
   if( it != folderMap.cend() )
      return it.value();

#ifdef Q_OS_WIN
   // On windows, network share start with \\,  so keep this information in a boolean and remove this "\"
   bool IsNetworkShare = FolderPath.startsWith("\\\\");
   if (IsNetworkShare) 
      FolderPath = FolderPath.mid(QString("\\\\").length());
#endif
   QStringList FolderList = FolderPath.split(QDir::separator());
#ifdef Q_OS_WIN
   // On windows, add previously \\ removed before to create the list
   if ( IsNetworkShare && FolderList.count() > 0) 
      FolderList[0].prepend("\\\\");
#endif
   if (FolderList.last().isEmpty()) 
      FolderList.removeLast();
   qlonglong Key = -1;
   QString ParentPath;   
   if (FolderList.count() > 1) 
      for (int i = 0; i < FolderList.count()-1; i++) 
         ParentPath = ParentPath + FolderList[i] + QDir::separator();
   qlonglong ParentKey = (FolderList.count() > 1) ? GetFolderKey(ParentPath) : -1;
   if (FolderList.count() > 0) 
   {
      QSqlQuery Query(Database->db);
      bool Ret = false;
      Query.prepare(QString("SELECT Key FROM %1 WHERE ParentKey=:ParentKey AND Name=:Name").arg(TableName));
      Query.bindValue(":ParentKey",ParentKey,         QSql::In);
      Query.bindValue(":Name",     FolderList.last(), QSql::In);
      if (!Query.exec()) 
         DisplayLastSQLError(&Query); 
      else while (Query.next()) if (!Query.value(0).isNull()) 
      {
         Ret = true;
         Key = Query.value(0).toLongLong(&Ret);
      }
      if (!Ret) 
      {
         // Path not found : then add it to the table without timestamp
         Query.prepare(QString("INSERT INTO %1 (Key,Name,ParentKey) VALUES (:Key,:Name,:ParentKey)").arg(TableName));
         Query.bindValue(":Key",      ++NextIndex,QSql::In);
         Query.bindValue(":Name",     FolderList.last(), QSql::In);
         Query.bindValue(":ParentKey",ParentKey,         QSql::In);
         Ret = Query.exec();
         if (!Ret) 
            DisplayLastSQLError(&Query); 
         else 
            Key = NextIndex;
      }
   }
   folderMap.insert(FolderPath,Key);
   return Key;
}

//=====================================================================================================
// Check if folder timestamp is the same
bool cFolderTable::CheckFolderTimestamp(QDir Folder,qlonglong FolderKey) 
{
   QMutexLocker locker(&Database->dbMutex);
   qlonglong TimeStamp = 0;
   QSqlQuery Query(Database->db);
   Query.prepare(QString("SELECT Timestamp FROM %1 WHERE Key=:Key").arg(TableName));
   Query.bindValue(":Key",FolderKey,QSql::In);
   if (!Query.exec()) 
      DisplayLastSQLError(&Query); 
   else 
      while (Query.next()) 
      {
         bool Ret;
         TimeStamp=Query.value(0).toLongLong(&Ret);
         if (!Ret) 
            TimeStamp = 0;
      }
   return TimeStamp == QFileInfo(Folder.absolutePath()).lastModified().toMSecsSinceEpoch();
}

//=====================================================================================================
// Update folder timestamp

void cFolderTable::UpdateFolderTimestamp(QDir Folder,qlonglong FolderKey) 
{
   QMutexLocker locker(&Database->dbMutex);
   QSqlQuery Query(Database->db);
   Query.prepare(QString("UPDATE %1 SET Timestamp=:Timestamp WHERE Key=:Key").arg(TableName));
   Query.bindValue(":Timestamp",QFileInfo(Folder.absolutePath()).lastModified().toMSecsSinceEpoch(),QSql::In);
   Query.bindValue(":Key",FolderKey,QSql::In);
   if (!Query.exec()) 
      DisplayLastSQLError(&Query);
}

//=====================================================================================================
// Get the path associated to a folder key
// path are always ended with a QDir::separator()

QString cFolderTable::GetFolderPath(qlonglong FolderKey) 
{
   QMutexLocker locker(&Database->dbMutex);
   QMap<qlonglong,QString>::const_iterator i = folderKeyMap.constFind(FolderKey);
   if( i != folderKeyMap.cend() )
      return i.value();

   QSqlQuery Query(Database->db);
   QString  Path;
   qlonglong ParentKey = -1;

   Query.prepare(QString("SELECT ParentKey,Name FROM %1 WHERE Key=:Key").arg(TableName));
   Query.bindValue(":Key",FolderKey,QSql::In);
   if (!Query.exec()) 
      DisplayLastSQLError(&Query); 
   else while (Query.next()) 
   {
      if (!Query.value(0).isNull()) 
      {
         bool Ret;
         ParentKey = Query.value(0).toLongLong(&Ret);
         if (!Ret) 
            ParentKey=-1;
      }
      Path = Query.value(1).toString();
   }
   if (!Path.endsWith(QDir::separator())) 
      Path = Path + QDir::separator();
   if (ParentKey!=-1) 
      Path = GetFolderPath(ParentKey) + Path;
   folderKeyMap.insert(FolderKey, Path);
   return Path;
}

//**********************************************************************************************
// cFilesTable : encapsulate media files in the table
//**********************************************************************************************

cFilesTable::cFilesTable(cDatabase *Database):cDatabaseTable(Database) 
{
   TypeTable = TypeTable_FileTable;
   TableName = "MediaFiles";
   IndexKeyName = "Key";
   CreateTableQuery = "create table MediaFiles ("\
      "Key                bigint primary key,"\
      "ShortName          varchar(256),"\
      "FolderKey          bigint,"\
      "Timestamp          bigint,"\
      "IsHidden           int,"\
      "IsDir              int,"\
      "CreatDateTime      text,"\
      "ModifDateTime      text,"\
      "FileSize           bigint,"\
      "MediaFileType      int,"\
      "BasicProperties    text,"\
      "ExtendedProperties text,"\
      "Thumbnail16        binary,"\
      "Thumbnail100       binary,"\
      "SoundWave          text"\
      ")";
   CreateIndexQuery.append("CREATE INDEX idx_MediaFiles_Key ON MediaFiles (Key)");
   CreateIndexQuery.append("CREATE INDEX idx_MediaFiles_FolderKey ON MediaFiles (FolderKey,ShortName)");
   CreateIndexQuery.append("CREATE INDEX idx_MediaFiles_FolderKey2 ON MediaFiles (FolderKey)");
   gfk = NULL;
   gfkfr = NULL;
   gfkInsert = NULL;
   getshortname = NULL;
   getbasicprops = NULL;
   updthumbs = NULL;
}

cFilesTable::~cFilesTable()
{
   delete gfk;
   delete gfkfr;
   delete gfkInsert;
   delete getshortname;
   delete getbasicprops;
   delete updthumbs;
}
//=====================================================================================================
// Get the key associated to a file
// If file not found in the database, then create it

qlonglong cFilesTable::GetFileKey(qlonglong FolderKey,QString ShortName,int MediaFileType,bool ForceRefresh) 
{
   QMutexLocker locker(&Database->dbMutex);
   //qDebug() << "GetFileKey for " << ShortName << " in folder " << FolderKey << " with refresh = " << ForceRefresh;
   cFolderTable *folderTable = (cFolderTable *)Database->GetTable(TypeTable_FolderTable);
   QSqlQuery   Query(Database->db);
   bool        Ret = false;
   qlonglong   FileKey = -1;
   QSqlQuery *pQuery;
   if( ForceRefresh )
   {
      //Query.prepare(QString("SELECT Key,Timestamp,CreatDateTime,ModifDateTime,FileSize,MediaFileType FROM %1 WHERE FolderKey=:FolderKey AND ShortName=:ShortName").arg(TableName));
      if( !gfk )
      {
         gfk = new QSqlQuery(Database->db);
         gfk->prepare(QString("SELECT Key,Timestamp,CreatDateTime,ModifDateTime,FileSize,MediaFileType FROM %1 WHERE FolderKey=:FolderKey AND ShortName=:ShortName").arg(TableName));
      }
      pQuery = gfk;
   }
   else
   {
      //Query.prepare(QString("SELECT Key FROM %1 WHERE FolderKey=:FolderKey AND ShortName=:ShortName").arg(TableName));
      if( !gfkfr )
      {
         gfkfr = new QSqlQuery(Database->db);
         gfkfr->prepare(QString("SELECT Key FROM %1 WHERE FolderKey=:FolderKey AND ShortName=:ShortName").arg(TableName));
      }
      pQuery = gfkfr;
   }
   pQuery->bindValue(":FolderKey",FolderKey,QSql::In);
   pQuery->bindValue(":ShortName",ShortName,QSql::In);
   if (!pQuery->exec()) 
   {
      DisplayLastSQLError(pQuery); 
      return FileKey;
   }
   while (pQuery->next()) 
   {
      if (!pQuery->value(0).isNull()) 
      {
         Ret = true;
         FileKey = pQuery->value(0).toLongLong(&Ret);
         //qDebug() << "key found " << FileKey;
         if (ForceRefresh) 
         {
            QString   FullPath = folderTable->GetFolderPath(FolderKey)+ShortName;
            QFileInfo FileInfo(FullPath);
            bool b;
            if( pQuery->value(1).toLongLong(&b) != FileInfo.lastModified().toMSecsSinceEpoch() 
               || pQuery->value(2).toDateTime() != FileInfo.lastModified()
               || pQuery->value(3).toDateTime() != FileInfo.FILECREATIONDATE()
               || pQuery->value(4).toLongLong(&b) != FileInfo.size() 
               || pQuery->value(5).toInt(&b) != MediaFileType
               )
            {
               //qDebug() << "do refresh for this file";
               Query.prepare(QString("UPDATE %1 SET Timestamp=:Timestamp,CreatDateTime=:CreatDateTime,ModifDateTime=:ModifDateTime,FileSize=:FileSize,MediaFileType=:MediaFileType,"\
                  "BasicProperties=NULL,ExtendedProperties=NULL,Thumbnail16=NULL,Thumbnail100=NULL WHERE Key=:Key").arg(TableName));
               Query.bindValue(":Key",          FileKey,                                    QSql::In);
               Query.bindValue(":Timestamp",    FileInfo.lastModified().toMSecsSinceEpoch(),QSql::In);
               Query.bindValue(":CreatDateTime",FileInfo.lastModified(),                    QSql::In);
               Query.bindValue(":ModifDateTime",FileInfo.FILECREATIONDATE(),                         QSql::In);
               Query.bindValue(":FileSize",     FileInfo.size(),                            QSql::In);
               Query.bindValue(":MediaFileType",MediaFileType,                              QSql::In);
               if (!Query.exec()) 
                  DisplayLastSQLError(&Query);
            }
         }
      }
   }
   if (!Ret) 
   {
      //qDebug() << "add the File to table";;
      // File not found : then add it to the table
      QString   FullPath = folderTable->GetFolderPath(FolderKey)+ShortName;
      QFileInfo FileInfo(FullPath);
      if( !gfkInsert )
      {
         gfkInsert = new QSqlQuery(Database->db);
         gfkInsert->prepare(QString("INSERT INTO %1 (Key,ShortName,FolderKey,Timestamp,IsHidden,IsDir,CreatDateTime,ModifDateTime,FileSize,MediaFileType) "\
            "VALUES (:Key,:ShortName,:FolderKey,:Timestamp,:IsHidden,:IsDir,:CreatDateTime,:ModifDateTime,:FileSize,:MediaFileType)").arg(TableName));
      }
      //Query.prepare(QString("INSERT INTO %1 (Key,ShortName,FolderKey,Timestamp,IsHidden,IsDir,CreatDateTime,ModifDateTime,FileSize,MediaFileType) "\
      //   "VALUES (:Key,:ShortName,:FolderKey,:Timestamp,:IsHidden,:IsDir,:CreatDateTime,:ModifDateTime,:FileSize,:MediaFileType)").arg(TableName));
      gfkInsert->bindValue(":Key",          ++NextIndex,                                                   QSql::In);
      gfkInsert->bindValue(":ShortName",    ShortName,                                                     QSql::In);
      gfkInsert->bindValue(":FolderKey",    FolderKey,                                                     QSql::In);
      gfkInsert->bindValue(":Timestamp",    FileInfo.lastModified().toMSecsSinceEpoch(),                   QSql::In);
      gfkInsert->bindValue(":IsHidden",     FileInfo.isHidden()||FileInfo.fileName().startsWith(".")?1:0,  QSql::In);
      gfkInsert->bindValue(":IsDir",        FileInfo.isDir()?1:0,                                          QSql::In);
      gfkInsert->bindValue(":CreatDateTime",FileInfo.lastModified(),                                       QSql::In);
      gfkInsert->bindValue(":ModifDateTime",FileInfo.FILECREATIONDATE(),                                            QSql::In);
      gfkInsert->bindValue(":FileSize",     FileInfo.size(),                                               QSql::In);
      gfkInsert->bindValue(":MediaFileType",MediaFileType,                                                 QSql::In);
      Ret = gfkInsert->exec();
      if (!Ret) 
         DisplayLastSQLError(gfkInsert); 
      else 
         FileKey = NextIndex;
   }
   return FileKey;
}

//=====================================================================================================
// Get the path associated to a folder key
// path are always ended with a QDir::separator()

QString cFilesTable::GetShortName(qlonglong FileKey) 
{
   QMutexLocker locker(&Database->dbMutex);
   //QSqlQuery   Query(Database->db);
   QString     ShortName;
   if( !getshortname )
   {
      getshortname = new QSqlQuery(Database->db);
      getshortname->prepare(QString("SELECT ShortName FROM %1 WHERE Key=:Key").arg(TableName));
   }
   //Query.prepare(QString("SELECT ShortName FROM %1 WHERE Key=:Key").arg(TableName));
   getshortname->bindValue(":Key",FileKey,QSql::In);
   if (!getshortname->exec()) 
      DisplayLastSQLError(getshortname); 
   else 
      while (getshortname->next()) 
         ShortName = getshortname->value(0).toString();
   return ShortName;
}

//=====================================================================================================
// Get the path associated to a folder key
// path are always ended with a QDir::separator()

QString cFilesTable::GetFileName(qlonglong FileKey) 
{
   QMutexLocker locker(&Database->dbMutex);
    QSqlQuery   Query(Database->db);
    QString     FolderPath;
    QString     ShortName;
    qlonglong   FolderKey=-1;

    Query.prepare(QString("SELECT FolderKey,ShortName FROM %1 WHERE Key=:Key").arg(TableName));
    Query.bindValue(":Key",FileKey,QSql::In);
    if (!Query.exec()) DisplayLastSQLError(&Query); else while (Query.next()) {
        if (!Query.value(0).isNull()) {
            bool Ret=true;
            FolderKey=Query.value(0).toLongLong(&Ret);
            if (Ret) FolderPath=((cFolderTable *)Database->GetTable(TypeTable_FolderTable))->GetFolderPath(FolderKey);
        }
        ShortName=Query.value(1).toString();
    }
    return FolderPath+ShortName;
}

//=====================================================================================================
// scan all files for a given folderkey and:
//  - delete files no longer exist
//  - set data field to null for modified files (different timestamp)

int cFilesTable::CleanTableForFolder(qlonglong FolderKey) 
{
   QMutexLocker locker(&Database->dbMutex);
   int         NbrModif = 0, Count = 0;
   QSqlQuery   Query(Database->db);
   QSqlQuery   Query2(Database->db);
   QString     FolderPath=((cFolderTable *)Database->GetTable(TypeTable_FolderTable))->GetFolderPath(FolderKey);
   QFileInfoList files = QDir(FolderPath).entryInfoList(QDir::Files | QDir::Hidden| QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);
   Query.prepare(QString("SELECT Key,ShortName,Timestamp FROM %1 WHERE FolderKey=:FolderKey order by ShortName").arg(TableName));
   Query.bindValue(":FolderKey",FolderKey,QSql::In);
   if (!Query.exec()) 
   {
      DisplayLastSQLError(&Query); 
      return 0;
   }
   while (Query.next()) 
   {
      qlonglong FileKey = -1;
      QString   ShortName;
      qlonglong Timestamp = -1;
      bool      Ret = false;

      if (!Query.value(0).isNull()) 
      {
         Ret = true;
         FileKey = Query.value(0).toLongLong(&Ret);
      }
      if (Ret)
      {
         ShortName = Query.value(1).toString();
         if (!Query.value(2).isNull()) 
         {
            Ret = true;
            Timestamp = Query.value(2).toLongLong(&Ret);
         } 
         else 
            NbrModif++;
      }

      if (Ret) 
      {
         QFileInfoList::Iterator it = files.begin();
         while( it != files.end() && it->fileName() != ShortName )
            it++;

         //QFileInfo FileInfo(FolderPath+ShortName);
         //if (!FileInfo.exists()) 
         if( it == files.end() )
         {
            //qDebug() << "CleanTableForFolder delete entry for " << ShortName;
            Query2.prepare((QString("DELETE FROM %1 WHERE Key=:Key").arg(TableName)));
            Query2.bindValue(":Key",FileKey,QSql::In);
            if (!Query2.exec()) 
               DisplayLastSQLError(&Query2);
            NbrModif++;
         } 
         else 
         {
            //qlonglong NewTimestamp = FileInfo.lastModified().toMSecsSinceEpoch();
            qlonglong NewTimestamp = it->lastModified().toMSecsSinceEpoch();
            if (NewTimestamp != Timestamp) 
            {
               //qDebug() << "CleanTableForFolder update entry for " << ShortName;

               Query2.prepare(QString("UPDATE %1 SET Thumbnail16=NULL,Thumbnail100=NULL,BasicProperties=NULL,ExtendedProperties=NULL,Timestamp=:Timestamp,"\
                  "CreatDateTime=:CreatDateTime,ModifDateTime=:ModifDateTime,FileSize=:FileSize WHERE Key=:Key").arg(TableName));
               Query2.bindValue(":Key",          FileKey,                                      QSql::In);
               Query2.bindValue(":Timestamp",    NewTimestamp,                                 QSql::In);
               Query2.bindValue(":CreatDateTime",it->lastModified().toString(Qt::ISODate),QSql::In);
               Query2.bindValue(":ModifDateTime",it->FILECREATIONDATE().toString(Qt::ISODate),     QSql::In);
               Query2.bindValue(":FileSize",     it->size(),                              QSql::In);
               if (!Query2.exec()) 
                  DisplayLastSQLError(&Query2);
               NbrModif++;
            }
            files.erase(it);
         }
      }
      Count++;
   }
   if (Count == 0 || files.count() > 0 ) 
      NbrModif++;
   return NbrModif;
}

//=====================================================================================================
// scan all files for a given folderkey and add new one to the table
void cFilesTable::UpdateTableForFolder(qlonglong FolderKey,bool ForceRefresh) 
{
   QMutexLocker locker(&Database->dbMutex);
   cFolderTable *folderTable = (cFolderTable *)Database->GetTable(TypeTable_FolderTable);
   QString FolderPath = folderTable->GetFolderPath(FolderKey);
   //qDebug() << "UpdateTableForFolder " << FolderPath << " refresh " << ForceRefresh;
   QDir    Folder(FolderPath);
   bool    NeedRefresh = (CleanTableForFolder(FolderKey) > 0) || (!folderTable->CheckFolderTimestamp(Folder,FolderKey));
   //qDebug() << "NeedRefresh " << NeedRefresh;

   if (NeedRefresh || ForceRefresh) 
   {
      QFileInfoList Files = Folder.entryInfoList(QDir::Dirs | QDir::AllDirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
      //QStringList videoFilters;
      //foreach(QString s, Database->ApplicationConfig->AllowVideoExtension)
      //   videoFilters.append(QString("*.").append(s));
      QStringList videoFiles;
      foreach(QFileInfo File, Files)
      {
         QString lowerSuffix = File.suffix().toLower();
         if (Database->ApplicationConfig->AllowVideoExtension.contains(lowerSuffix))
            videoFiles.append(File.completeBaseName());
      }
      foreach(QFileInfo File,Files)
      {
         QString ShortName = File.fileName();
         //autoTimer at(ShortName);
         //if (ShortName != "." && ShortName!="..")
         {
            int ObjectType = OBJECTTYPE_UNMANAGED;
            QString lowerSuffix = File.suffix().toLower();
            if (File.isDir())                                                                       
               ObjectType = OBJECTTYPE_FOLDER;
            else if (Database->ApplicationConfig->AllowImageVectorExtension.contains(lowerSuffix))  
               ObjectType = OBJECTTYPE_IMAGEVECTOR;
            else if (Database->ApplicationConfig->AllowVideoExtension.contains(lowerSuffix))        
               ObjectType = OBJECTTYPE_VIDEOFILE;
            else if (Database->ApplicationConfig->AllowMusicExtension.contains(lowerSuffix))        
               ObjectType = OBJECTTYPE_MUSICFILE;
            else if (lowerSuffix == "ffd")                                                          
               ObjectType = OBJECTTYPE_FFDFILE;
            else if (Database->ApplicationConfig->AllowImageExtension.contains(lowerSuffix)) 
            {
               ObjectType = OBJECTTYPE_IMAGEFILE;
               // Special case for folder Thumbnails
               if (File.fileName() == "folder.jpg") 
               {
                  ObjectType = OBJECTTYPE_THUMBNAIL;
               } 
               else if (File.fileName().toLower().endsWith("-poster.jpg") || File.fileName().toLower().endsWith("-poster.png")) 
               {  // Special case for video xbmc poster Thumbnails
                  // Search if a video with same name exist
                  QString ToSearch = File.fileName().left(File.fileName().toLower().indexOf("-poster."));
                  for (int i = 0; i < Files.count(); i++)
                     if ( Database->ApplicationConfig->AllowVideoExtension.contains(Files[i].suffix().toLower()) && (Files[i].completeBaseName() == ToSearch) )
                        ObjectType = OBJECTTYPE_THUMBNAIL;
               } 
               else if (lowerSuffix == "jpg") 
               {  // Special case for video Thumbnails
                  // Search if a video with same name exist
                  if(videoFiles.contains(File.completeBaseName()))
                     ObjectType = OBJECTTYPE_THUMBNAIL;
               //   QDir d(File.absolutePath());
               //   QString fBaseName = File.completeBaseName();
               //   QFileInfoList l = d.entryInfoList(QStringList(fBaseName+".*"));
               //   for (int i = 0; i < l.count(); i++)
               //   {
               //      if ( Database->ApplicationConfig->AllowVideoExtension.contains(l[i].suffix().toLower()) &&
               //         (l[i].completeBaseName() == fBaseName)  )
               //            ObjectType = OBJECTTYPE_THUMBNAIL;
               //   }
               }
            }   
            GetFileKey(FolderKey, File.fileName(), ObjectType, ForceRefresh);
         }
      }
      folderTable->UpdateFolderTimestamp(Folder, FolderKey);
   }
}

//=====================================================================================================
// Write basic properties to the database

bool cFilesTable::SetBasicProperties(qlonglong FileKey,QString Properties) 
{
   QMutexLocker locker(&Database->dbMutex);
    QSqlQuery Query(Database->db);
    Query.prepare((QString("UPDATE %1 SET BasicProperties=:BasicProperties WHERE Key=:Key").arg(TableName)));
    Query.bindValue(":Key",            FileKey,   QSql::In);
    Query.bindValue(":BasicProperties",Properties,QSql::In);
    if (!Query.exec()) {
        DisplayLastSQLError(&Query);
        return false;
    }
    return true;
}

//=====================================================================================================
// Read basic properties from the database

bool cFilesTable::GetBasicProperties(qlonglong FileKey,QString *Properties,QString FileName,int64_t *FileSize,QDateTime *CreatDateTime,QDateTime *ModifDateTime) 
{
   QMutexLocker locker(&Database->dbMutex);
   //AUTOTIMER(a,"cFilesTable::GetBasicProperties");
   if( !getbasicprops )
   {
      getbasicprops = new QSqlQuery(Database->db);
      getbasicprops->prepare(QString("SELECT BasicProperties,Timestamp,FileSize,CreatDateTime,ModifDateTime FROM %1 WHERE Key=:Key").arg(TableName));
   }
   getbasicprops->bindValue(":Key",FileKey,QSql::In);
   if (!getbasicprops->exec())
      DisplayLastSQLError(getbasicprops);
   else
      while (getbasicprops->next())
      {
         bool        Ret = true;
         qlonglong   Timestamp = 0;
         if (!getbasicprops->value(0).isNull())         *Properties = getbasicprops->value(0).toString();                                         else Ret = false;
         if (Ret && !getbasicprops->value(4).isNull())  *ModifDateTime = QDateTime().fromString(getbasicprops->value(4).toString(), Qt::ISODate); else Ret = false;
         if (Ret && !getbasicprops->value(3).isNull())  *CreatDateTime = QDateTime().fromString(getbasicprops->value(3).toString(), Qt::ISODate); else Ret = false;
         if (Ret && !getbasicprops->value(2).isNull())  *FileSize = getbasicprops->value(2).toLongLong(&Ret);                                     else Ret = false;
         if (Ret && !getbasicprops->value(1).isNull())  Timestamp = getbasicprops->value(1).toLongLong(&Ret);                                     else Ret = false;
         Ret = Ret && (Timestamp == QFileInfo(FileName).lastModified().toMSecsSinceEpoch());
         if (*ModifDateTime < *CreatDateTime) 
            *ModifDateTime = *CreatDateTime;
         return Ret;
      }
      return false;
}

//=====================================================================================================
// Write extended properties to the database

bool cFilesTable::SetExtendedProperties(qlonglong FileKey,QStringList *PropertiesList) 
{
   QMutexLocker locker(&Database->dbMutex);
#ifdef NPROPS
   QString props = PropertiesList->join(" ####"); 
   QSqlQuery Query(Database->db);
   Query.prepare((QString("UPDATE %1 SET ExtendedProperties=:ExtendedProperties WHERE Key=:Key").arg(TableName)));
   Query.bindValue(":Key",               FileKey,               QSql::In);
   Query.bindValue(":ExtendedProperties",props ,QSql::In);
   if (!Query.exec()) {
      DisplayLastSQLError(&Query);
      return false;
   }
   return true;
#else
   QDomDocument domDocument;
   QDomElement  root=domDocument.createElement("ExtendedProperties");
   domDocument.appendChild(root);
   for (int i=0;i<PropertiesList->count();i++) {
      QStringList Values=PropertiesList->at(i).split("##");
      QDomElement Element=domDocument.createElement(QString("Item-%1").arg(i));
      Element.setAttribute("Name", Values[0]);
      Element.setAttribute("Value",Values[1]);
      root.appendChild(Element);
   }
   QSqlQuery Query(Database->db);
   Query.prepare((QString("UPDATE %1 SET ExtendedProperties=:ExtendedProperties WHERE Key=:Key").arg(TableName)));
   Query.bindValue(":Key",               FileKey,               QSql::In);
   Query.bindValue(":ExtendedProperties",domDocument.toString(),QSql::In);
   if (!Query.exec()) {
      DisplayLastSQLError(&Query);
      return false;
   }
   return true;
#endif
}

//=====================================================================================================
// Read extended properties from the database

bool cFilesTable::GetExtendedProperties(qlonglong FileKey,QStringList *PropertiesList) 
{
   QMutexLocker locker(&Database->dbMutex);
#ifdef NPROPS
   QSqlQuery Query(Database->db);
   Query.prepare((QString("SELECT ExtendedProperties FROM %1 WHERE Key=:Key").arg(TableName)));
   Query.bindValue(":Key",FileKey,QSql::In);
   if (!Query.exec()) {
      DisplayLastSQLError(&Query);
      return false;
   }
   if(Query.next()) 
   {
      QString Value = Query.value(0).toString();
      if( Value.length() )
         *PropertiesList = Value.split(" ####");
      return true;
   }
   return false;
#else
   //AUTOTIMER(a,"cFilesTable::GetExtendedProperties");
   QSqlQuery Query(Database->db);
   Query.prepare((QString("SELECT ExtendedProperties FROM %1 WHERE Key=:Key").arg(TableName)));
   Query.bindValue(":Key",FileKey,QSql::In);
   if (!Query.exec()) {
      DisplayLastSQLError(&Query);
      return false;
   }
   while (Query.next()) {
      QDomDocument    domDocument;
      QString         errorStr;
      int             errorLine,errorColumn;
      QString         Value=Query.value(0).toString();
      if ((domDocument.setContent(Value,true,&errorStr,&errorLine,&errorColumn))&&
         (domDocument.elementsByTagName("ExtendedProperties").length()>0)&&
         (domDocument.elementsByTagName("ExtendedProperties").item(0).isElement()==true)) {

            QDomElement root=domDocument.elementsByTagName("ExtendedProperties").item(0).toElement();
            int i=0;
            while ((root.elementsByTagName(QString("Item-%1").arg(i)).length()>0)&&(root.elementsByTagName(QString("Item-%1").arg(i)).item(0).isElement()==true)) {
               QDomElement Element=root.elementsByTagName(QString("Item-%1").arg(i)).item(0).toElement();
               PropertiesList->append(Element.attribute("Name")+"##"+Element.attribute("Value"));
               i++;
            }
      }
      return true;
   }
   return false;
#endif
}

bool cFilesTable::HasExtendedProperties(qlonglong FileKey)
{
   QMutexLocker locker(&Database->dbMutex);
   //AUTOTIMER(a,"cFilesTable::HasExtendedProperties");
   QSqlQuery Query(Database->db);
   Query.prepare((QString("SELECT ExtendedProperties FROM %1 WHERE Key=:Key").arg(TableName)));
   Query.bindValue(":Key",FileKey,QSql::In);
   if (!Query.exec()) 
   {
      DisplayLastSQLError(&Query);
      return false;
   }
   if (Query.next()) 
      return true;
   return false;
}

//=====================================================================================================
// Write thumbnails to the database

bool cFilesTable::SetThumbs(qlonglong FileKey,QImage *Icon16,QImage *Icon100) 
{
   QMutexLocker locker(&Database->dbMutex);
   if( !updthumbs )
   {
      updthumbs = new QSqlQuery(Database->db);
      updthumbs->prepare((QString("UPDATE %1 SET Thumbnail16=:Thumbnail16,Thumbnail100=:Thumbnail100 WHERE Key=:Key").arg(TableName)));
   }
   updthumbs->bindValue(":Key",FileKey,QSql::In);
   if (!Icon16->isNull()) 
   {
      QByteArray  Data;
      QBuffer     BufData(&Data);
      BufData.open(QIODevice::WriteOnly);
      Icon16->save(&BufData,"PNG");
      updthumbs->bindValue(":Thumbnail16",Data,QSql::In|QSql::Binary);
   } 
   else 
      updthumbs->bindValue(":Thumbnail16",QVariant(QVariant::ByteArray),QSql::In|QSql::Binary);
   if (!Icon100->isNull()) 
   {
      QByteArray  Data;
      QBuffer     BufData(&Data);
      BufData.open(QIODevice::WriteOnly);
      Icon100->save(&BufData,"PNG");
      updthumbs->bindValue(":Thumbnail100",Data,QSql::In|QSql::Binary);
   } 
   else 
      updthumbs->bindValue(":Thumbnail100",QVariant(QVariant::ByteArray),QSql::In|QSql::Binary);
   if (!updthumbs->exec()) {
      DisplayLastSQLError(updthumbs);
      return false;
   }
   return true;
}

//=====================================================================================================
// Read thumbnails properties from the database

bool cFilesTable::GetThumbs(qlonglong FileKey,QImage *Icon16,QImage *Icon100) 
{
   QMutexLocker locker(&Database->dbMutex);
   QSqlQuery Query(Database->db);
   Query.prepare((QString("SELECT Thumbnail16,Thumbnail100 FROM %1 WHERE Key=:Key").arg(TableName)));
   Query.bindValue(":Key",FileKey,QSql::In);
   if (!Query.exec()) 
   {
      DisplayLastSQLError(&Query);
      return false;
   }
   QByteArray Data;
   while (Query.next()) 
   {
      if (!Query.value(0).isNull()) 
      {
         Data = Query.value(0).toByteArray();
         Icon16->loadFromData(Data);
      }
      if (!Query.value(1).isNull()) 
      {
         Data=Query.value(1).toByteArray();
         Icon100->loadFromData(Data);
      }
   }
   return (!Icon16->isNull())&&(!Icon100->isNull());
}

//====================================================================================================================

bool cFilesTable::GetAnalyseSound(qlonglong FileKey,QList<qreal> *Peak,QList<qreal> *Moyenne,int64_t *RealAudioDuration,int64_t *RealVideoDuration,qreal *MaxMoyenneValue) 
{
   QMutexLocker locker(&Database->dbMutex);
   QSqlQuery Query(Database->db);
   *RealAudioDuration = 0;
   if (RealVideoDuration) 
      *RealVideoDuration = 0;
   if( !Query.prepare((QString("SELECT SoundWave FROM %1 WHERE Key=:Key").arg(TableName))) )
   {
      DisplayLastSQLError(&Query);
      return false;
   }
   Query.bindValue(":Key",FileKey,QSql::In);
   if (!Query.exec()) 
   {
      DisplayLastSQLError(&Query);
      return false;
   }
   while (Query.next()) 
   {
      QDomDocument domDocument;
      QString      errorStr;
      int          errorLine,errorColumn;
      QString      Value = Query.value(0).toString();
      int          PeakNbr = 0;
      if ((domDocument.setContent(Value,true,&errorStr,&errorLine,&errorColumn)) &&
         (domDocument.elementsByTagName("SOUNDWAVE").length()>0) &&
         (domDocument.elementsByTagName("SOUNDWAVE").item(0).isElement() == true))                                                   
      {
         QDomElement root = domDocument.elementsByTagName("SOUNDWAVE").item(0).toElement();
         Peak->clear();
         Moyenne->clear();
         QDomNodeList peakNodes = root.elementsByTagName("Peak");
         for( int i = 0; i < peakNodes.length(); i++ )
         {
            QDomElement Element = peakNodes.at(i).toElement();   
            if( !Element.isNull() )
            {
               Peak->append(   GetDoubleValue(Element,"P"));
               Moyenne->append(GetDoubleValue(Element,"M"));
               PeakNbr++;
            }
         }
         //while ((root.elementsByTagName(QString("Peak-%1").arg(PeakNbr)).length() > 0) && (root.elementsByTagName(QString("Peak-%1").arg(PeakNbr)).item(0).isElement() == true)) 
         //{
         //   QDomElement Element = root.elementsByTagName(QString("Peak-%1").arg(PeakNbr)).item(0).toElement();
         //   Peak->append(   GetDoubleValue(Element,"P"));
         //   Moyenne->append(GetDoubleValue(Element,"M"));
         //   PeakNbr++;
         //}
         if (root.hasAttribute("RealAudioDuration"))                         
            *RealAudioDuration = root.attribute("RealAudioDuration").toLongLong();
         if ((RealVideoDuration) && (root.hasAttribute("RealVideoDuration")))  
            *RealVideoDuration = root.attribute("RealVideoDuration").toLongLong();
         if (root.hasAttribute("MaxMoyenne"))                                
            *MaxMoyenneValue = GetDoubleValue(root,"MaxMoyenne");
         //qDebug() << domDocument.toString();
      }
      return (PeakNbr > 0) && (*RealAudioDuration > 0) && (*MaxMoyenneValue > 0) && (!RealVideoDuration || *RealVideoDuration > 0);
   }
   return false;
}

//====================================================================================================================

void cFilesTable::SetAnalyseSound(qlonglong FileKey,QList<qreal> *Peak,QList<qreal> *Moyenne,int64_t RealAudioDuration,int64_t *RealVideoDuration,qreal MaxMoyenneValue) 
{
   QMutexLocker locker(&Database->dbMutex);
   QDomDocument domDocument("SOUNDWAVE");
   QDomElement  root = domDocument.createElement("SOUNDWAVE");
   for (int PeakNbr = 0; PeakNbr < Peak->count(); PeakNbr++) 
   {
      //QDomElement Element = domDocument.createElement(QString("Peak-%1").arg(PeakNbr));
      QDomElement Element = domDocument.createElement(QString("Peak"));//.arg(PeakNbr));
      //Element.setAttribute("N",PeakNbr);
      Element.setAttribute("P",Peak->at(PeakNbr));
      Element.setAttribute("M",Moyenne->at(PeakNbr));
      root.appendChild(Element);
   }
   root.setAttribute("RealAudioDuration",(qlonglong)RealAudioDuration);
   if (RealVideoDuration) 
      root.setAttribute("RealVideoDuration",(qlonglong)*RealVideoDuration);
   root.setAttribute("MaxMoyenne",MaxMoyenneValue);
   domDocument.appendChild(root);
   //qDebug() << domDocument.toString();

   QSqlQuery Query(Database->db);
   Query.prepare((QString("UPDATE %1 SET SoundWave=:SoundWave WHERE Key=:Key").arg(TableName)));
   Query.bindValue(":Key",      FileKey,               QSql::In);
   Query.bindValue(":SoundWave",domDocument.toString(),QSql::In);
   if (!Query.exec()) 
      DisplayLastSQLError(&Query);
   //QString s = domDocument.toString();
   //qDebug() << "domdoc = " << s;
}


//=====================================================================================================

bool cFilesTable::DoUpgradeTableVersion(qlonglong OldVersion) 
{
    QSqlQuery Query(Database->db);
    bool Ret = true;

    //if (OldVersion == 1)                           Ret = Query.exec("DROP TABLE MediaFiles");
    //else if (OldVersion == 2)                      Ret = Query.exec("DELETE FROM MediaFiles");
    //else if (OldVersion == 3)                      Ret = Query.exec("DELETE FROM MediaFiles");
    //else if (OldVersion >= 4 && OldVersion <= 6)   Ret = Query.exec("ALTER TABLE MediaFiles ADD COLUMN SoundWave text;");
    //else if (OldVersion == 7)                      Ret = Query.exec("UPDATE MediaFiles SET SoundWave=NULL WHERE SoundWave IS NOT NULL;");
    if (OldVersion < 9 )                           Ret = Query.exec("DROP TABLE MediaFiles");

    if (!Ret) 
       DisplayLastSQLError(&Query);
    return Ret;
}

//**********************************************************************************************
// cSlideThumbsTable
//**********************************************************************************************

cSlideThumbsTable::cSlideThumbsTable(cDatabase *Database) : cDatabaseTable(Database) 
{
    TypeTable       = TypeTable_SlideThumbsTable;
    TableName       = "SlideThumbs";
    IndexKeyName    = "Key";
    CreateTableQuery= "create table SlideThumbs ("\
                      "Key                bigint primary key,"\
                      "Thumbnail          binary"\
                      ")";
    CreateIndexQuery.append("CREATE INDEX idx_SlideThumbs_Key ON SlideThumbs (Key)");
}

//=====================================================================================================
// Reset the table : delete all items
bool cSlideThumbsTable::ClearTable() 
{
   ThumbsMap.clear();
   QSqlQuery Query(Database->db);
   if (!Query.exec(QString("DELETE FROM %1").arg(TableName))) 
   {
      DisplayLastSQLError(&Query);
      return false;
   }
   NextIndex = 0;
   return true;
}

//=====================================================================================================
// Write thumbnails to the database
bool cSlideThumbsTable::SetThumb(qlonglong *ThumbnailKey, const QImage &Thumb) 
{
   QMutexLocker locker(&Database->dbMutex);
   if( *ThumbnailKey == -1 )
      *ThumbnailKey = ++NextIndex;
   ThumbsMap.insert(*ThumbnailKey, Thumb);
   return true;

   QSqlQuery Query(Database->db);
   if (*ThumbnailKey == -1) 
   {
      Query.prepare(QString("INSERT INTO %1 (Key,Thumbnail) VALUES (:Key,:Thumbnail)").arg(TableName));
      Query.bindValue(":Key",++NextIndex,QSql::In);
      if (!Thumb.isNull()) 
      {
         QByteArray  Data;
         QBuffer     BufData(&Data);
         BufData.open(QIODevice::WriteOnly);
         Thumb.save(&BufData,"PNG");
         Query.bindValue(":Thumbnail",Data,QSql::In|QSql::Binary);
      } 
      else 
         Query.bindValue(":Thumbnail",QVariant(QVariant::ByteArray),QSql::In|QSql::Binary);
      bool Ret = Query.exec();
      if (!Ret) 
      {
         DisplayLastSQLError(&Query);
         return false;
      } 
      else 
         *ThumbnailKey = NextIndex;
   } 
   else 
   {
      Query.prepare((QString("UPDATE %1 SET Thumbnail=:Thumbnail WHERE Key=:Key").arg(TableName)));
      Query.bindValue(":Key",*ThumbnailKey,QSql::In);
      if (!Thumb.isNull()) 
      {
         QByteArray  Data;
         QBuffer     BufData(&Data);
         BufData.open(QIODevice::WriteOnly);
         Thumb.save(&BufData,"PNG");
         Query.bindValue(":Thumbnail",Data,QSql::In|QSql::Binary);
      } 
      else 
         Query.bindValue(":Thumbnail",QVariant(QVariant::ByteArray),QSql::In|QSql::Binary);
      if (!Query.exec()) 
      {
         DisplayLastSQLError(&Query);
         return false;
      }
   }
   return true;
}

//=====================================================================================================
// Write a thumbnails to the database and return a TRResKeyItem
cSlideThumbsTable::TRResKeyItem cSlideThumbsTable::AppendThumb(qlonglong OrigKey, const QImage &Thumb) 
{
   QMutexLocker locker(&Database->dbMutex);
   cSlideThumbsTable::TRResKeyItem Item;
   Item.OrigKey = OrigKey;
   Item.NewKey = -1;
   if (!SetThumb(&Item.NewKey,Thumb)) 
      Item.NewKey =-1;
   return Item;
}

//=====================================================================================================
// Read thumbnails properties from the database
bool cSlideThumbsTable::GetThumb(qlonglong *ThumbnailKey, QImage *Thumb) 
{
   // crazy logic here....
   QMutexLocker locker(&Database->dbMutex);
   if (*ThumbnailKey == -1)
   {
      *ThumbnailKey = ++NextIndex;
      return !Thumb->isNull();
   }
   QMap<qlonglong,QImage>::const_iterator i = ThumbsMap.constFind(*ThumbnailKey);
   if( i != ThumbsMap.cend() )
      *Thumb = i.value();
   return (!Thumb->isNull());

   QSqlQuery Query(Database->db);

   if (*ThumbnailKey == -1) 
   {
      Query.prepare(QString("INSERT INTO %1 (Key) VALUES (:Key)").arg(TableName));
      Query.bindValue(":Key",++NextIndex,QSql::In);
      bool Ret = Query.exec();
      if (!Ret) 
      {
         DisplayLastSQLError(&Query);
         return false;
      } 
      else 
         *ThumbnailKey = NextIndex;
   } 
   else 
   {
      Query.prepare((QString("SELECT Thumbnail FROM %1 WHERE Key=:Key").arg(TableName)));
      Query.bindValue(":Key",*ThumbnailKey,QSql::In);
      if (!Query.exec()) 
      {
         DisplayLastSQLError(&Query);
         return false;
      }
      QByteArray Data;
      while (Query.next()) 
      {
         if (!Query.value(0).isNull()) 
         {
            Data = Query.value(0).toByteArray();
            Thumb->loadFromData(Data);
         }
      }
   }
   return (!Thumb->isNull());
}

//=====================================================================================================
// Reset thumbnails properties from the database
bool cSlideThumbsTable::ClearThumb(qlonglong ThumbnailKey) 
{
   if (ThumbnailKey == -1) 
      return true;

   QMutexLocker locker(&Database->dbMutex);
   QMap<qlonglong,QImage>::iterator i = ThumbsMap.find(ThumbnailKey);
   if( i != ThumbsMap.cend() )
      i.value() = QImage();
   return true;

   QSqlQuery Query(Database->db);
   Query.prepare((QString("UPDATE %1 SET Thumbnail=NULL WHERE Key=:Key").arg(TableName)));
   Query.bindValue(":Key",ThumbnailKey,QSql::In);
   if (!Query.exec()) 
   {
      DisplayLastSQLError(&Query);
      return false;
   }
   return true;
}

//=====================================================================================================
// Remove thumbnails properties from the database
bool cSlideThumbsTable::RemoveThumb(qlonglong ThumbnailKey) 
{
   if (ThumbnailKey == -1) 
      return true;
   QMutexLocker locker(&Database->dbMutex);
   int nRemoved = ThumbsMap.remove(ThumbnailKey);
   return nRemoved > 0;

   QSqlQuery Query(Database->db);
   Query.prepare((QString("DELETE FROM %1 WHERE Key=:Key").arg(TableName)));
   Query.bindValue(":Key",ThumbnailKey,QSql::In);
   if (!Query.exec()) 
   {
      DisplayLastSQLError(&Query);
      return false;
   }
   return true;
}

//**********************************************************************************************
// cFolderTable : encapsulate folders in the table
//**********************************************************************************************

cLocationTable::cLocationTable(cDatabase *Database):cDatabaseTable(Database) {
    TypeTable       =TypeTable_LocationTable;
    TableName       ="Location";
    IndexKeyName    ="Key";
    CreateTableQuery="create table Location ("\
                            "Key                bigint primary key,"\
                            "Name               text,"\
                            "Address            text,"\
                            "Latitude           real,"\
                            "Longitude          real,"\
                            "Zoomlevel          int,"\
                            "Icon               text,"\
                            "Thumbnail          binary,"\
                            "FAddress           text"
                     ")";
    CreateIndexQuery.append("CREATE INDEX idx_Location_Key ON Location (Key)");
}

//=====================================================================================================

bool cLocationTable::DoUpgradeTableVersion(qlonglong OldVersion) {
    QSqlQuery Query(Database->db);
    bool Ret=true;
    // to rewrite!!!
    //if (OldVersion<=4) {
    //    Ret=Query.exec("DROP TABLE Location");
    //    if ((!Ret)&&(Query.lastError().number()==1)) Ret=true;
    //} else if (OldVersion==5) {
    //    Ret=Query.exec("ALTER TABLE Location ADD COLUMN FAddress text");
    //    if ((!Ret)&&(Query.lastError().number()==1)) Ret=true;
    //}

    if (!Ret) 
       DisplayLastSQLError(&Query);
    return Ret;
}

//=====================================================================================================

qlonglong cLocationTable::AppendLocation(QString Name,QString Address,QString FAddress,double Latitude,double Longitude,int Zoomlevel,QString Icon,QImage Thumbnail) 
{
   QMutexLocker locker(&Database->dbMutex);
    QSqlQuery Query(Database->db);
    Query.prepare(QString("INSERT INTO %1 (Key,Name,Address,FAddress,Latitude,Longitude,Zoomlevel,Icon,Thumbnail) VALUES (:Key,:Name,:Address,:FAddress,:Latitude,:Longitude,:Zoomlevel,:Icon,:Thumbnail)").arg(TableName));
    Query.bindValue(":Key",         ++NextIndex,QSql::In);
    Query.bindValue(":Name",        Name,QSql::In);
    Query.bindValue(":Address",     Address,QSql::In);
    Query.bindValue(":FAddress",    FAddress,QSql::In);
    Query.bindValue(":Latitude",    Latitude,QSql::In);
    Query.bindValue(":Longitude",   Longitude,QSql::In);
    Query.bindValue(":Zoomlevel",   Zoomlevel,QSql::In);
    Query.bindValue(":Icon",        Icon,QSql::In);

    QByteArray  Data;
    QBuffer     BufData(&Data);
    BufData.open(QIODevice::WriteOnly);
    Thumbnail.save(&BufData,"PNG");

    Query.bindValue(":Thumbnail",   Data,QSql::In);
    bool Ret=Query.exec();
    if (!Ret) {
        DisplayLastSQLError(&Query);
        return -1;
    } else return NextIndex;
}

//=====================================================================================================

qlonglong cLocationTable::UpdateLocation(qlonglong Key,QString Name,QString Address,QString FAddress,double Latitude,double Longitude,int Zoomlevel,QString Icon,QImage Thumbnail) 
{
   QMutexLocker locker(&Database->dbMutex);
    QSqlQuery Query(Database->db);
    Query.prepare(QString("UPDATE %1 SET Name=:Name,Address=:Address,FAddress=:FAddress,Latitude=:Latitude,Longitude=:Longitude,Zoomlevel=:Zoomlevel,Icon=:Icon,Thumbnail=:Thumbnail WHERE Key=:Key").arg(TableName));
    Query.bindValue(":Key",         Key,QSql::In);
    Query.bindValue(":Name",        Name,QSql::In);
    Query.bindValue(":Address",     Address,QSql::In);
    Query.bindValue(":FAddress",    FAddress,QSql::In);
    Query.bindValue(":Latitude",    Latitude,QSql::In);
    Query.bindValue(":Longitude",   Longitude,QSql::In);
    Query.bindValue(":Zoomlevel",   Zoomlevel,QSql::In);
    Query.bindValue(":Icon",        Icon,QSql::In);

    QByteArray  Data;
    QBuffer     BufData(&Data);
    BufData.open(QIODevice::WriteOnly);
    Thumbnail.save(&BufData,"PNG");

    Query.bindValue(":Thumbnail",   Data,QSql::In);
    bool Ret=Query.exec();
    if (!Ret) {
        DisplayLastSQLError(&Query);
        return -1;
    } else return NextIndex;
}
