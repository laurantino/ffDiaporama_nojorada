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

// Basic inclusions (common to all files)
#include <QDir>
#include "_GlobalDefines.h"
#include <QFontDatabase>

QString CurrentAppName;                         // Application name (including devel, beta, ...)
QString CurrentAppVersion;                      // Application version read from BUILDVERSION.txt
double  ScreenFontAdjust=1;                     // System Font adjustement
double  ScaleFontAdjust=0;
int     SCALINGTEXTFACTOR=700;                  // 700 instead of 400 (ffD 1.0/1.1/1.2) to keep similar display from plaintext to richtext

#ifdef Q_OS_WIN

    bool IsWindowsXP=false;

    #include <windows.h>
    #include <winbase.h>
    #include <stdio.h>

    #if QT_VERSION<0x050000

        // set low fragmentation heap to remove memory error
        // from http://social.msdn.microsoft.com/forums/en-US/vclanguage/thread/7eec66a1-07b5-47aa-816d-7c1d7f7be274
        // NOTE: To enable the low-fragmentation heap when running under a debugger, set the _NO_DEBUG_HEAP environment variable to 1.
        void SetLFHeap() {

            // Re-attach stdio if application was started from a console
            BOOL (WINAPI *pAttachConsole)(DWORD dwProcessId);
            pAttachConsole = (BOOL (WINAPI*)(DWORD))
            GetProcAddress(LoadLibraryA("kernel32.dll"), "AttachConsole");

            if (pAttachConsole != NULL && pAttachConsole(((DWORD)-1))) {
               if (_fileno(stdout) < 0) freopen("CONOUT$","wb",stdout);
               if (_fileno(stderr) < 0) freopen("CONOUT$","wb",stderr);
               if (_fileno(stdin) < 0)  freopen("CONIN$","rb",stdin);
               std::ios::sync_with_stdio(); // Fix C++
            }

            // Check Windows System Version
            if (QSysInfo().WindowsVersion<0x0030) {     // prior to Windows XP

                ToLog(LOGMSG_CRITICAL,"Sorry but this application can't work on this system");
                exit(1);

            } else if (QSysInfo().WindowsVersion==0x0030) {    // If Windows XP

                IsWindowsXP=true;

                // Why would we have have to code it the hard way, that is by pulling the function out of the kernel32.dll?
                // VS 6.0 doesn't have the API defined in its headers.

                // Missing enum borrowed from: C:\Program Files\Microsoft Visual Studio 8\VC\PlatformSDK\Include\WinNT.h(8815)
                typedef enum _HEAP_INFORMATION_CLASS {
                    HeapCompatibilityInformation
                } HEAP_INFORMATION_CLASS;

                // Function pointer prototype
                typedef BOOL (WINAPI *Function_HeapSetInformation) (HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);

                WCHAR WinFileName[256+1];
                MultiByteToWideChar(CP_ACP,0,QString("kernel32.dll").toLocal8Bit(),-1,WinFileName,256+1);
                HMODULE hKernel32 = GetModuleHandle(WinFileName);

                if(hKernel32) {
                    Function_HeapSetInformation heapSetInfo;
                    ULONG heapFlags = 2;  // LFH == 2
                    HANDLE hProcessHeap = GetProcessHeap();
                    heapSetInfo = (Function_HeapSetInformation)GetProcAddress(hKernel32, "HeapSetInformation");
                    if (heapSetInfo) {
                        if(heapSetInfo(hProcessHeap, HeapCompatibilityInformation, &heapFlags, sizeof(ULONG))) {
                            ToLog(LOGMSG_INFORMATION,"DLLMain's Request for Low Fragmentation Heap for the Process Heap Successful");
                        } else {
                            ToLog(LOGMSG_WARNING,"DLLMain's Request for Low Fragmentation Heap for the Process Heap Unsuccessful.  Will Run with the Standard Heap Allocators");
                        }
                        #if _MSC_VER >= 1300
                        // no way to get the pointer to the CRT heap in VC 6.0 (_crtheap)
                        if(heapSetInfo((HANDLE)_get_heap_handle(), HeapCompatibilityInformation, &heapFlags, sizeof(ULONG))) {
                            ToLog(LOGMSG_INFORMATION,"DLLMain's Request for Low Fragmentation for the CRT Heap Successful");
                        } else {
                            ToLog(LOGMSG_WARNING,"DLLMain's Request for Low Fragmentation for the CRT Heap Unsuccessful.  Will Run with the Standard Heap Allocators");
                        }
                        #endif
                    } else {
                        ToLog(LOGMSG_WARNING,"DllMain unable to GetProcAddress for HeapSetInformation");
                    }
                } else {
                    ToLog(LOGMSG_WARNING,"DllMain unable to GetModuleHandle(kernel32.dll)");
                }
                // Only try to set the heap once.  If it fails, live with it.
            }
            // If > Windows/XP : nothing to do !
        }
    #endif
