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

#include "BasicDefines.h"

//======================================================================
// Internal log defines and functions
//======================================================================

int         LogMsgLevel = LOGMSG_INFORMATION;    // Level from wich debug message was print to stdout
QStringList EventList;                           // Internal event queue
QObject     *EventReceiver = NULL;               // Windows wich receive event

//====================================================================================================================

void PostEvent(int EventType,QString EventParam) 
{
   EventList.append(QString(QLatin1String("%1###;###%2")).arg(EventType).arg(EventParam));
   if (EventReceiver != NULL) 
      QApplication::postEvent(EventReceiver, new QEvent(BaseAppEvent));
}

//====================================================================================================================

bool    PreviousBreak = true;
QMutex  LogMutex;
QFile   *logFile = NULL;

#ifdef Q_OS_WIN
std::string toAscii(QString tab) 
{
   char buffer[2048];
   CharToOemA(tab.toLocal8Bit().constData(), buffer);
   std::string str(buffer);
   return str;
}
#endif

bool logWithTime = false;
void ToLog(int MessageType, QString Message, QString Source, bool AddBreak) 
{
   LogMutex.lock();
   if( logFile == NULL )
   {
      QString logfilename = QDir::tempPath() + "/" + "ffdiaporama.log";
      logFile = new QFile(logfilename);
      logFile->open(QIODevice::Truncate|QIODevice::Text|QIODevice::WriteOnly|QIODevice::Unbuffered);
   }
   if ((MessageType >= LogMsgLevel) && (PreviousBreak)) 
   {
      QString DateTime;
      if(logWithTime) 
         DateTime = QTime::currentTime().toString(QStringLiteral("hh:mm:ss.zzz"));
#ifdef Q_OS_WIN
      if (Message.endsWith("\n")) Message = Message.left(Message.length() - QString("\n").length());
      if (Message.endsWith(char(10))) Message = Message.left(Message.length() - QString(char(10)).length());
      if (Message.endsWith(char(13))) Message = Message.left(Message.length() - QString(char(13)).length());
      if (Message.endsWith(char(10))) Message = Message.left(Message.length() - QString(char(10)).length());
#endif
      QString MSG = "";
      if ((Message != "LIBAV: No accelerated colorspace conversion found from yuv422p to rgb24.") 
         && (Message!="LIBAV: Increasing reorder buffer to 1")
         && (!Message.startsWith("LIBAV: Reference"))
         && (!Message.startsWith("LIBAV: error while decoding MB"))
         && (!Message.startsWith("LIBAV: left block unavailable for requested"))
         && (!Message.startsWith("LIBAV: max_analyze_duration reached"))
         && (!(Message.startsWith("LIBAV: mode:") && Message.contains("parity:") && (Message.contains("auto_enable:") || Message.contains("deint:"))) )
         && (!(Message.startsWith("LIBAV: w:") && Message.contains("h:") && Message.contains("pixfmt:")))
         ) 
      {
         switch (MessageType) 
         {
            case LOGMSG_DEBUGTRACE:    
               MSG = QString("[" + DateTime + ":DEBUG]\t"     + Message+(AddBreak?"\n":""));  
            break;
            case LOGMSG_INFORMATION:   
               MSG = QString("[" + DateTime + ":INFO]\t"      + Message+(AddBreak?"\n":""));  
            break;
            case LOGMSG_WARNING:       
               MSG = QString("[" + DateTime + ":WARNING]\t"   + Message+(AddBreak?"\n":""));  
            break;
            case LOGMSG_CRITICAL:      
               MSG = QString("[" + DateTime + ":ERROR]\t"     + Message+(AddBreak?"\n":""));  
            break;
         }                                                                              
      }
      if (!MSG.isEmpty()) 
      {
         if ( (MessageType != LOGMSG_DEBUGTRACE) && (EventReceiver != NULL))  
            PostEvent(EVENT_GeneralLogChanged,QString("%1###:###%2###:###%3").arg((int)MessageType).arg(Message).arg(Source));
#ifdef Q_OS_WIN
         //std::cout << toAscii(MSG) << std::flush;
         //fprintf(stderr, "%s", MSG.toAscii().constData());
         //qDebug() << MSG;
         logFile->write(MSG.toLocal8Bit().constData());
         if (MSG.endsWith("\n")) MSG=MSG.left(MSG.indexOf("\n"));
#else
         //std::cout << MSG.toLocal8Bit().constData() << std::flush;
         logFile->write(MSG.toLocal8Bit().constData());
         if (MSG.endsWith("\n")) MSG=MSG.left(MSG.indexOf("\n"));
#endif
      }
      PreviousBreak = (AddBreak || (Message.endsWith("\n")));
   } 
   else if (MessageType >= LogMsgLevel) 
   {
#ifdef Q_OS_WIN
      //std::cout << toAscii(Message) << std::flush;
      //fprintf(stderr, "%s", Message.toAscii().constData());
      logFile->write(Message.toLocal8Bit().constData());
      //qDebug() << Message;
#else
       logFile->write(Message.toLocal8Bit().constData());
//       qDebug() << Message;
       //std::cout << Message.toLocal8Bit().constData() << std::flush;
#endif
   }
   LogMutex.unlock();
}

//====================================================================================================================

double GetDoubleValue(QDomElement CorrectElement,QString Name) 
{
   QString sValue = CorrectElement.attribute(Name);
   bool    IsOk = true;
   double  dValue = sValue.toDouble(&IsOk);
   if (!IsOk) 
   {
      for (int i = 0; i < sValue.length(); i++) if (sValue[i] == ',') sValue[i] = '.';
      //sValue=sValue.replace(",",".");
      dValue = sValue.toDouble(&IsOk);
   }
   return dValue;
}

