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

   // Include some additional standard class
#include "cVideoFile.h"
#include "_Diaporama.h"
#include "ffTools.h"
extern "C" {
   #include "libavutil/imgutils.h"
}
#ifdef CUDA_MEMCHECK
#include "cuda.h"
#include "cuda_runtime_api.h"
#endif

#ifdef IS_FFMPEG_414

#define USE_YUVCACHE_MAP
#define NO_SEEK_DEBUG

#ifdef _MSC_VER
#pragma warning(default: 4996)      /* enable deprecation */
#endif
const char *cuvid_mappings[] =
{
   "h264|h264_cuvid",
   "hevc|hevc_cuvid",
   "mjpeg|mjpeg_cuvid",
   "mpeg1video|mpeg1_cuvid",
   "mpeg2video|mpeg2_cuvid",
   "mpeg4|mpeg4_cuvid",
   "vc1|vc1_cuvid",
   "vp8|vp8_cuvid",
   "vp9|vp9_cuvid",
   ""
};
//****************************************************************************************************************************************************************

//#define FFD_APPLICATION_ROOTNAME    "Project"           // Name of root node in the project xml file
#define MaxAudioLenDecoded          AVCODEC_MAX_AUDIO_FRAME_SIZE*4

#ifndef INT64_MAX
#define 	INT64_MAX   0x7fffffffffffffffLL
#define 	INT64_MIN   (-INT64_MAX - 1LL)
#endif

//#define PIXFMT          PIX_FMT_RGB24
//#define QTPIXFMT        QImage::Format_RGB888
#define PIXFMT      AV_PIX_FMT_BGRA
#define QTPIXFMT    QImage::Format_ARGB32

//====================================================================================================================


/*************************************************************************************************************************************
    CLASS cImageInCache
*************************************************************************************************************************************/
cImageInCache::cImageInCache(int64_t Position, AVFrame *FiltFrame, AVFrame *FrameBufferYUV)
{
   this->Position = Position;
   this->FiltFrame = FiltFrame;
   this->FrameBufferYUV = FrameBufferYUV;
}

cImageInCache::~cImageInCache()
{
   if (FiltFrame)
   {
      FREEFRAME(&FiltFrame);
   }
   if (FrameBufferYUV)
   {
      FREEFRAME(&FrameBufferYUV);
   }
   //FREEFRAME(&FrameBufferYUV);
}

/*************************************************************************************************************************************
    CLASS cVideoFile
*************************************************************************************************************************************/

cVideoFile::cVideoFile(cApplicationConfig *ApplicationConfig) : cBaseMediaFile(ApplicationConfig)
{
   Reset(OBJECTTYPE_VIDEOFILE);
   yuv2rgbImage = NULL;
   FrameBufferRGB = 0;
   img_convert_ctx = 0;
   isCUVIDDecoder = false;
   useDeinterlace = true;
}

void cVideoFile::Reset(OBJECTTYPE TheWantedObjectType)
{
   cBaseMediaFile::Reset();

   MusicOnly = (TheWantedObjectType == OBJECTTYPE_MUSICFILE);
   ObjectType = TheWantedObjectType;
   IsOpen = false;
   StartTime = QTime(0, 0, 0, 0);   // Start position
   EndTime = QTime(0, 0, 0, 0);   // End position
   AVStartTime = QTime(0, 0, 0, 0);
   LibavStartTime = 0;

   // Video part
   IsMTS = false;
   LibavVideoFile = NULL;
   VideoDecoderCodec = NULL;
   VideoCodecContext = nullptr;
   VideoStreamNumber = 0;
   FrameBufferYUV = NULL;
   FrameBufferYUVReady = false;
   FrameBufferYUVPosition = 0;
   VideoCodecInfo = "";
   VideoTrackNbr = 0;
   VideoStreamNumber = -1;
   NbrChapters = 0;

   // Audio part
   LibavAudioFile = NULL;
   AudioDecoderCodec = NULL;
   AudioCodecContext = NULL;
   LastAudioReadPosition = -1;
   IsVorbis = false;
   AudioCodecInfo = "";
   AudioTrackNbr = 0;
   AudioStreamNumber = -1;

   // Audio resampling
   RSC = NULL;
   RSC_InChannels = 2;
   RSC_OutChannels = 2;
   RSC_InSampleRate = 48000;
   RSC_OutSampleRate = 48000;
   av_channel_layout_default(&RSC_InChannelLayout, 2);
   av_channel_layout_default(&RSC_OutChannelLayout, 2);
   RSC_InSampleFmt = AV_SAMPLE_FMT_S16;
   RSC_OutSampleFmt = AV_SAMPLE_FMT_S16;

   // Filter part
   VideoFilterGraph = NULL;
   VideoFilterIn = NULL;
   VideoFilterOut = NULL;
}

//====================================================================================================================

cVideoFile::~cVideoFile()
{
   // Close LibAVFormat and LibAVCodec contexte for the file
   CloseCodecAndFile();
   if (yuv2rgbImage)
      delete yuv2rgbImage;
}

//====================================================================================================================

bool cVideoFile::DoAnalyseSound(QList<qreal> *Peak, QList<qreal> *Moyenne, bool *CancelFlag, volatile qreal *Analysed)
{
   bool IsAnalysed = LoadAnalyseSound(Peak, Moyenne);
   if (IsAnalysed)
      return IsAnalysed;

   qint64          NewPosition = 0, Position = -1;
   qint64          Duration = QTime(0, 0, 0, 0).msecsTo(GetRealDuration());
   int             WantedValues;
   QList<qreal>    Values;
   int16_t         *Block = NULL, *CurData = NULL;
   cSoundBlockList AnalyseMusic;

   AnalyseMusic.SetFPS(2000, 2, 1000, AV_SAMPLE_FMT_S16);
   WantedValues = (Duration / 2000);
   Peak->clear();
   Moyenne->clear();

   //*******************************************************************************************
   // Load music and compute music count, max value, 2000 peak and 2000 moyenne values
   // decibels=decibels>0?0.02*log10(decibels/32768.0):0;    // PCM S16 is from -48db to +48db
   //*******************************************************************************************
   while ((!*CancelFlag) && (Position != NewPosition))
   {
      *Analysed = qreal(NewPosition) / qreal(Duration);
      QApplication::processEvents();
      Position = NewPosition;
      ReadFrame(true, Position * 1000, true, false, &AnalyseMusic, 1, true);
      NewPosition += qreal(AnalyseMusic.ListCount())*AnalyseMusic.dDuration*qreal(1000);
      while (AnalyseMusic.ListCount() > 0)
      {
         Block = AnalyseMusic.DetachFirstPacket();
         if (Block)
         {
            CurData = Block;
            Values.reserve(WantedValues);
            for (int j = 0; j < AnalyseMusic.SoundPacketSize / 4; j++)
            {
               //int16_t sample16Bit = *CurData++;
               double  decibels1 = abs(*CurData++);
               //sample16Bit = *CurData++;
               double  decibels2 = abs(*CurData++);
               //double  decibels = (decibels1 + decibels2)/2;
               Values.append((decibels1 + decibels2) / 2);
               if (Values.count() == WantedValues)
               {
                  qreal vPeak = 0, vMoyenne = 0;
                  foreach(qreal V, Values)
                  {
                     if (vPeak < V)
                        vPeak = V;
                     vMoyenne = vMoyenne + V;
                  }
                  vMoyenne = vMoyenne / Values.count();
                  Peak->append(vPeak);
                  Moyenne->append(vMoyenne);
                  Values.clear();
                  Values.reserve(WantedValues);
               }
            }
            av_free(Block);
         }
      }
   }
   // tempdata
   CurData = (int16_t *)AnalyseMusic.TempData;
   for (int j = 0; j < AnalyseMusic.CurrentTempSize / 4; j++)
   {
      //int16_t sample16Bit = *CurData++;
      double  decibels1 = abs(*CurData++);
      //sample16Bit = *CurData++;
      double  decibels2 = abs(*CurData++);
      //double  decibels = (decibels1 + decibels2)/2;
      Values.append((decibels1 + decibels2) / 2);
      if (Values.count() == WantedValues)
      {
         qreal vPeak = 0, vMoyenne = 0;
         foreach(qreal V, Values)
         {
            if (vPeak < V)
               vPeak = V;
            vMoyenne = vMoyenne + V;
         }
         vMoyenne = vMoyenne / Values.count();
         Peak->append(vPeak);
         Moyenne->append(vMoyenne);
         Values.clear();
         Values.reserve(WantedValues);
      }
   }
   if (Values.count() > 0)
   {
      qreal vPeak = 0, vMoyenne = 0;
      foreach(qreal V, Values)
      {
         if (vPeak < V)
            vPeak = V;
         vMoyenne = vMoyenne + V;
      }
      vMoyenne = vMoyenne / Values.count();
      Peak->append(vPeak);
      Moyenne->append(vMoyenne);
      Values.clear();
   }

   // Compute MaxSoundValue as 90% of the max peak value
   QList<qreal> MaxVal;
   MaxVal = *Peak;
   //foreach (qreal Value,*Peak) 
   //   MaxVal.append(Value);
   QT_QSORT(MaxVal.begin(), MaxVal.end());
   qreal MaxSoundValue = MaxVal.count() > 0 ? MaxVal[MaxVal.count()*0.9] : 1.0;
   if (MaxSoundValue == 0)
      MaxSoundValue = 1.0;

   // Adjust Peak and Moyenne values by transforming them as % of the max value
   for (int i = 0; i < Peak->count(); i++)
   {
      (*Peak)[i] = (*Peak)[i] / MaxSoundValue;
      (*Moyenne)[i] = (*Moyenne)[i] / MaxSoundValue;
   }
   MaxVal = *Moyenne;
   //MaxVal.clear();
   //foreach (qreal Value,*Moyenne) 
   //   MaxVal.append(Value);
   QT_QSORT(MaxVal.begin(), MaxVal.end());
   MaxSoundValue = MaxVal.count() > 0 ? MaxVal[MaxVal.count()*0.9] : 1.0;
   if (MaxSoundValue <= 0)
      MaxSoundValue = 1.0;
   //**************************
   // End analyse
   //**************************
   IsAnalysed = true;
   SaveAnalyseSound(Peak, Moyenne, MaxSoundValue);
   if (EndTime > GetRealDuration())
      EndTime = GetRealDuration();
   return IsAnalysed;
}

//====================================================================================================================

bool cVideoFile::LoadBasicInformationFromDatabase(QDomElement *ParentElement, QString, QString, QStringList *, bool *, TResKeyList *, bool)
{
   imageWidth = ParentElement->attribute("ImageWidth").toInt();
   imageHeight = ParentElement->attribute("ImageHeight").toInt();
   imageOrientation = ParentElement->attribute("ImageOrientation").toInt();
   geometry = ParentElement->attribute("ObjectGeometry").toInt();
   aspectRatio = GetDoubleValue(*ParentElement, "AspectRatio");
   NbrChapters = ParentElement->attribute("NbrChapters").toInt();
   VideoStreamNumber = ParentElement->attribute("VideoStreamNumber").toInt();
   VideoTrackNbr = ParentElement->attribute("VideoTrackNbr").toInt();
   AudioStreamNumber = ParentElement->attribute("AudioStreamNumber").toInt();
   AudioTrackNbr = ParentElement->attribute("AudioTrackNbr").toInt();
   if (ParentElement->hasAttribute("Duration"))
      SetGivenDuration(QTime(0, 0, 0, 0).addMSecs(ParentElement->attribute("Duration").toLongLong()));
   if (ParentElement->hasAttribute("RealDuration"))
      SetRealAudioDuration(QTime(0, 0, 0, 0).addMSecs(ParentElement->attribute("RealDuration").toLongLong()));
   if (ParentElement->hasAttribute("RealAudioDuration"))
      SetRealAudioDuration(QTime(0, 0, 0, 0).addMSecs(ParentElement->attribute("RealAudioDuration").toLongLong()));
   if (ParentElement->hasAttribute("RealVideoDuration"))
      SetRealVideoDuration(QTime(0, 0, 0, 0).addMSecs(ParentElement->attribute("RealVideoDuration").toLongLong()));
   if (ParentElement->hasAttribute("SoundLevel"))
      SetSoundLevel(GetDoubleValue(*ParentElement, "SoundLevel"));
   if (ParentElement->hasAttribute("IsComputedAudioDuration"))
      IsComputedAudioDuration = ParentElement->attribute("IsComputedAudioDuration") == "1";
   if (EndTime == QTime(0, 0, 0, 0))
      EndTime = GetRealDuration();
   return true;
}

//====================================================================================================================

void cVideoFile::SaveBasicInformationToDatabase(QDomElement *ParentElement, QString, QString, bool, cReplaceObjectList *, QList<qlonglong> *, bool)
{
   ParentElement->setAttribute("ImageWidth", imageWidth);
   ParentElement->setAttribute("ImageHeight", imageHeight);
   ParentElement->setAttribute("ImageOrientation", imageOrientation);
   ParentElement->setAttribute("ObjectGeometry", geometry);
   ParentElement->setAttribute("AspectRatio", QString("%1").arg(aspectRatio, 0, 'f'));
   ParentElement->setAttribute("Duration", QTime(0, 0, 0, 0).msecsTo(GetGivenDuration()));
   ParentElement->setAttribute("RealAudioDuration", QTime(0, 0, 0, 0).msecsTo(GetRealAudioDuration()));
   if (ObjectType == OBJECTTYPE_VIDEOFILE)
      ParentElement->setAttribute("RealVideoDuration", QTime(0, 0, 0, 0).msecsTo(GetRealVideoDuration()));
   ParentElement->setAttribute("SoundLevel", GetSoundLevel());
   ParentElement->setAttribute("IsComputedAudioDuration", IsComputedAudioDuration ? "1" : "0");
   ParentElement->setAttribute("NbrChapters", NbrChapters);
   ParentElement->setAttribute("VideoStreamNumber", VideoStreamNumber);
   ParentElement->setAttribute("VideoTrackNbr", VideoTrackNbr);
   ParentElement->setAttribute("AudioStreamNumber", AudioStreamNumber);
   ParentElement->setAttribute("AudioTrackNbr", AudioTrackNbr);
}

//====================================================================================================================
// Overloaded function use to dertermine if format of media file is correct
bool cVideoFile::CheckFormatValide(QWidget *Window)
{
   bool IsOk = IsValide;

   // try to open file
   if (!OpenCodecAndFile())
   {
      QString ErrorMessage = QApplication::translate("MainWindow", "Format not supported", "Error message");
      CustomMessageBox(Window, QMessageBox::Critical, QApplication::translate("MainWindow", "Error", "Error message"), ShortName() + "\n\n" + ErrorMessage, QMessageBox::Close);
      IsOk = false;
   }

   // check if file have at least one sound track compatible
   if (IsOk && (AudioStreamNumber != -1))
   {
      //if (!((LibavAudioFile->streams[AudioStreamNumber]->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_S16) || (LibavAudioFile->streams[AudioStreamNumber]->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_U8)))
      if (!((AudioCodecContext->sample_fmt != AV_SAMPLE_FMT_S16) || (AudioCodecContext->sample_fmt != AV_SAMPLE_FMT_U8)))
      {
         QString ErrorMessage = "\n" + QApplication::translate("MainWindow", "This application support only audio track with unsigned 8 bits or signed 16 bits sample format", "Error message");
         CustomMessageBox(Window, QMessageBox::Critical, QApplication::translate("MainWindow", "Error", "Error message"), ShortName() + "\n\n" + ErrorMessage, QMessageBox::Close);
         IsOk = false;
      }

   }

   // Try to load an image to ensure all is ok
   if (IsOk)
   {
      QString FileExtension = QFileInfo(FileName()).completeSuffix().toLower();
      bool Deinterlace = (ApplicationConfig->Deinterlace) && ((FileExtension == "mts") || (FileExtension == "m2ts") || (FileExtension == "mod") || (FileExtension == "ts"));

      QImage *Image = ImageAt(true, 0, NULL, Deinterlace, 1, false, false);
      if (Image)
      {
         delete Image;
      }
      else
      {
         QString ErrorMessage = "\n" + QApplication::translate("MainWindow", "Impossible to read one image from the file", "Error message");
         CustomMessageBox(Window, QMessageBox::Critical, QApplication::translate("MainWindow", "Error", "Error message"), ShortName() + "\n\n" + ErrorMessage, QMessageBox::Close);
         IsOk = false;
      }
   }
   // close file if it was opened
   CloseCodecAndFile();

   return IsOk;
}

//====================================================================================================================

