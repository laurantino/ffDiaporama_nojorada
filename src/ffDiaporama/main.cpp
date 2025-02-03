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
    
    This Software with Version >2.2 devel was developed by Gerd Kokerbeck
    and Norbert Raschka
   ====================================================================== */

#define noUSE_VLD_CHECKING

#ifdef USE_VLD_CHECKING
#define VLD_FORCE_ENABLE
#include "vld.h"
//#define _CRTDBG_MAP_ALLOC
//  #include <stdlib.h>
//  #include <crtdbg.h>
//
//#include <new>
//
//namespace std { const nothrow_t nothrow = nothrow_t(); }
#endif


// Somethings needed by libav
#ifdef _STDINT_H
    #undef _STDINT_H            // Remove previous inclusion (if exist)
#endif
#define __STDC_CONSTANT_MACROS  // Activate macro for stdint
#include <stdint.h>             // Include stdint with macro activated
#include <iostream>

#include <QApplication>
#include <QTranslator>
#include <QtDebug>
#include <QSharedMemory>
#include <QSurfaceFormat>
#if QT_VERSION < 0x060000
#include <QAudioDeviceInfo>
#endif
#include "MainWindow/mainwindow.h"

//====================================================================================================================
bool CheckFolder(QString FileToTest,QString PathToTest) 
{
   QString Path = QDir(PathToTest).absolutePath();
   if (!Path.endsWith(QDir::separator())) 
      Path = Path + QDir::separator();
   bool IsFound = QFileInfo(Path + FileToTest).exists();
   if (IsFound) 
   {
      QDir::setCurrent(Path);
      ToLog(LOGMSG_INFORMATION,QString("Application found in directory %1").arg(QDir::toNativeSeparators(Path)+FileToTest));
   } 
   else 
   {
      ToLog(LOGMSG_INFORMATION,QString("Application not found in directory %1").arg(QDir::toNativeSeparators(Path)+FileToTest));
   }
   return IsFound;
}

//**************************************************
// First thing to do : ensure correct current path
// At program startup : CurrentPath is set to data folder (we search GlobalConfig file) that could be :
//      in current path
//      or in ../ApplicationGroupName
//      or in ../ApplicationName
//      or in $$PREFIX/share/ApplicationGroupName
//      or in $$PREFIX/share/ApplicationName
//**************************************************

bool SetWorkingPath(char * const argv[], QString ApplicationName) 
{
   QString StartupDir = QFileInfo(argv[0]).absolutePath();
   ToLog(LOGMSG_INFORMATION,"StartupDir " + QDir::toNativeSeparators(StartupDir));
   QDir::setCurrent(StartupDir);

   QString FileToTest = "BUILDVERSION.txt";
   QChar cSep = QDir().separator();
   QString dp("..");
   if (!CheckFolder(FileToTest,QDir::currentPath())
      && (!CheckFolder(FileToTest, dp + cSep + dp + cSep + dp + cSep + dp + cSep + ApplicationName)) // ../../../../ffdiaporama
      && (!CheckFolder(FileToTest, dp + cSep + dp + cSep + dp + cSep + ApplicationName))             // ../../../ffdiaporama
#if defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
      && (!CheckFolder(FileToTest,"/opt/share/" + ApplicationName)) // /opt/share/ffdiaporama
      && (!CheckFolder(FileToTest,"/usr/share/" + ApplicationName)) // /usr/share/ffdiaporama
#endif
      ) 
   {
      ToLog(LOGMSG_INFORMATION,QString("Critical error : Impossible to find application directory"));
      CustomMessageBox(NULL,QMessageBox::Critical,"ffDiaporama",
         QString("Critical error : Impossible to find application directory"),
         QMessageBox::Close);
      exit(1);
   }
   ToLog(LOGMSG_INFORMATION,"Set working path to " + QDir::toNativeSeparators(QDir::currentPath()));

   return true;
}