#endif


        void getMemInfo()
        {
           QString system_info;
           #ifdef Q_OS_WIN
           MEMORYSTATUSEX memory_status;
           ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
           memory_status.dwLength = sizeof(MEMORYSTATUSEX);
           if (GlobalMemoryStatusEx(&memory_status))
           {
              system_info.append(
                 QString("total RAM: %1 MB, free RAM: %2 MB")
                 .arg(memory_status.ullTotalPhys / (1024 * 1024)).arg(memory_status.ullAvailPhys / (1024 * 1024)));
           }
           else
           #endif
           {
              system_info.append("Unknown RAM");
           }
           //qDebug() << system_info;
        }

//====================================================================================================================

        QString UpInitials(QString Source) {
           for (int i = 0; i < Source.length(); i++)
              if ((i == 0) || (Source.at(i - 1) == ' ')) Source[i] = Source.at(i).toUpper();
           return Source;
        }

//====================================================================================================================

QString FormatLongDate(QDate EventDate) {
    //return UpInitials(EventDate.toString(Qt::DefaultLocaleLongDate));
    return UpInitials(QLocale().toString(EventDate, QLocale::LongFormat));
}

//====================================================================================================================

QString GetInformationValue(QString ValueToSearch, QStringList const *InformationList) 
{
   if (!InformationList) 
      return "";
   ValueToSearch.append("##");
   QStringList::ConstIterator i = InformationList->constBegin();
   while( i < InformationList->constEnd() )
   {
      if( (*i).startsWith(ValueToSearch) )
      {
         QStringList Values = (*i).split("##");
         if (Values.count() == 2) 
            return Values[1].trimmed();
      }
      i++;
   }
   return "";
}

QString GetInformationValue(QString ValueToSearch,const QStringList &InformationList)
{
   ValueToSearch.append("##");
   QStringList::ConstIterator i = InformationList.constBegin();
   while( i < InformationList.constEnd() )
   {
      if( (*i).startsWith(ValueToSearch) )
      {
         QStringList Values = (*i).split("##");
         if (Values.count() == 2) 
            return Values[1].trimmed();
      }
      i++;
   }
   return "";
}

//====================================================================================================================

QString GetCumulInfoStr(QStringList *InformationList,QString Key1,QString Key2) 
{
   int Num = 0;
   QString TrackNum = "";
   QString Value = "";
   QString Info = "";
   do 
   {
      TrackNum = QString("%1").arg(Num);
      while (TrackNum.length() < 3) 
         TrackNum = "0"+TrackNum;
      TrackNum = Key1 + "_"+TrackNum+":";
      Value = GetInformationValue(TrackNum+Key2,InformationList);
      if (Value != "") 
         Info = Info+((Num > 0) ? ",":"") + Value;
      // Next
      Num++;
   } while (Value != "");
   return Info;
}

//====================================================================================================================

QString ito2a(int val) {
    QString Ret=QString("%1").arg(val);
    while (Ret.length()<2) Ret="0"+Ret;
    return Ret;
}

QString ito3a(int val) {
    QString Ret=QString("%1").arg(val);
    while (Ret.length()<3) Ret="0"+Ret;
    return Ret;
}

//====================================================================================================================