// Overloaded function use to dertermine if media file correspond to WantedObjectType
//      WantedObjectType could be OBJECTTYPE_VIDEOFILE or OBJECTTYPE_MUSICFILE
//      if AudioOnly was set to true in constructor then ignore all video track and set WantedObjectType to OBJECTTYPE_MUSICFILE else set it to OBJECTTYPE_VIDEOFILE
//      return true if WantedObjectType=OBJECTTYPE_VIDEOFILE and at least one video track is present
//      return true if WantedObjectType=OBJECTTYPE_MUSICFILE and at least one audio track is present
bool cVideoFile::GetChildFullInformationFromFile(bool, cCustomIcon *Icon, QStringList *ExtendedProperties)
{
   //Mutex.lock();
   bool            Continu = true;
   AVFormatContext *LibavFile = NULL;
   QString         sFileName = FileName();

   //*********************************************************************************************************
   // Open file and get a LibAVFormat context and an associated LibAVCodec decoder
   //*********************************************************************************************************
   char filename[512];
   strcpy(filename, sFileName.toLocal8Bit());
   LibavFile = avformat_alloc_context();
   LibavFile->flags |= AVFMT_FLAG_GENPTS;       // Generate missing pts even if it requires parsing future NbrFrames.
   if (avformat_open_input(&LibavFile, filename, NULL, NULL) != 0)
   {
      LibavFile = NULL;
      return false;
   }
   ExtendedProperties->append(QString("Short Format##") + QString(LibavFile->iformat->name));
   ExtendedProperties->append(QString("Long Format##") + QString(LibavFile->iformat->long_name));

   //*********************************************************************************************************
   // Search stream in file
   //*********************************************************************************************************
   if (avformat_find_stream_info(LibavFile, NULL) < 0)
   {
      avformat_close_input(&LibavFile);
      LibavFile = NULL;
      Continu = false;
   }

   if (Continu)
   {
      //*********************************************************************************************************
      // Get metadata
      //*********************************************************************************************************
      AVDictionaryEntry *tag = NULL;
      while ((tag = av_dict_get(LibavFile->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
      {
         QString Value = QString().fromUtf8(tag->value);
         #ifdef Q_OS_WIN
         Value.replace(char(13), "\n");
         #endif
         if (Value.endsWith("\n")) Value = Value.left(Value.lastIndexOf("\n"));
         ExtendedProperties->append(QString().fromUtf8(tag->key).toLower() + QString("##") + Value);
      }

      //*********************************************************************************************************
      // Get chapters
      //*********************************************************************************************************
      NbrChapters = 0;
      for (uint i = 0; i < LibavFile->nb_chapters; i++)
      {
         AVChapter   *ch = LibavFile->chapters[i];
         QString     ChapterNum = QString("%1").arg(NbrChapters);
         while (ChapterNum.length() < 3) ChapterNum = "0" + ChapterNum;
         int64_t Start = double(ch->start)*(double(av_q2d(ch->time_base)) * 1000);     // Lib AV use 1/1 000 000 000 sec and we want msec !
         int64_t End = double(ch->end)*(double(av_q2d(ch->time_base)) * 1000);       // Lib AV use 1/1 000 000 000 sec and we want msec !

         // Special case if it's first chapter and start!=0 => add a chapter 0
         if ((NbrChapters == 0) && (LibavFile->chapters[i]->start > 0))
         {
            ExtendedProperties->append("Chapter_" + ChapterNum + ":Start" + QString("##") + QTime(0, 0, 0, 0).toString("hh:mm:ss.zzz"));
            ExtendedProperties->append("Chapter_" + ChapterNum + ":End" + QString("##") + QTime(0, 0, 0, 0).addMSecs(Start).toString("hh:mm:ss.zzz"));
            ExtendedProperties->append("Chapter_" + ChapterNum + ":Duration" + QString("##") + QTime(0, 0, 0, 0).addMSecs(Start).toString("hh:mm:ss.zzz"));
            if (GetInformationValue("title", ExtendedProperties) != "")
               ExtendedProperties->append("Chapter_" + ChapterNum + ":title##" + GetInformationValue("title", ExtendedProperties));
            else
               ExtendedProperties->append("Chapter_" + ChapterNum + ":title##" + QFileInfo(sFileName).baseName());
            NbrChapters++;
            ChapterNum = QString("%1").arg(NbrChapters);
            while (ChapterNum.length() < 3)
               ChapterNum = "0" + ChapterNum;
         }

         ExtendedProperties->append("Chapter_" + ChapterNum + ":Start" + QString("##") + QTime(0, 0, 0, 0).addMSecs(Start).toString("hh:mm:ss.zzz"));
         ExtendedProperties->append("Chapter_" + ChapterNum + ":End" + QString("##") + QTime(0, 0, 0, 0).addMSecs(End).toString("hh:mm:ss.zzz"));
         ExtendedProperties->append("Chapter_" + ChapterNum + ":Duration" + QString("##") + QTime(0, 0, 0, 0).addMSecs(End - Start).toString("hh:mm:ss.zzz"));
         // Chapter metadata
         while ((tag = av_dict_get(ch->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
            ExtendedProperties->append("Chapter_" + ChapterNum + ":" + QString().fromUtf8(tag->key).toLower() + QString("##") + QString().fromUtf8(tag->value));

         NbrChapters++;
      }

      //*********************************************************************************************************
      // Get information about duration
      //*********************************************************************************************************
      int64_t duration = (LibavFile->duration / AV_TIME_BASE);
      int hours = int(duration / 3600);
      int min = int(duration / 60 % 60);
      int sec = int(duration % 60);
      int ms = int(LibavFile->duration / (AV_TIME_BASE / 1000) % 1000);
      SetGivenDuration(QTime(hours, min, sec, ms));

      EndTime = GetRealDuration();

      //*********************************************************************************************************
      // Get information from track
      //*********************************************************************************************************
      for (int Track = 0; Track < (int)LibavFile->nb_streams; Track++)
      {

         // Find codec
         const AVCodec *Codec = avcodec_find_decoder(LibavFile->streams[Track]->codecpar->codec_id);
         AVCodecContext *CodecContext = avcodec_alloc_context3(Codec);
         avcodec_parameters_to_context(CodecContext, LibavFile->streams[Track]->codecpar);

         //*********************************************************************************************************
         // Audio track
         //*********************************************************************************************************
         if (CodecContext->codec_type == AVMEDIA_TYPE_AUDIO)
         {
            // Keep this as default track
            if (AudioStreamNumber == -1)
               AudioStreamNumber = Track;

            // Compute TrackNum
            QString TrackNum = QString("%1").arg(AudioTrackNbr, 3, 10, QLatin1Char('0'));
            //while (TrackNum.length() < 3) TrackNum = "0" + TrackNum;
            TrackNum = "Audio_" + TrackNum + ":";

            // General
            ExtendedProperties->append(TrackNum + QString("Track") + QString("##") + QString("%1").arg(Track));
            if (Codec)
               ExtendedProperties->append(TrackNum + QString("Codec") + QString("##") + QString(Codec->name));

            AVDictionary * av_opts = NULL;
            //av_dict_set(&av_opts, "thread_count", QString("%1").arg(getCpuCount()).toLocal8Bit().constData(), 0);

            // Hack to correct wrong frame rates that seem to be generated by some codecs
            //if (VideoCodecContext->time_base.num > 1000 && VideoCodecContext->time_base.den == 1)
            //   VideoCodecContext->time_base.den = 1000;

            if (avcodec_open2(CodecContext, Codec, &av_opts) >= 0)
            {
               // Channels and Sample format
               QString SampleFMT = "";
               switch (CodecContext->sample_fmt)
               {
                  case AV_SAMPLE_FMT_U8: SampleFMT = "-U8";   ExtendedProperties->append(TrackNum + QString("Sample format") + QString("##") + "unsigned 8 bits");          break;
                  case AV_SAMPLE_FMT_S16: SampleFMT = "-S16";  ExtendedProperties->append(TrackNum + QString("Sample format") + QString("##") + "signed 16 bits");           break;
                  case AV_SAMPLE_FMT_S32: SampleFMT = "-S32";  ExtendedProperties->append(TrackNum + QString("Sample format") + QString("##") + "signed 32 bits");           break;
                  case AV_SAMPLE_FMT_FLT: SampleFMT = "-FLT";  ExtendedProperties->append(TrackNum + QString("Sample format") + QString("##") + "float");                    break;
                  case AV_SAMPLE_FMT_DBL: SampleFMT = "-DBL";  ExtendedProperties->append(TrackNum + QString("Sample format") + QString("##") + "double");                   break;
                  case AV_SAMPLE_FMT_U8P: SampleFMT = "-U8P";  ExtendedProperties->append(TrackNum + QString("Sample format") + QString("##") + "unsigned 8 bits, planar");  break;
                  case AV_SAMPLE_FMT_S16P: SampleFMT = "-S16P"; ExtendedProperties->append(TrackNum + QString("Sample format") + QString("##") + "signed 16 bits, planar");   break;
                  case AV_SAMPLE_FMT_S32P: SampleFMT = "-S32P"; ExtendedProperties->append(TrackNum + QString("Sample format") + QString("##") + "signed 32 bits, planar");   break;
                  case AV_SAMPLE_FMT_FLTP: SampleFMT = "-FLTP"; ExtendedProperties->append(TrackNum + QString("Sample format") + QString("##") + "float, planar");            break;
                  case AV_SAMPLE_FMT_DBLP: SampleFMT = "-DBLP"; ExtendedProperties->append(TrackNum + QString("Sample format") + QString("##") + "double, planar");           break;
                  default: SampleFMT = "-?";    ExtendedProperties->append(TrackNum + QString("Sample format") + QString("##") + "Unknown");                  break;
               }
               if (CodecContext->CC_CHANNELS == 1)
                  ExtendedProperties->append(TrackNum + QString("Channels") + QString("##") + QApplication::translate("cBaseMediaFile", "Mono", "Audio channels mode") + SampleFMT);
               else if (CodecContext->CC_CHANNELS == 2)
                  ExtendedProperties->append(TrackNum + QString("Channels") + QString("##") + QApplication::translate("cBaseMediaFile", "Stereo", "Audio channels mode") + SampleFMT);
               else
                  ExtendedProperties->append(TrackNum + QString("Channels") + QString("##") + QString("%1").arg(CodecContext->CC_CHANNELS) + SampleFMT);

               // Frequency
               if (int(CodecContext->sample_rate / 1000) * 1000 > 0)
               {
                  if (int(CodecContext->sample_rate / 1000) * 1000 == CodecContext->sample_rate)
                     ExtendedProperties->append(TrackNum + QString("Frequency") + QString("##") + QString("%1").arg(int(CodecContext->sample_rate / 1000)) + "Khz");
                  else
                     ExtendedProperties->append(TrackNum + QString("Frequency") + QString("##") + QString("%1").arg(double(CodecContext->sample_rate) / 1000, 8, 'f', 1).trimmed() + "Khz");
               }

               // Bitrate
               if (int(CodecContext->bit_rate / 1000) > 0)
                  ExtendedProperties->append(TrackNum + QString("Bitrate") + QString("##") + QString("%1").arg(int(CodecContext->bit_rate / 1000)) + "Kb/s");

               // Stream metadata
               while ((tag = av_dict_get(LibavFile->streams[Track]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
               {
                  // OGV container affect TAG to audio stream !
                  QString Key = QString().fromUtf8(tag->key).toLower();
                  if ((sFileName.toLower().endsWith(".ogv")) && ((Key == "title") || (Key == "artist") || (Key == "album") || (Key == "comment") || (Key == "date") || (Key == "composer") || (Key == "encoder")))
                     ExtendedProperties->append(Key + QString("##") + QString().fromUtf8(tag->value));
                  else
                     ExtendedProperties->append(TrackNum + Key + QString("##") + QString().fromUtf8(tag->value));
               }

               // Ensure language exist (Note : AVI and FLV container own language at container level instead of track level)
               if (GetInformationValue(TrackNum + "language", ExtendedProperties) == "")
               {
                  QString Lng = GetInformationValue("language", ExtendedProperties);
                  ExtendedProperties->append(TrackNum + QString("language##") + (Lng == "" ? "und" : Lng));
               }
            }
            // Next
            AudioTrackNbr++;
         }
         //*********************************************************************************************************
         // Video track
         //*********************************************************************************************************
         else if (!MusicOnly && (CodecContext->codec_type == AVMEDIA_TYPE_VIDEO))
         {
            // Compute TrackNum
            QString TrackNum = QString("%1").arg(VideoTrackNbr);
            while (TrackNum.length() < 3) TrackNum = "0" + TrackNum;
            TrackNum = "Video_" + TrackNum + ":";

            // General
            ExtendedProperties->append(TrackNum + QString("Track") + QString("##") + QString("%1").arg(Track));
            if (Codec)
               ExtendedProperties->append(TrackNum + QString("Codec") + QString("##") + QString(Codec->name));

            // Bitrate
            if (LibavFile->streams[Track]->CODEC_OR_PAR->bit_rate > 0)
               ExtendedProperties->append(TrackNum + QString("Bitrate") + QString("##") + QString("%1").arg(int(LibavFile->streams[Track]->CODEC_OR_PAR->bit_rate / 1000)) + "Kb/s");

            // Frame rate
            if (int(double(LibavFile->streams[Track]->avg_frame_rate.num) / double(LibavFile->streams[Track]->avg_frame_rate.den)) > 0)
            {
               if (int(double(LibavFile->streams[Track]->avg_frame_rate.num) / double(LibavFile->streams[Track]->avg_frame_rate.den)) == double(LibavFile->streams[Track]->avg_frame_rate.num) / double(LibavFile->streams[Track]->avg_frame_rate.den))
                  ExtendedProperties->append(TrackNum + QString("Frame rate") + QString("##") + QString("%1").arg(int(double(LibavFile->streams[Track]->avg_frame_rate.num) / double(LibavFile->streams[Track]->avg_frame_rate.den))) + " FPS");
               else
                  ExtendedProperties->append(TrackNum + QString("Frame rate") + QString("##") + QString("%1").arg(double(double(LibavFile->streams[Track]->avg_frame_rate.num) / double(LibavFile->streams[Track]->avg_frame_rate.den)), 8, 'f', 3).trimmed() + " FPS");
            }

            // Stream metadata
            while ((tag = av_dict_get(LibavFile->streams[Track]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
               ExtendedProperties->append(TrackNum + QString(tag->key) + QString("##") + QString().fromUtf8(tag->value));

            // Ensure language exist (Note : AVI ‘AttachedPictureFrame’and FLV container own language at container level instead of track level)
            if (GetInformationValue(TrackNum + "language", ExtendedProperties) == "")
            {
               QString Lng = GetInformationValue("language", ExtendedProperties);
               ExtendedProperties->append(TrackNum + QString("language##") + (Lng == "" ? "und" : Lng));
            }

            // Keep this as default track
            if (VideoStreamNumber == -1)
            {
               QImage  *Img = NULL;
               AVFrame *FrameBufYUV = NULL;


               // Search if a jukebox mode thumbnail (jpg file with same name as video) exist
               QFileInfo   File(sFileName);
               QString     JPegFile = File.absolutePath() + (File.absolutePath().endsWith(QDir::separator()) ? "" : QString(QDir::separator())) + File.completeBaseName() + ".jpg";
               if (QFileInfo(JPegFile).exists()) Icon->LoadIcons(JPegFile);

               VideoStreamNumber = Track;
               IsMTS = (sFileName.toLower().endsWith(".mts", Qt::CaseInsensitive) || sFileName.toLower().endsWith(".m2ts", Qt::CaseInsensitive) || sFileName.toLower().endsWith(".mod", Qt::CaseInsensitive));
               LibavFile->streams[VideoStreamNumber]->discard = AVDISCARD_DEFAULT;  // Setup STREAM options

               // Setup decoder options
               AVDictionary * av_opts = NULL;
               av_dict_set(&av_opts, "thread_count", QString("%1").arg(getCpuCount()).toLocal8Bit().constData(), 0);

               // Hack to correct wrong frame rates that seem to be generated by some codecs
               //if (VideoCodecContext->time_base.num > 1000 && VideoCodecContext->time_base.den == 1)
               //   VideoCodecContext->time_base.den = 1000;

               if (avcodec_open2(CodecContext, Codec, &av_opts) >= 0)
               {
                  // Get Aspect Ratio

                  aspectRatio = double(CodecContext->sample_aspect_ratio.num) / double(CodecContext->sample_aspect_ratio.den);

                  if (CodecContext->sample_aspect_ratio.num != 0)
                     aspectRatio = double(CodecContext->sample_aspect_ratio.num) / double(CodecContext->sample_aspect_ratio.den);

                  if (aspectRatio == 0)
                     aspectRatio = 1;

                  // Special case for DVD mode video without PAR
                  if ((aspectRatio == 1) && (CodecContext->coded_width == 720) && ((CodecContext->coded_height == 576) || (CodecContext->coded_height == 480)))
                     aspectRatio = double((CodecContext->coded_height / 3) * 4) / 720;

                  // Try to load one image to be sure we can make something with this file
                  // and use this first image as thumbnail (if no jukebox thumbnail)
                  int64_t   Position = 0;
                  double    dEndFile = double(QTime(0, 0, 0, 0).msecsTo(GetRealDuration())) / 1000;    // End File Position in double format
                  if (dEndFile != 0)
                  {
                     // Allocate structure for YUV image

                     FrameBufYUV = ALLOCFRAME();

                     if (FrameBufYUV != NULL)
                     {

                        AVStream    *VideoStream = LibavFile->streams[VideoStreamNumber];
                        AVPacket    *StreamPacket = NULL;
                        bool        Continue = true;
                        bool        IsVideoFind = false;
                        double      FrameTimeBase = av_q2d(VideoStream->time_base);
                        double      FramePosition = 0;

                        while (Continue)
                        {
                           StreamPacket = getNewPacket();//new AVPacket();
                           StreamPacket->flags |= AV_PKT_FLAG_KEY;  // HACK for CorePNG to decode as normal PNG by default
                           if (av_read_frame(LibavFile, StreamPacket) == 0)
                           {
                              if (StreamPacket->stream_index == VideoStreamNumber)
                              {
                                 int packet_err = avcodec_send_packet(CodecContext, StreamPacket);
                                 if (packet_err == 0)
                                 {
                                    AVFrame *Frame = nullptr;
                                    int gotFrame = 1;
                                    while (gotFrame > 0)
                                    {
                                       Frame = ALLOCFRAME();
                                       gotFrame = avcodec_receive_frame(CodecContext, FrameBufYUV);
                                       while (gotFrame == AVERROR(EAGAIN))
                                       {
                                          av_read_frame(LibavFile, StreamPacket);
                                          if (StreamPacket->stream_index == VideoStreamNumber)
                                          {
                                             avcodec_send_packet(CodecContext, StreamPacket);
                                             gotFrame = avcodec_receive_frame(CodecContext, FrameBufYUV);
                                          }
                                       }
                                       if (gotFrame == 0)
                                       {
                                          int64_t pts = AV_NOPTS_VALUE;
                                          if ((FrameBufYUV->pts == (int64_t)AV_NOPTS_VALUE) && (FrameBufYUV->pts != (int64_t)AV_NOPTS_VALUE))
                                             pts = FrameBufYUV->pts;
                                          else
                                             pts = FrameBufYUV->pts;
                                          if (pts == (int64_t)AV_NOPTS_VALUE)
                                             pts = 0;
                                          else
                                             pts = pts - av_rescale_q(LibavFile->start_time, AV_TIME_BASE_Q, /*LibavFile->streams[VideoStreamNumber]*/CodecContext->time_base);
                                          FramePosition = double(pts)*FrameTimeBase;
                                          Img = ConvertYUVToRGB(false, FrameBufYUV);      // Create Img from YUV Buffer
                                          IsVideoFind = (Img != NULL) && (!Img->isNull());
                                          geometry = IMAGE_GEOMETRY_UNKNOWN;
                                       }
                                       if (Frame != NULL)
                                          FREEFRAME(&Frame);
                                    }
                                 }
                                 //int FrameDecoded = 0;
                                 //if (avcodec_decode_video2(CodecContext, FrameBufYUV, &FrameDecoded, StreamPacket) < 0)
                                 //   ToLog(LOGMSG_INFORMATION, "IN:cVideoFile::OpenCodecAndFile : avcodec_decode_video2 return an error");
                                 //if (FrameDecoded > 0)
                                 //{
                                 //   int64_t pts = AV_NOPTS_VALUE;
                                 //   if ((FrameBufYUV->pts == (int64_t)AV_NOPTS_VALUE) && (FrameBufYUV->pts != (int64_t)AV_NOPTS_VALUE))
                                 //      pts = FrameBufYUV->pts;
                                 //   else
                                 //      pts = FrameBufYUV->pts;
                                 //   if (pts == (int64_t)AV_NOPTS_VALUE)
                                 //      pts = 0;
                                 //   else
                                 //      pts = pts - av_rescale_q(LibavFile->start_time, AV_TIME_BASE_Q, /*LibavFile->streams[VideoStreamNumber]*/CodecContext->time_base);
                                 //   FramePosition = double(pts)*FrameTimeBase;
                                 //   Img = ConvertYUVToRGB(false, FrameBufYUV);      // Create Img from YUV Buffer
                                 //   IsVideoFind = (Img != NULL) && (!Img->isNull());
                                 //   geometry = IMAGE_GEOMETRY_UNKNOWN;
                                 //}
                              }
                              // Check if we need to continue loop
                              Continue = (IsVideoFind == false) && (FramePosition < dEndFile);
                           }
                           else
                           {
                              // if error in av_read_frame(...) then may be we have reach the end of file !
                              Continue = false;
                           }
                           // Continue with a new one
                           if (StreamPacket != NULL)
                           {
                              deletePacket(&StreamPacket);
                           }
                        }
                        if ((!IsVideoFind) && (!Img))
                        {
                           ToLog(LOGMSG_CRITICAL, QString("No video image return for position %1 => return black frame").arg(Position));
                           Img = new QImage(CodecContext->width, CodecContext->height, QImage::Format_ARGB32_Premultiplied);
                           Img->fill(0);
                        }
                        FREEFRAME(&FrameBufYUV);

                     }
                     else ToLog(LOGMSG_CRITICAL, "Error in cVideoFile::OpenCodecAndFile : Impossible to allocate FrameBufYUV");
                  }
                  else ToLog(LOGMSG_CRITICAL, "Error in cVideoFile::OpenCodecAndFile : dEndFile=0 ?????");
               }
               av_dict_free(&av_opts);
               if (Img)
               {
                  // Get information about size image
                  imageWidth = Img->width();
                  imageHeight = Img->height();
                  // Compute image geometry
                  geometry = geometryFromSize(imageWidth, imageHeight);
                  // Icon
                  if (Icon->Icon16.isNull())
                  {
                     QImage Final = (Video_ThumbWidth == 162 ? ApplicationConfig->VideoMask_162 : Video_ThumbWidth == 150 ? ApplicationConfig->VideoMask_150 : ApplicationConfig->VideoMask_120).copy();
                     QImage ImgF;
                     if (Img->width() > Img->height())
                        ImgF = Img->scaledToWidth(Video_ThumbWidth - 2, Qt::SmoothTransformation);
                     else
                        ImgF = Img->scaledToHeight(Video_ThumbHeight*0.7, Qt::SmoothTransformation);
                     QPainter Painter;
                     Painter.begin(&Final);
                     Painter.drawImage(QRect((Final.width() - ImgF.width()) / 2, (Final.height() - ImgF.height()) / 2, ImgF.width(), ImgF.height()), ImgF);
                     Painter.end();
                     Icon->LoadIcons(&Final);
                  }
                  delete Img;
               }
            }

            // Next
            VideoTrackNbr++;

         }
         avcodec_free_context(&CodecContext);


         //*********************************************************************************************************
         // Thumbnails (since lavf 54.2.0 - avformat.h)
         //*********************************************************************************************************
         if (LibavFile->streams[Track]->disposition & AV_DISPOSITION_ATTACHED_PIC)
         {
            AVStream *ThumbStream = LibavFile->streams[Track];
            AVPacket pkt = ThumbStream->attached_pic;
            AVFrame  *FrameYUV = ALLOCFRAME();
            if (FrameYUV)
            {

               const AVCodec *ThumbDecoderCodec = avcodec_find_decoder(ThumbStream->codecpar->codec_id);
               AVCodecContext *ThumbDecoderContext = avcodec_alloc_context3(ThumbDecoderCodec);
               avcodec_parameters_to_context(ThumbDecoderContext, ThumbStream->codecpar);

               // Setup decoder options
               AVDictionary * av_opts = NULL;
               av_dict_set(&av_opts, "thread_count", QString("%1").arg(1).toLocal8Bit().constData(), 0);
               if (avcodec_open2(ThumbDecoderContext, ThumbDecoderCodec, &av_opts) >= 0)
               {
                  AVFrame *FrameRGB = ALLOCFRAME();
                  int gotFrame;
                  int packet_err = avcodec_send_packet(ThumbDecoderContext, &pkt);
                  if (packet_err == 0)
                  {
                     gotFrame = avcodec_receive_frame(ThumbDecoderContext, FrameYUV);
                     if (gotFrame == 0)
                     {
                        int     W = FrameYUV->width, RealW = (W / 8) * 8;    if (RealW < W) RealW += 8;
                        int     H = FrameYUV->height, RealH = (H / 8) * 8;    if (RealH < H) RealH += 8;;
                        QImage  Thumbnail(RealW, RealH, QTPIXFMT);
                        AVFrame *FrameRGB = ALLOCFRAME();
                        av_image_fill_arrays(FrameRGB->data, FrameRGB->linesize, (const uint8_t *)Thumbnail.bits(), PIXFMT, RealW, RealH, 1);
                        struct SwsContext *img_convert_ctx = sws_getContext(FrameYUV->width, FrameYUV->height, (AVPixelFormat)FrameYUV->format, RealW, RealH, PIXFMT, SWS_FAST_BILINEAR, NULL, NULL, NULL);
                        if (img_convert_ctx != NULL)
                        {
                           int ret = sws_scale(img_convert_ctx, FrameYUV->data, FrameYUV->linesize, 0, FrameYUV->height, FrameRGB->data, FrameRGB->linesize);
                           if (ret > 0)
                           {
                              // sws_scaler truncate the width of the images to a multiple of 8. So cut resulting image to comply a multiple of 8
                              Thumbnail = Thumbnail.copy(0, 0, W, H);
                              Icon->LoadIcons(&Thumbnail);
                           }
                           sws_freeContext(img_convert_ctx);
                        }
                     }
                     if (FrameRGB)
                        FREEFRAME(&FrameRGB);
                  }
                  avcodec_free_context(&ThumbDecoderContext);
               }
               av_dict_free(&av_opts);
            }
            if (FrameYUV)
               FREEFRAME(&FrameYUV);
         }
      }

      // if no icon then load default for type
      if (Icon->Icon16.isNull() || Icon->Icon100.isNull())
         Icon->LoadIcons(ObjectType == OBJECTTYPE_VIDEOFILE ? &ApplicationConfig->DefaultVIDEOIcon : &ApplicationConfig->DefaultMUSICIcon);
   }

   // Close the libav file
   if (LibavFile != NULL)
   {
      avformat_close_input(&LibavFile);
      LibavFile = NULL;
   }

   //Mutex.unlock();
   return Continu;
}

//====================================================================================================================

QString cVideoFile::GetFileTypeStr()
{
   if (MusicOnly || (ObjectType == OBJECTTYPE_MUSICFILE)) return QApplication::translate("cBaseMediaFile", "Music", "File type");
   else return QApplication::translate("cBaseMediaFile", "Video", "File type");
}

//====================================================================================================================

QImage *cVideoFile::GetDefaultTypeIcon(cCustomIcon::IconSize Size)
{
   if (MusicOnly || (ObjectType == OBJECTTYPE_MUSICFILE)) return ApplicationConfig->DefaultMUSICIcon.GetIcon(Size);
   else return ApplicationConfig->DefaultVIDEOIcon.GetIcon(Size);
}

bool cVideoFile::LoadAnalyseSound(QList<qreal> *Peak, QList<qreal> *Moyenne)
{
   int64_t RealAudioDuration, RealVideoDuration;
   bool IsOk = ApplicationConfig->FilesTable->GetAnalyseSound(fileKey, Peak, Moyenne, &RealAudioDuration, ObjectType == OBJECTTYPE_VIDEOFILE ? &RealVideoDuration : NULL, &SoundLevel);
   if (IsOk)
   {
      SetRealAudioDuration(QTime(0, 0, 0, 0).addMSecs(RealAudioDuration));
      if (ObjectType == OBJECTTYPE_VIDEOFILE)
         SetRealVideoDuration(QTime(0, 0, 0, 0).addMSecs(RealVideoDuration));
   }
   return IsOk;
}

//====================================================================================================================

void cVideoFile::SaveAnalyseSound(QList<qreal> *Peak, QList<qreal> *Moyenne, qreal MaxMoyenneValue)
{
   int64_t RealVDuration = (ObjectType == OBJECTTYPE_VIDEOFILE) ? QTime(0, 0, 0, 0).msecsTo(GetRealVideoDuration()) : 0;
   SoundLevel = MaxMoyenneValue;
   ApplicationConfig->FilesTable->SetAnalyseSound(fileKey, Peak, Moyenne, QTime(0, 0, 0, 0).msecsTo(GetRealAudioDuration()), (ObjectType == OBJECTTYPE_VIDEOFILE ? &RealVDuration : NULL), SoundLevel);
}

//====================================================================================================================


//====================================================================================================================

QString cVideoFile::GetTechInfo(QStringList *ExtendedProperties)
{
   QString Info = "";
   if (ObjectType == OBJECTTYPE_MUSICFILE)
   {
      Info = GetCumulInfoStr(ExtendedProperties, "Audio", "Codec");
      if (GetCumulInfoStr(ExtendedProperties, "Audio", "Channels") != "")       Info = Info + (Info != "" ? "-" : "") + GetCumulInfoStr(ExtendedProperties, "Audio", "Channels");
      if (GetCumulInfoStr(ExtendedProperties, "Audio", "Bitrate") != "")        Info = Info + (Info != "" ? "-" : "") + GetCumulInfoStr(ExtendedProperties, "Audio", "Bitrate");
      if (GetCumulInfoStr(ExtendedProperties, "Audio", "Frequency") != "")      Info = Info + (Info != "" ? "-" : "") + GetCumulInfoStr(ExtendedProperties, "Audio", "Frequency");
   }
   else
   {
      Info = GetImageSizeStr();
      if (GetCumulInfoStr(ExtendedProperties, "Video", "Codec") != "")          Info = Info + (Info != "" ? "-" : "") + GetCumulInfoStr(ExtendedProperties, "Video", "Codec");
      if (GetCumulInfoStr(ExtendedProperties, "Video", "Frame rate") != "")     Info = Info + (Info != "" ? "-" : "") + GetCumulInfoStr(ExtendedProperties, "Video", "Frame rate");
      if (GetCumulInfoStr(ExtendedProperties, "Video", "Bitrate") != "")        Info = Info + (Info != "" ? "-" : "") + GetCumulInfoStr(ExtendedProperties, "Video", "Bitrate");

      int     Num = 0;
      QString TrackNum = "";
      QString Value = "";
      QString SubInfo = "";
      do
      {
         TrackNum = QString("%1").arg(Num);
         while (TrackNum.length() < 3) TrackNum = "0" + TrackNum;
         TrackNum = "Audio_" + TrackNum + ":";
         Value = GetInformationValue(TrackNum + "language", ExtendedProperties);
         if (Value != "")
         {
            if (Num == 0) Info = Info + "-"; else Info = Info + "/";
            SubInfo = GetInformationValue(TrackNum + "Codec", ExtendedProperties);
            if (GetInformationValue(TrackNum + "Channels", ExtendedProperties) != "")  SubInfo = SubInfo + (Info != "" ? "-" : "") + GetInformationValue(TrackNum + "Channels", ExtendedProperties);
            if (GetInformationValue(TrackNum + "Bitrate", ExtendedProperties) != "")   SubInfo = SubInfo + (Info != "" ? "-" : "") + GetInformationValue(TrackNum + "Bitrate", ExtendedProperties);
            if (GetInformationValue(TrackNum + "Frequency", ExtendedProperties) != "") SubInfo = SubInfo + (Info != "" ? "-" : "") + GetInformationValue(TrackNum + "Frequency", ExtendedProperties);
            Info = Info + Value + "(" + SubInfo + ")";
         }
         // Next
         Num++;
      } while (Value != "");
   }
   return Info;
}

//====================================================================================================================

QString cVideoFile::GetTAGInfo(QStringList *ExtendedProperties)
{
   QString Info = GetInformationValue("track", ExtendedProperties);
   if (GetInformationValue("title", ExtendedProperties) != "")          Info = Info + (Info != "" ? "-" : "") + GetInformationValue("title", ExtendedProperties);
   if (GetInformationValue("artist", ExtendedProperties) != "")         Info = Info + (Info != "" ? "-" : "") + GetInformationValue("artist", ExtendedProperties);
   if (GetInformationValue("album", ExtendedProperties) != "")          Info = Info + (Info != "" ? "-" : "") + GetInformationValue("album", ExtendedProperties);
   if (GetInformationValue("date", ExtendedProperties) != "")           Info = Info + (Info != "" ? "-" : "") + GetInformationValue("date", ExtendedProperties);
   if (GetInformationValue("genre", ExtendedProperties) != "")          Info = Info + (Info != "" ? "-" : "") + GetInformationValue("genre", ExtendedProperties);
   return Info;
}

//====================================================================================================================
// Close LibAVFormat and LibAVCodec contexte for the file
//====================================================================================================================

void cVideoFile::CloseCodecAndFile()
{
   QMutexLocker locker(&accessMutex);
   //Mutex.lock();

   #ifdef USE_YUVCACHE_MAP
   foreach(int64_t key, YUVCache.keys())
      delete YUVCache.value(key);
   YUVCache.clear();
   #else
   while (CacheImage.count() > 0)
      delete(CacheImage.takeLast());
   #endif

   // Close the resampling context
   CloseResampler();

   // Close the filter context
   if (VideoFilterGraph)
      VideoFilter_Close();

   // Close the video codec
   if (VideoCodecContext != NULL)
   {
      avcodec_free_context(&VideoCodecContext);
      //avcodec_close(LibavVideoFile->streams[VideoStreamNumber]->codec);
      //VideoDecoderCodec=NULL;
   }

   // Close the audio codec
   if (VideoCodecContext != NULL)
   {
      avcodec_free_context(&VideoCodecContext);
      //avcodec_close(LibavAudioFile->streams[AudioStreamNumber]->codec);
      //AudioDecoderCodec=NULL;
   }

   // Close the libav files
   if (LibavAudioFile != NULL)
   {
      avformat_close_input(&LibavAudioFile);
      LibavAudioFile = NULL;
   }
   if (LibavVideoFile != NULL)
   {
      avformat_close_input(&LibavVideoFile);
      LibavVideoFile = NULL;
   }

   if (FrameBufferYUV != NULL)
   {
      FREEFRAME(&FrameBufferYUV);
   }
   FrameBufferYUVReady = false;

   IsOpen = false;
   //Mutex.unlock();
   if (FrameBufferRGB)
      FREEFRAME(&FrameBufferRGB);
   if (img_convert_ctx)
      sws_freeContext(img_convert_ctx);
   img_convert_ctx = 0;
}

//*********************************************************************************************************************

void cVideoFile::CloseResampler()
{
   if (RSC)
   {
      swr_free(&RSC);
      RSC = NULL;
   }
}

//*********************************************************************************************************************

void cVideoFile::CheckResampler(int RSC_InChannels, int RSC_OutChannels, AVSampleFormat RSC_InSampleFmt, AVSampleFormat RSC_OutSampleFmt, int RSC_InSampleRate, int RSC_OutSampleRate
   , AVChannelLayout& RSC_InChannelLayout, AVChannelLayout& RSC_OutChannelLayout
   /*, uint64_t RSC_InChannelLayout, uint64_t RSC_OutChannelLayout*/)
{
   if (!av_channel_layout_check(&RSC_InChannelLayout) /* RSC_InChannelLayout == 0*/)
      av_channel_layout_default(&RSC_InChannelLayout, RSC_InChannels);// RSC_InChannelLayout = av_get_default_channel_layout(RSC_InChannels);
   if (!av_channel_layout_check(&RSC_OutChannelLayout) /* RSC_OutChannelLayout == 0*/)
      av_channel_layout_default(&RSC_OutChannelLayout, RSC_OutChannels); //RSC_OutChannelLayout = av_get_default_channel_layout(RSC_OutChannels);
   if ((RSC != NULL) &&
      ((RSC_InChannels != this->RSC_InChannels) || (RSC_OutChannels != this->RSC_OutChannels)
      || (RSC_InSampleFmt != this->RSC_InSampleFmt) || (RSC_OutSampleFmt != this->RSC_OutSampleFmt)
      || (RSC_InSampleRate != this->RSC_InSampleRate) || (RSC_OutSampleRate != this->RSC_OutSampleRate)
      || av_channel_layout_compare(&RSC_InChannelLayout, &this->RSC_InChannelLayout) || av_channel_layout_compare(&RSC_OutChannelLayout, &this->RSC_OutChannelLayout)
      )) 
      CloseResampler();
   if (!RSC)
   {
      this->RSC_InChannels = RSC_InChannels;
      this->RSC_OutChannels = RSC_OutChannels;
      this->RSC_InSampleFmt = RSC_InSampleFmt;
      this->RSC_OutSampleFmt = RSC_OutSampleFmt;
      this->RSC_InSampleRate = RSC_InSampleRate;
      this->RSC_OutSampleRate = RSC_OutSampleRate;

      this->RSC_InChannelLayout = RSC_InChannelLayout;
      this->RSC_OutChannelLayout = RSC_OutChannelLayout;
      /*RSC=swr_alloc_set_opts(NULL,RSC_OutChannelLayout,RSC_OutSampleFmt,RSC_OutSampleRate,
                                  RSC_InChannelLayout, RSC_InSampleFmt, RSC_InSampleRate,
                                  0, NULL);*/
      RSC = swr_alloc();
      av_opt_set_chlayout(RSC, "in_chlayout", &RSC_InChannelLayout, 0);
      av_opt_set_int(RSC, "in_sample_rate", RSC_InSampleRate, 0);
      av_opt_set_chlayout(RSC, "out_chlayout", &RSC_OutChannelLayout, 0);
      av_opt_set_int(RSC, "out_sample_rate", RSC_OutSampleRate, 0);
      av_opt_set_int(RSC, "in_channel_count", RSC_InChannels, 0);
      av_opt_set_int(RSC, "out_channel_count", RSC_OutChannels, 0);
      av_opt_set_sample_fmt(RSC, "in_sample_fmt", RSC_InSampleFmt, 0);
      av_opt_set_sample_fmt(RSC, "out_sample_fmt", RSC_OutSampleFmt, 0);
      if ((RSC) && (swr_init(RSC) < 0))
      {
         ToLog(LOGMSG_CRITICAL, QString("CheckResampler: swr_init failed"));
         swr_free(&RSC);
         RSC = NULL;
      }
      if (!RSC) ToLog(LOGMSG_CRITICAL, QString("CheckResampler: swr_alloc_set_opts failed"));
   }
}

//*********************************************************************************************************************
// VIDEO FILTER PART : This code was adapt from xbmc sources files
//*********************************************************************************************************************

int cVideoFile::VideoFilter_Open()
{
   int result;

   if (VideoFilterGraph)
      VideoFilter_Close();

   if (!(VideoFilterGraph = avfilter_graph_alloc()))
   {
      ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : unable to alloc filter graph"));
      return -1;
   }

   VideoFilterGraph->scale_sws_opts = av_strdup("flags=4");

   char args[512];
   snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
      VideoCodecContext->width, VideoCodecContext->height,
      VideoCodecContext->pix_fmt,
      VideoCodecContext->time_base.num, VideoCodecContext->time_base.den,
      VideoCodecContext->sample_aspect_ratio.num, VideoCodecContext->sample_aspect_ratio.den
   );

   const AVFilter *srcFilter = avfilter_get_by_name("buffer");
   const AVFilter *outFilter = avfilter_get_by_name("buffersink");

   if ((result = avfilter_graph_create_filter(&VideoFilterIn, srcFilter, "in", args, NULL, VideoFilterGraph)) < 0)
   {
      char errbuff[2048];
      av_strerror(result, errbuff,2048);

      ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_create_filter: src"));
      return result;
   }
   if ((result = avfilter_graph_create_filter(&VideoFilterOut, outFilter, "out", NULL, NULL, VideoFilterGraph)) < 0)
   {
      ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_create_filter: out"));
      return result;
   }
   AVFilterInOut *outputs = avfilter_inout_alloc();
   AVFilterInOut *inputs = avfilter_inout_alloc();

   outputs->name = av_strdup("in");
   outputs->filter_ctx = VideoFilterIn;
   outputs->pad_idx = 0;
   outputs->next = NULL;

   inputs->name = av_strdup("out");
   inputs->filter_ctx = VideoFilterOut;
   inputs->pad_idx = 0;
   inputs->next = NULL;

   if ((result = avfilter_graph_parse_ptr(VideoFilterGraph, QString("yadif=deint=interlaced:mode=send_frame:parity=auto").toLocal8Bit().constData(), &inputs, &outputs, NULL)) < 0)
   {
      ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_parse"));
      return result;
   }

   if ((result = avfilter_graph_config(VideoFilterGraph, NULL)) < 0)
   {
      ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_config"));
      return result;
   }
   return result;
}

//====================================================================================================================

void cVideoFile::VideoFilter_Close()
{
   if (VideoFilterGraph)
      avfilter_graph_free(&VideoFilterGraph);
   VideoFilterGraph = NULL;
   VideoFilterIn = NULL;
   VideoFilterOut = NULL;
}

int cVideoFile::cudaFilter_Open()
{
   int result;

   if (VideoFilterGraph)
      VideoFilter_Close();

   if (!(VideoFilterGraph = avfilter_graph_alloc()))
   {
      ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : unable to alloc filter graph"));
      return -1;
   }
   avfilter_graph_set_auto_convert(VideoFilterGraph, AVFILTER_AUTO_CONVERT_NONE);

   //??VideoFilterGraph->scale_sws_opts = av_strdup("flags=4");

   enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_CUDA, AV_PIX_FMT_NV12, AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
   char args[512];
   snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
      VideoCodecContext->width, VideoCodecContext->height,
      /*VideoCodecContext->pix_fmt*/ AV_PIX_FMT_CUDA,
      VideoCodecContext->framerate.num, VideoCodecContext->framerate.den,
      VideoCodecContext->sample_aspect_ratio.num, VideoCodecContext->sample_aspect_ratio.den
   );
   const AVFilter *srcFilter = avfilter_get_by_name("buffer");
   const AVFilter *outFilter = avfilter_get_by_name("buffersink");

   if ((result = avfilter_graph_create_filter(&VideoFilterIn, srcFilter, "in", args, NULL, VideoFilterGraph)) < 0)
   {
      char errbuff[2048];
      av_strerror(result, errbuff, 2048);

      ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_create_filter: src"));
      return result;
   }
   
   result = av_opt_set_int_list(VideoFilterIn, "pix_fmts", pix_fmts,AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
   if (result < 0) {
      //av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
      ToLog(LOGMSG_CRITICAL, QString("Cannot set output pixel format"));
   }
#if (LIBAVFILTER_VERSION_MAJOR<9)
   AVBufferSinkParams *params = av_buffersink_params_alloc();
   params->pixel_fmts = pix_fmts;
#else
   void* params = 0;
#endif
   if ((result = avfilter_graph_create_filter(&VideoFilterOut, outFilter, "out", NULL, params, VideoFilterGraph)) < 0)
   {
      ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_create_filter: out"));
      return result;
   }
   result = av_opt_set_int_list(VideoFilterOut, "pix_fmts", pix_fmts,AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
   if (result < 0) {
      //av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
      ToLog(LOGMSG_CRITICAL, QString("Cannot set output pixel format"));
   }
   AVFilterInOut *outputs = avfilter_inout_alloc();
   AVFilterInOut *inputs = avfilter_inout_alloc();

   outputs->name = av_strdup("in");
   outputs->filter_ctx = VideoFilterIn;
   outputs->pad_idx = 0;
   outputs->next = NULL;

   inputs->name = av_strdup("out");
   inputs->filter_ctx = VideoFilterOut;
   inputs->pad_idx = 0;
   inputs->next = NULL;

   if ((result = avfilter_graph_parse_ptr(VideoFilterGraph, QString("scale_npp=w=%1:h=%2:format=%3").arg(VideoCodecContext->width).arg(VideoCodecContext->height).arg(av_get_pix_fmt_name(AV_PIX_FMT_YUV420P)).toLocal8Bit().constData(), &inputs, &outputs, NULL)) < 0)
   {
      ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_parse"));
      return result;
   }

   if ((result = avfilter_graph_config(VideoFilterGraph, NULL)) < 0)
   {
      ToLog(LOGMSG_CRITICAL, QString("Error in cVideoFile::VideoFilter_Open : avfilter_graph_config"));
      return result;
   }
   return result;
}

//====================================================================================================================

void cVideoFile::cudaFilter_Close()
{
   if (VideoFilterGraph)
      avfilter_graph_free(&VideoFilterGraph);
   VideoFilterGraph = NULL;
   VideoFilterIn = NULL;
   VideoFilterOut = NULL;
}

//====================================================================================================================

bool cVideoFile::SeekFile(AVStream *VideoStream, AVStream *AudioStream, int64_t Position)
{
   #ifdef SEEK_DEBUG
   qDebug() << "SeekFile " << (VideoStream != 0 ? "Video " : "Audio ") << " to Position " << Position;
   #endif
   bool ret = true;
   AVFormatContext *LibavFile = NULL;
   int StreamNumber = 0;

   // Reset context variables and buffers
   if (AudioStream)
   {
      CloseResampler();
      LibavFile = LibavAudioFile;
      StreamNumber = AudioStreamNumber;
   }
   else if (VideoStream)
   {
      if (VideoFilterGraph)
         VideoFilter_Close();
      #ifdef USE_YUVCACHE_MAP
      foreach(int64_t key, YUVCache.keys())
         delete YUVCache.value(key);
      YUVCache.clear();
      #else
      while (!CacheImage.isEmpty())
         delete(CacheImage.takeLast());
      #endif
      FrameBufferYUVReady = false;
      FrameBufferYUVPosition = 0;
      LibavFile = LibavVideoFile;
      StreamNumber = VideoStreamNumber;
   }

   if (Position < 0)
      Position = 0;

   // Flush LibAV buffers
   if (AudioCodecContext != nullptr)
      avcodec_flush_buffers(AudioCodecContext);
   if (VideoCodecContext != nullptr)
      avcodec_flush_buffers(VideoCodecContext);
   //for (unsigned int i = 0; i < LibavFile->nb_streams; i++)
   //{
   //   AVCodecContext *codec_context = LibavFile->streams[i]->codec;
   //   if (codec_context && codec_context->codec) 
   //      avcodec_flush_buffers(codec_context);
   //}
   int64_t seek_target = av_rescale_q(Position, AV_TIME_BASE_Q, LibavFile->streams[StreamNumber]->time_base);
   #ifdef SEEK_DEBUG
   qDebug() << "seek position " << Position << " seek Target " << seek_target;
   #endif
   //seek_target -= LibavStartTime;
   if (seek_target < 0)
      seek_target = 0;
   int errcode = 0;
   if (SeekErrorCount > 0 || ((errcode = avformat_seek_file(LibavFile, StreamNumber, INT64_MIN, seek_target, INT64_MAX, AVSEEK_FLAG_BACKWARD)) < 0))
   {
      if (SeekErrorCount == 0)
         ToLog(LOGMSG_DEBUGTRACE, GetAvErrorMessage(errcode));
      // Try in AVSEEK_FLAG_ANY mode
      if ((errcode = av_seek_frame(LibavFile, StreamNumber, seek_target, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY)) < 0)
      {
         ToLog(LOGMSG_DEBUGTRACE, GetAvErrorMessage(errcode));
         // Try with default stream if exist
         int DefaultStream = av_find_default_stream_index(LibavFile);
         if ((DefaultStream == StreamNumber) || (Position > 0) || (DefaultStream < 0) || ((errcode = av_seek_frame(LibavFile, DefaultStream, 0, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_BYTE) < 0)))
         {
            ToLog(LOGMSG_DEBUGTRACE, GetAvErrorMessage(errcode));
            ToLog(LOGMSG_CRITICAL, "Error in cVideoFile::ReadFrame : Seek error");
            ret = false;
         }
      }
   }
   // read first packet to ensure we have correct position !
   // elsewhere, redo seek 5 times until exit with error
   if (AudioStream)
   {
      AVPacket* StreamPacket = getNewPacket();// new AVPacket();
      StreamPacket->flags |= AV_PKT_FLAG_KEY;
      int read_error, read_error_count = 0;
      while ((read_error = av_read_frame(LibavFile, StreamPacket)) != 0 && read_error_count++ < 10);
      int64_t FramePts = StreamPacket->pts != (int64_t)AV_NOPTS_VALUE ? StreamPacket->pts : -1;
      //if ((FramePts < (Position/1000)-500) || (FramePts > (Position/1000)+500)) 
      if (FramePts < (seek_target - 500 * 1000) || FramePts >(seek_target + 500 * 1000))
      {
         SeekErrorCount++;
         if (SeekErrorCount < 5)
            ret = SeekFile(VideoStream, AudioStream, Position);
      }
      deletePacket(&StreamPacket); 

   }
   return ret;
}

//====================================================================================================================

u_int8_t *cVideoFile::Resample(AVFrame *Frame, int64_t *SizeDecoded, int DstSampleSize)
{
   u_int8_t *Data = NULL;
   Data = (u_int8_t *)av_malloc(MaxAudioLenDecoded);
   memset(Data, 0, MaxAudioLenDecoded);
   u_int8_t *out[] = { Data };
   if (Data) *SizeDecoded = swr_convert(RSC, out, MaxAudioLenDecoded / DstSampleSize, (const u_int8_t **)Frame->data, Frame->nb_samples)*DstSampleSize;
   //qDebug() << "resample gives " << *SizeDecoded << " bytes";
   return Data;
}

//====================================================================================================================
// return duration of one frame
//====================================================================================================================

qreal cVideoFile::GetFPSDuration()
{
   qreal FPSDuration;
   if ((VideoStreamNumber >= 0) && (LibavVideoFile->streams[VideoStreamNumber]))
      FPSDuration = qreal(LibavVideoFile->streams[VideoStreamNumber]->r_frame_rate.den*(AV_TIME_BASE / 1000)) / qreal(LibavVideoFile->streams[VideoStreamNumber]->r_frame_rate.num);
   else
      FPSDuration = 1;
   return FPSDuration;
}

//====================================================================================================================
// Read a frame from current stream
//====================================================================================================================
// maximum diff between asked image position and founded image position
#define ALLOWEDDELTA    250000
// diff between asked image position and current image position before exit loop and return black frame
#define MAXDELTA        2500000

// Remark: Position must use AV_TIMEBASE Unit
QImage *cVideoFile::ReadFrame(bool PreviewMode, int64_t Position, bool DontUseEndPos, bool Deinterlace, cSoundBlockList *SoundTrackBloc, double Volume, bool ForceSoundOnly, int NbrDuration)
{
   //qDebug() << "cVideoFile::ReadFrame PreviewMode " << PreviewMode << " Position " << Position << " DontUseEndPos " << DontUseEndPos << " Deinterlace " << Deinterlace << " Volume " << Volume << " NbrDuration " <<NbrDuration;
   //qDebug() << "ReadFrame from " << FileName() << " for Position " << Position;
   // Ensure file was previously open
   if (!IsOpen && !OpenCodecAndFile())
      return NULL;

#ifdef CUDA_MEMCHECK
   size_t totalMem, freeMem;
   cudaMemGetInfo(&freeMem, &totalMem);
   qDebug() << QString("%1 bytes mem free of %2 bytes mem total").arg(freeMem).arg(totalMem);
#endif
   //AUTOTIMER(at,"cVideoFile::ReadFrame");
      //autoTimer at("cVideoFile::ReadFrame");

      //Position += LibavStartTime;//AV_TIME_BASE;
      //LONGLONG pc = curPCounter();
      //LONGLONG pcpart = pc;
      // Ensure file have an end file Position
   double dEndFile = double(QTime(0, 0, 0, 0).msecsTo(DontUseEndPos ? GetRealDuration() : EndTime)) / 1000.0;
   if (Position < 0)
      Position = 0;
   Position += LibavStartTime;//AV_TIME_BASE;
   dEndFile += LibavStartTime;
   if (dEndFile == 0)
   {
      ToLog(LOGMSG_CRITICAL, "Error in cVideoFile::ReadFrame : dEndFile=0 ?????");
      return NULL;
   }

   AVStream *VideoStream = (/*(!MusicOnly)&&(!ForceSoundOnly)&&*/VideoStreamNumber >= 0 ? LibavVideoFile->streams[VideoStreamNumber] : NULL);

   cVideoFile::sAudioContext AudioContext;
   AudioContext.SoundTrackBloc = SoundTrackBloc;
   AudioContext.AudioStream = (AudioStreamNumber >= 0 && SoundTrackBloc) ? LibavAudioFile->streams[AudioStreamNumber] : NULL;
   AudioContext.CodecContext = AudioCodecContext;
   AudioContext.FPSSize = SoundTrackBloc ? SoundTrackBloc->SoundPacketSize*SoundTrackBloc->NbrPacketForFPS : 0;
   AudioContext.FPSDuration = AudioContext.FPSSize ? (double(AudioContext.FPSSize) / (SoundTrackBloc->Channels*SoundTrackBloc->SampleBytes*SoundTrackBloc->SamplingRate))*AV_TIME_BASE : 0;
   AudioContext.TimeBase = AudioContext.AudioStream ? double(AudioContext.AudioStream->time_base.den) / double(AudioContext.AudioStream->time_base.num) : 0;
   AudioContext.DstSampleSize = SoundTrackBloc ? (SoundTrackBloc->SampleBytes*SoundTrackBloc->Channels) : 0;
   AudioContext.NeedResampling = false;
   AudioContext.AudioLenDecoded = 0;
   AudioContext.Counter = 20; // Retry counter (when len>0 and avcodec_decode_audio4 fail to retreave frame, we retry counter time before to discard the packet)
   AudioContext.Volume = Volume;
   AudioContext.dEndFile = &dEndFile;
   AudioContext.NbrDuration = NbrDuration;
   AudioContext.DontUseEndPos = DontUseEndPos;

   if (!AudioContext.FPSDuration)
   {
      if (PreviewMode)
         AudioContext.FPSDuration = double(AV_TIME_BASE) / ((cApplicationConfig *)ApplicationConfig)->PreviewFPS;
      else if (VideoStream)
         AudioContext.FPSDuration = double(VideoStream->r_frame_rate.den * AV_TIME_BASE) / double(VideoStream->r_frame_rate.num);
      else
         AudioContext.FPSDuration = double(AV_TIME_BASE) / double(SoundTrackBloc->SamplingRate);
   }

   if (!AudioContext.AudioStream && !VideoStream)
      return NULL;

   //autoTimer atAV("cVideoFile::ReadFrame all");
   //QMutexLocker locker(&Mutex);
   QMutexLocker locker(&accessMutex);
   //Mutex.lock();

   // If position >= end of file : disable audio (only if IsComputedAudioDuration)
   double dPosition = double(Position) / AV_TIME_BASE;
   //if ((dPosition > 0) && (dPosition >= dEndFile) && (IsComputedAudioDuration)) 
   if (dPosition > 0 && dPosition >= dEndFile + 1000 && IsComputedAudioDuration)
   {
      AudioContext.AudioStream = NULL; // Disable audio
      // Check if last image is ready and correspond to end of file
      if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= dEndFile * AV_TIME_BASE - AudioContext.FPSDuration)
      {
         //Mutex.unlock();
         return new QImage(LastImage.copy());
      }
      // If not then change Position to end file - a FPS to prepare a last image
      Position = dEndFile * AV_TIME_BASE - AudioContext.FPSDuration;
      dPosition = double(Position) / AV_TIME_BASE;
      if (SoundTrackBloc)
         SoundTrackBloc->UseLatestData();
   }

   //================================================
   bool ContinueVideo = true;
   AudioContext.ContinueAudio = (AudioContext.AudioStream) && (SoundTrackBloc);
   bool ResamplingContinue = (Position != 0);
   AudioContext.AudioFramePosition = dPosition;
   //================================================

   if (AudioContext.ContinueAudio)
   {
      AudioContext.NeedResampling = ((AudioContext.CodecContext->sample_fmt != AV_SAMPLE_FMT_S16)
         || (AudioContext.CodecContext->CC_CHANNELS != SoundTrackBloc->Channels)
         || (AudioContext.CodecContext->sample_rate != SoundTrackBloc->SamplingRate));

      //qDebug() << "audio needs resampling " << AudioContext.NeedResampling;
      // Calc if we need to seek to a position
      int64_t Start = /*SoundTrackBloc->LastReadPosition;*/SoundTrackBloc->CurrentPosition;
      int64_t End = Start + SoundTrackBloc->GetDuration();
      int64_t Wanted = AudioContext.FPSDuration * AudioContext.NbrDuration;
      //qDebug() << "Start " << Start << " End " << End << " Position " << Position << " Wanted " << Wanted;
      if ((Position >= Start && Position + Wanted <= End) /*|| Start < 0 */)
         AudioContext.ContinueAudio = false;
      if (AudioContext.ContinueAudio && (Position == 0 || Start < 0 || LastAudioReadPosition < 0 /*|| Position < Start*/ || Position > End + 1500000))
      {
         if (Position < 0)
            Position = 0;
         SoundTrackBloc->ClearList();                // Clear soundtrack list
         ResamplingContinue = false;
         LastAudioReadPosition = 0;
         SeekErrorCount = 0;
         int64_t seekPos = Position - SoundTrackBloc->WantedDuration * 1000.0;
         /*bool bSeekRet = */SeekFile(NULL, AudioContext.AudioStream, seekPos);        // Always seek one FPS before to ensure eventual filter have time to init
         //qDebug() << "seek to " << seekPos << " is " << (bSeekRet?"true":"false");
         AudioContext.AudioFramePosition = Position / AV_TIME_BASE;
      }

      // Prepare resampler
      if (AudioContext.ContinueAudio && AudioContext.NeedResampling)
      {
         if (!ResamplingContinue)
            CloseResampler();
         AVChannelLayout avcl;
         av_channel_layout_default(&avcl, SoundTrackBloc->Channels);
         CheckResampler(AudioContext.CodecContext->CC_CHANNELS, SoundTrackBloc->Channels,
            AVSampleFormat(AudioContext.CodecContext->sample_fmt), SoundTrackBloc->SampleFormat,
            AudioContext.CodecContext->sample_rate, SoundTrackBloc->SamplingRate,
            AudioContext.CodecContext->ch_layout ,
            avcl 
         );
      }
   }

   QImage *RetImage = NULL;
   int64_t RetImagePosition = 0;
   double VideoFramePosition = dPosition;

   // Count number of image > position
   bool IsVideoFind = false;
   #ifdef USE_YUVCACHE_MAP
   if (!YUVCache.isEmpty() /*&& YUVCache.lastKey() >= Position*/)
   {
      QMap<int64_t, cImageInCache *>::const_iterator i = YUVCache.constBegin();
      while (i != YUVCache.constEnd() && !IsVideoFind)
      {
         if (i.key() >= Position && i.key() - Position < ALLOWEDDELTA)
            IsVideoFind = true;
         ++i;
      }
   }
   #else
   for (int CNbr = 0; !IsVideoFind && CNbr < CacheImage.count(); CNbr++)
      if (CacheImage[CNbr]->Position >= Position && CacheImage[CNbr]->Position - Position < ALLOWEDDELTA)
         IsVideoFind = true;
   #endif
   ContinueVideo = (VideoStream && !IsVideoFind && !ForceSoundOnly);
   if (ContinueVideo)
   {
      int64_t DiffTimePosition = -1000000;  // Compute difftime between asked position and previous end decoded position

      if (FrameBufferYUVReady)
      {
         DiffTimePosition = Position - FrameBufferYUVPosition;
         //if ((Position==0)||(DiffTimePosition<0)||(DiffTimePosition>1500000))
         //    ToLog(LOGMSG_INFORMATION,QString("VIDEO-SEEK %1 TO %2").arg(ShortName).arg(Position));
      }

      // Calc if we need to seek to a position
      if (Position == 0 || DiffTimePosition < 0 || DiffTimePosition > 1500000) // Allow 1,5 sec diff (rounded double !)
      {
         if (Position < 0)
            Position = 0;
         SeekErrorCount = 0;
         SeekFile(VideoStream, NULL, Position);        // Always seek one FPS before to ensure eventual filter have time to init
         VideoFramePosition = Position / AV_TIME_BASE;
      }
   }

   //*************************************************************************************************************************************
   // Decoding process : Get StreamPacket until endposition is reach (if sound is wanted) or until image is ok (if image only is wanted)
   //*************************************************************************************************************************************
   //qDebug() << "readFrame prep " << PC2time(curPCounter()-pcpart,true);
   //pcpart = curPCounter();

   // AUDIO PART
   //QFutureWatcher<void> ThreadAudio;
   {
      //AUTOTIMER(atAudio,"cVideoFile::ReadFrame audio Part");
      //autoTimer atAudio("cVideoFile::ReadFrame audio Part");
      //if( !AudioContext.ContinueAudio )
      //   qDebug() << "cVideoFile::ReadFrame audio, AudioContext.ContinueAudio is false!!!!!";

      while (AudioContext.ContinueAudio)
      {
         AVPacket *StreamPacket = getNewPacket();//new AVPacket();
         if (!StreamPacket)
         {
            AudioContext.ContinueAudio = false;
         }
         else
         {
            StreamPacket->flags |= AV_PKT_FLAG_KEY;
            int err;
            if ((err = av_read_frame(LibavAudioFile, StreamPacket)) < 0)
            {
               // If error reading frame then we considere we have reach the end of file
               if (!IsComputedAudioDuration)
               {
                  dEndFile = qreal(SoundTrackBloc->/*LastReadPosition*/CurrentPosition) / AV_TIME_BASE;
                  dEndFile += qreal(SoundTrackBloc->GetDuration()) / 1000;
                  if (dEndFile - LibavStartTime / AV_TIME_BASE == double(QTime(0, 0, 0, 0).msecsTo(EndTime)) / 1000)
                     EndTime = QTime(0, 0, 0).addMSecs((dEndFile - LibavStartTime) * 1000);
                  SetRealAudioDuration(QTime(0, 0, 0, 0).addMSecs(qlonglong((dEndFile - LibavStartTime / AV_TIME_BASE) * 1000)));
               }
               AudioContext.ContinueAudio = false;
               //qDebug() << "cVideoFile::ReadFrame audio, set AudioContext.ContinueAudio to false!!!!! err is "<<err;

               // Use data in TempData to create a latest block
               SoundTrackBloc->UseLatestData();
            }
            else
            {
               DecodeAudio(&AudioContext, StreamPacket, Position);
               StreamPacket = NULL;
            }
         }
         // Continue with a new one
         if (StreamPacket != NULL)
         {
            deletePacket(&StreamPacket);
         }
      }
   }
   //qDebug() << "readFrame audio " << PC2time(curPCounter()-pcpart,true);
   //pcpart = curPCounter();

   // VIDEO PART
   if (VideoStream && !ForceSoundOnly)
   {
      //AUTOTIMER(atAudio,"cVideoFile::ReadFrame video Part");
      //autoTimer atVideo("cVideoFile::ReadFrame video Part");
      //LONGLONG pcpartv = curPCounter();
      if (!ContinueVideo)
      {
         ToLog(LOGMSG_DEBUGTRACE, QString("Video image for position %1 => use image in cache").arg(Position));
      }
      else if (Position < LibavStartTime)
      {
         ToLog(LOGMSG_CRITICAL, QString("Image position %1 is before video stream start => return black frame").arg(Position));
         RetImage = new QImage(VideoCodecContext->width, VideoCodecContext->height, QImage::Format_ARGB32_Premultiplied);
         RetImage->fill(0);
         RetImagePosition = Position;
      }
      else
      {
         #ifdef USE_YUVCACHE_MAP
         bool ByPassFirstImage = (Deinterlace) && (YUVCache.count() == 0);
         #else
         bool ByPassFirstImage = (Deinterlace) && (CacheImage.count() == 0);
         #endif
         int MaxErrorCount = 20;
         bool FreeFrames = false;

         while (ContinueVideo)
         {
            AVPacket *StreamPacket = getNewPacket();//new AVPacket();
            if (!StreamPacket)
            {
               ContinueVideo = false;
            }
            else
            {
               StreamPacket->flags |= AV_PKT_FLAG_KEY;  // HACK for CorePNG to decode as normal PNG by default

               int errcode = 0;
               if ((errcode = av_read_frame(LibavVideoFile, StreamPacket)) < 0)
               {
                  if (errcode == AVERROR_EOF)
                  {
                     // We have reach the end of file
                     if (!IsComputedAudioDuration)
                     {
                        dEndFile = VideoFramePosition;
                        if (dEndFile - LibavStartTime == double(QTime(0, 0, 0, 0).msecsTo(EndTime)) / 1000)
                           EndTime = QTime(0, 0, 0).addMSecs((dEndFile - LibavStartTime)*1000.0);
                        SetRealVideoDuration(QTime(0, 0, 0, 0).addMSecs(qlonglong((dEndFile - LibavStartTime) * 1000)));
                     }
                     ContinueVideo = false;

                     if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= (dEndFile - 1.5)*AV_TIME_BASE)
                     {
                        if (!RetImage)
                        {
                           RetImage = new QImage(LastImage);
                           RetImagePosition = FrameBufferYUVPosition;
                        }
                        IsVideoFind = true;
                        ContinueVideo = false;
                     }
                  }
                  else
                  {
                     ToLog(LOGMSG_CRITICAL, GetAvErrorMessage(errcode));
                     // If error reading frame
                     if (MaxErrorCount > 0)
                     {
                        // Files with stream could provoque this, so we ignore the first MaxErrorCount errors
                        MaxErrorCount--;
                     }
                     else
                     {
                        if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= (dEndFile - 1.5)*AV_TIME_BASE)
                        {
                           if (!RetImage)
                           {
                              RetImage = new QImage(LastImage);
                              RetImagePosition = FrameBufferYUVPosition;
                           }
                           IsVideoFind = true;
                           ContinueVideo = false;
                        }
                        else
                        {
                           SeekErrorCount = 0;
                           ContinueVideo = SeekFile(VideoStream, NULL, Position - 2 * AudioContext.FPSDuration);
                        }
                     }
                  }
               }
               else
               {
                  int64_t FramePts = StreamPacket->pts != (int64_t)AV_NOPTS_VALUE ? StreamPacket->pts : -1;
                  double TimeBase = double(LibavVideoFile->streams[StreamPacket->stream_index]->time_base.den) / double(LibavVideoFile->streams[StreamPacket->stream_index]->time_base.num);
                  if (FramePts >= 0)
                     VideoFramePosition = (double(FramePts) / TimeBase);

                  if (StreamPacket->stream_index == VideoStreamNumber)
                  {
                     // Allocate structures
                     if (FrameBufferYUV == NULL)
                        FrameBufferYUV = ALLOCFRAME();
                     if (FrameBufferYUV)
                     {
                        int FrameDecoded = -1;
                        LastLibAvMessageLevel = 0;    // Clear LastLibAvMessageLevel : some decoder dont return error but display errors messages !
                        int Error = avcodec_send_packet(VideoCodecContext, StreamPacket);
                        //int Error = avcodec_decode_video2(VideoCodecContext,FrameBufferYUV,&FrameDecoded,StreamPacket);
                        if (Error < 0 || LastLibAvMessageLevel == LOGMSG_CRITICAL)
                        {
                           if (MaxErrorCount > 0)
                           {
                              if (VideoFramePosition*1000000.0 < Position)
                              {
                                 ToLog(LOGMSG_INFORMATION, QString("IN:cVideoFile::ReadFrame - Error decoding packet: try left %1").arg(MaxErrorCount));
                              }
                              else
                              {
                                 ToLog(LOGMSG_INFORMATION, QString("IN:cVideoFile::ReadFrame - Error decoding packet: seek to backward and restart reading"));
                                 if (Position > 1000000)
                                 {
                                    SeekErrorCount = 0;
                                    SeekFile(VideoStream, NULL/*AudioStream*/, Position - 1000000); // 1 sec before
                                 }
                                 else
                                 {
                                    SeekErrorCount = 0;
                                    SeekFile(VideoStream, NULL, 0);
                                 }
                              }
                              MaxErrorCount--;
                           }
                           else
                           {
                              ToLog(LOGMSG_CRITICAL, QString("IN:cVideoFile::ReadFrame - Error decoding packet: and no try left"));
                              ContinueVideo = false;
                           }
                        }
                        else
                           FrameDecoded = avcodec_receive_frame(VideoCodecContext, FrameBufferYUV);
                        if (FrameDecoded == 0)
                        {
                           //FrameBufferYUV->pkt_pts = av_frame_get_best_effort_timestamp(FrameBufferYUV);
                           FrameBufferYUV->pts = FrameBufferYUV->pts;
                           // Video filter part
                           if (Deinterlace && !VideoFilterGraph && !isCUVIDDecoder )
                              VideoFilter_Open();
                           else if ((!Deinterlace || isCUVIDDecoder) && VideoFilterGraph)
                              VideoFilter_Close();

                           AVFrame *FiltFrame = NULL;
                           if (VideoFilterGraph)
                           {
                              // FFMPEG 2.0
                              // push the decoded frame into the filtergraph
                              if (av_buffersrc_add_frame_flags(VideoFilterIn, FrameBufferYUV, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                              {
                                 ToLog(LOGMSG_INFORMATION, "IN:cVideoFile::ReadFrame : Error while feeding the filtergraph");
                              }
                              else
                              {
                                 FiltFrame = ALLOCFRAME();
                                 // pull filtered frames from the filtergraph
                                 int ret = av_buffersink_get_frame(VideoFilterOut, FiltFrame);
                                 if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                                 {
                                    ToLog(LOGMSG_INFORMATION, "IN:cVideoFile::ReadFrame : No image return by filter process");
                                    FREEFRAME(&FiltFrame);
                                 }
                              }
                           }
                           if (ByPassFirstImage)
                           {
                              ByPassFirstImage = false;
                              FreeFrames = true;
                           }
                           else
                           {
                              int64_t pts = FrameBufferYUV->pts;
                              if (pts == (int64_t)AV_NOPTS_VALUE)
                              {
                                 if (FrameBufferYUV->pkt_dts != (int64_t)AV_NOPTS_VALUE)
                                 {
                                    pts = FrameBufferYUV->pkt_dts;
                                    ToLog(LOGMSG_DEBUGTRACE, QString("IN:cVideoFile::ReadFrame : No PTS so use DTS %1 for position %2").arg(pts).arg(Position));
                                 }
                                 else
                                 {
                                    pts = 0;
                                    ToLog(LOGMSG_DEBUGTRACE, QString("IN:cVideoFile::ReadFrame : No PTS and no DTS for position %1").arg(Position));
                                 }
                              }
                              FrameBufferYUVReady = true;                                            // Keep actual value for FrameBufferYUV
                              FrameBufferYUVPosition = int64_t((qreal(pts)*av_q2d(VideoStream->time_base))*AV_TIME_BASE);    // Keep actual value for FrameBufferYUV
                              // Append this frame
                              cImageInCache *ObjImage =
                                 new cImageInCache(FrameBufferYUVPosition, FiltFrame, FrameBufferYUV);
                              FreeFrames = false;
                              IsVideoFind = false;
                              YUVCache[FrameBufferYUVPosition] = ObjImage;
                              if (YUVCache.lastKey() >= Position)
                                 IsVideoFind = true;
                           }
                           if (FreeFrames)
                           {
                              if (FiltFrame)
                              {
                                 FREEFRAME(&FiltFrame);
                              }
                              FREEFRAME(&FrameBufferYUV);
                           }
                           else
                           {
                              FrameBufferYUV = NULL;
                              FiltFrame = NULL;
                           }
                        }
                     }
                  }
               }
               // Check if we need to continue loop
               // Note: FPSDuration*(!VideoStream?2:1) is to enhance preview speed
               ContinueVideo = ContinueVideo && (VideoStream && !IsVideoFind && (VideoFramePosition * 1000000 < Position
                  || VideoFramePosition * 1000000 - Position < MAXDELTA));
            }

            // Continue with a new one
            if (StreamPacket != NULL)
            {
               deletePacket(&StreamPacket);
            }
         }
      }
      //qDebug() << "readFrame video stage1 " << PC2time(curPCounter()-pcpartv,true);
      //pcpartv = curPCounter();

      if (!RetImage && YUVCache.count() > 0)
      {
         QMap<int64_t, cImageInCache *>::const_iterator i = YUVCache.lowerBound(Position - MAXDELTA);
         QMap<int64_t, cImageInCache *>::const_iterator upperBound = YUVCache.upperBound(Position + MAXDELTA);
         QMap<int64_t, cImageInCache *>::const_iterator nearest = YUVCache.end();
         int64_t Nearest = MAXDELTA;
         while (i != upperBound)
         {
            if (abs(Position - i.key()) < Nearest)
            {
               nearest = i;
               Nearest = abs(Position - i.key());
            }
            ++i;
         }
         if (nearest != YUVCache.end())
         {
            //autoTimer at("cVideoFile::ConvertYUVToRGB");
            RetImage = ConvertYUVToRGB(PreviewMode, nearest.value()->FiltFrame ? nearest.value()->FiltFrame : nearest.value()->FrameBufferYUV);
            RetImagePosition = nearest.key();
            //ToLog(LOGMSG_DEBUGTRACE,QString("Video image for position %1 => return image at %2").arg(Position).arg(CacheImage[i]->Position));
            //qDebug() << QString("Video image for position %1 => return image at %2").arg(Position).arg(RetImagePosition);
         }
         else
         {
            ToLog(LOGMSG_CRITICAL, QString("No video image return for position %1 => return image at %2").arg(Position).arg(YUVCache.first()->Position));
            RetImage = ConvertYUVToRGB(PreviewMode, YUVCache.first()->FiltFrame ? YUVCache.first()->FiltFrame : YUVCache.first()->FrameBufferYUV);
            RetImagePosition = YUVCache.first()->Position;
         }
      }
      //qDebug() << "readFrame video stage2 " << PC2time(curPCounter()-pcpartv,true);
      //pcpartv = curPCounter();

      if (!RetImage)
      {
         ToLog(LOGMSG_CRITICAL, QString("No video image return for position %1 => return black frame").arg(Position));
         RetImage = new QImage(LibavVideoFile->streams[VideoStreamNumber]->codecpar->width, LibavVideoFile->streams[VideoStreamNumber]->codecpar->height, QImage::Format_ARGB32_Premultiplied);
         RetImage->fill(0);
         RetImagePosition = Position;
      }
      int64_t cutoff = Position - 50000;
      QMutableMapIterator<int64_t, cImageInCache *> mutmapiter(YUVCache);
      while (mutmapiter.hasNext())
      {
         mutmapiter.next();
         if (mutmapiter.key() < cutoff)
         {
            delete mutmapiter.value();
            mutmapiter.remove();
         }
         else
            break;
      }
   }
   //qDebug() << "readFrame video " << PC2time(curPCounter()-pcpart,true);
   //pcpart = curPCounter();

   if (AudioContext.AudioStream && SoundTrackBloc && YUVCache.count() > 0)
      SoundTrackBloc->AdjustSoundPosition(RetImagePosition);

   //Mutex.unlock();
   //qDebug() << "readFrame " << PC2time(curPCounter()-pc,true) ;
#ifdef CUDA_MEMCHECK
   size_t totalMem2, freeMem2;
   cudaMemGetInfo(&freeMem2, &totalMem2);
   qDebug() << QString("readFrame %1 bytes mem free of %2 bytes mem total, used %3 (%4 MB)\n").arg(freeMem2).arg(totalMem2).arg(freeMem - freeMem2).arg((freeMem - freeMem2) / (1024 * 1024));
   #endif
   return RetImage;
}

AVFrame *cVideoFile::ReadYUVFrame(bool PreviewMode, int64_t Position, bool DontUseEndPos, bool Deinterlace, cSoundBlockList *SoundTrackBloc, double Volume, bool ForceSoundOnly, int NbrDuration)
{
   //qDebug() << "cVideoFile::ReadFrame PreviewMode " << PreviewMode << " Position " << Position << " DontUseEndPos " << DontUseEndPos << " Deinterlace " << Deinterlace << " Volume " << Volume << " NbrDuration " <<NbrDuration;
   //qDebug() << "ReadYUVFrame from " << FileName() << " for Position " << Position;
   // Ensure file was previously open
   if (!IsOpen && !OpenCodecAndFile())
      return NULL;

#ifdef CUDA_MEMCHECK
   size_t totalMem, freeMem;
   cudaMemGetInfo(&freeMem, &totalMem);
   qDebug() << QString("%1 bytes mem free of %2 bytes mem total").arg(freeMem).arg(totalMem);
#endif   //AUTOTIMER(at,"cVideoFile::ReadFrame");
   //autoTimer at("cVideoFile::ReadFrame");

   //Position += LibavStartTime;//AV_TIME_BASE;
   //LONGLONG pc = curPCounter();
   //LONGLONG pcpart = pc;
   // Ensure file have an end file Position
   double dEndFile = double(QTime(0, 0, 0, 0).msecsTo(DontUseEndPos ? GetRealDuration() : EndTime)) / 1000.0;
   if (Position < 0)
      Position = 0;
   Position += LibavStartTime;//AV_TIME_BASE;
   dEndFile += LibavStartTime;
   if (dEndFile == 0)
   {
      ToLog(LOGMSG_CRITICAL, "Error in cVideoFile::ReadFrame : dEndFile=0 ?????");
      return NULL;
   }

   AVStream *VideoStream = (/*(!MusicOnly)&&(!ForceSoundOnly)&&*/VideoStreamNumber >= 0 ? LibavVideoFile->streams[VideoStreamNumber] : NULL);

   cVideoFile::sAudioContext AudioContext;
   AudioContext.SoundTrackBloc = SoundTrackBloc;
   AudioContext.AudioStream = (AudioStreamNumber >= 0 && SoundTrackBloc) ? LibavAudioFile->streams[AudioStreamNumber] : NULL;
   AudioContext.FPSSize = SoundTrackBloc ? SoundTrackBloc->SoundPacketSize*SoundTrackBloc->NbrPacketForFPS : 0;
   AudioContext.FPSDuration = AudioContext.FPSSize ? (double(AudioContext.FPSSize) / (SoundTrackBloc->Channels*SoundTrackBloc->SampleBytes*SoundTrackBloc->SamplingRate))*AV_TIME_BASE : 0;
   AudioContext.TimeBase = AudioContext.AudioStream ? double(AudioContext.AudioStream->time_base.den) / double(AudioContext.AudioStream->time_base.num) : 0;
   AudioContext.DstSampleSize = SoundTrackBloc ? (SoundTrackBloc->SampleBytes*SoundTrackBloc->Channels) : 0;
   AudioContext.NeedResampling = false;
   AudioContext.AudioLenDecoded = 0;
   AudioContext.Counter = 20; // Retry counter (when len>0 and avcodec_decode_audio4 fail to retreave frame, we retry counter time before to discard the packet)
   AudioContext.Volume = Volume;
   AudioContext.dEndFile = &dEndFile;
   AudioContext.NbrDuration = NbrDuration;
   AudioContext.DontUseEndPos = DontUseEndPos;

   if (!AudioContext.FPSDuration)
   {
      if (PreviewMode)
         AudioContext.FPSDuration = double(AV_TIME_BASE) / ((cApplicationConfig *)ApplicationConfig)->PreviewFPS;
      else if (VideoStream)
         AudioContext.FPSDuration = double(VideoStream->r_frame_rate.den * AV_TIME_BASE) / double(VideoStream->r_frame_rate.num);
      else
         AudioContext.FPSDuration = double(AV_TIME_BASE) / double(SoundTrackBloc->SamplingRate);
   }

   if (!AudioContext.AudioStream && !VideoStream)
      return NULL;

   //autoTimer atAV("cVideoFile::ReadFrame all");
   QMutexLocker locker(&accessMutex);
   //Mutex.lock();

   // If position >= end of file : disable audio (only if IsComputedAudioDuration)
   double dPosition = double(Position) / AV_TIME_BASE;
   //if ((dPosition > 0) && (dPosition >= dEndFile) && (IsComputedAudioDuration)) 
   if (dPosition > 0 && dPosition >= dEndFile + 1000 && IsComputedAudioDuration)
   {
      AudioContext.AudioStream = NULL; // Disable audio
                                       // Check if last image is ready and correspond to end of file
      if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= dEndFile * AV_TIME_BASE - AudioContext.FPSDuration)
      {
         //Mutex.unlock();
         return NULL;//QImage(LastImage.copy());
      }
      // If not then change Position to end file - a FPS to prepare a last image
      Position = dEndFile * AV_TIME_BASE - AudioContext.FPSDuration;
      dPosition = double(Position) / AV_TIME_BASE;
      if (SoundTrackBloc)
         SoundTrackBloc->UseLatestData();
   }

   //================================================
   bool ContinueVideo = true;
   AudioContext.ContinueAudio = (AudioContext.AudioStream) && (SoundTrackBloc);
   bool ResamplingContinue = (Position != 0);
   AudioContext.AudioFramePosition = dPosition;
   //================================================

   if (AudioContext.ContinueAudio)
   {
      AudioContext.NeedResampling = ((AudioContext.AudioStream->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_S16)
         || (AudioContext.AudioStream->CODEC_OR_PAR->CC_CHANNELS != SoundTrackBloc->Channels)
         || (AudioContext.AudioStream->CODEC_OR_PAR->sample_rate != SoundTrackBloc->SamplingRate));

      //qDebug() << "audio needs resampling " << AudioContext.NeedResampling;
      // Calc if we need to seek to a position
      int64_t Start = /*SoundTrackBloc->LastReadPosition;*/SoundTrackBloc->CurrentPosition;
      int64_t End = Start + SoundTrackBloc->GetDuration();
      int64_t Wanted = AudioContext.FPSDuration * AudioContext.NbrDuration;
      //qDebug() << "Start " << Start << " End " << End << " Position " << Position << " Wanted " << Wanted;
      if ((Position >= Start && Position + Wanted <= End) /*|| Start < 0 */)
         AudioContext.ContinueAudio = false;
      if (AudioContext.ContinueAudio && (Position == 0 || Start < 0 || LastAudioReadPosition < 0 /*|| Position < Start*/ || Position > End + 1500000))
      {
         if (Position < 0)
            Position = 0;
         SoundTrackBloc->ClearList();                // Clear soundtrack list
         ResamplingContinue = false;
         LastAudioReadPosition = 0;
         SeekErrorCount = 0;
         int64_t seekPos = Position - SoundTrackBloc->WantedDuration * 1000.0;
         /*bool bSeekRet = */SeekFile(NULL, AudioContext.AudioStream, seekPos);        // Always seek one FPS before to ensure eventual filter have time to init
                                                                                       //qDebug() << "seek to " << seekPos << " is " << (bSeekRet?"true":"false");
         AudioContext.AudioFramePosition = Position / AV_TIME_BASE;
      }

      // Prepare resampler
      if (AudioContext.ContinueAudio && AudioContext.NeedResampling)
      {
         if (!ResamplingContinue)
            CloseResampler();
         AVChannelLayout avcl;
         av_channel_layout_default(&avcl, SoundTrackBloc->Channels);
         CheckResampler(AudioContext.AudioStream->codecpar->CC_CHANNELS, SoundTrackBloc->Channels,
            AVSampleFormat(AudioContext.AudioStream->codecpar->format), SoundTrackBloc->SampleFormat,
            AudioContext.AudioStream->codecpar->sample_rate, SoundTrackBloc->SamplingRate,
            AudioContext.AudioStream->codecpar->ch_layout ,
            avcl 
         );
      }
   }

   AVFrame *RetFrame = NULL;
   int64_t RetImagePosition = 0;
   double VideoFramePosition = dPosition;

   // Count number of image > position
   bool IsVideoFind = false;
   if (!YUVCache.isEmpty() /*&& YUVCache.lastKey() >= Position*/)
   {
      QMap<int64_t, cImageInCache *>::const_iterator i = YUVCache.constBegin();
      while (i != YUVCache.constEnd() && !IsVideoFind)
      {
         if (i.key() >= Position && i.key() - Position < ALLOWEDDELTA)
            IsVideoFind = true;
         ++i;
      }
   }
   ContinueVideo = (VideoStream && !IsVideoFind && !ForceSoundOnly);
   if (ContinueVideo)
   {
      int64_t DiffTimePosition = -1000000;  // Compute difftime between asked position and previous end decoded position

      if (FrameBufferYUVReady)
      {
         DiffTimePosition = Position - FrameBufferYUVPosition;
         //if ((Position==0)||(DiffTimePosition<0)||(DiffTimePosition>1500000))
         //    ToLog(LOGMSG_INFORMATION,QString("VIDEO-SEEK %1 TO %2").arg(ShortName).arg(Position));
      }

      // Calc if we need to seek to a position
      if (Position == 0 || DiffTimePosition < 0 || DiffTimePosition > 1500000) // Allow 1,5 sec diff (rounded double !)
      {
         if (Position < 0)
            Position = 0;
         SeekErrorCount = 0;
         SeekFile(VideoStream, NULL, Position);        // Always seek one FPS before to ensure eventual filter have time to init
         VideoFramePosition = Position / AV_TIME_BASE;
      }
   }

   //*************************************************************************************************************************************
   // Decoding process : Get StreamPacket until endposition is reach (if sound is wanted) or until image is ok (if image only is wanted)
   //*************************************************************************************************************************************
   //qDebug() << "readFrame prep " << PC2time(curPCounter()-pcpart,true);
   //pcpart = curPCounter();

   // AUDIO PART
   //QFutureWatcher<void> ThreadAudio;
   {
      //AUTOTIMER(atAudio,"cVideoFile::ReadFrame audio Part");
      //autoTimer atAudio("cVideoFile::ReadFrame audio Part");
      //if( !AudioContext.ContinueAudio )
      //   qDebug() << "cVideoFile::ReadFrame audio, AudioContext.ContinueAudio is false!!!!!";

      while (AudioContext.ContinueAudio)
      {
         AVPacket *StreamPacket = getNewPacket();
         if (!StreamPacket)
         {
            AudioContext.ContinueAudio = false;
         }
         else
         {
            StreamPacket->flags |= AV_PKT_FLAG_KEY;
            int err;
            if ((err = av_read_frame(LibavAudioFile, StreamPacket)) < 0)
            {
               // If error reading frame then we considere we have reach the end of file
               if (!IsComputedAudioDuration)
               {
                  dEndFile = qreal(SoundTrackBloc->/*LastReadPosition*/CurrentPosition) / AV_TIME_BASE;
                  dEndFile += qreal(SoundTrackBloc->GetDuration()) / 1000;
                  if (dEndFile - LibavStartTime / AV_TIME_BASE == double(QTime(0, 0, 0, 0).msecsTo(EndTime)) / 1000)
                     EndTime = QTime(0, 0, 0).addMSecs((dEndFile - LibavStartTime) * 1000);
                  SetRealAudioDuration(QTime(0, 0, 0, 0).addMSecs(qlonglong((dEndFile - LibavStartTime / AV_TIME_BASE) * 1000)));
               }
               AudioContext.ContinueAudio = false;
               //qDebug() << "cVideoFile::ReadFrame audio, set AudioContext.ContinueAudio to false!!!!! err is "<<err;

               // Use data in TempData to create a latest block
               SoundTrackBloc->UseLatestData();
            }
            else
            {
               DecodeAudio(&AudioContext, StreamPacket, Position);
               StreamPacket = NULL;
            }
         }
         // Continue with a new one
         if (StreamPacket != NULL)
         {
            deletePacket(&StreamPacket);
         }
      }
   }
   //qDebug() << "readFrame audio " << PC2time(curPCounter()-pcpart,true);
   //pcpart = curPCounter();

   // VIDEO PART
   if (VideoStream && !ForceSoundOnly)
   {
      //AUTOTIMER(atAudio,"cVideoFile::ReadFrame video Part");
      //autoTimer atVideo("cVideoFile::ReadFrame video Part");
      //LONGLONG pcpartv = curPCounter();
      if (!ContinueVideo)
      {
         ToLog(LOGMSG_DEBUGTRACE, QString("Video image for position %1 => use image in cache").arg(Position));
      }
      else if (Position < LibavStartTime)
      {
         ToLog(LOGMSG_CRITICAL, QString("Image position %1 is before video stream start => return black frame").arg(Position));
         //RetImage = new QImage(LibavVideoFile->streams[VideoStreamNumber]->codec->width, LibavVideoFile->streams[VideoStreamNumber]->codec->height, QImage::Format_ARGB32_Premultiplied);
         //RetImage->fill(0);
         RetImagePosition = Position;
      }
      else
      {
         bool ByPassFirstImage = (Deinterlace) && (YUVCache.count() == 0);
         int MaxErrorCount = 20;
         bool FreeFrames = false;

         while (ContinueVideo)
         {
            AVPacket *StreamPacket = getNewPacket();
            if (!StreamPacket)
            {
               ContinueVideo = false;
            }
            else
            {
               StreamPacket->flags |= AV_PKT_FLAG_KEY;  // HACK for CorePNG to decode as normal PNG by default

               int errcode = 0;
               if ((errcode = av_read_frame(LibavVideoFile, StreamPacket)) < 0)
               {
                  if (errcode == AVERROR_EOF)
                  {
                     // We have reach the end of file
                     if (!IsComputedAudioDuration)
                     {
                        dEndFile = VideoFramePosition;
                        if (dEndFile - LibavStartTime == double(QTime(0, 0, 0, 0).msecsTo(EndTime)) / 1000)
                           EndTime = QTime(0, 0, 0).addMSecs((dEndFile - LibavStartTime)*1000.0);
                        SetRealVideoDuration(QTime(0, 0, 0, 0).addMSecs(qlonglong((dEndFile - LibavStartTime) * 1000)));
                     }
                     ContinueVideo = false;

                     if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= (dEndFile - 1.5)*AV_TIME_BASE)
                     {
                        //if (!RetImage)
                        //{
                        //   RetImage = new QImage(LastImage);
                        //   RetImagePosition = FrameBufferYUVPosition;
                        //}
                        IsVideoFind = true;
                        ContinueVideo = false;
                     }
                  }
                  else
                  {
                     ToLog(LOGMSG_CRITICAL, GetAvErrorMessage(errcode));
                     // If error reading frame
                     if (MaxErrorCount > 0)
                     {
                        // Files with stream could provoque this, so we ignore the first MaxErrorCount errors
                        MaxErrorCount--;
                     }
                     else
                     {
                        if (!LastImage.isNull() && FrameBufferYUVReady && FrameBufferYUVPosition >= (dEndFile - 1.5)*AV_TIME_BASE)
                        {
                           //if (!RetImage)
                           //{
                           //   RetImage = new QImage(LastImage);
                           //   RetImagePosition = FrameBufferYUVPosition;
                           //}
                           IsVideoFind = true;
                           ContinueVideo = false;
                        }
                        else
                        {
                           SeekErrorCount = 0;
                           ContinueVideo = SeekFile(VideoStream, NULL, Position - 2 * AudioContext.FPSDuration);
                        }
                     }
                  }
               }
               else
               {
                  int64_t FramePts = StreamPacket->pts != (int64_t)AV_NOPTS_VALUE ? StreamPacket->pts : -1;
                  double TimeBase = double(LibavVideoFile->streams[StreamPacket->stream_index]->time_base.den) / double(LibavVideoFile->streams[StreamPacket->stream_index]->time_base.num);
                  if (FramePts >= 0)
                     VideoFramePosition = (double(FramePts) / TimeBase);

                  if (StreamPacket->stream_index == VideoStreamNumber)
                  {
                     // Allocate structures
                     if (FrameBufferYUV == NULL)
                        FrameBufferYUV = ALLOCFRAME();
                     if (FrameBufferYUV)
                     {
                        int FrameDecoded = -1;
                        LastLibAvMessageLevel = 0;    // Clear LastLibAvMessageLevel : some decoder dont return error but display errors messages !
                        int Error = avcodec_send_packet(VideoCodecContext, StreamPacket);
                        //int Error = avcodec_decode_video2(VideoStream->codec, FrameBufferYUV, &FrameDecoded, StreamPacket);
                        if (Error < 0 || LastLibAvMessageLevel == LOGMSG_CRITICAL)
                        {
                           if (MaxErrorCount > 0)
                           {
                              if (VideoFramePosition*1000000.0 < Position)
                              {
                                 ToLog(LOGMSG_INFORMATION, QString("IN:cVideoFile::ReadFrame - Error decoding packet: try left %1").arg(MaxErrorCount));
                              }
                              else
                              {
                                 ToLog(LOGMSG_INFORMATION, QString("IN:cVideoFile::ReadFrame - Error decoding packet: seek to backward and restart reading"));
                                 if (Position > 1000000)
                                 {
                                    SeekErrorCount = 0;
                                    SeekFile(VideoStream, NULL/*AudioStream*/, Position - 1000000); // 1 sec before
                                 }
                                 else
                                 {
                                    SeekErrorCount = 0;
                                    SeekFile(VideoStream, NULL, 0);
                                 }
                              }
                              MaxErrorCount--;
                           }
                           else
                           {
                              ToLog(LOGMSG_CRITICAL, QString("IN:cVideoFile::ReadFrame - Error decoding packet: and no try left"));
                              ContinueVideo = false;
                           }
                        }
                        else
                           FrameDecoded = avcodec_receive_frame(VideoCodecContext, FrameBufferYUV);
                        if (FrameDecoded == 0)
                        {

                           //FrameBufferYUV->pkt_pts = av_frame_get_best_effort_timestamp(FrameBufferYUV);
                           if(FrameBufferYUV->pts == AV_NOPTS_VALUE)
                           FrameBufferYUV->pts = FrameBufferYUV->best_effort_timestamp;
                           // Video filter part

                           AVFrame *FiltFrame = NULL;
                           if (VideoFilterGraph)
                           {
                              // FFMPEG 2.0
                              // push the decoded frame into the filtergraph
                              if (av_buffersrc_add_frame_flags(VideoFilterIn, FrameBufferYUV, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                              {
                                 ToLog(LOGMSG_INFORMATION, "IN:cVideoFile::ReadFrame : Error while feeding the filtergraph");
                              }
                              else
                              {
                                 FiltFrame = ALLOCFRAME();
                                 // pull filtered frames from the filtergraph
                                 int ret = av_buffersink_get_frame(VideoFilterOut, FiltFrame);
                                 if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                                 {
                                    ToLog(LOGMSG_INFORMATION, "IN:cVideoFile::ReadFrame : No image return by filter process");
                                    FREEFRAME(&FiltFrame);
                                 }
                              }
                           }
                           if (ByPassFirstImage)
                           {
                              ByPassFirstImage = false;
                              FreeFrames = true;
                           }
                           else
                           {
                              int64_t pts = FrameBufferYUV->pts;
                              if (pts == (int64_t)AV_NOPTS_VALUE)
                              {
                                 if (FrameBufferYUV->pkt_dts != (int64_t)AV_NOPTS_VALUE)
                                 {
                                    pts = FrameBufferYUV->pkt_dts;
                                    ToLog(LOGMSG_DEBUGTRACE, QString("IN:cVideoFile::ReadFrame : No PTS so use DTS %1 for position %2").arg(pts).arg(Position));
                                 }
                                 else
                                 {
                                    pts = 0;
                                    ToLog(LOGMSG_DEBUGTRACE, QString("IN:cVideoFile::ReadFrame : No PTS and no DTS for position %1").arg(Position));
                                 }
                              }
                              FrameBufferYUVReady = true;                                            // Keep actual value for FrameBufferYUV
                              FrameBufferYUVPosition = int64_t((qreal(pts)*av_q2d(VideoStream->time_base))*AV_TIME_BASE);    // Keep actual value for FrameBufferYUV
                                                                                                                             // Append this frame
                              cImageInCache *ObjImage =
                                 new cImageInCache(FrameBufferYUVPosition, FiltFrame, FrameBufferYUV);
                              FreeFrames = false;
                              IsVideoFind = false;
                              YUVCache[FrameBufferYUVPosition] = ObjImage;
                              if (YUVCache.lastKey() >= Position)
                                 IsVideoFind = true;
                           }
                           if (FreeFrames)
                           {
                              if (FiltFrame)
                              {
                                 FREEFRAME(&FiltFrame);
                                 //FiltFrame = NULL; // added
                                 av_frame_unref(FrameBufferYUV);
                              }
                              FREEFRAME(&FrameBufferYUV);
                           }
                           else
                           {
                              FrameBufferYUV = NULL;
                              FiltFrame = NULL;
                           }

                        }
                     }
                  }
               }
               // Check if we need to continue loop
               // Note: FPSDuration*(!VideoStream?2:1) is to enhance preview speed
               ContinueVideo = ContinueVideo && (VideoStream && !IsVideoFind && (VideoFramePosition * 1000000 < Position
                  || VideoFramePosition * 1000000 - Position < MAXDELTA));
            }

            // Continue with a new one
            if (StreamPacket != NULL)
            {
               deletePacket(&StreamPacket);
            }
         }
      }
      //qDebug() << "readFrame video stage1 " << PC2time(curPCounter()-pcpartv,true);
   //pcpartv = curPCounter();

   if (/*!RetImage && */YUVCache.count() > 0)
   {
      QMap<int64_t, cImageInCache *>::const_iterator i = YUVCache.lowerBound(Position - MAXDELTA);
      QMap<int64_t, cImageInCache *>::const_iterator upperBound = YUVCache.upperBound(Position + MAXDELTA);
      QMap<int64_t, cImageInCache *>::const_iterator nearest = YUVCache.end();
      int64_t Nearest = MAXDELTA;
      while (i != upperBound)
      {
         if (abs(Position - i.key()) < Nearest)
         {
            nearest = i;
            Nearest = abs(Position - i.key());
         }
         ++i;
      }
      if (nearest != YUVCache.end())
      {
         //autoTimer at("cVideoFile::ConvertYUVToRGB");
         //RetImage = ConvertYUVToRGB(PreviewMode, nearest.value()->FiltFrame ? nearest.value()->FiltFrame : nearest.value()->FrameBufferYUV);
         RetFrame = nearest.value()->FiltFrame ? nearest.value()->FiltFrame : nearest.value()->FrameBufferYUV;
         RetImagePosition = nearest.key();
         //ToLog(LOGMSG_DEBUGTRACE,QString("Video image for position %1 => return image at %2").arg(Position).arg(CacheImage[i]->Position));
      }
      else
      {
         ToLog(LOGMSG_CRITICAL, QString("No video YUV return for position %1 => return image at %2").arg(Position).arg(YUVCache.first()->Position));
         qDebug() << "no yuv for " << FileName();
         //RetImage = ConvertYUVToRGB(PreviewMode, YUVCache.first()->FiltFrame ? YUVCache.first()->FiltFrame : YUVCache.first()->FrameBufferYUV);
         RetFrame = YUVCache.first()->FiltFrame ? YUVCache.first()->FiltFrame : YUVCache.first()->FrameBufferYUV;
         RetImagePosition = YUVCache.first()->Position;
      }
   }
   //qDebug() << "readFrame video stage2 " << PC2time(curPCounter()-pcpartv,true);
   //pcpartv = curPCounter();

   //if (!RetImage)
   //{
   //   ToLog(LOGMSG_CRITICAL, QString("No video image return for position %1 => return black frame").arg(Position));
   //   RetImage = new QImage(LibavVideoFile->streams[VideoStreamNumber]->codec->width, LibavVideoFile->streams[VideoStreamNumber]->codec->height, QImage::Format_ARGB32_Premultiplied);
   //   RetImage->fill(0);
   //   RetImagePosition = Position;
   //}
   if (RetFrame)
   {
      RetFrame = CLONEFRAME(RetFrame);
      //qDebug() << "cloned Frame is " << RetFrame;
   }
   int64_t cutoff = Position - 50000;
   QMutableMapIterator<int64_t, cImageInCache *> mutmapiter(YUVCache);
   while (mutmapiter.hasNext())
   {
      mutmapiter.next();
      if (mutmapiter.key() < cutoff)
      {
         //bool noDel = false;
         //if(RetFrame == mutmapiter.value()->FiltFrame || RetFrame == mutmapiter.value()->FrameBufferYUV)
         //{ 
         //   qDebug() << "skip delete the current returning frame";
         //   noDel = true;
         //}
         //if( !noDel)
         {
            delete mutmapiter.value();
            mutmapiter.remove();
         }
      }
      else
         break;
   }
#ifdef CUDA_MEMCHECK
   size_t totalMem2, freeMem2;
   cudaMemGetInfo(&freeMem2, &totalMem2);
   qDebug() << QString("readYuvFrame %1 bytes mem free of %2 bytes mem total, used %3 (%4 MB)\n").arg(freeMem2).arg(totalMem2).arg(freeMem - freeMem2).arg((freeMem - freeMem2) / (1024 * 1024));
#endif
}
//qDebug() << "readFrame video " << PC2time(curPCounter()-pcpart,true);
//pcpart = curPCounter();

if (AudioContext.AudioStream && SoundTrackBloc && YUVCache.count() > 0)
SoundTrackBloc->AdjustSoundPosition(RetImagePosition);

//Mutex.unlock();
//qDebug() << "readFrame " << PC2time(curPCounter()-pc,true) ;
return RetFrame;
}

//====================================================================================================================

void cVideoFile::DecodeAudio(sAudioContext *AudioContext, AVPacket *StreamPacket, int64_t Position)
{
   //qDebug() << "decode Audio";
//autoTimer at("deceodeAudio");
   qreal FramePts = StreamPacket->pts != (int64_t)AV_NOPTS_VALUE ? StreamPacket->pts*av_q2d(AudioContext->AudioStream->time_base) : -1;
   if (StreamPacket->stream_index == AudioStreamNumber && StreamPacket->size > 0)
   {
      //qDebug() << "decode Audio, pts = " << StreamPacket->pts << " dts = " << StreamPacket->dts;
      //AVPacket PacketTemp;
      //av_init_packet(&PacketTemp);
      //PacketTemp.data = StreamPacket->data;
      //PacketTemp.size = StreamPacket->size;
      int packet_err = avcodec_send_packet(AudioCodecContext, StreamPacket);
      if (packet_err == 0)
      {
         AVFrame *Frame = nullptr;
         int gotFrame = 0;
         while (gotFrame == 0)
         {
            Frame = ALLOCFRAME();
            gotFrame = avcodec_receive_frame(AudioCodecContext, Frame);
            if (gotFrame == 0)
               DecodeAudioFrame(AudioContext, &FramePts, Frame, Position);
         }
         if (Frame != NULL)
            FREEFRAME(&Frame);
         //FREEFRAME(&Frame);
      }
      //qDebug() << "decode Audio, PacketTemp.size is " << PacketTemp.size;
      // NOTE: the audio packet can contain several NbrFrames
   //   while (AudioContext->Counter > 0 && AudioContext->ContinueAudio && PacketTemp.size > 0) 
   //   {
   //      AVFrame *Frame = ALLOCFRAME();
   //      int got_frame;
   //      int Len = avcodec_decode_audio4(AudioCodecContext,Frame,&got_frame,&PacketTemp);
   //      //qDebug() << "avcodec_decode_audio4 gives " << Len;
   //      if (Len < 0)
   //      {
   //         // if error, we skip the frame and exit the while loop
   //         PacketTemp.size = 0;
   //      } 
   //      else if (got_frame > 0) 
   //      {
   //         DecodeAudioFrame(AudioContext, &FramePts, Frame, Position);
   //         Frame = NULL;
   //         PacketTemp.data += Len;
   //         PacketTemp.size -= Len;
   //      } 
   //      else 
   //      {
   //         AudioContext->Counter--;
   //         if (AudioContext->Counter == 0) 
   //         {
   //            Len = 0;
   //            ToLog(LOGMSG_CRITICAL,QString("Impossible to decode audio frame: Discard it"));
   //         }
   //      }
   //      if (Frame != NULL) 
   //         FREEFRAME(&Frame);
   //   }
   }
   // Continue with a new one
   if (StreamPacket != NULL)
   {
      deletePacket(&StreamPacket);
   }
   // Check if we need to continue loop
   // Note: FPSDuration*(!VideoStream?2:1) is to enhance preview speed
   AudioContext->ContinueAudio = (AudioContext->ContinueAudio && AudioContext->Counter > 0 && AudioContext->AudioStream && AudioContext->SoundTrackBloc
      && ((AudioContext->SoundTrackBloc->ListCount() < AudioContext->SoundTrackBloc->NbrPacketForFPS)
      || (!(LastAudioReadPosition >= Position + AudioContext->FPSDuration*AudioContext->NbrDuration))));
}

//============================

void cVideoFile::DecodeAudioFrame(sAudioContext *AudioContext, qreal *FramePts, AVFrame *Frame, int64_t Position)
{
   //qDebug() << "DecodeAudioFrame";
   int64_t SizeDecoded = 0;
   u_int8_t *Data = NULL;
   if (AudioContext->NeedResampling && RSC != NULL)
   {
      Data = Resample(Frame, &SizeDecoded, AudioContext->DstSampleSize);
   }
   else
   {
      Data = Frame->data[0];
      SizeDecoded = Frame->nb_samples*av_get_bytes_per_sample(AudioCodecContext->sample_fmt)*AudioCodecContext->CC_CHANNELS;
   }
   AudioContext->ContinueAudio = (Data != NULL);
   if (AudioContext->ContinueAudio)
   {
      // Adjust FrameDuration with real Nbr Sample
      double FrameDuration = double(SizeDecoded) / (AudioContext->SoundTrackBloc->SamplingRate*AudioContext->DstSampleSize);
      // Adjust pts and inc FramePts int the case there are multiple blocks
      qreal pts = (*FramePts) / av_q2d(AudioContext->AudioStream->time_base);
      if (pts < 0)
         pts = qreal(Position + AudioContext->FPSDuration);
      (*FramePts) += FrameDuration;
      AudioContext->AudioFramePosition = *FramePts;
      // Adjust volume if master volume <>1
      double Volume = AudioContext->Volume;
      if (Volume == -1)
         Volume = GetSoundLevel() != -1 ? double(ApplicationConfig->DefaultSoundLevel) / double(GetSoundLevel() * 100) : 1;
      if (Volume != 1)
      {
         //qDebug() << "apply Volume " << Volume;
         int16_t *Buf1 = (int16_t*)Data;
         for (int j = 0; j < SizeDecoded / 4; j++)
         {
            // Left channel : Adjust if necessary (16 bits)
            *Buf1 = (int16_t)qBound(-32768.0, double(*Buf1)*Volume, 32767.0); Buf1++;
            // Right channel : Adjust if necessary (16 bits)
            *Buf1 = (int16_t)qBound(-32768.0, double(*Buf1)*Volume, 32767.0); Buf1++;
         }
      }
      // Append decoded data to SoundTrackBloc
      if (AudioContext->DontUseEndPos || AudioContext->AudioFramePosition < *AudioContext->dEndFile)
      {
         //qDebug() << "add decoded data to SoundTrackBloc AudioContext->DontUseEndPos:" << AudioContext->DontUseEndPos 
         //   << " AudioContext->AudioFramePosition:" << AudioContext->AudioFramePosition 
         //   << " AudioContext->dEndFile:" <<*AudioContext->dEndFile
         //   << " SizeDecoded " << SizeDecoded;
         AudioContext->SoundTrackBloc->AppendData(AudioContext->AudioFramePosition*AV_TIME_BASE, (int16_t*)Data, SizeDecoded);
         //AudioContext->SoundTrackBloc->LastReadPosition = AudioContext->AudioFramePosition*AV_TIME_BASE;
         AudioContext->AudioLenDecoded += SizeDecoded;
         AudioContext->AudioFramePosition = AudioContext->AudioFramePosition + FrameDuration;
      }
      else
      {
         AudioContext->ContinueAudio = false;
         // qDebug() << "do not add decoded data to SoundTrackBloc!!";
      }
   }
   LastAudioReadPosition = int64_t(AudioContext->AudioFramePosition*AV_TIME_BASE);    // Keep NextPacketPosition for determine next time if we need to seek
   if (Data != Frame->data[0])
      av_free(Data);
   FREEFRAME(&Frame);
}

//====================================================================================================================

//int QMyImage::offset = -1;
//bool QMyImage::offsetDetected = false;
//
//void QMyImage::detectOffset()
//{
//   if(offsetDetected)
//      return;
//   QImage testImg(1,1,QImage::Format_RGB32);
//   int *data = (int*)testImg.data_ptr();
//   int datasize = 50;
//   QList<int> firstRun;
//   QList<int> secondRun;
//   QList<int> thirdRun;
//   for(int i = 0; i < datasize; i++ )
//   {
//      if( *data == QImage::Format_RGB32 )
//         firstRun.append(i);
//      data++;
//   }
//   testImg = testImg.convertToFormat(QImage::Format_ARGB32);
//   data = (int*)testImg.data_ptr();
//   for(int i = 0; i < datasize; i++ )
//   {
//      if( *data == QImage::Format_ARGB32 )
//      {
//         if( firstRun.contains(i) )
//            secondRun.append(i);
//      }
//      data++;
//   }
//
//   testImg = testImg.convertToFormat(QImage::Format_ARGB32_Premultiplied);
//   data = (int*)testImg.data_ptr();
//   for(int i = 0; i < datasize; i++ )
//   {
//      if( *data == QImage::Format_ARGB32_Premultiplied )
//      {
//         if( secondRun.contains(i) )
//            thirdRun.append(i);
//      }
//      data++;
//   }
//   if( thirdRun.count() == 1 )
//      offset = thirdRun.first();
//   offsetDetected = true;
//}
//
//#ifdef QT_COMPILER_SUPPORTS_SSE4_1
//#define BYTE_MUL_SSE2(result, pixelVector, alphaChannel, colorMask, half) \
//{ \
//    /* 1. separate the colors in 2 vectors so each color is on 16 bits \
//       (in order to be multiplied by the alpha \
//       each 32 bit of dstVectorAG are in the form 0x00AA00GG \
//       each 32 bit of dstVectorRB are in the form 0x00RR00BB */\
//    __m128i pixelVectorAG = _mm_srli_epi16(pixelVector, 8); \
//    __m128i pixelVectorRB = _mm_and_si128(pixelVector, colorMask); \
// \
//    /* 2. multiply the vectors by the alpha channel */\
//    pixelVectorAG = _mm_mullo_epi16(pixelVectorAG, alphaChannel); \
//    pixelVectorRB = _mm_mullo_epi16(pixelVectorRB, alphaChannel); \
// \
//    /* 3. divide by 255, that's the tricky part. \
//       we do it like for BYTE_MUL(), with bit shift: X/255 ~= (X + X/256 + rounding)/256 */ \
//    /** so first (X + X/256 + rounding) */\
//    pixelVectorRB = _mm_add_epi16(pixelVectorRB, _mm_srli_epi16(pixelVectorRB, 8)); \
//    pixelVectorRB = _mm_add_epi16(pixelVectorRB, half); \
//    pixelVectorAG = _mm_add_epi16(pixelVectorAG, _mm_srli_epi16(pixelVectorAG, 8)); \
//    pixelVectorAG = _mm_add_epi16(pixelVectorAG, half); \
// \
//    /** second divide by 256 */\
//    pixelVectorRB = _mm_srli_epi16(pixelVectorRB, 8); \
//    /** for AG, we could >> 8 to divide followed by << 8 to put the \
//        bytes in the correct position. By masking instead, we execute \
//        only one instruction */\
//    pixelVectorAG = _mm_andnot_si128(colorMask, pixelVectorAG); \
// \
//    /* 4. combine the 2 pairs of colors */ \
//    result = _mm_or_si128(pixelVectorAG, pixelVectorRB); \
//} 
//#endif
//
//bool QMyImage::forcePremulFormat()
//{
//   if (isNull() || format() == QImage::Format_ARGB32_Premultiplied)
//      return true;
//   if (format() != QImage::Format_ARGB32)
//      return false;
//   if (!offsetDetected)
//      detectOffset();
//   if (offset < 0)
//      return false;
//   setFormat(QImage::Format_ARGB32_Premultiplied);
//   return true;
//}
//
////  x/255 = (x*257+257)>>16
//bool QMyImage::convert_ARGB_to_ARGB_PM_inplace()
//{
//   if( isNull() || format() ==  QImage::Format_ARGB32_Premultiplied )
//      return true;
//   if( format() != QImage::Format_ARGB32 )
//      return false;
//   if( !offsetDetected )
//      detectOffset();
//   if( offset < 0 ) 
//      return false;
//
//#ifndef QT_COMPILER_SUPPORTS_SSE4_1
//    uint *buffer = reinterpret_cast<uint*>( const_cast<uchar *>(((const QMyImage*)this)->bits()) );
//    int count = byteCount()/4;
//    for (int i = 0; i < count; ++i)
//    {
//        *buffer = qPremultiply(*buffer);
//        buffer++;
//    }
//#else
//    // extra pixels on each line
//    const int spare = width() & 3;
//    // width in pixels of the pad at the end of each line
//    const int pad = (bytesPerLine() >> 2) - width();
//    const int iter = width() >> 2;
//    int _height = height();
//
//    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
//    const __m128i nullVector = _mm_setzero_si128();
//    const __m128i half = _mm_set1_epi16(0x80);
//    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
//
//    __m128i *d = reinterpret_cast<__m128i*>( const_cast<uchar *>(((const QMyImage*)this)->bits()) );
//    while (_height--) {
//        const __m128i *end = d + iter;
//
//        for (; d != end; ++d) {
//            const __m128i srcVector = _mm_loadu_si128(d);
//            const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask);
//            if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) {
//                // opaque, data is unchanged
//            } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) == 0xffff) {
//                // fully transparent
//                _mm_storeu_si128(d, nullVector);
//            } else {
//                __m128i alphaChannel = _mm_srli_epi32(srcVector, 24);
//                alphaChannel = _mm_or_si128(alphaChannel, _mm_slli_epi32(alphaChannel, 16));
//
//                __m128i result;
//                BYTE_MUL_SSE2(result, srcVector, alphaChannel, colorMask, half);
//                result = _mm_or_si128(_mm_andnot_si128(alphaMask, result), srcVectorAlpha);
//                _mm_storeu_si128(d, result);
//            }
//        }
//
//        QRgb *p = reinterpret_cast<QRgb*>(d);
//        QRgb *pe = p+spare;
//        for (; p != pe; ++p) {
//            if (*p < 0x00ffffff)
//                *p = 0;
//            else if (*p < 0xff000000)
//                *p = qPremultiply(*p);
//        }
//
//        d = reinterpret_cast<__m128i*>(p+pad);
//    }
//#endif
//    setFormat(QImage::Format_ARGB32_Premultiplied);
//    return true;
//} 
//
//inline QRgb qUnpremultiply_sse4(QRgb p)
//{
//   const uint alpha = qAlpha(p);
//   if (alpha == 255 || alpha == 0)
//      return p;
//   const uint invAlpha = qt_inv_premul_factor[alpha];
//   const __m128i via = _mm_set1_epi32(invAlpha);
//   const __m128i vr = _mm_set1_epi32(0x8000);
//   __m128i vl = _mm_cvtepu8_epi32(_mm_cvtsi32_si128(p));
//   vl = _mm_mullo_epi32(vl, via);
//   vl = _mm_add_epi32(vl, vr);
//   vl = _mm_srai_epi32(vl, 16);
//   vl = _mm_insert_epi32(vl, alpha, 3);
//   vl = _mm_packus_epi32(vl, vl);
//   vl = _mm_packus_epi16(vl, vl);
//   return _mm_cvtsi128_si32(vl);
//}
//
//bool QMyImage::convert_ARGB_PM_to_ARGB_inplace()
//{
//   if( isNull() || format() ==  QImage::Format_ARGB32 )
//      return true;
//   if( !offsetDetected )
//      detectOffset();
//   if( format() != QImage::Format_ARGB32_Premultiplied || offset < 0)
//      return false;
//    uint *buffer = reinterpret_cast<uint*>( const_cast<uchar *>(((const QMyImage*)this)->bits()) );
//    int count = byteCount()/4;
//    for (int i = 0; i < count; ++i)
//    {
//       //*buffer = qUnpremultiply(*buffer++);
//       //*buffer = qUnpremultiply_sse4(*buffer++);
//       QRgb p = *buffer;
//       const uint alpha = qAlpha(p);
//       // Alpha 255 and 0 are the two most common values, which makes them beneficial to short-cut.
//       if (alpha == 0)
//          *buffer = 0;
//       if (alpha != 255)
//       {
//          // (p*(0x00ff00ff/alpha)) >> 16 == (p*255)/alpha for all p and alpha <= 256.
//          const uint invAlpha = qt_inv_premul_factor[alpha];
//          // We add 0x8000 to get even rounding. The rounding also ensures that qPremultiply(qUnpremultiply(p)) == p for all p.
//          *buffer = qRgba((qRed(p)*invAlpha + 0x8000) >> 16, (qGreen(p)*invAlpha + 0x8000) >> 16, (qBlue(p)*invAlpha + 0x8000) >> 16, alpha);
//       }
//    }
//   setFormat(QImage::Format_ARGB32);
//   return true;
//}
//
//
////void QMyImage::convert_ARGB_to_ARGB_PM_inplace()
////{
////    //Q_ASSERT(data->format() == QImage::Format_ARGB32);
////
////    // extra pixels on each line
////    const int spare = width() & 3;
////    // width in pixels of the pad at the end of each line
////    const int pad = (bytesPerLine() >> 2) - width();
////    const int iter = width() >> 2;
////    int _height = height();
////
////    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
////    const __m128i nullVector = _mm_setzero_si128();
////    const __m128i half = _mm_set1_epi16(0x80);
////    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
////
////    __m128i *d = reinterpret_cast<__m128i*>( const_cast<uchar *>(((const QMyImage*)this)->bits()) );
////    while (_height--) {
////        const __m128i *end = d + iter;
////
////        for (; d != end; ++d) {
////            const __m128i srcVector = _mm_loadu_si128(d);
////            const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask);
////            if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) {
////                // opaque, data is unchanged
////            } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) == 0xffff) {
////                // fully transparent
////                _mm_storeu_si128(d, nullVector);
////            } else {
////                __m128i alphaChannel = _mm_srli_epi32(srcVector, 24);
////                alphaChannel = _mm_or_si128(alphaChannel, _mm_slli_epi32(alphaChannel, 16));
////
////                __m128i result;
////                BYTE_MUL_SSE2(result, srcVector, alphaChannel, colorMask, half);
////                result = _mm_or_si128(_mm_andnot_si128(alphaMask, result), srcVectorAlpha);
////                _mm_storeu_si128(d, result);
////            }
////        }
////
////        QRgb *p = reinterpret_cast<QRgb*>(d);
////        QRgb *pe = p+spare;
////        for (; p != pe; ++p) {
////            if (*p < 0x00ffffff)
////                *p = 0;
////            else if (*p < 0xff000000)
////                *p = qPremultiply(*p);
////        }
////
////        d = reinterpret_cast<__m128i*>(p+pad);
////    }
////
////    //data->format = QImage::Format_ARGB32_Premultiplied;
////    setFormat(QImage::Format_ARGB32_Premultiplied);
////    //return true;
////} 
////void QMyImage::convert_ARGB_PM_to_ARGB_inplace()
////{
////    uint *buffer = reinterpret_cast<uint*>( const_cast<uchar *>(((const QMyImage*)this)->bits()) );
////    int count = byteCount()/4;
////    for (int i = 0; i < count; ++i)
////        buffer[i] = qUnpremultiply(buffer[i]);
////   setFormat(QImage::Format_ARGB32);
////}
////
/////*
////bool convert_ARGB_to_ARGB_PM_inplace_sse2(QMyImage/*Data* / *data/*, Qt::ImageConversionFlags* /)
////{
////    //Q_ASSERT(data->format() == QImage::Format_ARGB32);
////
////    // extra pixels on each line
////    const int spare = data->width() & 3;
////    // width in pixels of the pad at the end of each line
////    const int pad = (data->bytesPerLine() >> 2) - data->width();
////    const int iter = data->width() >> 2;
////    int height = data->height();
////
////    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
////    const __m128i nullVector = _mm_setzero_si128();
////    const __m128i half = _mm_set1_epi16(0x80);
////    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
////
////    __m128i *d = reinterpret_cast<__m128i*>(data->bits());
////    while (height--) {
////        const __m128i *end = d + iter;
////
////        for (; d != end; ++d) {
////            const __m128i srcVector = _mm_loadu_si128(d);
////            const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask);
////            if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) {
////                // opaque, data is unchanged
////            } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) == 0xffff) {
////                // fully transparent
////                _mm_storeu_si128(d, nullVector);
////            } else {
////                __m128i alphaChannel = _mm_srli_epi32(srcVector, 24);
////                alphaChannel = _mm_or_si128(alphaChannel, _mm_slli_epi32(alphaChannel, 16));
////
////                __m128i result;
////                BYTE_MUL_SSE2(result, srcVector, alphaChannel, colorMask, half);
////                result = _mm_or_si128(_mm_andnot_si128(alphaMask, result), srcVectorAlpha);
////                _mm_storeu_si128(d, result);
////            }
////        }
////
////        QRgb *p = reinterpret_cast<QRgb*>(d);
////        QRgb *pe = p+spare;
////        for (; p != pe; ++p) {
////            if (*p < 0x00ffffff)
////                *p = 0;
////            else if (*p < 0xff000000)
////                *p = qPremultiply(*p);
////        }
////
////        d = reinterpret_cast<__m128i*>(p+pad);
////    }
////
////    //data->format = QImage::Format_ARGB32_Premultiplied;
////    data->setFormat();
////    return true;
////} */

QImage *cVideoFile::ConvertYUVToRGB(bool PreviewMode, AVFrame *Frame)
{
   //AUTOTIMER(at,"cVideoFile::ConvertYUVToRGB");
   //LONGLONG cp = curPCounter();
   //LONGLONG cppart = cp;

   int W = Frame->width * aspectRatio;  W -= (W % 4);   // W must be a multiple of 4 ????
   int H = Frame->height;

   if (yuv2rgbImage == NULL)
      yuv2rgbImage = new QImage(W, H, QTPIXFMT);
   //LastImage = QImage(W,H,QTPIXFMT);

   // Allocate structure for RGB image
   //AVFrame *FrameBufferRGB = ALLOCFRAME();
   if (FrameBufferRGB == 0)
      FrameBufferRGB = ALLOCFRAME();

   //qDebug() << "YUV2RGB prep " << PC2time(curPCounter()-cppart,true);
   //cppart = curPCounter();
   if (FrameBufferRGB != NULL)
   {
      FrameBufferRGB->format = PIXFMT;
      av_image_fill_arrays(
         FrameBufferRGB->data,        // Buffer to prepare
         FrameBufferRGB->linesize,
         yuv2rgbImage->bits(),
         PIXFMT,                             // The format in which the picture data is stored (see http://wiki.aasimon.org/doku.php?id=Libav:pixelformat)
         W,                                  // The width of the image in pixels
         H,                                  // The height of the image in pixels
         1
      );

      //qDebug() << "YUV2RGB avpicture_fill " << PC2time(curPCounter()-cppart,true);
      //cppart = curPCounter();
         // Get a converter from libswscale
         //struct SwsContext *img_convert_ctx=sws_getContext(
         //   Frame->width,                                                     // Src width
         //   Frame->height,                                                    // Src height
         //   (PixelFormat)Frame->format,                                       // Src Format
         //   W,                                                                // Destination width
         //   H,                                                                // Destination height
         //   PIXFMT,                                                           // Destination Format
         //      SWS_BICUBIC,NULL,NULL,NULL);                                      // flags,src Filter,dst Filter,param
         //if(img_convert_ctx == 0) 
      img_convert_ctx = /*sws_getContext*/sws_getCachedContext(img_convert_ctx,
         Frame->width,                                                     // Src width
         Frame->height,                                                    // Src height
         (AVPixelFormat)Frame->format,                                       // Src Format
         W,                                                                // Destination width
         H,                                                                // Destination height
         PIXFMT,                                                           // Destination Format
         /*SWS_FAST_BILINEAR*/SWS_BICUBIC /*| SWS_ACCURATE_RND*/, NULL, NULL, NULL);                                      // flags,src Filter,dst Filter,param
//qDebug() << "YUV2RGB sws_getContext " << PC2time(curPCounter()-cppart,true);
//cppart = curPCounter();
      if (img_convert_ctx != NULL)
      {
         int ret;
         {
            //AUTOTIMER(scale,"sws_scale");
            ret = sws_scale(
               img_convert_ctx,                                           // libswscale converter
               Frame->data,                                               // Source buffer
               Frame->linesize,                                           // Source Stride ?
               0,                                                         // Source SliceY:the position in the source image of the slice to process, that is the number (counted starting from zero) in the image of the first row of the slice
               Frame->height,                                             // Source SliceH:the height of the source slice, that is the number of rows in the slice
               FrameBufferRGB->data,                                      // Destination buffer
               FrameBufferRGB->linesize                                   // Destination Stride
            );
            //qDebug() << "YUV2RGB sws_scale " << PC2time(curPCounter()-cppart,true);
            //cppart = curPCounter();
         }
         if (ret > 0)
         {
            //AUTOTIMER(scale,"assign & convert");
            bool bNeedCrop = ApplicationConfig->Crop1088To1080 && H == 1088 && W == 1920;
            bool bNeedScale = PreviewMode && Frame->height > ApplicationConfig->MaxVideoPreviewHeight;

            if (bNeedCrop && bNeedScale)
               LastImage = yuv2rgbImage->copy(0, 4, 1920, 1080).scaledToHeight(ApplicationConfig->MaxVideoPreviewHeight);//.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            else if (bNeedCrop)
               LastImage = yuv2rgbImage->copy(0, 4, 1920, 1080);//.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            else if (bNeedScale)
               LastImage = yuv2rgbImage->scaledToHeight(ApplicationConfig->MaxVideoPreviewHeight);//.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            else
            {
               if (LastImage.isNull() || LastImage.size() != yuv2rgbImage->size())
                  LastImage = QImage(yuv2rgbImage->size(), QImage::Format_ARGB32);
               memcpy(LastImage.bits(), yuv2rgbImage->bits(), yuv2rgbImage->QT_IMAGESIZE_IN_BYTES());
               //LastImage = *yuv2rgbImage;//->convertToFormat(QImage::Format_ARGB32_Premultiplied);
            }
            //// Auto crop image if 1088 format
            //if ((ApplicationConfig->Crop1088To1080) && (LastImage.height() == 1088) && (LastImage.width() == 1920))  
            //   LastImage = yuv2rgbImage->copy(0,4,1920,1080);
            //   //LastImage = LastImage.copy(0,4,1920,1080);
            //// Reduce image size for preview mode
            //if ((PreviewMode) && (LastImage.height()>ApplicationConfig->MaxVideoPreviewHeight)) 
            //   LastImage = LastImage.scaledToHeight(ApplicationConfig->MaxVideoPreviewHeight);
         }
         //sws_freeContext(img_convert_ctx);
      }

      // free FrameBufferRGB because we don't need it in the future
      //FREEFRAME(&FrameBufferRGB);
   }
#if QT_VERSION > 0x050900
   QImage* retImage = new QMyImage(LastImage);
   retImage->reinterpretAsFormat(QImage::Format_ARGB32_Premultiplied);
#else
   QMyImage *retImage = NULL;
   retImage = new QMyImage(LastImage/*.convertToFormat(QImage::Format_ARGB32_Premultiplied)*/);
   if (retImage->format() != QImage::Format_ARGB32_Premultiplied)
      QMyImage(*retImage).forcePremulFormat();
#endif
   return retImage;
}

//====================================================================================================================
// Load a video frame
// DontUseEndPos default=false
QImage *cVideoFile::ImageAt(bool PreviewMode, int64_t Position, cSoundBlockList *SoundTrackBloc, bool Deinterlace,
   double Volume, bool ForceSoundOnly, bool DontUseEndPos, int NbrDuration)
{
   if (!IsValide)
      return NULL;
   if (!IsOpen)
      OpenCodecAndFile();

   if (PreviewMode && !SoundTrackBloc)
   {
      // for speed improvment, try to find image in cache (only for interface)
      cLuLoImageCacheObject *ImageObject = ApplicationConfig->ImagesCache.FindObject(ressourceKey, fileKey, modifDateTime, imageOrientation, ApplicationConfig->Smoothing, true);
      if (!ImageObject)
         return ReadFrame(PreviewMode, Position * 1000, DontUseEndPos, Deinterlace, SoundTrackBloc, Volume, ForceSoundOnly);

      if (ImageObject->Position == Position && ImageObject->CachePreviewImage)
         return new QImage(ImageObject->CachePreviewImage->copy());

      if (ImageObject->CachePreviewImage)
      {
         delete ImageObject->CachePreviewImage;
         ImageObject->CachePreviewImage = NULL;
      }
      ImageObject->Position = Position;
      ImageObject->CachePreviewImage = ReadFrame(PreviewMode, Position * 1000, DontUseEndPos, Deinterlace, SoundTrackBloc, Volume, ForceSoundOnly, NbrDuration);
      if (ImageObject->CachePreviewImage)
         return new QImage(ImageObject->CachePreviewImage->copy());
      else
         return NULL;

   }
   else
      return ReadFrame(PreviewMode, Position * 1000, DontUseEndPos, Deinterlace, SoundTrackBloc, Volume, ForceSoundOnly);
}

AVFrame *cVideoFile::YUVAt(bool PreviewMode, int64_t Position, cSoundBlockList *SoundTrackBloc, bool Deinterlace,
   double Volume, bool ForceSoundOnly, bool DontUseEndPos, int NbrDuration)
{
   if (!IsValide)
      return NULL;
   if (!IsOpen)
      OpenCodecAndFile();

   if (PreviewMode && !SoundTrackBloc)
   {
      //// for speed improvment, try to find image in cache (only for interface)
      //cLuLoImageCacheObject *ImageObject = ApplicationConfig->ImagesCache.FindObject(ressourceKey, fileKey, modifDateTime, imageOrientation, ApplicationConfig->Smoothing, true);
      //if (!ImageObject)
      //   return ReadFrame(PreviewMode, Position * 1000, DontUseEndPos, Deinterlace, SoundTrackBloc, Volume, ForceSoundOnly);

      //if (ImageObject->Position == Position && ImageObject->CachePreviewImage)
      //   return new QImage(ImageObject->CachePreviewImage->copy());

      //if (ImageObject->CachePreviewImage)
      //{
      //   delete ImageObject->CachePreviewImage;
      //   ImageObject->CachePreviewImage = NULL;
      //}
      //ImageObject->Position = Position;
      //ImageObject->CachePreviewImage = ReadFrame(PreviewMode, Position * 1000, DontUseEndPos, Deinterlace, SoundTrackBloc, Volume, ForceSoundOnly, NbrDuration);
      //if (ImageObject->CachePreviewImage)
      //   return new QImage(ImageObject->CachePreviewImage->copy());
      //else
      return NULL;

   }
   else
      return ReadYUVFrame(PreviewMode, Position * 1000, DontUseEndPos, Deinterlace, SoundTrackBloc, Volume, ForceSoundOnly);
}

//====================================================================================================================

int cVideoFile::getThreadFlags(AVCodecID ID) {
   int Ret = 0;
   switch (ID)
   {
      case AV_CODEC_ID_PRORES:
      case AV_CODEC_ID_MPEG1VIDEO:
      case AV_CODEC_ID_DVVIDEO:
      case AV_CODEC_ID_MPEG2VIDEO:   Ret = FF_THREAD_SLICE;                    break;
      case AV_CODEC_ID_H264:        Ret = FF_THREAD_FRAME | FF_THREAD_SLICE;    break;
      default:                       Ret = FF_THREAD_FRAME;                    break;
   }
   return Ret;
}

//====================================================================================================================

bool cVideoFile::OpenCodecAndFile()
{
   //qDebug() << "open video";
   //getMemInfo();
   QMutexLocker locker(&accessMutex);

   // Ensure file was previously checked
   if (!IsValide)
      return false;
   if (!IsInformationValide)
      GetFullInformationFromFile();

   if (IsOpen)
      return true;
   // Clean memory if a previous file was loaded
   locker.unlock();
   CloseCodecAndFile(); // could this happen???
   locker.relock();

   //**********************************
   // Open audio stream
   //**********************************
   if (AudioStreamNumber != -1)
   {

      // Open the file and get a LibAVFormat context and an associated LibAVCodec decoder
      LibavAudioFile = avformat_alloc_context();
      LibavAudioFile->flags |= AVFMT_FLAG_GENPTS;       // Generate missing pts even if it requires parsing future NbrFrames.
      if (avformat_open_input(&LibavAudioFile, FileName().toLocal8Bit(), NULL, NULL) != 0)
         return false;
      if (avformat_find_stream_info(LibavAudioFile, NULL) < 0)
      {
         avformat_close_input(&LibavAudioFile);
         return false;
      }

      AVStream *AudioStream = LibavAudioFile->streams[AudioStreamNumber];

      // Setup STREAM options
      AudioStream->discard = AVDISCARD_DEFAULT;

      // Find the decoder for the audio stream and open it
      AudioDecoderCodec = avcodec_find_decoder(AudioStream->codecpar->codec_id);

      // Setup decoder options
      AVDictionary * av_opts = NULL;
      #if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
      AudioStream->codec->debug_mv = 0;                    // Debug level (0=nothing)
      AudioStream->codec->debug = 0;                    // Debug level (0=nothing)
      AudioStream->codec->workaround_bugs = 1;                    // Work around bugs in encoders which sometimes cannot be detected automatically : 1=autodetection
      AudioStream->codec->idct_algo = FF_IDCT_AUTO;         // IDCT algorithm, 0=auto
      AudioStream->codec->skip_frame = AVDISCARD_DEFAULT;    // ???????
      AudioStream->codec->skip_idct = AVDISCARD_DEFAULT;    // ???????
      AudioStream->codec->skip_loop_filter = AVDISCARD_DEFAULT;    // ???????
      AudioStream->codec->error_concealment = 3;
      AudioStream->codec->thread_count = getCpuCount();
      AudioStream->codec->thread_type = getThreadFlags(AudioStream->codec->codec_id);
      #else
      av_dict_set(&av_opts, "thread_count", QString("%1").arg(getCpuCount()).toLocal8Bit().constData(), 0);
      #endif
      AudioCodecContext = avcodec_alloc_context3(NULL);
      /*ret = */avcodec_parameters_to_context(AudioCodecContext, LibavAudioFile->streams[AudioStreamNumber]->codecpar);
      if ((AudioDecoderCodec == NULL) || (avcodec_open2(AudioCodecContext, AudioDecoderCodec, &av_opts) < 0))
      {
         //Mutex.unlock();
         av_dict_free(&av_opts);
         return false;
      }
      av_dict_free(&av_opts);
      IsVorbis = (strcmp(AudioDecoderCodec->name, "vorbis") == 0);

      if (LibavAudioFile->start_time != AV_NOPTS_VALUE)
         LibavStartTime = abs(LibavAudioFile->start_time);
      else
         LibavStartTime = 0;
      AVStartTime = QTime(0, 0, 0, 0).addMSecs((LibavStartTime * 1000) / AV_TIME_BASE);
      //qDebug() << "AVSTartTime " << AVStartTime.toString("hh:mm:ss.zzz");
      //AVStartTime    = QTime(0,0,0,0);
      //LibavStartTime = 0;
   }

   //**********************************
   // Open video stream
   //**********************************
   if ((VideoStreamNumber != -1) && (!MusicOnly))
   {
      // Open the file and get a LibAVFormat context and an associated LibAVCodec decoder
      LibavVideoFile = avformat_alloc_context();
      LibavVideoFile->flags |= AVFMT_FLAG_GENPTS;       // Generate missing pts even if it requires parsing future NbrFrames.
      if (avformat_open_input(&LibavVideoFile, FileName().toLocal8Bit(), NULL, NULL) != 0)
         return false;
      if (avformat_find_stream_info(LibavVideoFile, NULL) < 0)
      {
         avformat_close_input(&LibavVideoFile);
         return false;
      }

      AVStream *VideoStream = LibavVideoFile->streams[VideoStreamNumber];

      // Setup STREAM options
      VideoStream->discard = AVDISCARD_DEFAULT;

      // Find the decoder for the video stream and open it
      VideoDecoderCodec = avcodec_find_decoder(VideoStream->codecpar->codec_id);
#ifdef CUDA_MEMCHECK
      size_t totalMem, freeMem;

      cudaMemGetInfo(&freeMem, &totalMem);
      qDebug() << QString("%1 bytes mem free of %2 bytes mem total").arg(freeMem).arg(totalMem);
#endif
#ifdef ENABLE_FFMPEG_CUDA
      if (pAppConfig->enableCUVID)
      {
         int i = 0;
         bool found = false;
         QString s = cuvid_mappings[i];
         QString cuvidDecoderName;
         while (s.length() && !found)
         {
            if (s.startsWith(VideoDecoderCodec->name))
            {
               cuvidDecoderName = s.right(s.length() - s.indexOf("|") - 1);
               found = true;
            }
            s = cuvid_mappings[++i];
         }
         if (found)
         {
            const AVCodec *cuvidVideoDecoderCodec = avcodec_find_decoder_by_name(cuvidDecoderName.toLocal8Bit().constData());
            if (cuvidVideoDecoderCodec != 0)
            {
               isCUVIDDecoder = true;
               VideoDecoderCodec = cuvidVideoDecoderCodec;
            }
         }
      }
      qDebug() << "isCUVIDDecoder " << isCUVIDDecoder;
      if (pAppConfig->enableQSVDec)
      {
         int i = 0;
         bool found = false;
         QString s = qsv_mappings[i];
         QString qsvDecoderName;
         while (s.length() && !found)
         {
            if (s.startsWith(VideoDecoderCodec->name))
            {
               qsvDecoderName = s.right(s.length() - s.indexOf("|") - 1);
               found = true;
            }
            s = qsv_mappings[++i];
         }
         if (found)
         {
            const AVCodec *qsvVideoDecoderCodec = avcodec_find_decoder_by_name(qsvDecoderName.toLocal8Bit().constData());
            if (qsvVideoDecoderCodec != 0)
            {
               isQSVDecoder = true;
               VideoDecoderCodec = qsvVideoDecoderCodec;
            }
         }
      }
      qDebug() << "isqsvDecoder " << isQSVDecoder;
#endif
      VideoCodecContext = avcodec_alloc_context3(VideoDecoderCodec);
      /*ret = */avcodec_parameters_to_context(VideoCodecContext, LibavVideoFile->streams[VideoStreamNumber]->codecpar);

      // Setup decoder options
      AVDictionary * av_opts = NULL;
      #if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100)
      VideoStream->codec->debug_mv = 0;                    // Debug level (0=nothing)
      VideoStream->codec->debug = 0;                    // Debug level (0=nothing)
      VideoStream->codec->workaround_bugs = 1;                    // Work around bugs in encoders which sometimes cannot be detected automatically : 1=autodetection
      VideoStream->codec->idct_algo = FF_IDCT_AUTO;         // IDCT algorithm, 0=auto
      VideoStream->codec->skip_frame = AVDISCARD_DEFAULT;    // ???????
      VideoStream->codec->skip_idct = AVDISCARD_DEFAULT;    // ???????
      VideoStream->codec->skip_loop_filter = AVDISCARD_DEFAULT;    // ???????
      VideoStream->codec->error_concealment = 3;
      VideoStream->codec->thread_count = getCpuCount();
      VideoStream->codec->thread_type = getThreadFlags(VideoStream->codec->codec_id);
      #else
      if (useDeinterlace)
      {
         av_dict_set(&av_opts, "deint", QString("%1").arg(1).toLocal8Bit().constData(), 0); // BOB deinterlace
         av_dict_set(&av_opts, "drop_second_field", QString("%1").arg(1).toLocal8Bit().constData(), 0); 
      }
      #endif
      // Hack to correct wrong frame rates that seem to be generated by some codecs
      if (VideoCodecContext->time_base.num > 1000 && VideoCodecContext->time_base.den == 1)
         VideoCodecContext->time_base.den = 1000;

      if ((VideoDecoderCodec == NULL) || (avcodec_open2(VideoCodecContext, VideoDecoderCodec, &av_opts) < 0))
      {
         av_dict_free(&av_opts);
         return false;
      }
#ifdef CUDA_MEMCHECK
      size_t totalMem2, freeMem2;
      cudaMemGetInfo(&freeMem2, &totalMem2);
      qDebug() << QString("%1 bytes mem free of %2 bytes mem total, used %3 (%4 MB)\n").arg(freeMem2).arg(totalMem2).arg(freeMem-freeMem2).arg((freeMem - freeMem2)/(1024*1024));
#endif
      AVDictionaryEntry *t = NULL;
      while (t = av_dict_get(av_opts, "", t, AV_DICT_IGNORE_SUFFIX))
      {
         qDebug() << "opt " << t->key << " " << t->value;
      }
      av_dict_free(&av_opts);
      if (LibavVideoFile->start_time != AV_NOPTS_VALUE)
         LibavStartTime = abs(LibavVideoFile->start_time);
      else
         LibavStartTime = 0;
      AVStartTime = QTime(0, 0, 0, 0).addMSecs((LibavStartTime * 1000) / AV_TIME_BASE);
      //qDebug() << "AVSTartTime " << AVStartTime.toString("hh:mm:ss.zzz");
   }
   IsOpen = true;
   //getMemInfo();
   return IsOpen;
}

//*********************************************************************************************************************************************
// Base object for music definition
//*********************************************************************************************************************************************

cMusicObject::cMusicObject(cApplicationConfig *ApplicationConfig) :cVideoFile(ApplicationConfig)
{
   Volume = -1;                            // Volume as % from 1% to 150% or -1=auto
   AllowCredit = true;                          // // if true, this music will appear in credit title
   ForceFadIn = 0;
   ForceFadOut = 0;
   Reset(OBJECTTYPE_MUSICFILE);
   startCode = eDefault_Start;
   startOffset = 0;
}

//====================================================================================================================
// Overloaded function use to dertermine if format of media file is correct
bool cMusicObject::CheckFormatValide(QWidget *Window)
{
   bool IsOk = IsValide;

   // try to open file
   if (!OpenCodecAndFile())
   {
      QString ErrorMessage = QApplication::translate("MainWindow", "Format not supported", "Error message");
      CustomMessageBox(Window, QMessageBox::Critical, QApplication::translate("MainWindow", "Error", "Error message"), ShortName() + "\n\n" + ErrorMessage, QMessageBox::Close);
      IsOk = false;
   }

   // check if file have at least one sound track compatible
   if ((IsOk) && (AudioStreamNumber == -1))
   {
      QString ErrorMessage = QApplication::translate("MainWindow", "No audio track found", "Error message") + "\n";
      CustomMessageBox(Window, QMessageBox::Critical, QApplication::translate("MainWindow", "Error", "Error message"), ShortName() + "\n\n" + ErrorMessage, QMessageBox::Close);
      IsOk = false;

   }
   else
   {
      if (!((LibavAudioFile->streams[AudioStreamNumber]->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_S16) || (LibavAudioFile->streams[AudioStreamNumber]->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_U8)))
      {
         QString ErrorMessage = QApplication::translate("MainWindow", "This application support only audio track with unsigned 8 bits or signed 16 bits sample format", "Error message") + "\n";
         CustomMessageBox(Window, QMessageBox::Critical, QApplication::translate("MainWindow", "Error", "Error message"), ShortName() + "\n\n" + ErrorMessage, QMessageBox::Close);
         IsOk = false;
      }
   }
   // close file if it was opened
   CloseCodecAndFile();

   return IsOk;
}

//====================================================================================================================
void cMusicObject::SaveToXML(QDomElement *ParentElement, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel)
{
   QDomDocument DomDocument;
   QString xmlFragment;
   ExXmlStreamWriter xlmStream(&xmlFragment);
   SaveToXMLex(xlmStream, ElementName, PathForRelativPath, ForceAbsolutPath, ReplaceList, ResKeyList, IsModel);
   DomDocument.setContent(xmlFragment);
   ParentElement->appendChild(DomDocument.firstChildElement());
   return;
   /*
   QDomDocument    DomDocument;
    QDomElement     Element=DomDocument.createElement(ElementName);
    QString         TheFileName;

    if (ReplaceList) {
        TheFileName=ReplaceList->GetDestinationFileName(FileName());
    } else if (PathForRelativPath!="") {
        if (ForceAbsolutPath) TheFileName=QDir(QFileInfo(PathForRelativPath).absolutePath()).absoluteFilePath(FileName());
            else TheFileName=QDir(QFileInfo(PathForRelativPath).absolutePath()).relativeFilePath(FileName());
    } else TheFileName=FileName();

    Element.setAttribute("FilePath",     TheFileName);
    Element.setAttribute("iStartPos",    QTime(0,0,0,0).msecsTo(StartTime));
    Element.setAttribute("iEndPos",      QTime(0,0,0,0).msecsTo(EndTime));
    Element.setAttribute("Volume",       QString("%1").arg(Volume,0,'f'));
    Element.setAttribute("AllowCredit",  AllowCredit?"1":"0");
    Element.setAttribute("ForceFadIn",   qlonglong(ForceFadIn));
    Element.setAttribute("ForceFadOut",  qlonglong(ForceFadOut));
    Element.setAttribute("GivenDuration",QTime(0,0,0,0).msecsTo(GetGivenDuration()));
    if (IsComputedAudioDuration) {
        Element.setAttribute("RealAudioDuration",      QTime(0,0,0,0).msecsTo(GetRealAudioDuration()));
        Element.setAttribute("IsComputedAudioDuration",IsComputedAudioDuration?"1":0);
        Element.setAttribute("SoundLevel",             QString("%1").arg(SoundLevel,0,'f'));
    }
    ParentElement->appendChild(Element);
    */
}
void cMusicObject::SaveToXMLex(ExXmlStreamWriter &xmlStream, QString ElementName, QString PathForRelativPath, bool ForceAbsolutPath, cReplaceObjectList *ReplaceList, QList<qlonglong> *ResKeyList, bool IsModel)
{
   xmlStream.writeStartElement(ElementName);
   QString         TheFileName;

   if (ReplaceList)
   {
      TheFileName = ReplaceList->GetDestinationFileName(FileName());
   }
   else if (PathForRelativPath != "")
   {
      if (ForceAbsolutPath)
         TheFileName = QDir(QFileInfo(PathForRelativPath).absolutePath()).absoluteFilePath(FileName());
      else
         TheFileName = QDir(QFileInfo(PathForRelativPath).absolutePath()).relativeFilePath(FileName());
   }
   else
      TheFileName = FileName();

   xmlStream.writeAttribute("FilePath", TheFileName);
   xmlStream.writeAttribute("iStartPos", QTime(0, 0, 0, 0).msecsTo(StartTime));
   xmlStream.writeAttribute("iEndPos", QTime(0, 0, 0, 0).msecsTo(EndTime));
   xmlStream.writeAttribute("Volume", QString("%1").arg(Volume, 0, 'f'));
   xmlStream.writeAttribute("AllowCredit", AllowCredit);
   xmlStream.writeAttribute("ForceFadIn", qlonglong(ForceFadIn));
   xmlStream.writeAttribute("ForceFadOut", qlonglong(ForceFadOut));
   xmlStream.writeAttribute("GivenDuration", QTime(0, 0, 0, 0).msecsTo(GetGivenDuration()));
   if (IsComputedAudioDuration)
   {
      xmlStream.writeAttribute("RealAudioDuration", QTime(0, 0, 0, 0).msecsTo(GetRealAudioDuration()));
      xmlStream.writeAttribute("IsComputedAudioDuration", IsComputedAudioDuration);
      xmlStream.writeAttribute("SoundLevel", QString("%1").arg(SoundLevel, 0, 'f'));
   }
   xmlStream.writeEndElement();
}

//====================================================================================================================

bool cMusicObject::LoadFromXML(QDomElement *ParentElement, QString ElementName, QString PathForRelativPath, QStringList *AliasList, bool *ModifyFlag) {
   if ((ParentElement->elementsByTagName(ElementName).length() > 0) && (ParentElement->elementsByTagName(ElementName).item(0).isElement() == true))
   {
      QDomElement Element = ParentElement->elementsByTagName(ElementName).item(0).toElement();

      QString FileName = Element.attribute("FilePath", "");
      if ((!QFileInfo(FileName).exists()) && (PathForRelativPath != ""))
      {
         FileName = QDir::cleanPath(QDir(PathForRelativPath).absoluteFilePath(FileName));
         // Fixes a previous bug in relative path
         #ifndef Q_OS_WIN
         if (FileName.startsWith("/.."))
         {
            if (FileName.contains("/home/")) FileName = FileName.mid(FileName.indexOf("/home/"));
            if (FileName.contains("/mnt/"))  FileName = FileName.mid(FileName.indexOf("/mnt/"));
         }
         #endif
      }
      if (GetInformationFromFile(FileName, AliasList, ModifyFlag) && (CheckFormatValide(NULL)))
      {
         // Old format prior to ffDiaporama 2.2.2014.0308
         if (Element.hasAttribute("StartPos")) StartTime = QTime().fromString(Element.attribute("StartPos"));
         if (Element.hasAttribute("EndPos"))   EndTime = QTime().fromString(Element.attribute("EndPos"));
         // New format since ffDiaporama 2.2.2014.0308
         if (Element.hasAttribute("iStartPos")) StartTime = QTime(0, 0, 0, 0).addMSecs(Element.attribute("iStartPos").toLongLong());
         if (Element.hasAttribute("iEndPos"))   EndTime = QTime(0, 0, 0, 0).addMSecs(Element.attribute("iEndPos").toLongLong());

         if (Element.hasAttribute("Volume"))                  Volume = GetDoubleValue(Element, "Volume");
         if (Element.hasAttribute("GivenDuration"))           SetGivenDuration(QTime(0, 0, 0, 0).addMSecs(Element.attribute("GivenDuration").toLongLong()));
         if (Element.hasAttribute("IsComputedDuration"))      IsComputedAudioDuration = Element.attribute("IsComputedDuration") == "1";
         if (Element.hasAttribute("IsComputedAudioDuration")) IsComputedAudioDuration = Element.attribute("IsComputedAudioDuration") == "1";
         if (Element.hasAttribute("RealDuration"))            SetRealAudioDuration(QTime(0, 0, 0, 0).addMSecs(Element.attribute("RealDuration").toLongLong()));
         if (Element.hasAttribute("RealAudioDuration"))       SetRealAudioDuration(QTime(0, 0, 0, 0).addMSecs(Element.attribute("RealAudioDuration").toLongLong()));
         if (Element.hasAttribute("SoundLevel"))              SoundLevel = GetDoubleValue(Element, "SoundLevel");
         if (Element.hasAttribute("AllowCredit"))        AllowCredit = Element.attribute("AllowCredit") == "1";
         if (Element.hasAttribute("ForceFadIn"))         ForceFadIn = Element.attribute("ForceFadIn").toLongLong();
         if (Element.hasAttribute("ForceFadOut"))        ForceFadOut = Element.attribute("ForceFadOut").toLongLong();
         return true;
      }
      else return false;
   }
   else return false;
}

//====================================================================================================================

QTime cMusicObject::GetDuration() {
   return EndTime.addMSecs(-QTime(0, 0, 0, 0).msecsTo(StartTime));
}

//====================================================================================================================

qreal cMusicObject::GetFading(int64_t Position, bool SlideHaveFadIn, bool SlideHaveFadOut)
{
   int64_t RealFadIN = ForceFadIn;
   int64_t RealFadOUT = ForceFadOut;
   int64_t Duration = QTime(0, 0, 0, 0).msecsTo(GetDuration());
   qreal   RealVolume = Volume;
   // If fade duration longer than duration, then reduce them
   if (Duration < (RealFadIN + RealFadOUT))
   {
      qreal Ratio = qreal(RealFadIN + RealFadOUT) / qreal(Duration);
      RealFadIN = int64_t(qreal(RealFadIN) / Ratio);
      RealFadOUT = int64_t(qreal(RealFadOUT) / Ratio);
   }
   if (RealVolume == -1)
      RealVolume = 1;
   if ((!SlideHaveFadIn) && (Position < RealFadIN))
   {
      qreal PCTDone = ComputePCT(SPEEDWAVE_SINQUARTER, double(Position) / double(RealFadIN));
      RealVolume = RealVolume * PCTDone;
   }
   if ((!SlideHaveFadOut) && (Position > (Duration - RealFadOUT)))
   {
      if (RealFadOUT > 0)
      {
         qreal PCTDone = ComputePCT(SPEEDWAVE_SINQUARTER, double(Position - (Duration - RealFadOUT)) / double(RealFadOUT));
         RealVolume = RealVolume * (1 - PCTDone);
      }
      else
         RealVolume = 0;
   }
   if (RealVolume < 0)
      RealVolume = 0;
   //if (RealVolume == -1) 
   //   RealVolume = 1;
   return RealVolume;
}


// Remark: Position must use AV_TIMEBASE Unit
QImage *cMusicObject::ReadFrame(bool PreviewMode, int64_t Position, bool DontUseEndPos, bool /*Deinterlace*/, cSoundBlockList *SoundTrackBloc, double Volume, bool /*ForceSoundOnly*/, int NbrDuration)
{
   //qDebug() << "cMusicObject::ReadFrame PreviewMode " << PreviewMode << " Position " << Position << " DontUseEndPos " << DontUseEndPos << " Volume " << Volume << " NbrDuration " << NbrDuration;
   // Ensure file was previously open
   if (!IsOpen && !OpenCodecAndFile())
      return NULL;

   double dEndFile = double(QTime(0, 0, 0, 0).msecsTo(DontUseEndPos ? GetRealDuration() : EndTime)) / 1000.0;
   Position += LibavStartTime;///AV_TIME_BASE;
   dEndFile += double(QTime(0, 0).msecsTo(AVStartTime)) / 1000.0;//LibavStartTime/AV_TIME_BASE;
   if (dEndFile == 0)
   {
      ToLog(LOGMSG_CRITICAL, "Error in cVideoFile::ReadFrame : dEndFile=0 ?????");
      return NULL;
   }
   if (Position < 0)
      Position = 0;

   cVideoFile::sAudioContext AudioContext;
   AudioContext.SoundTrackBloc = SoundTrackBloc;
   AudioContext.AudioStream = (AudioStreamNumber >= 0 && SoundTrackBloc) ? LibavAudioFile->streams[AudioStreamNumber] : NULL;
   AudioContext.CodecContext = AudioCodecContext;
   AudioContext.FPSSize = SoundTrackBloc ? SoundTrackBloc->SoundPacketSize*SoundTrackBloc->NbrPacketForFPS : 0;
   AudioContext.FPSDuration = AudioContext.FPSSize ? (double(AudioContext.FPSSize) / (SoundTrackBloc->Channels*SoundTrackBloc->SampleBytes*SoundTrackBloc->SamplingRate))*AV_TIME_BASE : 0;
   AudioContext.TimeBase = AudioContext.AudioStream ? double(AudioContext.AudioStream->time_base.den) / double(AudioContext.AudioStream->time_base.num) : 0;
   AudioContext.DstSampleSize = SoundTrackBloc ? (SoundTrackBloc->SampleBytes*SoundTrackBloc->Channels) : 0;
   AudioContext.NeedResampling = false;
   AudioContext.AudioLenDecoded = 0;
   AudioContext.Counter = 20; // Retry counter (when len>0 and avcodec_decode_audio4 fail to retreave frame, we retry counter time before to discard the packet)
   AudioContext.Volume = Volume;
   AudioContext.dEndFile = &dEndFile;
   AudioContext.NbrDuration = NbrDuration;
   AudioContext.DontUseEndPos = DontUseEndPos;
   if (!AudioContext.AudioStream)
      return NULL;

   if (!AudioContext.FPSDuration)
   {
      if (PreviewMode)
         AudioContext.FPSDuration = double(AV_TIME_BASE) / ApplicationConfig->PreviewFPS;
      else
         AudioContext.FPSDuration = double(AV_TIME_BASE) / double(SoundTrackBloc->SamplingRate);
   }


   Mutex.lock();

   // If position >= end of file : disable audio (only if IsComputedAudioDuration)
   double dPosition = double(Position) / AV_TIME_BASE;
   if (dPosition > 0 && dPosition >= dEndFile - 0.1 && IsComputedAudioDuration)
   {
      AudioContext.AudioStream = NULL; // Disable audio

      // If not then change Position to end file - a FPS to prepare a last image
      Position = dEndFile * AV_TIME_BASE - AudioContext.FPSDuration;
      dPosition = double(Position) / AV_TIME_BASE;
      if (SoundTrackBloc)
         SoundTrackBloc->UseLatestData();
   }

   AudioContext.ContinueAudio = (AudioContext.AudioStream) && (SoundTrackBloc);
   bool ResamplingContinue = (Position != 0);
   AudioContext.AudioFramePosition = dPosition;

   if (AudioContext.ContinueAudio)
   {
      AudioContext.NeedResampling = ((AudioContext.AudioStream->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT != AV_SAMPLE_FMT_S16)
         || (AudioContext.AudioStream->CODEC_OR_PAR->CC_CHANNELS != SoundTrackBloc->Channels)
         || (AudioContext.AudioStream->CODEC_OR_PAR->sample_rate != SoundTrackBloc->SamplingRate));

      //qDebug() << "needResampling " << AudioContext.NeedResampling;
      // Calc if we need to seek to a position
      int64_t Start = /*SoundTrackBloc->LastReadPosition;*/SoundTrackBloc->CurrentPosition;
      int64_t End = Start + SoundTrackBloc->GetDuration();
      int64_t Wanted = AudioContext.FPSDuration * AudioContext.NbrDuration;
      //qDebug() << "Start " << Start << " End " << End << " Position " << Position << " Wanted " << Wanted;

      // ?????
      if ((Position >= Start && Position + Wanted <= End) /*|| Start < 0 */)
         AudioContext.ContinueAudio = false;

      if (AudioContext.ContinueAudio && (Position == 0 || Start < 0 || LastAudioReadPosition < 0 /*|| Position < Start*/ || Position > End + 1500000))
      {
         if (Position < 0)
            Position = 0;
         SoundTrackBloc->ClearList();                // Clear soundtrack list
         ResamplingContinue = false;
         LastAudioReadPosition = 0;
         SeekErrorCount = 0;
         int64_t seekPos = Position - SoundTrackBloc->WantedDuration * 1000.0;
         /*bool bSeekRet = */SeekFile(NULL, AudioContext.AudioStream, seekPos);        // Always seek one FPS before to ensure eventual filter have time to init
         //qDebug() << "seek to " << seekPos << " is " << (bSeekRet?"true":"false");
         AudioContext.AudioFramePosition = Position / AV_TIME_BASE;
      }

      // Prepare resampler
      if (AudioContext.ContinueAudio && AudioContext.NeedResampling)
      {
         if (!ResamplingContinue)
            CloseResampler();
         AVChannelLayout avcl;
         av_channel_layout_default(&avcl, SoundTrackBloc->Channels);
         CheckResampler(AudioContext.AudioStream->CODEC_OR_PAR->CC_CHANNELS, SoundTrackBloc->Channels,
            AVSampleFormat(AudioContext.AudioStream->CODEC_OR_PAR->CODEC_SAMPLE_FORMAT), SoundTrackBloc->SampleFormat,
            AudioContext.AudioStream->CODEC_OR_PAR->sample_rate, SoundTrackBloc->SamplingRate
            , AudioContext.AudioStream->CODEC_OR_PAR->ch_layout 
            , avcl 
         );
      }
   }

   //*************************************************************************************************************************************
   // Decoding process : Get StreamPackets until endposition is reached (if sound is wanted) 
   //*************************************************************************************************************************************
   while (AudioContext.ContinueAudio)
   {
       AVPacket *StreamPacket = getNewPacket();//new AVPacket();
      if (!StreamPacket)
      {
         AudioContext.ContinueAudio = false;
      }
      else
      {
         StreamPacket->flags |= AV_PKT_FLAG_KEY;
         int err;
         if ((err = av_read_frame(LibavAudioFile, StreamPacket)) < 0)
         {
            //qDebug() << "av_read_frame gives " << err;
            // If error reading frame then we considere we have reach the end of file
            if (!IsComputedAudioDuration)
            {
               dEndFile = qreal(SoundTrackBloc->/*LastReadPosition*/CurrentPosition) / AV_TIME_BASE;
               dEndFile += qreal(SoundTrackBloc->GetDuration()) / 1000;
               if (dEndFile - LibavStartTime / AV_TIME_BASE == double(QTime(0, 0, 0, 0).msecsTo(EndTime)) / 1000)
                  EndTime = QTime(0, 0, 0).addMSecs((dEndFile - LibavStartTime) * 1000);
               SetRealAudioDuration(QTime(0, 0, 0, 0).addMSecs(qlonglong((dEndFile - LibavStartTime / AV_TIME_BASE) * 1000)));
            }
            AudioContext.ContinueAudio = false;
            //qDebug() << "cVideoFile::ReadFrame audio, set AudioContext.ContinueAudio to false!!!!! err is "<<err;

            // Use data in TempData to create a latest block
            SoundTrackBloc->UseLatestData();
         }
         else
         {
            DecodeAudio(&AudioContext, StreamPacket, Position);
            StreamPacket = NULL;
         }
      }
      // Continue with a new one
      if (StreamPacket != NULL)
      {
         deletePacket(&StreamPacket);
      }
   }

   Mutex.unlock();
   return NULL;
}
#endif