double GetDoubleValue(QString sValue) 
{
   bool    IsOk = true;
   double  dValue = sValue.toDouble(&IsOk);
   if (!IsOk) {
      sValue = sValue.replace(",", ".");
      dValue = sValue.toDouble(&IsOk);
   }
   return dValue;
}

#include <QTransform>
#define PI M_PI
bool CalcWorkspace(qreal ImgWidth, qreal ImgHeight, double ImageRotation, qreal dmax, qreal *maxw, qreal *maxh, qreal *minw, qreal *minh)
{
   if ((ImgWidth == 0) || (ImgHeight == 0))
      return false;

   //*dmax = sqrt(qreal(ImgWidth*ImgWidth) + qreal(ImgHeight*ImgHeight));   // Calc hypothenuse of the image to define full canvas

   // calc rectangle before rotation
   qreal  rx = ImgWidth / 2.0;
   qreal  ry = ImgHeight / 2.0;

   double ir_pi_180 = ImageRotation*PI / 180.0;
   qreal dm2 = dmax / 2;
   QList<qreal> xlist, ylist;
   xlist.append(-rx*cos(ir_pi_180) + ry*sin(ir_pi_180) + dm2);
   xlist.append(+rx*cos(ir_pi_180) + ry*sin(ir_pi_180) + dm2);
   xlist.append(-rx*cos(ir_pi_180) - ry*sin(ir_pi_180) + dm2);
   xlist.append(+rx*cos(ir_pi_180) - ry*sin(ir_pi_180) + dm2);

   ylist.append(-rx*sin(ir_pi_180) + ry*cos(ir_pi_180) + dm2);
   ylist.append(+rx*sin(ir_pi_180) + ry*cos(ir_pi_180) + dm2);
   ylist.append(-rx*sin(ir_pi_180) - ry*cos(ir_pi_180) + dm2);
   ylist.append(+rx*sin(ir_pi_180) - ry*cos(ir_pi_180) + dm2);

   // Sort xtab and ytab
   //QT_QSORT(xlist.begin(), xlist.end());
   //QT_QSORT(ylist.begin(),xlist.end());
   std::sort(xlist.begin(), xlist.end()); // qt6
   std::sort(ylist.begin(), ylist.end()); // qt6
   //qSort(xlist);
   //qSort(ylist);
   *maxw = floor(xlist[3] - xlist[0]);   *minw = floor(xlist[2] - xlist[1]);
   *maxh = floor(ylist[3] - ylist[0]);   *minh = floor(ylist[2] - ylist[1]);
   return true;
}


/*
const char *PC2time(LONGLONG ticks, bool withMillis)
{
	static char clockBuffer[128];
   LARGE_INTEGER g_Frequency;
   QueryPerformanceFrequency((LARGE_INTEGER*)&g_Frequency);
	LONGLONG millis,nanos;
	LONGLONG lSeconds = ticks/g_Frequency.QuadPart;
	millis = (((ticks) * 1000i64) / g_Frequency.QuadPart)%1000i64;
   nanos = (((ticks) * 1000000i64) / g_Frequency.QuadPart)%1000i64;
   double d = (double)(ticks) / (double) g_Frequency.QuadPart;

   // To get duration in milliseconds
	//double dftDuration = (double) llTimeDiff * 1000.0 / (double) Frequency.QuadPart;
	//__int64 ms=((ticks) * 1000i64) / g_Frequency.QuadPart;

	long s = lSeconds % 60L;
	long m = lSeconds / 60L;
	long h = lSeconds / 3600L;
	m -= (h*60);
	if( withMillis )
		sprintf_s( clockBuffer,128,"%02d:%02d:%02d.%03lld.%03lld (%f secs)",h,m,s,millis,nanos,d);
	else
		sprintf_s( clockBuffer,128,"%02d:%02d:%02d",h,m,s);
	return clockBuffer;
}

LONGLONG curPCounter()
{
   LARGE_INTEGER g_CurentCount;
   QueryPerformanceCounter((LARGE_INTEGER*)&g_CurentCount);
   return g_CurentCount.QuadPart;
}
*/
/*
 * const char *nano2time(qint64 ticks, bool withMillis)
{
	static char clockBuffer[128];
	qint64 millis,nanos,mikros;
	qint64 lSeconds = ticks/1000000000ll;
	millis = ticks / 1000000LL % 1000i64;
   mikros = ticks / 1000LL % 1000i64;
   nanos = ticks % 1000i64;
   double d = (double)(ticks) / (double) 1000000000LL;
	long s = lSeconds % 60L;
	long m = lSeconds / 60L;
	long h = lSeconds / 3600L;
	m -= (h*60);
	if( withMillis )
		sprintf_s( clockBuffer,128,"%02d:%02d:%02d.%03lld.%03lld.%03lld (%f secs, %f fps)",h,m,s,millis,mikros,nanos,d,1.0/d);
	else
		sprintf_s( clockBuffer,128,"%02d:%02d:%02d",h,m,s);
	return clockBuffer;
}
*/
autoTimer::autoTimer(const QString &s)
{
   messageText = s;
   timer.start();
}

autoTimer::~autoTimer()
{
   //qDebug() << messageText << " " <<  nano2time(timer.nsecsElapsed(),true);
}