QString GetTextSize(int64_t Size) {
    ToLog(LOGMSG_DEBUGTRACE,"IN:GetTextSize");

    QString UnitStr="";
    int     Unit   =0;

    while ((Size>1024*1024)&&(Unit<2)) {
        Unit++;
        Size=Size/1024;
    }
    switch (Unit) {
        case 0 : UnitStr=QApplication::translate("QCustomFolderTree","Kb","Unit Kb");   break;
        case 1 : UnitStr=QApplication::translate("QCustomFolderTree","Mb","Unit Mb");   break;
        case 2 : UnitStr=QApplication::translate("QCustomFolderTree","Gb","Unit Gb");   break;
        case 3 : UnitStr=QApplication::translate("QCustomFolderTree","Tb","Unit Tb");   break;
    }
    if (Size==0) return "0";
    else if (double(Size)/double(1024)>0.1) return QString("%1").arg(double(Size)/double(1024),8,'f',1).trimmed()+" "+UnitStr;
    else return "<0.1"+UnitStr;
}

//====================================================================================================================

//functions used to retrieve number of processor
//Thanks to : Stuart Nixon
//See : http://lists.trolltech.com/qt-interest/2006-05/thread00922-0.html
int getCpuCount() {
    ToLog(LOGMSG_DEBUGTRACE,"IN:getCpuCount");
    int cpuCount=1;

    #ifdef Q_OS_WIN
    SYSTEM_INFO    si;
    GetSystemInfo(&si);
    cpuCount = si.dwNumberOfProcessors;
    #elif defined(Q_OS_LINUX) || defined(Q_OS_SOLARIS)
    cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
    #endif
    if(cpuCount<1) cpuCount=1;
    return cpuCount;
}

//====================================================================================================================
// UTILITY FUNCTIONS
//====================================================================================================================

QAction *CreateMenuAction(QImage *Icon,QString Text,int Data,bool Checkable,bool IsCheck,QWidget *Parent) {
    QAction *Action;
    if (Icon)
        Action=new QAction(QIcon(QPixmap().fromImage(*Icon)),Text,Parent);
    else
        Action=new QAction(Text,Parent);
    Action->setIconVisibleInMenu(true);
    Action->setCheckable(Checkable);
    Action->setFont(QFont("Sans Serif",9));
    if (Checkable) Action->setChecked(IsCheck);
    Action->setData(QVariant(Data));
    return Action;
}

//====================================================================================================================

QAction *CreateMenuAction(QIcon Icon,QString Text,int Data,bool Checkable,bool IsCheck,QWidget *Parent) {
    QAction *Action;
    Action=new QAction(Icon,Text,Parent);
    Action->setIconVisibleInMenu(true);
    Action->setCheckable(Checkable);
    Action->setFont(QFont("Sans Serif",9));
    if (Checkable) Action->setChecked(IsCheck);
    Action->setData(QVariant(Data));
    return Action;
}

void xmlReadWrite::readBool(bool *dest, const QDomElement &Element, const QString &attribute)
{
   if( Element.hasAttribute(attribute) )
      *dest = (Element.attribute(attribute) == "1");   
}

QDomNode xmlReadWrite::node(const QString &name, const QString& value)
{
   QDomNode node = domDocument.createElement(name);
   node.appendChild(domDocument.createTextNode(value));
   return node;
}

QDomNode xmlReadWrite::node(const QString &name, int value)
{
   QString x;
   x.setNum(value);
   return node(name, x);
}

QDomNode xmlReadWrite::node(const QString &name, qlonglong value)
{
   QString x;
   x.setNum(value);
   return node(name, x);
}

QDomNode xmlReadWrite::node(const QString &name, bool value)
{
   if( value )
      return node(name, QString("1"));
   return node(name, QString("0"));
}

QDomNode xmlReadWrite::node(const QString &name, double value)
{
   QString x;
   char buf[256];
   int count = qsnprintf(buf, sizeof(buf), "%.16g", value);
   if (count > 0)
      x = QString::fromLatin1(buf, count);
   else
      x.setNum(value); // Fallback
   return node(name, x);
}

QDomNode xmlReadWrite::node(const QString &name, const QRect r)
{
   QDomNode domNode = domDocument.createElement(name);
   domNode.appendChild( node("x" , r.x()));
   domNode.appendChild( node("y" , r.y()));
   domNode.appendChild( node("width" , r.width()));
   domNode.appendChild( node("height" , r.height()));
   return domNode;
}