//====================================================================================================================
int diaporama_main(int argc, char* argv[])
{

   int     zero = 1;
   char    *WM_NAME[] = { (char *)APPLICATION_NAME };
   //QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
   QApplication app(zero, WM_NAME);
   QTranslator qtTranslator;
   if (qtTranslator.load(QLocale::system(),
               "qt", "_",
               QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
   {
       app.installTranslator(&qtTranslator);
   }

   QTranslator qtBaseTranslator;
   if (qtBaseTranslator.load("qtbase_" + QLocale::system().name(),
               QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
   {
       app.installTranslator(&qtBaseTranslator);
   }

  
    ToLog(LOGMSG_INFORMATION,QString("QT Version:%1").arg(qVersion()));
    ToLog(LOGMSG_INFORMATION, QString("compiled with QT Version :%1").arg(QT_VERSION_STR));
    //ToLog(LOGMSG_INFORMATION,QString("PrefixPath:%1").arg(QLibraryInfo::location(QLibraryInfo::PrefixPath)));
    //ToLog(LOGMSG_INFORMATION,QString("DocumentationPath:%1").arg(QLibraryInfo::location(QLibraryInfo::DocumentationPath)));
    //ToLog(LOGMSG_INFORMATION,QString("HeadersPath:%1").arg(QLibraryInfo::location(QLibraryInfo::HeadersPath)));
    //ToLog(LOGMSG_INFORMATION,QString("LibrariesPath:%1").arg(QLibraryInfo::location(QLibraryInfo::LibrariesPath)));
    //ToLog(LOGMSG_INFORMATION,QString("LibraryExecutablesPath:%1").arg(QLibraryInfo::location(QLibraryInfo::LibraryExecutablesPath)));
    //ToLog(LOGMSG_INFORMATION,QString("BinariesPath:%1").arg(QLibraryInfo::location(QLibraryInfo::BinariesPath)));
    //ToLog(LOGMSG_INFORMATION,QString("PluginsPath:%1").arg(QLibraryInfo::location(QLibraryInfo::PluginsPath)));
    //ToLog(LOGMSG_INFORMATION,QString("ImportsPath:%1").arg(QLibraryInfo::location(QLibraryInfo::ImportsPath)));
    //ToLog(LOGMSG_INFORMATION,QString("Qml2ImportsPath:%1").arg(QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath)));
    //ToLog(LOGMSG_INFORMATION,QString("ArchDataPath:%1").arg(QLibraryInfo::location(QLibraryInfo::ArchDataPath)));
    //ToLog(LOGMSG_INFORMATION,QString("DataPath:%1").arg(QLibraryInfo::location(QLibraryInfo::DataPath)));
    //ToLog(LOGMSG_INFORMATION,QString("TranslationsPath:%1").arg(QLibraryInfo::location(QLibraryInfo::TranslationsPath)));
    //ToLog(LOGMSG_INFORMATION,QString("ExamplesPath:%1").arg(QLibraryInfo::location(QLibraryInfo::ExamplesPath)));
    //ToLog(LOGMSG_INFORMATION,QString("TestsPath:%1").arg(QLibraryInfo::location(QLibraryInfo::TestsPath)));
    //ToLog(LOGMSG_INFORMATION,QString("SettingsPath:%1").arg(QLibraryInfo::location(QLibraryInfo::SettingsPath)));

    // Change startup directory to the one containing BUILDVERSION.txt
    SetWorkingPath(argv,APPLICATION_NAME);

    // Read build version to set global variable CurrentAppName and CurrentAppVersion
    QFile file("BUILDVERSION.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) 
      exit(1);
    CurrentAppName = QString(file.readLine());
    CurrentAppName.replace("Version ","");
    if (CurrentAppName.endsWith("\n"))   
      CurrentAppName = CurrentAppName.left(CurrentAppName.length() - QString("\n").length());
    while (CurrentAppName.endsWith(" ")) 
      CurrentAppName = CurrentAppName.left(CurrentAppName.length()-1);
    if (CurrentAppName.lastIndexOf(" ")) 
    {
        CurrentAppVersion = CurrentAppName.mid(CurrentAppName.lastIndexOf(" ")+1);
        CurrentAppName    = CurrentAppName.left(CurrentAppName.lastIndexOf(" "));
        CurrentAppName.replace("_"," ");
        CurrentAppName.replace("-"," ");
    }
    file.close();

    // Init application
    ToLog(LOGMSG_INFORMATION,QString("Start %1 %2 (%3) ...").arg(APPLICATION_NAME).arg(CurrentAppName).arg(CurrentAppVersion));

    app.setApplicationName(APPLICATION_NAME + QString(" ") + CurrentAppVersion);

    // Parse parameters to find ForceLanguage and AutoLoad
    QCommandLineParser clParser;
    clParser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    clParser.setApplicationDescription(app.applicationName());
    clParser.addHelpOption();
    clParser.addVersionOption();
    clParser.addOption(QCommandLineOption("lang", "choose language", "lang"));
    clParser.addOption(QCommandLineOption("loglevel", "choose logging level", "loglevel"));
    clParser.addPositionalArgument("<ffd-file>", QCoreApplication::translate("main", "Diaporama file to open."));
    ToLog(LOGMSG_INFORMATION, "Parse command line ...");
    QString AutoLoad = "";
    QString ForceLanguage = "";
    int     FuturLogMsgLevel = LogMsgLevel;

    clParser.process(app);
    if (clParser.isSet("lang"))
    {
       ForceLanguage = clParser.value("lang");
    }
    if (clParser.isSet("loglevel"))
    {
       FuturLogMsgLevel = clParser.value("loglevel").toInt();
    }
    QStringList l = clParser.positionalArguments();
    if (l.count())
       AutoLoad = l.first();

    // Log Level part
    if (FuturLogMsgLevel < 1 || FuturLogMsgLevel > 4) 
      FuturLogMsgLevel = 2;
    switch (FuturLogMsgLevel)
    {
        case 1 : ToLog(LOGMSG_INFORMATION,"Set LogLevel to DEBUGTRACE");    break;
        case 2 : ToLog(LOGMSG_INFORMATION,"Set LogLevel to INFORMATION");   break;
        case 3 : ToLog(LOGMSG_INFORMATION,"Set LogLevel to WARNING");       break;
        case 4 :
        default: ToLog(LOGMSG_INFORMATION,"Set LogLevel to CRITICAL");      break;
    }
    LogMsgLevel = FuturLogMsgLevel;

    // Start GUI
    ToLog(LOGMSG_INFORMATION,"Start GUI ...");
    MainWindow w(ForceLanguage);
    w.FileForIO = AutoLoad;

    ToLog(LOGMSG_INFORMATION,QApplication::translate("MainWindow","Start ..."));

    int iRet = app.exec();
    return iRet;
}

#include "SSE_Check.h"
//#include <sys/sysinfo.h>
int main(int argc, char* argv[])
{
   getCPUFlags();
   //struct sysinfo memInfo;
   //memset(&memInfo,0,sizeof(memInfo));
   //sysinfo(&memInfo);
   int iRet = diaporama_main(argc, argv);
   #ifdef USE_VLD_CHECKING
   VLDReportLeaks();
   #endif
   return iRet;
}