QDomNode xmlReadWrite::node(const QString &name, const QRectF rf)
{
   QDomNode domNode = domDocument.createElement(name);
   domNode.appendChild( node("x" , rf.x()));
   domNode.appendChild( node("y" , rf.y()));
   domNode.appendChild( node("width" , rf.width()));
   domNode.appendChild( node("height" , rf.height()));
   return domNode;
}
QXmlStreamAttribute xmlReadWrite::attribute(const QString &name, const QString& value)
{
   return QXmlStreamAttribute(name, value);
}
QXmlStreamAttribute xmlReadWrite::attribute(const QString &name, int value)
{
   return QXmlStreamAttribute(name, QString("%1").arg(value));
}

QXmlStreamAttribute xmlReadWrite::attribute(const QString &name, qlonglong value)
{
   return QXmlStreamAttribute(name, QString("%1").arg(value));
}
QXmlStreamAttribute xmlReadWrite::attribute(const QString &name, bool value)
{
   return QXmlStreamAttribute(name, (value ? "1" : "0"));
}
QXmlStreamAttribute xmlReadWrite::attribute(const QString &name, double value)
{
   QString x;
   char buf[256];
   int count = qsnprintf(buf, sizeof(buf), "%.16g", value);
   if (count > 0)
      x = QString::fromLatin1(buf, count);
   else
      x.setNum(value); // Fallback
   return QXmlStreamAttribute(name, x);
}
QXmlStreamAttributes xmlReadWrite::attributes(const QString &name, const QRect r)
{
   Q_UNUSED(name)
   QXmlStreamAttributes atts;
   atts.append("x", QString("%1").arg(r.x()));
   atts.append("y", QString("%1").arg(r.y()));
   atts.append("width", QString("%1").arg(r.width()));
   atts.append("height", QString("%1").arg(r.height()));
   return atts;
}
QXmlStreamAttributes xmlReadWrite::attributes(const QString &name, const QRectF rf)
{
   Q_UNUSED(name)
   QXmlStreamAttributes atts;
   atts.append("x", QString("%1").arg(rf.x()));
   atts.append("y", QString("%1").arg(rf.y()));
   atts.append("width", QString("%1").arg(rf.width()));
   atts.append("height", QString("%1").arg(rf.height()));
   return atts;
}

void xmlReadWrite::getAttrib(bool& value, const QDomElement& Element, const QString& attribute)
{
   if (Element.hasAttribute(attribute))
      value = (Element.attribute(attribute) == "1");
}

void xmlReadWrite::getAttrib(int& value, const QDomElement& Element, const QString& attribute)
{
   if (Element.hasAttribute(attribute))
   {
      bool IsOk = true;
      QString sValue = Element.attribute(attribute);
      int  iValue = sValue.toDouble(&IsOk);
      if(IsOk)
         value = iValue;
   }
}

void xmlReadWrite::getAttrib(double& value, const QDomElement& Element, const QString& attribute)
{
   if (Element.hasAttribute(attribute))
   {
      QString sValue = Element.attribute(attribute);
      bool    IsOk = true;
      double  dValue = sValue.toDouble(&IsOk);
      if (!IsOk)
      {
         for (int i = 0; i < sValue.length(); i++) 
            if (sValue[i] == ',') 
               sValue[i] = '.';
         dValue = sValue.toDouble(&IsOk);
      }
      if (IsOk)
         value = dValue;
   }
}

void repairFonts(QString& ffdPart)
{
#if QT_VERSION >= 0x060000
   if (QSysInfo::productType() == "windows")
#else
   if (QSysInfo::windowsVersion() != QSysInfo::WV_None)
#endif
      repairFontsWindows(ffdPart);
   else
      repairFontsLinux(ffdPart);
}

void repairFontsWindows(QString& ffdPart)
{
   // on windows replace Serif with Times New Roman and Sans Serif with Tahoma
   // also fix font-families with more than one font
   QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes", QSettings::NativeFormat);
   qDebug() << settings.value("MS Shell Dlg 2").toString().trimmed();
   QStringList availableFonts = QFontDatabase().families();
   //QFont* font;
   int startIndex = -1;
   int endIndex = -1;
   startIndex = ffdPart.indexOf("font-family:", 0);
   QString replSansSerif = settings.value("MS Shell Dlg 2").toString().trimmed();
   QString replSerif = QString("Times New Roman");
   bool needRepl = false;
   while (startIndex > 0)
   {
      endIndex = ffdPart.indexOf(";", startIndex);
      qDebug() << ffdPart.mid(startIndex, endIndex - startIndex);
      QString fonts = ffdPart.mid(startIndex, endIndex - startIndex);
      QStringList l = fonts.split("'", Qt::SkipEmptyParts);
      qDebug() << l << " (" << l.last() << ")";
      QString fontToUse = l.last();
      if (l.count() > 3 || !availableFonts.contains(fontToUse))
         needRepl = true;
      if (availableFonts.contains(fontToUse))
         qDebug() << "font is in Database";
      if (needRepl)
      {
         needRepl = false;
         if (fontToUse.compare("Sans Serif") == 0)
            fontToUse = replSansSerif;
         else if (fontToUse.compare("Serif") == 0)
            fontToUse = replSerif;
         QString newFont = QString("font-family:'%1'").arg(fontToUse);
         ffdPart.replace(startIndex, endIndex - startIndex, newFont);
         endIndex = startIndex + newFont.length();
         needRepl = false;
      }
      startIndex = ffdPart.indexOf("font-family:", endIndex);
   }

}

void repairFontsLinux(QString& ffdPart)
{
   // on linux replace Tahoma with Sans Serif and Times New Roman with Serif 
   // also fix font-families with more than one font
   QStringList availableFonts = QFontDatabase().families();
   //QFont* font;
   int startIndex = -1;
   int endIndex = -1;
   startIndex = ffdPart.indexOf("font-family:", 0);
   QString replTahoma = QString("Sans Serif");
   QString replTms = QString("Serif");
   bool needRepl = false;
   while (startIndex > 0)
   {
      endIndex = ffdPart.indexOf(";", startIndex);
      qDebug() << ffdPart.mid(startIndex, endIndex - startIndex);
      QString fonts = ffdPart.mid(startIndex, endIndex - startIndex);
      QStringList l = fonts.split("'", Qt::SkipEmptyParts);
      qDebug() << l << " (" << l.last() << ")";
      QString fontToUse = l.last();
      if (l.count() > 3 || !availableFonts.contains(fontToUse))
         needRepl = true;
      if (availableFonts.contains(fontToUse))
         qDebug() << "font is in Database";
      if (needRepl)
      {
         needRepl = false;
         if (fontToUse.compare("Tahoma") == 0)
            fontToUse = replTahoma;
         else if (fontToUse.compare("Times New Roman") == 0)
            fontToUse = replTms;
         QString newFont = QString("font-family:'%1'").arg(fontToUse);
         ffdPart.replace(startIndex, endIndex - startIndex, newFont);
         endIndex = startIndex + newFont.length();
         needRepl = false;
      }
      startIndex = ffdPart.indexOf("font-family:", endIndex);
   }

}

QFont getAppFont(appFontNums which)
{
   static QString* replSansSerif = 0;
#if QT_VERSION >= 0x060000
   if (QSysInfo::productType() == "windows")
#else
   if (QSysInfo::windowsVersion() != QSysInfo::WV_None)
#endif
   {
      if (replSansSerif == 0)
      {
         QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes", QSettings::NativeFormat);
         replSansSerif = new QString(settings.value("MS Shell Dlg 2").toString().trimmed());
      }
   }
   else
   {
      if (replSansSerif == 0)
         replSansSerif = new QString("Sans Serif");
   }
   switch (which)
   {
      case Sans8:       return QFont(*replSansSerif, 8, QFont::Normal, QFont::StyleNormal);
      case Sans8Bold:   return QFont(*replSansSerif, 8, QFont::Bold, QFont::StyleNormal);
      case Sans9:       return QFont(*replSansSerif, 9, QFont::Normal, QFont::StyleNormal);
      case Sans9Bold:   return QFont(*replSansSerif, 9, QFont::Bold, QFont::StyleNormal);
   }
   return QFont(*replSansSerif, 9, QFont::Normal, QFont::StyleNormal);
}
